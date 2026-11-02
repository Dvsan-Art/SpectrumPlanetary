#pragma once
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

// Analizador de espectro por bandas (n bandas por octava). Todo el calculo es C++.
class SpectrumEngine
{
public:
    static constexpr int maxBands = 130;

    struct Settings
    {
        int res = 2, win = 0, channel = 0, bands = 4;
        float smooth = 0.5f, decay = 12.f, holdTime = 2.f, holdDecay = 3.f, avgTime = 3.f, avgSlope = 0.f, gain = 0.f;
        bool peakOn = true, holdOn = false, holdInf = false;
    };

    int numBands = 0;
    float fc[maxBands] {}, edgeLo[maxBands] {}, edgeHi[maxBands] {};
    float bar[maxBands] {}, hold[maxBands] {}, avg[maxBands] {};

    SpectrumEngine()
    {
        for (int i = 0; i < 3; ++i) fft[i] = std::make_unique<juce::dsp::FFT> (12 + i);
        data.assign (2 * 16384, 0.f);
        pw.assign (8192, 0.f); pw2.assign (8192, 0.f);
        pre.assign (8193, 0.0); wtab.assign (16384, 1.f);
        buildBands (4);
    }

    void reset()
    {
        for (int i = 0; i < maxBands; ++i) { bar[i] = hold[i] = avg[i] = -150.f; smP[i] = avP[i] = 0.0; holdT[i] = 0.f; }
    }

    void update (const float* rl, const float* rr, int ringSize, int wpos, double sr, const Settings& s, double dt)
    {
        if (s.bands != curBands) buildBands (s.bands);
        const int idx = juce::jlimit (0, 2, s.res), order = 12 + idx, N = 1 << order, half = N / 2;
        if (order != curOrder || s.win != curWin) buildWindow (order, s.win);
        const int mask = ringSize - 1;

        auto run = [&] (int src, std::vector<float>& out)
        {
            for (int i = 0; i < N; ++i)
            {
                const int j = (wpos - N + i) & mask;
                const float l = rl[j], r = rr[j];
                const float x = src == 0 ? l : src == 1 ? r : src == 2 ? 0.5f * (l + r) : 0.5f * (l - r);
                data[(size_t) i] = x * wtab[(size_t) i];
            }
            std::fill (data.begin() + N, data.begin() + 2 * N, 0.f);
            fft[idx]->performFrequencyOnlyForwardTransform (data.data(), true);
            const float sc = 2.f / ((float) N * sumW2);
            out[0] = 0.f;
            for (int k = 1; k < half; ++k) out[(size_t) k] = sc * data[(size_t) k] * data[(size_t) k];
        };

        if (s.channel == 0)
        {
            run (0, pw); run (1, pw2);
            for (int k = 0; k < half; ++k) pw[(size_t) k] = 0.5f * (pw[(size_t) k] + pw2[(size_t) k]);
        }
        else run (s.channel - 1, pw);

        pre[0] = 0.0;
        for (int k = 0; k < half; ++k) pre[(size_t) k + 1] = pre[(size_t) k] + (double) pw[(size_t) k];

        const double df = sr / N;
        auto F = [&] (double x)
        {
            const double y = x + 0.5;
            int kk = (int) std::floor (y);
            double fr = y - kk;
            if (kk < 0) { kk = 0; fr = 0.0; }
            if (kk >= half) { kk = half - 1; fr = 1.0; }
            return pre[(size_t) kk] + (double) pw[(size_t) kk] * fr;
        };

        dt = juce::jlimit (0.001, 0.25, dt);
        const double sm = std::pow ((double) s.smooth, dt * 60.0);
        const double aA = 1.0 - std::exp (-dt / (double) std::max (0.1f, s.avgTime));
        for (int b = 0; b < numBands; ++b)
        {
            double p = F (edgeHi[b] / df) - F (edgeLo[b] / df);
            if (p < 0.0) p = 0.0;
            smP[b] = smP[b] * sm + p * (1.0 - sm);
            avP[b] += (p - avP[b]) * aA;
            const float lv = (float) (10.0 * std::log10 (smP[b] + 1.0e-15) + 3.0103) + s.gain;
            bar[b] = s.peakOn ? std::max (lv, bar[b] - s.decay * (float) dt) : lv;
            const float v = bar[b];
            if (v > hold[b]) { hold[b] = v; holdT[b] = 0.f; }
            else { holdT[b] += (float) dt; if (! s.holdInf && holdT[b] > s.holdTime) hold[b] -= s.holdDecay * (float) dt; }
            avg[b] = (float) (10.0 * std::log10 (avP[b] + 1.0e-15) + 3.0103) + s.gain
                   + s.avgSlope * (float) std::log2 ((double) fc[b] / 1000.0);
        }
    }

private:
    std::unique_ptr<juce::dsp::FFT> fft[3];
    std::vector<float> data, pw, pw2, wtab;
    std::vector<double> pre;
    double smP[maxBands] {}, avP[maxBands] {};
    float holdT[maxBands] {};
    int curOrder = -1, curWin = -1, curBands = -1;
    float sumW2 = 1.f;

    void buildBands (int B)
    {
        curBands = B; numBands = 0;
        const int i0 = (int) std::ceil (B * std::log2 (20.0 / 1000.0)), i1 = (int) std::floor (B * std::log2 (20000.0 / 1000.0));
        for (int i = i0; i <= i1 && numBands < maxBands; ++i)
        {
            const double f = 1000.0 * std::pow (2.0, (double) i / B);
            fc[numBands] = (float) f;
            edgeLo[numBands] = (float) (f * std::pow (2.0, -0.5 / B));
            edgeHi[numBands] = (float) (f * std::pow (2.0, 0.5 / B));
            ++numBands;
        }
        reset();
    }

    void buildWindow (int order, int win)
    {
        const int N = 1 << order;
        const double twoPi = 6.283185307179586;
        double sw = 0.0;
        for (int i = 0; i < N; ++i)
        {
            const double x = twoPi * i / (N - 1);
            double v = 1.0;
            if (win == 0) v = 0.5 - 0.5 * std::cos (x);
            else if (win == 1) v = 0.42 - 0.5 * std::cos (x) + 0.08 * std::cos (2.0 * x);
            else if (win == 2) v = 0.54 - 0.46 * std::cos (x);
            wtab[(size_t) i] = (float) v;
            sw += v * v;
        }
        sumW2 = (float) sw; curOrder = order; curWin = win;
    }
};
