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
  p.layer[0].wave = Wave::SuperSaw;    p.layer[0].voices = 15; p.layer[0].detune = 0.65f; p.layer[0].octave = 0;  p.layer[0].level = 0.6f;
  p.layer[1].wave = Wave::SuperSaw;    p.layer[1].voices = 11; p.layer[1].detune = 0.5f;  p.layer[1].octave = -1; p.layer[1].level = 0.65f; p.layer[1].tune = 9.f;
  p.layer[2].wave = Wave::SuperSquare; p.layer[2].voices = 7;  p.layer[2].detune = 0.4f;  p.layer[2].octave = -1; p.layer[2].level = 0.6f; p.layer[2].pwm = 0.5f;
  p.subLevel = 0.4f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.detuneRange = 1.4f; p.lfoRate = 6.5f; p.lfoPwm = 0.7f; p.lfoDetune = 0.35f;
  p.pitchAmt = 12.f; p.pitchTime = 0.09f;
  p.cutoff = 3500.f; p.reso = 0.3f; p.drive = 0.5f; p.dist = 0.14f; p.lowCutHz = 45.f;   // weight comes from the stacks and soft drive into the filter, not from hard distortion
  p.master = 0.52f; p.amp = {0.002f, 0.5f, 0.85f, 0.25f}; p.spread = 0.85f; p.chorusMix = 0.4f; p.reverbMix = 0.04f; p.reverbSize = 0.5f;
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
  p.master = 0.87f;
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
  p.master = 0.72f; p.amp = {0.001f, 0.25f, 0.f, 0.1f}; p.spread = 0.85f; p.chorusMix = 0.2f; p.reverbMix = 0.08f;
}

// Trance lead: super saws detuned hard, very light high-passed noise behind, filter envelope.
// Low cutoff = plucky, open cutoff = sustained lead (short attack, long decay, high sustain, long release).
inline void tranceLead(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 15; p.layer[0].detune = 0.92f; p.layer[0].level = 0.6f;                                  // main stack
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 15; p.layer[1].detune = 0.8f;  p.layer[1].octave = -1; p.layer[1].level = 0.5f;          // sub-octave stack for body
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 9;  p.layer[2].detune = 0.75f; p.layer[2].octave = 1;  p.layer[2].level = 0.1f;          // a little air on top
  p.subLevel = 0.18f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.detuneRange = 1.3f; p.noiseLevel = 0.09f; p.noiseHpHz = 5000.f;
  p.cutoff = 1900.f; p.reso = 0.12f; p.drive = 0.25f; p.dist = 0.05f; p.envAmt = 0.5f; p.fenv = {0.004f, 0.6f, 0.55f, 0.5f}; p.lowCutHz = 80.f;
  p.master = 0.96f; p.amp = {0.004f, 0.8f, 0.85f, 0.45f}; p.spread = 1.f; p.chorusMix = 0.3f; p.delayMix = 0.08f; p.delayTime = 0.28f; p.delayFb = 0.3f; p.reverbMix = 0.07f; p.reverbSize = 0.6f;
}

// Hybrid trance supersaw: a tight centre stack for the fundamental, two wide stacks detuned a few cents down and up,
// a high pitched filtered noise for air, low cut near 270 Hz so the kick and sub keep their room, a touch of tube saturation
// before the time effects, a ping-pong delay and a reverb that skips the low end.
inline void tranceHybrid(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 3; p.layer[0].detune = 0.15f; p.layer[0].level = 0.5f;                                      // core, stays clear
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 9; p.layer[1].cents = 22.f; p.layer[1].tune = -6.f; p.layer[1].level = 0.55f; p.layer[1].pan = -0.2f;
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 9; p.layer[2].cents = 22.f; p.layer[2].tune = 6.f;  p.layer[2].level = 0.55f; p.layer[2].pan = 0.2f;
  p.subLevel = 0.1f; p.subWave = Wave::Sine; p.subOctave = -1;
  p.noiseLevel = 0.05f; p.noiseHpHz = 8000.f;
  p.cutoff = 11000.f; p.reso = 0.08f; p.drive = 0.2f; p.dist = 0.06f; p.envAmt = 0.1f; p.fenv = {0.003f, 0.5f, 0.7f, 0.3f}; p.lowCutHz = 270.f;
  p.master = 0.9f; p.amp = {0.003f, 0.35f, 0.6f, 0.2f}; p.spread = 1.f;
  p.chorusMix = 0.25f; p.delayMix = 0.12f; p.delayTime = 0.214f; p.delayFb = 0.35f; p.delayPing = 1.f; p.reverbMix = 0.12f; p.reverbSize = 0.7f; p.reverbLowCutHz = 500.f;
}

// 90s hard trance saw: rawer and warmer, wide analog style detune, more drive, a slightly open filter.
inline void trance90s(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 7; p.layer[0].detune = 0.85f; p.layer[0].level = 0.6f;
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 9; p.layer[1].detune = 0.9f; p.layer[1].octave = -1; p.layer[1].tune = -5.f; p.layer[1].level = 0.35f;
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 9; p.layer[2].detune = 0.7f; p.layer[2].tune = 7.f; p.layer[2].level = 0.5f;
  p.cutoff = 6500.f; p.reso = 0.2f; p.drive = 0.4f; p.dist = 0.12f; p.envAmt = 0.3f; p.fenv = {0.003f, 0.5f, 0.5f, 0.3f}; p.lowCutHz = 200.f; p.drift = 0.3f;
  p.master = 0.9f; p.amp = {0.002f, 0.5f, 0.7f, 0.3f}; p.spread = 0.9f; p.chorusMix = 0.2f; p.delayMix = 0.1f; p.delayTime = 0.214f; p.delayFb = 0.3f; p.reverbMix = 0.1f; p.reverbSize = 0.65f; p.reverbLowCutHz = 400.f;
}

