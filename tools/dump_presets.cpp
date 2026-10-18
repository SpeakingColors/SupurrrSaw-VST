// Writes every built-in preset as JSON (used by the browser playground so it plays exactly the same settings).
// Usage: dump_presets > presets.json
#include "../presets/presets.h"
#include <cstdio>
using namespace supurr;
static const char* W[] = {"Off", "SuperSaw", "SuperSquare", "Saw", "Pulse", "Tri", "Sine", "Sample"};
static const char* FM[] = {"LP24", "LP18", "HP", "BP"};
int main() {
  const char* names[] = {"hoover", "hoover_classic", "hoover_prodigy", "pick", "pluck", "trance_lead", "talking_lead", "daft_funk", "trance_bass", "funk_bass", "freeway_bass",
                         "better_off_alone", "gimme_lead", "levels_lead", "sandstorm_lead", "gigi_lead", "alexf_lead", "mgmt_lead", "pendulum_lead", "nightcall_keys"};
  std::printf("{\n"); bool first = true;
  for (const char* n : names) {
    Params p; if (!presets::byName(n, p)) { std::fprintf(stderr, "missing %s\n", n); return 1; }
    std::printf("%s\"%s\": {\"layers\":[", first ? "" : ",\n", n); first = false;
    for (int l = 0; l < kLayers; ++l) { const LayerParams& a = p.layer[l];
      std::printf("%s{\"wave\":\"%s\",\"level\":%g,\"voices\":%d,\"detune\":%g,\"pwm\":%g,\"octave\":%d,\"tune\":%g,\"pan\":%g,\"sync\":%s,\"syncSemis\":%g,\"syncEnv\":%g,\"syncLfo\":%g,\"syncKey\":%g,\"syncWheel\":%g,\"cents\":%g,\"lfoPitchCents\":%g}",
        l ? "," : "", W[(int)a.wave], a.level, a.voices, a.detune, a.pwm, a.octave, a.tune, a.pan, a.sync ? "true" : "false", a.syncSemis, a.syncEnv, a.syncLfo, a.syncKey, a.syncWheel, a.cents, a.lfoPitchCents); }
    std::printf("],\"p\":{\"subLevel\":%g,\"subWave\":\"%s\",\"subOctave\":%d,\"subPreFilter\":%s,\"mono\":%s,\"glide\":%g,\"filterMode\":\"%s\",\"cutoff\":%g,\"reso\":%g,\"drive\":%g,\"envAmt\":%g,\"keyTrack\":%g,\"lowCutHz\":%g,\"dist\":%g,\"distPre\":%s,\"noiseLevel\":%g,\"noiseHpHz\":%g,",
      p.subLevel, W[(int)p.subWave], p.subOctave, p.subPreFilter ? "true" : "false", p.mono ? "true" : "false", p.glide, FM[(int)p.filterMode], p.cutoff, p.reso, p.drive, p.envAmt, p.keyTrack, p.lowCutHz, p.dist, p.distPre ? "true" : "false", p.noiseLevel, p.noiseHpHz);
    std::printf("\"ampA\":%g,\"ampD\":%g,\"ampS\":%g,\"ampR\":%g,\"fenvA\":%g,\"fenvD\":%g,\"fenvS\":%g,\"fenvR\":%g,", p.amp.a, p.amp.d, p.amp.s, p.amp.r, p.fenv.a, p.fenv.d, p.fenv.s, p.fenv.r);
    std::printf("\"pitchAmt\":%g,\"pitchTime\":%g,\"pitchAttack\":%g,\"pitchFall\":%g,\"pitchFallTime\":%g,\"lfoRate\":%g,\"lfoPwm\":%g,\"lfoPitch\":%g,\"lfoCutoff\":%g,\"lfoDetune\":%g,\"drift\":%g,\"detuneRange\":%g,\"spread\":%g,\"bassMonoHz\":%g,",
      p.pitchAmt, p.pitchTime, p.pitchAttack, p.pitchFall, p.pitchFallTime, p.lfoRate, p.lfoPwm, p.lfoPitch, p.lfoCutoff, p.lfoDetune, p.drift, p.detuneRange, p.spread, p.bassMonoHz);
    std::printf("\"chorusMix\":%g,\"chorusDepth\":%g,\"chorusRate\":%g,\"delayMix\":%g,\"delayTime\":%g,\"delayFb\":%g,\"reverbMix\":%g,\"reverbSize\":%g,\"reverbWidth\":%g,\"reverbLowCutHz\":%g,\"master\":%g,\"modWheel\":%g}}",
      p.chorusMix, p.chorusDepth, p.chorusRate, p.delayMix, p.delayTime, p.delayFb, p.reverbMix, p.reverbSize, p.reverbWidth, p.reverbLowCutHz, p.master, p.modWheel);
  }
  std::printf("\n}\n"); return 0; }
