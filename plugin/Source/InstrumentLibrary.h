// Studio 87 SupurrSaw: finds and loads the optional instrument pack (multisampled piano, FM piano, trumpet, violin, strings).
// A pack is a folder with instruments.json and the FLAC files it lists; tools/make_instruments.py builds one from CC0 sample libraries.
// The pack is not part of the plugin download. Put the folder in any of the places returned by searchFolders().
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>
#include <memory>
#include "Diagnostics.h"
#include "../../core/supurrsaw_core.h"

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
                const auto file = info.dir.getChildFile (z["file"].toString());
                std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (file));
                if (rd == nullptr || rd->lengthInSamples < 16) { Diagnostics::log ("cannot read " + file.getFullPathName()); continue; }
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
    struct Info { juce::File dir; juce::var json; };

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

    mutable juce::CriticalSection lock;
    std::map<juce::String, Info> infos;
    std::map<juce::String, std::shared_ptr<const supurr::SampleSet>> cache;
};

} // namespace s87
