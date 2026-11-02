#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <vector>

// Analizador por BANCO DE FILTROS (sin FFT): cada banda es un pasa-banda Butterworth de
// 1/N de octava (-3 dB en los bordes), con detector RMS/Pico, caida de picos, Peak Hold y promedio.
// Se ejecuta en el hilo de audio; la interfaz solo lee los resultados.
class FilterBankAnalyzer
{
public:
    static constexpr int maxBands = 130;
    static constexpr int maxSecs = 4;

    struct Params
    {
        int bands = 4, order = 3, channel = 0, detector = 0;
        float integMs = 120.f, decay = 12.f, holdTime = 2.f, holdDecay = 3.f, avgTime = 3.f, avgSlope = 0.f, gain = 0.f;
        bool peakOn = true, holdInf = false;
    };

    std::atomic<int> numBands { 0 };
    float fc[maxBands] {}, lo[maxBands] {}, hi[maxBands] {};
    std::atomic<float> bar[maxBands], hold[maxBands], avg[maxBands];

    FilterBankAnalyzer() { clearLevels(); }

    void prepare (double sr, int maxBlock)
    {
        sampleRate = sr; curBands = -1; curOrder = -1;
        mid.assign ((size_t) maxBlock, 0.f); side.assign ((size_t) maxBlock, 0.f);
    }

    void reset()
    {
        for (auto& b : bank) for (auto& s : b.sec) { s.z1[0] = s.z1[1] = s.z2[0] = s.z2[1] = 0.0; }
        for (int i = 0; i < maxBands; ++i) { ps[i] = pa[i] = 0.0; holdT[i] = 0.f; }
        clearLevels();
    }

    void process (const float* L, const float* R, int n, const Params& p)
    {
        if (sampleRate <= 0.0 || n <= 0) return;
        if (p.bands != curBands || p.order != curOrder) rebuild (p.bands, p.order);
        if ((int) mid.size() < n) { mid.resize ((size_t) n); side.resize ((size_t) n); }

        const float* a = L; const float* b2 = nullptr;
        if (p.channel == 0) b2 = R;
        else if (p.channel == 2) a = R;
        else if (p.channel >= 3)
        {
            for (int i = 0; i < n; ++i) { mid[(size_t) i] = 0.5f * (L[i] + R[i]); side[(size_t) i] = 0.5f * (L[i] - R[i]); }
            a = p.channel == 3 ? mid.data() : side.data();
        }

        const int nb = numBands.load (std::memory_order_relaxed);
        const double dt = (double) n / sampleRate;
        const double integT = std::max (0.003, (double) p.integMs * 0.001);
        const double aAvg = 1.0 - std::exp (-dt / std::max (0.1, (double) p.avgTime));
        for (int b = 0; b < nb; ++b)
        {
            double sumA = 0, pkA = 0, sumB = 0, pkB = 0;
            runBand (b, 0, a, n, sumA, pkA);
            if (b2 != nullptr) runBand (b, 1, b2, n, sumB, pkB);
            const double msq = (sumA + sumB) / ((double) n * (b2 != nullptr ? 2.0 : 1.0));
            const double pk = std::max (pkA, pkB);

            float lvl;
            if (p.detector == 1) lvl = (float) (20.0 * std::log10 (std::max (pk, 1.0e-7))) + p.gain;
            else
            {
                const double tau = std::max (integT, 1.5 / (double) fc[b]);
                ps[b] += (1.0 - std::exp (-dt / tau)) * (msq - ps[b]);
                lvl = (float) (10.0 * std::log10 (ps[b] + 1.0e-15) + 3.0103) + p.gain;
            }
            float v = bar[b].load (std::memory_order_relaxed);
            v = p.peakOn ? std::max (lvl, v - p.decay * (float) dt) : lvl;
            float h = hold[b].load (std::memory_order_relaxed);
            if (v > h) { h = v; holdT[b] = 0.f; }
            else { holdT[b] += (float) dt; if (! p.holdInf && holdT[b] > p.holdTime) h -= p.holdDecay * (float) dt; }
            pa[b] += aAvg * (msq - pa[b]);
            bar[b].store (v, std::memory_order_relaxed);
            hold[b].store (h, std::memory_order_relaxed);
            avg[b].store ((float) (10.0 * std::log10 (pa[b] + 1.0e-15) + 3.0103) + p.gain
                          + p.avgSlope * (float) std::log2 ((double) fc[b] / 1000.0), std::memory_order_relaxed);
        }
    }

private:
    struct Sec { double b0 = 0, a1 = 0, a2 = 0, z1[2] = { 0, 0 }, z2[2] = { 0, 0 }; };
    struct Band { int ns = 0; Sec sec[maxSecs]; };
    std::array<Band, maxBands> bank;
    double ps[maxBands] {}, pa[maxBands] {};
    float holdT[maxBands] {};
    std::vector<float> mid, side;
    double sampleRate = 0.0;
    int curBands = -1, curOrder = -1;

