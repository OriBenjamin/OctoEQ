#pragma once
#include "PluginProcessor.h"
#include "ui/Theme.h"
#include "ui/EqDisplay.h"
#include "ui/BandStrip.h"

// Right-hand column: mode, edit channel, scale, output, Hi-Q.
class GlobalPanel : public juce::Component {
public:
    explicit GlobalPanel(juce::AudioProcessorValueTreeState& state);
    void setEditVisible(bool v);
    void paint(juce::Graphics&) override;
    void resized() override;

    juce::ComboBox mode, edit;
    juce::Slider scale, output;
    juce::TextButton hiq { "Hi-Q" };

private:
    juce::Label modeLabel, editLabel, scaleLabel, outputLabel;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAtt, editAtt;
    juce::AudioProcessorValueTreeState::SliderAttachment scaleAtt, outputAtt;
    juce::AudioProcessorValueTreeState::ButtonAttachment hiqAtt;
};

class OctoEqEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit OctoEqEditor(OctoEqProcessor& p);
    ~OctoEqEditor() override;

    EqDisplay& display() { return eqDisplay; }
    BandStrip& strip() { return bandStrip; }
    void refreshBindings();   // follow Mode / Edit changes

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    theme::LookAndFeel lnf;
    OctoEqProcessor& proc;
    EqDisplay eqDisplay;
    BandStrip bandStrip;
    GlobalPanel globals;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctoEqEditor)
};
