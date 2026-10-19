// Studio 87 SupurrSaw: maps the page's parameter names (same names as the playground engine) onto supurr::Params.
#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
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
inline float asFR(const juce::var& v, float lo, float hi) { return std::min(hi, std::max(lo, asF(v))); }   // clamped to the range of the page's knob
inline int asIR(const juce::var& v, int lo, int hi) { return std::min(hi, std::max(lo, asI(v))); }

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
    return supurr::filterModeFromName(s.toRawUTF8());   // every name of the page's filter list; anything unknown is LP24
}

inline int smpFrom(const juce::String& s) { return supurr::smpFromId(s.toRawUTF8()); }   // built-in sample id ("kick_punch", ...); unknown or "Off" is 0

inline void decodeMenv(float* menv, const juce::String& hex)   // 64 values as 128 hex digits; anything else restores the default fall
{
    for (int i = 0; i < 64; ++i)
        menv[i] = hex.length() >= 128 ? (float) hex.substring(i * 2, i * 2 + 2).getHexValue32() / 255.f
                                      : (float) std::exp(-4.0 * i / 63.0);
}

inline const std::unordered_map<std::string, Setter>& globalTable()
{
    static const std::unordered_map<std::string, Setter> t = [] {
        std::unordered_map<std::string, Setter> m;
#define S87_F(name) m[#name] = [](Params& p, const juce::var& v) { p.name = asF(v); }
#define S87_I(name) m[#name] = [](Params& p, const juce::var& v) { p.name = asI(v); }
#define S87_B(name) m[#name] = [](Params& p, const juce::var& v) { p.name = asB(v); }
#define S87_FR(name, lo, hi) m[#name] = [](Params& p, const juce::var& v) { p.name = asFR(v, lo, hi); }
#define S87_IR(name, lo, hi) m[#name] = [](Params& p, const juce::var& v) { p.name = asIR(v, lo, hi); }
        S87_F(subLevel); S87_I(subOctave); S87_B(subPreFilter); S87_B(mono); S87_I(stackCount); S87_F(stackCents); S87_F(glide);
        S87_F(cutoff); S87_F(reso); S87_F(drive); S87_F(envAmt); S87_F(keyTrack); S87_FR(lowCutHz, 5.f, 20000.f); S87_F(dist); S87_B(distPre);
        S87_F(noiseLevel); S87_F(noiseHpHz); S87_IR(subVoices, 1, 5); S87_F(subDetune); S87_F(subWidth); S87_IR(noiseVoices, 1, 5); S87_F(noiseDetune); S87_F(noiseWidth);
        S87_F(pitchAmt); S87_F(pitchTime); S87_F(pitchAttack); S87_F(pitchFall); S87_F(pitchFallTime);
        S87_F(lfoRate); S87_F(lfoPwm); S87_F(lfoPitch); S87_F(lfoCutoff); S87_F(lfoDetune); S87_I(lfoShape); S87_F(lfoPan); S87_F(lfoAmp);
        S87_F(wheelCutoff); S87_F(velCutoff); S87_F(wheelVib); S87_B(phaseReset); S87_F(drift); S87_F(detuneRange);
        S87_F(spread); S87_F(bassMonoHz);
        S87_F(chorusMix); S87_F(chorusDepth); S87_F(chorusRate); S87_F(delayMix); S87_F(delayTime); S87_F(delayFb); S87_B(delaySync); S87_I(delayDiv);
        S87_F(bpm); S87_F(optoAmt); S87_F(ottDepth); S87_F(ottMix); S87_F(sqzAmt);
        S87_F(reverbMix); S87_F(reverbSize); S87_F(reverbWidth); S87_F(reverbLowCutHz);
        S87_F(master); S87_F(clipKnee); S87_B(limiter); S87_F(limCeil); S87_I(polyphony); S87_F(modWheel);
        S87_F(dynMix); S87_F(dmMix); S87_F(dmDrive); S87_F(dmClip); S87_F(dmLow); S87_F(dmMid); S87_F(dmHigh); S87_F(drvDrive); S87_F(drvXs); S87_F(drvFuzz); S87_F(drvMix);
        S87_B(eqBypass); S87_B(eq1On); S87_F(eq1F); S87_B(eq2On); S87_F(eq2F); S87_F(eq2G); S87_B(eq3On); S87_F(eq3F); S87_F(eq3G); S87_F(eq3Q);
        S87_B(eq4On); S87_F(eq4F); S87_F(eq4G); S87_F(eq4Q); S87_B(eq5On); S87_F(eq5F); S87_F(eq5G); S87_B(eq6On); S87_F(eq6F);
        S87_F(chorusDelay); S87_F(chorusWidth); S87_F(delayDamp); S87_F(delayPing); S87_F(reverbPre); S87_F(reverbDamp);
        S87_F(optoGain); S87_F(optoRel); S87_F(ottLow); S87_F(ottMid); S87_F(ottHigh); S87_F(ottTime);
        S87_F(sqzAtk); S87_F(sqzRel); S87_F(sqzMakeup);
        S87_F(menvTime); S87_B(menvInv); S87_B(menvLoop); S87_F(lfoDepth); S87_F(lfoFade); S87_F(lfoStart); S87_F(pitchHold); S87_F(pitchCurve);
        // added with the big build: ranges are the ones of the page's knobs
        S87_FR(highCutHz, 20.f, 20000.f); S87_IR(auxSlope, 0, 5); S87_B(ampLink); S87_IR(fRoute, 0, 1); S87_FR(build, 0.f, 1.f);
        S87_IR(modFxType, 0, 2); S87_FR(chorusFb, -0.95f, 0.95f); S87_FR(crushBits, 1.f, 16.f); S87_FR(crushRate, 1.f, 40.f); S87_FR(crushMix, 0.f, 1.f);
        S87_FR(widMid, 0.f, 2.f); S87_FR(widHi, 0.f, 2.f); S87_FR(msMid, 0.f, 1.f); S87_FR(msSide, 0.f, 1.f);
        S87_FR(f2Cut, 20.f, 20000.f); S87_FR(f2Res, 0.f, 1.f); S87_FR(f2Drv, 0.f, 1.5f); S87_F(f2Env); S87_F(f2Key);
        S87_FR(f3Cut, 20.f, 20000.f); S87_FR(f3Res, 0.f, 1.f); S87_FR(f3Drv, 0.f, 1.5f); S87_F(f3Env); S87_F(f3Key);
        S87_FR(lfo2Rate, 0.05f, 30.f); S87_IR(lfo2Shape, 0, 6); S87_FR(lfo2Depth, 0.f, 1.f); S87_FR(lfo2Fade, 0.f, 5.f); S87_FR(lfo2Start, 0.f, 1.f);
        S87_FR(lfo3Rate, 0.05f, 30.f); S87_IR(lfo3Shape, 0, 6); S87_FR(lfo3Depth, 0.f, 1.f); S87_FR(lfo3Fade, 0.f, 5.f); S87_FR(lfo3Start, 0.f, 1.f);
        S87_F(menv2Time); S87_B(menv2Inv); S87_B(menv2Loop); S87_F(menv3Time); S87_B(menv3Inv); S87_B(menv3Loop); S87_B(menvSync); S87_B(menv2Sync); S87_B(menv3Sync); S87_IR(menvDiv, 0, 15); S87_IR(menv2Div, 0, 15); S87_IR(menv3Div, 0, 15);
        S87_FR(nzSmpPitch, -24.f, 24.f); S87_FR(nzSmpVol, 0.f, 1.5f); S87_B(smpPost);
        S87_FR(eq2Dyn, -18.f, 18.f); S87_FR(eq2Thr, -60.f, 0.f); S87_FR(eq2Atk, 0.5f, 100.f); S87_FR(eq2Rel, 10.f, 1000.f); S87_B(eq2Below); S87_FR(eq2Sat, 0.f, 1.f); S87_IR(eq2SatT, 0, 7);
        S87_FR(eq3Dyn, -18.f, 18.f); S87_FR(eq3Thr, -60.f, 0.f); S87_FR(eq3Atk, 0.5f, 100.f); S87_FR(eq3Rel, 10.f, 1000.f); S87_B(eq3Below); S87_FR(eq3Sat, 0.f, 1.f); S87_IR(eq3SatT, 0, 7);
        S87_FR(eq4Dyn, -18.f, 18.f); S87_FR(eq4Thr, -60.f, 0.f); S87_FR(eq4Atk, 0.5f, 100.f); S87_FR(eq4Rel, 10.f, 1000.f); S87_B(eq4Below); S87_FR(eq4Sat, 0.f, 1.f); S87_IR(eq4SatT, 0, 7);
        S87_FR(eq5Dyn, -18.f, 18.f); S87_FR(eq5Thr, -60.f, 0.f); S87_FR(eq5Atk, 0.5f, 100.f); S87_FR(eq5Rel, 10.f, 1000.f); S87_B(eq5Below); S87_FR(eq5Sat, 0.f, 1.f); S87_IR(eq5SatT, 0, 7);
#undef S87_FR
#undef S87_IR
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
        m["f2Mode"] = [](Params& p, const juce::var& v) { p.f2Mode = filterFrom(v.toString()); };
        m["f3Mode"] = [](Params& p, const juce::var& v) { p.f3Mode = filterFrom(v.toString()); };
        m["nzSmp"] = [](Params& p, const juce::var& v) { p.nzSmp = smpFrom(v.toString()); };
        // per layer amp envelopes (used when ampLink is off) and the envelopes of filters 2 and 3
        struct EnvKey { const char* name; supurr::Adsr Params::*e; };
        const EnvKey envs[4] = { { "amp2", &Params::amp2 }, { "amp3", &Params::amp3 }, { "fenv2", &Params::fenv2 }, { "fenv3", &Params::fenv3 } };
        for (const auto& ek : envs)
        {
            const std::string n = ek.name; auto e = ek.e;
            m[n + "A"]  = [e](Params& p, const juce::var& v) { (p.*e).a = asF(v); };
            m[n + "D"]  = [e](Params& p, const juce::var& v) { (p.*e).d = asF(v); };
            m[n + "S"]  = [e](Params& p, const juce::var& v) { (p.*e).s = asF(v); };
            m[n + "R"]  = [e](Params& p, const juce::var& v) { (p.*e).r = asF(v); };
            m[n + "CA"] = [e](Params& p, const juce::var& v) { (p.*e).ca = asF(v); };
            m[n + "CD"] = [e](Params& p, const juce::var& v) { (p.*e).cd = asF(v); };
            m[n + "CR"] = [e](Params& p, const juce::var& v) { (p.*e).cr = asF(v); };
        }
        m["menvData"]  = [](Params& p, const juce::var& v) { decodeMenv(p.menv, v.toString()); };
        m["menv2Data"] = [](Params& p, const juce::var& v) { decodeMenv(p.menv2, v.toString()); };
        m["menv3Data"] = [](Params& p, const juce::var& v) { decodeMenv(p.menv3, v.toString()); };
        m["menvPts"] = [](Params&, const juce::var&) {};   // editor-only node data
        m["menv2Pts"] = [](Params&, const juce::var&) {};
        m["menv3Pts"] = [](Params&, const juce::var&) {};
        for (int s = 0; s < supurr::kMxSrc; ++s)
            for (int d = 0; d < supurr::kMxDst; ++d)
                m[std::string("mx_") + supurr::kMxSrcNames[s] + "_" + supurr::kMxDstNames[d]] = [s, d](Params& p, const juce::var& v) { p.mx[s][d] = asF(v); };
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
    if (k == "smp")        { l.smp = smpFrom(v.toString()); return true; }
    if (k == "smpPitch")   { l.smpPitch = asFR(v, -24.f, 24.f); return true; }
    if (k == "smpVol")     { l.smpVol = asFR(v, 0.f, 1.5f); return true; }
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
