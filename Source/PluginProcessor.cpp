#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace octoeq;

// ---------------------------------------------------------------- SpectrumFifo
void SpectrumFifo::push(const float* mono, int n) {
    const auto scope = fifo.write(std::min(n, fifo.getFreeSpace()));
    if (scope.blockSize1 > 0) std::copy(mono, mono + scope.blockSize1, data.begin() + scope.startIndex1);
    if (scope.blockSize2 > 0) std::copy(mono + scope.blockSize1, mono + scope.blockSize1 + scope.blockSize2, data.begin() + scope.startIndex2);
}

int SpectrumFifo::pull(float* dest, int maxSamples) {
    const auto scope = fifo.read(std::min(maxSamples, fifo.getNumReady()));
    if (scope.blockSize1 > 0) std::copy(data.begin() + scope.startIndex1, data.begin() + scope.startIndex1 + scope.blockSize1, dest);
    if (scope.blockSize2 > 0) std::copy(data.begin() + scope.startIndex2, data.begin() + scope.startIndex2 + scope.blockSize2, dest + scope.blockSize1);
    return scope.blockSize1 + scope.blockSize2;
}

// ---------------------------------------------------------------- parameters
static juce::NormalisableRange<float> logRange(float lo, float hi, float interval) {
    return { lo, hi,
             [](float s, float e, float v) { return s * std::pow(e / s, v); },
             [](float s, float e, float v) { return std::log(v / s) / std::log(e / s); },
             [interval](float s, float e, float v) { return juce::jlimit(s, e, interval > 0 ? std::round(v / interval) * interval : v); } };
}

static juce::String freqText(float v, int) {
    return v < 1000.0f ? juce::String(juce::roundToInt(v)) + " Hz" : juce::String(v / 1000.0f, 2) + " kHz";
}
static float freqFromText(const juce::String& t) {
    const float v = t.getFloatValue();
    return t.containsIgnoreCase("k") ? v * 1000.0f : v;
}
static juce::String dbText(float v, int) { return (v >= 0 ? "+" : "") + juce::String(v, 1) + " dB"; }

const juce::StringArray& filterTypeNames() {
    static const juce::StringArray names { "Low Cut 12", "Low Cut 48", "Low Shelf", "Bell", "Notch", "High Shelf", "High Cut 12", "High Cut 48" };
    return names;
}

juce::String OctoEqProcessor::paramId(int set, int band, const char* field) {
    return juce::String("b") + (set == 0 ? "A" : "B") + juce::String(band + 1) + "_" + field;
}

juce::AudioProcessorValueTreeState::ParameterLayout OctoEqProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    for (int s = 0; s < kNumSets; ++s)
        for (int b = 0; b < kNumBands; ++b) {
            const auto d = defaultBand(b);
            const String prefix = String(s == 0 ? "A" : "B") + String(b + 1) + " ";
            auto group = std::make_unique<AudioProcessorParameterGroup>(paramId(s, b, "grp"), prefix.trim(), "|");
            group->addChild(std::make_unique<AudioParameterBool>(ParameterID { paramId(s, b, "on"), 1 }, prefix + "On", d.enabled));
            group->addChild(std::make_unique<AudioParameterChoice>(ParameterID { paramId(s, b, "type"), 1 }, prefix + "Type", filterTypeNames(), int(d.type)));
            group->addChild(std::make_unique<AudioParameterFloat>(ParameterID { paramId(s, b, "freq"), 1 }, prefix + "Freq", logRange(10.0f, 22000.0f, 0.0f), d.freq,
                AudioParameterFloatAttributes().withStringFromValueFunction(freqText).withValueFromStringFunction(freqFromText)));
            group->addChild(std::make_unique<AudioParameterFloat>(ParameterID { paramId(s, b, "gain"), 1 }, prefix + "Gain", NormalisableRange<float>(-15.0f, 15.0f, 0.01f), d.gainDb,
                AudioParameterFloatAttributes().withStringFromValueFunction(dbText)));
            group->addChild(std::make_unique<AudioParameterFloat>(ParameterID { paramId(s, b, "q"), 1 }, prefix + "Q", logRange(0.1f, 18.0f, 0.0f), d.q,
                AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return String(v, 2); })));
            layout.add(std::move(group));
        }
    layout.add(std::make_unique<AudioParameterChoice>(ParameterID { "mode", 1 }, "Mode", StringArray { "Stereo", "L/R", "M/S" }, 0));
    layout.add(std::make_unique<AudioParameterChoice>(ParameterID { "edit", 1 }, "Edit", StringArray { "A", "B" }, 0,
        AudioParameterChoiceAttributes().withAutomatable(false)));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "scale", 1 }, "Scale", NormalisableRange<float>(-200.0f, 200.0f, 0.1f), 100.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return String(roundToInt(v)) + " %"; })));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "output", 1 }, "Output", NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction(dbText)));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "hiq", 1 }, "Hi-Q", false));
    return layout;
}

