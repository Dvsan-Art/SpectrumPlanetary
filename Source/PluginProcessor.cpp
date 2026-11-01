#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrumProcessor::SpectrumProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    for (int i = 0; i < 3; ++i)
        ffts[i] = std::make_unique<juce::dsp::FFT> (12 + i);
    fftData.assign (2 * 16384, 0.0f);
    dbArr.assign (8192, -150.0f);
    winTab.assign (16384, 1.0f);
    resetState();
}

bool SpectrumProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SpectrumProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    subLen = (int) (sr * 0.1);
    setupFilters();
    resetState();
}

void SpectrumProcessor::setupFilters()
{
    const double pi = juce::MathConstants<double>::pi;
    double f = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
    double K = std::tan (pi * f / sr), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
    double a0 = 1.0 + K / Q + K * K;
    Biquad s;
    s.b0 = (Vh + Vb * K / Q + K * K) / a0;
    s.b1 = 2.0 * (K * K - Vh) / a0;
    s.b2 = (Vh - Vb * K / Q + K * K) / a0;
    s.a1 = 2.0 * (K * K - 1.0) / a0;
    s.a2 = (1.0 - K / Q + K * K) / a0;

    f = 38.13547087602444; Q = 0.5003270373238773;
    K = std::tan (pi * f / sr);
    a0 = 1.0 + K / Q + K * K;
    Biquad h;
    h.b0 = 1.0; h.b1 = -2.0; h.b2 = 1.0;
    h.a1 = 2.0 * (K * K - 1.0) / a0;
    h.a2 = (1.0 - K / Q + K * K) / a0;

    for (int c = 0; c < 2; ++c) { shelf[c] = s; hp[c] = h; }
}

void SpectrumProcessor::resetState()
{
    for (int c = 0; c < 2; ++c) { shelf[c].clear(); hp[c].clear(); }
    eL = eR = 0.0; en = 0; si = 0; filled = 0; subCount = 0;
    subE.fill (0.0); subEL.fill (0.0); subER.fill (0.0);
    hBlk.clear(); hSh.clear();
    meters.mom = meters.sh = meters.integ = -100.f;
    meters.mL = meters.mR = -100.f;
    meters.tpL = meters.tpR = meters.tpMax = -100.f;
    meters.pkL = meters.pkR = meters.pkMax = -100.f;
    meters.rmsL = meters.rmsR = -100.f;
    meters.lra = 0.f;
}

void SpectrumProcessor::addHist (Hist& h, double l, double e)
{
    const int bin = (int) std::floor ((l + 70.0) * 10.0);
    if (bin >= 0 && bin < 800) { h.n[(size_t) bin] += 1.0; h.e[(size_t) bin] += e; }
}

double SpectrumProcessor::integrated() const
{
    double c = 0, e = 0;
    for (size_t i = 0; i < 800; ++i) { c += hBlk.n[i]; e += hBlk.e[i]; }
    if (c <= 0) return -100.0;
    const double rel = lufs (e / c) - 10.0;
    double c2 = 0, e2 = 0;
    for (size_t i = 0; i < 800; ++i)
        if (-70.0 + (double) i * 0.1 >= rel) { c2 += hBlk.n[i]; e2 += hBlk.e[i]; }
    return c2 > 0 ? lufs (e2 / c2) : -100.0;
}

double SpectrumProcessor::lra() const
{
    double c = 0, e = 0;
    for (size_t i = 0; i < 800; ++i) { c += hSh.n[i]; e += hSh.e[i]; }
    if (c < 3) return 0.0;
    const double rel = lufs (e / c) - 20.0;
    double tot = 0;
    for (size_t i = 0; i < 800; ++i)
        if (-70.0 + (double) i * 0.1 >= rel) tot += hSh.n[i];
    if (tot < 3) return 0.0;
    auto pct = [&] (double q)
    {
        double acc = 0;
        for (size_t i = 0; i < 800; ++i)
            if (-70.0 + (double) i * 0.1 >= rel)
            {
                acc += hSh.n[i];
                if (acc >= tot * q) return -70.0 + (double) i * 0.1 + 0.05;
            }
        return rel;
    };
    return pct (0.95) - pct (0.10);
}

