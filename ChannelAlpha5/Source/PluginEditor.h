// ============================================================================
// Source/PluginEditor.h
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ChannelAlpha2Editor : public juce::AudioProcessorEditor,
    private juce::Timer {
public:
    ChannelAlpha2Editor(ChannelAlpha2Processor&);
    ~ChannelAlpha2Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ChannelAlpha2Processor& processor;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> channelIDAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> emuAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> preEmphasisAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonicsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intersampleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseFloorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;

    // Button parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> selectButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoRecButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> emuButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> busSendButtonAttachment;  // DEBUG

    juce::Label channelLabel;
    juce::ComboBox channelIDSelector;
    juce::Slider panKnob;
    juce::Slider fader;
    juce::Label levelDisplay;
    juce::Label busSendStatusLabel;  // DEBUG: Shows bus send activity
    juce::TextButton selectButton;
    juce::TextButton autoRecButton;
    juce::TextButton soloButton;
    juce::TextButton muteButton;
    juce::TextButton emuButton;
    juce::TextButton busSendButton;  // DEBUG: Bus send toggle
    juce::Slider emuAmountKnob;
    juce::Slider preEmphasisKnob;
    juce::Slider harmonicsKnob;
    juce::Slider intersampleKnob;
    juce::Slider noiseFloorKnob;
    juce::Slider inputGainKnob;
    juce::Label emuAmountLabel;
    juce::Label preEmphasisLabel;
    juce::Label harmonicsLabel;
    juce::Label intersampleLabel;
    juce::Label noiseFloorLabel;
    juce::Label inputGainLabel;

    int64_t lastWriteCount = 0;  // DEBUG: Track data being sent

    static constexpr int BACKGROUND_COLOR = 0xff2a2a2a;
    static constexpr int PANEL_COLOR = 0xff1a1a1a;
    static constexpr int BORDER_COLOR = 0xff404040;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelAlpha2Editor)
};