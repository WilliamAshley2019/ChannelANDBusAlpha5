#pragma once
#include <JuceHeader.h>
#include "BusAlpha5Processor.h"

class BusAlpha5Editor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit BusAlpha5Editor(BusAlpha5Processor&);
    ~BusAlpha5Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    BusAlpha5Processor& processor;

    juce::Label channelLabel;
    juce::ComboBox channelIDSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> channelIDAttachment;

    juce::Label statusLabel;
    juce::Label activeChannelsLabel;
    juce::Label bufferLevelLabel;

    int activeChannels = 0;
    int bufferLevel = 0;
    int64_t readCount = 0;

    static constexpr int BACKGROUND_COLOR = 0xff1a1a1a;
    static constexpr int PANEL_COLOR = 0xff2a2a2a;
    static constexpr int BORDER_COLOR = 0xff404040;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BusAlpha5Editor)
};