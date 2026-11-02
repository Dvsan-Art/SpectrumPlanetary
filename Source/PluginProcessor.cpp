#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using ID = juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto flt = [&] (const char* id, const char* name, float lo, float hi, float step, float def)
    { l.add (std::make_unique<juce::AudioParameterFloat> (ID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, step), def)); };
    auto cho = [&] (const char* id, const char* name, juce::StringArray items, int def)
    { l.add (std::make_unique<juce::AudioParameterChoice> (ID { id, 1 }, name, items, def)); };
    auto bol = [&] (const char* id, const char* name, bool def)
    { l.add (std::make_unique<juce::AudioParameterBool> (ID { id, 1 }, name, def)); };

    flt ("inGain", "Input Trim", -24.f, 12.f, 0.1f, 0.f);
    flt ("outGain", "Output", -24.f, 12.f, 0.1f, 0.f);
    cho ("res", "Resolution", { "Soft", "Normal", "Sharp" }, 1);
    cho ("win", "Detector", { "RMS", "Peak" }, 0);
    bol ("run", "Analyze", true);
    cho ("root", "Root Note", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0);
    l.add (std::make_unique<juce::AudioParameterInt> (ID { "oct", 1 }, "Octave", 1, 7, 4));
    cho ("width", "Width", { "0.5", "1", "2", "3", "6", "12", "24" }, 1);
    flt ("gain", "Display Gain", -30.f, 30.f, 0.5f, 0.f);
    cho ("bands", "Bands per Octave", { "3", "4", "6", "12" }, 1);
    flt ("peakDecay", "Peak Decay", 1.f, 60.f, 0.1f, 12.f);
    l.add (std::make_unique<juce::AudioParameterFloat> (ID { "integ", 1 }, "Integration", juce::NormalisableRange<float> (5.f, 1000.f, 1.f, 0.4f), 120.f));
    cho ("style", "Display", { "Bars", "Curve", "Fill" }, 0);
    cho ("channel", "Channel", { "Both", "Left", "Right", "Mid", "Side" }, 0);
    cho ("grid", "Freq. Grid", { "Octave", "Decade", "None" }, 0);
    flt ("ceiling", "Ceiling", -12.f, 24.f, 1.f, 3.f);
    flt ("gridDb", "Grid", 1.f, 24.f, 1.f, 5.f);
    flt ("range", "Range", 12.f, 120.f, 1.f, 51.f);
    bol ("peakOn", "Peak On", true);
    bol ("holdOn", "Peak Hold On", false);
    flt ("holdTime", "Hold Time", 0.1f, 10.f, 0.1f, 2.f);
    flt ("holdDecay", "Hold Decay", 0.f, 30.f, 0.1f, 3.f);
    bol ("holdInf", "Hold Infinite", false);
    bol ("avgOn", "Average On", false);
    flt ("avgTime", "Average Time", 0.5f, 30.f, 0.1f, 3.f);
    flt ("avgSlope", "Average Slope", -6.f, 6.f, 0.1f, 0.f);
    return l;
}

SpectrumProcessor::SpectrumProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    inParam  = apvts.getRawParameterValue ("inGain");
    outParam = apvts.getRawParameterValue ("outGain");
    runParam = apvts.getRawParameterValue ("run");
    pp.bands = apvts.getRawParameterValue ("bands"); pp.res = apvts.getRawParameterValue ("res"); pp.win = apvts.getRawParameterValue ("win");
    pp.channel = apvts.getRawParameterValue ("channel"); pp.integ = apvts.getRawParameterValue ("integ"); pp.decay = apvts.getRawParameterValue ("peakDecay");
    pp.holdTime = apvts.getRawParameterValue ("holdTime"); pp.holdDecay = apvts.getRawParameterValue ("holdDecay");
    pp.avgTime = apvts.getRawParameterValue ("avgTime"); pp.avgSlope = apvts.getRawParameterValue ("avgSlope"); pp.gain = apvts.getRawParameterValue ("gain");
    pp.peakOn = apvts.getRawParameterValue ("peakOn"); pp.holdInf = apvts.getRawParameterValue ("holdInf");
    resetState();
}

bool SpectrumProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SpectrumProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    subLen = (int) (sr * 0.1);
    analyzer.prepare (sampleRate, juce::jmax (samplesPerBlock, 4096));
    scratchL.assign ((size_t) juce::jmax (samplesPerBlock, 4096), 0.f);
    scratchR.assign ((size_t) juce::jmax (samplesPerBlock, 4096), 0.f);
    setupFilters();
    resetState();
}

void SpectrumProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr) copyXmlToBinary (*xml, dest);
}

void SpectrumProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
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
    analyzer.reset();
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

    if (runParam->load() > 0.5f)
    {
        if ((int) scratchL.size() < n) { scratchL.resize ((size_t) n); scratchR.resize ((size_t) n); }
        const float gIn = juce::Decibels::decibelsToGain (inParam->load());
        const float* L = buffer.getReadPointer (0);
        const float* R = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);
        for (int i = 0; i < n; ++i) { scratchL[(size_t) i] = L[i] * gIn; scratchR[(size_t) i] = R[i] * gIn; }
        analyseChunk (scratchL.data(), scratchR.data(), n);

        static const int bandList[4] = { 3, 4, 6, 12 };
        FilterBankAnalyzer::Params q;
        q.bands = bandList[juce::jlimit (0, 3, (int) pp.bands->load())];
        q.order = 2 + juce::jlimit (0, 2, (int) pp.res->load());
        q.detector = (int) pp.win->load(); q.channel = (int) pp.channel->load();
        q.integMs = pp.integ->load(); q.decay = pp.decay->load(); q.holdTime = pp.holdTime->load(); q.holdDecay = pp.holdDecay->load();
        q.avgTime = pp.avgTime->load(); q.avgSlope = pp.avgSlope->load(); q.gain = pp.gain->load();
        q.peakOn = pp.peakOn->load() > 0.5f; q.holdInf = pp.holdInf->load() > 0.5f;
        analyzer.process (scratchL.data(), scratchR.data(), n, q);
    }
    buffer.applyGain (juce::Decibels::decibelsToGain (outParam->load()));
}

void SpectrumProcessor::analyseChunk (const float* L, const float* R, int n)
{
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
    auto keepMax = [] (std::atomic<float>& a, float v) { if (v > a.load()) a.store (v); };
    keepMax (meters.pkL, dB (pL)); keepMax (meters.pkR, dB (pR));
    keepMax (meters.tpL, dB (tp[0])); keepMax (meters.tpR, dB (tp[1]));
    meters.tpMax = std::max (meters.tpMax.load(), std::max (dB (tp[0]), dB (tp[1])));
    meters.pkMax = std::max (meters.pkMax.load(), std::max (dB (pL), dB (pR)));
    meters.rmsL = (float) (10.0 * std::log10 (sqL / n + 1.0e-10));
    meters.rmsR = (float) (10.0 * std::log10 (sqR / n + 1.0e-10));
}

juce::AudioProcessorEditor* SpectrumProcessor::createEditor() { return new SpectrumEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpectrumProcessor(); }
