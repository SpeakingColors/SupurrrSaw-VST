// Writes every built-in preset as JSON (used by the browser playground so it plays exactly the same settings).
// Usage: dump_presets > presets.json
#include "../presets/presets.h"
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
using namespace supurr;
static const char* W[] = {"Off", "SuperSaw", "SuperSquare", "Saw", "Pulse", "Tri", "Sine", "Sample", "Custom"};
static const char* FM[] = {"LP24", "LP18", "HP", "BP", "LP12", "LP6"};
int main() {
  const char* names[] = {"hoover", "hoover_classic", "hoover_prodigy", "pick", "pluck", "trance_lead", "trance_hybrid", "trance_90s", "talking_lead", "daft_funk", "trance_bass", "funk_bass", "freeway_bass",
                         "better_off_alone", "gimme_lead", "levels_lead", "sandstorm_lead", "gigi_lead", "alexf_lead", "mgmt_lead", "pendulum_lead", "nightcall_keys", "brejcha_bass", "feellove_bass", "nboy_303", "spaghetti_bass", "access_bass", "epiano_dx", "epiano_wurly", "piano", "felt_piano", "organ", "clav", "strings", "warm_pad", "glass_pad", "choir", "bell", "brass", "kick", "snare", "closed_hat", "open_hat", "crash"};
  std::printf("{\n"); bool first = true;
  std::vector<std::string> all(std::begin(names), std::end(names)); { int rn; const presets::RaveEntry* RL = presets::raveList(rn); for (int i = 0; i < rn; ++i) all.push_back(RL[i].name); }
  for (const std::string& n : all) {
    Params p; if (!presets::byName(n, p)) { std::fprintf(stderr, "missing %s\n", n.c_str()); return 1; }
    std::printf("%s\"%s\": {\"layers\":[", first ? "" : ",\n", n.c_str()); first = false;
    for (int l = 0; l < kLayers; ++l) { const LayerParams& a = p.layer[l];
      std::printf("%s{\"wave\":\"%s\",\"level\":%g,\"voices\":%d,\"detune\":%g,\"pwm\":%g,\"octave\":%d,\"tune\":%g,\"pan\":%g,\"sync\":%s,\"syncSemis\":%g,\"syncEnv\":%g,\"syncLfo\":%g,\"syncKey\":%g,\"syncWheel\":%g,\"cents\":%g,\"lfoPitchCents\":%g,\"fmSrc\":%d,\"fmAmt\":%g,\"amSrc\":%d,\"amAmt\":%g}",
        l ? "," : "", W[(int)a.wave], a.level, a.voices, a.detune, a.pwm, a.octave, a.tune, a.pan, a.sync ? "true" : "false", a.syncSemis, a.syncEnv, a.syncLfo, a.syncKey, a.syncWheel, a.cents, a.lfoPitchCents, a.fmSrc, a.fmAmt, a.amSrc, a.amAmt); }
    std::printf("],\"p\":{\"subLevel\":%g,\"subWave\":\"%s\",\"subOctave\":%d,\"subPreFilter\":%s,\"mono\":%s,\"glide\":%g,\"filterMode\":\"%s\",\"cutoff\":%g,\"reso\":%g,\"drive\":%g,\"envAmt\":%g,\"keyTrack\":%g,\"lowCutHz\":%g,\"dist\":%g,\"distPre\":%s,\"noiseLevel\":%g,\"noiseHpHz\":%g,",
      p.subLevel, W[(int)p.subWave], p.subOctave, p.subPreFilter ? "true" : "false", p.mono ? "true" : "false", p.glide, FM[(int)p.filterMode], p.cutoff, p.reso, p.drive, p.envAmt, p.keyTrack, p.lowCutHz, p.dist, p.distPre ? "true" : "false", p.noiseLevel, p.noiseHpHz);
    std::printf("\"ampCA\":%g,\"ampCD\":%g,\"ampCR\":%g,\"fenvCA\":%g,\"fenvCD\":%g,\"fenvCR\":%g,\"ampA\":%g,\"ampD\":%g,\"ampS\":%g,\"ampR\":%g,\"fenvA\":%g,\"fenvD\":%g,\"fenvS\":%g,\"fenvR\":%g,", p.amp.ca, p.amp.cd, p.amp.cr, p.fenv.ca, p.fenv.cd, p.fenv.cr, p.amp.a, p.amp.d, p.amp.s, p.amp.r, p.fenv.a, p.fenv.d, p.fenv.s, p.fenv.r);
    std::printf("\"pitchAmt\":%g,\"pitchTime\":%g,\"pitchAttack\":%g,\"pitchFall\":%g,\"pitchFallTime\":%g,\"lfoRate\":%g,\"lfoPwm\":%g,\"lfoPitch\":%g,\"lfoCutoff\":%g,\"lfoDetune\":%g,\"lfoShape\":%d,\"lfoPan\":%g,\"lfoAmp\":%g,\"wheelCutoff\":%g,\"phaseReset\":%s,\"drift\":%g,\"detuneRange\":%g,\"spread\":%g,\"bassMonoHz\":%g,",
      p.pitchAmt, p.pitchTime, p.pitchAttack, p.pitchFall, p.pitchFallTime, p.lfoRate, p.lfoPwm, p.lfoPitch, p.lfoCutoff, p.lfoDetune, p.lfoShape, p.lfoPan, p.lfoAmp, p.wheelCutoff, p.phaseReset ? "true" : "false", p.drift, p.detuneRange, p.spread, p.bassMonoHz);
    std::printf("\"chorusMix\":%g,\"chorusDepth\":%g,\"chorusRate\":%g,\"delayMix\":%g,\"delayTime\":%g,\"delayFb\":%g,\"reverbMix\":%g,\"reverbSize\":%g,\"reverbWidth\":%g,\"reverbLowCutHz\":%g,\"master\":%g,\"clipKnee\":%g,\"limiter\":%s,\"modWheel\":%g,\"stackCount\":%d,\"stackCents\":%g,\"velCutoff\":%g,\"wheelVib\":%g,\"delaySync\":%s,\"delayDiv\":%d,\"bpm\":%g,\"optoAmt\":%g,\"ottDepth\":%g,\"ottMix\":%g,\"sqzAmt\":%g,\"limCeil\":%g,\"polyphony\":%d,\"dmMix\":%g,\"dmDrive\":%g,\"dmClip\":%g,\"pitchHold\":%g,\"pitchCurve\":%g",
      p.chorusMix, p.chorusDepth, p.chorusRate, p.delayMix, p.delayTime, p.delayFb, p.reverbMix, p.reverbSize, p.reverbWidth, p.reverbLowCutHz, p.master, p.clipKnee, p.limiter ? "true" : "false", p.modWheel, p.stackCount, p.stackCents, p.velCutoff, p.wheelVib, p.delaySync ? "true" : "false", p.delayDiv, p.bpm, p.optoAmt, p.ottDepth, p.ottMix, p.sqzAmt, p.limCeil, p.polyphony, p.dmMix, p.dmDrive, p.dmClip, p.pitchHold, p.pitchCurve);
    std::printf(",\"dmLow\":%g,\"dmHigh\":%g,\"dynMix\":%g,\"chorusDelay\":%g,\"chorusWidth\":%g,\"delayDamp\":%g,\"delayPing\":%g,\"reverbPre\":%g,\"reverbDamp\":%g,\"optoGain\":%g,\"optoRel\":%g,\"ottLow\":%g,\"ottMid\":%g,\"ottHigh\":%g,\"ottTime\":%g,\"sqzAtk\":%g,\"sqzRel\":%g,\"sqzMakeup\":%g,\"menvTime\":%g,\"menvLoop\":%s,\"menvInv\":%s,\"lfoDepth\":%g,\"lfoFade\":%g,\"lfoStart\":%g",
      p.dmLow, p.dmHigh, p.dynMix, p.chorusDelay, p.chorusWidth, p.delayDamp, p.delayPing, p.reverbPre, p.reverbDamp, p.optoGain, p.optoRel, p.ottLow, p.ottMid, p.ottHigh, p.ottTime, p.sqzAtk, p.sqzRel, p.sqzMakeup, p.menvTime, p.menvLoop ? "true" : "false", p.menvInv ? "true" : "false", p.lfoDepth, p.lfoFade, p.lfoStart);
    { static const char* S[] = {"lfo", "fenv", "aenv", "penv", "whl", "key", "vel", "rnd", "men"}; static const char* D[] = {"pitch", "cut", "res", "drv", "pwm", "det", "pan", "vol", "sy1", "sy2", "sy3", "sub", "nz", "lc"};
      for (int s = 0; s < 9; ++s) for (int q = 0; q < 14; ++q) if (p.mx[s][q] != 0.f) std::printf(",\"mx_%s_%s\":%g", S[s], D[q], p.mx[s][q]);
      Params dflt; bool same = true; for (int i = 0; i < 64; ++i) if (p.menv[i] != dflt.menv[i]) same = false;
      if (!same) { std::printf(",\"menvData\":\""); for (int i = 0; i < 64; ++i) std::printf("%02x", (int)(std::min(1.f, std::max(0.f, p.menv[i])) * 255.f + 0.5f)); std::printf("\""); } }
    std::printf("}}");
  }
  std::printf("\n}\n"); return 0; }
