// ============================================================================
// Source/PluginEditor.cpp
// ============================================================================
#include "PluginEditor.h"
#include "BusShared.h"

ChannelAlpha2Editor::ChannelAlpha2Editor(ChannelAlpha2Processor& p)
    : AudioProcessorEditor(&p), processor(p) {

    setSize(processor.pluginWidth, processor.pluginHeight);

    channelLabel.setText("CHANNEL", juce::dontSendNotification);
    channelLabel.setJustificationType(juce::Justification::centred);
    channelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    channelLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 32; ++i)
        channelIDSelector.addItem(juce::String(i), i);
    channelIDSelector.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(channelIDSelector);

    channelIDAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "channelID", channelIDSelector);

    panKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panKnob.setRange(-1.0, 1.0, 0.01);
    panKnob.setValue(0.0);
    panKnob.setDoubleClickReturnValue(true, 0.0);
    panKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
    addAndMakeVisible(panKnob);

    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "pan", panKnob);

    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setRange(-60.0, 10.0, 0.1);
    fader.setValue(0.0);
    fader.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(fader);

    faderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "fader", fader);

    levelDisplay.setText("0.0 dB", juce::dontSendNotification);
    levelDisplay.setJustificationType(juce::Justification::centred);
    levelDisplay.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    levelDisplay.setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(levelDisplay);

    // DEBUG: Bus send status display
    busSendStatusLabel.setText("Bus: OFF", juce::dontSendNotification);
    busSendStatusLabel.setJustificationType(juce::Justification::centred);
    busSendStatusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    busSendStatusLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    addAndMakeVisible(busSendStatusLabel);

    // Button setup with parameter attachments
    selectButton.setButtonText("SEL");
    selectButton.setClickingTogglesState(true);
    selectButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    addAndMakeVisible(selectButton);

    selectButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getAPVTS(), "select", selectButton);

    autoRecButton.setButtonText("AUTO");
    autoRecButton.setClickingTogglesState(true);
    autoRecButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    addAndMakeVisible(autoRecButton);

    autoRecButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getAPVTS(), "autoRec", autoRecButton);

    soloButton.setButtonText("SOLO");
    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    addAndMakeVisible(soloButton);

    soloButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getAPVTS(), "solo", soloButton);

    muteButton.setButtonText("MUTE");
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    addAndMakeVisible(muteButton);

    muteButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getAPVTS(), "mute", muteButton);

    emuButton.setButtonText("EMU");
    emuButton.setClickingTogglesState(true);
    emuButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    addAndMakeVisible(emuButton);

    emuButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getAPVTS(), "ddxEmulation", emuButton);

    // DEBUG: Bus Send Toggle Button
    busSendButton.setButtonText("BUS");
    busSendButton.setClickingTogglesState(true);
    busSendButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan);
    addAndMakeVisible(busSendButton);

    busSendButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getAPVTS(), "busSendEnabled", busSendButton);

    emuAmountKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    emuAmountKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    emuAmountKnob.setRange(0.0, 2.0, 0.01);
    emuAmountKnob.setValue(1.0);
    emuAmountKnob.setDoubleClickReturnValue(true, 1.0);
    emuAmountKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange.withAlpha(0.7f));
    addAndMakeVisible(emuAmountKnob);

    emuAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "emuAmount", emuAmountKnob);

    emuAmountLabel.setText("AMT", juce::dontSendNotification);
    emuAmountLabel.setJustificationType(juce::Justification::centred);
    emuAmountLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    emuAmountLabel.setFont(juce::Font(9.0f));
    addAndMakeVisible(emuAmountLabel);

    preEmphasisKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    preEmphasisKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    preEmphasisKnob.setRange(0.0, 1.0, 0.01);
    preEmphasisKnob.setValue(0.0);
    preEmphasisKnob.setDoubleClickReturnValue(true, 0.0);
    preEmphasisKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan.withAlpha(0.7f));
    addAndMakeVisible(preEmphasisKnob);

    preEmphasisAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "preEmphasis", preEmphasisKnob);

    preEmphasisLabel.setText("PRE", juce::dontSendNotification);
    preEmphasisLabel.setJustificationType(juce::Justification::centred);
    preEmphasisLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    preEmphasisLabel.setFont(juce::Font(9.0f));
    addAndMakeVisible(preEmphasisLabel);

    harmonicsKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    harmonicsKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    harmonicsKnob.setRange(0.0, 1.0, 0.01);
    harmonicsKnob.setValue(0.0);
    harmonicsKnob.setDoubleClickReturnValue(true, 0.0);
    harmonicsKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::magenta.withAlpha(0.7f));
    addAndMakeVisible(harmonicsKnob);

    harmonicsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "harmonics", harmonicsKnob);

    harmonicsLabel.setText("HARM", juce::dontSendNotification);
    harmonicsLabel.setJustificationType(juce::Justification::centred);
    harmonicsLabel.setColour(juce::Label::textColourId, juce::Colours::magenta);
    harmonicsLabel.setFont(juce::Font(9.0f));
    addAndMakeVisible(harmonicsLabel);

    intersampleKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    intersampleKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    intersampleKnob.setRange(0.0, 1.0, 0.01);
    intersampleKnob.setValue(0.0);
    intersampleKnob.setDoubleClickReturnValue(true, 0.0);
    intersampleKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::yellow.withAlpha(0.7f));
    addAndMakeVisible(intersampleKnob);

    intersampleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "intersampleMod", intersampleKnob);

    intersampleLabel.setText("ISM", juce::dontSendNotification);
    intersampleLabel.setJustificationType(juce::Justification::centred);
    intersampleLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    intersampleLabel.setFont(juce::Font(9.0f));
    addAndMakeVisible(intersampleLabel);

    noiseFloorKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    noiseFloorKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    noiseFloorKnob.setRange(-120.0, -60.0, 0.1);
    noiseFloorKnob.setValue(-96.0);
    noiseFloorKnob.setDoubleClickReturnValue(true, -96.0);
    noiseFloorKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::grey.withAlpha(0.7f));
    addAndMakeVisible(noiseFloorKnob);

    noiseFloorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "noiseFloor", noiseFloorKnob);

    noiseFloorLabel.setText("NOISE", juce::dontSendNotification);
    noiseFloorLabel.setJustificationType(juce::Justification::centred);
    noiseFloorLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    noiseFloorLabel.setFont(juce::Font(9.0f));
    addAndMakeVisible(noiseFloorLabel);

    inputGainKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    inputGainKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    inputGainKnob.setRange(-20.0, 60.0, 0.1);
    inputGainKnob.setValue(0.0);
    inputGainKnob.setDoubleClickReturnValue(true, 0.0);
    inputGainKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightgrey.withAlpha(0.7f));
    addAndMakeVisible(inputGainKnob);

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), "inputGain", inputGainKnob);

    inputGainLabel.setText("GAIN", juce::dontSendNotification);
    inputGainLabel.setJustificationType(juce::Justification::centred);
    inputGainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    inputGainLabel.setFont(juce::Font(9.0f));
    addAndMakeVisible(inputGainLabel);

    startTimerHz(30);
}

