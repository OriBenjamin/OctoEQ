#pragma once
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

// Frequency-response display: grid, live spectrum, combined curve and draggable band nodes.
class EqDisplay : public juce::Component, private juce::Timer {
public:
    explicit EqDisplay(OctoEqProcessor& p);
    ~EqDisplay() override { stopTimer(); }

    static constexpr float kMinFreq = 10.0f, kMaxFreq = 22000.0f, kViewDb = 18.0f;
    static float freqToX(double freq, float width);
    static double xToFreq(float x, float width);
    static float gainToY(double db, float height);
    static double yToGain(float y, float height);

    int activeSet() const;                         // 0 = A, 1 = B (Stereo always A)
    octoeq::BandSettings readBand(int set, int band) const;
    juce::Point<float> nodePosition(int band) const;
    int hitTestNode(juce::Point<float> p) const;   // -1 if none
    void setBandFromPoint(int band, juce::Point<float> p);
    void setSelectedBand(int band);
    int getSelectedBand() const { return selected; }
    std::function<void(int)> onBandSelected;
    void updateSpectrum();                         // pull analyser samples and recompute

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    juce::RangedAudioParameter* param(int band, const char* field) const;
    void setParam(int band, const char* field, float plain);
    double displayRate() const;
    void drawGrid(juce::Graphics&);
    void drawSpectrum(juce::Graphics&);
    void drawCurves(juce::Graphics&);
    void drawNodes(juce::Graphics&);

    OctoEqProcessor& proc;
    int selected = 3, hovered = -1, dragging = -1;
    float dragStartQ = 0.71f, dragStartY = 0;

    static constexpr int kFftOrder = 12, kFftSize = 1 << kFftOrder;
    juce::dsp::FFT fft { kFftOrder };
    juce::dsp::WindowingFunction<float> window { size_t(kFftSize), juce::dsp::WindowingFunction<float>::hann, false };
    std::vector<float> history = std::vector<float>(kFftSize, 0.0f);
    int historyPos = 0;
    std::vector<float> pullBuf = std::vector<float>(16384, 0.0f);
    std::vector<float> fftData = std::vector<float>(kFftSize * 2, 0.0f);
    std::vector<float> specDb = std::vector<float>(kFftSize / 2 + 1, -120.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqDisplay)
};
