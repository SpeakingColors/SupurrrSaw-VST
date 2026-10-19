// Rave pack: recipe-based presets (rave, hardcore, hardstyle, trance, house, garage, keys, bells, brass, bass, drums, textures).
// Included at the end of presets.h. Parts the engine has no module for (bitcrusher, wavefolder, comb filter, phaser, formant filter)
// are approximated with distortion, drive, resonance, sync, FM and band-pass filtering. Author: Studio 87
#pragma once
#include "presets_rave_gain.h"   // generated output trim per preset (tools/calibrate_rave.cpp), keeps every preset at a similar loudness

namespace supurr::presets {

// mod matrix indices
enum { S_LFO, S_FENV, S_AENV, S_PENV, S_WHL, S_KEY, S_VEL, S_RND, S_MEN, S_LFO2, S_LFO3, S_FENV2, S_FENV3, S_MEN2, S_MEN3 };
enum { D_PITCH, D_CUT, D_RES, D_DRV, D_PWM, D_DET, D_PAN, D_VOL, D_SY1, D_SY2, D_SY3, D_SUB, D_NZ, D_LC, D_CUT2, D_CUT3, D_RVB, D_DLY, D_DRS, D_CLIP, D_HPF, D_LPF, D_WID };

inline void lay(Params& p, int i, Wave w, float lv, int oct = 0, float tune = 0.f, int vo = 1, float cents = 0.f, float pwm = 0.5f, float pan = 0.f) {
  LayerParams& l = p.layer[i]; l.wave = w; l.level = lv; l.octave = oct; l.tune = tune; l.voices = vo; l.cents = cents; l.pwm = pwm; l.pan = pan;
}

// ---------- basics ----------
inline void moogBass(Params& p) {   // simple analog ladder bass: two saws an octave apart, a pulse sub, closed LP24 with a short pluck
  p = Params();
  lay(p, 0, Wave::Saw, 0.55f); lay(p, 1, Wave::Saw, 0.35f, -1, 7.f); lay(p, 2, Wave::Pulse, 0.3f, -1, -5.f, 1, 0.f, 0.5f);
  p.mono = true; p.glide = 0.02f; p.filterMode = FilterMode::LP24; p.cutoff = 420.f; p.reso = 0.28f; p.envAmt = 0.5f; p.keyTrack = 0.25f; p.fenv = {0.002f, 0.28f, 0.3f, 0.15f};
  p.drift = 0.12f; p.drive = 0.15f; p.amp = {0.002f, 0.25f, 0.8f, 0.12f}; p.spread = 0.15f; p.bassMonoHz = 250.f; p.lowCutHz = 28.f;
}

// ---------- leads ----------
inline void virusCommercial(Params& p) {   // Classic Virus commercial trance lead: unison saws, open filter, dotted delay, long reverb
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 7, 16.f); lay(p, 1, Wave::SuperSaw, 0.25f, 1, 0.f, 5, 10.f); lay(p, 2, Wave::Pulse, 0.3f, -1, 0.f, 1, 0.f, 0.5f);
  p.cutoff = 6500.f; p.reso = 0.15f; p.envAmt = 0.2f; p.keyTrack = 0.4f; p.fenv = {0.003f, 0.5f, 0.6f, 0.3f}; p.lowCutHz = 150.f;
  p.mono = true; p.glide = 0.05f; p.amp = {0.003f, 0.3f, 0.9f, 0.35f}; p.drift = 0.15f; p.spread = 0.85f;
  p.chorusMix = 0.25f; p.delayMix = 0.22f; p.delaySync = true; p.delayDiv = 8; p.delayFb = 0.4f; p.reverbMix = 0.22f; p.reverbSize = 0.85f; p.reverbPre = 120.f; p.reverbLowCutHz = 290.f;
}
inline void hardstyleStab(Params& p) {   // laser-knock stab: pitch dive on the transient, no sustain
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.55f, 0, 0.f, 5, 18.f); lay(p, 1, Wave::SuperSquare, 0.3f, 0, 0.f, 3, 10.f);
  p.pitchAmt = 24.f; p.pitchTime = 0.035f; p.pitchCurve = 0.f;
  p.cutoff = 1800.f; p.reso = 0.2f; p.envAmt = 0.6f; p.fenv = {0.001f, 0.18f, 0.f, 0.1f}; p.dist = 0.35f; p.lowCutHz = 160.f;
  p.amp = {0.001f, 0.24f, 0.f, 0.1f}; p.spread = 0.8f; p.reverbMix = 0.12f; p.reverbSize = 0.5f;
}
inline void hyperTranceChord(Params& p) {   // wide, OTT-squashed minor 9th stunner
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.45f, 0, 0.f, 9, 24.f); lay(p, 1, Wave::SuperSaw, 0.25f, 0, 300.f, 5, 16.f); lay(p, 2, Wave::SuperSquare, 0.2f, 1, 0.f, 5, 14.f);
  p.cutoff = 5200.f; p.reso = 0.1f; p.envAmt = 0.3f; p.fenv = {0.001f, 0.35f, 0.5f, 0.3f}; p.lowCutHz = 200.f;
  p.amp = {0.002f, 0.4f, 0.65f, 0.3f}; p.spread = 1.f; p.chorusMix = 0.35f;
  p.ottDepth = 0.5f; p.ottMix = 0.4f; p.reverbMix = 0.2f; p.reverbSize = 0.7f; p.delayMix = 0.12f; p.delaySync = true; p.delayDiv = 7;
}
inline void vowelPluck(Params& p) {   // melodic techno vowel-drive pluck (band-pass vowel sweep approximated, no formant filter)
  p = Params();
  lay(p, 0, Wave::Saw, 0.5f, 0, 0.f, 1, 0.f); lay(p, 1, Wave::Pulse, 0.3f, 0, 7.f, 1, 0.f, 0.35f);
  p.filterMode = FilterMode::BP; p.cutoff = 900.f; p.reso = 0.55f; p.envAmt = 0.5f; p.fenv = {0.001f, 0.3f, 0.f, 0.15f}; p.drive = 0.4f; p.dist = 0.15f;
  p.amp = {0.001f, 0.3f, 0.f, 0.15f}; p.lfoRate = 0.3f; p.lfoCutoff = 0.12f; p.lowCutHz = 120.f;
  p.delayMix = 0.25f; p.delaySync = true; p.delayDiv = 8; p.delayFb = 0.45f; p.reverbMix = 0.2f; p.reverbSize = 0.7f;
}
inline void minimalPluck(Params& p) {   // tech-trance minimal FM pluck
  p = Params();
  lay(p, 0, Wave::Sine, 0.65f); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.3f;
  lay(p, 1, Wave::Sine, 0.f, 1);   // modulator only
  lay(p, 2, Wave::Tri, 0.25f, 1);
  p.cutoff = 3500.f; p.reso = 0.1f; p.envAmt = 0.4f; p.keyTrack = 0.4f; p.fenv = {0.001f, 0.18f, 0.f, 0.1f}; p.lowCutHz = 100.f;
  p.amp = {0.001f, 0.2f, 0.f, 0.1f}; p.delayMix = 0.25f; p.delaySync = true; p.delayDiv = 8; p.reverbMix = 0.15f; p.reverbSize = 0.6f;
}
inline void houseM1Piano(Params& p) {   // dance piano in the Korg M1 spirit
  p = Params();
  lay(p, 0, Wave::Saw, 0.4f); lay(p, 1, Wave::Pulse, 0.3f, 0, 0.f, 1, 0.f, 0.3f); lay(p, 2, Wave::Tri, 0.3f, 1);
  p.cutoff = 2800.f; p.reso = 0.08f; p.envAmt = 0.5f; p.keyTrack = 0.5f; p.velCutoff = 0.3f; p.fenv = {0.001f, 0.5f, 0.15f, 0.3f}; p.lowCutHz = 90.f;
  p.amp = {0.001f, 1.2f, 0.1f, 0.3f}; p.chorusMix = 0.25f; p.reverbMix = 0.15f; p.reverbSize = 0.6f;
}
inline void hardcorePiano(Params& p) {   // bright happy hardcore piano
  p = Params();
  lay(p, 0, Wave::Pulse, 0.4f, 0, 0.f, 1, 0.f, 0.4f); lay(p, 1, Wave::Saw, 0.3f, 1); lay(p, 2, Wave::Tri, 0.25f, -1);
  p.cutoff = 6000.f; p.reso = 0.05f; p.envAmt = 0.35f; p.keyTrack = 0.6f; p.velCutoff = 0.3f; p.fenv = {0.001f, 0.6f, 0.2f, 0.3f}; p.lowCutHz = 100.f;
  p.amp = {0.001f, 0.9f, 0.25f, 0.25f}; p.chorusMix = 0.3f; p.reverbMix = 0.15f; p.spread = 0.8f;
}
inline void dreamTrancePiano(Params& p) {   // Robert Miles style glassy piano
  p = Params();
  lay(p, 0, Wave::Tri, 0.55f); lay(p, 1, Wave::Pulse, 0.3f, 1, 0.f, 1, 0.f, 0.2f); lay(p, 2, Wave::Sine, 0.1f, 2);
  p.filterMode = FilterMode::LP12; p.cutoff = 9000.f; p.reso = 0.05f; p.keyTrack = 1.f; p.lowCutHz = 100.f;
  p.amp = {0.002f, 1.2f, 0.2f, 0.4f}; p.chorusMix = 0.5f; p.chorusDepth = 0.6f; p.chorusRate = 0.3f;
  p.delayMix = 0.25f; p.delaySync = true; p.delayDiv = 4; p.delayPing = 1.f; p.reverbMix = 0.2f; p.reverbSize = 0.7f;
}
inline void garage4amEpiano(Params& p) {   // smoky UK garage e-piano, velocity opens the FM and filter
  p = Params();
  lay(p, 0, Wave::Sine, 0.7f); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.35f;
  lay(p, 1, Wave::Tri, 0.f);   // modulator only
  p.cutoff = 1600.f; p.reso = 0.1f; p.envAmt = 0.4f; p.velCutoff = 0.5f; p.fenv = {0.001f, 0.2f, 0.f, 0.1f}; p.lowCutHz = 60.f;
  p.amp = {0.001f, 0.45f, 0.f, 0.15f}; p.chorusMix = 0.3f; p.chorusRate = 0.5f; p.reverbMix = 0.12f; p.reverbSize = 0.5f; p.lfoPan = 0.1f;
}
inline void donkOrganKeys(Params& p) {   // deep house rubbery organ keys
  p = Params();
  lay(p, 0, Wave::Sine, 0.7f); lay(p, 1, Wave::Pulse, 0.25f, 1, 700.f, 1, 0.f, 0.5f);
  p.cutoff = 700.f; p.reso = 0.12f; p.envAmt = 0.5f; p.fenv = {0.001f, 0.18f, 0.f, 0.08f}; p.lowCutHz = 40.f;
  p.amp = {0.001f, 0.18f, 0.f, 0.08f}; p.spread = 0.3f; p.reverbMix = 0.08f; p.reverbSize = 0.4f;
}
inline void iceBells(Params& p) {   // happy hardcore / rave ice bell (bitcrusher shimmer approximated with a touch of distortion)
  p = Params();
  lay(p, 0, Wave::Sine, 0.5f); lay(p, 1, Wave::Sine, 0.4f, 3); lay(p, 2, Wave::Tri, 0.25f, 0, 700.f);
  p.cutoff = 12000.f; p.dist = 0.12f; p.lowCutHz = 200.f; p.amp = {0.001f, 0.8f, 0.f, 0.5f}; p.spread = 0.8f;
  p.reverbMix = 0.35f; p.reverbSize = 0.85f; p.reverbLowCutHz = 300.f; p.chorusMix = 0.2f;
}
inline void pizzPluckBell(Params& p) {   // Insomnia style pizzicato: a dry pluck turned into a bell by fast delay reflections
  p = Params();
  lay(p, 0, Wave::Pulse, 0.5f, 0, 0.f, 1, 0.f, 0.25f); lay(p, 1, Wave::Saw, 0.25f, 1);
  p.cutoff = 1000.f; p.reso = 0.1f; p.envAmt = 0.6f; p.fenv = {0.001f, 0.09f, 0.f, 0.04f}; p.lowCutHz = 100.f;
  p.amp = {0.001f, 0.09f, 0.f, 0.04f}; p.delayMix = 0.4f; p.delaySync = true; p.delayDiv = 10; p.delayFb = 0.6f; p.delayPing = 1.f; p.delayDamp = 0.5f;
  p.reverbMix = 0.1f; p.reverbSize = 0.5f;
}
inline void siren(Params& p) {   // hardstyle siren screech: wide hypersaws, LFO swings the pitch +/-12 semitones, dotted delay
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 7, 35.f); lay(p, 1, Wave::SuperSaw, 0.4f, 0, 0.f, 7, 28.f);
  p.cutoff = 7000.f; p.reso = 0.2f; p.dist = 0.6f; p.lowCutHz = 200.f; p.amp = {0.002f, 0.3f, 0.9f, 0.2f};
  p.lfoRate = 0.5f; p.lfoShape = 0; p.mx[S_LFO][D_PITCH] = 1.f;
  p.delayMix = 0.3f; p.delaySync = true; p.delayDiv = 8; p.delayFb = 0.6f; p.reverbMix = 0.15f;
}
inline void bigroomLead2013(Params& p) {   // sync square + saw, OTT'd bigroom lead
  p = Params();
  lay(p, 0, Wave::SuperSquare, 0.35f, 0, 0.f, 5, 14.f); lay(p, 1, Wave::SuperSaw, 0.5f, 0, 0.f, 7, 20.f);
  p.cutoff = 7000.f; p.reso = 0.12f; p.lowCutHz = 180.f; p.amp = {0.002f, 0.3f, 0.9f, 0.25f};
  p.ottDepth = 0.6f; p.ottMix = 0.5f; p.reverbMix = 0.15f; p.reverbSize = 0.6f; p.chorusMix = 0.2f;
}
inline void rawHardstyleLead(Params& p) {
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 7, 22.f); lay(p, 1, Wave::SuperSaw, 0.25f, 1, 0.f, 5, 12.f);
  p.cutoff = 8000.f; p.reso = 0.15f; p.dist = 0.35f; p.lowCutHz = 200.f; p.amp = {0.002f, 0.3f, 0.9f, 0.2f};
  p.ottDepth = 0.4f; p.ottMix = 0.3f; p.chorusMix = 0.2f; p.reverbMix = 0.15f; p.reverbSize = 0.55f;
}
inline void goaAcidLead(Params& p) {
  p = Params();
  lay(p, 0, Wave::Saw, 0.7f);
  p.mono = true; p.glide = 0.08f; p.filterMode = FilterMode::LP24; p.cutoff = 1800.f; p.reso = 0.7f; p.envAmt = 0.6f; p.fenv = {0.002f, 0.25f, 0.3f, 0.1f};
  p.dist = 0.4f; p.lfoRate = 0.4f; p.lfoCutoff = 0.2f; p.amp = {0.002f, 0.2f, 0.85f, 0.12f};
  p.delayMix = 0.28f; p.delaySync = true; p.delayDiv = 8; p.delayFb = 0.5f; p.reverbMix = 0.1f;
}
inline void melodicTechnoLead(Params& p) {
  p = Params();
  lay(p, 0, Wave::Saw, 0.5f, 0, 0.f, 1, 0.f); lay(p, 1, Wave::Pulse, 0.3f, 0, 7.f, 1, 0.f, 0.45f);
  p.mono = true; p.glide = 0.03f; p.filterMode = FilterMode::LP12; p.cutoff = 1500.f; p.reso = 0.25f; p.envAmt = 0.3f; p.fenv = {0.01f, 0.4f, 0.5f, 0.3f};
  p.wheelCutoff = 0.5f; p.lfoRate = 0.15f; p.lfoPwm = 0.2f; p.subLevel = 0.2f; p.drift = 0.2f; p.amp = {0.005f, 0.3f, 0.85f, 0.4f};
  p.delayMix = 0.25f; p.delaySync = true; p.delayDiv = 8; p.reverbMix = 0.3f; p.reverbSize = 0.85f; p.reverbLowCutHz = 250.f;
}
inline void hyperpopPluck(Params& p) {
  p = Params();
  lay(p, 0, Wave::SuperSquare, 0.45f, 0, 0.f, 3, 12.f); lay(p, 1, Wave::Saw, 0.3f, 1);
  p.pitchAmt = 5.f; p.pitchTime = 0.02f; p.cutoff = 9000.f; p.envAmt = 0.3f; p.fenv = {0.001f, 0.12f, 0.f, 0.08f}; p.dist = 0.5f; p.drive = 0.3f; p.lowCutHz = 150.f;
  p.amp = {0.001f, 0.2f, 0.f, 0.12f}; p.chorusMix = 0.4f; p.reverbMix = 0.2f;
}
inline void rave2UnlimitedBrass(Params& p) {   // "parp" brass: slow filter attack
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 2, 8.f); lay(p, 1, Wave::SuperSaw, 0.35f, 1, 0.f, 2, 8.f); lay(p, 2, Wave::Pulse, 0.3f, 0, 0.f, 1, 0.f, 0.4f);
  p.filterMode = FilterMode::LP12; p.cutoff = 1000.f; p.reso = 0.12f; p.envAmt = 0.65f; p.fenv = {0.035f, 0.3f, 0.4f, 0.2f}; p.dist = 0.25f; p.lowCutHz = 90.f;
  p.amp = {0.01f, 0.3f, 0.85f, 0.2f}; p.chorusMix = 0.45f; p.reverbMix = 0.1f;
}
inline void bruteBrass(Params& p) {
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 8, 35.f); lay(p, 1, Wave::SuperSaw, 0.45f, -1, 0.f, 8, 20.f);
  p.cutoff = 5000.f; p.dist = 0.8f; p.lowCutHz = 50.f; p.amp = {0.01f, 0.3f, 0.9f, 0.2f};
  p.delayMix = 0.2f; p.delaySync = true; p.delayDiv = 7; p.reverbMix = 0.1f;
}
inline void hhDonkPluckLead(Params& p) {   // rubber ball bounce: muffled filter snapped open per note, legato glide
  p = Params();
  lay(p, 0, Wave::SuperSquare, 0.45f, 0, 0.f, 2, 10.f); lay(p, 1, Wave::SuperSquare, 0.4f, 1, 0.f, 2, 10.f);
  p.mono = true; p.glide = 0.05f; p.cutoff = 500.f; p.reso = 0.15f; p.envAmt = 0.75f; p.fenv = {0.001f, 0.11f, 0.f, 0.08f}; p.lowCutHz = 120.f;
  p.amp = {0.001f, 0.25f, 0.2f, 0.1f}; p.chorusMix = 0.2f; p.reverbMix = 0.1f;
}
inline void anjunaPluck(Params& p) {   // progressive pluck, mod wheel opens it up
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.45f, 0, 0.f, 3, 10.f); lay(p, 1, Wave::Pulse, 0.3f, 0, 0.f, 1, 0.f, 0.35f);
  p.cutoff = 1800.f; p.reso = 0.12f; p.envAmt = 0.55f; p.fenv = {0.001f, 0.35f, 0.f, 0.2f}; p.wheelCutoff = 0.6f; p.lowCutHz = 140.f;
  p.amp = {0.001f, 0.4f, 0.f, 0.25f}; p.delayMix = 0.28f; p.delaySync = true; p.delayDiv = 8; p.delayFb = 0.45f; p.reverbMix = 0.22f; p.reverbSize = 0.75f;
}
inline void vocalTrancePluck2000s(Params& p) {
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 5, 12.f); lay(p, 1, Wave::Saw, 0.25f, 1);
  p.cutoff = 2500.f; p.reso = 0.1f; p.envAmt = 0.6f; p.fenv = {0.001f, 0.3f, 0.f, 0.2f}; p.lowCutHz = 150.f; p.amp = {0.001f, 0.35f, 0.f, 0.2f};
  p.delayMix = 0.28f; p.delaySync = true; p.delayDiv = 8; p.reverbMix = 0.2f; p.reverbSize = 0.7f; p.chorusMix = 0.2f;
}
inline void complextroLaser(Params& p) {   // sync sweep laser; the bitcrusher is approximated with distortion
  p = Params();
  lay(p, 0, Wave::Pulse, 0.5f); p.layer[0].sync = true; p.layer[0].syncSemis = 2.f; p.layer[0].syncEnv = 24.f;
  lay(p, 1, Wave::Saw, 0.35f, 1); p.layer[1].sync = true; p.layer[1].syncSemis = 2.f; p.layer[1].syncEnv = 24.f;
  p.mono = true; p.cutoff = 6000.f; p.fenv = {0.001f, 0.12f, 0.f, 0.05f}; p.dist = 0.3f; p.lowCutHz = 120.f; p.amp = {0.001f, 0.25f, 0.5f, 0.08f};
  p.delayMix = 0.3f; p.delaySync = true; p.delayDiv = 10; p.delayFb = 0.f;
}
inline void industrialHammer(Params& p) {
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 3, 26.f); p.noiseLevel = 0.4f; p.noiseHpHz = 800.f;
  p.cutoff = 1200.f; p.reso = 0.55f; p.envAmt = 0.45f; p.fenv = {0.001f, 0.15f, 0.f, 0.1f}; p.dist = 0.8f; p.lowCutHz = 180.f;
  p.amp = {0.001f, 0.2f, 0.f, 0.1f}; p.reverbMix = 0.12f; p.reverbSize = 0.5f;
}
inline void glassFm(Params& p) {   // liquid crystal 2-op FM bell-key
  p = Params();
  lay(p, 0, Wave::Sine, 0.7f); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.28f;
  lay(p, 1, Wave::Sine, 0.f, 2);   // silent modulator, two octaves up
  lay(p, 2, Wave::Sine, 0.05f, 3);
  p.cutoff = 12000.f; p.lowCutHz = 100.f; p.pitchAmt = 0.f; p.amp = {0.001f, 1.0f, 0.f, 0.5f}; p.reverbMix = 0.2f; p.chorusMix = 0.15f;
}
inline void plasticFmOrgan(Params& p) {
  p = Params();
  lay(p, 0, Wave::Sine, 0.7f); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.38f;
  lay(p, 1, Wave::Tri, 0.f, 1, 700.f);
  p.filterMode = FilterMode::LP12; p.cutoff = 2200.f; p.reso = 0.1f; p.envAmt = 0.45f; p.fenv = {0.001f, 0.15f, 0.3f, 0.1f}; p.lowCutHz = 60.f;
  p.amp = {0.001f, 0.25f, 0.5f, 0.12f}; p.reverbMix = 0.1f;
}
inline void raveChab(Params& p) {   // 90s rave chord stab
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.5f, 0, 0.f, 3, 14.f); lay(p, 1, Wave::Pulse, 0.3f, 0, 0.f, 1, 0.f, 0.4f);
  p.cutoff = 2200.f; p.reso = 0.18f; p.envAmt = 0.5f; p.fenv = {0.001f, 0.25f, 0.15f, 0.12f}; p.lowCutHz = 150.f;
  p.amp = {0.001f, 0.3f, 0.2f, 0.12f}; p.chorusMix = 0.35f; p.reverbMix = 0.2f; p.reverbSize = 0.55f;
}
inline void eurodanceChordStack(Params& p) {   // one key plays root + minor third + fifth
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.35f, 0, 0.f, 5, 12.f); lay(p, 1, Wave::SuperSaw, 0.3f, 0, 300.f, 5, 12.f); lay(p, 2, Wave::SuperSaw, 0.3f, 0, 700.f, 5, 12.f);
  p.cutoff = 4500.f; p.reso = 0.1f; p.envAmt = 0.3f; p.fenv = {0.002f, 0.4f, 0.5f, 0.25f}; p.lowCutHz = 180.f;
  p.amp = {0.003f, 0.4f, 0.7f, 0.25f}; p.chorusMix = 0.3f; p.reverbMix = 0.15f; p.spread = 0.9f;
}
inline void futureBassSwell(Params& p) {   // sidechained supersaw swell chord (pump from the looping mod envelope, one beat at 130 bpm)
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.35f, 0, 0.f, 7, 24.f); lay(p, 1, Wave::SuperSaw, 0.3f, 0, 300.f, 7, 20.f); lay(p, 2, Wave::SuperSaw, 0.3f, 0, 700.f, 7, 20.f);
  p.cutoff = 900.f; p.reso = 0.1f; p.envAmt = 0.7f; p.fenv = {0.5f, 0.5f, 0.6f, 0.4f}; p.lowCutHz = 150.f; p.amp = {0.2f, 0.3f, 0.9f, 0.4f};
  p.ottDepth = 0.6f; p.ottMix = 0.5f; p.chorusMix = 0.3f; p.reverbMix = 0.2f; p.reverbSize = 0.7f;
  p.menvTime = 0.46f; p.menvLoop = true; p.mx[S_MEN][D_VOL] = -0.7f;
}
inline void formantPad90s(Params& p) {   // vowel pad: PWM squares through a swept band-pass
  p = Params();
  lay(p, 0, Wave::Pulse, 0.5f, 0, 0.f, 1, 0.f, 0.5f, -0.3f); lay(p, 1, Wave::Pulse, 0.5f, 0, 8.f, 1, 0.f, 0.4f, 0.3f);
  p.filterMode = FilterMode::BP; p.cutoff = 800.f; p.reso = 0.35f; p.lfoRate = 0.2f; p.lfoPwm = 0.6f; p.lfoCutoff = 0.15f; p.lowCutHz = 100.f;
  p.amp = {0.8f, 0.4f, 1.f, 2.f}; p.chorusMix = 0.4f; p.reverbMix = 0.4f; p.reverbSize = 0.95f; p.reverbDamp = 0.5f; p.spread = 1.f;
}
inline void breakbeatPad92(Params& p) {
  p = Params();
  lay(p, 0, Wave::Tri, 0.45f); lay(p, 1, Wave::Sine, 0.35f, 0, 700.f); lay(p, 2, Wave::Saw, 0.2f, 1, 700.f);
  p.filterMode = FilterMode::LP12; p.cutoff = 1800.f; p.lfoRate = 0.1f; p.lfoShape = 4; p.lfoCutoff = 0.1f; p.lowCutHz = 100.f;
  p.amp = {0.8f, 0.5f, 1.f, 1.5f}; p.chorusMix = 0.6f; p.chorusDepth = 0.5f; p.reverbMix = 0.5f; p.reverbSize = 0.95f; p.reverbDamp = 0.6f; p.spread = 1.f;
}
inline void whiteNoiseRiser(Params& p) {   // all oscillators off; a band-pass sweeps the noise open (slow here, stretch the envelope for 8 or 16 bars)
  p = Params();
  p.noiseLevel = 1.f; p.noiseHpHz = 100.f; p.filterMode = FilterMode::BP; p.cutoff = 100.f; p.reso = 0.15f; p.envAmt = 1.f; p.fenv = {2.5f, 0.1f, 1.f, 1.f};
  p.wheelCutoff = 0.5f; p.amp = {1.5f, 0.1f, 1.f, 0.8f}; p.chorusMix = 1.f; p.chorusWidth = 1.f; p.spread = 1.f; p.bassMonoHz = 20.f; p.lowCutHz = 100.f;
}