// ---- keys, pianos, pads and strings (approximations built from the synth's own parts; FM from a second layer makes the bell and tine sounds) ----
// DX style electric piano: sine carrier frequency modulated by a sine at the same pitch (mellow tine), a faint octave-up layer for the attack ping.
inline void epianoDx(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Sine; p.layer[0].level = 0.7f; p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.18f;
  p.layer[1].wave = Wave::Sine; p.layer[1].level = 0.f;                                                  // modulator only, silent
  p.layer[2].wave = Wave::Sine; p.layer[2].octave = 2; p.layer[2].level = 0.06f; p.layer[2].fmSrc = 2; p.layer[2].fmAmt = 0.1f;   // tine
  p.cutoff = 5200.f; p.reso = 0.05f; p.envAmt = 0.35f; p.keyTrack = 0.3f; p.velCutoff = 0.25f; p.fenv = {0.001f, 0.6f, 0.2f, 0.3f}; p.lowCutHz = 60.f;
  p.master = 0.9f; p.amp = {0.002f, 1.8f, 0.12f, 0.35f}; p.lfoRate = 4.2f; p.lfoPan = 0.18f; p.spread = 0.8f;
  p.chorusMix = 0.35f; p.chorusDepth = 0.35f; p.reverbMix = 0.12f; p.reverbSize = 0.6f; p.reverbLowCutHz = 250.f;
}
// Wurlitzer style: reedy pulse and triangle with a little drive and a quick bark on the attack.
inline void epianoWurly(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.38f; p.layer[0].level = 0.45f;
  p.layer[1].wave = Wave::Tri; p.layer[1].level = 0.5f; p.layer[1].octave = 0;
  p.cutoff = 2400.f; p.reso = 0.12f; p.envAmt = 0.45f; p.keyTrack = 0.35f; p.velCutoff = 0.3f; p.fenv = {0.001f, 0.35f, 0.15f, 0.25f}; p.drive = 0.15f; p.dist = 0.1f; p.lowCutHz = 70.f;
  p.master = 0.9f; p.amp = {0.002f, 1.2f, 0.1f, 0.3f}; p.lfoRate = 5.f; p.lfoAmp = 0.12f; p.chorusMix = 0.2f; p.reverbMix = 0.1f;
}
// Piano: a bright saw and a triangle one octave down, a fast filter decay that follows velocity, long decay with no sustain.
inline void piano(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Saw; p.layer[0].level = 0.4f; p.layer[0].tune = 3.f; p.layer[0].voices = 3;
  p.layer[1].wave = Wave::Tri; p.layer[1].level = 0.5f; p.layer[1].tune = -3.f; p.layer[1].voices = 3;
  p.layer[2].wave = Wave::Sine; p.layer[2].octave = 1; p.layer[2].level = 0.12f; p.layer[2].voices = 1;
  p.cutoff = 2200.f; p.reso = 0.04f; p.envAmt = 0.55f; p.keyTrack = 0.55f; p.velCutoff = 0.45f; p.fenv = {0.001f, 0.9f, 0.1f, 0.3f}; p.lowCutHz = 55.f;
  p.master = 0.9f; p.amp = {0.002f, 3.0f, 0.04f, 0.5f}; p.spread = 0.6f; p.reverbMix = 0.14f; p.reverbSize = 0.55f; p.reverbLowCutHz = 250.f; p.chorusMix = 0.08f;
}
// Felt piano: softer and darker.
inline void feltPiano(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Tri; p.layer[0].level = 0.7f; p.layer[0].voices = 3;
  p.layer[1].wave = Wave::Sine; p.layer[1].level = 0.35f; p.layer[1].octave = 1; p.layer[1].voices = 1;
  p.cutoff = 1500.f; p.envAmt = 0.4f; p.keyTrack = 0.4f; p.velCutoff = 0.3f; p.fenv = {0.002f, 0.7f, 0.1f, 0.3f}; p.lowCutHz = 50.f;
  p.master = 1.2f; p.amp = {0.004f, 2.5f, 0.06f, 0.6f}; p.reverbMix = 0.2f; p.reverbSize = 0.65f; p.reverbLowCutHz = 200.f; p.chorusMix = 0.1f;
}
// Drawbar organ: 8', 4' and 2 2/3' sine drawbars plus a sub, a little key click drive and a rotary-style chorus.
inline void organ(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Sine; p.layer[0].level = 0.6f;
  p.layer[1].wave = Wave::Sine; p.layer[1].level = 0.4f; p.layer[1].octave = 1;
  p.layer[2].wave = Wave::Sine; p.layer[2].level = 0.3f; p.layer[2].octave = 1; p.layer[2].semi = 7; p.layer[2].tune = 2.f;
  p.subLevel = 0.35f; p.subWave = Wave::Sine; p.subOctave = -1;
  p.cutoff = 9000.f; p.dist = 0.12f; p.drive = 0.1f; p.lowCutHz = 40.f;
  p.master = 0.26f; p.amp = {0.004f, 0.05f, 1.f, 0.06f}; p.chorusMix = 0.5f; p.chorusRate = 6.f; p.chorusDepth = 0.5f; p.reverbMix = 0.1f; p.spread = 0.5f;
}
// Clavinet: narrow pulse, very short filter decay that follows the key.
inline void clav(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.25f; p.layer[0].level = 0.6f;
  p.layer[1].wave = Wave::Saw; p.layer[1].level = 0.25f; p.layer[1].octave = 1;
  p.cutoff = 1200.f; p.reso = 0.25f; p.envAmt = 0.8f; p.keyTrack = 0.5f; p.velCutoff = 0.4f; p.fenv = {0.001f, 0.18f, 0.f, 0.1f}; p.lowCutHz = 90.f; p.filterMode = FilterMode::LP12;
  p.master = 1.5f; p.amp = {0.001f, 0.4f, 0.05f, 0.12f}; p.reverbMix = 0.04f;
}
// String section: three detuned super saws, slow attack, gentle filter, ensemble chorus and a wide hall.
inline void strings(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 7; p.layer[0].cents = 10.f; p.layer[0].level = 0.45f; p.layer[0].pan = -0.15f;
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 7; p.layer[1].cents = 14.f; p.layer[1].level = 0.45f; p.layer[1].pan = 0.15f; p.layer[1].tune = 6.f;
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 5; p.layer[2].cents = 8.f; p.layer[2].level = 0.25f; p.layer[2].octave = -1;
  p.cutoff = 4200.f; p.reso = 0.05f; p.envAmt = 0.15f; p.keyTrack = 0.4f; p.fenv = {0.3f, 0.6f, 0.8f, 0.5f}; p.lowCutHz = 110.f; p.drift = 0.2f;
  p.master = 1.4f; p.amp = {0.35f, 0.5f, 0.9f, 0.55f}; p.spread = 1.f; p.chorusMix = 0.5f; p.chorusDepth = 0.4f; p.reverbMix = 0.25f; p.reverbSize = 0.8f; p.reverbLowCutHz = 300.f;
}
// Warm pad: triangle and soft saws, slow, slightly moving filter.
inline void warmPad(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 5; p.layer[0].cents = 9.f; p.layer[0].level = 0.4f;
  p.layer[1].wave = Wave::Tri; p.layer[1].level = 0.5f; p.layer[1].octave = -1;
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 5; p.layer[2].cents = 12.f; p.layer[2].level = 0.25f; p.layer[2].tune = -7.f;
  p.cutoff = 1500.f; p.reso = 0.12f; p.envAmt = 0.3f; p.fenv = {1.2f, 1.f, 0.6f, 1.f}; p.lfoRate = 0.15f; p.lfoCutoff = 0.25f; p.lowCutHz = 90.f; p.drift = 0.3f;
  p.master = 0.85f; p.amp = {1.1f, 0.8f, 0.9f, 1.6f}; p.spread = 1.f; p.chorusMix = 0.55f; p.chorusRate = 0.4f; p.reverbMix = 0.35f; p.reverbSize = 0.85f; p.reverbLowCutHz = 250.f;
}
// Glass pad: FM sines give a bright bell-like body that slowly opens.
inline void glassPad(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Sine; p.layer[0].level = 0.5f; p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.22f;
  p.layer[1].wave = Wave::Sine; p.layer[1].octave = 1; p.layer[1].level = 0.f; p.layer[1].tune = 5.f;
  p.layer[2].wave = Wave::SuperSaw; p.layer[2].voices = 5; p.layer[2].cents = 7.f; p.layer[2].level = 0.18f; p.layer[2].octave = 1;
  p.cutoff = 5000.f; p.envAmt = 0.2f; p.fenv = {1.5f, 1.f, 0.7f, 1.f}; p.lowCutHz = 120.f; p.lfoRate = 0.3f; p.lfoPan = 0.3f;
  p.master = 1.3f; p.amp = {0.8f, 0.8f, 0.85f, 1.8f}; p.spread = 1.f; p.chorusMix = 0.4f; p.reverbMix = 0.4f; p.reverbSize = 0.9f; p.reverbLowCutHz = 300.f; p.delayMix = 0.12f; p.delayTime = 0.43f; p.delayFb = 0.4f;
}
// Choir: saws through a band pass filter for a vowel colour.
inline void choir(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 5; p.layer[0].cents = 8.f; p.layer[0].level = 0.7f;
  p.layer[1].wave = Wave::Pulse; p.layer[1].pwm = 0.4f; p.layer[1].level = 0.3f; p.layer[1].tune = 4.f;
  p.filterMode = FilterMode::BP; p.cutoff = 1100.f; p.reso = 0.35f; p.keyTrack = 0.5f; p.envAmt = 0.1f; p.fenv = {0.4f, 0.5f, 0.8f, 0.6f}; p.lowCutHz = 100.f; p.lfoRate = 5.2f; p.lfoPitch = 0.12f;
  p.master = 1.1f; p.amp = {0.45f, 0.5f, 0.9f, 0.7f}; p.spread = 0.9f; p.chorusMix = 0.4f; p.reverbMix = 0.35f; p.reverbSize = 0.85f; p.reverbLowCutHz = 250.f;
}
// Bell: inharmonic FM (a modulator a fifth above one octave up), long ring, no sustain.
inline void bell(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Sine; p.layer[0].level = 0.6f; p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.3f;
  p.layer[1].wave = Wave::Sine; p.layer[1].octave = 1; p.layer[1].semi = 7; p.layer[1].tune = 2.f; p.layer[1].level = 0.f;
  p.layer[2].wave = Wave::Sine; p.layer[2].octave = 2; p.layer[2].level = 0.1f;
  p.cutoff = 9000.f; p.envAmt = 0.2f; p.keyTrack = 0.3f; p.fenv = {0.001f, 1.5f, 0.f, 0.5f}; p.lowCutHz = 100.f;
  p.master = 0.9f; p.amp = {0.001f, 3.5f, 0.f, 1.f}; p.reverbMix = 0.25f; p.reverbSize = 0.75f; p.reverbLowCutHz = 300.f;
}
// Analog brass: saws with a filter swell on the attack.
inline void brass(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 5; p.layer[0].cents = 9.f; p.layer[0].level = 0.55f;
  p.layer[1].wave = Wave::Saw; p.layer[1].level = 0.35f; p.layer[1].tune = 6.f;
  p.cutoff = 1300.f; p.reso = 0.1f; p.envAmt = 0.6f; p.keyTrack = 0.3f; p.velCutoff = 0.3f; p.fenv = {0.09f, 0.4f, 0.55f, 0.25f}; p.lowCutHz = 90.f;
  p.master = 0.9f; p.amp = {0.04f, 0.3f, 0.85f, 0.15f}; p.spread = 0.8f; p.chorusMix = 0.25f; p.reverbMix = 0.12f;
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
  p.master = 0.48f; p.amp = {0.003f, 0.6f, 0.85f, 0.25f}; p.spread = 0.8f; p.chorusMix = 0.2f; p.delayMix = 0.08f; p.reverbMix = 0.02f;
}

