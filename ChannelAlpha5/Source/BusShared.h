#pragma once
#include <JuceHeader.h>

#if JUCE_WINDOWS
#include <windows.h>
#endif

// Per-channel ring buffer for mixing
struct ChannelRingBuffer
{
    static constexpr int bufferSizeSamples = 262144; // ~5.5 seconds at 48kHz

    std::atomic<int> writePos{ 0 };
    std::atomic<int> readPos{ 0 };
    float leftChannel[bufferSizeSamples];
    float rightChannel[bufferSizeSamples];
    std::atomic<int> activeWriters{ 0 }; // Count of Channel Alpha instances writing
    std::atomic<int64_t> totalWritten{ 0 };
    std::atomic<int64_t> totalRead{ 0 };
};

// Shared memory structure for all 32 channels
struct BusSharedMemory
{
    ChannelRingBuffer channels[32]; // Channels 1-32 (index 0-31)
};

class BusShared
{
public:
    static BusShared& getInstance()
    {
        static BusShared instance;
        return instance;
    }

    bool isInitialized() const { return sharedBuffer != nullptr; }

    // Called by Channel Alpha 5 to write to a specific channel
    void writeToChannel(int channelID, const float* left, const float* right, int numSamples) noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return;

        int channelIndex = channelID - 1;
        auto& channel = sharedBuffer->channels[channelIndex];

        const int mask = ChannelRingBuffer::bufferSizeSamples - 1;
        int writeIndex = channel.writePos.load(std::memory_order_acquire);

        // Direct write (like Loopback - no clearing needed)
        for (int i = 0; i < numSamples; ++i)
        {
            int idx = (writeIndex + i) & mask;

            float leftSample = left[i];
            float rightSample = right ? right[i] : left[i];

            channel.leftChannel[idx] = leftSample;
            channel.rightChannel[idx] = rightSample;
        }

        channel.writePos.store((writeIndex + numSamples) & mask, std::memory_order_release);
        channel.totalWritten.fetch_add(numSamples, std::memory_order_relaxed);
    }

    // Called by Bus Alpha 5 to read from a specific channel  
    void readFromChannel(int channelID, float* left, float* right, int numSamples) noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32)
        {
            juce::FloatVectorOperations::clear(left, numSamples);
            if (right) juce::FloatVectorOperations::clear(right, numSamples);
            return;
        }

        int channelIndex = channelID - 1;
        auto& channel = sharedBuffer->channels[channelIndex];

        const int mask = ChannelRingBuffer::bufferSizeSamples - 1;
        int writeIndex = channel.writePos.load(std::memory_order_acquire);
        int readIndex = channel.readPos.load(std::memory_order_acquire);

        // Calculate available samples (like Loopback)
        int available = (writeIndex - readIndex) & mask;

        // If no data, output silence
        if (available < numSamples)
        {
            juce::FloatVectorOperations::clear(left, numSamples);
            if (right) juce::FloatVectorOperations::clear(right, numSamples);
            return;
        }

        // Read samples (NO CLEARING - like Loopback!)
        for (int i = 0; i < numSamples; ++i)
        {
            int idx = (readIndex + i) & mask;

            left[i] = channel.leftChannel[idx];
            if (right) right[i] = channel.rightChannel[idx];

            // NO CLEARING! Data stays in buffer
        }

        // Update read position
        channel.readPos.store((readIndex + numSamples) & mask, std::memory_order_release);
        channel.totalRead.fetch_add(numSamples, std::memory_order_relaxed);
    }

    // Register/unregister writers
    void registerWriter(int channelID) noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return;
        int result = sharedBuffer->channels[channelID - 1].activeWriters.fetch_add(1, std::memory_order_relaxed);
        DBG("BusShared: Registered writer for channel " << channelID << " (now " << (result + 1) << " writers)");
    }

    void unregisterWriter(int channelID) noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return;
        int result = sharedBuffer->channels[channelID - 1].activeWriters.fetch_sub(1, std::memory_order_relaxed);
        DBG("BusShared: Unregistered writer for channel " << channelID << " (now " << (result - 1) << " writers)");
    }

    int getActiveWriters(int channelID) const noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return 0;
        return sharedBuffer->channels[channelID - 1].activeWriters.load(std::memory_order_relaxed);
    }

    int getNumAvailable(int channelID) const noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return 0;

        auto& channel = sharedBuffer->channels[channelID - 1];
        int writeIndex = channel.writePos.load(std::memory_order_acquire);
        int readIndex = channel.readPos.load(std::memory_order_acquire);
        return (writeIndex - readIndex) & (ChannelRingBuffer::bufferSizeSamples - 1);
    }

    int64_t getTotalWritten(int channelID) const noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return 0;
        return sharedBuffer->channels[channelID - 1].totalWritten.load(std::memory_order_relaxed);
    }

    int64_t getTotalRead(int channelID) const noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return 0;
        return sharedBuffer->channels[channelID - 1].totalRead.load(std::memory_order_relaxed);
    }

    void clearChannel(int channelID) noexcept
    {
        if (!sharedBuffer || channelID < 1 || channelID > 32) return;

        auto& channel = sharedBuffer->channels[channelID - 1];

        for (int i = 0; i < ChannelRingBuffer::bufferSizeSamples; ++i)
        {
            channel.leftChannel[i] = 0.0f;
            channel.rightChannel[i] = 0.0f;
        }

        channel.writePos.store(0, std::memory_order_release);
        channel.readPos.store(0, std::memory_order_release);
    }

private:
    BusShared()
    {
#if JUCE_WINDOWS
        const char* mapName = "Global\\BusAlpha5SharedMemory_V2";  // Changed version

        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(BusSharedMemory),
            mapName
        );

        if (hMapFile == NULL)
        {
            DBG("BusShared: FAILED to create file mapping - Error: " << GetLastError());
            return;
        }

        bool existed = (GetLastError() == ERROR_ALREADY_EXISTS);

        sharedBuffer = (BusSharedMemory*)MapViewOfFile(
            hMapFile,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            sizeof(BusSharedMemory)
        );

        if (sharedBuffer == NULL)
        {
            DBG("BusShared: FAILED to map view of file - Error: " << GetLastError());
            CloseHandle(hMapFile);
            hMapFile = NULL;
            return;
        }

        if (!existed)
        {
            memset(sharedBuffer, 0, sizeof(BusSharedMemory));
            DBG("BusShared: ✅ Created NEW shared memory (Size: " << sizeof(BusSharedMemory) << " bytes = " << (sizeof(BusSharedMemory) / 1024 / 1024) << " MB)");
        }
        else
        {
            DBG("BusShared: ✅ Opened EXISTING shared memory");
        }
#endif
    }

    ~BusShared()
    {
#if JUCE_WINDOWS
        if (sharedBuffer)
        {
            UnmapViewOfFile(sharedBuffer);
            sharedBuffer = nullptr;
        }
        if (hMapFile)
        {
            CloseHandle(hMapFile);
            hMapFile = NULL;
        }
        DBG("BusShared: Cleaned up shared memory");
#endif
    }

    BusShared(const BusShared&) = delete;
    BusShared& operator=(const BusShared&) = delete;

#if JUCE_WINDOWS
    HANDLE hMapFile = NULL;
#endif
    BusSharedMemory* sharedBuffer = nullptr;
};