// Writes every built-in preset as JSON (used by the browser playground so it plays exactly the same settings).
// Usage: dump_presets > presets.json
#include "../presets/presets.h"
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
using namespace supurr;
static const char* FM(FilterMode m) { return kFilterNames[(int)m]; }
static const char* B(bool b) { return b ? "true" : "false"; }
static const char* W[] = {"Off", "SuperSaw", "SuperSquare", "Saw", "Pulse", "Tri", "Sine", "Sample", "Custom"};
int main() {
  const char* names[] = {"hoover", "hoover_classic", "hoover_prodigy", "pick", "pluck", "trance_lead", "trance_hybrid", "trance_90s", "talking_lead", "daft_funk", "daft_funk_fat", "trance_bass", "funk_bass", "freeway_bass",
                         "better_off_alone", "gimme_lead", "levels_lead", "sandstorm_lead", "gigi_lead", "alexf_lead", "mgmt_lead", "pendulum_lead", "nightcall_keys", "brejcha_bass", "feellove_bass", "nboy_303", "spaghetti_bass", "access_bass", "epiano_dx", "epiano_wurly", "piano", "felt_piano", "organ", "clav", "strings", "warm_pad", "glass_pad", "choir", "bell", "brass", "kick", "snare", "closed_hat", "open_hat", "crash"};
  std::printf("{\n"); bool first = true;
  std::vector<std::string> all(std::begin(names), std::end(names)); { int rn; const presets::RaveEntry* RL = presets::raveList(rn); for (int i = 0; i < rn; ++i) all.push_back(RL[i].name); }
  for (const std::string& n : all) {
    Params p; if (!presets::byName(n, p)) { std::fprintf(stderr, "missing %s\n", n.c_str()); return 1; }
    std::printf("%s\"%s\": {\"layers\":[", first ? "" : ",\n", n.c_str()); first = false;
    for (int l = 0; l < kLayers; ++l) { const LayerParams& a = p.layer[l];
      std::printf("%s{\"wave\":\"%s\",\"level\":%g,\"voices\":%d,\"detune\":%g,\"pwm\":%g,\"octave\":%d,\"tune\":%g,\"pan\":%g,\"sync\":%s,\"syncSemis\":%g,\"syncEnv\":%g,\"syncLfo\":%g,\"syncKey\":%g,\"syncWheel\":%g,\"cents\":%g,\"lfoPitchCents\":%g,\"fmSrc\":%d,\"fmAmt\":%g,\"amSrc\":%d,\"amAmt\":%g",
        l ? "," : "", W[(int)a.wave], a.level, a.voices, a.detune, a.pwm, a.octave, a.tune, a.pan, a.sync ? "true" : "false", a.syncSemis, a.syncEnv, a.syncLfo, a.syncKey, a.syncWheel, a.cents, a.lfoPitchCents, a.fmSrc, a.fmAmt, a.amSrc, a.amAmt);
      std::printf(",\"smp\":\"%s\",\"smpPitch\":%g,\"smpVol\":%g}", kSmpIds[a.smp], a.smpPitch, a.smpVol); }
    std::printf("],\"p\":{\"subLevel\":%g,\"subWave\":\"%s\",\"subOctave\":%d,\"subPreFilter\":%s,\"mono\":%s,\"glide\":%g,\"filterMode\":\"%s\",\"cutoff\":%g,\"reso\":%g,\"drive\":%g,\"envAmt\":%g,\"keyTrack\":%g,\"lowCutHz\":%g,\"dist\":%g,\"distPre\":%s,\"noiseLevel\":%g,\"noiseHpHz\":%g,",
      p.subLevel, W[(int)p.subWave], p.subOctave, p.subPreFilter ? "true" : "false", p.mono ? "true" : "false", p.glide, FM(p.filterMode), p.cutoff, p.reso, p.drive, p.envAmt, p.keyTrack, p.lowCutHz, p.dist, p.distPre ? "true" : "false", p.noiseLevel, p.noiseHpHz);
    std::printf("\"ampCA\":%g,\"ampCD\":%g,\"ampCR\":%g,\"fenvCA\":%g,\"fenvCD\":%g,\"fenvCR\":%g,\"ampA\":%g,\"ampD\":%g,\"ampS\":%g,\"ampR\":%g,\"fenvA\":%g,\"fenvD\":%g,\"fenvS\":%g,\"fenvR\":%g,", p.amp.ca, p.amp.cd, p.amp.cr, p.fenv.ca, p.fenv.cd, p.fenv.cr, p.amp.a, p.amp.d, p.amp.s, p.amp.r, p.fenv.a, p.fenv.d, p.fenv.s, p.fenv.r);
    std::printf("\"pitchAmt\":%g,\"pitchTime\":%g,\"pitchAttack\":%g,\"pitchFall\":%g,\"pitchFallTime\":%g,\"lfoRate\":%g,\"lfoPwm\":%g,\"lfoPitch\":%g,\"lfoCutoff\":%g,\"lfoDetune\":%g,\"lfoShape\":%d,\"lfoPan\":%g,\"lfoAmp\":%g,\"wheelCutoff\":%g,\"phaseReset\":%s,\"drift\":%g,\"detuneRange\":%g,\"spread\":%g,\"bassMonoHz\":%g,",
      p.pitchAmt, p.pitchTime, p.pitchAttack, p.pitchFall, p.pitchFallTime, p.lfoRate, p.lfoPwm, p.lfoPitch, p.lfoCutoff, p.lfoDetune, p.lfoShape, p.lfoPan, p.lfoAmp, p.wheelCutoff, p.phaseReset ? "true" : "false", p.drift, p.detuneRange, p.spread, p.bassMonoHz);
    std::printf("\"chorusMix\":%g,\"chorusDepth\":%g,\"chorusRate\":%g,\"delayMix\":%g,\"delayTime\":%g,\"delayFb\":%g,\"reverbMix\":%g,\"reverbSize\":%g,\"reverbWidth\":%g,\"reverbLowCutHz\":%g,\"master\":%g,\"clipKnee\":%g,\"limiter\":%s,\"modWheel\":%g,\"stackCount\":%d,\"stackCents\":%g,\"velCutoff\":%g,\"wheelVib\":%g,\"delaySync\":%s,\"delayDiv\":%d,\"bpm\":%g,\"optoAmt\":%g,\"ottDepth\":%g,\"ottMix\":%g,\"sqzAmt\":%g,\"limCeil\":%g,\"polyphony\":%d,\"dmMix\":%g,\"dmDrive\":%g,\"dmClip\":%g,\"pitchHold\":%g,\"pitchCurve\":%g",
      p.chorusMix, p.chorusDepth, p.chorusRate, p.delayMix, p.delayTime, p.delayFb, p.reverbMix, p.reverbSize, p.reverbWidth, p.reverbLowCutHz, p.master, p.clipKnee, p.limiter ? "true" : "false", p.modWheel, p.stackCount, p.stackCents, p.velCutoff, p.wheelVib, p.delaySync ? "true" : "false", p.delayDiv, p.bpm, p.optoAmt, p.ottDepth, p.ottMix, p.sqzAmt, p.limCeil, p.polyphony, p.dmMix, p.dmDrive, p.dmClip, p.pitchHold, p.pitchCurve);
    std::printf(",\"dmLow\":%g,\"dmHigh\":%g,\"dynMix\":%g,\"chorusDelay\":%g,\"chorusWidth\":%g,\"delayDamp\":%g,\"delayPing\":%g,\"reverbPre\":%g,\"reverbDamp\":%g,\"optoGain\":%g,\"optoRel\":%g,\"ottLow\":%g,\"ottMid\":%g,\"ottHigh\":%g,\"ottTime\":%g,\"sqzAtk\":%g,\"sqzRel\":%g,\"sqzMakeup\":%g,\"menvTime\":%g,\"menvLoop\":%s,\"menvInv\":%s,\"lfoDepth\":%g,\"lfoFade\":%g,\"lfoStart\":%g,\"drvDrive\":%g,\"drvXs\":%g,\"drvFuzz\":%g,\"drvMix\":%g,\"eq1On\":%s,\"eq1F\":%g,\"eq2On\":%s,\"eq2F\":%g,\"eq2G\":%g,\"eq3On\":%s,\"eq3F\":%g,\"eq3G\":%g,\"eq3Q\":%g,\"eq4On\":%s,\"eq4F\":%g,\"eq4G\":%g,\"eq4Q\":%g,\"eq5On\":%s,\"eq5F\":%g,\"eq5G\":%g,\"eq6On\":%s,\"eq6F\":%g",
      p.dmLow, p.dmHigh, p.dynMix, p.chorusDelay, p.chorusWidth, p.delayDamp, p.delayPing, p.reverbPre, p.reverbDamp, p.optoGain, p.optoRel, p.ottLow, p.ottMid, p.ottHigh, p.ottTime, p.sqzAtk, p.sqzRel, p.sqzMakeup, p.menvTime, p.menvLoop ? "true" : "false", p.menvInv ? "true" : "false", p.lfoDepth, p.lfoFade, p.lfoStart, p.drvDrive, p.drvXs, p.drvFuzz, p.drvMix, p.eq1On ? "true" : "false", p.eq1F, p.eq2On ? "true" : "false", p.eq2F, p.eq2G, p.eq3On ? "true" : "false", p.eq3F, p.eq3G, p.eq3Q, p.eq4On ? "true" : "false", p.eq4F, p.eq4G, p.eq4Q, p.eq5On ? "true" : "false", p.eq5F, p.eq5G, p.eq6On ? "true" : "false", p.eq6F);
    { // keys added with the big build: three LFOs, three drawn envelopes, per layer envelopes, filters 2 and 3, samples, crusher, width, build macro, EQ dynamics
      auto F = [&](const char* k, double v) { std::printf(",\"%s\":%g", k, v); }; auto Bo = [&](const char* k, bool v) { std::printf(",\"%s\":%s", k, B(v)); }; auto St = [&](const char* k, const char* v) { std::printf(",\"%s\":\"%s\"", k, v); };
      F("highCutHz", p.highCutHz); F("auxSlope", p.auxSlope); Bo("ampLink", p.ampLink); F("fRoute", p.fRoute); F("build", p.build); F("modFxType", p.modFxType); F("chorusFb", p.chorusFb);
      F("crushBits", p.crushBits); F("crushRate", p.crushRate); F("crushMix", p.crushMix); F("widMid", p.widMid); F("widHi", p.widHi); F("msMid", p.msMid); F("msSide", p.msSide);
      St("f2Mode", FM(p.f2Mode)); F("f2Cut", p.f2Cut); F("f2Res", p.f2Res); F("f2Drv", p.f2Drv); F("f2Env", p.f2Env); F("f2Key", p.f2Key);
      St("f3Mode", FM(p.f3Mode)); F("f3Cut", p.f3Cut); F("f3Res", p.f3Res); F("f3Drv", p.f3Drv); F("f3Env", p.f3Env); F("f3Key", p.f3Key);
      F("lfo2Rate", p.lfo2Rate); F("lfo2Shape", p.lfo2Shape); F("lfo2Depth", p.lfo2Depth); F("lfo2Fade", p.lfo2Fade); F("lfo2Start", p.lfo2Start);
      F("lfo3Rate", p.lfo3Rate); F("lfo3Shape", p.lfo3Shape); F("lfo3Depth", p.lfo3Depth); F("lfo3Fade", p.lfo3Fade); F("lfo3Start", p.lfo3Start);
      F("dmMid", p.dmMid); Bo("menvSync", p.menvSync); F("menvDiv", p.menvDiv); Bo("menv2Sync", p.menv2Sync); F("menv2Div", p.menv2Div); Bo("menv3Sync", p.menv3Sync); F("menv3Div", p.menv3Div); F("menv2Time", p.menv2Time); Bo("menv2Loop", p.menv2Loop); Bo("menv2Inv", p.menv2Inv); F("menv3Time", p.menv3Time); Bo("menv3Loop", p.menv3Loop); Bo("menv3Inv", p.menv3Inv);
      St("nzSmp", kSmpIds[p.nzSmp]); F("nzSmpPitch", p.nzSmpPitch); F("nzSmpVol", p.nzSmpVol); Bo("smpPost", p.smpPost);
      const Adsr* E[4] = {&p.amp2, &p.amp3, &p.fenv2, &p.fenv3}; const char* EN[4] = {"amp2", "amp3", "fenv2", "fenv3"};
      for (int i = 0; i < 4; ++i) { char k[24]; const Adsr& e = *E[i]; const double v[7] = {e.a, e.d, e.s, e.r, e.ca, e.cd, e.cr}; const char* sfx[7] = {"A", "D", "S", "R", "CA", "CD", "CR"}; for (int j = 0; j < 7; ++j) { std::snprintf(k, sizeof k, "%s%s", EN[i], sfx[j]); F(k, v[j]); } }
      
      #define EQX(n) F("eq" #n "Dyn", p.eq##n##Dyn); F("eq" #n "Thr", p.eq##n##Thr); F("eq" #n "Atk", p.eq##n##Atk); F("eq" #n "Rel", p.eq##n##Rel); Bo("eq" #n "Below", p.eq##n##Below); F("eq" #n "Sat", p.eq##n##Sat); F("eq" #n "SatT", p.eq##n##SatT);
      EQX(2) EQX(3) EQX(4) EQX(5)
      #undef EQX
      Params dflt; const float* M[3] = {p.menv, p.menv2, p.menv3}; const char* MN[3] = {"menvData", "menv2Data", "menv3Data"};
      for (int m = 0; m < 3; ++m) { bool same = true; for (int i = 0; i < 64; ++i) if (M[m][i] != dflt.menv[i]) same = false;
        if (!same) { std::printf(",\"%s\":\"", MN[m]); for (int i = 0; i < 64; ++i) std::printf("%02x", (int)(std::min(1.f, std::max(0.f, M[m][i])) * 255.f + 0.5f)); std::printf("\""); } }
      for (int s = 0; s < kMxSrc; ++s) for (int q = 0; q < kMxDst; ++q) if (p.mx[s][q] != 0.f) std::printf(",\"mx_%s_%s\":%g", kMxSrcNames[s], kMxDstNames[q], p.mx[s][q]); }
    std::printf("}}");
  }
  std::printf("\n}\n"); return 0; }
