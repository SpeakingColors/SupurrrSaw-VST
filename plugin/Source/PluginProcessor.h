// Studio 87 SupurrSaw: audio processor. The sound engine is the header only core in core/supurrsaw_core.h.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include "../../core/supurrsaw_core.h"

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
    struct MeterSnap { float l = 0, r = 0, lim = 0, gr[3] {}, dm[3] {}; int active = 0, trips = 0; };
    MeterSnap takeMeters();
    int readScope (float* dest, int maxCount);       // new samples since the last call (mono), newest last

    double currentSampleRate() const { return sampleRateNow.load(); }

private:
    void applyPendingParams();
    void pushEvent (int type, int note, float vel);
    void buildTable (int layer, const float* data, int len);

    supurr::Engine engine;

    // parameters: message thread edits `pending`, the audio thread copies it when `dirty`
    juce::SpinLock paramLock;
    supurr::Params pending;
    std::atomic<bool> dirty { false };
    std::shared_ptr<const supurr::WaveTable> tables[3];
    std::vector<std::shared_ptr<const supurr::WaveTable>> retired;   // keeps old tables alive off the audio thread
    juce::String tableSrc[3];

    // saved state (message thread only)
    juce::CriticalSection stateLock;
    juce::var paramsJson;   // nested object exactly as the page sends it
    juce::var uiJson;

    // notes from the page
    struct Ev { int type = 0, note = 0; float vel = 0; };
    juce::AbstractFifo fifo { 1024 };
    std::array<Ev, 1024> fifoData {};

    std::atomic<float> volume { 1.f };
    std::atomic<bool> muted { false };
    bool wasMuted = false;
    float volNow = 1.f;
    float wheelCC = 0.f; bool wheelSeen = false;

    std::atomic<double> sampleRateNow { 48000.0 };
    std::atomic<float> mPeakL { 0 }, mPeakR { 0 }, mLim { 0 }, mGr[3], mDm[3];
    std::atomic<int> mActive { 0 }, mTrips { 0 };

    static constexpr int kScope = 65536;
    std::array<float, kScope> scope {};
    std::atomic<juce::uint64> scopeWp { 0 };
    juce::uint64 scopeRp = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SupurrProcessor)
};

} // namespace s87
