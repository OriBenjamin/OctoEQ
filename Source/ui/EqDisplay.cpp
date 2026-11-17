#include "ui/EqDisplay.h"
#include "ui/Theme.h"

using namespace octoeq;

EqDisplay::EqDisplay(OctoEqProcessor& p) : proc(p) {
    setOpaque(true);
    startTimerHz(30);
}

float EqDisplay::freqToX(double f, float w) {
    return w * float(std::log(f / kMinFreq) / std::log(double(kMaxFreq) / kMinFreq));
}
double EqDisplay::xToFreq(float x, float w) {
    return kMinFreq * std::pow(double(kMaxFreq) / kMinFreq, double(x) / double(juce::jmax(1.0f, w)));
}
float EqDisplay::gainToY(double db, float h) { return h * 0.5f * (1.0f - float(db) / kViewDb); }
double EqDisplay::yToGain(float y, float h) { return kViewDb * (1.0 - 2.0 * double(y) / double(juce::jmax(1.0f, h))); }

int EqDisplay::activeSet() const {
    if (int(proc.apvts.getRawParameterValue("mode")->load()) == ModeStereo) return 0;
    return int(proc.apvts.getRawParameterValue("edit")->load()) == 1 ? 1 : 0;
}

juce::RangedAudioParameter* EqDisplay::param(int band, const char* field) const {
    return proc.apvts.getParameter(OctoEqProcessor::paramId(activeSet(), band, field));
}

void EqDisplay::setParam(int band, const char* field, float plain) {
    if (auto* p = param(band, field)) p->setValueNotifyingHost(p->convertTo0to1(plain));
}

BandSettings EqDisplay::readBand(int set, int band) const {
    auto v = [&](const char* f) { return proc.apvts.getRawParameterValue(OctoEqProcessor::paramId(set, band, f))->load(); };
    BandSettings b;
    b.enabled = v("on") > 0.5f;
    b.type = FilterType(juce::jlimit(0, kNumFilterTypes - 1, int(v("type"))));
    b.freq = v("freq"); b.gainDb = v("gain"); b.q = v("q");
    return b;
}

double EqDisplay::displayRate() const {
    const double sr = proc.getSampleRate();
    return sr > 0 ? sr : 48000.0;
}

juce::Point<float> EqDisplay::nodePosition(int band) const {
    const auto b = readBand(activeSet(), band);
    const float w = float(getWidth()), h = float(getHeight());
    return { freqToX(b.freq, w), gainToY(typeUsesGain(b.type) ? b.gainDb : 0.0f, h) };
}

int EqDisplay::hitTestNode(juce::Point<float> p) const {
    int best = -1; float bestDist = 11.0f;
    for (int b = kNumBands - 1; b >= 0; --b) {
        const float d = nodePosition(b).getDistanceFrom(p);
        if (d < bestDist) { bestDist = d; best = b; }
    }
    return best;
}

void EqDisplay::setBandFromPoint(int band, juce::Point<float> p) {
    const float w = float(getWidth()), h = float(getHeight());
    setParam(band, "freq", float(juce::jlimit<double>(kMinFreq, kMaxFreq, xToFreq(juce::jlimit(0.0f, w, p.x), w))));
    if (typeUsesGain(readBand(activeSet(), band).type))
        setParam(band, "gain", float(juce::jlimit(-15.0, 15.0, yToGain(p.y, h))));
    repaint();
}

void EqDisplay::setSelectedBand(int band) {
    if (band == selected) return;
    selected = juce::jlimit(0, kNumBands - 1, band);
    repaint();
}

// ------------------------------------------------------------------ mouse
void EqDisplay::mouseDown(const juce::MouseEvent& e) {
    dragging = hitTestNode(e.position);
    if (dragging < 0) return;
    setSelectedBand(dragging);
    if (onBandSelected) onBandSelected(dragging);
    const auto b = readBand(activeSet(), dragging);
    dragStartQ = b.q; dragStartY = e.position.y;
    param(dragging, "freq")->beginChangeGesture();
    param(dragging, typeUsesGain(b.type) ? "gain" : "q")->beginChangeGesture();
}

