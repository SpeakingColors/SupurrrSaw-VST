// Checks that the plugin processor sounds exactly like the core engine: every built-in preset is sent through the
// same JSON messages the page uses, played with MIDI and compared sample by sample with the engine loaded directly.
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Source/PluginProcessor.h"
#include "../Source/ParamBridge.h"
#include "../../presets/presets.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::printf ("usage: S87Test presets.json\n"); return 2; }
    const auto all = juce::JSON::parse (juce::File (argv[1]));
    auto* obj = all.getDynamicObject();
    if (obj == nullptr) { std::printf ("cannot read %s\n", argv[1]); return 2; }

    int bad = 0, n = 0;
    for (auto& nv : obj->getProperties())
    {
        const std::string name = nv.name.toString().toStdString();
        supurr::Params ref;
        if (! supurr::presets::byName (name, ref)) { std::printf ("skip %s (no C++ preset)\n", name.c_str()); continue; }

        s87::SupurrProcessor proc;
        proc.setPlayConfigDetails (0, 2, 48000.0, 512);
        proc.prepareToPlay (48000.0, 512);
        // the preset file keeps globals under "p" and the layers beside them; the page sends one flat object
        auto* flat = new juce::DynamicObject();
        if (auto* po = nv.value["p"].getDynamicObject()) for (auto& kv : po->getProperties()) flat->setProperty (kv.name, kv.value);
        flat->setProperty ("layers", nv.value["layers"]);
        auto* msg = new juce::DynamicObject();
        msg->setProperty ("t", "set"); msg->setProperty ("p", juce::var (flat));
        proc.handleUiMessage (juce::var (msg));

        supurr::Params viaJson; s87::setAll (viaJson, juce::var (flat));
        supurr::Engine eng; supurr::presets::byName ("hoover_classic", eng.p); eng.prepare (48000.0); eng.p = viaJson;
        double maxDiff = 0.0, peak = 0.0;
        juce::AudioBuffer<float> buf (2, 512);
        std::vector<float> rl (512), rr (512);
        for (int blk = 0; blk < 40; ++blk)
        {
            juce::MidiBuffer midi;
            if (blk == 0) { midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.8f), 37); midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.7f), 200); }
            if (blk == 25) midi.addEvent (juce::MidiMessage::noteOff (1, 57), 100);
            proc.processBlock (buf, midi);

            // reference: same slices
            std::fill (rl.begin(), rl.end(), 0.f); std::fill (rr.begin(), rr.end(), 0.f);
            int pos = 0;
            auto go = [&] (int end) { if (end > pos) eng.render (rl.data() + pos, rr.data() + pos, end - pos); pos = std::max (pos, end); };
            if (blk == 0) { go (37); eng.noteOn (57, 102.f / 127.f); go (200); eng.noteOn (64, 89.f / 127.f); }   // MIDI velocity is 7 bit
            if (blk == 25) { go (100); eng.noteOff (57); }
            go (512);
            for (int i = 0; i < 512; ++i)
            {
                maxDiff = std::max (maxDiff, (double) std::abs (buf.getSample (0, i) - rl[(size_t) i]));
                maxDiff = std::max (maxDiff, (double) std::abs (buf.getSample (1, i) - rr[(size_t) i]));
                peak = std::max (peak, (double) std::abs (rl[(size_t) i]));
            }
        }
        ++n;
        const bool ok = maxDiff < 1e-6 && peak > 0.01;
        if (! ok) ++bad;
        {   // how far the JSON route is from the C++ preset itself (the dump prints 6 digits)
            supurr::Engine a, b; a.p = ref; b.p = viaJson; a.prepare (48000.0); b.prepare (48000.0); a.noteOn (57, 0.8f); b.noteOn (57, 0.8f);
            std::vector<float> al (4096), ar (4096), bl (4096), br (4096); a.render (al.data(), ar.data(), 4096); b.render (bl.data(), br.data(), 4096);
            double m = 0; for (size_t i = 0; i < 4096; ++i) m = std::max (m, (double) std::abs (al[i] - bl[i]));
            std::printf ("  [json vs C++ preset %.2e] ", m);
        }
        std::printf ("%-18s peak %.3f  max diff %.2e  %s\n", name.c_str(), peak, maxDiff, ok ? "ok" : "MISMATCH");
    }

    // ---- drum kit mode through the same messages the page sends ----
    int kitBad = 0;
    {
        auto flatOf = [&] (const char* name) -> juce::var
        {
            auto* f = new juce::DynamicObject();
            const auto pv = obj->getProperty (name);
            if (auto* po = pv["p"].getDynamicObject()) for (auto& kv : po->getProperties()) f->setProperty (kv.name, kv.value);
            f->setProperty ("layers", pv["layers"]);
            return juce::var (f);
        };
        auto pad = [&] (const char* name, int key, int play, double g, juce::Array<juce::var> ch)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("n", juce::Array<juce::var> { key }); o->setProperty ("play", play); o->setProperty ("g", g);
            o->setProperty ("ch", ch); o->setProperty ("full", flatOf (name));
            return juce::var (o);
        };
        juce::Array<juce::var> pads { pad ("kick_909", 36, 36, 1.0, {}), pad ("snare_909", 38, 60, 0.9, {}), pad ("hat_909_closed", 42, 60, 0.5, { 46 }), pad ("hat_909_open", 46, 60, 0.58, {}) };
        auto* km = new juce::DynamicObject(); km->setProperty ("t", "kit"); km->setProperty ("k", pads);
        const juce::var kitMsg (km);

        auto energy = [] (s87::SupurrProcessor& pr, int note, int blocks)
        {
            juce::AudioBuffer<float> b (2, 512); double e = 0;
            for (int i = 0; i < blocks; ++i)
            {
                juce::MidiBuffer m; if (i == 0 && note >= 0) m.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 10);
                pr.processBlock (b, m);
                for (int j = 0; j < 512; ++j) e += (double) b.getSample (0, j) * b.getSample (0, j);
            }
            return e;
        };
        auto fresh = [&] ()
        {
            auto p = std::make_unique<s87::SupurrProcessor>();
            p->setPlayConfigDetails (0, 2, 48000.0, 512); p->prepareToPlay (48000.0, 512); return p;
        };
        auto pr = fresh();
        { juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; pr->processBlock (b, m); }
        pr->handleUiMessage (kitMsg);
        { juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; pr->processBlock (b, m); }   // the audio thread takes the kit over
        const double eKick = energy (*pr, 36, 40), eNone = energy (*pr, 60, 20);
        std::printf ("kit: kick energy %.4f, unmapped key %.6f\n", eKick, eNone);
        if (! (eKick > 1e-3) || eNone > 1e-4 * eKick) ++kitBad;   // the unmapped key only hears the kick tail still ringing

        // the processor must sound like the core KitEngine given the same pads
        {
            std::vector<supurr::KitEngine::Def> defs;
            for (auto& pv : pads) { supurr::KitEngine::Def d; d.n = { (int) pv["n"][0] }; d.play = (int) pv["play"]; d.g = (float) (double) pv["g"]; for (auto& c : *pv["ch"].getArray()) d.ch.push_back ((int) c); s87::setAll (d.p, pv["full"]); defs.push_back (d); }
            supurr::KitEngine ref; ref.build (defs, 48000.0, 8192);
            auto pr2 = fresh(); pr2->handleUiMessage (kitMsg);
            { juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; pr2->processBlock (b, m); }
            supurr::Params mainP; supurr::presets::byName ("trance_90s", mainP);
            double md = 0; std::vector<float> rl (512), rr (512);
            for (int blk = 0; blk < 30; ++blk)
            {
                juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
                if (blk == 0) m.addEvent (juce::MidiMessage::noteOn (1, 36, 0.8f), 0);
                if (blk == 8) m.addEvent (juce::MidiMessage::noteOn (1, 46, 0.8f), 0);
                if (blk == 12) m.addEvent (juce::MidiMessage::noteOn (1, 42, 0.8f), 0);
                pr2->processBlock (b, m);
                if (blk == 0) ref.noteOn (36, 102.f / 127.f);
                if (blk == 8) ref.noteOn (46, 102.f / 127.f);
                if (blk == 12) ref.noteOn (42, 102.f / 127.f);
                ref.render (rl.data(), rr.data(), 512, mainP.master, mainP.clipKnee);
                for (int i = 0; i < 512; ++i) md = std::max (md, (double) std::abs (b.getSample (0, i) - rl[(size_t) i]));
            }
            std::printf ("kit: processor vs core KitEngine max diff %.2e\n", md);
            if (md > 1e-6) ++kitBad;
        }

        // the kit is saved with the project and comes back
        juce::MemoryBlock st; pr->getStateInformation (st);
        auto pr3 = std::make_unique<s87::SupurrProcessor>();
        pr3->setStateInformation (st.getData(), (int) st.getSize());
        pr3->setPlayConfigDetails (0, 2, 48000.0, 512); pr3->prepareToPlay (48000.0, 512);
        { juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; pr3->processBlock (b, m); }
        const double eBack = energy (*pr3, 36, 40), eBackNone = energy (*pr3, 60, 20);
        std::printf ("kit restored from project state: kick %.4f, unmapped %.6f\n", eBack, eBackNone);
        if (! (eBack > 1e-3) || eBackNone > 1e-4 * eBack) ++kitBad;

        // kit off: the synth plays again on every key
        auto* off = new juce::DynamicObject(); off->setProperty ("t", "kit"); off->setProperty ("k", juce::var());
        pr->handleUiMessage (juce::var (off));
        { juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; pr->processBlock (b, m); }
        const double eSynth = energy (*pr, 60, 40);
        std::printf ("kit off: synth plays key 60, energy %.4f\n", eSynth);
        if (! (eSynth > 1e-3)) ++kitBad;
    }
    std::printf ("kit mode: %s\n", kitBad == 0 ? "ok" : "FAILED");
    bad += kitBad;
    std::printf ("%d presets checked, %d mismatches\n", n, bad);
    return bad == 0 && n > 0 ? 0 : 1;
}
