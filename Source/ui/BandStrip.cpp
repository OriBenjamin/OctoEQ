#include "ui/BandStrip.h"
#include "ui/Theme.h"

using namespace octoeq;

static void setupKnob(juce::Slider& s, juce::Colour c, juce::Component& parent) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 13);
    s.setColour(juce::Slider::rotarySliderFillColourId, c);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour(juce::Slider::textBoxTextColourId, theme::text);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    parent.addAndMakeVisible(s);
}

BandColumn::BandColumn(int i) : index(i) {
    const auto c = theme::band(i);
    on.setColour(juce::ToggleButton::tickColourId, c);
    on.setTooltip("Band on/off");
    addAndMakeVisible(on);
    type.addItemList({ "Low Cut 12", "Low Cut 48", "Low Shelf", "Bell", "Notch", "High Shelf", "High Cut 12", "High Cut 48" }, 1);
    type.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(type);
    setupKnob(freq, c, *this);
    setupKnob(gain, c, *this);
    setupKnob(q, c, *this);
    gain.getProperties().set("bipolar", true);
    gain.setDoubleClickReturnValue(true, 0.0);
    for (auto* l : { &freqLabel, &gainLabel, &qLabel }) {
        l->setJustificationType(juce::Justification::centred);
        l->setFont(theme::font(10.0f));
        l->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(*l);
    }
    freqLabel.setText("Freq", juce::dontSendNotification);
    gainLabel.setText("Gain", juce::dontSendNotification);
    qLabel.setText("Q", juce::dontSendNotification);
}

void BandColumn::bind(juce::AudioProcessorValueTreeState& state, int set) {
    using A = juce::AudioProcessorValueTreeState;
    onAtt.reset(); typeAtt.reset(); freqAtt.reset(); gainAtt.reset(); qAtt.reset();
    onAtt   = std::make_unique<A::ButtonAttachment>(state, OctoEqProcessor::paramId(set, index, "on"), on);
    typeAtt = std::make_unique<A::ComboBoxAttachment>(state, OctoEqProcessor::paramId(set, index, "type"), type);
    freqAtt = std::make_unique<A::SliderAttachment>(state, OctoEqProcessor::paramId(set, index, "freq"), freq);
    gainAtt = std::make_unique<A::SliderAttachment>(state, OctoEqProcessor::paramId(set, index, "gain"), gain);
    qAtt    = std::make_unique<A::SliderAttachment>(state, OctoEqProcessor::paramId(set, index, "q"), q);
    updateEnablement();
}

void BandColumn::updateEnablement() {
    const int t = type.getSelectedItemIndex();
    gain.setEnabled(t < 0 || typeUsesGain(FilterType(t)));
}

void BandColumn::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(2.0f, 0.0f);
    g.setColour(selected ? theme::panelLight : theme::panel);
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(theme::band(index).withAlpha(on.getToggleState() ? 1.0f : 0.35f));
    g.fillRoundedRectangle(r.removeFromTop(3.0f), 1.5f);
    g.setColour(theme::text);
    g.setFont(theme::font(12.0f, true));
    g.drawText(juce::String(index + 1), getLocalBounds().withHeight(26).withTrimmedLeft(8), juce::Justification::centredLeft);
}

void BandColumn::resized() {
    auto r = getLocalBounds().reduced(6, 4);
    auto top = r.removeFromTop(22);
    on.setBounds(top.removeFromRight(22));
    r.removeFromTop(2);
    type.setBounds(r.removeFromTop(20));
    r.removeFromTop(4);
    // Freq (widest readout) gets the top row; Gain | Q share the bottom row.
    auto place = [](juce::Rectangle<int> cell, juce::Label& l, juce::Slider& s) {
        l.setBounds(cell.removeFromTop(11));
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, cell.getWidth(), 13);
        s.setBounds(cell);
    };
    auto top2 = r.removeFromTop(r.getHeight() / 2);
    place(top2, freqLabel, freq);
    place(r.removeFromLeft(r.getWidth() / 2), gainLabel, gain);
    place(r, qLabel, q);
}

// ------------------------------------------------------------------ strip
BandStrip::BandStrip(juce::AudioProcessorValueTreeState& state) : apvts(state) {
    for (int i = 0; i < kNumBands; ++i) {
        columns[size_t(i)] = std::make_unique<BandColumn>(i);
        columns[size_t(i)]->onSelect = [this](int b) { setSelectedBand(b); if (onBandSelected) onBandSelected(b); };
        addAndMakeVisible(*columns[size_t(i)]);
    }
    bindToSet(0);
}

void BandStrip::bindToSet(int s) {
    if (s == set) return;
    set = s;
    for (auto& c : columns) c->bind(apvts, set);
}

void BandStrip::setSelectedBand(int band) {
    for (int i = 0; i < kNumBands; ++i) columns[size_t(i)]->setSelected(i == band);
}

void BandStrip::updateEnablement() { for (auto& c : columns) { c->updateEnablement(); c->repaint(); } }

void BandStrip::paint(juce::Graphics& g) { g.fillAll(theme::background); }

void BandStrip::resized() {
    auto r = getLocalBounds().reduced(4, 4);
    const int w = r.getWidth() / kNumBands;
    for (auto& c : columns) c->setBounds(r.removeFromLeft(w));
}