// Rolling trance bass: saw + sub, short and punchy, mono and centred, sits under offbeat patterns.
inline void tranceBass(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Saw; p.layer[0].level = 0.55f;
  p.layer[1].wave = Wave::Pulse; p.layer[1].pwm = 0.45f; p.layer[1].level = 0.3f; p.layer[1].tune = 6.f;
  p.subLevel = 0.5f; p.subWave = Wave::Sine; p.subOctave = -1;
  p.cutoff = 380.f; p.reso = 0.25f; p.envAmt = 0.55f; p.fenv = {0.001f, 0.16f, 0.1f, 0.08f}; p.drive = 0.3f; p.dist = 0.15f; p.lowCutHz = 28.f;
  p.amp = {0.002f, 0.2f, 0.55f, 0.07f}; p.spread = 0.f; p.bassMonoHz = 200.f; p.master = 0.52f;
}

// Funky / groovy bass: square + saw, resonant filter with a quick envelope pluck, triangle sub, tracks the keyboard.
inline void funkBass(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.35f; p.layer[0].level = 0.5f;
  p.layer[1].wave = Wave::Saw; p.layer[1].level = 0.4f; p.layer[1].octave = -1;
  p.subLevel = 0.35f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.filterMode = FilterMode::LP24; p.cutoff = 250.f; p.reso = 0.45f; p.envAmt = 0.75f; p.keyTrack = 0.4f; p.fenv = {0.001f, 0.2f, 0.05f, 0.1f}; p.drive = 0.35f; p.dist = 0.2f; p.lowCutHz = 30.f;
  p.amp = {0.001f, 0.28f, 0.35f, 0.08f}; p.spread = 0.f; p.bassMonoHz = 220.f; p.master = 0.67f;
}