// ---------- bass ----------
inline void subDonkBass(Params& p) {
  p = Params();
  lay(p, 0, Wave::Sine, 0.8f); p.pitchAmt = 7.f; p.pitchTime = 0.02f;
  p.dist = 0.3f; p.mx[S_MEN][D_DRV] = 0.6f; p.menvTime = 0.12f; p.cutoff = 3000.f;
  p.amp = {0.001f, 0.5f, 0.7f, 0.1f}; p.spread = 0.f; p.bassMonoHz = 400.f; p.lowCutHz = 25.f;
}
inline void psyRollingBass(Params& p) {
  p = Params();
  lay(p, 0, Wave::Saw, 0.6f); p.subLevel = 0.5f; p.mono = true;
  p.cutoff = 500.f; p.reso = 0.45f; p.envAmt = 0.5f; p.fenv = {0.001f, 0.1f, 0.f, 0.05f}; p.dist = 0.15f;
  p.amp = {0.001f, 0.12f, 0.f, 0.05f}; p.spread = 0.f; p.bassMonoHz = 400.f; p.lowCutHz = 30.f;
}
inline void talkingYoiBass(Params& p) {   // sync sweep plus a resonant filter makes it say "yoi"
  p = Params();
  lay(p, 0, Wave::Saw, 0.6f); p.layer[0].sync = true; p.layer[0].syncSemis = 3.f; p.layer[0].syncEnv = 12.f; p.subLevel = 0.3f; p.mono = true;
  p.cutoff = 600.f; p.reso = 0.55f; p.envAmt = 0.4f; p.fenv = {0.005f, 0.35f, 0.2f, 0.1f}; p.dist = 0.3f;
  p.amp = {0.002f, 0.35f, 0.5f, 0.1f}; p.spread = 0.f; p.bassMonoHz = 400.f; p.lowCutHz = 30.f;
}
inline void alpha909Screech(Params& p) {
  p = Params();
  lay(p, 0, Wave::Saw, 0.7f); p.layer[0].sync = true; p.layer[0].syncSemis = 7.f; p.layer[0].syncEnv = 5.f;
  lay(p, 1, Wave::SuperSaw, 0.3f, 0, 0.f, 5, 20.f);
  p.mono = true; p.glide = 0.03f; p.cutoff = 5000.f; p.reso = 0.45f; p.envAmt = 0.3f; p.fenv = {0.002f, 0.3f, 0.5f, 0.1f};
  p.dist = 0.7f; p.distPre = true; p.lowCutHz = 150.f; p.amp = {0.002f, 0.3f, 0.9f, 0.12f}; p.delayMix = 0.2f; p.delaySync = true; p.delayDiv = 8;
}
inline void acid303Classic80s(Params& p) {
  p = Params();
  lay(p, 0, Wave::Pulse, 0.7f, 0, 0.f, 1, 0.f, 0.5f);
  p.mono = true; p.glide = 0.06f; p.cutoff = 700.f; p.reso = 0.65f; p.envAmt = 0.55f; p.fenv = {0.001f, 0.22f, 0.f, 0.06f}; p.dist = 0.25f; p.drive = 0.2f;
  p.amp = {0.001f, 0.25f, 0.6f, 0.06f}; p.spread = 0.f; p.bassMonoHz = 300.f;
}
inline void italoBass(Params& p) {
  p = Params();
  lay(p, 0, Wave::Saw, 0.55f); p.subLevel = 0.4f; p.mono = true;
  p.filterMode = FilterMode::LP12; p.cutoff = 1100.f; p.reso = 0.2f; p.envAmt = 0.3f; p.fenv = {0.003f, 0.25f, 0.3f, 0.1f};
  p.amp = {0.003f, 0.2f, 0.6f, 0.08f}; p.chorusMix = 0.1f; p.bassMonoHz = 200.f; p.lowCutHz = 30.f;
}
inline void hoover1991(Params& p) {
  p = Params();
  lay(p, 0, Wave::Pulse, 0.5f, 0, 0.f, 1, 0.f, 0.3f, -0.4f); lay(p, 1, Wave::Pulse, 0.5f, 0, 14.f, 1, 0.f, 0.35f, 0.4f); lay(p, 2, Wave::Pulse, 0.4f, -1, 0.f, 1, 0.f, 0.4f);
  p.mono = true; p.glide = 0.06f; p.pitchAmt = 12.f; p.pitchTime = 0.25f; p.lfoRate = 5.f; p.lfoPwm = 0.4f;
  p.cutoff = 5000.f; p.reso = 0.15f; p.dist = 0.4f; p.chorusMix = 0.3f; p.amp = {0.003f, 0.3f, 0.9f, 0.25f}; p.lowCutHz = 90.f; p.reverbMix = 0.1f;
}
inline void electroHouseBass2007(Params& p) {   // saws + pump (looping mod envelope, one beat at 128 bpm)
  p = Params();
  lay(p, 0, Wave::Saw, 0.5f, 0, -12.f); lay(p, 1, Wave::Saw, 0.45f, 0, 12.f); p.subLevel = 0.3f;
  p.cutoff = 600.f; p.reso = 0.25f; p.envAmt = 0.4f; p.fenv = {0.002f, 0.3f, 0.4f, 0.1f}; p.dist = 0.5f;
  p.amp = {0.002f, 0.3f, 0.9f, 0.1f}; p.spread = 0.2f; p.bassMonoHz = 250.f; p.lowCutHz = 30.f;
  p.menvTime = 0.469f; p.menvLoop = true; p.mx[S_MEN][D_VOL] = -0.85f;
}
inline void hardcoreReese(Params& p) {   // two single saws, plus and minus 15 cents, slow flanger-like chorus
  p = Params();
  lay(p, 0, Wave::Saw, 0.55f, 0, -15.f); lay(p, 1, Wave::Saw, 0.55f, 0, 15.f);
  p.filterMode = FilterMode::LP12; p.cutoff = 700.f; p.reso = 0.15f; p.mono = true;
  p.chorusMix = 0.5f; p.chorusRate = 0.08f; p.chorusDepth = 0.6f; p.amp = {0.003f, 0.3f, 0.9f, 0.15f}; p.bassMonoHz = 150.f; p.lowCutHz = 30.f;
}
inline void hardstyleReverseBass(Params& p) {
  p = Params();
  lay(p, 0, Wave::Saw, 0.6f, 0, 0.f); lay(p, 1, Wave::Sine, 0.5f); p.mono = true;
  p.cutoff = 900.f; p.reso = 0.2f; p.envAmt = 0.3f; p.fenv = {0.001f, 0.15f, 0.5f, 0.05f}; p.dist = 0.5f;
  p.amp = {0.001f, 0.2f, 0.8f, 0.05f}; p.spread = 0.f; p.bassMonoHz = 300.f; p.lowCutHz = 30.f;
}
inline void ukBasslineWarped(Params& p) {
  p = Params();
  lay(p, 0, Wave::Pulse, 0.6f, 0, 0.f, 1, 0.f, 0.5f); p.subLevel = 0.3f; p.mono = true;
  p.cutoff = 500.f; p.reso = 0.5f; p.envAmt = 0.5f; p.fenv = {0.001f, 0.15f, 0.4f, 0.08f}; p.drive = 0.3f; p.dist = 0.3f; p.distPre = true;
  p.lfoRate = 3.f; p.mx[S_LFO][D_PWM] = 0.3f; p.amp = {0.001f, 0.2f, 0.7f, 0.08f}; p.spread = 0.f; p.bassMonoHz = 300.f; p.lowCutHz = 30.f;
}
inline void techHouseSubPluck(Params& p) {
  p = Params();
  lay(p, 0, Wave::Sine, 0.8f); lay(p, 1, Wave::Tri, 0.3f, 1);
  p.pitchAmt = 3.f; p.pitchTime = 0.02f; p.cutoff = 500.f; p.reso = 0.12f; p.envAmt = 0.4f; p.fenv = {0.001f, 0.15f, 0.f, 0.06f}; p.dist = 0.1f;
  p.amp = {0.001f, 0.2f, 0.f, 0.06f}; p.spread = 0.f; p.bassMonoHz = 300.f;
}
inline void donkOrganBass(Params& p) {
  p = Params();
  lay(p, 0, Wave::Sine, 0.8f); lay(p, 1, Wave::Pulse, 0.25f, 1, 700.f, 1, 0.f, 0.5f); p.subLevel = 0.3f;
  p.cutoff = 600.f; p.reso = 0.15f; p.envAmt = 0.5f; p.fenv = {0.001f, 0.2f, 0.f, 0.08f}; p.amp = {0.001f, 0.22f, 0.f, 0.08f}; p.spread = 0.f; p.bassMonoHz = 300.f;
}
inline void dubstepTearGrowl(Params& p) {   // shark-fin mod envelope (smooth rise, hard drop) loops at 1/4 note and drives cutoff, detune and drive
  p = Params();
  lay(p, 0, Wave::Saw, 0.7f); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.35f;
  lay(p, 1, Wave::Sine, 0.f, 1, 700.f);   // silent FM modulator
  p.mono = true; p.cutoff = 500.f; p.reso = 0.3f; p.dist = 0.7f; p.lowCutHz = 30.f;
  for (int i = 0; i < 64; ++i) p.menv[i] = i < 48 ? std::sin(1.5707963f * (float)i / 47.f) : 0.f;
  p.menvTime = 0.5f; p.menvLoop = true; p.mx[S_MEN][D_CUT] = 0.7f; p.mx[S_MEN][D_DRV] = 0.4f;
  p.amp = {0.002f, 0.3f, 0.95f, 0.08f}; p.spread = 0.4f; p.bassMonoHz = 150.f;
}
inline void reeseTear(Params& p) {
  p = Params();
  lay(p, 0, Wave::Saw, 0.6f, -1); lay(p, 1, Wave::Pulse, 0.3f, -1, 5.f);
  p.mono = true; p.cutoff = 900.f; p.reso = 0.3f; p.dist = 0.85f; p.lfoRate = 2.f; p.lfoShape = 1; p.lfoCutoff = 0.5f; p.chorusMix = 0.4f;
  p.amp = {0.002f, 0.3f, 0.95f, 0.1f}; p.bassMonoHz = 150.f; p.lowCutHz = 30.f;
}