void EqDisplay::mouseDrag(const juce::MouseEvent& e) {
    if (dragging < 0) return;
    const auto b = readBand(activeSet(), dragging);
    if (typeUsesGain(b.type)) {
        setBandFromPoint(dragging, e.position);
    } else {
        const float w = float(getWidth());
        setParam(dragging, "freq", float(juce::jlimit<double>(kMinFreq, kMaxFreq, xToFreq(juce::jlimit(0.0f, w, e.position.x), w))));
        setParam(dragging, "q", juce::jlimit(0.1f, 18.0f, dragStartQ * std::pow(2.0f, (dragStartY - e.position.y) / 60.0f)));
        repaint();
    }
}

void EqDisplay::mouseUp(const juce::MouseEvent&) {
    if (dragging < 0) return;
    const auto b = readBand(activeSet(), dragging);
    param(dragging, "freq")->endChangeGesture();
    param(dragging, typeUsesGain(b.type) ? "gain" : "q")->endChangeGesture();
    dragging = -1;
}

void EqDisplay::mouseMove(const juce::MouseEvent& e) {
    const int h = hitTestNode(e.position);
    if (h != hovered) { hovered = h; repaint(); }
    setMouseCursor(h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void EqDisplay::mouseExit(const juce::MouseEvent&) { hovered = -1; repaint(); }

void EqDisplay::mouseDoubleClick(const juce::MouseEvent& e) {
    const int b = hitTestNode(e.position);
    if (b < 0) return;
    auto* p = param(b, "on");
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f);
    p->endChangeGesture();
    repaint();
}

void EqDisplay::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    int b = hitTestNode(e.position);
    if (b < 0) b = selected;
    auto* p = param(b, "q");
    const float q = readBand(activeSet(), b).q * std::pow(2.0f, wheel.deltaY * (wheel.isReversed ? -2.0f : 2.0f));
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(juce::jlimit(0.1f, 18.0f, q)));
    p->endChangeGesture();
    repaint();
}

// ------------------------------------------------------------------ spectrum
void EqDisplay::timerCallback() {
    updateSpectrum();
    repaint();
}

void EqDisplay::updateSpectrum() {
    const int n = proc.spectrum().pull(pullBuf.data(), int(pullBuf.size()));
    for (int i = 0; i < n; ++i) { history[size_t(historyPos)] = pullBuf[size_t(i)]; historyPos = (historyPos + 1) % kFftSize; }

    std::fill(fftData.begin(), fftData.end(), 0.0f);
    for (int i = 0; i < kFftSize; ++i) fftData[size_t(i)] = history[size_t((historyPos + i) % kFftSize)];
    window.multiplyWithWindowingTable(fftData.data(), size_t(kFftSize));
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const float norm = 4.0f / kFftSize; // Hann coherent gain 0.5, one-sided spectrum
    for (size_t i = 0; i < specDb.size(); ++i) {
        const float db = juce::Decibels::gainToDecibels(fftData[i] * norm, -120.0f);
        specDb[i] = db > specDb[i] ? db : juce::jmax(db, specDb[i] - 1.5f); // fast attack, slow release
    }
}

// ------------------------------------------------------------------ painting
void EqDisplay::paint(juce::Graphics& g) {
    g.fillAll(theme::background);
    drawGrid(g);
    drawSpectrum(g);
    drawCurves(g);
    drawNodes(g);
}

void EqDisplay::drawGrid(juce::Graphics& g) {
    const float w = float(getWidth()), h = float(getHeight());
    g.setFont(theme::font(10.0f));
    for (double decade = 10; decade <= 10000; decade *= 10)
        for (int m = 1; m <= 9; ++m) {
            const double f = decade * m;
            if (f < kMinFreq || f > kMaxFreq) continue;
            const float x = freqToX(f, w);
            const bool major = m == 1;
            g.setColour(major ? theme::grid.brighter(0.15f) : theme::grid.withAlpha(0.6f));
            g.drawVerticalLine(juce::roundToInt(x), 0.0f, h);
            if (major || m == 2 || m == 5) {
                const juce::String label = f >= 1000 ? juce::String(int(f / 1000)) + "k" : juce::String(int(f));
                g.setColour(theme::textDim);
                g.drawText(label, juce::Rectangle<float>(x + 3, h - 15, 30, 12), juce::Justification::left);
            }
        }
    for (int db = -12; db <= 12; db += 6) {
        const float y = gainToY(db, h);
        g.setColour(db == 0 ? theme::grid.brighter(0.3f) : theme::grid);
        g.drawHorizontalLine(juce::roundToInt(y), 0.0f, w);
        g.setColour(theme::textDim);
        g.drawText((db > 0 ? "+" : "") + juce::String(db), juce::Rectangle<float>(w - 30, y - 12, 26, 11), juce::Justification::right);
    }
}