// ---- Recipes (from Syntorial's published patch recipes, translated to this engine; percentages are knob positions) ----
// Filter knob positions are mapped on a 20 Hz to 20 kHz log scale: 35% = 224 Hz, 55% = 894 Hz, 65% = 1.8 kHz, 70% = 2.5 kHz, 75% = 3.6 kHz, 85% = 7.1 kHz.

// Daft Punk "Da Funk" lead: one pulse, band-pass filter with resonance, long filter envelope that opens then closes (the "wah"), drive.
// Recipe values from the Syntorial recipe, plus the extra pre-filter drive and multiband squash the user found closer by ear.
inline void daftFunk(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.5f; p.layer[0].level = 1.0f;
  // Values fitted by measurement: band-pass lead whose average spectrum peaks near 1.3 kHz and falls about 10 dB per octave above it,
  // opens in about 80 ms, then closes slowly (fitted against a demo recording of the sound, timbre only).
  p.filterMode = FilterMode::BP; p.cutoff = 268.f; p.reso = 0.22f; p.keyTrack = 0.3f; p.drive = 1.47f;
  p.envAmt = 0.86f; p.fenv = {0.077f, 3.2f, 0.015f, 0.11f, 0.f, -0.365f, -0.4f};
  p.dist = 0.9f; p.amp = {0.001f, 1.f, 1.f, 0.05f}; p.spread = 0.f; p.drift = 0.21f; p.master = 0.6f;
  p.dmMix = 0.7f; p.dmDrive = 0.74f; p.dmClip = 1.0f; p.dmLow = 1.0f; p.dmHigh = -0.5f;
}

// Da Funk, fatter: same lead plus a quiet second pulse a hair out of tune and an octave-up edge for bite, high-passed
// at 140 Hz so the low mud is gone, a side-only chorus for width (the centre stays the same, so it sums to mono cleanly)
// and a short side-only reverb.
inline void daftFunkFat(Params& p) {
  daftFunk(p);
  p.layer[1].wave = Wave::Pulse; p.layer[1].pwm = 0.42f; p.layer[1].level = 0.45f; p.layer[1].cents = 0.f; p.layer[1].tune = 7.f;
  p.layer[2].wave = Wave::Saw; p.layer[2].octave = 1; p.layer[2].level = 0.18f;
  p.lowCutHz = 140.f;
  p.chorusMix = 0.22f; p.chorusDepth = 0.35f; p.chorusRate = 0.35f; p.chorusWidth = 1.f;
  p.reverbMix = 0.08f; p.reverbSize = 0.35f; p.reverbWidth = 0.9f; p.reverbLowCutHz = 300.f;
  p.master = 0.5f;
}

