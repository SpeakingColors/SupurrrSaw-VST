// Studio 87 SupurrSaw: audio processor. The sound engine is the header only core in core/supurrsaw_core.h.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include "../../core/supurrsaw_core.h"
#include "InstrumentLibrary.h"

namespace s87 {

class SupurrProcessor : public juce::AudioProcessor
{
public:
    SupurrProcessor();
    ~SupurrProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- message thread: everything the page sends ----
    void handleUiMessage (const juce::var& m);       // {t:"set"|"p"|"on"|"off"|"all"|"cyc", ...}
    void setUiState (const juce::var& ui);           // {name, base, page}
    void setOutputVolume (float v) { volume.store (v); }
    void setMuted (bool m) { muted.store (m); }
    juce::var getInitState();                        // what the page restores at start

    // ---- meters and scope, read by the editor timer ----
    struct MeterSnap { float l = 0, r = 0, lim = 0, gr[3] {}, dm[3] {}, wet[8] {}; int active = 0, trips = 0; };
    MeterSnap takeMeters();
    int readScope (float* dest, int maxCount);       // new samples since the last call (mono), newest last

    double currentSampleRate() const { return sampleRateNow.load(); }

    // instrument pack (multisampled piano, strings, trumpet...); empty when no pack is installed
    juce::StringArray instrumentNames();

private:
    void applyPendingParams();
    void useInstrument (int layer, const juce::String& name);                    // message thread: loads (cached) and hands the set to the engine
    void resolveInstruments (supurr::Params& np, const juce::var& params);       // fills sampleSet of every layer that names an instrument
    void pushEvent (int type, int note, float vel);
    void buildTable (int layer, const float* data, int len);
    void setKitFromJson (const juce::var& k);        // drum kit mode: a list of pads, or null for off
    void rebuildKit();                               // builds the pads on the message thread and hands them to the audio thread

    supurr::Engine engine;

    // drum kit mode: the message thread builds a KitEngine, the audio thread takes it over at the next block
    juce::SpinLock kitLock;
    std::shared_ptr<supurr::KitEngine> kitNext;                       // handed over (under kitLock)
    std::vector<std::shared_ptr<supurr::KitEngine>> kitRetired;       // keeps old kits alive so they are freed off the audio thread
    std::shared_ptr<supurr::KitEngine> kitLive;                       // audio thread only
    std::atomic<bool> kitDirty { false };
    std::vector<supurr::KitEngine::Def> kitDefs;                      // message thread: lets the kit be rebuilt when the sample rate changes
    std::atomic<int> maxBlockNow { 8192 };

    // parameters: message thread edits `pending`, the audio thread copies it when `dirty`
    juce::SpinLock paramLock;
    supurr::Params pending;
    std::atomic<bool> dirty { false };
    std::shared_ptr<const supurr::WaveTable> tables[3];
    std::vector<std::shared_ptr<const supurr::WaveTable>> retired;   // keeps old tables alive off the audio thread
    juce::String tableSrc[3];
    InstrumentLibrary library;
    std::shared_ptr<const supurr::SampleSet> sets[3];
    std::vector<std::shared_ptr<const supurr::SampleSet>> setsRetired;   // old sets stay alive: voices that are still sounding point into them

    // saved state (message thread only)
    juce::CriticalSection stateLock;
    juce::var paramsJson;   // nested object exactly as the page sends it
    juce::var uiJson;
    juce::var kitJson;      // the kit exactly as the page sent it (resolved pads), saved with the project

    // notes from the page
    struct Ev { int type = 0, note = 0; float vel = 0; };
    juce::AbstractFifo fifo { 1024 };
    std::array<Ev, 1024> fifoData {};

    std::atomic<float> volume { 1.f };
    std::atomic<bool> muted { false };
    bool wasMuted = false;
    float volNow = 1.f;
    float wheelCC = 0.f; bool wheelSeen = false;
    std::atomic<float> liveBend { 0.f }, livePress { 0.f }, liveSlide { 0.f };   // pitch wheel, pressure and CC74 on the main channel: from MIDI or from the page

    std::atomic<double> sampleRateNow { 48000.0 };
    std::atomic<float> mPeakL { 0 }, mPeakR { 0 }, mLim { 0 }, mGr[3], mDm[3], mWet[8];
    std::atomic<int> mActive { 0 }, mTrips { 0 };

    static constexpr int kScope = 65536;
    std::array<float, kScope> scope {};
    std::atomic<juce::uint64> scopeWp { 0 };
    juce::uint64 scopeRp = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SupurrProcessor)
};

} // namespace s87
