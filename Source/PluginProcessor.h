#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
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

    // Datos públicos seguros para la interfaz nativa
    std::vector<float> getSpectrumData() const {
        std::vector<float> copy(600);
        for(int i=0; i<600; ++i) copy[i] = smoothSpec[i];
        return copy;
    }

    std::atomic<float> peakL {-100.f}, peakR {-100.f}, rmsL {-100.f}, rmsR {-100.f};

private:
    static constexpr int ringSize = 32768;
    std::array<float, ringSize> ringL{}, ringR{};
    int writePos = 0;
    double sr = 44100.0;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fftData, smoothSpec;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumProcessor)
};
