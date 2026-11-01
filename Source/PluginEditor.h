#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class SpectrumEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpectrumEditor (SpectrumProcessor&);
    ~SpectrumEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    SpectrumProcessor& proc;
    juce::TextButton analyzeButton {"Analyze"};
    juce::Slider inputKnob, outputKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumEditor)
}
