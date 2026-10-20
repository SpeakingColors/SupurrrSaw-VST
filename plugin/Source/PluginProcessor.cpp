#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParamBridge.h"
#include "Diagnostics.h"
#include "../../presets/presets.h"

namespace s87 {

SupurrProcessor::SupurrProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    for (int i = 0; i < 8; ++i) mWet[i].store ((i & 1) ? -1e9f : 1e9f);
    for (auto& a : mGr) a.store (0.f);
    for (auto& a : mDm) a.store (0.f);
    supurr::presets::byName ("trance_90s", pending);   // same patch the page starts with, so MIDI works before the editor opens
    engine.p = pending;
    Diagnostics::log ("plugin created, version " + juce::String (JucePlugin_VersionString));
}

SupurrProcessor::~SupurrProcessor() { Diagnostics::log ("plugin closed"); }

bool SupurrProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SupurrProcessor::prepareToPlay (double sr, int block)
{
    sampleRateNow.store (sr);
    maxBlockNow.store (juce::jmax (block, 8192));
    engine.prepare (sr);
    engine.reserve (juce::jmax (block, 8192));
    {
        const juce::SpinLock::ScopedLockType sl (paramLock);
        engine.p = pending;
        dirty.store (false);
    }
    if (! kitDefs.empty()) rebuildKit();   // a kit from the project, or from before a sample rate change
    Diagnostics::log ("audio start: " + juce::String (sr, 0) + " Hz, block " + juce::String (block));
}

void SupurrProcessor::applyPendingParams()
{
    if (! dirty.load()) return;
    const juce::SpinLock::ScopedTryLockType sl (paramLock);
    if (! sl.isLocked()) return;
    engine.p = pending;
    if (wheelSeen) engine.p.modWheel = wheelCC;
    dirty.store (false);
}

void SupurrProcessor::pushEvent (int type, int note, float vel)
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) { fifoData[(size_t) s1] = { type, note, vel }; fifo.finishedWrite (1); }
}

void SupurrProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    const bool isMuted = muted.load();
    if (isMuted && ! wasMuted) { engine.allOff(); if (kitLive) kitLive->allOff(); }
    wasMuted = isMuted;

    applyPendingParams();
    if (kitDirty.load())
    {
        const juce::SpinLock::ScopedTryLockType kl (kitLock);
        if (kl.isLocked()) { kitLive = kitNext; kitDirty.store (false); engine.allOff(); }
    }
    auto noteOnAny = [&] (int note, float vel) { if (kitLive) kitLive->noteOn (note, vel); else engine.noteOn (note, vel); };
    auto noteOffAny = [&] (int note) { if (! kitLive) engine.noteOff (note); };
    auto allOffAny = [&] { engine.allOff(); if (kitLive) kitLive->allOff(); };

    // notes from the on screen keyboard and the demo player
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        auto run = [&] (int start, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                const Ev& e = fifoData[(size_t) (start + i)];
                if (isMuted) continue;
                if (e.type == 0) noteOnAny (e.note, e.vel);
                else if (e.type == 1) noteOffAny (e.note);
                else allOffAny();
            }
        };
        run (s1, n1); run (s2, n2);
        fifo.finishedRead (n1 + n2);
    }

    // host MIDI, rendered in slices so notes land on the right sample
    int pos = 0;
    auto renderTo = [&] (int end)
    {
        if (end > pos && ! isMuted)
        {
            if (kitLive) kitLive->render (L + pos, R + pos, end - pos, engine.p.master, engine.p.clipKnee);
            else engine.render (L + pos, R + pos, end - pos);
        }
        pos = juce::jmax (pos, end);
    };
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        renderTo (juce::jlimit (0, n, meta.samplePosition));
        if (isMuted) continue;
        if (msg.isNoteOn()) noteOnAny (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff()) noteOffAny (msg.getNoteNumber());
        else if (msg.isAllSoundOff()) allOffAny();
        else if (msg.isAllNotesOff()) engine.releaseAll();   // hosts send this when a clip changes or the transport stops: let the notes release, no cut
        else if (msg.isController() && msg.getControllerNumber() == 1)
        {
            wheelCC = (float) msg.getControllerValue() / 127.f;
            wheelSeen = true;
            engine.p.modWheel = wheelCC;
        }
    }
    renderTo (n);

    // output volume (smoothed), peaks and scope
    const float target = volume.load();
    const float step = (target - volNow) / (float) juce::jmax (1, n);
    float pl = 0.f, pr = 0.f;
    auto wp = scopeWp.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        volNow += step;
        L[i] *= volNow; R[i] *= volNow;
        pl = juce::jmax (pl, std::abs (L[i])); pr = juce::jmax (pr, std::abs (R[i]));
        scope[(size_t) (wp++ & (kScope - 1))] = 0.5f * (L[i] + R[i]);
    }
    volNow = target;
    scopeWp.store (wp, std::memory_order_release);

    auto bump = [] (std::atomic<float>& a, float v) { float o = a.load(); while (v > o && ! a.compare_exchange_weak (o, v)) {} };
    bump (mPeakL, pl); bump (mPeakR, pr);
    const auto m = engine.meters();
    mLim.store (m.lim); mActive.store (kitLive ? kitLive->activePads() : m.active); mTrips.store (m.trips);
    for (int i = 0; i < 3; ++i) { mGr[i].store (m.gr[i]); mDm[i].store (m.dm[i]); }
    {   // wet signal range of delay and reverb since the last editor read
        float w[8]; engine.wetTake (w);
        for (int i = 0; i < 8; ++i) { auto& a = mWet[i]; float o = a.load(); if (i & 1) { while (w[i] > o && ! a.compare_exchange_weak (o, w[i])) {} } else { while (w[i] < o && ! a.compare_exchange_weak (o, w[i])) {} } }
    }
}

