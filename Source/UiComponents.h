#pragma once
#include "PluginProcessor.h"
#include "FilterBank.h"
#include <functional>
#include <memory>
#include <vector>

namespace ui
{
static const juce::Colour kOrange (0xffff9a2e), kText (0xffe2e5ea), kMuted (0xff8e96a3), kLine (0xff2d323b),
                          kPanel (0xff1a1d23), kDeep (0xff0f1216), kBtn (0xff262a32);
static const double kWidths[] = { 2.0, 4.0, 6.0, 8.0, 10.0, 12.0 };
static const int kBands[] = { 3, 4, 6 };
static const char* kNote[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static const char* kSolf[] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };

inline juce::String pmStr() { return juce::String::fromUTF8 ("\xc2\xb1"); }
inline juce::String infStr() { return juce::String::fromUTF8 ("-\xe2\x88\x9e"); }
inline double midiFreq (double m) { return 440.0 * std::pow (2.0, (m - 69.0) / 12.0); }
inline void setF (juce::Graphics& g, float h, bool bold = false) { g.setFont (h); if (bold) g.setFont (g.getCurrentFont().boldened()); }
inline float pv (juce::AudioProcessorValueTreeState& a, const char* id) { return a.getRawParameterValue (id)->load(); }
inline int currentMidi (juce::AudioProcessorValueTreeState& a) { return ((int) pv (a, "oct") + 1) * 12 + (int) pv (a, "root"); }
inline juce::String noteName (int midi) { return juce::String (kNote[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1); }
inline juce::RangedAudioParameter* rp (juce::AudioProcessorValueTreeState& a, const char* id)
{ return dynamic_cast<juce::RangedAudioParameter*> (a.getParameter (id)); }
inline void setParam (juce::AudioProcessorValueTreeState& a, const char* id, float denorm)
{
    auto* p = rp (a, id);
    if (p == nullptr) return;
    p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (denorm)); p->endChangeGesture();
}

// ---------------------------------------------------------------- estilo
class PlanetLnF : public juce::LookAndFeel_V4
{
public:
    PlanetLnF()
    {
        setColourScheme (juce::LookAndFeel_V4::getDarkColourScheme());
        setColour (juce::Slider::thumbColourId, kOrange);
        setColour (juce::Slider::trackColourId, juce::Colour (0xff3a404a));
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xff20242b));
        setColour (juce::Slider::textBoxTextColourId, kText);
        setColour (juce::Slider::textBoxBackgroundColourId, kDeep);
        setColour (juce::Slider::textBoxOutlineColourId, kLine);
        setColour (juce::ComboBox::backgroundColourId, kBtn);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a404a));
        setColour (juce::ComboBox::textColourId, kText);
        setColour (juce::Label::textColourId, kText);
        setColour (juce::PopupMenu::backgroundColourId, kPanel);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kOrange.darker (0.5f));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        const float r = (float) juce::jmin (w, h) * 0.5f - 2.f, cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a515d), cx - r * 0.3f, cy - r * 0.4f, juce::Colour (0xff1a1d23), cx + r, cy + r, true));
        g.fillEllipse (cx - r, cy - r, 2.f * r, 2.f * r);
        g.setColour (kOrange); g.drawEllipse (cx - r, cy - r, 2.f * r, 2.f * r, 2.f);
        const float a = a0 + pos * (a1 - a0);
        juce::Path p; p.addRoundedRectangle (-1.5f, -r + 3.f, 3.f, r * 0.45f, 1.5f);
        g.setColour (juce::Colours::white);
        g.fillPath (p, juce::AffineTransform::rotation (a).translated (cx, cy));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? juce::Colour (0xff343a43) : hover ? juce::Colour (0xff2d323b) : kBtn);
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (b.getToggleState() ? kOrange : juce::Colour (0xff3a404a));
        g.drawRoundedRectangle (r, 4.f, 1.f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setColour (b.getToggleState() ? kOrange : kText);
        setF (g, juce::jmin (13.f, (float) b.getHeight() * 0.58f));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, true);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool) override
    {
        const float s = 14.f, y = ((float) b.getHeight() - s) * 0.5f;
        g.setColour (kDeep); g.fillRoundedRectangle (0.f, y, s, s, 3.f);
        g.setColour (juce::Colour (0xff3a404a)); g.drawRoundedRectangle (0.5f, y + 0.5f, s - 1.f, s - 1.f, 3.f, 1.f);
        if (b.getToggleState()) { g.setColour (kOrange); g.fillRoundedRectangle (3.f, y + 3.f, s - 6.f, s - 6.f, 2.f); }
        g.setColour (kText); setF (g, 12.f);
        g.drawText (b.getButtonText(), (int) s + 6, 0, b.getWidth() - (int) s - 6, b.getHeight(), juce::Justification::centredLeft, true);
    }
};

