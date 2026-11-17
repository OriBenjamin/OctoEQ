#include "dsp/EqEngine.h"
#include <algorithm>
#include <cmath>

namespace octoeq {
namespace {
constexpr int kSubBlock = 32;
constexpr double kRampSeconds = 0.020;
}

BandSettings defaultBand(int band) {
    static const FilterType types[kNumBands] = {
        FilterType::LowCut12, FilterType::LowShelf, FilterType::Bell, FilterType::Bell,
        FilterType::Bell, FilterType::Bell, FilterType::HighShelf, FilterType::HighCut12 };
    static const float freqs[kNumBands] = { 30, 100, 250, 600, 1500, 4000, 8000, 18000 };
    band = std::clamp(band, 0, kNumBands - 1);
    BandSettings s;
    s.enabled = band != 0 && band != kNumBands - 1;
    s.type = types[band];
    s.freq = freqs[band];
    s.gainDb = 0;
    s.q = 0.71f;
    return s;
}

bool typeUsesGain(FilterType t) {
    return t == FilterType::Bell || t == FilterType::LowShelf || t == FilterType::HighShelf;
}

EqEngine::EqEngine() {
    for (int s = 0; s < kNumSets; ++s)
        for (int b = 0; b < kNumBands; ++b) target[s][b] = defaultBand(b);
    prepare(44100, 512);
}

void EqEngine::prepare(double sampleRate, int) {
    sr = sampleRate > 0 ? sampleRate : 44100;
    snapSmoothing();
    for (auto& ch : filt) for (auto& b : ch) for (auto& st : b) st.reset();
}

void EqEngine::snapSmoothing() {
    for (int s = 0; s < kNumSets; ++s)
        for (int b = 0; b < kNumBands; ++b) {
            cur[s][b] = { std::log(double(target[s][b].freq)), double(target[s][b].gainDb), double(target[s][b].q) };
            updateCoeffs(s, b);
        }
}

void EqEngine::setBand(int set, int band, const BandSettings& in) {
    if (set < 0 || set >= kNumSets || band < 0 || band >= kNumBands) return;
    BandSettings s = in;
    if (!std::isfinite(s.freq)) s.freq = 1000;
    if (!std::isfinite(s.gainDb)) s.gainDb = 0;
    if (!std::isfinite(s.q)) s.q = 0.71f;
    s.freq = std::clamp(s.freq, 10.0f, 22000.0f);
    s.gainDb = std::clamp(s.gainDb, -15.0f, 15.0f);
    s.q = std::clamp(s.q, 0.1f, 18.0f);

    const BandSettings old = target[set][band];
    target[set][band] = s;
    if (old.type != s.type || old.enabled != s.enabled) {
        // Structural change: jump straight to the new settings and clear state on
        // every channel that might use this set (avoids unstable state carry-over).
        cur[set][band] = { std::log(double(s.freq)), double(s.gainDb), double(s.q) };
        for (auto& ch : filt) for (auto& st : ch[band]) st.reset();
        updateCoeffs(set, band);
    }
}

void EqEngine::setMode(int m) {
    m = std::clamp(m, 0, 2);
    if (m == mode) return;
    mode = m;
    for (auto& ch : filt) for (auto& b : ch) for (auto& st : b) st.reset();
}

void EqEngine::setScale(float s) {
    if (!std::isfinite(s)) s = 1.0f;
    scale = std::clamp(s, -2.0f, 2.0f);
    for (int set = 0; set < kNumSets; ++set)
        for (int b = 0; b < kNumBands; ++b) updateCoeffs(set, b);
}

float EqEngine::effectiveGain(int set, int band, double gain) const {
    return typeUsesGain(target[set][band].type) ? float(gain * scale) : float(gain);
}

void EqEngine::updateCoeffs(int set, int band) {
    const auto& t = target[set][band];
    const auto& c = cur[set][band];
    const double g = effectiveGain(set, band, c.gain);
    for (int st = 0; st < stagesFor(t.type); ++st)
        coeffs[set][band][st] = designStage(t.type, st, std::exp(c.logFreq), g, c.q, sr);
}

bool EqEngine::advanceSmoothing(int samples) {
    const double frac = std::min(1.0, samples / (kRampSeconds * sr));
    bool any = false;
    for (int s = 0; s < kNumSets; ++s)
        for (int b = 0; b < kNumBands; ++b) {
            const auto& t = target[s][b];
            auto& c = cur[s][b];
            const double tf = std::log(double(t.freq)), tg = t.gainDb, tq = t.q;
            if (c.logFreq == tf && c.gain == tg && c.q == tq) continue;
            auto step = [frac](double& v, double goal, double full) {
                const double maxStep = full * frac;
                const double d = goal - v;
                v = std::fabs(d) <= maxStep ? goal : v + (d > 0 ? maxStep : -maxStep);
            };
            // Ramp each quantity across its full range in kRampSeconds.
            step(c.logFreq, tf, std::log(22000.0 / 10.0));
            step(c.gain, tg, 30.0);
            step(c.q, tq, 17.9);
            updateCoeffs(s, b);
            any = true;
        }
    return any;
}

void EqEngine::process(float* const* ch, int numCh, int numSamples) {
    if (numSamples <= 0 || numCh <= 0 || ch == nullptr) return;
    const int nch = std::min(numCh, 2);
    const bool ms = mode == ModeMS && nch == 2;
    const int setFor[2] = { 0, (mode == ModeStereo || nch < 2) ? 0 : 1 };

    for (int start = 0; start < numSamples; start += kSubBlock) {
        const int n = std::min(kSubBlock, numSamples - start);
        advanceSmoothing(n);

        if (ms)
            for (int i = start; i < start + n; ++i) {
                const float l = ch[0][i], r = ch[1][i];
                ch[0][i] = 0.5f * (l + r);
                ch[1][i] = 0.5f * (l - r);
            }

        for (int c = 0; c < nch; ++c) {
            const int set = setFor[c];
            for (int b = 0; b < kNumBands; ++b) {
                const auto& t = target[set][b];
                if (!t.enabled) continue;
                const int stages = stagesFor(t.type);
                for (int st = 0; st < stages; ++st) {
                    Biquad& f = filt[c][b][st];
                    f.c = coeffs[set][b][st];
                    for (int i = start; i < start + n; ++i) ch[c][i] = f.process(ch[c][i]);
                }
            }
        }

        if (ms)
            for (int i = start; i < start + n; ++i) {
                const float m = ch[0][i], s = ch[1][i];
                ch[0][i] = m + s;
                ch[1][i] = m - s;
            }
    }
}

double EqEngine::bandResponseDb(int set, int band, double freq) const {
    if (set < 0 || set >= kNumSets || band < 0 || band >= kNumBands) return 0;
    const auto& t = target[set][band];
    if (!t.enabled) return 0;
    const double g = effectiveGain(set, band, t.gainDb);
    double sum = 0;
    for (int st = 0; st < stagesFor(t.type); ++st)
        sum += magnitudeDb(designStage(t.type, st, t.freq, g, t.q, sr), freq, sr);
    return sum;
}

double EqEngine::responseDb(int set, double freq) const {
    double sum = 0;
    for (int b = 0; b < kNumBands; ++b) sum += bandResponseDb(set, b, freq);
    return sum;
}

} // namespace octoeq