SupurrProcessor::MeterSnap SupurrProcessor::takeMeters()
{
    MeterSnap s;
    s.l = mPeakL.exchange (0.f); s.r = mPeakR.exchange (0.f);
    s.lim = mLim.load(); s.active = mActive.load(); s.trips = mTrips.load();
    for (int i = 0; i < 3; ++i) { s.gr[i] = mGr[i].load(); s.dm[i] = mDm[i].load(); }
    for (int i = 0; i < 8; ++i) { const float v = mWet[i].exchange ((i & 1) ? -1e9f : 1e9f); s.wet[i] = std::abs (v) > 1e8f ? 0.f : v; }
    return s;
}

int SupurrProcessor::readScope (float* dest, int maxCount)
{
    const auto wp = scopeWp.load (std::memory_order_acquire);
    if (wp - scopeRp > (juce::uint64) kScope - 4096) scopeRp = wp - (juce::uint64) maxCount;   // fell behind: skip ahead
    if (wp < scopeRp) scopeRp = wp;
    int n = (int) juce::jmin<juce::uint64> (wp - scopeRp, (juce::uint64) maxCount);
    for (int i = 0; i < n; ++i) dest[i] = scope[(size_t) ((scopeRp + (juce::uint64) i) & (kScope - 1))];
    scopeRp += (juce::uint64) n;
    return n;
}

// ---------------------------------------------------------------- page messages

void SupurrProcessor::buildTable (int layer, const float* data, int len)
{
    std::shared_ptr<const supurr::WaveTable> t;
    if (data != nullptr && len >= 8)
    {
        auto w = std::make_shared<supurr::WaveTable>();
        w->setCycle (data, len);
        if (w->ready()) t = w;
    }
    const juce::SpinLock::ScopedLockType sl (paramLock);
    if (tables[layer]) { retired.push_back (tables[layer]); if (retired.size() > 24) retired.erase (retired.begin()); }
    tables[layer] = t;
    pending.layer[layer].table = t;
    dirty.store (true);
}

static std::vector<int> intList (const juce::var& v)
{
    std::vector<int> r;
    if (auto* a = v.getArray()) for (auto& x : *a) r.push_back ((int) x);
    return r;
}

void SupurrProcessor::setKitFromJson (const juce::var& k)
{
    kitDefs.clear();
    if (auto* pads = k.getArray())
        for (auto& s : *pads)
        {
            supurr::KitEngine::Def d;
            d.n = intList (s["n"]); d.ch = intList (s["ch"]);
            d.play = (s["play"].isVoid() || s["play"].isUndefined()) ? -1 : (int) s["play"];
            d.g = s.hasProperty ("g") ? (float) (double) s["g"] : 1.f;
            setAll (d.p, s["full"]);
            if (auto* cy = s["cyc"].getArray())
                for (int i = 0; i < juce::jmin (3, cy->size()); ++i)
                    if (auto* arr = (*cy)[i].getArray())
                    {
                        std::vector<float> f; f.reserve ((size_t) arr->size());
                        for (auto& x : *arr) f.push_back ((float) (double) x);
                        if (f.size() >= 8)
                        {
                            auto w = std::make_shared<supurr::WaveTable>();
                            w->setCycle (f.data(), (int) f.size());
                            if (w->ready()) d.p.layer[i].table = w;
                        }
                    }
            if (! d.n.empty()) kitDefs.push_back (std::move (d));
        }
    rebuildKit();
}

void SupurrProcessor::rebuildKit()
{
    std::shared_ptr<supurr::KitEngine> k;
    if (! kitDefs.empty())
    {
        k = std::make_shared<supurr::KitEngine>();
        k->build (kitDefs, sampleRateNow.load(), maxBlockNow.load());
    }
    std::vector<std::shared_ptr<supurr::KitEngine>> drop;
    {
        const juce::SpinLock::ScopedLockType sl (kitLock);
        if (kitNext) kitRetired.push_back (kitNext);
        while (kitRetired.size() > 4) { drop.push_back (kitRetired.front()); kitRetired.erase (kitRetired.begin()); }
        kitNext = k;
        kitDirty.store (true);
    }
}