// ---------------------------------------------------------------- knob con textos
class KnobBox : public juce::Component
{
public:
    KnobBox (juce::AudioProcessorValueTreeState& a, const char* id, const juce::String& topText, std::function<juce::String (double)> fmt)
        : top (topText), fmtFn (std::move (fmt))
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setMouseDragSensitivity (150);
        s.onValueChange = [this] { repaint(); };
        addAndMakeVisible (s);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, id, s);
    }
    void resized() override
    {
        const int topH = top.isEmpty() ? 0 : 12, h = getHeight() - topH - 12, d = juce::jmin (getWidth(), h);
        s.setBounds ((getWidth() - d) / 2, topH + (h - d) / 2, d, d);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (kText); setF (g, 10.5f);
        if (top.isNotEmpty()) g.drawText (top, 0, 0, getWidth(), 12, juce::Justification::centred, false);
        g.drawText (fmtFn ? fmtFn (s.getValue()) : juce::String(), 0, getHeight() - 12, getWidth(), 12, juce::Justification::centred, false);
    }
private:
    juce::Slider s;
    juce::String top;
    std::function<juce::String (double)> fmtFn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// ---------------------------------------------------------------- boton que rota valores de un parametro
class CycleButton : public juce::TextButton
{
public:
    CycleButton (juce::RangedAudioParameter& p, int lo, int hi, juce::String prefix = {}, juce::String suffix = {})
        : param (p), lo_ (lo), hi_ (hi), pre (std::move (prefix)), suf (std::move (suffix)),
          att (p, [this] (float v) { setButtonText (pre + param.getText (param.convertTo0to1 (v), 16) + suf); })
    {
        onClick = [this]
        {
            int i = (int) std::lround (param.convertFrom0to1 (param.getValue()));
            i = i >= hi_ ? lo_ : i + 1;
            att.setValueAsCompleteGesture ((float) i);
        };
        att.sendInitialUpdate();
    }
private:
    juce::RangedAudioParameter& param;
    int lo_, hi_;
    juce::String pre, suf;
    juce::ParameterAttachment att;
};

// ---------------------------------------------------------------- vista del espectro
class SpectrumView : public juce::Component
{
public:
    SpectrumView (juce::AudioProcessorValueTreeState& a, FilterBankAnalyzer& e) : apvts (a), eng (e) {}
    std::function<void (int)> onPick;