// ---------- drums and percussion ----------
inline void kick909(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Sine, 0.9f); p.pitchAmt = 30.f; p.pitchTime = 0.04f; p.dist = 0.35f; p.cutoff = 6000.f; p.lowCutHz = 25.f;
  p.amp = {0.001f, 0.45f, 0.f, 0.1f};
}
inline void gabberKick(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Sine, 0.9f); p.pitchAmt = 48.f; p.pitchTime = 0.06f; p.dist = 0.9f; p.drive = 0.4f; p.cutoff = 5000.f; p.lowCutHz = 25.f;
  p.amp = {0.001f, 0.28f, 0.f, 0.1f};
}
inline void snare909(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Tri, 0.5f, 0); p.pitchAmt = 12.f; p.pitchTime = 0.03f; p.noiseLevel = 1.5f; p.noiseHpHz = 1500.f;
  p.filterMode = FilterMode::BP; p.cutoff = 2500.f; p.reso = 0.2f; p.lowCutHz = 120.f; p.amp = {0.001f, 0.18f, 0.f, 0.08f};
}
inline void hat909Closed(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Pulse, 0.2f, 3); lay(p, 1, Wave::Pulse, 0.2f, 3, 530.f); lay(p, 2, Wave::Pulse, 0.2f, 3, 1050.f);
  p.noiseLevel = 1.2f; p.noiseHpHz = 7000.f; p.filterMode = FilterMode::HP; p.cutoff = 7000.f; p.lowCutHz = 400.f; p.amp = {0.001f, 0.05f, 0.f, 0.03f};
}
inline void hat909Open(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Pulse, 0.2f, 3); lay(p, 1, Wave::Pulse, 0.2f, 3, 530.f); lay(p, 2, Wave::Pulse, 0.2f, 3, 1050.f);
  p.noiseLevel = 1.2f; p.noiseHpHz = 6500.f; p.filterMode = FilterMode::HP; p.cutoff = 6500.f; p.lowCutHz = 400.f; p.amp = {0.001f, 0.3f, 0.05f, 0.25f};
}
inline void ride909(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Pulse, 0.2f, 3); lay(p, 1, Wave::Pulse, 0.2f, 3, 410.f); lay(p, 2, Wave::Pulse, 0.15f, 3, 1130.f);
  p.noiseLevel = 0.8f; p.noiseHpHz = 5000.f; p.filterMode = FilterMode::HP; p.cutoff = 5000.f; p.lowCutHz = 400.f; p.amp = {0.001f, 0.9f, 0.f, 0.6f};
}
inline void crash909(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Pulse, 0.2f, 3); lay(p, 1, Wave::Pulse, 0.2f, 3, 530.f); lay(p, 2, Wave::Pulse, 0.15f, 3, 1050.f);
  p.noiseLevel = 1.5f; p.noiseHpHz = 3500.f; p.filterMode = FilterMode::HP; p.cutoff = 3000.f; p.lowCutHz = 300.f; p.amp = {0.001f, 1.4f, 0.f, 1.2f};
  p.reverbMix = 0.15f; p.reverbSize = 0.6f;
}
inline void cowbell808(Params& p) {
  drumBase(p);
  lay(p, 0, Wave::Pulse, 0.5f, 1); lay(p, 1, Wave::Pulse, 0.5f, 1, 700.f);
  p.filterMode = FilterMode::BP; p.cutoff = 800.f; p.reso = 0.3f; p.dist = 0.1f; p.lowCutHz = 200.f; p.amp = {0.001f, 0.25f, 0.f, 0.06f};
}

