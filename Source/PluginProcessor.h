#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>
#include "FilterBank.h"

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
    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    static constexpr int ringSize = 32768;
    const float* ringLeft() const { return ringL.data(); }
    const float* ringRight() const { return ringR.data(); }
    int getWritePos() const { return writePos.load (std::memory_order_acquire); }
    double getSampleRate() const { return sr; }
    void requestReset() { resetRequested = true; }

    struct Meters
    {
        std::atomic<float> mom { -100.f }, sh { -100.f }, integ { -100.f }, lra { 0.f },
                           mL { -100.f }, mR { -100.f }, tpL { -100.f }, tpR { -100.f }, tpMax { -100.f },
                           pkL { -100.f }, pkR { -100.f }, pkMax { -100.f }, rmsL { -100.f }, rmsR { -100.f };
    };
    Meters meters;
    juce::AudioProcessorValueTreeState apvts;
    FilterBankAnalyzer analyzer;
    std::atomic<float> corrOut { 0.f }, balOut { 0.f };   // correlacion -1..+1 y balance L(-1)..R(+1)

private:
    std::array<float, ringSize> ringL {}, ringR {};
    std::atomic<int> writePos { 0 };
    std::atomic<bool> resetRequested { false };
    double sr = 44100.0;
    double corrA = 0.99, sLL = 0.0, sRR = 0.0, sLR = 0.0;   // ventana exponencial de 100 ms
    std::vector<float> scratchL, scratchR;

    std::atomic<float>* inParam = nullptr;
    std::atomic<float>* outParam = nullptr;
    std::atomic<float>* runParam = nullptr;
    std::atomic<float>* chanParam = nullptr;
    std::atomic<float>* bandsParam = nullptr;
    std::atomic<float>* peakOnParam = nullptr;
    std::atomic<float>* peakDecayParam = nullptr;
    std::atomic<float>* holdOnParam = nullptr;
    std::atomic<float>* holdTimeParam = nullptr;
    std::atomic<float>* holdDecayParam = nullptr;
    std::atomic<float>* holdInfParam = nullptr;
    std::atomic<float>* avgOnParam = nullptr;
    std::atomic<float>* avgTimeParam = nullptr;
    std::atomic<float>* avgSlopeParam = nullptr;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* monParam = nullptr;

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

    double eL = 0, eR = 0;
    int en = 0, subLen = 4410, si = 0, filled = 0, subCount = 0;
    std::array<double, 30> subE {}, subEL {}, subER {};

    void setupFilters();
    void resetState();
    void endSub();
    void analyseChunk (const float* L, const float* R, int n);
    double integrated() const;
    double lra() const;
    static double lufs (double e) { return e > 0.0 ? -0.691 + 10.0 * std::log10 (e) : -100.0; }
    static void addHist (Hist& h, double l, double e);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumProcessor)
};