void SpectrumProcessor::endSub()
{
    const double sL = eL / en, sR = eR / en;
    eL = eR = 0.0; en = 0;
    subE[(size_t) si] = sL + sR; subEL[(size_t) si] = sL; subER[(size_t) si] = sR;
    si = (si + 1) % 30;
    if (filled < 30) ++filled;
    if (filled < 4) return;

    double b = 0, bl = 0, br = 0;
    for (int k = 1; k <= 4; ++k)
    {
        const size_t idx = (size_t) ((si - k + 30) % 30);
        b += subE[idx]; bl += subEL[idx]; br += subER[idx];
    }
    b /= 4; bl /= 4; br /= 4;
    const double m = lufs (b);
    meters.mom = (float) m; meters.mL = (float) lufs (bl); meters.mR = (float) lufs (br);
    addHist (hBlk, m, b);
    meters.integ = (float) integrated();

    if (filled >= 30)
    {
        double s = 0;
        for (double v : subE) s += v;
        s /= 30.0;
        const double sh = lufs (s);
        meters.sh = (float) sh;
        addHist (hSh, sh, s);
        if (++subCount % 10 == 0) meters.lra = (float) lra();
    }
}

void SpectrumProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() == 0 || n == 0) return;
    if (resetRequested.exchange (false)) resetState();

    const float* L = buffer.getReadPointer (0);
    const float* R = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);

    int w = writePos.load (std::memory_order_relaxed);
    float pL = 0.f, pR = 0.f;
    double sqL = 0.0, sqR = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const float l = L[i], r = R[i];
        ringL[(size_t) w] = l; ringR[(size_t) w] = r;
        w = (w + 1) & (ringSize - 1);
        pL = std::max (pL, std::abs (l)); pR = std::max (pR, std::abs (r));
        sqL += (double) l * l; sqR += (double) r * r;
        const double kl = hp[0].process (shelf[0].process ((double) l));
        const double kr = hp[1].process (shelf[1].process ((double) r));
        eL += kl * kl; eR += kr * kr;
        if (++en >= subLen) endSub();
    }
    writePos.store (w, std::memory_order_release);

    // True Peak (sobremuestreo x4 aproximado)
    float tp[2] = { pL, pR };
    for (int c = 0; c < 2; ++c)
    {
        const float* x = c == 0 ? L : R;
        for (int i = 1; i < n - 2; ++i)
        {
            const float p0 = x[i - 1], p1 = x[i], p2 = x[i + 1], p3 = x[i + 2];
            for (float t : { 0.25f, 0.5f, 0.75f })
            {
                const float v = std::abs (0.5f * (2.f * p1 + (-p0 + p2) * t + (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * t * t
                                                  + (-p0 + 3.f * p1 - 3.f * p2 + p3) * t * t * t));
                if (v > tp[c]) tp[c] = v;
            }
        }
    }
    auto dB = [] (float v) { return 20.0f * std::log10 (std::max (v, 1.0e-5f)); };
    meters.pkL = dB (pL); meters.pkR = dB (pR);
    const float tL = dB (tp[0]), tR = dB (tp[1]);
    meters.tpL = tL; meters.tpR = tR;
    meters.tpMax = std::max (meters.tpMax.load(), std::max (tL, tR));
    meters.pkMax = std::max (meters.pkMax.load(), std::max (dB (pL), dB (pR)));
    meters.rmsL = (float) (10.0 * std::log10 (sqL / n + 1.0e-10));
    meters.rmsR = (float) (10.0 * std::log10 (sqR / n + 1.0e-10));
}

void SpectrumProcessor::rebuildWindow (int order, int win)
{
    const int N = 1 << order;
    const double twoPi = juce::MathConstants<double>::twoPi;
    sumW = 0.0;
    for (int i = 0; i < N; ++i)
    {
        const double x = twoPi * i / (N - 1);
        double v = 1.0;
        if (win == 0) v = 0.5 - 0.5 * std::cos (x);
        else if (win == 1) v = 0.42 - 0.5 * std::cos (x) + 0.08 * std::cos (2.0 * x);
        else if (win == 2) v = 0.54 - 0.46 * std::cos (x);
        winTab[(size_t) i] = (float) v;
        sumW += v;
    }
    curOrder = order; curWin = win;
}

