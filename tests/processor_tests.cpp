#include "test_util.h"
#include "PluginProcessor.h"

static void setParam(OctoEqProcessor& p, const char* id, float plainValue) {
    auto* param = p.apvts.getParameter(id);
    param->setValueNotifyingHost(param->convertTo0to1(plainValue));
}
static float getParam(OctoEqProcessor& p, const char* id) {
    return p.apvts.getRawParameterValue(id)->load();
}

TEST(parameter_ids_exist) {
    OctoEqProcessor p;
    for (const char* id : {"bA1_on", "bA1_type", "bA1_freq", "bA1_gain", "bA1_q", "bB8_q",
                           "mode", "edit", "scale", "output", "hiq"})
        CHECK(p.apvts.getParameter(id) != nullptr);
    CHECK_NEAR(getParam(p, "bA3_freq"), 250.0, 0.5);
    CHECK_NEAR(getParam(p, "bA1_on"), 0.0, 0);
    CHECK_NEAR(getParam(p, "bA7_type"), 5.0, 0);
    CHECK_NEAR(getParam(p, "scale"), 100.0, 0.01);
}
TEST(state_roundtrip) {
    juce::MemoryBlock mb;
    {
        OctoEqProcessor p;
        setParam(p, "bA3_gain", 4.5f); setParam(p, "mode", 2); setParam(p, "hiq", 1); setParam(p, "bB5_freq", 3210.0f);
        p.getStateInformation(mb);
    }
    OctoEqProcessor q;
    q.setStateInformation(mb.getData(), int(mb.getSize()));
    CHECK_NEAR(getParam(q, "bA3_gain"), 4.5, 0.01);
    CHECK_NEAR(getParam(q, "mode"), 2, 0);
    CHECK_NEAR(getParam(q, "hiq"), 1, 0);
    CHECK_NEAR(getParam(q, "bB5_freq"), 3210, 1);
}
TEST(silence_in_silence_out) {
    OctoEqProcessor p; p.setPlayConfigDetails(2, 2, 48000, 512); p.prepareToPlay(48000, 512);
    juce::AudioBuffer<float> buf(2, 512); buf.clear(); juce::MidiBuffer midi;
    p.processBlock(buf, midi);
    CHECK(buf.getMagnitude(0, 512) == 0.0f);
}
TEST(gain_reaches_engine) {
    OctoEqProcessor p; p.setPlayConfigDetails(2, 2, 48000, 512); p.prepareToPlay(48000, 512);
    setParam(p, "bA4_gain", 6.0f); setParam(p, "bA4_freq", 1000.0f); setParam(p, "bA4_q", 1.0f);
    juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;
    buf.clear(); p.processBlock(buf, midi);
    CHECK_NEAR(p.engine().responseDb(0, 1000), 6.0, 0.1);
}
TEST(output_gain) {
    OctoEqProcessor p; p.setPlayConfigDetails(2, 2, 48000, 512); p.prepareToPlay(48000, 512);
    setParam(p, "output", -6.0f);
    juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;
    for (int i = 0; i < 40; ++i) { for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::fill(buf.getWritePointer(c), 0.5f, 512); p.processBlock(buf, midi); }
    CHECK_NEAR(buf.getSample(0, 511), 0.5 * std::pow(10.0, -6.0 / 20.0), 0.01);
    CHECK_NEAR(buf.getSample(1, 511), 0.5 * std::pow(10.0, -6.0 / 20.0), 0.01);
}
TEST(hiq_toggle_runs) {
    OctoEqProcessor p; p.setPlayConfigDetails(2, 2, 44100, 256); p.prepareToPlay(44100, 256);
    juce::AudioBuffer<float> buf(2, 256); juce::MidiBuffer midi;
    bool finite = true;
    for (int i = 0; i < 20; ++i) {
        setParam(p, "hiq", float(i % 2));
        for (int s = 0; s < 256; ++s) buf.setSample(0, s, 0.3f * std::sin(0.1f * s)), buf.setSample(1, s, 0.3f);
        p.processBlock(buf, midi);
        for (int c = 0; c < 2; ++c) for (int s = 0; s < 256; ++s) finite &= std::isfinite(buf.getSample(c, s));
        if (i % 2 == 1) CHECK(p.getLatencySamples() > 0);
        if (i % 2 == 0) CHECK(p.getLatencySamples() == 0);
    }
    CHECK(finite);
}
TEST(mono_layout) {
    OctoEqProcessor p;
    juce::AudioProcessor::BusesLayout mono; mono.inputBuses.add(juce::AudioChannelSet::mono()); mono.outputBuses.add(juce::AudioChannelSet::mono());
    CHECK(p.isBusesLayoutSupported(mono));
    p.setBusesLayout(mono); p.prepareToPlay(48000, 128);
    juce::AudioBuffer<float> buf(1, 128); buf.clear(); buf.setSample(0, 0, 1.0f); juce::MidiBuffer midi;
    setParam(p, "hiq", 1);
    p.processBlock(buf, midi);
    CHECK(std::isfinite(buf.getSample(0, 64)));
}
TEST(spectrum_receives_audio) {
    OctoEqProcessor p; p.setPlayConfigDetails(2, 2, 48000, 512); p.prepareToPlay(48000, 512);
    juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;
    for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::fill(buf.getWritePointer(c), 0.25f, 512);
    p.processBlock(buf, midi);
    float out[1024]; int n = p.spectrum().pull(out, 1024);
    CHECK(n == 512);
}

int main() { juce::ScopedJuceInitialiser_GUI init; return runAll(); }
