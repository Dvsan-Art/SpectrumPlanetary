#pragma once
// Port escalar (sin SSE) de Seven Phases Spectrum Analyzer v1.09 (analyzer.h, sp/coefficients.h, sp/more.h).
// Licencia original: MIT (c) Seven Phases. Mismas ecuaciones y constantes.
#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

class FilterBankAnalyzer
{
public:
    static constexpr int maxBands   = 64;
    static constexpr int maxFilt    = maxBands + 1;
    static constexpr int ringFrames = 512;
    static constexpr double pollMs  = 48.0;   // pollTime del original

    struct Params
    {
        int   bands = 4;            // 3, 4 o 6 bandas por octava
        int   channel = 2;          // 0 Left, 1 Right, 2 Both, 3 Mix
        float peakDecay = 12.f;     // dB/s
        float holdTime = 2000.f;    // ms
        float holdDecay = 3.f;      // dB/s
        float avgTime = 3000.f;     // ms
        float avgSlope = 0.f;       // dB/oct
        float gain = 0.f;           // ganancia de visualizacion (extra)
        bool  peakOn = true, holdOn = false, holdInf = false, avgOn = false;
    };

    std::atomic<int>   numBands { 0 };
    std::atomic<float> freqMinA { 0.f }, freqMaxA { 0.f };
    float fc[maxBands] {}, lo[maxBands] {}, hi[maxBands] {};
    std::atomic<float> bar[maxBands], hold[maxBands], avg[maxBands];   // dB, listos para dibujar

    FilterBankAnalyzer()
    {
        for (int i = 0; i < maxBands; ++i) { bar[i].store (-200.f); hold[i].store (-200.f); avg[i].store (-200.f); }
    }

    void prepare (double sr, int) { sampleRate = sr; curBands = -1; reset(); }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            zPrev[c] = 0.0;
            for (int m = 0; m < maxFilt; ++m) { z0[c][m] = z1[c][m] = 0.0; }
        }
        for (int m = 0; m < maxFilt; ++m)
        {
            peakAcc[m] = energyAcc[m] = 0.0;
            peakM[m] = holdM[m] = -200.0;
            holdT[m] = 0.0;
            for (int r = 0; r < ringFrames; ++r) ring[m][r] = 0.0;
        }
        for (int i = 0; i < maxBands; ++i) { bar[i].store (-200.f); hold[i].store (-200.f); avg[i].store (-200.f); }
        pollCounter = 0; frameSamples = 0; ringPos = 0;
    }

    void process (const float* L, const float* R, int n, const Params& p)
    {
        if (sampleRate <= 0.0 || n <= 0) return;
        if (p.bands != curBands || sampleRate != curFs) rebuild (p.bands);
        if (bandCount <= 0) return;

        for (int i = 0; i < n; ++i)
        {
            if (p.channel == 2)      { sample (0, L[i]); sample (1, R[i]); }
            else if (p.channel == 0) { sample (0, L[i]); }
            else if (p.channel == 1) { sample (0, R[i]); }
            else                     { sample (0, 0.5f * (L[i] + R[i])); }   // Mix
        }
        frameSamples += (p.channel == 2 ? 2 : 1) * n;
        pollCounter  += n;

        const int pollSamples = std::max (1, (int) std::lround (sampleRate * pollMs * 0.001));
        if (pollCounter >= pollSamples)
        {
            pollCounter -= pollSamples;
            tick (p);
        }
    }

