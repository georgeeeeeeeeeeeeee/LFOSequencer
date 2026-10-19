#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class Win95LookAndFeel : public juce::LookAndFeel_V4
{
public:
    Win95LookAndFeel();

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour&, bool highlighted, bool down) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool highlighted, bool down) override;

    void drawComboBox(juce::Graphics&, int w, int h, bool isButtonDown,
                      int bx, int by, int bw, int bh, juce::ComboBox&) override;

    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

    void drawLabel(juce::Graphics&, juce::Label&) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;
};

//==============================================================================
class LFOWaveformDisplay : public juce::Component, public juce::Timer
{
public:
    explicit LFOWaveformDisplay(LFOChannel& channel);
    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    LFOChannel& ch;
};

//==============================================================================
class LFOChannelPanel : public juce::Component
{
public:
    LFOChannelPanel(LFOChannel& channel, int channelIndex);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void tick(); // called by editor timer to refresh live displays

private:
    void refreshFromChannel();

    LFOChannel& ch;
    int         index;

    LFOWaveformDisplay waveDisplay;

    juce::ToggleButton activeBtn     { "ON" };
    juce::ComboBox     waveSelector;
    juce::ToggleButton noteEnableBtn { "Note" };
    juce::ToggleButton ccEnableBtn   { "CC" };

    // Row 1 — titles
    juce::Label freqLabel, phaseLabel, noteLabel, chanLabel, threshLabel;
    // Row 1 — value boxes
    juce::Label freqBox, phaseBox, noteBox, chanBox, thrBox;

    // Row 2 — titles
    juce::Label ccLabel, velMinLabel, velMaxLabel, noteLenLabel, velLabel;
    // Row 2 — value boxes
    juce::Label ccBox, velMinBox, velMaxBox, lenBox, velBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOChannelPanel)
};

//==============================================================================
class LFOSequencerEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit LFOSequencerEditor(LFOSequencerProcessor&);
    ~LFOSequencerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    static constexpr int DISPLAY_CHANNELS = 4;

    LFOSequencerProcessor& processorRef;
    Win95LookAndFeel       win95LF;

    juce::OwnedArray<LFOChannelPanel> channelPanels;
    juce::TextButton syncBtn   { "Sync" };
    juce::TextButton spreadBtn { "Spread" };
    juce::TextButton resetBtn  { "Reset All" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOSequencerEditor)
};