// Classic hoover (recipe 1): two saws (second +2 octaves), triangle sub, 4-voice unison 36 cents, pitch envelope that rises 8 semitones then falls, long release.
inline void hooverClassic(Params& p) {
  p = Params();
  p.mono = true; p.subPreFilter = true;
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 4; p.layer[0].cents = 36.f; p.layer[0].level = 0.7f;
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 4; p.layer[1].cents = 36.f; p.layer[1].octave = 2; p.layer[1].level = 0.3f;
  p.subLevel = 1.0f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.filterMode = FilterMode::LP24; p.cutoff = 894.f; p.envAmt = 0.7f; p.fenv = {0.001f, 0.1f, 1.f, 0.3f};
  p.pitchAmt = 8.f; p.pitchAttack = 0.13f; p.pitchTime = 0.4f;
  p.amp = {0.001f, 0.1f, 1.f, 2.9f}; p.spread = 0.7f; p.master = 0.85f;
}

// Prodigy-style hoover (recipe 2): two pulses (second +2 octaves), a loud triangle sub through the filter, 4-voice unison 21 cents, noise, band-pass. No pitch envelope.
inline void hooverProdigy(Params& p) {
  p = Params();
  p.mono = true; p.subPreFilter = true;
  p.layer[0].wave = Wave::SuperSquare; p.layer[0].voices = 4; p.layer[0].cents = 21.f; p.layer[0].pwm = 0.5f; p.layer[0].level = 0.7f;
  p.layer[1].wave = Wave::SuperSquare; p.layer[1].voices = 4; p.layer[1].cents = 21.f; p.layer[1].pwm = 0.5f; p.layer[1].octave = 2; p.layer[1].level = 0.3f;
  p.subLevel = 1.4f; p.subWave = Wave::Tri; p.subOctave = -1; p.noiseLevel = 0.1f; p.noiseHpHz = 200.f;
  p.filterMode = FilterMode::BP; p.cutoff = 1780.f; p.reso = 0.4f; p.keyTrack = 0.7f;
  p.amp = {0.001f, 0.1f, 1.f, 0.02f}; p.spread = 0.1f; p.master = 1.44f;
}

// Alice Deejay "Better Off Alone" lead: one saw, nothing else, big reverb (20% wet).
inline void betterOffAlone(Params& p) {
  p = Params();
  p.mono = true; p.layer[0].wave = Wave::Saw; p.layer[0].level = 1.0f; p.cutoff = 18000.f;
  p.amp = {0.001f, 0.1f, 1.f, 0.02f}; p.spread = 0.f; p.reverbMix = 0.2f; p.reverbSize = 0.9f; p.master = 0.68f;
}

// ABBA "Gimme Gimme" lead: saw + narrow pulse an octave up, low-pass, 1/16 vibrato of 42 cents (10 Hz at 150 BPM), chorus and reverb.
inline void gimmeLead(Params& p) {
  p = Params();
  p.mono = true;
  p.layer[0].wave = Wave::Saw; p.layer[0].level = 0.5f; p.layer[0].lfoPitchCents = 42.f;
  p.layer[1].wave = Wave::Pulse; p.layer[1].pwm = 0.9f; p.layer[1].octave = 1; p.layer[1].level = 0.5f; p.layer[1].lfoPitchCents = 42.f;
  p.lfoShape = 1; p.lfoRate = 10.f; p.cutoff = 7100.f; p.reso = 0.4f;
  p.amp = {0.001f, 0.1f, 1.f, 0.02f}; p.spread = 0.5f; p.chorusMix = 0.3f; p.reverbMix = 0.2f; p.reverbSize = 0.6f; p.master = 0.93f;
}

// Avicii "Levels" lead: two unison pulses (second +1 octave with vibrato), low-pass, short amp envelope, 1/8 delay and a little reverb.
inline void levelsLead(Params& p) {
  p = Params();
  p.mono = true;
  p.layer[0].wave = Wave::SuperSquare; p.layer[0].voices = 4; p.layer[0].cents = 13.f; p.layer[0].pwm = 0.3f; p.layer[0].level = 0.3f;
  p.layer[1].wave = Wave::SuperSquare; p.layer[1].voices = 4; p.layer[1].cents = 13.f; p.layer[1].pwm = 0.5f; p.layer[1].octave = 1; p.layer[1].level = 0.7f; p.layer[1].lfoPitchCents = 28.f;
  p.lfoShape = 1; p.lfoRate = 10.f; p.cutoff = 3560.f; p.reso = 0.35f; p.keyTrack = 0.2f;
  p.amp = {0.001f, 0.3f, 0.4f, 0.6f}; p.spread = 0.6f;
  p.delayMix = 0.2f; p.delayTime = 0.2f; p.delayFb = 0.5f; p.reverbMix = 0.1f; p.reverbSize = 0.6f; p.master = 2.93f;   // delay 1/8 note at 150 BPM
}

// Darude "Sandstorm" lead: saw + very narrow pulse 31 semitones up, low-pass, drive, long decay, a little reverb.
inline void sandstormLead(Params& p) {
  p = Params();
  p.mono = true;
  p.layer[0].wave = Wave::Saw; p.layer[0].level = 0.5f;
  p.layer[1].wave = Wave::Pulse; p.layer[1].pwm = 0.95f; p.layer[1].octave = 1; p.layer[1].semi = 19; p.layer[1].tune = 0.f; p.layer[1].level = 0.5f;
  p.cutoff = 2510.f; p.reso = 0.4f; p.dist = 0.6f;
  p.amp = {0.001f, 6.f, 0.f, 0.02f}; p.spread = 0.f; p.reverbMix = 0.1f; p.reverbSize = 0.6f; p.master = 0.73f;
}

