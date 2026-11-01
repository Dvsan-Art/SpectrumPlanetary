#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrumProcessor::SpectrumProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    fft = std::make_unique<juce::dsp::FFT>(13); // 8192 points
    fftData.resize(2 * 8192, 0.0f);
    smoothSpec.resize(600, -120.0f);
}

bool SpectrumProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SpectrumProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
}

void SpectrumProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() == 0 || n == 0) return;

    const float* L = buffer.getReadPointer(0);
    const float* R = buffer.getReadPointer(buffer.getNumChannels() > 1 ? 1 : 0);

    float pL = 0.f, pR = 0.f;
    double sqL = 0.0, sqR = 0.0;

    for (int i = 0; i < n; ++i)
    {
        float l = L[i], r = R[i];
        ringL[writePos] = l;
        ringR[writePos] = r;
        writePos = (writePos + 1) & (ringSize - 1);

        pL = std::max(pL, std::abs(l));
        pR = std::max(pR, std::abs(r));
        sqL += l * l;
        sqR += r * r;
    }

    peakL = 20.0f * std::log10(std::max(pL, 1.0e-5f));
    peakR = 20.0f * std::log10(std::max(pR, 1.0e-5f));
    rmsL = (float)(10.0 * std::log10(sqL / n + 1.0e-10));
    rmsR = (float)(10.0 * std::log10(sqR / n + 1.0e-10));

    // Llenar datos FFT para el espectro nativo
    int N = 8192;
    for (int i = 0; i < N; ++i)
    {
        int idx = (writePos - N + i + ringSize) & (ringSize - 1);
        fftData[i] = 0.5f * (ringL[idx] + ringR[idx]);
    }
    std::fill(fftData.begin() + N, fftData.end(), 0.0f);
    fft->performFrequencyOnlyForwardTransform(fftData.data(), true);

    for (int i = 0; i < 600; ++i)
    {
        int bin = juce::jlimit(1, N / 2 - 1, (int)(i * (N / 4) / 600.0));
        float db = 20.0f * std::log10(fftData[bin] + 1.0e-9f) + 12.0f;
        db = juce::jlimit(-100.0f, 12.0f, db);
        smoothSpec[i] = smoothSpec[i] * 0.7f + db * 0.3f; // Suavizado fluido
    }
}

juce::AudioProcessorEditor* SpectrumProcessor::createEditor() { return new SpectrumEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpectrumProcessor(); }
