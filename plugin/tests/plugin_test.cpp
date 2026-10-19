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
    std::printf ("%d presets checked, %d mismatches\n", n, bad);
    return bad == 0 && n > 0 ? 0 : 1;
}