// Gigi D'Agostino lead: mono saw unison (4 voices, 17 cents), low-pass at 85%, chorus, a little delay (1/2 note at 150 BPM) and a big reverb.
// (The recipe also has a short envelope on oscillator 2 pitch; with only one oscillator in the recipe it is not heard, so it is left out.)
inline void gigiLead(Params& p) {
  p = Params();
  p.mono = true; p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 4; p.layer[0].cents = 17.f; p.layer[0].level = 1.0f;
  p.cutoff = 7100.f; p.keyTrack = 0.5f; p.dist = 0.09f; p.amp = {0.001f, 0.1f, 1.f, 0.02f}; p.spread = 0.7f;
  p.chorusMix = 0.5f; p.delayMix = 0.1f; p.delayTime = 0.8f; p.delayFb = 0.2f; p.reverbMix = 0.2f; p.reverbSize = 0.9f; p.master = 0.53f;
}

// Alex F "Axel F" style lead: narrow pulse, 2-voice unison with full stereo, low-pass, PWM from a triangle LFO at 1/4 note (2.5 Hz at 150 BPM), 1/8 delay.
inline void alexFLead(Params& p) {
  p = Params();
  p.mono = true; p.layer[0].wave = Wave::SuperSquare; p.layer[0].voices = 2; p.layer[0].cents = 4.f; p.layer[0].pwm = 0.8f; p.layer[0].level = 1.0f;
  p.lfoShape = 1; p.lfoRate = 2.5f; p.lfoPwm = 0.65f; p.cutoff = 7100.f; p.amp = {0.01f, 0.1f, 1.f, 0.07f}; p.spread = 1.f;
  p.delayMix = 0.3f; p.delayTime = 0.2f; p.delayFb = 0.3f; p.master = 0.82f;
}

// MGMT "Time to Pretend" lead: mono legato pulse (30%), low-pass with resonance, 70 cent vibrato at 1/32 note (20 Hz at 150 BPM), 20 ms glide, chorus, a little reverb.
inline void mgmtLead(Params& p) {
  p = Params();
  p.mono = true; p.glide = 0.02f; p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.3f; p.layer[0].level = 1.0f; p.layer[0].lfoPitchCents = 70.f;
  p.lfoShape = 1; p.lfoRate = 20.f; p.cutoff = 3560.f; p.reso = 0.45f; p.dist = 0.02f; p.amp = {0.001f, 0.1f, 1.f, 0.06f}; p.spread = 0.4f;
  p.chorusMix = 0.2f; p.reverbMix = 0.1f; p.reverbSize = 0.6f; p.master = 0.52f;
}

// Pendulum "Nothing for Free" lead: two 7-voice saw stacks (the first an octave up), 24 dB low-pass at 90%, drive, 41% chorus, large reverb with its low end cut.
inline void pendulumLead(Params& p) {
  p = Params();
  p.mono = true;
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 7; p.layer[0].detune = 0.5f; p.layer[0].octave = 1; p.layer[0].level = 0.71f;
  p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 7; p.layer[1].detune = 0.5f; p.layer[1].level = 0.29f;
  p.cutoff = 6450.f; p.reso = 0.28f; p.drive = 0.17f; p.amp = {0.001f, 0.1f, 1.f, 0.73f}; p.spread = 1.f;
  p.chorusMix = 0.41f; p.reverbMix = 0.37f; p.reverbSize = 0.99f; p.reverbLowCutHz = 530.f; p.master = 1.73f;
}

// Kavinsky "Nightcall" keys: poly saw with 2-voice unison, noise, resonant low-pass with a long envelope that closes slowly, chorus and reverb.
inline void nightCallKeys(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 2; p.layer[0].cents = 8.f; p.layer[0].level = 1.0f;
  p.noiseLevel = 0.15f; p.noiseHpHz = 200.f;
  p.cutoff = 632.f; p.reso = 0.45f; p.envAmt = 0.9f; p.fenv = {0.001f, 3.7f, 0.f, 0.1f};
  p.amp = {0.001f, 0.1f, 1.f, 0.02f}; p.spread = 0.6f; p.chorusMix = 0.35f; p.reverbMix = 0.2f; p.reverbSize = 0.6f; p.master = 1.15f;
}

// Aretha Franklin "Freeway of Love" bass: mono saw with a pulse sub, resonant low-pass with full key tracking and a short envelope, chorus.
inline void freewayBass(Params& p) {
  p = Params();
  p.mono = true; p.subPreFilter = true; p.layer[0].wave = Wave::Saw; p.layer[0].level = 1.0f;
  p.subLevel = 0.35f; p.subWave = Wave::Pulse; p.subOctave = -1;
  p.cutoff = 1260.f; p.reso = 0.6f; p.keyTrack = 1.0f; p.envAmt = 0.4f; p.fenv = {0.001f, 0.35f, 0.f, 0.001f};
  p.amp = {0.01f, 0.1f, 1.f, 0.05f}; p.spread = 0.f; p.bassMonoHz = 200.f; p.chorusMix = 0.45f; p.delayMix = 0.1f; p.delayTime = 0.05f; p.master = 0.53f;
}

// ---- Bass recipes ----
// Boris Brejcha style bass: saw + saw an octave up, heavy noise, low-pass with a quick envelope, delay, reverb; the mod wheel opens the cutoff. Tight phase reset.
inline void brejchaBass(Params& p) {
  p = Params();
  p.mono = true; p.phaseReset = true;
  p.layer[0].wave = Wave::Saw; p.layer[0].level = 0.8f; p.layer[1].wave = Wave::Saw; p.layer[1].octave = 1; p.layer[1].level = 0.2f;
  p.noiseLevel = 0.7f; p.noiseHpHz = 100.f;
  p.cutoff = 159.f; p.envAmt = 0.7f; p.fenv = {0.001f, 0.17f, 0.f, 10.f}; p.wheelCutoff = 1.1f;
  p.amp = {0.001f, 1.05f, 0.7f, 0.15f}; p.spread = 0.2f; p.bassMonoHz = 200.f;
  p.delayMix = 0.15f; p.delayTime = 0.2f; p.delayFb = 0.25f; p.reverbMix = 0.2f; p.reverbSize = 0.6f; p.master = 0.91f;
}