// ---------- registry ----------

// ---------- FX: risers, sweeps, downlifters, impacts (recipes round 3). Times are in seconds; stretch the envelope to fit 4 or 8 bars. ----------
inline void noiseWashDown(Params& p) {   // white noise, LP12 closes from about 13 kHz to 450 Hz while it fades, 100 percent wet chorus keeps the centre free for the kick
  p = Params();
  p.noiseLevel = 1.f; p.noiseHpHz = 100.f; p.filterMode = FilterMode::LP12; p.cutoff = 450.f; p.reso = 0.12f; p.envAmt = 1.f; p.fenv = {0.001f, 3.5f, 0.f, 3.5f, 0.f, 0.f, 0.f};
  p.amp = {0.001f, 3.5f, 0.f, 3.5f, 0.f, -0.2f, -0.2f};   // release as long as the decay, so even a short tap plays the whole downlifter
  p.spread = 1.f; p.chorusMix = 1.f; p.chorusDepth = 0.6f; p.chorusWidth = 1.f; p.lowCutHz = 150.f;
  p.delayMix = 0.3f; p.delaySync = true; p.delayDiv = 4; p.delayFb = 0.45f; p.bpm = 140.f; p.delayPing = 1.f; p.master = 0.8f;
}
inline void clubCrushWash(Params& p) {   // white noise rising over about 7 s, a ramp LFO pumps it on every quarter note (140 BPM) to leave room for the kick
  p = Params();
  p.noiseLevel = 1.f; p.noiseHpHz = 100.f; p.filterMode = FilterMode::LP12; p.cutoff = 400.f; p.reso = 0.12f; p.envAmt = 1.f; p.fenv = {7.f, 0.1f, 1.f, 0.3f, 0.f, 0.f, -0.4f};
  p.amp = {1.f, 0.1f, 1.f, 0.6f}; p.lfoShape = 3; p.lfoRate = 2.333f; p.lfoAmp = 0.8f; p.spread = 1.f; p.chorusMix = 0.6f; p.lowCutHz = 150.f; p.master = 0.8f;
}
inline void subInverterRoll(Params& p) {   // dark noise roll: LP24 opens from 60 to about 400 Hz with 60 percent resonance (a moving sub bump). Noise high-pass is 100 Hz at the lowest.
  p = Params();
  p.noiseLevel = 1.f; p.noiseHpHz = 100.f; p.filterMode = FilterMode::LP24; p.cutoff = 60.f; p.reso = 0.6f; p.envAmt = 0.55f; p.fenv = {5.f, 0.1f, 1.f, 0.3f, 0.f, 0.f, -0.4f};
  p.amp = {1.f, 0.1f, 1.f, 0.4f}; p.spread = 0.f; p.bassMonoHz = 400.f; p.lowCutHz = 20.f;
}
inline void pitchScreamRiser(Params& p) {   // happy hardcore uplifter: stacked saws plus an octave saw climb three octaves in about 7 s, then snap back
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.55f, 0, 0.f, 4); p.layer[0].cents = 0.f; p.layer[0].detune = 0.15f; lay(p, 1, Wave::Saw, 0.35f, 1);
  p.pitchAmt = 36.f; p.pitchAttack = 7.f; p.pitchTime = 0.08f; p.filterMode = FilterMode::LP24; p.cutoff = 18000.f; p.reso = 0.1f; p.dist = 0.15f;
  p.amp = {0.05f, 0.1f, 1.f, 0.15f}; p.spread = 0.7f; p.chorusMix = 0.25f; p.delayMix = 0.15f; p.delaySync = true; p.delayDiv = 10; p.delayFb = 0.35f; p.bpm = 170.f; p.reverbMix = 0.12f; p.lowCutHz = 120.f;
}
inline void laserRollRiser(Params& p) {   // hardstyle laser roll: synced pulse, band-pass at 1.5 kHz, a falling 1/16 ramp on pitch and cutoff (140 BPM) while the base pitch climbs
  p = Params();
  lay(p, 0, Wave::Pulse, 0.7f, 0, 0.f, 1, 0.f, 0.5f); p.layer[0].sync = true; p.layer[0].syncSemis = 4.5f;
  p.filterMode = FilterMode::BP; p.cutoff = 1500.f; p.reso = 0.4f; p.lfoShape = 3; p.lfoRate = 9.333f; p.mx[S_LFO][D_PITCH] = -1.f; p.lfoCutoff = -0.25f;
  p.pitchAmt = 24.f; p.pitchAttack = 4.f; p.pitchTime = 0.05f; p.dist = 0.2f; p.amp = {0.01f, 0.1f, 1.f, 0.1f}; p.spread = 0.5f;
  p.delayMix = 0.15f; p.delaySync = true; p.delayDiv = 10; p.bpm = 140.f; p.lowCutHz = 150.f;
}
inline void fmLaserChop(Params& p) {   // psy-trance FM laser: sine carrier with a +3 octave sine modulator, 1/32 gate (140 BPM), pitch slides up two octaves
  p = Params();
  lay(p, 0, Wave::Sine, 0.8f); lay(p, 1, Wave::Sine, 0.f, 3); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.45f;
  p.lfoShape = 2; p.lfoRate = 18.667f; p.lfoAmp = 1.f; p.pitchAmt = 24.f; p.pitchAttack = 0.8f; p.pitchTime = 4.f;
  p.filterMode = FilterMode::LP24; p.cutoff = 12000.f; p.amp = {0.002f, 0.1f, 1.f, 0.08f}; p.spread = 0.4f; p.reverbMix = 0.1f; p.lowCutHz = 120.f;
}
inline void annihilatorAlarm(Params& p) {   // 90s rave alarm: square with FM from a saw, 12 Hz triangle LFO moves the pitch plus and minus an octave, clipped, 1/16 ping-pong
  p = Params();
  lay(p, 0, Wave::Pulse, 0.7f, 0, 0.f, 1, 0.f, 0.5f); lay(p, 1, Wave::Saw, 0.f, 1); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.5f;
  p.lfoShape = 1; p.lfoRate = 12.f; p.mx[S_LFO][D_PITCH] = 1.f; p.dist = 0.6f; p.filterMode = FilterMode::LP24; p.cutoff = 9000.f;
  p.delayMix = 0.25f; p.delaySync = true; p.delayDiv = 10; p.delayFb = 0.4f; p.bpm = 140.f; p.delayPing = 1.f; p.amp = {0.002f, 0.1f, 1.f, 0.1f}; p.spread = 0.8f; p.lowCutHz = 200.f;
}
inline void metalScrape(Params& p) {   // jungle metal scrape (no comb filter here): pulse plus noise into a very resonant band-pass swept by a ramp LFO on 1/4 notes, hard clipped
  p = Params();
  lay(p, 0, Wave::Pulse, 0.6f, 0, 0.f, 1, 0.f, 0.5f); p.noiseLevel = 0.35f; p.noiseHpHz = 1000.f;
  p.filterMode = FilterMode::BP; p.cutoff = 1200.f; p.reso = 0.9f; p.lfoShape = 3; p.lfoRate = 2.333f; p.lfoCutoff = 1.2f; p.dist = 0.85f; p.drive = 0.5f;
  p.amp = {0.002f, 0.1f, 1.f, 0.1f}; p.spread = 0.6f; p.lowCutHz = 150.f;
}
inline void tapeStopGlitch(Params& p) {   // complextro tape stop: hypersaw whose pitch falls 24 st and whose filter closes together, a little crunch
  p = Params();
  lay(p, 0, Wave::SuperSaw, 0.6f, 0, 0.f, 7); p.layer[0].cents = 0.f; p.layer[0].detune = 0.3f;
  p.pitchFall = 24.f; p.pitchFallTime = 0.5f; p.filterMode = FilterMode::LP24; p.cutoff = 500.f; p.envAmt = 1.f; p.fenv = {0.001f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f};
  p.amp = {0.001f, 1.4f, 0.f, 1.4f, 0.f, 0.f, -0.4f}; p.dist = 0.2f; p.spread = 0.8f; p.lowCutHz = 100.f;
}
inline void stretchedStringRiser(Params& p) {   // melodic techno string riser: two detuned triangles hard left and right, muffled filter opens over 6 s, dotted 1/8 delay into a big hall
  p = Params();
  lay(p, 0, Wave::Tri, 0.5f, 0, -25.f, 1, 0.f, 0.5f, -1.f); lay(p, 1, Wave::Tri, 0.5f, 0, 25.f, 1, 0.f, 0.5f, 1.f);
  p.filterMode = FilterMode::LP24; p.cutoff = 250.f; p.reso = 0.15f; p.envAmt = 0.85f; p.fenv = {6.f, 0.1f, 1.f, 0.5f}; p.amp = {1.5f, 0.1f, 1.f, 1.f}; p.drift = 0.3f; p.spread = 1.f;
  p.chorusMix = 0.5f; p.delayMix = 0.35f; p.delaySync = true; p.delayDiv = 8; p.delayFb = 0.55f; p.bpm = 124.f; p.reverbMix = 0.45f; p.reverbSize = 0.9f; p.reverbLowCutHz = 300.f;
}
inline void subDropImpact(Params& p) {   // clean sub boom: a sine falls 36 st (about 280 Hz to 35 Hz when played on C#1), no effects, low-passed
  p = Params();
  lay(p, 0, Wave::Sine, 1.f); p.pitchAmt = 36.f; p.pitchTime = 0.12f; p.filterMode = FilterMode::LP24; p.cutoff = 160.f; p.reso = 0.1f;
  p.amp = {0.001f, 1.5f, 0.f, 1.5f}; p.spread = 0.f; p.bassMonoHz = 500.f; p.lowCutHz = 20.f;
}
inline void subSuckSweeper(Params& p) {   // sub that climbs 24 st in 1.5 s (35 Hz to 140 Hz when played on C#1) under a 140 Hz low-pass; release the key one beat before the drop
  p = Params();
  lay(p, 0, Wave::Sine, 1.f); p.pitchAmt = 24.f; p.pitchAttack = 1.5f; p.pitchTime = 8.f; p.filterMode = FilterMode::LP24; p.cutoff = 140.f; p.reso = 0.1f;
  p.amp = {0.02f, 0.1f, 1.f, 0.05f}; p.spread = 0.f; p.bassMonoHz = 500.f; p.lowCutHz = 20.f;
}
inline void glassShrapnel(Params& p) {   // hardstyle glass impact: FM bell (sine with a +4 octave modulator), high-passed at 4 kHz, fast decay, fast chorus, 1/16 delay
  p = Params();
  lay(p, 0, Wave::Sine, 0.8f); lay(p, 1, Wave::Sine, 0.f, 4); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.42f;
  p.filterMode = FilterMode::HP; p.cutoff = 4000.f; p.reso = 0.1f; p.amp = {0.001f, 0.45f, 0.f, 0.45f}; p.spread = 1.f;
  p.chorusMix = 0.5f; p.chorusRate = 4.f; p.chorusDepth = 0.8f; p.delayMix = 0.25f; p.delaySync = true; p.delayDiv = 10; p.bpm = 150.f; p.reverbMix = 0.15f; p.lowCutHz = 200.f;
}
inline void chimeShardDown(Params& p) {   // neo-rave downlifter: FM chime that falls an octave while the high-pass closes from 8 to 2 kHz, wide wash and a huge tail
  p = Params();
  lay(p, 0, Wave::Sine, 0.8f); lay(p, 1, Wave::Sine, 0.f, 3); p.layer[0].fmSrc = 2; p.layer[0].fmAmt = 0.35f;
  p.pitchAmt = 12.f; p.pitchTime = 1.2f; p.filterMode = FilterMode::HP; p.cutoff = 2000.f; p.envAmt = 0.4f; p.fenv = {0.001f, 4.5f, 0.f, 4.5f, 0.f, 0.f, 0.f};
  p.amp = {0.001f, 4.5f, 0.f, 4.5f}; p.spread = 1.f; p.chorusMix = 0.4f; p.chorusRate = 0.4f; p.chorusDepth = 0.9f; p.reverbMix = 0.5f; p.reverbSize = 0.95f; p.reverbDamp = 0.2f; p.lowCutHz = 200.f;
}

