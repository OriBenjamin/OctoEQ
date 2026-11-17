#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp/EqEngine.h"

// Lock-free single-producer/single-consumer FIFO of post-EQ mono samples for the analyser.
class SpectrumFifo {
public:
    void push(const float* mono, int n);
    int pull(float* dest, int maxSamples);
private:
    static constexpr int kSize = 16384;
    juce::AbstractFifo fifo { kSize };
    std::array<float, kSize> data {};
};

class OctoEqProcessor : public juce::AudioProcessor {
public:
    OctoEqProcessor();
    ~OctoEqProcessor() override = default;

    static juce::String paramId(int set, int band, const char* field); // set 0/1, band 0..7
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "OctoEQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    octoeq::EqEngine& engine() { return eq; }
    SpectrumFifo& spectrum() { return fifo; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void syncParameters();
    void applyHiQ(bool on);

    struct BandParams { std::atomic<float>* on; std::atomic<float>* type; std::atomic<float>* freq; std::atomic<float>* gain; std::atomic<float>* q; };
    BandParams bandParams[octoeq::kNumSets][octoeq::kNumBands] {};
    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* scaleParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* hiqParam = nullptr;

    octoeq::EqEngine eq;
    SpectrumFifo fifo;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    bool hiqActive = false;
    double baseRate = 44100;
    int maxBlock = 512;
    juce::LinearSmoothedValue<float> outGain;
    juce::HeapBlock<float> monoScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctoEqProcessor)
};
