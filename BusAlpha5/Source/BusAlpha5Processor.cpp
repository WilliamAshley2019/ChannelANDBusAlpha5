#include "BusAlpha5Processor.h"
#include "BusAlpha5Editor.h"

BusAlpha5Processor::BusAlpha5Processor()
    : AudioProcessor(BusesProperties()
        // NO INPUT - Generator/Instrument only!
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

BusAlpha5Processor::~BusAlpha5Processor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout BusAlpha5Processor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ PARAM_CHANNEL_ID, 1 },
        "Channel ID",
        1, 32, 1));

    return { params.begin(), params.end() };
}

void BusAlpha5Processor::prepareToPlay(double, int) {}

void BusAlpha5Processor::releaseResources() {}

void BusAlpha5Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer first
    buffer.clear();

    int channelID = getChannelID();
    const int numSamples = buffer.getNumSamples();

    // DEBUG: Log every 1000 blocks
    static int debugCounter = 0;
    if (++debugCounter >= 1000) {
        debugCounter = 0;
        int available = BusShared::getInstance().getNumAvailable(channelID);
        int writers = BusShared::getInstance().getActiveWriters(channelID);
        DBG("Bus Alpha: Ch " << channelID
            << " | Writers: " << writers
            << " | Available: " << available
            << " | Shared Mem: " << (BusShared::getInstance().isInitialized() ? "YES" : "NO"));
    }

    // Read summed audio from shared memory for this channel
    BusShared::getInstance().readFromChannel(
        channelID,
        buffer.getWritePointer(0),
        buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr,
        numSamples
    );

    // Update active channel count for display
    activeChannelCount = BusShared::getInstance().getActiveWriters(channelID);

    // Prevent smart disable with tiny DC offset (CRITICAL for generators!)
    if (numSamples > 0)
    {
        buffer.setSample(0, 0, buffer.getSample(0, 0) + 0.0000001f);
        if (buffer.getNumChannels() >= 2)
            buffer.setSample(1, 0, buffer.getSample(1, 0) + 0.0000001f);
    }
}

bool BusAlpha5Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only output, no input
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo();
}

int BusAlpha5Processor::getChannelID() const
{
    return static_cast<int>(apvts.getRawParameterValue(PARAM_CHANNEL_ID)->load());
}

juce::AudioProcessorEditor* BusAlpha5Processor::createEditor()
{
    return new BusAlpha5Editor(*this);
}

void BusAlpha5Processor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BusAlpha5Processor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xmlState);
            apvts.replaceState(state);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BusAlpha5Processor();
}