struct RaveEntry { const char* name; void (*fn)(Params&); int note; float hold, total; };
inline const RaveEntry* raveList(int& n) {
  static const RaveEntry L[] = {
    {"moog_bass", moogBass, 36, 0.8f, 1.6f},
    {"virus_commercial", virusCommercial, 69, 1.5f, 3.0f}, {"hardstyle_stab", hardstyleStab, 57, 0.3f, 1.2f}, {"hyper_trance_chord", hyperTranceChord, 60, 1.5f, 3.0f},
    {"vowel_pluck", vowelPluck, 57, 0.3f, 1.5f}, {"minimal_pluck", minimalPluck, 69, 0.3f, 1.2f}, {"house_m1_piano", houseM1Piano, 60, 1.2f, 3.0f},
    {"hardcore_piano", hardcorePiano, 72, 1.0f, 2.5f}, {"dream_trance_piano", dreamTrancePiano, 64, 1.2f, 3.5f}, {"garage_4am_epiano", garage4amEpiano, 60, 0.5f, 1.5f},
    {"donk_organ_keys", donkOrganKeys, 60, 0.3f, 1.0f}, {"ice_bells", iceBells, 84, 0.3f, 2.5f}, {"pizz_pluck_bell", pizzPluckBell, 72, 0.15f, 1.2f},
    {"siren_screech", siren, 64, 1.5f, 3.0f}, {"bigroom_lead_2013", bigroomLead2013, 69, 1.2f, 2.5f}, {"raw_hardstyle_lead", rawHardstyleLead, 69, 1.2f, 2.5f},
    {"goa_acid_lead", goaAcidLead, 60, 1.0f, 2.0f}, {"melodic_techno_lead", melodicTechnoLead, 62, 1.5f, 3.0f}, {"hyperpop_pluck", hyperpopPluck, 72, 0.3f, 1.2f},
    {"rave_brass_2unl", rave2UnlimitedBrass, 60, 1.0f, 2.2f}, {"brute_brass", bruteBrass, 48, 1.0f, 2.2f}, {"hh_donk_pluck_lead", hhDonkPluckLead, 72, 0.4f, 1.2f},
    {"anjuna_pluck", anjunaPluck, 64, 0.4f, 1.8f}, {"vocal_trance_pluck", vocalTrancePluck2000s, 69, 0.4f, 1.8f}, {"complextro_laser", complextroLaser, 64, 0.5f, 1.2f},
    {"industrial_hammer", industrialHammer, 40, 0.3f, 1.2f}, {"glass_fm", glassFm, 76, 0.4f, 2.5f}, {"plastic_fm_organ", plasticFmOrgan, 60, 0.8f, 1.5f},
    {"rave_chab", raveChab, 60, 0.3f, 1.2f}, {"eurodance_chord_stack", eurodanceChordStack, 57, 1.0f, 2.2f}, {"future_bass_swell", futureBassSwell, 57, 2.0f, 3.5f},
    {"formant_pad_90s", formantPad90s, 60, 3.0f, 5.5f}, {"breakbeat_pad_92", breakbeatPad92, 60, 3.0f, 6.0f}, {"white_noise_riser", whiteNoiseRiser, 60, 3.5f, 5.0f},
    {"sub_donk_bass", subDonkBass, 36, 0.8f, 1.5f}, {"psy_rolling_bass", psyRollingBass, 36, 0.3f, 1.0f}, {"talking_yoi_bass", talkingYoiBass, 40, 0.5f, 1.2f},
    {"alpha909_screech", alpha909Screech, 52, 1.0f, 2.0f}, {"acid303_80s", acid303Classic80s, 36, 0.8f, 1.5f}, {"italo_bass", italoBass, 40, 0.8f, 1.5f},
    {"hoover_1991", hoover1991, 57, 1.2f, 2.5f}, {"electro_house_bass", electroHouseBass2007, 36, 1.2f, 2.2f}, {"hardcore_reese", hardcoreReese, 36, 1.2f, 2.2f},
    {"hardstyle_reverse_bass", hardstyleReverseBass, 36, 0.5f, 1.2f}, {"uk_bassline_warped", ukBasslineWarped, 36, 0.8f, 1.5f}, {"techhouse_sub_pluck", techHouseSubPluck, 36, 0.3f, 1.0f},
    {"donk_organ_bass", donkOrganBass, 36, 0.3f, 1.0f}, {"dubstep_tear_growl", dubstepTearGrowl, 36, 1.5f, 2.5f}, {"reese_tear", reeseTear, 40, 1.2f, 2.2f},
    {"kick_909", kick909, 36, 0.3f, 1.0f}, {"gabber_kick", gabberKick, 36, 0.3f, 1.0f}, {"snare_909", snare909, 60, 0.15f, 0.8f},
    {"hat_909_closed", hat909Closed, 60, 0.05f, 0.5f}, {"hat_909_open", hat909Open, 60, 0.1f, 1.0f}, {"ride_909", ride909, 60, 0.1f, 1.8f},
    {"crash_909", crash909, 60, 0.1f, 2.5f}, {"cowbell_808", cowbell808, 60, 0.1f, 0.8f},
    {"noise_wash_down", noiseWashDown, 60, 0.2f, 4.5f}, {"club_crush_wash", clubCrushWash, 60, 7.5f, 8.5f}, {"sub_inverter_roll", subInverterRoll, 60, 5.5f, 6.5f},
    {"pitch_scream_riser", pitchScreamRiser, 48, 7.5f, 8.5f}, {"laser_roll_riser", laserRollRiser, 60, 4.f, 5.f}, {"fm_laser_chop", fmLaserChop, 60, 1.2f, 2.f},
    {"annihilator_alarm", annihilatorAlarm, 69, 1.5f, 2.5f}, {"metal_scrape", metalScrape, 48, 2.f, 3.f}, {"tape_stop_glitch", tapeStopGlitch, 60, 0.3f, 1.6f},
    {"stretched_string_riser", stretchedStringRiser, 60, 6.5f, 9.f}, {"sub_drop_impact", subDropImpact, 25, 0.1f, 1.8f}, {"sub_suck_sweeper", subSuckSweeper, 25, 1.5f, 1.8f},
    {"glass_shrapnel", glassShrapnel, 72, 0.1f, 1.2f}, {"chime_shard_down", chimeShardDown, 84, 0.1f, 5.5f}};
  n = (int)(sizeof(L) / sizeof(L[0])); return L;
}
inline float raveGain(const std::string& name) {
  for (const auto& g : kRaveGain) if (name == g.name) return g.gain;
  return 1.f;
}
inline bool raveByName(const std::string& n, Params& p) {
  int c; const RaveEntry* L = raveList(c);
  for (int i = 0; i < c; ++i) if (n == L[i].name) { L[i].fn(p); p.master *= raveGain(n); return true; }
  return false;
}

}  // namespace supurr::presets
