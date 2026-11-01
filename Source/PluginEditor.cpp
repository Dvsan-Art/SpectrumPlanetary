#include "PluginEditor.h"

SpectrumEditor::SpectrumEditor (SpectrumProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setSize (950, 600);

    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff262a32));
    analyzeButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffff9a2e));
    analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(analyzeButton);

    inputKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    inputKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(inputKnob);

    outputKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(outputKnob);

    startTimerHz (30);
}

SpectrumEditor::~SpectrumEditor() { stopTimer(); }

void SpectrumEditor::timerCallback()
{
    repaint(); // Refresca el espectro y medidores en tiempo real
}

void SpectrumEditor::paint (juce::Graphics& g)
{
    // Fondo general oscuro estilo DAW
    g.fillAll (juce::Colour (0xff0a0b0d));

    // Panel superior (Título y controles principales)
    g.setColour (juce::Colour (0xff1a1d23));
    g.fillRect (10, 10, 930, 65);
    g.setColour (juce::Colour (0xff2d323b));
    g.drawRect (10, 10, 930, 65, 1);

    // Título "SPECTRUM PLANETARY"
    g.setColour (juce::Colour (0xffff9a2e));
    g.setFont (juce::Font("Roboto", 24.0f, juce::Font::bold));
    g.drawText ("SPECTRUM PLANETARY", 25, 25, 300, 35, juce::Justification::left);

    // Caja principal del Espectro
    g.setColour (juce::Colour (0xff14161a));
    g.fillRect (10, 85, 620, 360);
    g.setColour (juce::Colour (0xff2d323b));
    g.drawRect (10, 85, 620, 360, 1);

    // Dibujar líneas de la retícula del espectro
    g.setColour (juce::Colour (0xff20252d));
    for (int y = 85; y < 445; y += 45)
        g.drawHorizontalLine(y, 10.0f, 630.0f);

    // Dibujar la curva del espectro en tiempo real
    auto spec = proc.getSpectrumData();
    juce::Path spectrumPath;
    float w = 610.0f;
    float h = 340.0f;
    float dx = w / (float)spec.size();

    for (size_t i = 0; i < spec.size(); ++i)
    {
        float x = 20.0f + i * dx;
        float normY = juce::jmap(spec[i], -100.0f, 12.0f, 435.0f, 95.0f);
        if (i == 0) spectrumPath.startNewSubPath(x, normY);
        else spectrumPath.lineTo(x, normY);
    }

    g.setColour (juce::Colour (0xffff9a2e));
    g.strokePath(spectrumPath, juce::PathStrokeType(2.0f));

    // Panel de Medidores (Derecha)
    g.setColour (juce::Colour (0xff1a1d23));
    g.fillRect (640, 85, 300, 360);
    g.setColour (juce::Colour (0xff2d323b));
    g.drawRect (640, 85, 300, 360, 1);

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawText ("METERS (PEAK / RMS)", 650, 100, 280, 25, juce::Justification::center);

    // Barras de medidores nativas
    float pkL = juce::jmap(proc.peakL.load(), -48.0f, 0.0f, 0.0f, 200.0f);
    float pkR = juce::jmap(proc.peakR.load(), -48.0f, 0.0f, 0.0f, 200.0f);

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(700, 150, 25, 200);
    g.fillRect(750, 150, 25, 200);

    g.setColour(juce::Colour(0xff46d066));
    g.fillRect(700, 350 - juce::jlimit(0.0f, 200.0f, pkL), 25, juce::jlimit(0.0f, 200.0f, pkL));
    g.fillRect(750, 350 - juce::jlimit(0.0f, 200.0f, pkR), 25, juce::jlimit(0.0f, 200.0f, pkR));
}

void SpectrumEditor::resized()
{
    analyzeButton.setBounds(500, 25, 100, 35);
    inputKnob.setBounds(350, 20, 45, 45);
    outputKnob.setBounds(420, 20, 45, 45);
}
