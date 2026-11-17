#include "test_util.h"
#include "dsp/EqEngine.h"
#include <vector>

using namespace octoeq;
static constexpr double kPi = 3.14159265358979323846;

static double rmsDb(const std::vector<float>& v, size_t from) {
    double s = 0; for (size_t i = from; i < v.size(); ++i) s += double(v[i]) * v[i];
    return 10 * std::log10(s / double(v.size() - from) + 1e-30);
}
static void runSine(EqEngine& e, std::vector<float>& l, std::vector<float>& r, double f, double sr, float amp) {
    for (size_t i = 0; i < l.size(); ++i) l[i] = r[i] = amp * float(std::sin(2 * kPi * f * double(i) / sr));
    for (size_t off = 0; off < l.size(); off += 512) {
        int n = int(std::min<size_t>(512, l.size() - off));
        float* ch[2] = { l.data() + off, r.data() + off };
        e.process(ch, 2, n);
    }
}

TEST(defaults_match_design) {
    const int types[8] = {0, 2, 3, 3, 3, 3, 5, 6};
    const bool on[8] = {false, true, true, true, true, true, true, false};
    const float freqs[8] = {30, 100, 250, 600, 1500, 4000, 8000, 18000};
    for (int b = 0; b < 8; ++b) {
        auto d = defaultBand(b);
        CHECK(int(d.type) == types[b]); CHECK(d.enabled == on[b]);
        CHECK_NEAR(d.freq, freqs[b], 0.01); CHECK_NEAR(d.gainDb, 0, 0); CHECK_NEAR(d.q, 0.71, 0.001);
    }
}
TEST(defaults_flat) {
    EqEngine e; e.prepare(48000, 512);
    for (double f : {200.0, 1000.0, 5000.0}) CHECK_NEAR(e.responseDb(0, f), 0.0, 0.1);
}
TEST(scale) {
    EqEngine e; e.prepare(48000, 512);
    BandSettings b = defaultBand(3); b.freq = 1000; b.gainDb = 6; b.q = 1; e.setBand(0, 3, b);
    CHECK_NEAR(e.responseDb(0, 1000), 6.0, 0.1);
    e.setScale(-1.0f); CHECK_NEAR(e.responseDb(0, 1000), -6.0, 0.1);
    e.setScale(0.0f);  CHECK_NEAR(e.responseDb(0, 1000), 0.0, 0.1);
}
TEST(band_response) {
    EqEngine e; e.prepare(48000, 512);
    BandSettings b = defaultBand(3); b.freq = 1000; b.gainDb = 6; b.q = 1; e.setBand(0, 3, b);
    CHECK_NEAR(e.bandResponseDb(0, 3, 1000), 6.0, 0.1);
    CHECK_NEAR(e.bandResponseDb(0, 2, 1000), 0.0, 0.1);
}
TEST(lr_mode) {
    EqEngine e; e.prepare(48000, 512); e.setMode(ModeLR);
    BandSettings b = defaultBand(2); b.freq = 1000; b.gainDb = 12; b.q = 1; e.setBand(1, 2, b);
    std::vector<float> l(48000), r(48000);
    runSine(e, l, r, 1000, 48000, 0.1f);
    CHECK_NEAR(rmsDb(r, 24000) - rmsDb(l, 24000), 12.0, 0.5);
}
TEST(ms_mode) {
    EqEngine e; e.prepare(48000, 512); e.setMode(ModeMS);
    BandSettings b = defaultBand(2); b.freq = 1000; b.gainDb = 12; b.q = 1; e.setBand(1, 2, b);
    std::vector<float> l(48000), r(48000);
    runSine(e, l, r, 1000, 48000, 0.1f);
    float maxErr = 0;
    for (size_t i = 24000; i < l.size(); ++i) {
        float ref = 0.1f * float(std::sin(2 * kPi * 1000 * double(i) / 48000));
        maxErr = std::max({maxErr, std::fabs(l[i] - ref), std::fabs(r[i] - ref)});
    }
    CHECK(maxErr < 1e-4f);
}
TEST(mono_bus) {
    EqEngine e; e.prepare(48000, 512); e.setMode(ModeMS);
    BandSettings b = defaultBand(3); b.gainDb = 10; e.setBand(0, 3, b);
    std::vector<float> m(512, 0.3f); float* ch[1] = { m.data() };
    e.process(ch, 1, 512);
    bool finite = true; for (float x : m) finite &= std::isfinite(x);
    CHECK(finite);
}
TEST(reprepare_and_tiny_blocks) {
    EqEngine e; e.prepare(44100, 512);
    BandSettings b = defaultBand(3); b.freq = 1000; b.gainDb = 9; b.q = 1; e.setBand(0, 3, b);
    std::vector<float> l(512, 0.2f), r(512, 0.2f); float* ch[2] = { l.data(), r.data() };
    e.process(ch, 2, 512);
    e.prepare(96000, 1);
    CHECK_NEAR(e.sampleRate(), 96000, 0);
    bool finite = true;
    for (int i = 0; i < 1000; ++i) { float a = 0.2f, c = 0.2f; float* p[2] = { &a, &c }; e.process(p, 2, 1); finite &= std::isfinite(a) && std::isfinite(c); }
    e.process(ch, 2, 0);
    CHECK(finite);
    CHECK_NEAR(e.responseDb(0, 1000), magnitudeDb(designBiquad(FilterType::Bell, 1000, 9, 1, 96000), 1000, 96000), 0.01);
}
TEST(param_jump) {
    EqEngine e; e.prepare(48000, 512);
    BandSettings b = defaultBand(3); b.freq = 1000; b.q = 1; e.setBand(0, 3, b);
    const int N = 9600; std::vector<float> l(N), r(N);
    for (int i = 0; i < N; ++i) l[i] = r[i] = 0.25f * float(std::sin(2 * kPi * 1000 * i / 48000.0));
    float* ch[2] = { l.data(), r.data() };
    e.process(ch, 2, 4800 + 100);
    b.gainDb = 15; e.setBand(0, 3, b);
    float* ch2[2] = { l.data() + 4900, r.data() + 4900 };
    e.process(ch2, 2, N - 4900);
    float maxDelta = 0; bool finite = true;
    for (int i = 1; i < N; ++i) { finite &= std::isfinite(l[i]); maxDelta = std::max(maxDelta, std::fabs(l[i] - l[i - 1])); }
    CHECK(finite); CHECK(maxDelta < 0.5f);
    CHECK_NEAR(rmsDb(l, N - 2400) - 20 * std::log10(0.25 / std::sqrt(2.0)), 15.0, 0.3);
}

int main() { return runAll(); }
