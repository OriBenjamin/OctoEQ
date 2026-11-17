#include "test_util.h"
#include "PluginEditor.h"

static float getParam(OctoEqProcessor& p, const juce::String& id) { return p.apvts.getRawParameterValue(id)->load(); }
static void setParam(OctoEqProcessor& p, const juce::String& id, float v) {
    auto* param = p.apvts.getParameter(id); param->setValueNotifyingHost(param->convertTo0to1(v));
}

TEST(mapping_roundtrip) {
    CHECK_NEAR(EqDisplay::freqToX(10, 800), 0, 0.01);
    CHECK_NEAR(EqDisplay::freqToX(22000, 800), 800, 0.01);
    CHECK_NEAR(EqDisplay::xToFreq(EqDisplay::freqToX(1000, 800), 800), 1000, 0.5);
    CHECK_NEAR(EqDisplay::gainToY(0, 300), 150, 0.01);
    CHECK_NEAR(EqDisplay::gainToY(18, 300), 0, 0.01);
    CHECK_NEAR(EqDisplay::yToGain(EqDisplay::gainToY(-7.5, 300), 300), -7.5, 0.01);
}
TEST(free_band_response_matches_engine) {
    octoeq::EqEngine e; e.prepare(48000, 64);
    auto b = octoeq::defaultBand(4); b.gainDb = 5; b.freq = 2000; e.setBand(0, 4, b); e.setScale(0.5f);
    CHECK_NEAR(octoeq::bandResponseDb(b, 0.5f, 2000, 48000), e.bandResponseDb(0, 4, 2000), 1e-9);
}
TEST(drag_node_sets_params) {
    OctoEqProcessor p; OctoEqEditor ed(p); ed.setSize(900, 480);
    auto& d = ed.display();
    const float w = float(d.getWidth()), h = float(d.getHeight());
    d.setBandFromPoint(3, { EqDisplay::freqToX(2000, w), EqDisplay::gainToY(6, h) });
    CHECK_NEAR(getParam(p, "bA4_freq"), 2000, 5);
    CHECK_NEAR(getParam(p, "bA4_gain"), 6, 0.05);
    CHECK(d.hitTestNode({ EqDisplay::freqToX(2000, w), EqDisplay::gainToY(6, h) }) == 3);
    d.setBandFromPoint(3, { EqDisplay::freqToX(2000, w), EqDisplay::gainToY(40, h) - 50 });
    CHECK_NEAR(getParam(p, "bA4_gain"), 15, 0.01); // clamped
}
TEST(edit_switch_rebinds_strip) {
    OctoEqProcessor p; OctoEqEditor ed(p); ed.setSize(900, 480);
    setParam(p, "mode", 1); setParam(p, "edit", 1);
    ed.refreshBindings();
    CHECK(ed.display().activeSet() == 1);
    ed.strip().column(2).gain.setValue(4.0, juce::sendNotificationSync);
    CHECK_NEAR(getParam(p, "bB3_gain"), 4.0, 0.05);
    CHECK_NEAR(getParam(p, "bA3_gain"), 0.0, 0.0);
    setParam(p, "mode", 0); ed.refreshBindings();
    CHECK(ed.display().activeSet() == 0); // Stereo always edits A
}
TEST(param_text_formatting) {
    OctoEqProcessor p;
    CHECK(p.apvts.getParameter("bA3_freq")->getText(p.apvts.getParameter("bA3_freq")->convertTo0to1(250.0f), 20) == "250 Hz");
    CHECK(p.apvts.getParameter("bA5_freq")->getText(p.apvts.getParameter("bA5_freq")->convertTo0to1(1500.0f), 20) == "1.50 kHz");
    CHECK(p.apvts.getParameter("bA3_gain")->getText(p.apvts.getParameter("bA3_gain")->convertTo0to1(3.0f), 20) == "+3.0 dB");
}
TEST(editor_snapshot) {
    OctoEqProcessor p; p.setPlayConfigDetails(2, 2, 48000, 512); p.prepareToPlay(48000, 512);
    setParam(p, "bA3_gain", 6); setParam(p, "bA6_gain", -4); setParam(p, "bA1_on", 1);
    juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi; juce::Random r(1);
    OctoEqEditor ed(p); ed.setSize(900, 480);
    for (int k = 0; k < 16; ++k) {
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) buf.setSample(c, i, r.nextFloat() * 0.5f - 0.25f);
        p.processBlock(buf, midi);
        ed.display().updateSpectrum();
    }
    auto img = ed.createComponentSnapshot(ed.getLocalBounds());
    CHECK(img.isValid() && img.getWidth() == 900);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile("snapshot.png");
    out.deleteFile(); juce::FileOutputStream os(out); juce::PNGImageFormat().writeImageToStream(img, os);
    ed.setSize(700, 400); CHECK(ed.createComponentSnapshot(ed.getLocalBounds()).isValid());
}

int main() { juce::ScopedJuceInitialiser_GUI init; return runAll(); }
