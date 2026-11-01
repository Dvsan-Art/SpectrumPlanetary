#include "PluginEditor.h"
#include <BinaryData.h>

static std::vector<std::byte> toBytes (const char* d, int n)
{
    auto* p = reinterpret_cast<const std::byte*> (d);
    return std::vector<std::byte> (p, p + n);
}

SpectrumEditor::SpectrumEditor (SpectrumProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    using Opts = juce::WebBrowserComponent::Options;

    auto opts = Opts {}
        .withNativeIntegrationEnabled()
        .withResourceProvider ([this] (const juce::String& url) { return getResource (url); })
        .withEventListener ("cfg", [this] (juce::var v)
        {
            proc.fftIdx = (int) v.getProperty ("res", 2);
            proc.winIdx = (int) v.getProperty ("win", 0);
        })
        .withEventListener ("reset", [this] (juce::var) { proc.requestReset(); });

   #if JUCE_WINDOWS
    opts = opts.withBackend (Opts::Backend::webview2)
               .withWinWebView2Options (Opts::WinWebView2 {}
                    .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)));
   #endif

    web = std::make_unique<juce::WebBrowserComponent> (opts);
    addAndMakeVisible (*web);
    web->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, true);
    setResizeLimits (900, 600, 2400, 1500);
    setSize (1180, 740);
    startTimerHz (30);
}

SpectrumEditor::~SpectrumEditor() { stopTimer(); }

void SpectrumEditor::resized() { if (web != nullptr) web->setBounds (getLocalBounds()); }

void SpectrumEditor::timerCallback()
{
    if (web != nullptr)
        web->emitEventIfBrowserIsVisible ("frame", proc.buildFrame());
}

std::optional<juce::WebBrowserComponent::Resource> SpectrumEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return juce::WebBrowserComponent::Resource { toBytes (BinaryData::index_html, BinaryData::index_htmlSize),
                                                     juce::String ("text/html") };
    return std::nullopt;
}
