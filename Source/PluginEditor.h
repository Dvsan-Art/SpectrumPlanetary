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
    void toggleMonitor (int idx);

    SpectrumProcessor& proc;
    ui::PlanetLnF lnf;
    ui::Content content;
    ui::SpectrumView spectrum;
    ui::MeterPanel meters;
    ui::PhasePanel phase;
    ui::KeyboardView keyboard;
    ui::SettingsPanel settings;

    ui::MonitorKnob monL { "Left" }, monR { "Right" }, monM { "Mono" }, monD { "Diff" };
    std::unique_ptr<ui::CycleButton> bWidth, bRoot, bOct;
    juce::TextButton bFrame, bSet, bUp, bDn, bRun2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> frameAtt, runAtt2;

    bool lastFrameOn = true;
    int lastSize = -1;
    double lastMs = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumEditor)
};