// ---------------------------------------------------------------- processor
OctoEqProcessor::OctoEqProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "OctoEQ", createLayout()) {
    for (int s = 0; s < kNumSets; ++s)
        for (int b = 0; b < kNumBands; ++b)
            bandParams[s][b] = { apvts.getRawParameterValue(paramId(s, b, "on")), apvts.getRawParameterValue(paramId(s, b, "type")),
                                 apvts.getRawParameterValue(paramId(s, b, "freq")), apvts.getRawParameterValue(paramId(s, b, "gain")),
                                 apvts.getRawParameterValue(paramId(s, b, "q")) };
    modeParam = apvts.getRawParameterValue("mode");
    scaleParam = apvts.getRawParameterValue("scale");
    outputParam = apvts.getRawParameterValue("output");
    hiqParam = apvts.getRawParameterValue("hiq");
}

bool OctoEqProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void OctoEqProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    baseRate = sampleRate > 0 ? sampleRate : 44100;
    maxBlock = std::max(1, samplesPerBlock);
    const int numCh = std::max(1, getTotalNumOutputChannels());
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        size_t(numCh), 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampler->initProcessing(size_t(maxBlock));
    monoScratch.allocate(size_t(maxBlock), true);
    outGain.reset(baseRate, 0.02);
    outGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputParam->load()));
    hiqActive = hiqParam->load() > 0.5f;
    syncParameters();
    eq.prepare(hiqActive ? baseRate * 2 : baseRate, hiqActive ? maxBlock * 2 : maxBlock);
    setLatencySamples(hiqActive ? int(std::round(oversampler->getLatencyInSamples())) : 0);
}

void OctoEqProcessor::applyHiQ(bool on) {
    if (on == hiqActive || oversampler == nullptr) return;
    hiqActive = on;
    oversampler->reset();
    eq.prepare(on ? baseRate * 2 : baseRate, on ? maxBlock * 2 : maxBlock);
    setLatencySamples(on ? int(std::round(oversampler->getLatencyInSamples())) : 0);
}

void OctoEqProcessor::syncParameters() {
    for (int s = 0; s < kNumSets; ++s)
        for (int b = 0; b < kNumBands; ++b) {
            const auto& p = bandParams[s][b];
            BandSettings bs;
            bs.enabled = p.on->load() > 0.5f;
            bs.type = FilterType(juce::jlimit(0, kNumFilterTypes - 1, int(p.type->load())));
            bs.freq = p.freq->load();
            bs.gainDb = p.gain->load();
            bs.q = p.q->load();
            eq.setBand(s, b, bs);
        }
    eq.setMode(int(modeParam->load()));
    eq.setScale(scaleParam->load() / 100.0f);
}

void OctoEqProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min(buffer.getNumChannels(), getTotalNumOutputChannels());
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, n);
    if (n <= 0 || numCh <= 0 || oversampler == nullptr) return;

    applyHiQ(hiqParam->load() > 0.5f);
    syncParameters();

    // Hosts may exceed the announced block size; process in chunks we prepared for.
    for (int start = 0; start < n; start += maxBlock) {
        const int len = std::min(maxBlock, n - start);
        juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(), size_t(numCh), size_t(start), size_t(len));
        if (hiqActive && size_t(numCh) == oversampler->numChannels) {
            auto up = oversampler->processSamplesUp(block);
            float* ptrs[2] = { up.getChannelPointer(0), up.getNumChannels() > 1 ? up.getChannelPointer(1) : nullptr };
            eq.process(ptrs, int(up.getNumChannels()), int(up.getNumSamples()));
            oversampler->processSamplesDown(block);
        } else {
            float* ptrs[2] = { buffer.getWritePointer(0, start), numCh > 1 ? buffer.getWritePointer(1, start) : nullptr };
            eq.process(ptrs, numCh, len);
        }
    }

    outGain.setTargetValue(juce::Decibels::decibelsToGain(outputParam->load()));
    if (outGain.isSmoothing()) {
        for (int i = 0; i < n; ++i) {
            const float g = outGain.getNextValue();
            for (int c = 0; c < numCh; ++c) buffer.getWritePointer(c)[i] *= g;
        }
    } else {
        for (int c = 0; c < numCh; ++c) buffer.applyGain(c, 0, n, outGain.getTargetValue());
    }

    // Post-EQ mono sum for the analyser.
    for (int start = 0; start < n; start += maxBlock) {
        const int len = std::min(maxBlock, n - start);
        juce::FloatVectorOperations::copy(monoScratch.get(), buffer.getReadPointer(0, start), len);
        if (numCh > 1) {
            juce::FloatVectorOperations::add(monoScratch.get(), buffer.getReadPointer(1, start), len);
            juce::FloatVectorOperations::multiply(monoScratch.get(), 0.5f, len);
        }
        fifo.push(monoScratch.get(), len);
    }
}

juce::AudioProcessorEditor* OctoEqProcessor::createEditor() { return new OctoEqEditor(*this); }

void OctoEqProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void OctoEqProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OctoEqProcessor(); }
