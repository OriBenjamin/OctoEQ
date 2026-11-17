#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace theme {
inline const juce::Colour background { 0xff1b1c1f };
inline const juce::Colour panel      { 0xff232529 };
inline const juce::Colour panelLight { 0xff2c2f34 };
inline const juce::Colour grid       { 0xff33363b };
inline const juce::Colour text       { 0xffd0d3d8 };
inline const juce::Colour textDim    { 0xff7f848c };
inline const juce::Colour curve      { 0xfff2f2f2 };
inline const juce::Colour spectrum   { 0xff4a5361 };

inline juce::Colour band(int i) {
    static const juce::uint32 c[8] = { 0xffff6b6b, 0xffffa94d, 0xffffd43b, 0xff69db7c,
                                       0xff38d9a9, 0xff4dabf7, 0xff9775fa, 0xfff783ac };
    return juce::Colour(c[juce::jlimit(0, 7, i)]);
}
inline juce::Font font(float h, bool bold = false) {
    return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain));
}

class LookAndFeel : public juce::LookAndFeel_V4 {
public:
    LookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, background);
        setColour(juce::ComboBox::backgroundColourId, panelLight);
        setColour(juce::ComboBox::outlineColourId, grid);
        setColour(juce::ComboBox::textColourId, text);
        setColour(juce::ComboBox::arrowColourId, textDim);
        setColour(juce::PopupMenu::backgroundColourId, panel);
        setColour(juce::PopupMenu::textColourId, text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, panelLight.brighter(0.2f));
        setColour(juce::Slider::textBoxTextColourId, text);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::rotarySliderOutlineColourId, grid);
        setColour(juce::Label::textColourId, textDim);
        setColour(juce::TextButton::buttonColourId, panelLight);
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xfff0b429));
        setColour(juce::TextButton::textColourOffId, textDim);
        setColour(juce::TextButton::textColourOnId, background);
        setColour(juce::ToggleButton::tickColourId, text);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float start, float end, juce::Slider& s) override {
        const auto bounds = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(3.0f);
        const float r = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = start + pos * (end - start);
        const bool bipolar = s.getProperties()["bipolar"];
        const float from = bipolar ? (start + end) * 0.5f : start;
        const auto fill = s.isEnabled() ? s.findColour(juce::Slider::rotarySliderFillColourId) : textDim.withAlpha(0.4f);

        juce::Path track; track.addCentredArc(c.x, c.y, r - 2, r - 2, 0, start, end, true);
        g.setColour(grid); g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path val; val.addCentredArc(c.x, c.y, r - 2, r - 2, 0, juce::jmin(from, angle), juce::jmax(from, angle), true);
        g.setColour(fill); g.strokePath(val, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(panelLight); g.fillEllipse(juce::Rectangle<float>(r * 1.25f, r * 1.25f).withCentre(c));
        const auto tip = c.getPointOnCircumference(r * 0.55f, angle);
        g.setColour(s.isEnabled() ? text : textDim);
        g.drawLine({ c.getPointOnCircumference(r * 0.15f, angle), tip }, 2.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override {
        const auto area = b.getLocalBounds().toFloat().reduced(2.0f);
        const auto led = juce::Rectangle<float>(14.0f, 14.0f).withCentre(area.getCentre());
        const auto col = b.findColour(juce::ToggleButton::tickColourId);
        g.setColour(b.getToggleState() ? col : panelLight.brighter(hover ? 0.25f : 0.1f));
        g.fillEllipse(led);
        g.setColour(grid.brighter(0.3f)); g.drawEllipse(led, 1.0f);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override { return font(12.0f); }
    juce::Font getPopupMenuFont() override { return font(13.0f); }
    juce::Font getLabelFont(juce::Label&) override { return font(11.0f); }
    juce::Font getTextButtonFont(juce::TextButton&, int) override { return font(12.0f, true); }
};
} // namespace theme
