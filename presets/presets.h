// Built-in SupurrSaw presets as plain C++ (the plugin will load the same values from files later).
// Each function fills a Params. Written from the author's descriptions of how he builds these sounds; to be tuned by ear.
#pragma once
#include <string>
#include "../core/supurrsaw_core.h"

namespace supurr::presets {

// Hoover: a saw and a square on different octaves, detuned, a pitch dive, PWM on the square,
// a fast LFO on the saw detune, then saturation and a little chorus/reverb to thicken.
inline void hoover(Params& p) {
  p = Params();
  // three layers: two detuned super saws (one an octave down) and a super square on a different octave, plus a sub for weight
  p.layer[0].wave = Wave::SuperSaw;    p.layer[0].voices = 9; p.layer[0].detune = 0.65f; p.layer[0].octave = 0;  p.layer[0].level = 0.55f;
  p.layer[1].wave = Wave::SuperSaw;    p.layer[1].voices = 7; p.layer[1].detune = 0.5f;  p.layer[1].octave = -1; p.layer[1].level = 0.55f; p.layer[1].tune = 9.f;
  p.layer[2].wave = Wave::SuperSquare; p.layer[2].voices = 5; p.layer[2].detune = 0.4f;  p.layer[2].octave = -1; p.layer[2].level = 0.5f; p.layer[2].pwm = 0.5f;
  p.subLevel = 0.3f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.detuneRange = 1.4f; p.lfoRate = 6.5f; p.lfoPwm = 0.7f; p.lfoDetune = 0.35f;
  p.pitchAmt = 12.f; p.pitchTime = 0.09f;
  p.cutoff = 4600.f; p.reso = 0.3f; p.drive = 0.28f; p.dist = 0.22f; p.lowCutHz = 45.f;   // thick but not smashed
  p.master = 0.9f; p.amp = {0.002f, 0.5f, 0.85f, 0.25f}; p.spread = 0.85f; p.chorusMix = 0.4f; p.reverbMix = 0.03f; p.reverbSize = 0.5f;
}

// Pick (makina / Afrojack style blip, like a synth water bubble): a square and a saw, pitch rising into the note
// (negative pitch envelope), a resonant low-pass that moves up while it speaks, distortion before the filter.
inline void pick(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.45f; p.layer[0].level = 0.5f;
  p.layer[1].wave = Wave::Saw;   p.layer[1].level = 0.35f; p.layer[1].tune = 6.f;
  p.pitchAmt = -7.f; p.pitchTime = 0.07f;
  p.filterMode = FilterMode::LP24; p.cutoff = 500.f; p.reso = 0.55f; p.drive = 0.3f; p.envAmt = 0.5f; p.fenv = {0.001f, 0.2f, 0.f, 0.1f};
  p.dist = 0.5f; p.distPre = true; p.lowCutHz = 100.f;
  p.amp = {0.001f, 0.32f, 0.f, 0.15f}; p.spread = 0.3f; p.delayMix = 0.12f; p.delayTime = 0.18f; p.delayFb = 0.3f;
}

// Pluck: short and tight. The pitch envelope is only a tiny transient at the start.
inline void pluck(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Saw;      p.layer[0].level = 0.45f;
  p.layer[1].wave = Wave::Pulse;    p.layer[1].level = 0.35f; p.layer[1].tune = 7.f; p.layer[1].pwm = 0.4f;
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 9; p.layer[2].detune = 0.55f; p.layer[2].level = 0.4f;   // fat detuned stack
  p.subLevel = 0.2f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.pitchAmt = 1.5f; p.pitchTime = 0.008f;
  p.cutoff = 700.f; p.reso = 0.25f; p.envAmt = 0.6f; p.fenv = {0.001f, 0.22f, 0.f, 0.1f}; p.drive = 0.2f; p.dist = 0.08f;
  p.master = 0.8f; p.amp = {0.001f, 0.25f, 0.f, 0.1f}; p.spread = 0.85f; p.chorusMix = 0.2f; p.reverbMix = 0.1f;
}

// Trance lead: super saws detuned hard, very light high-passed noise behind, filter envelope.
// Low cutoff = plucky, open cutoff = sustained lead (short attack, long decay, high sustain, long release).
inline void tranceLead(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 15; p.layer[0].detune = 0.92f; p.layer[0].level = 0.55f;                   // main stack
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 15; p.layer[1].detune = 0.8f;  p.layer[1].octave = -1; p.layer[1].level = 0.4f;   // sub-octave stack for body
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 9;  p.layer[2].detune = 0.75f; p.layer[2].octave = 1;  p.layer[2].level = 0.2f;   // air on top
  p.detuneRange = 1.3f; p.noiseLevel = 0.09f; p.noiseHpHz = 5000.f;
  p.cutoff = 2400.f; p.reso = 0.15f; p.drive = 0.3f; p.dist = 0.12f; p.envAmt = 0.45f; p.fenv = {0.004f, 0.6f, 0.55f, 0.5f}; p.lowCutHz = 90.f;
  p.master = 0.6f; p.amp = {0.004f, 0.8f, 0.85f, 0.45f}; p.spread = 1.f; p.chorusMix = 0.3f; p.delayMix = 0.2f; p.delayTime = 0.28f; p.delayFb = 0.35f; p.reverbMix = 0.2f; p.reverbSize = 0.7f;
}

// Talking lead (Daft Punk style): a sync oscillator whose slave pitch is swept by an LFO and the envelope, follows the
// note played (higher notes open the sync more) and the mod wheel, so it "talks". Saturated, almost dry.
inline void talkingLead(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Saw; p.layer[0].sync = true; p.layer[0].syncSemis = 10.f; p.layer[0].syncLfo = 9.f; p.layer[0].syncEnv = 8.f;
  p.layer[0].syncKey = 5.f; p.layer[0].syncWheel = 12.f; p.layer[0].level = 0.55f;
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 9; p.layer[1].detune = 0.45f; p.layer[1].level = 0.45f; p.layer[1].octave = -1;   // sub-octave body
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 9; p.layer[2].detune = 0.6f;  p.layer[2].level = 0.3f;                          // fat unison at pitch
  p.lfoRate = 4.5f; p.cutoff = 5000.f; p.reso = 0.2f; p.envAmt = 0.2f; p.fenv = {0.002f, 0.4f, 0.6f, 0.3f}; p.drive = 0.25f; p.dist = 0.5f;
  p.master = 0.75f; p.amp = {0.003f, 0.6f, 0.85f, 0.25f}; p.spread = 0.8f; p.chorusMix = 0.2f; p.delayMix = 0.08f; p.reverbMix = 0.02f;
}

// Rolling trance bass: saw + sub, short and punchy, mono and centred, sits under offbeat patterns.
inline void tranceBass(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Saw; p.layer[0].level = 0.55f;
  p.layer[1].wave = Wave::Pulse; p.layer[1].pwm = 0.45f; p.layer[1].level = 0.3f; p.layer[1].tune = 6.f;
  p.subLevel = 0.5f; p.subWave = Wave::Sine; p.subOctave = -1;
  p.cutoff = 380.f; p.reso = 0.25f; p.envAmt = 0.55f; p.fenv = {0.001f, 0.16f, 0.1f, 0.08f}; p.drive = 0.3f; p.dist = 0.15f; p.lowCutHz = 28.f;
  p.amp = {0.002f, 0.2f, 0.55f, 0.07f}; p.spread = 0.f; p.bassMonoHz = 200.f; p.master = 0.7f;
}

// Funky / groovy bass: square + saw, resonant filter with a quick envelope pluck, triangle sub, tracks the keyboard.
inline void funkBass(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.35f; p.layer[0].level = 0.5f;
  p.layer[1].wave = Wave::Saw; p.layer[1].level = 0.4f; p.layer[1].octave = -1;
  p.subLevel = 0.35f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.filterMode = FilterMode::LP24; p.cutoff = 250.f; p.reso = 0.45f; p.envAmt = 0.75f; p.keyTrack = 0.4f; p.fenv = {0.001f, 0.2f, 0.05f, 0.1f}; p.drive = 0.35f; p.dist = 0.2f; p.lowCutHz = 30.f;
  p.amp = {0.001f, 0.28f, 0.35f, 0.08f}; p.spread = 0.f; p.bassMonoHz = 220.f; p.master = 0.7f;
}

inline bool byName(const std::string& n, Params& p) {
  if (n == "hoover") hoover(p); else if (n == "pick") pick(p); else if (n == "pluck") pluck(p); else if (n == "trance_lead") tranceLead(p);
  else if (n == "talking_lead") talkingLead(p); else if (n == "trance_bass") tranceBass(p); else if (n == "funk_bass") funkBass(p); else return false;
  return true;
}

}  // namespace supurr::presets
