#include "PluginEditor.h"
#include "Diagnostics.h"
#include "BinaryData.h"

namespace s87 {

static constexpr int kBaseW = 1500, kBaseH = 740;

juce::String SupurrEditor::safeName (const juce::String& s)
{
    return s.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-. ").replace ("..", "_").trim();
}

// db paths look like "presets/my_patch"; collections map to folders, documents to .json files
juce::File SupurrEditor::dbFile (const juce::String& path)
{
    auto parts = juce::StringArray::fromTokens (path, "/", "");
    parts.removeEmptyStrings();
    if (parts.size() != 2) return {};
    const auto col = safeName (parts[0]), id = safeName (parts[1]);
    if (col.isEmpty() || id.isEmpty()) return {};
    return Diagnostics::folder (col).getChildFile (id + ".json");
}

std::optional<juce::WebBrowserComponent::Resource> SupurrEditor::serve (const juce::String& path)
{
    if (path == "/" || path == "/index.html" || path.isEmpty())
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (reinterpret_cast<const std::byte*> (BinaryData::index_html),
                                    reinterpret_cast<const std::byte*> (BinaryData::index_html) + BinaryData::index_htmlSize),
            "text/html; charset=utf-8" };
    return std::nullopt;
}

SupurrEditor::SupurrEditor (SupurrProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    using Opt = juce::WebBrowserComponent::Options;
    using Completion = juce::WebBrowserComponent::NativeFunctionCompletion;

    auto init = proc.getInitState();
    auto opts = Opt()
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withInitialisationData ("sampleRate", proc.currentSampleRate())
        .withInitialisationData ("state", juce::JSON::toString (init, true))
        .withInitialisationData ("version", juce::String (JucePlugin_VersionString))
        .withInitialisationData ("dataDir", Diagnostics::root().getFullPathName())
        .withResourceProvider ([this] (const juce::String& path) { return serve (path); })
        .withEventListener ("msg", [this] (juce::var m) { proc.handleUiMessage (m); })
        .withEventListener ("vol", [this] (juce::var m) { proc.setOutputVolume ((float) (double) m["v"]); })
        .withEventListener ("mute", [this] (juce::var m) { proc.setMuted ((bool) m["on"]); Diagnostics::log (((bool) m["on"]) ? "audio stopped from the page" : "audio started from the page"); })
        .withEventListener ("uistate", [this] (juce::var m) { proc.setUiState (m); })
        .withNativeFunction ("dbSet", [] (const juce::Array<juce::var>& a, Completion done)
        {
            bool ok = false;
            if (a.size() >= 2) { auto f = dbFile (a[0].toString()); if (f != juce::File()) ok = f.replaceWithText (a[1].toString()); }
            done (ok);
        })
        .withNativeFunction ("dbDelete", [] (const juce::Array<juce::var>& a, Completion done)
        {
            bool ok = false;
            if (a.size() >= 1) { auto f = dbFile (a[0].toString()); if (f != juce::File()) ok = f.deleteFile(); }
            done (ok);
        })
        .withNativeFunction ("dbList", [] (const juce::Array<juce::var>& a, Completion done)
        {
            juce::Array<juce::var> out;
            if (a.size() >= 1)
            {
                auto col = safeName (a[0].toString());
                if (col.isNotEmpty())
                    for (auto& f : Diagnostics::folder (col).findChildFiles (juce::File::findFiles, false, "*.json"))
                    {
                        auto* o = new juce::DynamicObject();
                        o->setProperty ("id", f.getFileNameWithoutExtension());
                        o->setProperty ("json", f.loadFileAsString());
                        out.add (juce::var (o));
                    }
            }
            done (juce::var (out));
        })
        .withNativeFunction ("saveFile", [] (const juce::Array<juce::var>& a, Completion done)
        {
            // args: filename, base64 data
            juce::String result;
            if (a.size() >= 2)
            {
                auto name = safeName (a[0].toString());
                if (name.isEmpty()) name = "export";
                auto f = Diagnostics::folder ("exports").getNonexistentChildFile (name.upToLastOccurrenceOf (".", false, false),
                                                                                   name.containsChar ('.') ? "." + name.fromLastOccurrenceOf (".", false, false) : "", false);
                juce::MemoryOutputStream out;
                if (juce::Base64::convertFromBase64 (out, a[1].toString()) && f.replaceWithData (out.getData(), out.getDataSize()))
                {
                    result = f.getFullPathName();
                    Diagnostics::log ("saved " + result);
                }
            }
            done (result);
        })
        .withNativeFunction ("openFolder", [] (const juce::Array<juce::var>& a, Completion done)
        {
            auto which = a.size() ? a[0].toString() : juce::String();
            juce::File f = which == "log" ? Diagnostics::folder ("logs") : Diagnostics::folder (safeName (which).isEmpty() ? "exports" : safeName (which));
            f.revealToUser(); done (true);
        })
        .withNativeFunction ("copyDiagnostics", [] (const juce::Array<juce::var>&, Completion done)
        {
            juce::SystemClipboard::copyTextToClipboard (Diagnostics::report());
            done (true);
        })
        .withNativeFunction ("log", [] (const juce::Array<juce::var>& a, Completion done)
        {
            if (a.size() >= 1) Diagnostics::log ("page: " + a[0].toString().substring (0, 600));
            done (true);
        });

#if JUCE_WINDOWS
    opts = opts.withBackend (Opt::Backend::webview2)
               .withWinWebView2Options (Opt::WinWebView2()
                                            .withUserDataFolder (Diagnostics::folder ("webview"))
                                            .withStatusBarDisabled());
#endif

    web = std::make_unique<juce::WebBrowserComponent> (opts);
    addAndMakeVisible (*web);
    web->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, false);
    setResizeLimits (kBaseW / 3, kBaseH / 3, kBaseW * 2, kBaseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kBaseW / kBaseH);
    setSize (juce::jmin (kBaseW, 1180), (int) (juce::jmin (kBaseW, 1180) * (double) kBaseH / kBaseW));
    edges = std::make_unique<juce::ResizableBorderComponent> (this, getConstrainer());
    edges->setBorderThickness (juce::BorderSize<int> (6));
    addAndMakeVisible (*edges);
    tripsSeen = proc.takeMeters().trips;
    startTimerHz (30);
}

