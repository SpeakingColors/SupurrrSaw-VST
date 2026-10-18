// Built-in SupurrSaw presets as plain C++ (the plugin will load the same values from files later).
// Each function fills a Params. Written from the author's descriptions of how he builds these sounds; to be tuned by ear.
#pragma once
#include "../core/supurrsaw_core.h"

namespace supurr::presets {

// Hoover: a saw and a square on different octaves, detuned, a pitch dive, PWM on the square,
// a fast LFO on the saw detune, then saturation and a little chorus/reverb to thicken.
inline void hoover(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw;    p.layer[0].voices = 5; p.layer[0].detune = 0.55f; p.layer[0].octave = 0;  p.layer[0].level = 0.5f;
  p.layer[1].wave = Wave::SuperSquare; p.layer[1].voices = 3; p.layer[1].detune = 0.35f; p.layer[1].octave = -1; p.layer[1].level = 0.5f; p.layer[1].pwm = 0.5f;
  p.detuneRange = 1.3f; p.lfoRate = 6.5f; p.lfoPwm = 0.7f; p.lfoDetune = 0.35f;
  p.pitchAmt = 12.f; p.pitchTime = 0.09f;
  p.cutoff = 3000.f; p.reso = 0.35f; p.drive = 0.15f; p.dist = 0.3f; p.lowCutHz = 90.f;
  p.master = 0.6f; p.amp = {0.002f, 0.5f, 0.8f, 0.25f}; p.spread = 0.8f; p.chorusMix = 0.25f; p.reverbMix = 0.1f; p.reverbSize = 0.6f;
}

// Pick (makina / Afrojack style blip): sine + triangle, the pitch envelope IS the sound,
// band-pass filter, distortion before the filter so it screeches.
inline void pick(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Tri;  p.layer[0].level = 0.6f;
  p.layer[1].wave = Wave::Sine; p.layer[1].level = 0.5f; p.layer[1].octave = 1;
  p.pitchAmt = 12.f; p.pitchTime = 0.04f;
  p.filterMode = FilterMode::BP; p.cutoff = 1800.f; p.reso = 0.55f; p.drive = 0.3f;
  p.dist = 0.7f; p.distPre = true; p.lowCutHz = 120.f;
  p.amp = {0.001f, 0.12f, 0.f, 0.06f}; p.spread = 0.2f; p.delayMix = 0.12f; p.delayTime = 0.18f; p.delayFb = 0.3f;
}

// Pluck: short and tight. The pitch envelope is only a tiny transient at the start.
inline void pluck(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Saw;   p.layer[0].level = 0.5f;
  p.layer[1].wave = Wave::Pulse; p.layer[1].level = 0.4f; p.layer[1].tune = 7.f; p.layer[1].pwm = 0.4f;
  p.pitchAmt = 1.5f; p.pitchTime = 0.008f;
  p.cutoff = 700.f; p.reso = 0.25f; p.envAmt = 0.6f; p.fenv = {0.001f, 0.22f, 0.f, 0.1f};
  p.amp = {0.001f, 0.25f, 0.f, 0.1f}; p.spread = 0.6f; p.reverbMix = 0.1f;
}

// Trance lead: super saws detuned hard, very light high-passed noise behind, filter envelope.
// Low cutoff = plucky, open cutoff = sustained lead (short attack, long decay, high sustain, long release).
inline void tranceLead(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 15; p.layer[0].detune = 0.85f; p.layer[0].level = 0.6f;
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 9;  p.layer[1].detune = 0.7f;  p.layer[1].octave = 1; p.layer[1].level = 0.25f;
  p.detuneRange = 1.2f; p.noiseLevel = 0.05f; p.noiseHpHz = 4000.f;
  p.cutoff = 2200.f; p.reso = 0.15f; p.envAmt = 0.45f; p.fenv = {0.004f, 0.6f, 0.55f, 0.5f}; p.lowCutHz = 120.f;
  p.amp = {0.004f, 0.8f, 0.85f, 0.45f}; p.spread = 1.f; p.chorusMix = 0.25f; p.delayMix = 0.2f; p.delayTime = 0.28f; p.delayFb = 0.35f; p.reverbMix = 0.2f; p.reverbSize = 0.7f;
}

// Talking lead (Daft Punk style): a sync oscillator whose slave pitch is swept by an LFO and the envelope, so it "talks".
inline void talkingLead(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Saw; p.layer[0].sync = true; p.layer[0].syncSemis = 7.f; p.layer[0].syncLfo = 5.f; p.layer[0].syncEnv = 4.f; p.layer[0].level = 0.6f;
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 5; p.layer[1].detune = 0.3f; p.layer[1].level = 0.25f; p.layer[1].octave = -1;
  p.lfoRate = 4.5f; p.cutoff = 5000.f; p.reso = 0.2f; p.envAmt = 0.2f; p.fenv = {0.002f, 0.4f, 0.6f, 0.3f}; p.dist = 0.2f;
  p.master = 0.55f; p.amp = {0.003f, 0.6f, 0.85f, 0.25f}; p.spread = 0.5f; p.chorusMix = 0.2f; p.delayMix = 0.15f; p.reverbMix = 0.12f;
}

}  // namespace supurr::presets
