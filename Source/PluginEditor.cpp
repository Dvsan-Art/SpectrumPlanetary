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

    // cabecera: FRAME y SET, mismo tamano, centrados verticalmente en la cabecera
    bFrame.setButtonText ("FRAME");
    bFrame.setClickingTogglesState (true);
    frameAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (A, "frameOn", bFrame);
    place (bFrame, 436, 21, 56, 22);
    bSet.setButtonText ("SET");
    bSet.onClick = [this] { settings.setVisible (! settings.isVisible()); settings.toFront (false); };
    place (bSet, 498, 21, 56, 22);

    // panel lateral (Frame Adjust)
    bUp.setButtonText ("+"); bDn.setButtonText ("-");
    bUp.onClick = [this] { pickMidi (currentMidi (proc.apvts) + 1); };
    bDn.onClick = [this] { pickMidi (currentMidi (proc.apvts) - 1); };
    place (bUp, 436, 88, 56, 22); place (bDn, 498, 88, 56, 22);
    bWidth = std::make_unique<CycleButton> (*rp (A, "width"), 0, 5, "WIDTH: ");
    place (*bWidth, 436, 128, 118, 22);
    bRoot = std::make_unique<CycleButton> (*rp (A, "root"), 0, 11);  place (*bRoot, 436, 168, 118, 22);
    bOct  = std::make_unique<CycleButton> (*rp (A, "oct"), 1, 9);    place (*bOct, 436, 208, 118, 22);

    place (monL, 433, 236, 62, 62); place (monR, 495, 236, 62, 62);
    place (monM, 433, 300, 62, 62); place (monD, 495, 300, 62, 62);
    monL.onClick = [this] { toggleMonitor (1); };
    monR.onClick = [this] { toggleMonitor (2); };
    monM.onClick = [this] { toggleMonitor (3); };
    monD.onClick = [this] { toggleMonitor (4); };

    bRun2.setButtonText ("Analyze"); bRun2.setClickingTogglesState (true);
    runAtt2 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (A, "run", bRun2);
    place (bRun2, 436, 364, 118, 20);

    // vistas
    place (spectrum, 10, 68, 410, 316);
    spectrum.onPick = [this] (int m) { pickMidi (m); };
    place (meters, 566, 64, 188, 236);
    place (phase, 566, 304, 188, 158);
    place (keyboard, 12, 398, 542, 62);
    keyboard.onPick = [this] (int m) { pickMidi (m); };
    place (settings, 6, 64, 554, 324);
    settings.setVisible (false);
    content.onReset = [this] { proc.requestReset(); };

    setSize (760, 470);
    setResizable (false, false);      // sin esquina de redimensionar: el tamano sale de SET
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

void SpectrumEditor::toggleMonitor (int idx)
{
    // exclusivo: si ya esta encendido se apaga (vuelve a Stereo); si no, cambia al elegido
    const int cur = (int) pv (proc.apvts, "monitor");
    setParam (proc.apvts, "monitor", (float) (cur == idx ? 0 : idx));
}

void SpectrumEditor::pickMidi (int midi)
{
    midi = juce::jlimit (24, 131, midi);     // C1 .. B9
    setParam (proc.apvts, "root", (float) (midi % 12));
    setParam (proc.apvts, "oct", (float) (midi / 12 - 1));
}

void SpectrumEditor::timerCallback()
{
    // tamano de interfaz desde SET: Normal, Grande, Maximo (proporcion fija)
    const int sz = juce::jlimit (0, 2, (int) pv (proc.apvts, "uiSize"));
    if (sz != lastSize)
    {
        lastSize = sz;
        static const int W[] = { 760, 950, 1140 };
        static const int H[] = { 470, 588, 705 };
        setSize (W[sz], H[sz]);
    }

    // FRAME apagado: deshabilita y atenua los controles de Frame Adjust
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

    const int mon = (int) pv (proc.apvts, "monitor");
    monL.setActive (mon == 1); monR.setActive (mon == 2); monM.setActive (mon == 3); monD.setActive (mon == 4);

    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.001, 0.25, (now - lastMs) / 1000.0);
    lastMs = now;
    if (pv (proc.apvts, "run") > 0.5f) { meters.tick (dt); phase.tick (dt); }
    spectrum.repaint();
    keyboard.repaint();
}