    void clearLevels() { for (int i = 0; i < maxBands; ++i) { bar[i].store (-150.f); hold[i].store (-150.f); avg[i].store (-150.f); } }

    void runBand (int b, int ch, const float* x, int n, double& sum, double& pk)
    {
        Band& bd = bank[(size_t) b];
        double s2 = 0.0, pm = 0.0;
        for (int i = 0; i < n; ++i)
        {
            double v = x[i];
            for (int k = 0; k < bd.ns; ++k)
            {
                Sec& s = bd.sec[k];
                const double y = s.b0 * v + s.z1[ch];
                s.z1[ch] = s.z2[ch] - s.a1 * y;
                s.z2[ch] = -s.b0 * v - s.a2 * y;
                v = y;
            }
            s2 += v * v;
            const double av = std::abs (v);
            if (av > pm) pm = av;
        }
        sum = s2; pk = pm;
    }

    void rebuild (int B, int order)
    {
        curBands = B; curOrder = order;
        const double nyq = sampleRate * 0.5;
        const int i0 = (int) std::ceil (B * std::log2 (19.5 / 1000.0)), i1 = (int) std::floor (B * std::log2 (20500.0 / 1000.0));
        int nb = 0;
        for (int i = i0; i <= i1 && nb < maxBands; ++i)
        {
            const double f = 1000.0 * std::pow (2.0, (double) i / B);
            const double f1 = f * std::pow (2.0, -0.5 / B), f2 = f * std::pow (2.0, 0.5 / B);
            if (f2 >= nyq * 0.98) break;
            fc[nb] = (float) f; lo[nb] = (float) f1; hi[nb] = (float) f2;
            design (nb, f1, f2, order);
            ++nb;
        }
        reset();
        numBands.store (nb, std::memory_order_release);
    }

    // Pasa-banda Butterworth de orden N (2N polos) por transformacion bilineal, en secciones biquad con ganancia 1 en el centro.
    void design (int b, double f1, double f2, int N)
    {
        const double pi = 3.14159265358979323846, fs = sampleRate, c = 2.0 * fs;
        const double w1 = c * std::tan (pi * f1 / fs), w2 = c * std::tan (pi * f2 / fs);
        const double w0 = std::sqrt (w1 * w2), bw = w2 - w1;
        const double W = 2.0 * std::atan (w0 / c);
        Band& bd = bank[(size_t) b];
        bd.ns = 0;
        for (int k = 0; k < N; ++k)
        {
            const double ang = pi * (2.0 * k + N + 1.0) / (2.0 * N);
            const std::complex<double> p (std::cos (ang), std::sin (ang));
            if (p.imag() < -1.0e-9) continue;
            const std::complex<double> pb = p * bw, d = std::sqrt (pb * pb - 4.0 * w0 * w0);
            const std::complex<double> q[2] = { (pb + d) * 0.5, (pb - d) * 0.5 };
            const int cnt = std::abs (p.imag()) < 1.0e-9 ? 1 : 2;
            for (int j = 0; j < cnt && bd.ns < maxSecs; ++j)
            {
                const double a0 = std::norm (q[j]), a1 = -2.0 * q[j].real();
                const double D0 = c * c + a1 * c + a0;
                Sec s;
                s.a1 = (-2.0 * c * c + 2.0 * a0) / D0;
                s.a2 = (c * c - a1 * c + a0) / D0;
                const double b0 = c / D0;
                const std::complex<double> z1 = std::polar (1.0, -W), z2 = std::polar (1.0, -2.0 * W);
                const std::complex<double> h = (b0 * (1.0 - z2)) / (1.0 + s.a1 * z1 + s.a2 * z2);
                s.b0 = b0 / std::abs (h);
                bd.sec[bd.ns++] = s;
            }
        }
    }
};