// Donna Summer "I Feel Love" bass: saw + triangle sub, low-pass with a short envelope, square-wave auto-pan at 1/8 note, chorus. (The FM amount in the recipe has the modulator at 0%, so it is not used.)
inline void feelLoveBass(Params& p) {
  p = Params();
  p.mono = true; p.subPreFilter = true;
  p.layer[0].wave = Wave::Saw; p.layer[0].level = 1.0f; p.subLevel = 0.4f; p.subWave = Wave::Tri; p.subOctave = -1;
  p.cutoff = 317.f; p.envAmt = 1.0f; p.fenv = {0.001f, 0.25f, 0.f, 0.19f};
  p.lfoShape = 2; p.lfoRate = 5.f; p.lfoPan = 0.6f;
  p.amp = {0.001f, 0.65f, 0.f, 0.22f}; p.spread = 0.f; p.bassMonoHz = 200.f; p.chorusMix = 0.5f; p.chorusRate = 0.3f; p.master = 0.44f;
}

// Fatboy Slim "Everybody Needs a 303" bass: narrow pulse, low-pass at 45% with 90% resonance, short decaying envelope, drive. (The recipe's 100% pulse width is a very thin pulse here.)
inline void nboy303(Params& p) {
  p = Params();
  p.mono = true; p.layer[0].wave = Wave::Pulse; p.layer[0].pwm = 0.95f; p.layer[0].level = 1.0f;
  p.cutoff = 448.f; p.reso = 0.9f; p.keyTrack = 0.3f; p.envAmt = 0.4f; p.fenv = {0.001f, 0.45f, 0.f, 0.05f};
  p.dist = 0.65f; p.amp = {0.001f, 0.1f, 1.f, 0.02f}; p.spread = 0.f; p.reverbMix = 0.1f; p.reverbSize = 0.6f; p.master = 0.55f;
}

// Le Sserafim "Spaghetti" bass: two sines (one an octave up), very low cutoff opened by an envelope, a tiny pitch blip, glide, heavy soft-clip, chorus. The recipe's multiband compressor and EQ are not modelled.
inline void spaghettiBass(Params& p) {
  p = Params();
  p.mono = true; p.glide = 0.024f;
  p.layer[0].wave = Wave::Sine; p.layer[0].octave = 1; p.layer[0].level = 0.71f; p.layer[1].wave = Wave::Sine; p.layer[1].level = 0.71f;
  p.cutoff = 59.f; p.drive = 0.5f; p.envAmt = 0.64f; p.fenv = {0.001f, 0.171f, 0.14f, 0.09f};
  p.pitchAmt = 7.f; p.pitchTime = 0.01f; p.dist = 0.71f;
  p.amp = {0.001f, 0.1f, 1.f, 0.09f}; p.spread = 0.2f; p.bassMonoHz = 200.f; p.chorusMix = 0.26f; p.reverbMix = 0.18f; p.reverbSize = 0.3f; p.master = 0.42f;
}

// Martin Garrix "Access" bass: three-voice saw, resonant low-pass with key tracking, a fast volume LFO for the rhythmic gating (the recipe has two LFOs, this has one at 1/16), mod wheel on cutoff.
inline void accessBass(Params& p) {
  p = Params();
  p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 3; p.layer[0].detune = 0.1f; p.layer[0].level = 1.0f;
  p.cutoff = 404.f; p.reso = 0.76f; p.keyTrack = 0.28f; p.wheelCutoff = 0.46f;
  p.lfoShape = 1; p.lfoRate = 10.f; p.lfoAmp = 0.76f;
  p.amp = {0.001f, 0.1f, 1.f, 0.09f}; p.spread = 0.f; p.bassMonoHz = 200.f; p.master = 1.29f;
}


