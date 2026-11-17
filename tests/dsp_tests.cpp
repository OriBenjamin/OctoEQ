#include "test_util.h"
#include "dsp/Biquad.h"
#include <random>

using namespace octoeq;

TEST(bell_peak) {
    auto c = designBiquad(FilterType::Bell, 1000, 6, 1, 48000);
    CHECK_NEAR(magnitudeDb(c, 1000, 48000), 6.0, 0.05);
    CHECK_NEAR(magnitudeDb(c, 100, 48000), 0.0, 0.5);
}
TEST(lowshelf) {
    auto c = designBiquad(FilterType::LowShelf, 200, 6, 0.71, 48000);
    CHECK_NEAR(magnitudeDb(c, 20, 48000), 6.0, 0.3);
    CHECK_NEAR(magnitudeDb(c, 10000, 48000), 0.0, 0.3);
}
TEST(lowcut12) {
    auto c = designBiquad(FilterType::LowCut12, 1000, 0, 0.7071, 48000);
    CHECK_NEAR(magnitudeDb(c, 1000, 48000), -3.0, 0.2);
    CHECK_NEAR(magnitudeDb(c, 250, 48000), -24.0, 1.5);
}
static double cascadeDb(FilterType t, double f0, double q, double f, double sr) {
    double sum = 0;
    for (int s = 0; s < stagesFor(t); ++s) sum += magnitudeDb(designStage(t, s, f0, 0, q, sr), f, sr);
    return sum;
}
TEST(lowcut48) {
    CHECK(stagesFor(FilterType::LowCut48) == 4);
    CHECK(stagesFor(FilterType::Bell) == 1);
    CHECK(cascadeDb(FilterType::LowCut48, 1000, 0.7071, 500, 48000) <= -45.0);
    CHECK_NEAR(cascadeDb(FilterType::LowCut48, 1000, 0.7071, 1000, 48000), -3.0, 0.5);
}
TEST(highcut48) {
    CHECK(cascadeDb(FilterType::HighCut48, 1000, 0.7071, 2000, 48000) <= -45.0);
    CHECK_NEAR(cascadeDb(FilterType::HighCut48, 1000, 0.7071, 1000, 48000), -3.0, 0.5);
}
TEST(highshelf) {
    auto c = designBiquad(FilterType::HighShelf, 5000, -6, 0.71, 48000);
    CHECK_NEAR(magnitudeDb(c, 20000, 48000), -6.0, 0.4);
    CHECK_NEAR(magnitudeDb(c, 100, 48000), 0.0, 0.3);
}
TEST(notch) {
    auto c = designBiquad(FilterType::Notch, 1000, 0, 1, 48000);
    CHECK(magnitudeDb(c, 1000, 48000) <= -40.0);
}
TEST(nyquist_clamp) {
    Biquad b; b.c = designBiquad(FilterType::Bell, 22000, 15, 1, 44100);
    std::mt19937 rng(1); std::uniform_real_distribution<float> d(-1, 1);
    bool finite = true;
    for (int i = 0; i < 4096; ++i) finite &= std::isfinite(b.process(d(rng)));
    CHECK(finite);
}

int main() { return runAll(); }
