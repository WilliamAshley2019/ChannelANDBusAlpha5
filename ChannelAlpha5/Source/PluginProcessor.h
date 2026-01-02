// ============================================================================
// Source/PluginProcessor.h
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include <random>
#include <cstdint>

class ChannelAlpha2Processor : public juce::AudioProcessor,
    public juce::AudioProcessorValueTreeState::Listener {
public:
    ChannelAlpha2Processor();
    ~ChannelAlpha2Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    const juce::String getName() const override { return "Channel Alpha 5"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    bool isMuted() const { return muted.load(); }
    bool isSoloed() const { return soloed.load(); }
    bool isSelected() const { return selected.load(); }
    bool isAutoRecEnabled() const { return autoRec.load(); }
    bool isDDXEmulationEnabled() const { return ddxEmulation.load(); }
    bool isBusSendEnabled() const { return busSendEnabled.load(); }

    void setMuted(bool shouldMute);
    void setSoloed(bool shouldSolo);
    void setSelected(bool shouldSelect);
    void setAutoRec(bool shouldAutoRec);
    void setDDXEmulation(bool shouldEnable);
    void setBusSendEnabled(bool shouldEnable);

    int getChannelID() const;

    int pluginWidth = 160;
    int pluginHeight = 530;

    // AudioProcessorValueTreeState::Listener override
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<bool> muted{ false };
    std::atomic<bool> soloed{ false };
    std::atomic<bool> selected{ false };
    std::atomic<bool> autoRec{ false };
    std::atomic<bool> ddxEmulation{ false };
    std::atomic<bool> busSendEnabled{ true };

    // Track current channel ID for registration changes
    int currentChannelID = 1;

    // Smooth parameter changes
    juce::LinearSmoothedValue<float> muteGain;
    juce::LinearSmoothedValue<float> faderGain;
    juce::LinearSmoothedValue<float> panValue;

    // DDX3216 parameter smoothers
    juce::LinearSmoothedValue<float> emuAmountSmoother;
    juce::LinearSmoothedValue<float> preEmphasisSmoother;
    juce::LinearSmoothedValue<float> harmonicsSmoother;
    juce::LinearSmoothedValue<float> intersampleSmoother;
    juce::LinearSmoothedValue<float> saturationSmoother;
    juce::LinearSmoothedValue<float> noiseFloorSmoother;
    juce::LinearSmoothedValue<float> inputGainSmoother;

    // DDX3216 DSP chains
    juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,      // 0: HPF (22Hz)
        juce::dsp::IIR::Filter<float>,      // 1: Low-shelf warmth
        juce::dsp::IIR::Filter<float>,      // 2: High-shelf air
        juce::dsp::IIR::Filter<float>       // 3: Pre-emphasis
    > preEQChain;

    juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>       // Gentle tilt
    > postEQChain;

    juce::dsp::ProcessSpec spec;

    // Oversampling for intersample modulation
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    bool oversamplerPrepared = false;

    // Random generators for noise (one per channel)
    std::vector<uint32_t> channelSeeds;
    std::vector<float> noiseHPFStates;
    float noiseHPFCoeff{ 0.0f };

    // DDX3216 processing functions
    void processDDX3216(juce::AudioBuffer<float>& buffer);

    // Individual processing functions
    void applySaturation(juce::AudioBuffer<float>& buffer, float amount);
    void applyHarmonics(juce::AudioBuffer<float>& buffer, float amount);
    void applyNoise(juce::AudioBuffer<float>& buffer, float noiseFloorDB, float inputGainDB, float emuAmount);
    void applyIntersampleModulation(juce::AudioBuffer<float>& buffer, float amount);

    // Helper functions
    inline float fastTanh(float x) const;
    inline float softClip(float x) const;
    inline float hardClip(float x, float threshold) const;
    inline float generateWhiteNoise(uint32_t& seed) const;

    static constexpr const char* PARAM_CHANNEL_ID = "channelID";
    static constexpr const char* PARAM_FADER = "fader";
    static constexpr const char* PARAM_PAN = "pan";
    static constexpr const char* PARAM_EMU_AMOUNT = "emuAmount";
    static constexpr const char* PARAM_PRE_EMPHASIS = "preEmphasis";
    static constexpr const char* PARAM_HARMONICS = "harmonics";
    static constexpr const char* PARAM_INTERSAMPLE = "intersampleMod";
    static constexpr const char* PARAM_NOISE_FLOOR = "noiseFloor";
    static constexpr const char* PARAM_INPUT_GAIN = "inputGain";
    static constexpr const char* PARAM_SELECT = "select";
    static constexpr const char* PARAM_AUTO_REC = "autoRec";
    static constexpr const char* PARAM_SOLO = "solo";
    static constexpr const char* PARAM_MUTE = "mute";
    static constexpr const char* PARAM_DDX_EMULATION = "ddxEmulation";
    static constexpr const char* PARAM_BUS_SEND_ENABLED = "busSendEnabled";

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelAlpha2Processor)
};