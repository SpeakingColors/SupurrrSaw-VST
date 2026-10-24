// Checks the instrument pack path of the plugin: a pack folder is found, an instrument is chosen with the same JSON messages the page sends,
// played with MIDI it sounds in tune, it survives a project save and load, and a missing pack is silent instead of crashing.
// The pack is generated here (sine zones as WAV files), so no recordings are needed. HOME is pointed at a temporary folder.
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Source/PluginProcessor.h"
#include <cstdlib>

static double freqOf (const std::vector<float>& x, double sr)
{
    double first = -1, last = -1; int n = 0;
    for (size_t i = 1; i < x.size(); ++i)
        if (x[i - 1] < 0.f && x[i] >= 0.f) { const double t = ((double) i - 1.0 + (-x[i - 1]) / (x[i] - x[i - 1])) / sr; if (first < 0) first = t; last = t; ++n; }
    return n > 2 ? (n - 1) / (last - first) : 0.0;
}

// fundamental by autocorrelation, for sounds with many harmonics (a piano has far more zero crossings than periods)
static double acfFreq (const std::vector<float>& x, double sr)
{
    const size_t n = std::min<size_t> (x.size(), 8192);
    double best = 0; size_t bestLag = 0;
    for (size_t lag = (size_t) (sr / 1000.0); lag < (size_t) (sr / 80.0) && lag < n / 2; ++lag)
    {
        double a = 0, e1 = 0, e2 = 0;
        for (size_t i = 0; i + lag < n; ++i) { a += (double) x[i] * x[i + lag]; e1 += (double) x[i] * x[i]; e2 += (double) x[i + lag] * x[i + lag]; }
        const double c = a / std::sqrt (e1 * e2 + 1e-12);
        if (c > best) { best = c; bestLag = lag; }
    }
    return bestLag > 0 ? sr / (double) bestLag : 0.0;
}

static void writeSine (const juce::File& f, int root, double tuneCents, double secs)
{
    const double sr = 44100.0, fr = 440.0 * std::pow (2.0, (root - 69 + tuneCents / 100.0) / 12.0);
    const int n = (int) (secs * sr);
    juce::AudioBuffer<float> b (1, n);
    for (int i = 0; i < n; ++i) b.setSample (0, i, 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * fr * i / sr));
    juce::WavAudioFormat wav;
    f.deleteFile();
    std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream());
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 1, 16, {}, 0));
    if (w != nullptr) { os.release(); w->writeFromAudioSampleBuffer (b, 0, n); }
}

static juce::var msgP (const juce::String& k, const juce::var& v)
{
    auto* o = new juce::DynamicObject(); o->setProperty ("t", "p"); o->setProperty ("k", k); o->setProperty ("v", v); return juce::var (o);
}

// plays one note through the processor, returns the left channel
static std::vector<float> play (s87::SupurrProcessor& p, int note, int blocks)
{
    std::vector<float> out;
    juce::AudioBuffer<float> buf (2, 512);
    for (int b = 0; b < blocks; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
        buf.clear();
        p.processBlock (buf, midi);
        if (b >= 6) for (int i = 0; i < 512; ++i) out.push_back (buf.getSample (0, i));
    }
    return out;
}
static double rms (const std::vector<float>& x, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b && i < x.size(); ++i) s += x[i] * x[i]; return std::sqrt (s / (double) (b - a)); }

static std::unique_ptr<s87::SupurrProcessor> make()
{
    auto p = std::make_unique<s87::SupurrProcessor>();
    p->setPlayConfigDetails (0, 2, 48000.0, 512); p->prepareToPlay (48000.0, 512);
    return p;
}