SupurrEditor::~SupurrEditor() { stopTimer(); }

void SupurrEditor::resized()
{
    if (web) web->setBounds (getLocalBounds());
    if (edges) { edges->setBounds (getLocalBounds()); edges->toFront (false); }
}

void SupurrEditor::timerCallback()
{
    if (! web) return;
    const auto m = proc.takeMeters();
    juce::Array<juce::var> arr;
    arr.add (m.l); arr.add (m.r); arr.add (m.active); arr.add (m.lim);
    for (int i = 0; i < 3; ++i) arr.add (m.gr[i]);
    for (int i = 0; i < 3; ++i) arr.add (m.dm[i]);
    arr.add (m.trips);
    web->emitEventIfBrowserIsVisible ("meters", juce::var (arr));
    if (m.trips > tripsSeen) { tripsSeen = m.trips; Diagnostics::log ("safety guard reset the engine (count " + juce::String (m.trips) + ")"); }

    // scope: new mono samples as Int16, base64
    static float buf[8192];
    const int n = proc.readScope (buf, 8192);
    if (n > 0 && web->isShowing())
    {
        juce::Array<juce::int16> s; s.resize (n);
        for (int i = 0; i < n; ++i) s.set (i, (juce::int16) juce::jlimit (-32767, 32767, (int) std::lround (buf[i] * 32767.f)));
        web->emitEventIfBrowserIsVisible ("scope", juce::Base64::toBase64 (s.getRawDataPointer(), (size_t) n * 2));
    }
}

} // namespace s87
