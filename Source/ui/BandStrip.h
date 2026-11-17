#pragma once
#include "PluginProcessor.h"

// One column of per-band controls.
class BandColumn : public juce::Component {
public:
    explicit BandColumn(int index);
    void bind(juce::AudioProcessorValueTreeState& state, int set);
    void setSelected(bool s) { if (s != selected) { selected = s; repaint(); } }
    void updateEnablement();   // dim gain for cut/notch types, Q for nothing
    std::function<void(int)> onSelect;

    juce::ToggleButton on;
    juce::ComboBox type;
    juce::Slider freq, gain, q;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override { if (onSelect) onSelect(index); }

private:
    const int index;
    bool selected = false;
    juce::Label freqLabel, gainLabel, qLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAtt, gainAtt, qAtt;
};

class BandStrip : public juce::Component {
public:
    explicit BandStrip(juce::AudioProcessorValueTreeState& state);
    void bindToSet(int set);
    int boundSet() const { return set; }
    BandColumn& column(int i) { return *columns[size_t(juce::jlimit(0, octoeq::kNumBands - 1, i))]; }
    void setSelectedBand(int band);
    void updateEnablement();
    std::function<void(int)> onBandSelected;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    int set = -1;
    std::array<std::unique_ptr<BandColumn>, octoeq::kNumBands> columns;
};