    void mouseMove (const juce::MouseEvent& e) override { hov = e.position; hasHov = true; }
    void mouseExit (const juce::MouseEvent&) override { hasHov = false; }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const float L = 30.f, Wd = (float) getWidth() - L - 6.f;
        const double fmin = eng.freqMinA.load(), fmax = eng.freqMaxA.load();
        if (onPick == nullptr || fmin <= 0.0 || fmax <= fmin || e.position.x < L) return;
        onPick (midiAt (e.position.x, L, Wd, fmin, fmax));
    }

    static float xOf (double f, float L, float Wd, double fmin, double fmax)
    {
        if (fmin <= 0.0 || fmax <= fmin || f <= 0.0) return L;
        return L + Wd * (float) (std::log (f / fmin) / std::log (fmax / fmin));
    }

    static int midiAt (float x, float L, float Wd, double fmin, double fmax)
    {
        const double f = fmin * std::pow (fmax / fmin, (double) (x - L) / (double) Wd);
        return (int) std::lround (69.0 + 12.0 * std::log2 (f / 440.0));
    }

    void paint (juce::Graphics& g) override
    {
        auto& A = apvts;
        const float ceilDb = pv (A, "ceiling"), range = pv (A, "range"), gridDb = pv (A, "gridDb");
        const float L = 30.f, T = 6.f, Wd = (float) getWidth() - L - 6.f, Ht = (float) getHeight() - T - 20.f, B = T + Ht;
        const double fmin = eng.freqMinA.load(), fmax = eng.freqMaxA.load();
        const int nb = juce::jlimit (0, FilterBankAnalyzer::maxBands, eng.numBands.load());
        auto X = [=] (double f) { return xOf (f, L, Wd, fmin, fmax); };
        auto Y = [=] (float db) { return T + (ceilDb - juce::jlimit (ceilDb - range, ceilDb, db)) / range * Ht; };

        g.fillAll (juce::Colour (0xff101318));
        setF (g, 10.f);
        for (float v = std::floor (ceilDb / gridDb) * gridDb; v >= ceilDb - range; v -= gridDb)
        {
            if (v > ceilDb) continue;
            const int y = (int) std::floor (Y (v));
            g.setColour (juce::Colour (0xff2a3038)); g.drawHorizontalLine (y, L, L + Wd);
            g.setColour (kMuted); g.drawText (juce::String ((int) v), 0, y - 6, (int) L - 4, 12, juce::Justification::centredRight, false);
        }

        const int gm = (int) pv (A, "grid");
        auto vline = [&] (double f, const char* label)
        {
            const float x = std::floor (X (f));
            if (x < L || x > L + Wd) return;
            g.setColour (juce::Colour (0xff3a424d)); g.drawVerticalLine ((int) x, T, B);
            if (label != nullptr) { g.setColour (kMuted); g.drawText (label, (int) x - 20, (int) B + 4, 40, 12, juce::Justification::centred, false); }
        };
        if (gm == 0)
        {
            static const double fr[] = { 31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
            static const char* tx[] = { "31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
            for (int i = 0; i < 10; ++i) vline (fr[i], tx[i]);
        }
        else if (gm == 1)
        {
            for (double dec = 10.0; dec <= 10000.0; dec *= 10.0)
                for (int k = 1; k < 10; ++k) vline (k * dec, nullptr);
            static const double fr[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
            static const char* tx[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
            for (int i = 0; i < 10; ++i) { const float x = X (fr[i]); if (x >= L && x <= L + Wd) { g.setColour (kMuted); g.drawText (tx[i], (int) x - 20, (int) B + 4, 40, 12, juce::Justification::centred, false); } }
        }

        float bars[FilterBankAnalyzer::maxBands], holds[FilterBankAnalyzer::maxBands], avgs[FilterBankAnalyzer::maxBands];
        for (int i = 0; i < nb; ++i) { bars[i] = eng.bar[i].load(); holds[i] = eng.hold[i].load(); avgs[i] = eng.avg[i].load(); }
        const int  style  = (int) pv (A, "style");
        const bool showPk = pv (A, "peakOn") > 0.5f;
        const float spacing = nb > 1 ? Wd / (float) (nb - 1) : Wd;
        const float bw = juce::jmax (1.f, spacing * 0.8f);

        if (showPk && nb > 0)
        {
            juce::ColourGradient grad (juce::Colour (0xffffc15e), 0.f, T, juce::Colour (0xffb8600a), 0.f, B, false);
            g.setGradientFill (grad);
            for (int i = 0; i < nb; ++i)
            {
                const float xc = X (eng.fc[i]), y = Y (bars[i]);
                g.fillRect (xc - bw * 0.5f, y, bw, B - y);
            }
        }

        auto line = [&] (const float* arr, juce::Colour c, float thick)
        {
            if (nb < 2) return;
            if (style == 0)
            {
                g.setColour (c);
                for (int i = 0; i < nb; ++i) g.fillRect (X (eng.fc[i]) - bw * 0.5f, Y (arr[i]), bw, thick);
                return;
            }
            juce::Path p;
            for (int i = 0; i < nb; ++i) { const float x = X (eng.fc[i]), y = Y (arr[i]); if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
            if (style == 2)
            {
                juce::Path fillP (p); fillP.lineTo (X (eng.fc[nb - 1]), B); fillP.lineTo (X (eng.fc[0]), B); fillP.closeSubPath();
                g.setColour (c.withAlpha (0.25f)); g.fillPath (fillP);
            }
            g.setColour (c); g.strokePath (p, juce::PathStrokeType (thick));
        };
        if (pv (A, "avgOn") > 0.5f) line (avgs, juce::Colour (0xe6ffe27a), 3.f);
        if (pv (A, "holdOn") > 0.5f) line (holds, juce::Colour (0xe6e8f0ff), 2.f);

        // marco de la nota seleccionada (+1, +3, +6, +9, +12 semitonos)
        const int m = currentMidi (A);
        const bool frameOn = pv (A, "frameOn") > 0.5f;
        const double w = kWidths[juce::jlimit (0, 5, (int) pv (A, "width"))];
        const float x1 = X (midiFreq (m - w)), x2 = X (midiFreq (m + w)), xc = X (midiFreq (m));
        if (frameOn)
        {
            g.setColour (juce::Colour (0x24beb4a0)); g.fillRect (x1, T, x2 - x1, Ht);
            g.setColour (kOrange); g.drawRect (x1, T, x2 - x1, Ht, 2.f);
            g.setColour (kOrange.withAlpha (0.8f));
            for (float y = T; y < B; y += 8.f) g.fillRect (xc - 0.5f, y, 1.f, 4.f);
        }

        // indicador superior derecho: nota | frecuencia (cursor o nota enmarcada)
        const bool inside = hasHov && hov.x > L && hov.x < L + Wd && hov.y > T && hov.y < B && fmin > 0.0;
        const int infoMidi = inside ? midiAt (hov.x, L, Wd, fmin, fmax) : m;
        const juce::String info = noteName (infoMidi) + " | " + juce::String (midiFreq (infoMidi), 2) + " Hz";
        if (frameOn || inside)
        {
        setF (g, 12.f, true);
        const float ibw = (float) info.length() * 7.4f + 16.f, ibx = L + Wd - ibw - 4.f;
        g.setColour (juce::Colour (0xe6161820)); g.fillRoundedRectangle (ibx, T + 4.f, ibw, 20.f, 4.f);
        g.setColour (juce::Colour (0xff6a5a44)); g.drawRoundedRectangle (ibx, T + 4.f, ibw, 20.f, 4.f, 1.f);
        g.setColour (kOrange); g.drawText (info, (int) ibx, (int) T + 4, (int) ibw, 20, juce::Justification::centred, false);
        }

        if (inside) { g.setColour (juce::Colours::white.withAlpha (0.45f)); g.drawVerticalLine ((int) hov.x, T, B); }
    }
private:
    juce::AudioProcessorValueTreeState& apvts;
    FilterBankAnalyzer& eng;
    juce::Point<float> hov;
    bool hasHov = false;
};

// ---------------------------------------------------------------- teclado: 3 octavas visibles, barra de desplazamiento
class KeyboardView : public juce::Component
{
public:
    explicit KeyboardView (juce::AudioProcessorValueTreeState& a) : apvts (a) {}
    std::function<void (int)> onPick;
    int start = 48;
    static constexpr int span = 36;

    static bool isBlack (int m) { const int n = m % 12; return n == 1 || n == 3 || n == 6 || n == 8 || n == 10; }

    void paint (juce::Graphics& g) override
    {
        const float W = (float) getWidth(), H = (float) getHeight(), kw = W / 21.f, bw = kw * 0.6f;
        const int root = currentMidi (apvts);
        const double w = kWidths[juce::jlimit (0, 5, (int) pv (apvts, "width"))];
        g.fillAll (juce::Colour (0xff0e1013));
        for (int m = start; m < start + span; ++m)
        {
            if (isBlack (m)) continue;
            const float x = whitesBefore (m) * kw;
            const juce::Colour c = m == root ? kOrange : std::abs ((double) (m - root)) <= w ? juce::Colour (0xffffc98a) : juce::Colour (0xffe8e8e8);
            g.setColour (c); g.fillRect (x, 0.f, kw, H);
            g.setColour (juce::Colour (0xff222222)); g.drawRect (x, 0.f, kw, H, 1.f);
            if (m % 12 == 0) { g.setColour (juce::Colour (0xff444444)); setF (g, 10.f); g.drawText ("C" + juce::String (m / 12 - 1), (int) x, (int) H - 16, (int) kw, 14, juce::Justification::centred, false); }
        }
        for (int m = start; m < start + span; ++m)
        {
            if (! isBlack (m)) continue;
            const float cx = whitesBefore (m) * kw;
            const juce::Colour c = m == root ? juce::Colour (0xffd97a14) : std::abs ((double) (m - root)) <= w ? juce::Colour (0xff9a5a1a) : juce::Colour (0xff111111);
            g.setColour (c); g.fillRect (cx - bw * 0.5f, 0.f, bw, H * 0.62f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const float kw = (float) getWidth() / 21.f, bw = kw * 0.6f;
        for (int m = start; m < start + span; ++m)
            if (isBlack (m) && juce::Rectangle<float> (whitesBefore (m) * kw - bw * 0.5f, 0.f, bw, (float) getHeight() * 0.62f).contains (e.position))
            { if (onPick) onPick (m); return; }
        for (int m = start; m < start + span; ++m)
            if (! isBlack (m) && juce::Rectangle<float> (whitesBefore (m) * kw, 0.f, kw, (float) getHeight()).contains (e.position))
            { if (onPick) onPick (m); return; }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    int whitesBefore (int m) const { int c = 0; for (int i = start; i < m; ++i) if (! isBlack (i)) ++c; return c; }
};

class KeyScroll : public juce::Component
{
public:
    std::function<void (int)> onStart;
    int start = 48;
    static constexpr int minStart = 24, maxStart = 96, span = 36;

    void paint (juce::Graphics& g) override
    {
        const float W = (float) getWidth(), H = (float) getHeight(), tw = thumbW(), tx = thumbX (start, W, tw);
        g.setColour (juce::Colour (0xff0e1013)); g.fillRoundedRectangle (0.f, 0.f, W, H, 3.f);
        g.setColour (kOrange.withAlpha (0.85f)); g.fillRoundedRectangle (tx, 0.f, tw, H, 3.f);
    }
    void mouseDown (const juce::MouseEvent& e) override { moveTo (e.position.x); }
    void mouseDrag (const juce::MouseEvent& e) override { moveTo (e.position.x); }

private:
    float thumbW() const { return (float) getWidth() * (float) span / 108.f; }
    static float thumbX (int s, float W, float tw) { return (W - tw) * (float) (s - minStart) / (float) (maxStart - minStart); }
    void moveTo (float x)
    {
        const float W = (float) getWidth(), tw = thumbW();
        const float t = juce::jlimit (0.f, 1.f, (x - tw * 0.5f) / (W - tw));
        int s = minStart + 12 * (int) std::lround (t * (float) (maxStart - minStart) / 12.f);
        s = juce::jlimit (minStart, maxStart, s);
        if (s != start) { start = s; if (onStart) onStart (s); repaint(); }
    }
};

// ---------------------------------------------------------------- panel LUFS / True Peak / Peak / RMS
class MeterPanel : public juce::Component
{
public:
    explicit MeterPanel (SpectrumProcessor& p) : proc (p) { bv.fill (-100.f); mk.fill (-100.f); vals.fill (-100.f); }
    int mode = 0;

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto pt = e.getPosition();
        if (pt.y < 26 || (pt.y > 206 && pt.y < 232)) { mode = (mode + 1) % 4; repaint(); }
    }

    void tick (double dt)
    {
        auto& m = proc.meters;
        const float mom = m.mom.load(), sh = m.sh.load(), integ = m.integ.load(), mL = m.mL.load(), mR = m.mR.load();
        const float pL = m.pkL.exchange (-100.f), pR = m.pkR.exchange (-100.f), tL = m.tpL.exchange (-100.f), tR = m.tpR.exchange (-100.f);
        const float rL = m.rmsL.load(), rR = m.rmsR.load(), tpMax = m.tpMax.load(), pkMax = m.pkMax.load();
        const float fdt = (float) dt;
        float t0 = 0.f, t1 = 0.f; bool slow = false;
        if (mode == 0) { vals[0] = mom; vals[1] = sh; vals[2] = integ; t0 = mL; t1 = mR; slow = true; }
        else if (mode == 1) { holdTp = std::max (std::max (tL, tR), holdTp - 6.f * fdt); vals[0] = std::max (tL, tR); vals[1] = holdTp; vals[2] = tpMax; t0 = tL; t1 = tR; }
        else if (mode == 2) { vals[0] = pL; vals[1] = pR; vals[2] = pkMax; t0 = pL; t1 = pR; }
        else { vals[0] = rL; vals[1] = rR; vals[2] = (float) (10.0 * std::log10 ((std::pow (10.0, rL / 10.0) + std::pow (10.0, rR / 10.0)) / 2.0 + 1.0e-12)); t0 = rL; t1 = rR; slow = true; }
        const float tg[4] = { t0, t1, t0, t1 };
        for (int i = 0; i < 4; ++i)
        {
            const float x = std::max (tg[i], -100.f);
            if (i < 2) bv[(size_t) i] = x > bv[(size_t) i] ? x : std::max (x, bv[(size_t) i] - 60.f * fdt);
            else if (slow) bv[(size_t) i] += (x - bv[(size_t) i]) * (1.f - std::exp (-fdt / 1.5f));
            else bv[(size_t) i] = x > bv[(size_t) i] ? x : std::max (x, bv[(size_t) i] - 6.f * fdt);
            mk[(size_t) i] = std::max (bv[(size_t) i], mk[(size_t) i] - 8.f * fdt);
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        static const char* titles[] = { "LUFS", "TRUE PEAK", "PEAK", "RMS" };
        static const char* units[] = { "LUFS", "dBTP", "dBFS", "dBFS" };
        static const char* heads[4][3] = { { "Momentary", "Short Term", "Integrated" }, { "Actual", "Hold", "Max" }, { "Peak L", "Peak R", "Max" }, { "RMS L", "RMS R", "Average" } };
        const juce::Colour vc[3] = { juce::Colour (0xffff7a50), juce::Colour (0xffff9a60), juce::Colour (0xff4cd964) };

        g.setColour (kDeep); g.fillRoundedRectangle (4.f, 2.f, (float) getWidth() - 8.f, (float) getHeight() - 4.f, 6.f);
        g.setColour (kText); setF (g, 15.f, true);
        g.drawText (titles[mode], 0, 4, getWidth(), 20, juce::Justification::centred, false);
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (kText); setF (g, 9.5f);
            g.drawText (heads[mode][i], 4 + i * 60, 28, 60, 12, juce::Justification::centred, false);
            g.setColour (vc[i]); setF (g, 13.f, true);
            g.drawText (vals[(size_t) i] <= -99.f ? infStr() : juce::String (vals[(size_t) i], 1), 4 + i * 60, 41, 60, 16, juce::Justification::centred, false);
        }
        const float by = 64.f, bh = 140.f;
        auto bar = [&] (float x, float db, float mark)
        {
            g.setColour (juce::Colour (0xff16191e)); g.fillRect (x, by, 17.f, bh);
            const float p = juce::jlimit (0.f, 1.f, (db + 48.f) / 48.f);
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion ((int) x, (int) (by + bh * (1.f - p)), 18, (int) (bh * p) + 1);
                juce::ColourGradient gr (juce::Colour (0xffe53a1a), 0.f, by, juce::Colour (0xff1e9a43), 0.f, by + bh, false);
                gr.addColour (0.15, juce::Colour (0xffff8a1f)); gr.addColour (0.35, juce::Colour (0xfff0c93a)); gr.addColour (0.65, juce::Colour (0xff46d066));
                g.setGradientFill (gr); g.fillRect (x, by, 17.f, bh);
            }
            const float pm = juce::jlimit (0.f, 1.f, (mark + 48.f) / 48.f);
            g.setColour (juce::Colours::white); g.fillRect (x, by + bh * (1.f - pm) - 1.f, 17.f, 2.f);
        };
        bar (10.f, bv[0], mk[0]); bar (29.f, bv[1], mk[1]);
        bar (142.f, bv[2], mk[2]); bar (161.f, bv[3], mk[3]);
        g.setColour (kMuted); setF (g, 9.5f);
        for (int i = 0; i < 9; ++i)
            g.drawText (juce::String (-6 * i), 50, (int) (by + (bh - 12.f) * (float) i / 8.f), 86, 12, juce::Justification::centred, false);
        g.setColour (kText); setF (g, 13.f, true);
        g.drawText (units[mode], 8, 210, 60, 18, juce::Justification::centredLeft, false);
        g.drawText (units[mode], 120, 210, 60, 18, juce::Justification::centredRight, false);
    }
private:
    SpectrumProcessor& proc;
    std::array<float, 4> bv {}, mk {};
    std::array<float, 3> vals {};
    float holdTp = -100.f;
};

// ---------------------------------------------------------------- correlacion de fase / vectorscopio semicircular
class PhasePanel : public juce::Component
{
public:
    explicit PhasePanel (SpectrumProcessor& p) : proc (p) { trail = juce::Image (juce::Image::RGB, 352, 232, true); }
    bool vec = false;

    void mouseDown (const juce::MouseEvent& e) override { if (e.getPosition().y < 26) { vec = ! vec; repaint(); } }

    void tick()
    {
        const float* RL = proc.ringLeft(); const float* RR = proc.ringRight();
        const int w = proc.getWritePos(), mask = SpectrumProcessor::ringSize - 1;
        double ll = 0, rr = 0, lr = 0, mm = 0, ss = 0;
        for (int i = 0; i < 2048; ++i)
        {
            const int j = (w - 2048 + i) & mask;
            const double l = RL[j], r = RR[j];
            ll += l * l; rr += r * r; lr += l * r; mm += (l + r) * (l + r) / 4.0; ss += (l - r) * (l - r) / 4.0;
        }
        const float c = (ll > 1e-9 && rr > 1e-9) ? (float) (lr / std::sqrt (ll * rr)) : 0.f;
        const float wd = (mm + ss > 1e-12) ? (float) (ss / (mm + ss)) : 0.f;
        corr += (c - corr) * 0.25f; width += (wd - width) * 0.25f;
        if (vec)
        {
            juce::Graphics g (trail);
            g.setColour (juce::Colour (0xff0c0e12).withAlpha (0.28f)); g.fillAll();
            g.setColour (kOrange.withAlpha (0.6f));
            const float x0 = 176.f, y0 = 188.f, R = 152.f;
            for (int i = 0; i < 512; ++i)
            {
                const int j = (w - 512 + i) & mask;
                const float l = RL[j], r = RR[j];
                float X = (r - l) * 0.5f, Y = (l + r) * 0.5f;
                if (Y < 0.f) { X = -X; Y = -Y; }
                const float rad = std::hypot (X, Y);
                if (rad < 1e-4f) continue;
                const float k = std::pow (std::min (rad, 1.f), 0.55f) / rad;
                g.fillRect (x0 + X * k * R - 1.f, y0 - Y * k * R - 1.f, 2.f, 2.f);
            }
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (kDeep); g.fillRoundedRectangle (4.f, 2.f, (float) getWidth() - 8.f, (float) getHeight() - 4.f, 6.f);
        g.setColour (kText); setF (g, 13.f, true);
        g.drawText (vec ? "VECTORSCOPE" : "PHASE CORRELATION", 0, 5, getWidth(), 18, juce::Justification::centred, false);
        const juce::String cs = (corr >= 0.f ? "+" : "") + juce::String (corr, 1);
        if (! vec)
        {
            g.setColour (corr < 0.f ? juce::Colour (0xffff5a4a) : corr < 0.3f ? juce::Colour (0xffffb24d) : juce::Colour (0xff4cd964));
            setF (g, 15.f, true); g.drawText (cs, 0, 32, getWidth(), 18, juce::Justification::centred, false);
            const juce::Rectangle<float> bar (14.f, 62.f, (float) getWidth() - 28.f, 16.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a2424), bar.getX(), 0.f, juce::Colour (0xff5a2424), bar.getRight(), 0.f, false));
            g.fillRect (bar);
            juce::ColourGradient gr (juce::Colour (0xff5a2424), bar.getX(), 0.f, juce::Colour (0xff5a2424), bar.getRight(), 0.f, false);
            gr.addColour (0.5, juce::Colour (0xff1f6a35)); g.setGradientFill (gr); g.fillRect (bar);
            g.setColour (kLine); g.drawRect (bar, 1.f);
            const float nx = bar.getX() + (corr + 1.f) * 0.5f * bar.getWidth();
            g.setColour (juce::Colours::white); g.fillRect (nx - 1.5f, bar.getY() - 4.f, 3.f, bar.getHeight() + 8.f);
            g.setColour (kText); setF (g, 11.f);
            g.drawText ("-1", 10, 84, 30, 14, juce::Justification::centredLeft, false);
            g.drawText ("+0.0", 0, 84, getWidth(), 14, juce::Justification::centred, false);
            g.drawText ("+1", getWidth() - 40, 84, 30, 14, juce::Justification::centredRight, false);
            g.setColour (kMuted); g.drawText ("Width " + juce::String ((int) std::lround (width * 100.f)) + "%", 0, 112, getWidth(), 14, juce::Justification::centred, false);
            return;
        }
        const float sx = 6.f, sy = 28.f, cx0 = sx + 88.f, cy0 = sy + 94.f, R = 76.f, pi = juce::MathConstants<float>::pi;
        g.drawImage (trail, juce::Rectangle<float> (sx, sy, 176.f, 116.f));
        auto pt = [&] (float deg, float r) { const float a = juce::degreesToRadians (deg); return juce::Point<float> (cx0 + r * std::sin (a), cy0 - r * std::cos (a)); };
        for (float a : { -67.5f, -22.5f, 22.5f, 67.5f }) { g.setColour (juce::Colour (0xff1f252c)); g.drawLine (juce::Line<float> (juce::Point<float> (cx0, cy0), pt (a, R)), 1.f); }
        for (float a : { -90.f, -45.f, 0.f, 45.f, 90.f }) { g.setColour (juce::Colour (0xff3a424d)); g.drawLine (juce::Line<float> (juce::Point<float> (cx0, cy0), pt (a, R)), 1.f); }
        setF (g, 9.f);
        const float lv[] = { 1.f, 0.5f, 0.25f, 0.0631f }; const char* lt[] = { "0", "-6", "-12", "-24" };
        for (int i = 0; i < 4; ++i)
        {
            const float rr = R * std::pow (lv[i], 0.55f);
            juce::Path p; p.addCentredArc (cx0, cy0, rr, rr, 0.f, -pi * 0.5f, pi * 0.5f, true);
            g.setColour (i == 0 ? juce::Colour (0xff6a7482) : juce::Colour (0xff2f3640)); g.strokePath (p, juce::PathStrokeType (i == 0 ? 1.5f : 1.f));
            if (i > 0) { g.setColour (kMuted); g.drawText (lt[i], (int) cx0 + 3, (int) (cy0 - rr) - 1, 22, 10, juce::Justification::centredLeft, false); }
        }
        for (float a = -90.f; a <= 90.f; a += 7.5f)
        {
            const bool big = std::fmod (std::abs (a), 45.f) < 0.01f;
            g.setColour (juce::Colour (0xff6a7482)); g.drawLine (juce::Line<float> (pt (a, R), pt (a, R + (big ? 5.f : 3.f))), 1.f);
        }
        g.setColour (juce::Colour (0xffb8c0cc)); setF (g, 11.f, true);
        auto txt = [&] (const juce::String& t, juce::Point<float> p) { g.drawText (t, (int) p.x - 20, (int) p.y - 6, 40, 12, juce::Justification::centred, false); };
        txt ("MONO", pt (0.f, R + 9.f)); txt ("L", pt (-45.f, R + 11.f)); txt ("R", pt (45.f, R + 11.f));
        txt ("S", juce::Point<float> (cx0 - R - 8.f, cy0 - 2.f)); txt ("S", juce::Point<float> (cx0 + R + 8.f, cy0 - 2.f));
        g.setColour (kMuted); setF (g, 10.5f);
        g.drawText ("CORR " + cs + "   WIDTH " + juce::String ((int) std::lround (width * 100.f)) + "%", 0, (int) cy0 + 6, getWidth(), 14, juce::Justification::centred, false);
    }
private:
    SpectrumProcessor& proc;
    juce::Image trail;
    float corr = 0.f, width = 0.f;
};

// ---------------------------------------------------------------- ventana de ajustes (como "Spectrum Analyzer Settings")
class SettingsPanel : public juce::Component
{
public:
    explicit SettingsPanel (juce::AudioProcessorValueTreeState& a) : apvts (a)
    {
        auto label = [this] (const juce::String& t, bool hdr)
        {
            auto l = std::make_unique<juce::Label>();
            l->setText (t, juce::dontSendNotification);
            l->setFont (l->getFont().withHeight (hdr ? 14.f : 12.f));
            if (hdr) { l->setFont (l->getFont().boldened()); l->setColour (juce::Label::textColourId, kOrange); }
            addAndMakeVisible (*l);
            auto* raw = l.get(); owned.push_back (std::move (l)); return raw;
        };
        auto hdr = [&] (int col, const juce::String& t) { items.push_back ({ col, label (t, true), nullptr }); };
        auto combo = [&] (int col, const juce::String& t, const char* id)
        {
            auto cb = std::make_unique<juce::ComboBox>();
            cb->addItemList (dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id))->choices, 1);
            addAndMakeVisible (*cb);
            cAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, *cb));
            items.push_back ({ col, label (t, false), cb.get() }); owned.push_back (std::move (cb));
        };
        auto slider = [&] (int col, const juce::String& t, const char* id, int dec)
        {
            auto sl = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
            sl->setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
            sl->setNumDecimalPlacesToDisplay (dec);
            addAndMakeVisible (*sl);
            sAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, *sl));
            items.push_back ({ col, label (t, false), sl.get() }); owned.push_back (std::move (sl));
        };
        auto toggle = [&] (int col, const juce::String& t, const char* id)
        {
            auto tb = std::make_unique<juce::ToggleButton> (t);
            addAndMakeVisible (*tb);
            bAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, *tb));
            items.push_back ({ col, nullptr, tb.get() }); owned.push_back (std::move (tb));
        };
        combo (0, "Channel:", "channel"); combo (0, "Bands/Octave:", "bands"); combo (0, "Freq. Grid:", "grid");
        hdr (0, "Level"); slider (0, "Ceiling (dB):", "ceiling", 0); slider (0, "Grid (dB):", "gridDb", 0); slider (0, "Range (dB):", "range", 0);
        hdr (1, "Peak"); slider (1, "Decay (dB/s):", "peakDecay", 1); toggle (1, "On", "peakOn");
        hdr (1, "Peak Hold"); slider (1, "Time (s):", "holdTime", 1); slider (1, "Decay (dB/s):", "holdDecay", 1); toggle (1, "On", "holdOn"); toggle (1, "Infinite", "holdInf");
        hdr (1, "Average"); slider (1, "Time (s):", "avgTime", 1); slider (1, "Slope (dB/oct):", "avgSlope", 1); toggle (1, "On", "avgOn"); combo (1, "Display:", "style");

        close.setButtonText ("Close"); close.onClick = [this] { setVisible (false); };
        defaults.setButtonText ("Reset defaults");
        defaults.onClick = [this]
        {
            for (auto* p : apvts.processor.getParameters())
                if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                {
                    const juce::String id = r->paramID;
                    if (id == "channel" || id == "bands" || id == "grid" || id == "ceiling" || id == "gridDb" || id == "range" || id == "peakDecay" || id == "peakOn"
                        || id == "holdOn" || id == "holdTime" || id == "holdDecay" || id == "holdInf" || id == "avgOn" || id == "avgTime" || id == "avgSlope")
                    { r->beginChangeGesture(); r->setValueNotifyingHost (r->getDefaultValue()); r->endChangeGesture(); }
                }
        };
        addAndMakeVisible (close); addAndMakeVisible (defaults);
    }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xf4161920));
        g.setColour (kLine); g.drawRect (getLocalBounds(), 1);
        g.setColour (kText); setF (g, 15.f, true);
        g.drawText ("Spectrum Analyzer Settings", 14, 8, 300, 20, juce::Justification::centredLeft, false);
        g.setColour (kLine); g.drawVerticalLine (getWidth() / 2 - 6, 38.f, (float) getHeight() - 40.f);
    }
    void resized() override
    {
        int y[2] = { 40, 40 };
        for (auto& it : items)
        {
            const int x = it.col == 0 ? 14 : getWidth() / 2 + 6;
            int& yy = y[it.col];
            if (it.lab != nullptr && it.ctl == nullptr) { it.lab->setBounds (x, yy, 240, 20); yy += 24; }
            else if (it.lab != nullptr) { it.lab->setBounds (x, yy, 110, 20); it.ctl->setBounds (x + 112, yy, getWidth() / 2 - 140, 20); yy += 22; }
            else { it.ctl->setBounds (x + 112, yy, 120, 20); yy += 22; }
        }
        close.setBounds (getWidth() - 74, 8, 62, 22);
        defaults.setBounds (14, getHeight() - 32, 110, 22);
    }