private:
    // ------------------------------------------------------------ muestra a muestra
    void sample (int c, float x)
    {
        const int nf = bandCount + 1;
        const double in = double (x) + 1.0e-10;              // anti-denormal del original
        const double zl = in + zPrev[c];                     // ZeroLP: out = in + z ; z = in
        zPrev[c] = in;

        double prev = 0.0;
        for (int m = 0; m < nf; ++m)
        {
            // TwoPoleLPSAx: out = z0 ; z0 = in*k0 + z0*k1 + z1*k2 ; z1 = out
            const double o = z0[c][m];
            z0[c][m] = zl * kc[0][m] + z0[c][m] * kc[1][m] + z1[c][m] * kc[2][m];
            z1[c][m] = o;

            // banda m = salida filtro m - salida filtro m-1 (interband)
            const double y  = o - prev;
            prev = o;
            const double ye = y * em[m];                     // emphasis (amplitud)
            const double a  = std::fabs (ye);
            if (a > peakAcc[m]) peakAcc[m] = a;              // Peak: max
            energyAcc[m] += ye * ye;                         // Average: suma de cuadrados
        }
    }

    // ------------------------------------------------------------ poll cada 48 ms (Meter + Averager)
    void tick (const Params& p)
    {
        const int nb = bandCount;
        const double frameCnt = frameSamples > 0 ? double (frameSamples) : 1.0;
        const double slope  = 10.0 / nb * p.avgSlope;               // slope por banda
        const double outset = 6.0 - 0.5 * nb * slope;
        const int    size   = std::clamp ((int) (p.avgTime / pollMs), 1, ringFrames);
        const double pdec   = p.peakDecay * pollMs * 0.001;
        const double hdec   = p.holdDecay * pollMs * 0.001;

        for (int m = 0; m < nb; ++m)
        {
            // Meter (peak / hold), en dB
            const double pv = std::max (-200.0, 20.0 * std::log10 (std::max (peakAcc[m], 1.0e-10)));
            peakM[m] -= pdec;
            if (peakM[m] < pv) peakM[m] = pv;
            if (holdT[m] > 0.0) holdT[m] -= pollMs;
            else holdM[m] -= (p.holdInf ? 0.0 : hdec);
            if (holdM[m] < pv) { holdM[m] = pv; holdT[m] = p.holdTime; }

            // Averager: media rectangular de 'size' frames de energia
            ring[m][ringPos] = energyAcc[m] / frameCnt;
            double sum = 0.0;
            for (int k = 0; k < size; ++k) sum += ring[m][(ringPos - k + ringFrames) % ringFrames];
            const double ms = std::max (sum / size, 1.0e-20);
            const double avrg = 0.5 * (outset + 20.0 * std::log10 (ms) + slope * m);

            bar[m].store ((float) (peakM[m] + p.gain));
            hold[m].store ((float) (holdM[m] + p.gain));
            avg[m].store ((float) (avrg + p.gain));
        }

        ringPos = (ringPos + 1) % ringFrames;
        frameSamples = 0;
        for (int m = 0; m < maxFilt; ++m) { peakAcc[m] = 0.0; energyAcc[m] = 0.0; }
        numBands.store (nb);
    }

    // ------------------------------------------------------------ diseno (Analyzer::update)
    static double emphasis (int bpo, double f, double fedg)
    {
        const double base = 1.025;
        f = f * (1.0 / fedg);
        if (bpo == 6) return base + 2.2 * f * f * f - 1.25 * f * f * f * f * f;
        return base + f * f;
    }

    // sp::twoPoleLPCoeffs
    void lpCoeffs (double fs, double f, double q, double prewrap, int n)
    {
        const double pi = 3.14159265358979323846;
        const double w = 2.0 * pi * f / fs;
        const double y = std::sin (w) / (q * (1.0 + std::cos (w * prewrap)));
        const double a = 1.0 / (1.0 + y);
        const double c2 = a * (y - 1.0);
        const double c1 = a * 2.0 * std::cos (w);
        const double c0 = (0.5 / q) * (1.0 - c1 - c2);
        kc[0][n] = c0; kc[1][n] = c1; kc[2][n] = c2;
    }

    void rebuild (int bpo)
    {
        curBands = bpo; curFs = sampleRate;
        const double fs = sampleRate;
        const double k  = std::exp (0.69314718055994530942 * (1.0 / bpo));
        double f = 15.625;
        const double fmin = 22.0;
        while (f < fmin) f *= k;
        const double freqMinV = f;

        const double fanc = 1000.0, fedg = 0.94 * 0.5 * fs;
        const int    i0   = int (0.5 + std::log (fedg / (fanc * std::sqrt (k))) / std::log (k));
        const double fli  = 1.0 / (fanc * std::pow (k, i0));
        const double r    = (1.0 - (fedg / std::sqrt (k)) * fli) * fli * fli;
        double ferr = 1.0;
        if (bpo == 3) ferr = .904; else if (bpo == 4) ferr = .905; else if (bpo == 6) ferr = .909;

        double ff = (f / std::sqrt (k)) * (1.0 - r * f * f);
        int n = 0;
        double freqMaxV = 0.0;
        bandCount = 0;
        for (;;)
        {
            lpCoeffs (fs, ff, 2.0 * bpo, ferr, n);
            em[n]   = emphasis (bpo, ff, fedg);
            fref[n] = f;
            ff = f * std::sqrt (k) * (1.0 - r * f * f);
            if (ff > (fedg + 1.0) || ff > 29000.0) break;
            if (n + 1 >= maxBands) break;
            bandCount = ++n;
            freqMaxV  = f;
            f *= k;
        }

        for (int i = 0; i < bandCount; ++i) { fc[i] = (float) fref[i]; lo[i] = hi[i] = fc[i]; }
        freqMinA.store ((float) freqMinV);
        freqMaxA.store ((float) freqMaxV);
        numBands.store (bandCount);
        reset();
    }

    // ------------------------------------------------------------ miembros
    double kc[3][maxFilt] {};
    double em[maxFilt] {};
    double fref[maxFilt] {};
    double z0[2][maxFilt] {}, z1[2][maxFilt] {};
    double zPrev[2] {};
    double peakAcc[maxFilt] {}, energyAcc[maxFilt] {};
    double peakM[maxFilt] {}, holdM[maxFilt] {}, holdT[maxFilt] {};
    double ring[maxFilt][ringFrames] {};
    int    ringPos = 0, pollCounter = 0, frameSamples = 0;
    int    bandCount = 0, curBands = -1;
    double sampleRate = 0.0, curFs = 0.0;
};