ChannelAlpha2Editor::~ChannelAlpha2Editor() {
    stopTimer();
}

void ChannelAlpha2Editor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour((juce::uint32)BACKGROUND_COLOR));

    auto area = getLocalBounds();
    g.setColour(juce::Colour((juce::uint32)PANEL_COLOR));
    g.fillRect(area.removeFromTop(100));

    g.setColour(juce::Colour((juce::uint32)BORDER_COLOR));
    g.drawRect(getLocalBounds(), 2);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("Channel Alpha 5", getLocalBounds().removeFromTop(25),
        juce::Justification::centred, true);
}

void ChannelAlpha2Editor::resized() {
    auto area = getLocalBounds().reduced(5);
    area.removeFromTop(30);

    auto topArea = area.removeFromTop(80);
    channelLabel.setBounds(topArea.removeFromTop(20));
    channelIDSelector.setBounds(topArea.removeFromTop(24).reduced(20, 0));
    panKnob.setBounds(topArea.reduced(40, 5));

    int columnWidth = 50;
    auto leftColumn = area.removeFromLeft(columnWidth);
    auto middleColumn = area.removeFromLeft(columnWidth);
    auto rightColumn = area;

    int buttonHeight = 22;
    int buttonSpacing = 2;
    leftColumn.removeFromTop(10);

    selectButton.setBounds(leftColumn.removeFromTop(buttonHeight));
    leftColumn.removeFromTop(buttonSpacing);
    autoRecButton.setBounds(leftColumn.removeFromTop(buttonHeight));
    leftColumn.removeFromTop(buttonSpacing);
    soloButton.setBounds(leftColumn.removeFromTop(buttonHeight));
    leftColumn.removeFromTop(buttonSpacing);
    muteButton.setBounds(leftColumn.removeFromTop(buttonHeight));
    leftColumn.removeFromTop(buttonSpacing);
    emuButton.setBounds(leftColumn.removeFromTop(buttonHeight));
    leftColumn.removeFromTop(buttonSpacing);
    busSendButton.setBounds(leftColumn.removeFromTop(buttonHeight));  // DEBUG: Bus send button
    leftColumn.removeFromTop(buttonSpacing);
    busSendStatusLabel.setBounds(leftColumn.removeFromTop(18));  // DEBUG: Status display

    int faderHeight = middleColumn.getHeight() - 25;
    fader.setBounds(middleColumn.removeFromTop(faderHeight).reduced(10, 0));
    levelDisplay.setBounds(middleColumn.removeFromTop(20));

    int knobHeight = 35;
    int labelHeight = 12;
    int spacing = 4;

    rightColumn.removeFromTop(10);

    emuAmountKnob.setBounds(rightColumn.removeFromTop(knobHeight).reduced(10, 0));
    emuAmountLabel.setBounds(rightColumn.removeFromTop(labelHeight));
    rightColumn.removeFromTop(spacing);

    preEmphasisKnob.setBounds(rightColumn.removeFromTop(knobHeight).reduced(10, 0));
    preEmphasisLabel.setBounds(rightColumn.removeFromTop(labelHeight));
    rightColumn.removeFromTop(spacing);

    harmonicsKnob.setBounds(rightColumn.removeFromTop(knobHeight).reduced(10, 0));
    harmonicsLabel.setBounds(rightColumn.removeFromTop(labelHeight));
    rightColumn.removeFromTop(spacing);

    intersampleKnob.setBounds(rightColumn.removeFromTop(knobHeight).reduced(10, 0));
    intersampleLabel.setBounds(rightColumn.removeFromTop(labelHeight));
    rightColumn.removeFromTop(spacing);

    noiseFloorKnob.setBounds(rightColumn.removeFromTop(knobHeight).reduced(10, 0));
    noiseFloorLabel.setBounds(rightColumn.removeFromTop(labelHeight));
    rightColumn.removeFromTop(spacing);

    inputGainKnob.setBounds(rightColumn.removeFromTop(knobHeight).reduced(10, 0));
    inputGainLabel.setBounds(rightColumn.removeFromTop(labelHeight));
}

void ChannelAlpha2Editor::timerCallback() {
    // Update level display
    float faderDB = processor.getAPVTS().getRawParameterValue("fader")->load();
    levelDisplay.setText(juce::String(faderDB, 1) + " dB", juce::dontSendNotification);

    // DEBUG: Update bus send status
    bool busSendEnabled = processor.isBusSendEnabled();
    int channelID = processor.getChannelID();
    int64_t totalWritten = BusShared::getInstance().getTotalWritten(channelID);

    if (busSendEnabled && totalWritten > lastWriteCount) {
        busSendStatusLabel.setText("Bus: ● ON", juce::dontSendNotification);
        busSendStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lime);
        lastWriteCount = totalWritten;
    }
    else if (busSendEnabled) {
        busSendStatusLabel.setText("Bus: ON", juce::dontSendNotification);
        busSendStatusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    }
    else {
        busSendStatusLabel.setText("Bus: OFF", juce::dontSendNotification);
        busSendStatusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    }
}