// Drums. Noise plus a pitch-swept tone, shaped by the envelopes. Play them around C2 to C3 (the kick pitch follows the key).
inline void drumBase(Params& p) { p = Params(); p.dmMix = 0.f; p.layer[0].wave = Wave::Off; p.layer[1].wave = Wave::Off; p.layer[2].wave = Wave::Off; p.spread = 0.f; p.bassMonoHz = 20.f; p.cutoff = 18000.f; p.reso = 0.f; p.lowCutHz = 20.f; }
inline void kick(Params& p) {
  drumBase(p);
  p.layer[0].wave = Wave::Sine; p.layer[0].level = 0.9f;
  p.pitchAmt = 36.f; p.pitchTime = 0.035f; p.pitchCurve = 0.f;
  p.cutoff = 5000.f; p.dist = 0.25f; p.lowCutHz = 25.f; p.amp = {0.001f, 0.32f, 0.f, 0.08f}; p.master = 0.84f;
}
inline void snare(Params& p) {
  drumBase(p);
  p.layer[0].wave = Wave::Tri; p.layer[0].level = 0.55f; p.layer[0].octave = 1;
  p.pitchAmt = 10.f; p.pitchTime = 0.03f; p.noiseLevel = 1.8f; p.noiseHpHz = 1400.f;
  p.cutoff = 11000.f; p.dist = 0.12f; p.lowCutHz = 110.f; p.amp = {0.001f, 0.17f, 0.f, 0.06f}; p.reverbMix = 0.08f; p.reverbSize = 0.35f; p.master = 0.89f;
}
inline void closedHat(Params& p) {
  drumBase(p);
  p.noiseLevel = 2.f; p.noiseHpHz = 7000.f; p.filterMode = FilterMode::HP; p.cutoff = 6500.f; p.reso = 0.25f; p.lowCutHz = 300.f;
  p.amp = {0.001f, 0.05f, 0.f, 0.03f}; p.master = 1.6f;
}
inline void openHat(Params& p) {
  drumBase(p);
  p.noiseLevel = 2.f; p.noiseHpHz = 6500.f; p.filterMode = FilterMode::HP; p.cutoff = 6000.f; p.reso = 0.3f; p.lowCutHz = 300.f;
  p.amp = {0.001f, 0.28f, 0.05f, 0.22f}; p.master = 1.44f;
}
inline void crash(Params& p) {
  drumBase(p);
  p.noiseLevel = 2.f; p.noiseHpHz = 3500.f; p.filterMode = FilterMode::HP; p.cutoff = 2800.f; p.reso = 0.35f; p.lowCutHz = 250.f;
  p.amp = {0.002f, 1.6f, 0.f, 1.2f}; p.chorusMix = 0.25f; p.reverbMix = 0.15f; p.reverbSize = 0.6f; p.master = 0.9f;
}

}  // namespace supurr::presets
#include "presets_rave.h"
namespace supurr::presets {
inline bool byNameRaw(const std::string& n, Params& p) {
  if (n == "hoover") hoover(p); else if (n == "pick") pick(p); else if (n == "pluck") pluck(p); else if (n == "trance_lead") tranceLead(p);
  else if (n == "trance_hybrid") tranceHybrid(p); else if (n == "trance_90s") trance90s(p);
  else if (n == "epiano_dx") epianoDx(p); else if (n == "epiano_wurly") epianoWurly(p); else if (n == "piano") piano(p); else if (n == "felt_piano") feltPiano(p);
  else if (n == "organ") organ(p); else if (n == "clav") clav(p); else if (n == "strings") strings(p); else if (n == "warm_pad") warmPad(p); else if (n == "glass_pad") glassPad(p);
  else if (n == "choir") choir(p); else if (n == "bell") bell(p); else if (n == "brass") brass(p);
  else if (n == "talking_lead") talkingLead(p); else if (n == "trance_bass") tranceBass(p); else if (n == "funk_bass") funkBass(p);
  else if (n == "daft_funk") daftFunk(p); else if (n == "daft_funk_fat") daftFunkFat(p); else if (n == "hoover_classic") hooverClassic(p); else if (n == "hoover_prodigy") hooverProdigy(p);
  else if (n == "better_off_alone") betterOffAlone(p); else if (n == "gimme_lead") gimmeLead(p); else if (n == "levels_lead") levelsLead(p); else if (n == "sandstorm_lead") sandstormLead(p);
  else if (n == "gigi_lead") gigiLead(p); else if (n == "alexf_lead") alexFLead(p); else if (n == "mgmt_lead") mgmtLead(p); else if (n == "pendulum_lead") pendulumLead(p);
  else if (n == "nightcall_keys") nightCallKeys(p); else if (n == "freeway_bass") freewayBass(p);
  else if (n == "brejcha_bass") brejchaBass(p); else if (n == "feellove_bass") feelLoveBass(p); else if (n == "nboy_303") nboy303(p); else if (n == "spaghetti_bass") spaghettiBass(p); else if (n == "access_bass") accessBass(p);
  else if (n == "kick") kick(p); else if (n == "snare") snare(p); else if (n == "closed_hat") closedHat(p); else if (n == "open_hat") openHat(p); else if (n == "crash") crash(p);
  else return raveByName(n, p);
  return true;
}


// Keys, pianos, pads, bells and organs: the Multiband comp clipper is turned down so they stay clean (the default 40 percent mix adds about 4 percent harmonic distortion to a clean tone, this setting about 0.4 percent).
inline bool cleanKeys(const std::string& n) {
  static const char* k[] = {"nightcall_keys", "epiano_dx", "epiano_wurly", "piano", "felt_piano", "organ", "clav", "strings", "warm_pad", "glass_pad", "choir", "bell", "house_m1_piano", "hardcore_piano", "dream_trance_piano", "garage_4am_epiano", "donk_organ_keys", "ice_bells", "pizz_pluck_bell", "glass_fm", "formant_pad_90s", "breakbeat_pad_92", "aah_choir_pad", "lush_string_pad", "warm_analog_pad", "wurli_bark_keys", "glass_bell_pad", "vocal_ooh_pad", "marimba_soft", "organ_drawbar_full", "honky_piano", "inst_moist_keys", "inst_moist_keys_pad", "inst_italo_stab_keys", "inst_rave_piano_stab", "inst_epiano_glass_pad", "inst_lullaby_keys", "inst_sub_piano", "inst_strings_and_saws", "inst_jp_string_ensemble", "inst_pluck_strings_trance", "inst_epic_orchestra_pad", "dance_super_aahh", "inst_dance_piano", "inst_dance_piano_stab", "inst_dance_piano_house", "inst_dance_piano_pad", "inst_organ", "inst_choir", "inst_bottle_bell", "inst_pan_mallet", "inst_e_piano_a", "inst_vibes", "inst_magic_piano", "inst_timpani_and_bells", "inst_e_piano_b", "aah_stab_classic"};
  for (const char* s : k) if (n == s) return true;
  return false;
}
inline bool byName(const std::string& n, Params& p) {
  if (!byNameRaw(n, p)) return false;
  if (cleanKeys(n) && p.dmMix > 0.f) { p.dmMix = 0.15f; p.dmDrive = 0.3f; p.dmClip = 0.f; }
  return true;
}

}  // namespace supurr::presets
