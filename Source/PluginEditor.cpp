#include "PluginEditor.h"

// ------------------------------------------------------------------ globals
static void setupGlobalKnob(juce::Slider& s, juce::Component& parent, bool bipolar) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 14);
    s.setColour(juce::Slider::rotarySliderFillColourId, theme::curve.withAlpha(0.85f));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.getProperties().set("bipolar", bipolar);
    parent.addAndMakeVisible(s);
}

GlobalPanel::GlobalPanel(juce::AudioProcessorValueTreeState& state)
    : modeAtt(state, "mode", (mode.addItemList({ "Stereo", "L/R", "M/S" }, 1), mode)),
      editAtt(state, "edit", (edit.addItemList({ "A (Left / Mid)", "B (Right / Side)" }, 1), edit)),
      scaleAtt(state, "scale", scale),
      outputAtt(state, "output", output),
      hiqAtt(state, "hiq", hiq) {
    addAndMakeVisible(mode);
    addAndMakeVisible(edit);
    setupGlobalKnob(scale, *this, true);
    setupGlobalKnob(output, *this, true);
    scale.setDoubleClickReturnValue(true, 100.0);
    output.setDoubleClickReturnValue(true, 0.0);
    hiq.setClickingTogglesState(true);
    hiq.setTooltip("2x oversampling: more accurate curves near the top of the spectrum");
    addAndMakeVisible(hiq);
    for (auto [l, t] : { std::pair { &modeLabel, "Mode" }, { &editLabel, "Edit" }, { &scaleLabel, "Scale" }, { &outputLabel, "Output" } }) {
        l->setText(t, juce::dontSendNotification);
        l->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(*l);
    }
}

void GlobalPanel::setEditVisible(bool v) {
    edit.setVisible(v);
    editLabel.setVisible(v);
}

void GlobalPanel::paint(juce::Graphics& g) {
    g.fillAll(theme::panel);
    g.setColour(theme::text);
    g.setFont(theme::font(18.0f, true));
    g.drawText("OctoEQ", getLocalBounds().removeFromTop(40).reduced(12, 0), juce::Justification::centredLeft);
}

void GlobalPanel::resized() {
    auto r = getLocalBounds().reduced(12, 0);
    r.removeFromTop(40);
    auto row = [&](int h) { auto x = r.removeFromTop(h); r.removeFromTop(6); return x; };
    modeLabel.setBounds(row(14)); mode.setBounds(row(22));
    editLabel.setBounds(row(14)); edit.setBounds(row(22));
    const int knobH = juce::jlimit(60, 110, (r.getHeight() - 60) / 2);
    scaleLabel.setBounds(row(14)); scale.setBounds(row(knobH));
    outputLabel.setBounds(row(14)); output.setBounds(row(knobH));
    hiq.setBounds(row(26));
}

// ------------------------------------------------------------------ editor
OctoEqEditor::OctoEqEditor(OctoEqProcessor& p)
    : juce::AudioProcessorEditor(p), proc(p), eqDisplay(p), bandStrip(p.apvts), globals(p.apvts) {
    setLookAndFeel(&lnf);
    addAndMakeVisible(eqDisplay);
    addAndMakeVisible(bandStrip);
    addAndMakeVisible(globals);
    eqDisplay.onBandSelected = [this](int b) { bandStrip.setSelectedBand(b); };
    bandStrip.onBandSelected = [this](int b) { eqDisplay.setSelectedBand(b); };
    bandStrip.setSelectedBand(eqDisplay.getSelectedBand());
    refreshBindings();
    setResizable(true, true);
    setResizeLimits(700, 400, 1600, 900);
    setSize(900, 480);
    startTimerHz(10);
}

OctoEqEditor::~OctoEqEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void OctoEqEditor::refreshBindings() {
    const bool stereo = int(proc.apvts.getRawParameterValue("mode")->load()) == octoeq::ModeStereo;
    globals.setEditVisible(!stereo);
    const int set = eqDisplay.activeSet();
    if (set != bandStrip.boundSet()) {
        bandStrip.bindToSet(set);
        eqDisplay.repaint();
    }
}

void OctoEqEditor::timerCallback() {
    refreshBindings();
    bandStrip.updateEnablement();
}

void OctoEqEditor::paint(juce::Graphics& g) { g.fillAll(theme::background); }

void OctoEqEditor::resized() {
    auto r = getLocalBounds();
    globals.setBounds(r.removeFromRight(160));
    const int stripH = juce::jlimit(190, 280, int(r.getHeight() * 0.44f));
    bandStrip.setBounds(r.removeFromBottom(stripH));
    eqDisplay.setBounds(r);
}