juce::var SpectrumProcessor::buildFrame()
{
    const int idx = juce::jlimit (0, 2, fftIdx.load());
    const int win = juce::jlimit (0, 3, winIdx.load());
    const int order = 12 + idx, N = 1 << order, half = N / 2;
    if (order != curOrder || win != curWin) rebuildWindow (order, win);

    const int mask = ringSize - 1;
    const int w = writePos.load (std::memory_order_acquire);
    for (int i = 0; i < N; ++i)
    {
        const size_t j = (size_t) ((w - N + i) & mask);
        fftData[(size_t) i] = 0.5f * (ringL[j] + ringR[j]) * winTab[(size_t) i];
    }
    std::fill (fftData.begin() + N, fftData.begin() + 2 * N, 0.0f);
    ffts[idx]->performFrequencyOnlyForwardTransform (fftData.data(), true);

    const double k0 = 2.0 / sumW;
    for (int k = 0; k < half; ++k)
        dbArr[(size_t) k] = (float) (20.0 * std::log10 (k0 * (double) fftData[(size_t) k] + 1.0e-9));

    // 600 puntos repartidos en escala logaritmica 20 Hz - 20 kHz
    const int NP = 600;
    const double nyq = sr * 0.5;
    juce::Array<juce::var> spec;
    for (int i = 0; i < NP; ++i)
    {
        const double f0 = 20.0 * std::pow (10.0, 3.0 * i / NP), f1 = 20.0 * std::pow (10.0, 3.0 * (i + 1) / NP);
        const double p0 = f0 / nyq * half, p1 = f1 / nyq * half;
        double m;
        if (p1 - p0 < 1.0)
        {
            const double pos = juce::jlimit (0.0, (double) half - 2.0, (p0 + p1) * 0.5);
            const int i0 = (int) pos;
            const double fr = pos - i0;
            m = dbArr[(size_t) i0] * (1.0 - fr) + dbArr[(size_t) i0 + 1] * fr;
        }
        else
        {
            m = -200.0;
            const int b0 = std::max (1, (int) std::ceil (p0)), b1 = std::min (half - 1, (int) std::floor (p1));
            for (int b = b0; b <= b1; ++b) m = std::max (m, (double) dbArr[(size_t) b]);
        }
        spec.add (std::round (m * 10.0) / 10.0);
    }

    // Vectorscopio (512 pares L/R) y correlacion
    juce::Array<juce::var> vec;
    for (int i = 0; i < 512; ++i)
    {
        const size_t j = (size_t) ((w - 512 + i) & mask);
        vec.add (std::round ((double) ringL[j] * 1000.0) / 1000.0);
        vec.add (std::round ((double) ringR[j] * 1000.0) / 1000.0);
    }
    double ll = 0, rr = 0, lr = 0, mm = 0, ss = 0;
    for (int i = 0; i < 2048; ++i)
    {
        const size_t j = (size_t) ((w - 2048 + i) & mask);
        const double l = ringL[j], r = ringR[j];
        ll += l * l; rr += r * r; lr += l * r;
        mm += (l + r) * (l + r) / 4.0; ss += (l - r) * (l - r) / 4.0;
    }
    const double corr = (ll > 1e-9 && rr > 1e-9) ? lr / std::sqrt (ll * rr) : 0.0;
    const double width = (mm + ss > 1e-12) ? ss / (mm + ss) : 0.0;

    juce::Array<juce::var> m;
    for (double v : { (double) meters.mom.load(), (double) meters.sh.load(), (double) meters.integ.load(), (double) meters.lra.load(),
                      (double) meters.mL.load(), (double) meters.mR.load(), (double) meters.tpL.load(), (double) meters.tpR.load(),
                      (double) meters.tpMax.load(), (double) meters.pkL.load(), (double) meters.pkR.load(), (double) meters.pkMax.load(),
                      (double) meters.rmsL.load(), (double) meters.rmsR.load(), corr, width })
        m.add (std::round (v * 100.0) / 100.0);

    auto* o = new juce::DynamicObject();
    o->setProperty ("spec", spec);
    o->setProperty ("vec", vec);
    o->setProperty ("m", m);
    return juce::var (o);
}

juce::AudioProcessorEditor* SpectrumProcessor::createEditor() { return new SpectrumEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpectrumProcessor(); }
