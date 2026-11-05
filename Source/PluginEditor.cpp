#include "PluginEditor.h"

using namespace ui;

namespace
{
    constexpr int baseW = 760, baseH = 470;
    double scaleFor (int idx) { return idx == 2 ? 1.4 : (idx == 1 ? 1.2 : 1.0); }
}

SpectrumEditor::SpectrumEditor (SpectrumProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      spectrum (p.apvts, p.analyzer), meters (p), phase (p), keyboard (p.apvts), settings (p.apvts)
{
    auto& A = proc.apvts;
    content.setBounds (0, 0, baseW, baseH);
    content.setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    auto place = [this] (juce::Component& c, int x, int y, int w, int h) { content.addAndMakeVisible (c); c.setBounds (x, y, w, h); };
    auto dbText = [] (double v) { return juce::String ((int) std::lround (v)) + "dB"; };

    // cabecera: FRAME y SET, mismo tamano, simetricos
    bFrame.setButtonText ("FRAME");
    bFrame.setClickingTogglesState (true);
    frameAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (A, "frameOn", bFrame);
    place (bFrame, 280, 19, 110, 26);
    bSet.setButtonText ("SET");
    bSet.onClick = [this] { settings.setVisible (! settings.isVisible()); settings.toFront (false); };
    place (bSet, 400, 19, 110, 26);

    // panel lateral (Frame Adjust)
    bUp.setButtonText ("+"); bDn.setButtonText ("-");
    bUp.onClick = [this] { pickMidi (currentMidi (proc.apvts) + 1); };
    bDn.onClick = [this] { pickMidi (currentMidi (proc.apvts) - 1); };
    place (bUp, 436, 88, 56, 22); place (bDn, 498, 88, 56, 22);
    bWidth = std::make_unique<CycleButton> (*rp (A, "width"), 0, 5, "WIDTH: ");
    place (*bWidth, 436, 128, 118, 22);
    bRoot = std::make_unique<CycleButton> (*rp (A, "root"), 0, 11);  place (*bRoot, 436, 168, 118, 22);
    bOct  = std::make_unique<CycleButton> (*rp (A, "oct"), 1, 9);    place (*bOct, 436, 208, 118, 22);

    knGain  = std::make_unique<KnobBox> (A, "gain", "Gain", dbText);
    knRange = std::make_unique<KnobBox> (A, "range", "Range", dbText);
    knDecay = std::make_unique<KnobBox> (A, "peakDecay", "Decay", [] (double v) { return juce::String (v, 0) + "dB/s"; });
    knTime  = std::make_unique<KnobBox> (A, "avgTime", "Time", [] (double v) { return juce::String (v / 1000.0, 1) + "s"; });
    place (*knGain, 433, 236, 62, 62);  place (*knRange, 495, 236, 62, 62);
    place (*knDecay, 433, 300, 62, 62); place (*knTime, 495, 300, 62, 62);

    bRun2.setButtonText ("Analyze"); bRun2.setClickingTogglesState (true);
    runAtt2 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (A, "run", bRun2);
    place (bRun2, 436, 364, 118, 20);

    // vistas
    place (spectrum, 10, 68, 410, 316);
    spectrum.onPick = [this] (int m) { pickMidi (m); };
    place (meters, 566, 64, 188, 236);
    place (phase, 566, 304, 188, 158);
    place (keyboard, 12, 402, 542, 56);
    keyboard.onPick = [this] (int m) { pickMidi (m); };
    place (settings, 6, 64, 554, 324);
    settings.setVisible (false);
    content.onReset = [this] { proc.requestReset(); };

    setResizable (false, false);
    applySize();
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
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) baseW));
}

void SpectrumEditor::applySize()
{
    const int idx = (int) pv (proc.apvts, "size");
    lastSize = idx;
    const double s = scaleFor (idx);
    setSize ((int) std::lround (baseW * s), (int) std::lround (baseH * s));
}

void SpectrumEditor::pickMidi (int midi)
{
    midi = juce::jlimit (24, 131, midi);                 // C1 .. B9 (15.8 kHz)
    setParam (proc.apvts, "root", (float) (midi % 12));
    setParam (proc.apvts, "oct", (float) (midi / 12 - 1));
}

void SpectrumEditor::timerCallback()
{
    const bool on = pv (proc.apvts, "frameOn") > 0.5f;
    if (on != lastFrameOn)
    {
        lastFrameOn = on;
        const float a = on ? 1.f : 0.35f;
        bUp.setEnabled (on); bDn.setEnabled (on);
        bWidth->setEnabled (on); bRoot->setEnabled (on); bOct->setEnabled (on);
        bUp.setAlpha (a); bDn.setAlpha (a); bWidth->setAlpha (a); bRoot->setAlpha (a); bOct->setAlpha (a);
        content.frameOn = on;
        content.repaint();
    }

    if ((int) pv (proc.apvts, "size") != lastSize) applySize();

    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.001, 0.25, (now - lastMs) / 1000.0);
    lastMs = now;
    if (pv (proc.apvts, "run") > 0.5f) { meters.tick (dt); phase.tick(); }
    spectrum.repaint();
    keyboard.repaint();
}