static void choose (s87::SupurrProcessor& p, const juce::String& inst)
{
    p.handleUiMessage (msgP ("L0_wave", "Sample"));
    p.handleUiMessage (msgP ("L0_instrument", inst));
    p.handleUiMessage (msgP ("L0_level", 1.0));
    p.handleUiMessage (msgP ("L1_wave", "Off"));
    p.handleUiMessage (msgP ("L2_wave", "Off"));
    p.handleUiMessage (msgP ("cutoff", 20000.0));
    p.handleUiMessage (msgP ("subLevel", 0.0));
    for (auto k : { "noiseLevel", "chorusMix", "reverbMix", "delayMix", "dmMix", "dist", "drvDrive", "drvFuzz", "drvXs", "optoAmt", "ottDepth", "sqzAmt" }) p.handleUiMessage (msgP (k, 0.0));
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const auto home = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("s87_inst_test_" + juce::String (juce::Random::getSystemRandom().nextInt (999999)));
    home.createDirectory();
    setenv ("HOME", home.getFullPathName().toRawUTF8(), 1);
    setenv ("XDG_CONFIG_HOME", home.getChildFile (".config").getFullPathName().toRawUTF8(), 1);

    int fails = 0;
    auto ck = [&] (bool ok, const char* what, double got, double want) { std::printf ("%-52s got %.3f want %.3f %s\n", what, got, want, ok ? "ok" : "FAIL"); if (! ok) ++fails; };

    // 1. the pack inside the plugin: listed, sounds, right pitch; an instrument that does not exist is silent
    {
        auto p = make();
        const auto names = p->instrumentNames();
        ck (names.contains ("Grand Piano") && names.contains ("Moist Keys") && names.contains ("Cello Ensemble"), "built-in pack: instruments are listed", (double) names.size(), 8);
        for (auto nm : { "Grand Piano", "Moist Keys", "FM Piano", "Trumpet", "Solo Violin", "Violin Ensemble", "Viola Ensemble", "Cello Ensemble" })
        {
            auto pp = make(); choose (*pp, nm);
            auto x = play (*pp, 60, 40);
            const double r = rms (x, 0, x.size());
            char what[80]; std::snprintf (what, sizeof what, "built-in %s sounds (rms)", nm);
            ck (r > 0.01, what, r, 0.1);
            if (juce::String (nm) == "Grand Piano" || juce::String (nm) == "Trumpet")
            {
                const double got = acfFreq (x, 48000.0), want = 261.626;
                std::snprintf (what, sizeof what, "built-in %s pitch at C4 (Hz)", nm);
                ck (got > 0.0 && std::abs (1200.0 * std::log2 (got / want)) < 12.0, what, got, want);
            }
        }
        auto q = make();
        choose (*q, "No Such Instrument");
        auto x = play (*q, 60, 30);
        ck (rms (x, 0, x.size()) < 1e-4, "unknown instrument: layer is silent", rms (x, 0, x.size()), 0);
    }

    // 2. a pack with two zones, one recorded 20 cents sharp
    const auto packDir = s87::Diagnostics::root().getChildFile ("Instruments").getChildFile ("test");
    packDir.createDirectory();
    writeSine (packDir.getChildFile ("a.wav"), 48, 0.0, 1.5);
    writeSine (packDir.getChildFile ("b.wav"), 60, 20.0, 1.5);
    packDir.getChildFile ("instruments.json").replaceWithText (
        R"({"version":1,"instruments":[{"name":"Test Piano","loop":false,"zones":[
            {"file":"a.wav","root":48,"tune":0.0,"loopStart":0,"loopEnd":0,"lo":0,"hi":53},
            {"file":"b.wav","root":60,"tune":20.0,"loopStart":0,"loopEnd":0,"lo":54,"hi":127}]}]})");
    {
        auto p = make();
        const auto names = p->instrumentNames();
        ck (names.contains ("Test Piano"), "pack found: Test Piano is listed", (double) names.size(), 1);
        choose (*p, "Test Piano");
        for (int note : { 45, 48, 52, 60, 67, 72 })
        {
            auto pp = make(); choose (*pp, "Test Piano");
            auto x = play (*pp, note, 30);
            const double want = 440.0 * std::pow (2.0, (note - 69) / 12.0), got = freqOf (x, 48000.0);
            char what[64]; std::snprintf (what, sizeof what, "pitch of MIDI note %d (Hz)", note);
            ck (got > 0.0 && std::abs (1200.0 * std::log2 (got / want)) < 4.0, what, got, want);
        }
        // 3. project save and load keeps the instrument
        juce::MemoryBlock state; p->getStateInformation (state);
        auto q = make(); q->setStateInformation (state.getData(), (int) state.getSize());
        auto x = play (*q, 60, 30);
        ck (rms (x, 0, x.size()) > 0.05, "project reload: instrument still sounds", rms (x, 0, x.size()), 0.35);
        // 4. a preset sent as one "set" message (as the page does on preset change)
        auto* layer0 = new juce::DynamicObject(); layer0->setProperty ("wave", "Sample"); layer0->setProperty ("instrument", "Test Piano"); layer0->setProperty ("level", 1.0);
        auto* off = new juce::DynamicObject(); off->setProperty ("wave", "Off");
        juce::Array<juce::var> ls; ls.add (juce::var (layer0)); ls.add (juce::var (off)); ls.add (juce::var (off->clone().get()));
        auto* flat = new juce::DynamicObject(); flat->setProperty ("layers", ls); flat->setProperty ("cutoff", 20000.0); flat->setProperty ("subLevel", 0.0);
        auto* m = new juce::DynamicObject(); m->setProperty ("t", "set"); m->setProperty ("p", juce::var (flat));
        auto r = make(); r->handleUiMessage (juce::var (m));
        auto y = play (*r, 64, 30);
        ck (rms (y, 0, y.size()) > 0.05, "preset message: instrument sounds", rms (y, 0, y.size()), 0.35);
    }

    home.deleteRecursively();
    std::printf (fails ? "SOME TESTS FAILED\n" : "all instrument tests passed\n");
    return fails ? 1 : 0;
}