static void setInJson (juce::var& root, const juce::String& key, const juce::var& v)
{
    if (! root.isObject()) root = juce::var (new juce::DynamicObject());
    auto* o = root.getDynamicObject();
    if (key.length() > 3 && key[0] == 'L' && key[2] == '_' && key[1] >= '0' && key[1] <= '2')
    {
        auto layers = o->getProperty ("layers");
        if (! layers.isArray()) { layers = juce::var (juce::Array<juce::var>()); o->setProperty ("layers", layers); }
        auto* arr = layers.getArray();
        while (arr->size() < 3) arr->add (juce::var (new juce::DynamicObject()));
        if (auto* lo = (*arr)[key[1] - '0'].getDynamicObject()) lo->setProperty (key.substring (3), v);
    }
    else o->setProperty (key, v);
}

void SupurrProcessor::handleUiMessage (const juce::var& m)
{
    const auto t = m.getProperty ("t", {}).toString();
    if (t == "on")       pushEvent (0, (int) m["n"], (float) (double) m["v"]);
    else if (t == "off") pushEvent (1, (int) m["n"], 0.f);
    else if (t == "all") pushEvent (2, 0, 0.f);
    else if (t == "p")
    {
        const auto k = m["k"].toString();
        const juce::ScopedLock sl (stateLock);
        setInJson (paramsJson, k, m["v"]);
        const juce::SpinLock::ScopedLockType pl (paramLock);
        if (setKey (pending, k, m["v"])) dirty.store (true);
    }
    else if (t == "set")
    {
        const juce::ScopedLock sl (stateLock);
        paramsJson = m["p"];
        supurr::Params np;
        setAll (np, paramsJson);
        const juce::SpinLock::ScopedLockType pl (paramLock);
        for (int i = 0; i < 3; ++i) np.layer[i].table = tables[i];
        pending = np;
        dirty.store (true);
    }
    else if (t == "kit")
    {
        {
            const juce::ScopedLock sl (stateLock);
            kitJson = m["k"];
        }
        setKitFromJson (m["k"]);
    }
    else if (t == "cyc")
    {
        const int ly = juce::jlimit (0, 2, (int) m["ly"]);
        std::vector<float> d;
        if (auto* a = m["d"].getArray()) { d.reserve ((size_t) a->size()); for (auto& x : *a) d.push_back ((float) (double) x); }
        buildTable (ly, d.empty() ? nullptr : d.data(), (int) d.size());
    }
}

void SupurrProcessor::setUiState (const juce::var& ui)
{
    const juce::ScopedLock sl (stateLock);
    uiJson = ui;
}

juce::var SupurrProcessor::getInitState()
{
    const juce::ScopedLock sl (stateLock);
    auto* o = new juce::DynamicObject();
    o->setProperty ("params", paramsJson);
    o->setProperty ("ui", uiJson);
    o->setProperty ("vol", (double) volume.load());
    return juce::var (o);
}

// ---------------------------------------------------------------- project state

void SupurrProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto* o = new juce::DynamicObject();
    {
        const juce::ScopedLock sl (stateLock);
        o->setProperty ("version", 1);
        o->setProperty ("params", paramsJson);
        o->setProperty ("ui", uiJson);
        o->setProperty ("vol", (double) volume.load());
        if (kitJson.isArray()) o->setProperty ("kit", kitJson);
    }
    const auto text = juce::JSON::toString (juce::var (o), true);
    dest.append (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

static std::vector<float> decodeCycle (const juce::String& b64)
{
    juce::MemoryOutputStream out;
    juce::Base64::convertFromBase64 (out, b64);
    std::vector<float> f (out.getDataSize() / 2);
    auto* src = static_cast<const juce::int16*> (out.getData());
    for (size_t i = 0; i < f.size(); ++i) f[i] = (float) src[i] / 32767.f;
    return f;
}

void SupurrProcessor::setStateInformation (const void* data, int size)
{
    const auto v = juce::JSON::parse (juce::String::fromUTF8 (static_cast<const char*> (data), size));
    if (! v.isObject()) return;
    {
        const juce::ScopedLock sl (stateLock);
        paramsJson = v["params"];
        uiJson = v["ui"];
    }
    if (v.hasProperty ("vol")) volume.store ((float) (double) v["vol"]);
    {
        const juce::ScopedLock sl (stateLock);
        kitJson = v["kit"];
    }
    setKitFromJson (kitJson);
    if (paramsJson.isObject())
    {
        supurr::Params np;
        setAll (np, paramsJson);
        if (auto* layers = paramsJson["layers"].getArray())
            for (int i = 0; i < juce::jmin (3, layers->size()); ++i)
            {
                const auto cyc = (*layers)[i]["cycle"].toString();
                if (cyc.isNotEmpty() && np.layer[i].wave == supurr::Wave::Custom)
                {
                    auto f = decodeCycle (cyc);
                    buildTable (i, f.data(), (int) f.size());
                }
                np.layer[i].table = tables[i];
            }
        const juce::SpinLock::ScopedLockType pl (paramLock);
        pending = np;
        dirty.store (true);
    }
    Diagnostics::log ("project state loaded");
}

juce::AudioProcessorEditor* SupurrProcessor::createEditor() { return new SupurrEditor (*this); }

} // namespace s87

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new s87::SupurrProcessor(); }
