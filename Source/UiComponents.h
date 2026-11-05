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
            g.setColour (kMuted); g.drawText (juce::String ((int) v), 0, y - 6, (int) L - 12, 12, juce::Justification::centredRight, false);
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

        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<int> ((int) L, (int) T, (int) Wd + 1, (int) Ht + 1));
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
        }

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

// ---------------------------------------------------------------- teclado: 9 octavas (C1-B9)
class KeyboardView : public juce::Component
{
public:
    explicit KeyboardView (juce::AudioProcessorValueTreeState& a) : apvts (a) {}
    std::function<void (int)> onPick;
    static constexpr int first = 24, last = 131, whiteCount = 63;

    static bool isBlack (int m) { const int n = m % 12; return n == 1 || n == 3 || n == 6 || n == 8 || n == 10; }

    void paint (juce::Graphics& g) override
    {
        const float W = (float) getWidth(), H = (float) getHeight(), kw = W / (float) whiteCount, bw = kw * 0.62f;
        const bool on = pv (apvts, "frameOn") > 0.5f;
        const int root = currentMidi (apvts);
        const double w = kWidths[juce::jlimit (0, 5, (int) pv (apvts, "width"))];
        g.fillAll (juce::Colour (0xff0e1013));

        for (int m = first; m <= last; ++m)
        {
            if (isBlack (m)) continue;
            const float x = (float) whitesBefore (m) * kw;
            const bool hl = on && std::abs ((double) (m - root)) <= w;
            juce::Colour c (0xffe8e8e8);
            if (on && m == root) c = kOrange;
            else if (hl) c = juce::Colour (0xffffc98a);
            g.setColour (c); g.fillRect (x, 0.f, kw, H);
            g.setColour (juce::Colour (0xff222222)); g.drawRect (x, 0.f, kw, H, 1.f);
            if (m % 12 == 0)
            {
                g.setColour (juce::Colour (0xff444444)); setF (g, 8.5f);
                g.drawText ("C" + juce::String (m / 12 - 1), (int) x - 6, (int) H - 14, (int) kw + 12, 12, juce::Justification::centred, false);
            }
        }
        for (int m = first; m <= last; ++m)
        {
            if (! isBlack (m)) continue;
            const float cx = (float) whitesBefore (m) * kw;
            const bool hl = on && std::abs ((double) (m - root)) <= w;
            juce::Colour c (0xff111111);
            if (on && m == root) c = juce::Colour (0xffd97a14);
            else if (hl) c = juce::Colour (0xff9a5a1a);
            g.setColour (c); g.fillRect (cx - bw * 0.5f, 0.f, bw, H * 0.62f);
        }
        if (! on) { g.setColour (juce::Colour (0xff0e1013).withAlpha (0.65f)); g.fillAll(); }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const float kw = (float) getWidth() / (float) whiteCount, bw = kw * 0.62f;
        for (int m = first; m <= last; ++m)
            if (isBlack (m) && juce::Rectangle<float> ((float) whitesBefore (m) * kw - bw * 0.5f, 0.f, bw, (float) getHeight() * 0.62f).contains (e.position))
            { if (onPick) onPick (m); return; }
        for (int m = first; m <= last; ++m)
            if (! isBlack (m) && juce::Rectangle<float> ((float) whitesBefore (m) * kw, 0.f, kw, (float) getHeight()).contains (e.position))
            { if (onPick) onPick (m); return; }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    int whitesBefore (int m) const { int c = 0; for (int i = first; i < m; ++i) if (! isBlack (i)) ++c; return c; }
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
