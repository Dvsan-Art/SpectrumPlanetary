#pragma once
#include "PluginProcessor.h"
#include "FilterBank.h"
#include "UiComponents.h"

class SpectrumEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpectrumEditor (SpectrumProcessor&);
    ~SpectrumEditor() override;
    void resized() override;

private:
    void timerCallback() override;
    void pickMidi (int midi);

    SpectrumProcessor& proc;
    ui::PlanetLnF lnf;
    ui::Content content;
    ui::SpectrumView spectrum;
    ui::MeterPanel meters;
    ui::PhasePanel phase;
    ui::KeyboardView keyboard;
    ui::SettingsPanel settings;

    std::unique_ptr<ui::KnobBox> knIn, knOut, knGain, knBands, knDecay, knSmooth;
    std::unique_ptr<ui::CycleButton> bRes, bWin, bWidth, bRoot, bOct;
    juce::TextButton bRun1, bRun2, bUp, bDn, bSet;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> runAtt1, runAtt2;

    double lastMs = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumEditor)
};