private:
    struct Item { int col; juce::Label* lab; juce::Component* ctl; };
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<Item> items;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> cAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> bAtt;
    juce::TextButton close, defaults;
};

// ---------------------------------------------------------------- fondo y textos fijos
class Content : public juce::Component
{
public:
    std::function<void()> onReset;
    bool frameOn = true;
    void mouseDoubleClick (const juce::MouseEvent& e) override { if (onReset && juce::Rectangle<int> (566, 6, 188, 52).contains (e.getPosition())) onReset(); }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff12141a));
        auto box = [&] (int x, int y, int w, int h)
        {
            g.setColour (kPanel); g.fillRoundedRectangle ((float) x, (float) y, (float) w, (float) h, 6.f);
            g.setColour (kLine); g.drawRoundedRectangle ((float) x + 0.5f, (float) y + 0.5f, (float) w - 1.f, (float) h - 1.f, 6.f, 1.f);
        };
        box (6, 6, 554, 52); box (566, 6, 188, 52); box (6, 64, 418, 324); box (430, 64, 130, 324); box (6, 394, 554, 70); box (566, 64, 188, 400);
        g.setColour (kOrange); g.fillEllipse (21.f, 25.f, 14.f, 14.f);
        juce::Path ring; ring.addEllipse (-19.f, -5.f, 38.f, 10.f);
        g.strokePath (ring, juce::PathStrokeType (2.f), juce::AffineTransform::rotation (-0.35f).translated (28.f, 32.f));
        setF (g, 17.f, true);
        g.setColour (kOrange); g.drawText ("SPECTRUM", 52, 10, 160, 20, juce::Justification::centredLeft, false);
        g.setColour (juce::Colour (0xffd3d6dc)); g.drawText ("PLANETARY", 52, 30, 160, 20, juce::Justification::centredLeft, false);
        setF (g, 24.f, true); g.setColour (juce::Colour (0xffc9ccd2));
        g.drawText ("METERS", 566, 6, 188, 52, juce::Justification::centred, false);
        const float a = frameOn ? 1.f : 0.35f;
        setF (g, 12.f);
        g.setColour (kText.withAlpha (a));
        g.drawText ("FRAME ADJUST", 430, 70, 130, 14, juce::Justification::centred, false);
        g.setColour (kMuted.withAlpha (a)); g.drawText ("(" + pmStr() + " semitones)", 430, 112, 130, 12, juce::Justification::centred, false);
        g.setColour (kText.withAlpha (a));
        g.drawText ("ROOT NOTE:", 430, 152, 130, 14, juce::Justification::centred, false);
        g.drawText ("OCTAVE:", 430, 192, 130, 14, juce::Justification::centred, false);
    }
};
} // namespace ui
