#pragma once
#include "dsp/Biquad.h"
#include <array>

namespace octoeq {

constexpr int kNumBands = 8;
constexpr int kNumSets = 2;
constexpr int kMaxStages = 4;

enum Mode { ModeStereo = 0, ModeLR = 1, ModeMS = 2 };

struct BandSettings {
    bool enabled = true;
    FilterType type = FilterType::Bell;
    float freq = 1000.0f, gainDb = 0.0f, q = 0.71f;
};

// Factory defaults per band index 0..7 (see design doc).
BandSettings defaultBand(int band);
bool typeUsesGain(FilterType t);
// Response of one band in dB at `freq`; `scale` multiplies gain for bell/shelf types. 0 when disabled.
double bandResponseDb(const BandSettings& b, float scale, double freq, double sampleRate);

class EqEngine {
public:
    EqEngine();
    void prepare(double sampleRate, int maxBlock);
    void setBand(int set, int band, const BandSettings& s);
    void setMode(int mode);
    void setScale(float scale);
    void process(float* const* ch, int numCh, int numSamples);
    double responseDb(int set, double freq) const;
    double bandResponseDb(int set, int band, double freq) const;
    double sampleRate() const { return sr; }
    const BandSettings& band(int set, int b) const { return target[set][b]; }

private:
    struct Smoothed { double logFreq = 0, gain = 0, q = 0; };
    int setForChannel(int ch) const;
    void snapSmoothing();
    bool advanceSmoothing(int samples);
    void updateCoeffs(int set, int band);
    float effectiveGain(int set, int band, double gain) const;

    double sr = 44100;
    int mode = ModeStereo;
    float scale = 1.0f;
    BandSettings target[kNumSets][kNumBands];
    Smoothed cur[kNumSets][kNumBands];
    BiquadCoeffs coeffs[kNumSets][kNumBands][kMaxStages];
    Biquad filt[2][kNumBands][kMaxStages]; // per processed channel
};

} // namespace octoeq
