#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

class SpectrumProcessor : public juce::AudioProcessor
{
public:
    SpectrumProcessor();
    ~SpectrumProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Spectrum Planetary"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // Lo llama la interfaz 30 veces por segundo
    juce::var buildFrame();
    void requestReset() { resetRequested = true; }

    std::atomic<int> fftIdx { 2 };   // 0=Coarse 1=Medium 2=Fine
    std::atomic<int> winIdx { 0 };   // 0=Hanning 1=Blackman 2=Hamming 3=Rect

private:
    static constexpr int ringSize = 32768;
    std::array<float, ringSize> ringL {}, ringR {};
    std::atomic<int> writePos { 0 };
    std::atomic<bool> resetRequested { false };
    double sr = 44100.0;

    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        double process (double x)
        {
            const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            return y;
        }
        void clear() { x1 = x2 = y1 = y2 = 0; }
    };
    Biquad shelf[2], hp[2];

    struct Hist
    {
        std::array<double, 800> n {}, e {};
        void clear() { n.fill (0.0); e.fill (0.0); }
    };
    Hist hBlk, hSh;

    struct Meters
    {
        std::atomic<float> mom { -100.f }, sh { -100.f }, integ { -100.f }, lra { 0.f },
                           mL { -100.f }, mR { -100.f }, tpL { -100.f }, tpR { -100.f }, tpMax { -100.f },
                           pkL { -100.f }, pkR { -100.f }, pkMax { -100.f }, rmsL { -100.f }, rmsR { -100.f };
    } meters;

    double eL = 0, eR = 0;
    int en = 0, subLen = 4410, si = 0, filled = 0, subCount = 0;
    std::array<double, 30> subE {}, subEL {}, subER {};

    void setupFilters();
    void resetState();
    void endSub();
    double integrated() const;
    double lra() const;
    static double lufs (double e) { return e > 0.0 ? -0.691 + 10.0 * std::log10 (e) : -100.0; }
    static void addHist (Hist& h, double l, double e);

    std::unique_ptr<juce::dsp::FFT> ffts[3];
    std::vector<float> fftData, dbArr, winTab;
    int curOrder = -1, curWin = -1;
    double sumW = 1.0;
    void rebuildWindow (int order, int win);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumProcessor)
};