void EqDisplay::drawSpectrum(juce::Graphics& g) {
    const float w = float(getWidth()), h = float(getHeight());
    const double sr = displayRate();
    juce::Path p; p.startNewSubPath(0, h);
    for (float x = 0; x <= w; x += 2.0f) {
        const double f = xToFreq(x, w);
        const double bin = f * kFftSize / sr;
        const int i0 = juce::jlimit(0, int(specDb.size()) - 2, int(bin));
        const float frac = juce::jlimit(0.0f, 1.0f, float(bin - i0));
        float db = specDb[size_t(i0)] + frac * (specDb[size_t(i0 + 1)] - specDb[size_t(i0)]);
        db += 3.0f * float(std::log2(f / 1000.0)); // +3 dB/oct tilt so pink noise reads flat
        const float y = juce::jlimit(0.0f, h, h * (-db / 90.0f));
        p.lineTo(x, y);
    }
    p.lineTo(w, h); p.closeSubPath();
    g.setColour(theme::spectrum.withAlpha(0.55f)); g.fillPath(p);
}

void EqDisplay::drawCurves(juce::Graphics& g) {
    const float w = float(getWidth()), h = float(getHeight());
    const int set = activeSet();
    const float scale = proc.apvts.getRawParameterValue("scale")->load() / 100.0f;
    const double sr = displayRate();
    BandSettings bands[kNumBands];
    for (int b = 0; b < kNumBands; ++b) bands[b] = readBand(set, b);

    juce::Path total, sel;
    const float zeroY = gainToY(0, h);
    sel.startNewSubPath(0, zeroY);
    for (int px = 0; px <= int(w); ++px) {
        const float x = float(px);
        const double f = xToFreq(x, w);
        double sum = 0;
        for (int b = 0; b < kNumBands; ++b) {
            const double r = bandResponseDb(bands[b], scale, f, sr);
            sum += r;
            if (b == selected) sel.lineTo(x, juce::jlimit(-50.0f, h + 50.0f, gainToY(r, h)));
        }
        const float y = juce::jlimit(-50.0f, h + 50.0f, gainToY(sum, h));
        if (px == 0) total.startNewSubPath(x, y); else total.lineTo(x, y);
    }
    sel.lineTo(w, zeroY); sel.closeSubPath();

    if (bands[selected].enabled) {
        g.setColour(theme::band(selected).withAlpha(0.22f)); g.fillPath(sel);
        g.setColour(theme::band(selected).withAlpha(0.7f)); g.strokePath(sel, juce::PathStrokeType(1.0f));
    }
    g.setColour(theme::curve);
    g.strokePath(total, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void EqDisplay::drawNodes(juce::Graphics& g) {
    const int set = activeSet();
    g.setFont(theme::font(11.0f, true));
    for (int b = 0; b < kNumBands; ++b) {
        const auto band = readBand(set, b);
        const auto pos = nodePosition(b);
        const float r = (b == hovered || b == dragging) ? 10.0f : 8.5f;
        const auto circle = juce::Rectangle<float>(r * 2, r * 2).withCentre(pos);
        g.setColour(theme::band(b).withAlpha(band.enabled ? 1.0f : 0.3f));
        g.fillEllipse(circle);
        if (b == selected) { g.setColour(theme::curve); g.drawEllipse(circle.expanded(2.0f), 1.5f); }
        g.setColour(band.enabled ? theme::background : theme::textDim);
        g.drawText(juce::String(b + 1), circle, juce::Justification::centred);
    }
}
