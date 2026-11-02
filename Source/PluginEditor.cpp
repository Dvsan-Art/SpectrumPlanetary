#include "PluginEditor.h"

using namespace ui;

SpectrumEditor::SpectrumEditor (SpectrumProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      spectrum (p.apvts, p.analyzer), meters (p), phase (p), keyboard (p.apvts), settings (p.apvts)
{
    auto& A = proc.apvts;
    content.setBounds (0, 0, 760, 470);
    content.setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    auto place = [this] (juce::Component& c, int x, int y, int w, int h) { content.addAndMakeVisible (c); c.setBounds (x, y, w, h); };
    auto dbText = [] (double v) { return juce::String ((int) std::lround (v)) + "dB"; };

    // cabecera
    knIn  = std::make_unique<KnobBox> (A, "inGain", "", dbText);   place (*knIn, 224, 6, 34, 52);
    knOut = std::make_unique<KnobBox> (A, "outGain", "", dbText);  place (*knOut, 262, 6, 34, 52);
    bRes = std::make_unique<CycleButton> (*A.getParameter ("res"), 0, 2);   place (*bRes, 302, 30, 58, 22);
    bWin = std::make_unique<CycleButton> (*A.getParameter ("win"), 0, 1);   place (*bWin, 364, 30, 60, 22);
    bRun1.setButtonText ("Analyze"); bRun1.setClickingTogglesState (true);
    runAtt1 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (A, "run", bRun1);
    place (bRun1, 430, 30, 58, 22);
    bSet.setButtonText ("SET");
    bSet.onClick = [this] { settings.setVisible (! settings.isVisible()); settings.toFront (false); };
    place (bSet, 494, 30, 58, 22);

    // panel lateral
    bUp.setButtonText ("+"); bDn.setButtonText ("-");
    bUp.onClick = [this] { pickMidi (currentMidi (proc.apvts) + 1); };
    bDn.onClick = [this] { pickMidi (currentMidi (proc.apvts) - 1); };
    place (bUp, 436, 88, 56, 22); place (bDn, 498, 88, 56, 22);
    bWidth = std::make_unique<CycleButton> (*A.getParameter ("width"), 0, 6, "WIDTH: " + pmStr());
    place (*bWidth, 436, 128, 118, 22);
    bRoot = std::make_unique<CycleButton> (*A.getParameter ("root"), 0, 11);  place (*bRoot, 436, 168, 118, 22);
    bOct  = std::make_unique<CycleButton> (*A.getParameter ("oct"), 1, 7);    place (*bOct, 436, 208, 118, 22);
    knGain   = std::make_unique<KnobBox> (A, "gain", "Gain", dbText);
    knBands  = std::make_unique<KnobBox> (A, "bands", "Bands", [] (double v) { return juce::String (kBands[juce::jlimit (0, 3, (int) std::lround (v))]) + "/oct"; });
    knDecay  = std::make_unique<KnobBox> (A, "peakDecay", "Decay", [] (double v) { return juce::String (v, 0) + "dB/s"; });
    knSmooth = std::make_unique<KnobBox> (A, "integ", "Time", [] (double v) { return juce::String ((int) std::lround (v)) + "ms"; });
    place (*knGain, 433, 236, 62, 62); place (*knBands, 495, 236, 62, 62);
    place (*knDecay, 433, 300, 62, 62); place (*knSmooth, 495, 300, 62, 62);
    bRun2.setButtonText ("Analyze"); bRun2.setClickingTogglesState (true);
    runAtt2 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (A, "run", bRun2);
    place (bRun2, 436, 364, 118, 20);

    // vistas
    place (spectrum, 10, 68, 410, 316);
    spectrum.onPick = [this] (int m) { pickMidi (m); };
    place (meters, 566, 64, 188, 236);
    place (phase, 566, 304, 188, 158);
    place (keyboard, 12, 400, 542, 58);
    keyboard.onPick = [this] (int m) { pickMidi (m); };
    place (settings, 6, 64, 554, 324);
    settings.setVisible (false);
    content.onReset = [this] { proc.requestReset(); };

    setSize (760, 470);
    setResizable (true, true);
    setResizeLimits (570, 352, 1520, 940);
    getConstrainer()->setFixedAspectRatio (760.0 / 470.0);
    lastMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

SpectrumEditor::~SpectrumEditor()
{
    stopTimer();
    content.setLookAndFeel (nullptr);
}

void SpectrumEditor::resized()
{
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / 760.f));
}

void SpectrumEditor::pickMidi (int midi)
{
    midi = juce::jlimit (24, 95, midi);
    setParam (proc.apvts, "root", (float) (midi % 12));
    setParam (proc.apvts, "oct", (float) (midi / 12 - 1));
}

void SpectrumEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.001, 0.25, (now - lastMs) / 1000.0);
    lastMs = now;
    if (pv (proc.apvts, "run") > 0.5f) { meters.tick (dt); phase.tick(); }
    spectrum.repaint();
}
