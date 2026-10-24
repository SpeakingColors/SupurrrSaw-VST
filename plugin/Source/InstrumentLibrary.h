// Studio 87 SupurrSaw: finds and loads the optional instrument pack (multisampled piano, FM piano, trumpet, violin, strings).
// A pack is a folder with instruments.json and the FLAC files it lists; tools/make_instruments.py builds one from CC0 sample libraries.
// The standard pack is compiled into the plugin (assets/instruments.bin, S87InstData). A folder with instruments.json, in any of the places
// returned by searchFolders(), adds more instruments or replaces one of the same name.
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>
#include <memory>
#include <cstring>
#include <vector>
#include "Diagnostics.h"
#include "../../core/supurrsaw_core.h"
#include "S87InstData.h"

namespace s87 {

class InstrumentLibrary
{
public:
    // Places searched for instruments.json (the folder itself, or one folder level below it).
    static juce::Array<juce::File> searchFolders()
    {
        juce::Array<juce::File> f;
        f.add (Diagnostics::root().getChildFile ("Instruments"));
       #if JUCE_WINDOWS
        f.add (juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory).getChildFile ("Studio 87").getChildFile ("SupurrSaw").getChildFile ("Instruments"));
       #elif JUCE_MAC
        f.add (juce::File ("/Library/Application Support/Studio 87/SupurrSaw/Instruments"));
       #else
        f.add (juce::File ("/usr/local/share/studio87/supurrsaw/Instruments"));
        f.add (juce::File ("/usr/share/studio87/supurrsaw/Instruments"));
       #endif
        return f;
    }

    // Reads every instruments.json it can find. Safe to call again later (a pack added while the plugin runs).
    void refresh()
    {
        const juce::ScopedLock sl (lock);
        infos.clear();
        for (auto& root : searchFolders())
        {
            if (! root.isDirectory()) continue;
            scanOne (root);
            for (auto& sub : root.findChildFiles (juce::File::findDirectories, false)) scanOne (sub);
        }
        scanEmbedded();   // last, so a folder pack with the same instrument name wins
    }

    juce::StringArray names() const
    {
        const juce::ScopedLock sl (lock);
        juce::StringArray n;
        for (auto& kv : infos) n.add (kv.first);
        return n;
    }

    // Decodes an instrument (once; the result is cached and kept for the life of the plugin, because playing voices point into it).
    std::shared_ptr<const supurr::SampleSet> load (const juce::String& name)
    {
        const juce::ScopedLock sl (lock);
        if (name.isEmpty()) return nullptr;
        if (auto c = cache.find (name); c != cache.end()) return c->second;
        auto it = infos.find (name);
        if (it == infos.end()) { refresh(); it = infos.find (name); }
        if (it == infos.end()) { Diagnostics::log ("instrument not found: " + name); return nullptr; }

        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        auto set = std::make_shared<supurr::SampleSet>();
        set->name = name.toStdString();
        const auto& info = it->second;
        const bool loop = (bool) info.json["loop"];
        if (auto* zones = info.json["zones"].getArray())
            for (auto& z : *zones)
            {
                std::unique_ptr<juce::AudioFormatReader> rd;
                juce::String what;
                if (info.embedded)
                {
                    const int zi = (int) (&z - zones->begin());
                    if (zi < info.blobs.size())
                        rd.reset (fm.createReaderFor (std::make_unique<juce::MemoryInputStream> (info.blobs[zi].first, (size_t) info.blobs[zi].second, false)));
                    what = "built-in " + z["file"].toString();
                }
                else
                {
                    const auto file = info.dir.getChildFile (z["file"].toString());
                    rd.reset (fm.createReaderFor (file));
                    what = file.getFullPathName();
                }
                if (rd == nullptr || rd->lengthInSamples < 16) { Diagnostics::log ("cannot read " + what); continue; }
                const int n = (int) juce::jmin<juce::int64> (rd->lengthInSamples, 1 << 24);
                juce::AudioBuffer<float> buf ((int) juce::jmax<unsigned> (1u, rd->numChannels), n);
                rd->read (&buf, 0, n, 0, true, true);
                auto s = std::make_shared<supurr::Sample>();
                s->sampleRate = rd->sampleRate;
                s->rootNote = juce::jlimit (0, 127, (int) z["root"]);
                s->tune = (double) z["tune"];
                s->loop = loop;
                s->loopStart = (size_t) juce::jmax (0, (int) z["loopStart"]);
                s->loopEnd = (size_t) juce::jmax (0, (int) z["loopEnd"]);
                if (s->loop && (s->loopEnd <= s->loopStart || s->loopEnd + 4 >= (size_t) n)) s->loop = false;   // a broken loop plays once instead of glitching
                s->l.assign (buf.getReadPointer (0), buf.getReadPointer (0) + n);
                if (buf.getNumChannels() > 1) s->r.assign (buf.getReadPointer (1), buf.getReadPointer (1) + n);
                set->zones.push_back ({ s, juce::jlimit (0, 127, (int) z["lo"]), juce::jlimit (0, 127, (int) z["hi"]) });
            }
        if (set->zones.empty()) { Diagnostics::log ("instrument has no usable zones: " + name); return nullptr; }
        Diagnostics::log ("loaded instrument " + name + ", " + juce::String ((int) set->zones.size()) + " zones");
        cache[name] = set;
        return set;
    }

private:
    struct Info { juce::File dir; juce::var json; bool embedded = false; std::vector<std::pair<const char*, int>> blobs; };

