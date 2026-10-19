// Studio 87 SupurrSaw: maps the page's parameter names (same names as the playground engine) onto supurr::Params.
#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include <unordered_map>
#include <string>
#include "../../core/supurrsaw_core.h"

namespace s87 {

using supurr::Params;
using Setter = std::function<void(Params&, const juce::var&)>;

inline float asF(const juce::var& v) { return (float) (double) v; }
inline int asI(const juce::var& v) { return (int) std::lround((double) v); }
inline bool asB(const juce::var& v) { return v.isBool() ? (bool) v : ((double) v > 0.5); }

inline supurr::Wave waveFrom(const juce::String& s)
{
    using W = supurr::Wave;
    if (s == "SuperSaw") return W::SuperSaw;
    if (s == "SuperSquare") return W::SuperSquare;
    if (s == "Saw") return W::Saw;
    if (s == "Pulse") return W::Pulse;
    if (s == "Tri") return W::Tri;
    if (s == "Sine") return W::Sine;
    if (s == "Sample") return W::Sample;
    if (s == "Custom") return W::Custom;
    return W::Off;
}

inline supurr::FilterMode filterFrom(const juce::String& s)
{
    using F = supurr::FilterMode;
    if (s == "LP18") return F::LP18;
    if (s == "HP") return F::HP;
    if (s == "BP") return F::BP;
    if (s == "LP12") return F::LP12;
    if (s == "LP6") return F::LP6;
    return F::LP24;
}

inline void decodeMenv(Params& p, const juce::String& hex)
{
    for (int i = 0; i < 64; ++i)
        p.menv[i] = hex.length() >= 128 ? (float) hex.substring(i * 2, i * 2 + 2).getHexValue32() / 255.f
                                        : std::exp(-4.f * (float) i / 63.f);
}

inline const std::unordered_map<std::string, Setter>& globalTable()
{
    static const std::unordered_map<std::string, Setter> t = [] {
        std::unordered_map<std::string, Setter> m;
#define S87_F(name) m[#name] = [](Params& p, const juce::var& v) { p.name = asF(v); }
#define S87_I(name) m[#name] = [](Params& p, const juce::var& v) { p.name = asI(v); }
#define S87_B(name) m[#name] = [](Params& p, const juce::var& v) { p.name = asB(v); }
        S87_F(subLevel); S87_I(subOctave); S87_B(subPreFilter); S87_B(mono); S87_I(stackCount); S87_F(stackCents); S87_F(glide);
        S87_F(cutoff); S87_F(reso); S87_F(drive); S87_F(envAmt); S87_F(keyTrack); S87_F(lowCutHz); S87_F(dist); S87_B(distPre);
        S87_F(noiseLevel); S87_F(noiseHpHz);
        S87_F(pitchAmt); S87_F(pitchTime); S87_F(pitchAttack); S87_F(pitchFall); S87_F(pitchFallTime);
        S87_F(lfoRate); S87_F(lfoPwm); S87_F(lfoPitch); S87_F(lfoCutoff); S87_F(lfoDetune); S87_I(lfoShape); S87_F(lfoPan); S87_F(lfoAmp);
        S87_F(wheelCutoff); S87_F(velCutoff); S87_F(wheelVib); S87_B(phaseReset); S87_F(drift); S87_F(detuneRange);
        S87_F(spread); S87_F(bassMonoHz);
        S87_F(chorusMix); S87_F(chorusDepth); S87_F(chorusRate); S87_F(delayMix); S87_F(delayTime); S87_F(delayFb); S87_B(delaySync); S87_I(delayDiv);
        S87_F(bpm); S87_F(optoAmt); S87_F(ottDepth); S87_F(ottMix); S87_F(sqzAmt);
        S87_F(reverbMix); S87_F(reverbSize); S87_F(reverbWidth); S87_F(reverbLowCutHz);
        S87_F(master); S87_F(clipKnee); S87_B(limiter); S87_F(limCeil); S87_I(polyphony); S87_F(modWheel);
        S87_F(dynMix); S87_F(dmMix); S87_F(dmDrive); S87_F(dmClip); S87_F(dmLow); S87_F(dmHigh); S87_F(drvDrive); S87_F(drvXs); S87_F(drvFuzz); S87_F(drvMix);
        S87_F(chorusDelay); S87_F(chorusWidth); S87_F(delayDamp); S87_F(delayPing); S87_F(reverbPre); S87_F(reverbDamp);
        S87_F(optoGain); S87_F(optoRel); S87_F(ottLow); S87_F(ottMid); S87_F(ottHigh); S87_F(ottTime);
        S87_F(sqzAtk); S87_F(sqzRel); S87_F(sqzMakeup);
        S87_F(menvTime); S87_B(menvInv); S87_B(menvLoop); S87_F(lfoDepth); S87_F(lfoFade); S87_F(lfoStart); S87_F(pitchHold); S87_F(pitchCurve);
#undef S87_F
#undef S87_I
#undef S87_B
        m["ampCA"] = [](Params& p, const juce::var& v) { p.amp.ca = asF(v); };
        m["ampCD"] = [](Params& p, const juce::var& v) { p.amp.cd = asF(v); };
        m["ampCR"] = [](Params& p, const juce::var& v) { p.amp.cr = asF(v); };
        m["fenvCA"] = [](Params& p, const juce::var& v) { p.fenv.ca = asF(v); };
        m["fenvCD"] = [](Params& p, const juce::var& v) { p.fenv.cd = asF(v); };
        m["fenvCR"] = [](Params& p, const juce::var& v) { p.fenv.cr = asF(v); };
        m["ampA"] = [](Params& p, const juce::var& v) { p.amp.a = asF(v); };
        m["ampD"] = [](Params& p, const juce::var& v) { p.amp.d = asF(v); };
        m["ampS"] = [](Params& p, const juce::var& v) { p.amp.s = asF(v); };
        m["ampR"] = [](Params& p, const juce::var& v) { p.amp.r = asF(v); };
        m["fenvA"] = [](Params& p, const juce::var& v) { p.fenv.a = asF(v); };
        m["fenvD"] = [](Params& p, const juce::var& v) { p.fenv.d = asF(v); };
        m["fenvS"] = [](Params& p, const juce::var& v) { p.fenv.s = asF(v); };
        m["fenvR"] = [](Params& p, const juce::var& v) { p.fenv.r = asF(v); };
        m["subWave"] = [](Params& p, const juce::var& v) { p.subWave = waveFrom(v.toString()); };
        m["filterMode"] = [](Params& p, const juce::var& v) { p.filterMode = filterFrom(v.toString()); };
        m["menvData"] = [](Params& p, const juce::var& v) { decodeMenv(p, v.toString()); };
        m["menvPts"] = [](Params&, const juce::var&) {};   // editor-only node data
        static const char* src[9] = { "lfo", "fenv", "aenv", "penv", "whl", "key", "vel", "rnd", "men" };
        static const char* dst[14] = { "pitch", "cut", "res", "drv", "pwm", "det", "pan", "vol", "sy1", "sy2", "sy3", "sub", "nz", "lc" };
        for (int s = 0; s < 9; ++s)
            for (int d = 0; d < 14; ++d)
                m[std::string("mx_") + src[s] + "_" + dst[d]] = [s, d](Params& p, const juce::var& v) { p.mx[s][d] = asF(v); };
        return m;
    }();
    return t;
}

// Layer fields need the layer index, so they have their own entry point.
inline bool setLayerKey(supurr::LayerParams& l, const std::string& k, const juce::var& v)
{
    if (k == "wave")       { l.wave = waveFrom(v.toString()); return true; }
    if (k == "level")      { l.level = asF(v); return true; }
    if (k == "voices")     { l.voices = asI(v); return true; }
    if (k == "detune")     { l.detune = asF(v); return true; }
    if (k == "pwm")        { l.pwm = asF(v); return true; }
    if (k == "octave")     { l.octave = asI(v); return true; }
    if (k == "tune")       { l.tune = asF(v); return true; }
    if (k == "pan")        { l.pan = asF(v); return true; }
    if (k == "sync")       { l.sync = asB(v); return true; }
    if (k == "syncSemis")  { l.syncSemis = asF(v); return true; }
    if (k == "syncEnv")    { l.syncEnv = asF(v); return true; }
    if (k == "syncLfo")    { l.syncLfo = asF(v); return true; }
    if (k == "syncKey")    { l.syncKey = asF(v); return true; }
    if (k == "syncWheel")  { l.syncWheel = asF(v); return true; }
    if (k == "cents")      { l.cents = asF(v); return true; }
    if (k == "lfoPitchCents") { l.lfoPitchCents = asF(v); return true; }
    if (k == "fmSrc")      { l.fmSrc = asI(v); return true; }
    if (k == "fmAmt")      { l.fmAmt = asF(v); return true; }
    if (k == "amSrc")      { l.amSrc = asI(v); return true; }
    if (k == "amAmt")      { l.amAmt = asF(v); return true; }
    if (k == "cycle")      return true;   // handled through the "cyc" message
    return false;
}

// Applies one page parameter (global name, or "L<n>_<field>") to p. Returns false for names it does not know.
inline bool setKey(Params& p, const juce::String& key, const juce::var& v)
{
    if (key.length() > 3 && key[0] == 'L' && key[2] == '_' && key[1] >= '0' && key[1] <= '2')
        return setLayerKey(p.layer[key[1] - '0'], key.substring(3).toStdString(), v);
    auto& t = globalTable();
    auto it = t.find(key.toStdString());
    if (it == t.end()) return false;
    it->second(p, v);
    return true;
}

// Applies a whole nested parameter object, as sent by the page ({layers:[{..},{..},{..}], ...flat keys}).
inline void setAll(Params& p, const juce::var& obj)
{
    auto* o = obj.getDynamicObject();
    if (o == nullptr) return;
    for (auto& nv : o->getProperties())
    {
        auto name = nv.name.toString();
        if (name == "layers")
        {
            if (auto* arr = nv.value.getArray())
                for (int i = 0; i < arr->size() && i < 3; ++i)
                    if (auto* lo = (*arr)[i].getDynamicObject())
                        for (auto& lv : lo->getProperties())
                            setLayerKey(p.layer[i], lv.name.toString().toStdString(), lv.value);
        }
        else setKey(p, name, nv.value);
    }
}

} // namespace s87
