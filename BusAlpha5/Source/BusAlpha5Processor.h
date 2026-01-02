#pragma once
#include <JuceHeader.h>
#include "BusShared.h"

class BusAlpha5Processor : public juce::AudioProcessor
{
public:
    BusAlpha5Processor();
    ~BusAlpha5Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Bus Alpha 5"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return std::numeric_limits<double>::infinity(); }
    bool silenceInProducesSilenceOut() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    int getChannelID() const;
    int getActiveChannelCount() const { return activeChannelCount; }

private:
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> activeChannelCount{ 0 };

    static constexpr const char* PARAM_CHANNEL_ID = "channelID";

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BusAlpha5Processor)
};