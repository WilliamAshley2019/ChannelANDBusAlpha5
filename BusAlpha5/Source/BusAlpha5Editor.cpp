#include "BusAlpha5Editor.h"

BusAlpha5Editor::BusAlpha5Editor(BusAlpha5Processor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(320, 280);

    // Channel selector
    channelLabel.setText("BUS CHANNEL", juce::dontSendNotification);
    channelLabel.setJustificationType(juce::Justification::centred);
    channelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    channelLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(channelLabel);

    for (int i = 1; i <= 32; ++i)
        channelIDSelector.addItem(juce::String(i), i);
    channelIDSelector.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(channelIDSelector);

    channelIDAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "channelID", channelIDSelector);

    // Status displays
    statusLabel.setText("IDLE", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(statusLabel);

    activeChannelsLabel.setText("Active Channels: 0", juce::dontSendNotification);
    activeChannelsLabel.setJustificationType(juce::Justification::centred);
    activeChannelsLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    activeChannelsLabel.setFont(juce::Font(13.0f));
    addAndMakeVisible(activeChannelsLabel);

    bufferLevelLabel.setText("Buffer: 0 samples", juce::dontSendNotification);
    bufferLevelLabel.setJustificationType(juce::Justification::centred);
    bufferLevelLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    bufferLevelLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(bufferLevelLabel);

    startTimerHz(30);
}

BusAlpha5Editor::~BusAlpha5Editor()
{
    stopTimer();
}

void BusAlpha5Editor::timerCallback()
{
    int channelID = processor.getChannelID();

    activeChannels = BusShared::getInstance().getActiveWriters(channelID);
    bufferLevel = BusShared::getInstance().getNumAvailable(channelID);
    readCount = BusShared::getInstance().getTotalRead(channelID);

    // Update status
    if (activeChannels > 0 && bufferLevel > 0)
    {
        statusLabel.setText("● ACTIVE", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lime);
    }
    else
    {
        statusLabel.setText("○ IDLE", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    }

    activeChannelsLabel.setText("Active Channels: " + juce::String(activeChannels),
        juce::dontSendNotification);
    bufferLevelLabel.setText("Buffer: " + juce::String(bufferLevel) + " samples",
        juce::dontSendNotification);

    repaint();
}

void BusAlpha5Editor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour((juce::uint32)BACKGROUND_COLOR));

    // Header panel
    auto headerArea = getLocalBounds().removeFromTop(90);
    g.setColour(juce::Colour((juce::uint32)PANEL_COLOR));
    g.fillRect(headerArea);

    // Border
    g.setColour(juce::Colour((juce::uint32)BORDER_COLOR));
    g.drawRect(getLocalBounds(), 2);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("Bus Alpha 5", getLocalBounds().removeFromTop(30),
        juce::Justification::centred, true);

    // Buffer visualization bar
    int barY = 160;
    int barHeight = 30;
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(20, barY, getWidth() - 40, barHeight);

    // Fill based on buffer level
    float fillRatio = juce::jmin(1.0f, bufferLevel / 22050.0f); // ~0.5 sec at 44.1kHz
    g.setColour(juce::Colours::lime.withAlpha(0.7f));
    g.fillRect(20, barY, static_cast<int>((getWidth() - 40) * fillRatio), barHeight);

    // Statistics
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Total Read: " + juce::String(readCount) + " samples",
        0, getHeight() - 30, getWidth(), 20, juce::Justification::centred);
}

void BusAlpha5Editor::resized()
{
    auto area = getLocalBounds().reduced(10);

    area.removeFromTop(35); // Title space

    channelLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(5);
    channelIDSelector.setBounds(area.removeFromTop(24).reduced(60, 0));

    area.removeFromTop(15);
    statusLabel.setBounds(area.removeFromTop(25));

    area.removeFromTop(10);
    activeChannelsLabel.setBounds(area.removeFromTop(20));

    area.removeFromTop(45); // Space for buffer bar
    bufferLevelLabel.setBounds(area.removeFromTop(20));
}