    void scanOne (const juce::File& dir)
    {
        auto f = dir.getChildFile ("instruments.json");
        if (! f.existsAsFile()) return;
        const auto j = juce::JSON::parse (f.loadFileAsString());
        if (auto* arr = j["instruments"].getArray())
            for (auto& inst : *arr)
            {
                const auto nm = inst["name"].toString();
                if (nm.isNotEmpty() && infos.find (nm) == infos.end()) infos[nm] = { dir, inst };
            }
    }

    // the pack inside the plugin: "S87I", u32 version, u32 jsonLen, json, then u32 length + FLAC bytes for every zone in json order
    void scanEmbedded()
    {
        const char* d = S87InstData::instruments_bin;
        const size_t n = (size_t) S87InstData::instruments_binSize;
        auto u32 = [&] (size_t p) { return (uint32_t) (uint8_t) d[p] | ((uint32_t) (uint8_t) d[p + 1] << 8) | ((uint32_t) (uint8_t) d[p + 2] << 16) | ((uint32_t) (uint8_t) d[p + 3] << 24); };
        if (n < 12 || std::memcmp (d, "S87I", 4) != 0) return;
        const size_t jl = u32 (8);
        if (12 + jl > n) return;
        const auto j = juce::JSON::parse (juce::String::fromUTF8 (d + 12, (int) jl));
        size_t pos = 12 + jl;
        if (auto* arr = j["instruments"].getArray())
            for (auto& inst : *arr)
            {
                Info info; info.json = inst; info.embedded = true;
                bool ok = true;
                if (auto* zs = inst["zones"].getArray())
                    for (size_t i = 0; i < (size_t) zs->size(); ++i)
                    {
                        if (pos + 4 > n) { ok = false; break; }
                        const size_t len = u32 (pos); pos += 4;
                        if (pos + len > n) { ok = false; break; }
                        info.blobs.push_back ({ d + pos, (int) len }); pos += len;
                    }
                const auto nm = inst["name"].toString();
                if (ok && nm.isNotEmpty() && infos.find (nm) == infos.end()) infos[nm] = std::move (info);
            }
    }

    mutable juce::CriticalSection lock;
    std::map<juce::String, Info> infos;
    std::map<juce::String, std::shared_ptr<const supurr::SampleSet>> cache;
};

} // namespace s87
