#include "dsp/Biquad.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace octoeq {
namespace {
constexpr double kPi = 3.14159265358979323846;
// Stage Qs of an 8th-order Butterworth, low Q first so the resonant stage is last.
constexpr double kButter8Q[4] = { 0.50979558, 0.60134489, 0.89997622, 2.56291545 };

BiquadCoeffs normalise(double b0, double b1, double b2, double a0, double a1, double a2) {
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}
} // namespace

int stagesFor(FilterType t) {
    return (t == FilterType::LowCut48 || t == FilterType::HighCut48) ? 4 : 1;
}

BiquadCoeffs designStage(FilterType t, int stage, double freq, double gainDb, double q, double sampleRate) {
    if (!(sampleRate > 0)) return {};
    freq = std::clamp(std::isfinite(freq) ? freq : 1000.0, 10.0, 0.49 * sampleRate);
    q = std::clamp(std::isfinite(q) ? q : 0.7071, 0.1, 18.0);
    if (!std::isfinite(gainDb)) gainDb = 0;

    if (t == FilterType::LowCut48 || t == FilterType::HighCut48) {
        stage = std::clamp(stage, 0, 3);
        double sq = kButter8Q[stage];
        if (stage == 3) sq *= q / 0.70710678;
        q = std::clamp(sq, 0.1, 40.0);
    }

    const double w0 = 2 * kPi * freq / sampleRate;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2 * q);
    const double A = std::pow(10.0, gainDb / 40.0);

    switch (t) {
        case FilterType::LowCut12:
        case FilterType::LowCut48:
            return normalise((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + alpha, -2 * cw, 1 - alpha);
        case FilterType::HighCut12:
        case FilterType::HighCut48:
            return normalise((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + alpha, -2 * cw, 1 - alpha);
        case FilterType::Bell:
            return normalise(1 + alpha * A, -2 * cw, 1 - alpha * A, 1 + alpha / A, -2 * cw, 1 - alpha / A);
        case FilterType::Notch:
            return normalise(1, -2 * cw, 1, 1 + alpha, -2 * cw, 1 - alpha);
        case FilterType::LowShelf: {
            const double s = 2 * std::sqrt(A) * alpha;
            return normalise(A * ((A + 1) - (A - 1) * cw + s), 2 * A * ((A - 1) - (A + 1) * cw),
                             A * ((A + 1) - (A - 1) * cw - s), (A + 1) + (A - 1) * cw + s,
                             -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - s);
        }
        case FilterType::HighShelf: {
            const double s = 2 * std::sqrt(A) * alpha;
            return normalise(A * ((A + 1) + (A - 1) * cw + s), -2 * A * ((A - 1) + (A + 1) * cw),
                             A * ((A + 1) + (A - 1) * cw - s), (A + 1) - (A - 1) * cw + s,
                             2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - s);
        }
    }
    return {};
}

BiquadCoeffs designBiquad(FilterType t, double freq, double gainDb, double q, double sampleRate) {
    return designStage(t, 0, freq, gainDb, q, sampleRate);
}

double magnitudeDb(const BiquadCoeffs& c, double freq, double sampleRate) {
    const double w = 2 * kPi * freq / sampleRate;
    const std::complex<double> z1 = std::polar(1.0, -w), z2 = z1 * z1;
    const auto num = c.b0 + c.b1 * z1 + c.b2 * z2;
    const auto den = 1.0 + c.a1 * z1 + c.a2 * z2;
    const double mag = std::abs(num) / std::max(std::abs(den), 1e-30);
    return 20 * std::log10(std::max(mag, 1e-12));
}

float Biquad::process(float xf) {
    const double x = xf;
    const double y = c.b0 * x + z1;
    z1 = c.b1 * x - c.a1 * y + z2;
    z2 = c.b2 * x - c.a2 * y;
    // Flush denormals so silence stays cheap.
    if (std::fabs(z1) < 1e-25) z1 = 0;
    if (std::fabs(z2) < 1e-25) z2 = 0;
    return static_cast<float>(y);
}

} // namespace octoeq
