#pragma once

namespace octoeq {

enum class FilterType { LowCut12, LowCut48, LowShelf, Bell, Notch, HighShelf, HighCut12, HighCut48 };
constexpr int kNumFilterTypes = 8;

struct BiquadCoeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

int stagesFor(FilterType t);
BiquadCoeffs designStage(FilterType t, int stage, double freq, double gainDb, double q, double sampleRate);
BiquadCoeffs designBiquad(FilterType t, double freq, double gainDb, double q, double sampleRate);
double magnitudeDb(const BiquadCoeffs& c, double freq, double sampleRate);

struct Biquad {
    BiquadCoeffs c;
    double z1 = 0, z2 = 0;
    float process(float x);
    void reset() { z1 = z2 = 0; }
};

} // namespace octoeq
