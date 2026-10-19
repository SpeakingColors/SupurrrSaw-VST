// SupurrSaw DSP core. No JUCE dependency, header only, so it can be tested on its own.
// Licence: AGPL-3.0-or-later. Author: Studio 87
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace supurr {

constexpr int kMaxVoices = 32;  // hard limit; Params::polyphony sets how many are used
constexpr int kMaxSaws   = 15;  // unison voices per layer
constexpr int kLayers    = 3;
constexpr float kPi      = 3.14159265358979f;

enum class Wave { Off, SuperSaw, SuperSquare, Saw, Pulse, Tri, Sine, Sample, Custom };
enum class FilterMode { LP24, LP18, HP, BP, LP12, LP6 };   // LP12 and LP6 take the earlier taps of the same four pole ladder

// A loaded sample (WAV data already decoded). Shared between layers and voices, read only while playing.
struct Sample {
  std::vector<float> l, r;
  double sampleRate = 44100.0;
  int rootNote = 60;
  bool loop = false;
  bool oneShot = false;  // plays to the end, key release does not cut it (stabs)
  size_t loopStart = 0, loopEnd = 0;
};

// One imported or drawn cycle, stored as 11 band-limited copies (2048 samples, halving the harmonic count each step) so it never aliases.
// Build it outside the audio thread (setCycle), then share the pointer with the layers.
struct WaveTable {
  static constexpr int N = 2048, kMips = 11;
  std::vector<float> m[kMips];
  static void fft(std::vector<float>& re, std::vector<float>& im, bool inv) {
    const size_t n = re.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t b = n >> 1; for (; j & b; b >>= 1) j ^= b; j ^= b; if (i < j) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); } }
    for (size_t len = 2; len <= n; len <<= 1) {
      double ang = 2.0 * 3.14159265358979323846 / (double)len * (inv ? 1 : -1); double wr = std::cos(ang), wi = std::sin(ang);
      for (size_t i = 0; i < n; i += len) { double cr = 1, ci = 0;
        for (size_t k = 0; k < len / 2; ++k) { size_t a = i + k, b = i + k + len / 2;
          double tr = re[b] * cr - im[b] * ci, ti = re[b] * ci + im[b] * cr;
          re[b] = (float)(re[a] - tr); im[b] = (float)(im[a] - ti); re[a] = (float)(re[a] + tr); im[a] = (float)(im[a] + ti);
          double nr = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = nr; } } }
    if (inv) for (size_t i = 0; i < n; ++i) { re[i] /= (float)n; im[i] /= (float)n; }
  }
  // cyc: any length >= 8 (resampled to N with linear interpolation); DC removed; level set so the RMS matches a saw (0.577).
  void setCycle(const float* cyc, int len) {
    if (len < 8) return;
    std::vector<float> re(N), im(N, 0.f);
    for (int i = 0; i < N; ++i) { double x = (double)i * len / N; int i0 = (int)x; double f = x - i0; re[i] = (float)(cyc[i0 % len] * (1.0 - f) + cyc[(i0 + 1) % len] * f); }
    double dc = 0; for (float v : re) dc += v; dc /= N; double e = 0; for (float& v : re) { v -= (float)dc; e += (double)v * v; }
    float sc = e > 1e-12 ? (float)(0.577 / std::sqrt(e / N)) : 0.f; for (float& v : re) v *= sc;
    fft(re, im, false);
    for (int k = 0; k < kMips; ++k) {
      int H = (N / 2) >> k; if (H < 1) H = 1;   // highest harmonic kept in this copy
      std::vector<float> r2(N, 0.f), i2(N, 0.f);
      for (int h = 1; h <= H && h < N / 2; ++h) { r2[h] = re[h]; i2[h] = im[h]; r2[N - h] = re[N - h]; i2[N - h] = im[N - h]; }
      fft(r2, i2, true); m[k].assign(r2.begin(), r2.end()); m[k].push_back(m[k][0]);   // guard sample for interpolation
    }
  }
  bool ready() const { return !m[0].empty(); }
  // phase 0..1, d = phase increment per sample
  float read(float ph, float d) const {
    int k = 0; float lim = 0.45f / std::max(d, 1e-6f);   // harmonics that fit under Nyquist
    while (k < kMips - 1 && (float)((N / 2) >> k) > lim) ++k;
    const std::vector<float>& t = m[k]; float x = ph * N; int i = (int)x; float f = x - (float)i; if (i >= N) { i = 0; f = 0; }
    return t[i] * (1.f - f) + t[i + 1] * f;
  }
};

// seconds, sustain 0..1. ca/cd/cr bend the attack, decay and release: 0 is a straight line, +1 bulges outward (bell, away from the straight line),
// -1 bends inward (under the line, fast then slow). The decay and release defaults reproduce the earlier exponential feel.
struct Adsr { float a = 0.002f, d = 0.3f, s = 0.8f, r = 0.3f, ca = 0.f, cd = -0.4f, cr = -0.4f; };
// fraction of a move done after fraction f of its time; rising: outward = fast start, falling: outward = stays high longer
inline float envShape(float f, float c, bool rising) {
  float p = 1.f + 4.f * std::fabs(c);
  bool fastStart = rising ? (c >= 0.f) : (c < 0.f);
  return fastStart ? 1.f - std::pow(1.f - f, p) : std::pow(f, p);
}

struct LayerParams {
  Wave wave = Wave::Off;
  float level = 0.7f;
  int voices = 7;          // 1..15 for the super waves
  float detune = 0.4f;     // 0..1 (0 = one clean oscillator)
  float pwm = 0.5f;        // pulse width 0.05..0.95
  int octave = 0;          // -3..+3
  float tune = 0.f;        // cents
  float pan = 0.f;         // -1..1 for the whole layer
  // hard sync (Saw, Pulse, Tri, Sine layers): the master runs at the played pitch and restarts the slave every cycle.
  // The slave runs syncSemis above the master; sweeping it (envelope or LFO) makes the lead "talk".
  bool sync = false;
  float syncSemis = 7.f;       // 0..36 semitones above the master
  float syncEnv = 0.f;         // semitones added by the filter envelope
  float syncLfo = 0.f;         // semitones added by the LFO
  float cents = 0.f;           // > 0: super wave spread in cents (outer voices at +/- this), replaces the detune knob
  float lfoPitchCents = 0.f;   // LFO vibrato on this layer only, in cents
  float syncKey = 0.f;         // semitones added per octave above/below middle C (the sync opens more or less by note)
  float syncWheel = 0.f;       // semitones added at full mod wheel (automate or play it live)
  int fmSrc = 0;               // frequency modulation source: 0 none, 1..3 layer 1..3, 4 sub, 5 noise
  float fmAmt = 0.f;           // 0..1 depth (frequency deviation up to 3x the layer's own frequency)
  int amSrc = 0;               // amplitude modulation source, same list
  float amAmt = 0.f;           // 0..1; 1 is ring modulation, lower keeps some of the carrier
  std::shared_ptr<const Sample> sample;
  std::shared_ptr<const WaveTable> table;   // used by Wave::Custom (an imported or drawn cycle, played as a super stack)
};

struct Params {
  LayerParams layer[kLayers];
  // sub oscillator (always mono and centred)
  float subLevel = 0.f;
  Wave subWave = Wave::Sine;   // Sine, Tri, Pulse, Saw
  int subOctave = -1;          // -4..+4 octaves from the note (0 = same pitch)
  // filter
  FilterMode filterMode = FilterMode::LP24;
  float cutoff = 4000.f, reso = 0.2f, drive = 0.f, envAmt = 0.f, keyTrack = 0.f;
  bool subPreFilter = false;   // true: the sub oscillator goes through the filter like the other oscillators
  float glide = 0.f;           // seconds of portamento in mono mode (0 = off)
  bool mono = false;           // one voice, last note wins, a new note restarts the envelopes without a click
  int stackCount = 1;          // Stack mode (mono off): every key plays this many voices (1..4) spread by stackCents; 1 = plain poly
  float stackCents = 14.f;     // pitch spread of the stacked copies
  float lowCutHz = 20.f;       // removes low rumble before the sub is added
  float dist = 0.f;            // 0..1 distortion (acid, hoover, dirty reese, screech)
  bool distPre = false;        // true: distortion before the filter (the filter shapes the screech), false: after it
  float noiseLevel = 0.f;      // 0..1 white noise added before the filter
  float noiseHpHz = 3000.f;    // high-pass on the noise so it never muddies the low end
  Adsr amp, fenv;
  // pitch envelope (hoover dive): starts at +pitchAmt semitones, decays to 0
  float pitchAmt = 0.f, pitchTime = 0.09f, pitchAttack = 0.f;   // attack > 0: the pitch rises to pitchAmt over that time first, then decays
  // pitch fall (slow hoover bend down): drops by pitchFall semitones, reaching about 63% of it after pitchFallTime
  float pitchFall = 0.f, pitchFallTime = 1.f;
  // LFO
  float lfoRate = 6.f, lfoPwm = 0.f, lfoPitch = 0.f, lfoCutoff = 0.f;
  int lfoShape = 0;            // 0 sine, 1 triangle, 2 square
  float lfoPan = 0.f, lfoAmp = 0.f;   // LFO auto-pan (0..1 of the full width) and LFO volume tremolo (0..1 depth)
  float wheelCutoff = 0.f;     // cutoff added at full mod wheel, same units as envAmt (1 = 5 octaves)
  float velCutoff = 0.f;       // cutoff added at full velocity (1 = 5 octaves), 0 = velocity only changes loudness
  float wheelVib = 0.f;        // vibrato depth in cents added at full mod wheel (uses the LFO)
  bool phaseReset = false;     // true: oscillators start at phase 0 on every note (tight, repeatable)
  float lfoDetune = 0.f;       // 0..1: LFO moves the super saw detune (hoover shimmer)
  // analog feel and range
  float drift = 0.f;
  float detuneRange = 1.f;     // 1 = JP-8000 range. Raise it to push further.
  // stereo
  float spread = 0.7f;         // 0 = mono, 1 = full width
  float bassMonoHz = 120.f;    // everything below this is summed to mono
  // effects, all built to stay mono compatible (side-only chorus, mid/side reverb)
  float chorusMix = 0.f, chorusDepth = 0.5f, chorusRate = 0.6f;
  float delayMix = 0.f, delayTime = 0.25f, delayFb = 0.4f;
  bool delaySync = false;      // true: delay time follows the tempo (bpm) with delayDiv, false: delayTime in seconds
  int delayDiv = 7;            // index into kDelayBeats (7 = 1/8 note, 4 = 1/4 note)
  float bpm = 120.f;           // set by the host
  float reverbMix = 0.f, reverbSize = 0.6f, reverbWidth = 0.8f;   // reverbMix is a real dry/wet crossfade (0.1 = 10% wet)
  float reverbLowCutHz = 150.f;   // gentle 6 dB/oct high-pass on what goes into the reverb
  // dynamics after the effects: opto leveller, OTT three-band squash, glue squeeze, then the output limiter
  float optoAmt = 0.f;         // 0..1 how much the opto compressor works (0 = off)
  float ottDepth = 0.f;        // 0..1 amount of up/down compression in each band
  float ottMix = 0.f;          // 0..1 dry/wet of the OTT stage (0 = off)
  float sqzAmt = 0.f;          // 0..1 gentle one-band squeeze (0 = off)
  // output stage: master is a clean linear gain; a soft limiter and a smooth knee clipper keep peaks under 0 dBFS without harshness
  float master = 1.f;
  float clipKnee = 0.72f;      // level where the soft clipper starts to bend (linear below it)
  int polyphony = 12;          // voices available (1..kMaxVoices); oldest releasing voice is stolen first
  bool limiter = true;         // slow, smooth peak limiter in front of the clipper
  float limCeil = 0.9f;        // limiter ceiling (linear)
  float modWheel = 0.f;        // 0..1, set by the host (MIDI CC1)
  // Mastering: dynMix scales the whole opto/OTT/squeeze chain; dm* is the dance master chain (3 band compressor, clipper, dry/wet)
  float lfoDepth = 1.f, lfoFade = 0.f, lfoStart = 0.f;   // LFO master depth, fade-in time (s) and start phase (0..1); shapes: 0 sine, 1 tri, 2 square, 3 saw, 4 sample and hold
  float pitchHold = 0.f, pitchCurve = 0.f;               // pitch envelope: hold before the fall (s), 0 = exponential fall, 1 = linear fall
  // extra effect controls (defaults reproduce the original sound)
  float chorusDelay = 12.f, chorusWidth = 1.f;               // ms, 0..1
  float delayDamp = 0.7f, delayPing = 1.f;                   // darkness of the repeats, 0 straight .. 1 ping-pong
  float reverbPre = 0.f, reverbDamp = 0.25f;                 // ms, tail darkness
  float optoGain = 0.f, optoRel = 1.f;                       // dB, release scale
  float ottLow = 0.f, ottMid = 0.f, ottHigh = 0.f, ottTime = 1.f;   // band trims in dB, speed scale
  float sqzAtk = 4.f, sqzRel = 90.f, sqzMakeup = 0.f;        // ms, ms, dB
  // 6 band EQ at the end of the chain (before the output gain): low cut, low shelf, two bells, high shelf, high cut
  bool eq1On = false, eq2On = true, eq3On = true, eq4On = true, eq5On = true, eq6On = false;
  float eq1F = 30.f, eq2F = 120.f, eq2G = 0.f, eq3F = 500.f, eq3G = 0.f, eq3Q = 1.f, eq4F = 3000.f, eq4G = 0.f, eq4Q = 1.f, eq5F = 9000.f, eq5G = 0.f, eq6F = 18000.f;
  float drvDrive = 0.f, drvXs = 0.f, drvFuzz = 0.f, drvMix = 1.f;   // colour stack on the voice sum, before the effects (Drive, Transistor, Fuzz)
  float dynMix = 1.f, dmMix = 0.4f, dmDrive = 0.5f, dmClip = 0.35f, dmLow = 0.f, dmHigh = 0.f;
  // Full mod matrix: mx[source][destination], -1..1. Sources: LFO, filter env, amp env, pitch env, wheel, key, velocity, random, drawn mod envelope.
  // Destinations: pitch(12 st), cutoff(5 oct), reso, drive, pwm, detune, pan, volume, sync1..3 (12 st), sub, noise, low cut(4 oct).
  float mx[9][14] = {};   // row 8 is the drawn mod envelope
  float menv[64]; float menvTime = 0.5f; bool menvLoop = false, menvInv = false;   // drawn modulation envelope (0..1 over menvTime seconds, one shot or loop)
  Params() { for (int i = 0; i < 64; ++i) menv[i] = std::exp(-4.f * (float)i / 63.f); }
};

// smooth knee clipper: identity below T, then T + w*tanh((|x|-T)/w); C2-continuous at T, tops out at exactly 1
inline float softClip(float x, float T) {
  float a = std::fabs(x); if (a <= T) return x;
  float w = 1.f - T, y = T + w * std::tanh((a - T) / w); return x < 0.f ? -y : y;
}

// delay lengths in beats: 1/1, 1/2, 1/2 dotted, 1/2 triplet, 1/4, 1/4 dotted, 1/4 triplet, 1/8, 1/8 dotted, 1/8 triplet, 1/16, 1/16 dotted, 1/16 triplet, 1/32
static const float kDelayBeats[14] = {4.f, 2.f, 3.f, 4.f / 3.f, 1.f, 1.5f, 2.f / 3.f, 0.5f, 0.75f, 1.f / 3.f, 0.25f, 0.375f, 1.f / 6.f, 0.125f};
inline float delaySeconds(const Params& p) {
  float t = p.delaySync ? kDelayBeats[std::max(0, std::min(13, p.delayDiv))] * 60.f / std::max(20.f, std::min(400.f, p.bpm)) : p.delayTime;
  return std::max(0.02f, std::min(1.9f, t));
}

inline float midiToHz(float n) { return 440.f * std::exp2((n - 69.f) / 12.f); }

// JP-8000 detune knob response (Adam Szabo, 2010), same curve as HyperSaw
inline float jpDetune(float x) {
  float y = 10028.7312891634f;
  y = y * x - 50818.8652045924f;  y = y * x + 111363.4808729368f;
  y = y * x - 138150.6761080548f; y = y * x + 106649.6679158292f;
  y = y * x - 53046.9642751875f;  y = y * x + 17019.9518580080f;
  y = y * x - 3425.0836591318f;   y = y * x + 404.2703938388f;
  y = y * x - 24.1878824391f;     y = y * x + 0.6717417634f;
  y = y * x + 0.0030115596f;
  return std::min(1.f, std::max(0.f, y));
}
constexpr float kMaxDetune = 0.11f;  // outer voice at full detune
static const float kSpread15[15] = {-1.f, -.78f, -.58f, -.41f, -.26f, -.14f, -.05f, 0.f,
                                    .052f, .143f, .262f, .413f, .583f, .782f, 1.f};
// spread position of unison voice j of n (n voices, centre voice included when n is odd)
inline float sawSpread(int j, int n) {
  if (n <= 1) return 0.f;
  float t = (float)j * 14.f / (float)(n - 1);
  int i = std::min(13, (int)t);
  float f = t - (float)i;
  return kSpread15[i] * (1.f - f) + kSpread15[i + 1] * f;
}

inline float blep(float t, float dt) {
  if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
  if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
  return 0.f;
}

struct Rng { uint32_t s = 12345u; float next() { s = s * 1664525u + 1013904223u; return (float)(s >> 8) * (1.f / 16777216.f); } };

struct EnvState {
  float v = 0.f, v0 = 0.f; int stage = 0; float n = 0.f;  // 0 idle 1 attack 2 decay/sustain 3 release; n counts samples in the stage
  void on() { stage = 1; n = 0.f; v0 = v; }
  void off() { if (stage && stage != 3) { stage = 3; n = 0.f; v0 = v; } }
  float tick(const Adsr& e, float sr) {
    switch (stage) {
      case 1: { n += 1.f; float len = std::max(1.f, e.a * sr * (1.f - v0)); float f = n / len;
        if (f >= 1.f) { v = 1.f; stage = 2; n = 0.f; v0 = 1.f; } else v = v0 + (1.f - v0) * envShape(f, e.ca, true); break; }
      case 2: { n += 1.f; float f = n / std::max(1.f, e.d * sr); v = f >= 1.f ? e.s : v0 + (e.s - v0) * envShape(f, e.cd, false); break; }
      case 3: { n += 1.f; float f = n / std::max(1.f, e.r * sr); if (f >= 1.f) { v = 0.f; stage = 0; } else v = v0 * (1.f - envShape(f, e.cr, false)); break; }
      default: break;
    }
    return v;
  }
};

// 4-pole ladder (zero delay feedback) with LP24 and LP18 taps, plus a 12 dB state variable filter for HP and BP.
struct Filter {
  float s[4] = {0, 0, 0, 0}, z1 = 0, z2 = 0, inE = 0, outE = 0;   // inE/outE follow the level before and after the drive curve
  void reset() { s[0] = s[1] = s[2] = s[3] = z1 = z2 = inE = outE = 0; }
  float process(float x, FilterMode m, float cutoffHz, float reso, float drive, float sr) {
    float fc = std::min(cutoffHz, 0.45f * sr);
    float g = std::tan(kPi * fc / sr);
    if (m == FilterMode::LP24 || m == FilterMode::LP18 || m == FilterMode::LP12 || m == FilterMode::LP6) {
      float G = g / (1.f + g), G2 = G * G, G4 = G2 * G2;
      float k = 3.95f * reso;
      float S = G2 * G * s[0] / (1.f + g) + G2 * s[1] / (1.f + g) + G * s[2] / (1.f + g) + s[3] / (1.f + g);
      const float dg = 0.5f + 3.f * drive;   // soft input stage: unity small-signal gain, drive only adds curvature
      float in = std::tanh(x * dg) / dg;
      // drive adds harmonics, not quietness: follow the level lost in the curve (30 ms) and give it back
      const float fa = 1.f - std::exp(-1.f / (0.03f * sr));
      inE += fa * (std::fabs(x) - inE); outE += fa * (std::fabs(in) - outE);
      if (outE > 1e-5f && drive > 0.f) in *= std::min(4.f, std::max(1.f, inE / outE));
      float y4 = (G4 * in + S) / (1.f + k * G4);
      float u = in - k * y4;
      float y[4]; float xi = u;
      for (int i = 0; i < 4; ++i) { float v = G * (xi - s[i]); y[i] = v + s[i]; s[i] = y[i] + v; xi = y[i]; }
      return (m == FilterMode::LP24) ? y[3] : (m == FilterMode::LP18 ? y[2] : (m == FilterMode::LP12 ? y[1] : y[0]));
    }
    float R = 0.5f * (1.f - 0.95f * reso) + 0.02f;
    float hp = (x - (2.f * R + g) * z1 - z2) / (1.f + 2.f * R * g + g * g);
    float bp = g * hp + z1; z1 = g * hp + bp;
    float lp = g * bp + z2; z2 = g * bp + lp;
    (void)lp;
    return (m == FilterMode::HP) ? hp : bp * (2.f * R);
  }
};

struct Voice {
  bool active = false; int note = 60; float vel = 1.f; uint64_t age = 0;
  float phase[kLayers][kMaxSaws] = {};
  double samplePos[kLayers] = {};
  float gl = 0.f; float syncPhase[kLayers] = {}; float noiseHp = 0.f, noiseHp2 = 0.f;
  float rnd = 0.f, sh = 0.f; float subPhase = 0.f, drift[kMaxSaws] = {}, driftT[kMaxSaws] = {};
  float lfoPhase = 0.f, t = 0.f, stackCents = 0.f, stackGain = 1.f;
  float lyOut[kLayers] = {}, fmSubPh = 0.f, nzRaw = 0.f;   // last sample of each layer, FM/AM sources
  EnvState amp, fenv; Filter fl, fr; float hpL = 0.f, hpR = 0.f; bool oneShot = false;
};


// ---- effects ----
struct DelayLine {
  std::vector<float> b; size_t w = 0;
  void init(size_t n) { b.assign(n + 4, 0.f); w = 0; }
  void push(float x) { b[w] = x; if (++w >= b.size()) w = 0; }
  float read(float d) const {  // d samples back, linear interpolation
    float rp = (float)w - d; while (rp < 0) rp += (float)b.size(); if (rp >= (float)b.size()) rp -= (float)b.size(); size_t i0 = (size_t)rp; float f = rp - (float)i0;
    size_t i1 = i0 + 1 >= b.size() ? 0 : i0 + 1; return b[i0] * (1.f - f) + b[i1] * f; }
};
struct Reverb {
  struct Comb { std::vector<float> b; size_t i = 0; float lp = 0; float run(float x, float fb, float damp) { float o = b[i]; lp = o * (1.f - damp) + lp * damp; b[i] = x + lp * fb; if (++i >= b.size()) i = 0; return o; } };
  struct Ap { std::vector<float> b; size_t i = 0; float run(float x) { float o = b[i]; float y = -x + o; b[i] = x + o * 0.5f; if (++i >= b.size()) i = 0; return y; } };
  Comb c[2][8]; Ap a[2][4];
  void init(float sr) {
    const int cl[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617}, al[4] = {556, 441, 341, 225};
    float k = sr / 44100.f;
    for (int ch = 0; ch < 2; ++ch) { for (int j = 0; j < 8; ++j) { c[ch][j].b.assign((size_t)((cl[j] + ch * 23) * k), 0.f); c[ch][j].i = 0; c[ch][j].lp = 0; }
      for (int j = 0; j < 4; ++j) { a[ch][j].b.assign((size_t)((al[j] + ch * 11) * k), 0.f); a[ch][j].i = 0; } }
  }
  // input mono, outputs wet L/R
  void run(float in, float size, float damp, float& wl, float& wr) {
    float fb = 0.70f + 0.27f * size, o[2];
    for (int ch = 0; ch < 2; ++ch) { float s = 0; for (int j = 0; j < 8; ++j) s += c[ch][j].run(in * 0.15f, fb, damp); for (int j = 0; j < 4; ++j) s = a[ch][j].run(s); o[ch] = s; }
    wl = o[0]; wr = o[1];
  }
};
// ---- colour stack: Drive, Transistor, Fuzz (PurrOne stages), 2x oversampled with a 33 tap half-band, fixed 16 sample latency ----
inline float sstep(float t) { t = t < 0.f ? 0.f : t > 1.f ? 1.f : t; return t * t * (3.f - 2.f * t); }
inline float asymTanh(float u, float nl) { return u >= 0.f ? std::tanh(u) : nl * std::tanh(u / nl); }
inline float purrShaper(float x, float drive, float xs, float fuzz) {
  const float dn = sstep((drive - 0.8f) * 5.f), xn = sstep((xs - 0.8f) * 5.f), fn = sstep((fuzz - 0.8f) * 5.f); float y = x;
  if (drive > 0.001f) { y = std::tanh(x * (1.f + drive * 5.f + dn * 30.f)); if (dn > 0.f) y += dn * (std::tanh(y * (2.f + 10.f * dn)) - y); }
  if (xs > 0.001f) { const float nl = std::max(0.25f, 1.f - 0.3f * xs - 0.25f * xn), u = y * (1.f + xs * 5.f + xn * 30.f); y = asymTanh(u, nl); y += (0.3f * xs + 0.25f * xn) * y * y; if (xn > 0.f) { const float y2 = asymTanh(y * (2.f + 10.f * xn) + 0.15f * xn, nl); y += xn * (y2 - y); } }
  if (fuzz > 0.001f) { const float fz = 1.f + fuzz * 14.f + fn * 70.f, bf = 0.5f * std::min(1.f, fuzz); y = std::tanh(y * fz + bf) - std::tanh(bf); if (fn > 0.f) { const float r = std::sqrt(y * y + 0.02f) - 0.1414f; y += fn * 0.7f * (r - 0.3f); y = std::tanh(y * (1.f + 0.8f * fn)); } }
  return y;
}
inline float purrMakeup(float drive, float xs, float fuzz, float ref) { float a = 0.f, b = 0.f; for (int i = 0; i < 64; ++i) { const float x = ref * std::sin(2.f * kPi * (float)i / 64.f); a += x * x; const float y = purrShaper(x, drive, xs, fuzz); b += y * y; } const float m = b > 1e-9f ? std::sqrt(a / b) : 1.f; return std::min(4.f, std::max(0.25f, m)); }
struct HalfBand33 { double h[33], o[16];
  HalfBand33() { auto i0 = [](double x) { double s = 1, t = 1; for (int k = 1; k < 30; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; };
    for (int j = 0; j < 33; ++j) { double r = 2.0 * j / 32.0 - 1.0, w = i0(8.0 * std::sqrt(1.0 - r * r)) / i0(8.0); int k = j - 16; h[j] = (k == 0 ? 0.5 : std::sin(3.14159265358979323846 * k / 2.0) / (3.14159265358979323846 * k)) * w; }
    for (int k = 0; k < 16; ++k) o[k] = h[2 * k + 1]; } };
inline const HalfBand33& halfBand33() { static const HalfBand33 hb; return hb; }
struct DriveCh {
  float x[32] = {}, s[64] = {}, dd[16] = {}; int xi = 0, si = 0, di = 0; float dc = 0.f;
  float run(float v, bool on, float drive, float xs, float fuzz, float mk, float mix) {
    const HalfBand33& hb = halfBand33(); x[xi] = v; const int xc = xi; xi = (xi + 1) & 31;
    const float dry = dd[di]; dd[di] = v; di = (di + 1) & 15;
    if (!on) { si = (si + 1) & 63; s[si] = 0.f; si = (si + 1) & 63; s[si] = 0.f; dc = 0.f; return dry; }
    const float u0 = x[(xc - 8) & 31]; double u1 = 0; for (int k = 0; k < 16; ++k) u1 += hb.o[k] * x[(xc - k) & 31]; u1 *= 2.0;
    si = (si + 1) & 63; s[si] = purrShaper(u0 * 1.5f, drive, xs, fuzz) * mk / 1.5f;
    const int p = si; double y = 0.5 * s[(p - 16) & 63]; for (int k = 0; k < 16; ++k) y += hb.o[k] * s[(p - 2 * k - 1) & 63];
    si = (si + 1) & 63; s[si] = purrShaper((float)u1 * 1.5f, drive, xs, fuzz) * mk / 1.5f;
    dc += 0.0005f * ((float)y - dc); return dry * (1.f - mix) + ((float)y - dc) * mix; } };
struct DriveStage {
  DriveCh c[2]; float kd = -1.f, kx = -1.f, kf = -1.f, mk = 1.f; float sl[16] = {}, sr[16] = {}; int si = 0; float oL = 0.f, oR = 0.f, sL = 0.f, sR = 0.f;
  void process(const Params& p, float L, float R, float sbl, float sbr) {
    const float d = p.drvDrive, xs = p.drvXs, fz = p.drvFuzz; const bool on = d > 0.001f || xs > 0.001f || fz > 0.001f;
    if (on && (d != kd || xs != kx || fz != kf)) { kd = d; kx = xs; kf = fz; mk = purrMakeup(d, xs, fz, 0.75f); }
    oL = c[0].run(L, on, d, xs, fz, mk, p.drvMix); oR = c[1].run(R, on, d, xs, fz, mk, p.drvMix);
    sL = sl[si]; sR = sr[si]; sl[si] = sbl; sr[si] = sbr; si = (si + 1) & 15; } };
// ---- 6 band EQ: low cut, low shelf, 2 bells, high shelf, high cut (RBJ biquads, double precision) ----
enum class EqType { HP, LS, PK, HS, LP };
inline void eqCoef(EqType type, double f, double g, double q, double sr, double* o) {
  const double w = 2.0 * 3.14159265358979323846 * std::min(std::max(f, 10.0), 0.45 * sr) / sr, c = std::cos(w), sn = std::sin(w), al = sn / (2.0 * std::max(0.1, q));
  double b0, b1, b2, a0, a1, a2;
  if (type == EqType::HP) { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = b0; a0 = 1 + al; a1 = -2 * c; a2 = 1 - al; }
  else if (type == EqType::LP) { b0 = (1 - c) / 2; b1 = 1 - c; b2 = b0; a0 = 1 + al; a1 = -2 * c; a2 = 1 - al; }
  else { const double A = std::pow(10.0, g / 40.0);
    if (type == EqType::PK) { b0 = 1 + al * A; b1 = -2 * c; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * c; a2 = 1 - al / A; }
    else { const double be = 2 * std::sqrt(A) * al;
      if (type == EqType::LS) { b0 = A * ((A + 1) - (A - 1) * c + be); b1 = 2 * A * ((A - 1) - (A + 1) * c); b2 = A * ((A + 1) - (A - 1) * c - be); a0 = (A + 1) + (A - 1) * c + be; a1 = -2 * ((A - 1) + (A + 1) * c); a2 = (A + 1) + (A - 1) * c - be; }
      else { b0 = A * ((A + 1) + (A - 1) * c + be); b1 = -2 * A * ((A - 1) + (A + 1) * c); b2 = A * ((A + 1) + (A - 1) * c - be); a0 = (A + 1) - (A - 1) * c + be; a1 = 2 * ((A - 1) - (A + 1) * c); a2 = (A + 1) - (A - 1) * c - be; } } }
  o[0] = b0 / a0; o[1] = b1 / a0; o[2] = b2 / a0; o[3] = a1 / a0; o[4] = a2 / a0;
}
struct EqStage {
  double sr = 48000.0, z[24] = {}, co[30] = {}; bool act[6] = {}, any = false; float key[18] = {-1e9f}; float oL = 0.f, oR = 0.f;
  void prepare(double s) { sr = s; for (double& v : z) v = 0; for (float& k : key) k = -1e9f; any = false; }
  void update(const Params& p) {
    const float k[18] = {(float)p.eq1On, p.eq1F, (float)p.eq2On, p.eq2F, p.eq2G, (float)p.eq3On, p.eq3F, p.eq3G, p.eq3Q, (float)p.eq4On, p.eq4F, p.eq4G, p.eq4Q, (float)p.eq5On, p.eq5F, p.eq5G, (float)p.eq6On, p.eq6F};
    bool same = true; for (int i = 0; i < 18; ++i) if (k[i] != key[i]) same = false; if (same) return; for (int i = 0; i < 18; ++i) key[i] = k[i];
    const bool on[6] = {p.eq1On, p.eq2On, p.eq3On, p.eq4On, p.eq5On, p.eq6On}; const double f[6] = {p.eq1F, p.eq2F, p.eq3F, p.eq4F, p.eq5F, p.eq6F}, g[6] = {0, p.eq2G, p.eq3G, p.eq4G, p.eq5G, 0}, q[6] = {0.7071, 0.7071, p.eq3Q, p.eq4Q, 0.7071, 0.7071};
    const EqType t[6] = {EqType::HP, EqType::LS, EqType::PK, EqType::PK, EqType::HS, EqType::LP}; any = false;
    for (int b = 0; b < 6; ++b) { const bool gainBand = b >= 1 && b <= 4; act[b] = on[b] && !(gainBand && std::fabs(g[b]) < 0.01); if (act[b]) { any = true; eqCoef(t[b], f[b], g[b], q[b], sr, &co[b * 5]); } } }
  void process(float L, float R) {
    if (!any) { oL = L; oR = R; return; }
    double l = L, r = R;
    for (int b = 0; b < 6; ++b) { if (!act[b]) continue; const double* c = &co[b * 5]; double* zz = &z[b * 4];
      double y = c[0] * l + zz[0]; zz[0] = c[1] * l - c[3] * y + zz[1]; zz[1] = c[2] * l - c[4] * y; l = y;
      y = c[0] * r + zz[2]; zz[2] = c[1] * r - c[3] * y + zz[3]; zz[3] = c[2] * r - c[4] * y; r = y; }
    oL = (float)l; oR = (float)r; } };
struct Fx {
  float sr = 48000.f; DelayLine chor, dl, dr, pre; Reverb rev; float lfo = 0.f, dlp[2] = {0, 0}, rhp = 0.f, dtSm = -1.f;
  void prepare(float s) { sr = s; chor.init((size_t)(0.05f * s)); dl.init((size_t)(2.f * s)); dr.init((size_t)(2.f * s)); rev.init(s); pre.init((size_t)(0.26f * s)); lfo = 0.f; dlp[0] = dlp[1] = 0.f; rhp = 0.f; dtSm = -1.f; }
  void process(const Params& p, float& L, float& R) {
    float m = 0.5f * (L + R);
    if (p.chorusMix > 0.f) {  // side-only: the left and right wet signals are opposite, so they cancel in mono
      lfo += p.chorusRate / sr; if (lfo >= 1.f) lfo -= 1.f;
      chor.push(m); const float cd = p.chorusDelay * 0.001f; float d = (cd + 0.006f * p.chorusDepth * std::sin(2.f * kPi * lfo)) * sr;
      float d2 = (cd + 0.005f + 0.006f * p.chorusDepth * std::sin(2.f * kPi * (lfo + 0.5f))) * sr;
      // side = difference of two differently delayed copies: adds width without shifting the left/right balance
      float w = chor.read(d) - chor.read(d2);
      L += p.chorusMix * w; R -= p.chorusMix * w * p.chorusWidth;
    }
    if (p.delayMix > 0.f) {  // ping-pong, darker on every repeat
      float tt = delaySeconds(p) * sr;
      if (dtSm < 0.f) dtSm = tt; dtSm += (tt - dtSm) * (1.f - std::exp(-1.f / (0.06f * sr)));   // time changes glide like tape instead of clicking
      float t = dtSm;
      float a = dl.read(t), b = dr.read(t);
      const float dc = 1.f - 0.93f * p.delayDamp, pg = p.delayPing; dlp[0] += dc * (a - dlp[0]); dlp[1] += dc * (b - dlp[1]);
      dl.push(m + (pg * dlp[1] + (1.f - pg) * dlp[0]) * p.delayFb); dr.push((pg * dlp[0] + (1.f - pg) * dlp[1]) * p.delayFb + (1.f - pg) * m);
      L += p.delayMix * dlp[0]; R += p.delayMix * dlp[1];
    }
    if (p.reverbMix > 0.f) {  // mid and side are scaled separately: narrow centre, wide sides
      float ra = 1.f - std::exp(-2.f * kPi * std::max(20.f, p.reverbLowCutHz) / sr);
      rhp += ra * (m - rhp); float rin = m - rhp;
      float rin2 = rin; if (p.reverbPre > 0.f) { pre.push(rin); rin2 = pre.read(std::max(1.f, p.reverbPre * 0.001f * sr)); }
      float wl, wr; rev.run(rin2, p.reverbSize, p.reverbDamp, wl, wr);
      float wm = 0.5f * (wl + wr), ws = 0.5f * (wl - wr);
      float outM = wm * 0.8f, outS = ws * (0.4f + 1.2f * p.reverbWidth);
      const float k = 0.29f;   // wet level matches the dry level at mix = 1 (measured with noise), so the knob reads as % wet
      float mx = std::min(1.f, p.reverbMix);
      L = L * (1.f - mx) + mx * (outM + outS) * k; R = R * (1.f - mx) + mx * (outM - outS) * k;
    }
  }
};

// ---- dynamics after the effects ----
inline float dbToLin(float d) { return std::pow(10.f, d * 0.05f); }
inline float linToDb(float x) { return 20.f * std::log10(std::max(x, 1e-6f)); }
// 2nd-order Butterworth section; two in a row give a Linkwitz-Riley 4th-order crossover
struct Bq {
  float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  void set(bool hp, float fc, float sr) {
    float w = std::tan(kPi * std::min(fc, 0.45f * sr) / sr), q = 0.70710678f, n = 1.f / (1.f + w / q + w * w);
    if (hp) { b0 = n; b1 = -2.f * n; b2 = n; } else { b0 = w * w * n; b1 = 2.f * b0; b2 = b0; }
    a1 = 2.f * (w * w - 1.f) * n; a2 = (1.f - w / q + w * w) * n;
  }
  float tick(float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};
struct Lr4 { Bq a, b; void set(bool hp, float fc, float sr) { a.set(hp, fc, sr); b.set(hp, fc, sr); } float tick(float x) { return b.tick(a.tick(x)); } };

struct Dynamics {
  float sr = 48000.f;
  // opto (LA-2A style leveller): peak detector, soft knee 3:1, two release speeds (fast 60 ms, slow 1.5 s) so long compression lets go slowly
  float oEnv = 0.f, oFast = 0.f, oSlow = 0.f;
  // squeeze: one-band glue compressor
  float sEnv = 0.f, sGr = 0.f;
  float gr[3] = {0.f, 0.f, 0.f};   // meters: opto, OTT average and squeeze gain reduction in dB (for the interface)
  // OTT: three bands, each with a level follower; crossovers at 120 Hz and 2.5 kHz
  Lr4 lo[2], hiSplit1[2], midLo[2], hiHp[2], apLo1[2], apLo2[2];
  float bEnv[3] = {0, 0, 0}, bGain[3] = {1, 1, 1};
  void prepare(float s) {
    sr = s; oEnv = oFast = oSlow = sEnv = sGr = 0.f; for (int i = 0; i < 3; ++i) { bEnv[i] = 0.f; bGain[i] = 1.f; }
    for (int c = 0; c < 2; ++c) { lo[c] = Lr4(); hiSplit1[c] = Lr4(); midLo[c] = Lr4(); hiHp[c] = Lr4(); apLo1[c] = Lr4(); apLo2[c] = Lr4();
      lo[c].set(false, 120.f, sr); hiSplit1[c].set(true, 120.f, sr); midLo[c].set(false, 2500.f, sr); hiHp[c].set(true, 2500.f, sr);
      apLo1[c].set(false, 2500.f, sr); apLo2[c].set(true, 2500.f, sr); }
  }
  static float coef(float seconds, float sr) { return 1.f - std::exp(-1.f / std::max(1.f, seconds * sr)); }
  void process(const Params& p, float& L, float& R) {
    if (p.optoAmt > 0.f) {
      float pk = std::max(std::fabs(L), std::fabs(R));
      oEnv += (pk > oEnv ? coef(0.010f, sr) : coef(0.040f, sr)) * (pk - oEnv);
      float db = linToDb(oEnv), T = -4.f - 26.f * p.optoAmt, over = db - T, knee = 12.f, gr;
      if (over <= -knee * 0.5f) gr = 0.f; else if (over >= knee * 0.5f) gr = over * (1.f - 1.f / 3.f);
      else { float x = over + knee * 0.5f; gr = (1.f - 1.f / 3.f) * x * x / (2.f * knee); }
      oFast += (gr > oFast ? coef(0.010f, sr) : coef(0.060f * p.optoRel, sr)) * (gr - oFast);
      oSlow += (gr > oSlow ? coef(0.300f, sr) : coef(1.500f * p.optoRel, sr)) * (gr - oSlow);
      float g = dbToLin(-std::max(oFast, 0.8f * oSlow) + 0.45f * (-T) * (1.f - 1.f / 3.f) + p.optoGain);   // auto makeup
      L *= g; R *= g;
    }
    if (p.ottMix > 0.f) {
      float in[2] = {L, R}, band[2][3], depth = p.ottDepth;
      for (int c = 0; c < 2; ++c) {
        float low = lo[c].tick(in[c]), rest = hiSplit1[c].tick(in[c]);
        float mid = midLo[c].tick(rest), high = hiHp[c].tick(rest);
        float lowAp = apLo1[c].tick(low) + apLo2[c].tick(low);   // keep the low band in phase with the other two
        band[c][0] = lowAp; band[c][1] = mid; band[c][2] = high;
      }
      const float atk[3] = {0.0218f, 0.0135f, 0.0104f}, rel[3] = {0.247f, 0.132f, 0.072f}, tm = p.ottTime, trimB[3] = {p.ottLow, p.ottMid, p.ottHigh};   // seconds per band (mine, in the range such pedals use)
      float outL = 0.f, outR = 0.f;
      for (int b = 0; b < 3; ++b) {
        float pk = std::max(std::fabs(band[0][b]), std::fabs(band[1][b]));
        bEnv[b] += (pk > bEnv[b] ? coef(atk[b] * tm, sr) : coef(rel[b] * tm, sr)) * (pk - bEnv[b]);
        float db = linToDb(bEnv[b]), gdb = 0.f;
        const float Th = -14.f, Tl = -38.f, rD = 1.f + 5.f * depth, rU = 1.f + 3.f * depth;
        if (db > Th) gdb = -(db - Th) * (1.f - 1.f / rD);
        else if (db < Tl) gdb = std::min(18.f * depth, (Tl - db) * (1.f - 1.f / rU)) * std::min(1.f, std::max(0.f, (db + 80.f) / 20.f));   // no boosting of silence
        float tg = dbToLin(gdb + 3.f * depth + trimB[b]); bGain[b] += (tg - bGain[b]) * 0.05f;
        outL += band[0][b] * bGain[b]; outR += band[1][b] * bGain[b];
      }
      L = L * (1.f - p.ottMix) + outL * p.ottMix; R = R * (1.f - p.ottMix) + outR * p.ottMix;
    }
    if (p.sqzAmt > 0.f) {
      float pk = std::max(std::fabs(L), std::fabs(R));
      sEnv += (pk > sEnv ? coef(p.sqzAtk * 0.001f, sr) : coef(p.sqzRel * 0.001f, sr)) * (pk - sEnv);
      float db = linToDb(sEnv), T = -8.f - 14.f * p.sqzAmt, R_ = 1.f + 3.f * p.sqzAmt, over = db - T, knee = 6.f, gr;
      if (over <= -knee * 0.5f) gr = 0.f; else if (over >= knee * 0.5f) gr = over * (1.f - 1.f / R_);
      else { float x = over + knee * 0.5f; gr = (1.f - 1.f / R_) * x * x / (2.f * knee); }
      sGr += (gr - sGr) * 0.01f;
      float g = dbToLin(-sGr + 0.5f * (-T) * (1.f - 1.f / R_) + p.sqzMakeup);
      L *= g; R *= g;
    }
    gr[0] = p.optoAmt > 0.f ? std::max(oFast, 0.8f * oSlow) : 0.f;
    gr[1] = p.ottMix > 0.f ? 20.f * std::log10((bGain[0] + bGain[1] + bGain[2]) / 3.f + 1e-9f) : 0.f;
    gr[2] = p.sqzAmt > 0.f ? sGr : 0.f;
  }
};

struct Master {
  float sr = 48000.f, env[3] = {0, 0, 0}, gn[3] = {1, 1, 1}, gr[3] = {0, 0, 0}, xl = 0.f, xr = 0.f, outL = 0.f, outR = 0.f;
  Lr4 lo[2], hi1[2], midLo[2], hiHp[2], ap1[2], ap2[2];
  void prepare(float s) {
    sr = s; for (int i = 0; i < 3; ++i) { env[i] = 0.f; gn[i] = 1.f; gr[i] = 0.f; } xl = xr = 0.f;
    for (int c = 0; c < 2; ++c) { lo[c].set(false, 150.f, sr); hi1[c].set(true, 150.f, sr); midLo[c].set(false, 3000.f, sr); hiHp[c].set(true, 3000.f, sr); ap1[c].set(false, 3000.f, sr); ap2[c].set(true, 3000.f, sr); }
  }
  static float coef(float seconds, float sr) { return 1.f - std::exp(-1.f / std::max(1.f, seconds * sr)); }
  static float lncosh(float u) { float a = std::fabs(u); return a + std::log1p(std::exp(-2.f * a)) - 0.6931471805599453f; }
  static float clip(float x, float prev, float c) { float d = x - prev; if (std::fabs(d) < 1e-5f) return c * std::tanh(0.5f * (x + prev) / c); return c * c * (lncosh(x / c) - lncosh(prev / c)) / d; }
  void process(const Params& p, float L, float R) {
    const float mix = p.dmMix;
    if (!(mix > 0.f)) { gr[0] = gr[1] = gr[2] = 0.f; outL = L; outR = R; return; }
    float in[2] = {L, R}, band[2][3];
    for (int c = 0; c < 2; ++c) {
      float low = lo[c].tick(in[c]), rest = hi1[c].tick(in[c]), mid = midLo[c].tick(rest), high = hiHp[c].tick(rest);
      band[c][0] = ap1[c].tick(low) + ap2[c].tick(low); band[c][1] = mid; band[c][2] = high;
    }
    const float atk[3] = {0.014f, 0.007f, 0.0025f}, rel[3] = {0.16f, 0.09f, 0.05f}, T0[3] = {-9.f, -10.f, -12.f}, trim[3] = {p.dmLow, 0.f, p.dmHigh};
    const float R_ = 1.5f + 3.5f * p.dmDrive; float wl = 0.f, wr = 0.f;
    for (int b = 0; b < 3; ++b) {
      float pk = std::max(std::fabs(band[0][b]), std::fabs(band[1][b]));
      env[b] += (pk > env[b] ? coef(atk[b], sr) : coef(rel[b], sr)) * (pk - env[b]);
      float T = T0[b] - 12.f * p.dmDrive, over = linToDb(env[b]) - T, knee = 8.f, g;
      if (over <= -knee * 0.5f) g = 0.f; else if (over >= knee * 0.5f) g = over * (1.f - 1.f / R_); else { float x = over + knee * 0.5f; g = (1.f - 1.f / R_) * x * x / (2.f * knee); }
      gn[b] += (dbToLin(-g + 0.85f * (-T) * (1.f - 1.f / R_) + trim[b] * 6.f) - gn[b]) * 0.2f; gr[b] = g;
      wl += band[0][b] * gn[b]; wr += band[1][b] * gn[b];
    }
    const float dl0 = band[0][0] + band[0][1] + band[0][2], dr0 = band[1][0] + band[1][1] + band[1][2];
    const float gc = 1.f + p.dmClip * 1.2f, c = 0.85f, a = wl * gc, b2 = wr * gc;
    const float yl = clip(a, xl, c), yr = clip(b2, xr, c); xl = a; xr = b2;
    outL = dl0 * (1.f - mix) + yl * mix; outR = dr0 * (1.f - mix) + yr * mix;
  }
};

class Engine {
 public:
  Params p;
  void prepare(double sampleRate) { sr = (float)sampleRate; for (auto& v : voices) v = Voice(); drv = DriveStage(); eq.prepare(sr); bl = br = 0.f; bl2 = br2 = 0.f; limEnv = 0.f; limAtk = 1.f - std::exp(-1.f / (0.0004f * sr)); limRel = 1.f - std::exp(-1.f / (0.12f * sr)); fx.prepare(sr); dyn.prepare(sr); dm.prepare(sr); }
  void noteOn(int note, float vel) {
    if (p.mono) {  // reuse the one sounding voice so tails never pile up
      Voice* v = nullptr;
      for (auto& c : voices) if (c.active) { if (!v || c.age > v->age) v = &c; }
      if (v) { for (auto& c : voices) if (&c != v) c.active = false; v->gl += (float)(v->note - note); v->note = note; v->vel = vel; v->rnd = 2.f * rng.next() - 1.f; v->sh = v->rnd; v->age = ++clock; v->t = 0.f; v->amp.on(); v->fenv.on(); return; }
    }
    const int n = p.mono ? 1 : std::max(1, std::min(4, p.stackCount));   // Stack mode plays several detuned copies per key
    for (int k = 0; k < n; ++k) {
      Voice* v = allocVoice();
      *v = Voice(); v->active = true; v->note = note; v->vel = vel; v->rnd = 2.f * rng.next() - 1.f; v->sh = v->rnd; v->age = ++clock;
      v->stackCents = n > 1 ? ((float)k / (float)(n - 1) * 2.f - 1.f) * p.stackCents : 0.f; v->stackGain = 1.f / std::sqrt((float)n);
      for (int l = 0; l < kLayers; ++l) for (int j = 0; j < kMaxSaws; ++j) v->phase[l][j] = p.phaseReset ? 0.f : rng.next();
      v->amp.on(); v->fenv.on();
      for (int l = 0; l < kLayers; ++l) if (p.layer[l].wave == Wave::Sample && p.layer[l].sample && p.layer[l].sample->oneShot) v->oneShot = true;
    }
  }
  void noteOff(int note) { for (auto& v : voices) if (v.active && v.note == note && v.amp.stage != 3 && !v.oneShot) { v.amp.off(); v.fenv.off(); } }
  void allOff() { for (auto& v : voices) v.active = false; }
  struct Meters { float l = 0.f, r = 0.f, lim = 0.f; int active = 0, trips = 0; float gr[3] = {0, 0, 0}, dm[3] = {0, 0, 0}; };
  Meters meters() const { Meters m = mtr; m.lim = limEnv; m.trips = trips; for (int i = 0; i < 3; ++i) { m.gr[i] = dyn.gr[i]; m.dm[i] = dm.gr[i]; } return m; }
  void reserve(int maxBlock) { if ((int)subBusL.size() < maxBlock) { subBusL.assign(maxBlock, 0.f); subBusR.assign(maxBlock, 0.f); } }
  int activeVoices() const { int n = 0; for (const auto& v : voices) if (v.active) ++n; return n; }

  // adds nothing, overwrites outL/outR
  void render(float* outL, float* outR, int n) {
    for (int i = 0; i < n; ++i) outL[i] = outR[i] = 0.f;
    if ((int)subBusL.size() < n) { subBusL.assign(n, 0.f); subBusR.assign(n, 0.f); }
    std::fill(subBusL.begin(), subBusL.begin() + n, 0.f); std::fill(subBusR.begin(), subBusR.begin() + n, 0.f); sbL = subBusL.data(); sbR = subBusR.data();
    for (auto& v : voices) if (v.active) renderVoice(v, outL, outR, n);
    mtr.active = activeVoices();
    // bass mono: sum everything below bassMonoHz, keep the rest stereo
    eq.update(p); float a = 1.f - std::exp(-2.f * kPi * std::max(10.f, p.bassMonoHz) / sr); bool bad = false;
    for (int i = 0; i < n; ++i) {
      drv.process(p, outL[i], outR[i], sbL[i], sbR[i]); outL[i] = drv.oL; outR[i] = drv.oR; const float sbl = drv.sL, sbr = drv.sR;
      fx.process(p, outL[i], outR[i]);
      const float fl = outL[i], fr = outR[i]; float dl = fl, dr = fr;
      dyn.process(p, dl, dr);
      dl = fl + (dl - fl) * p.dynMix; dr = fr + (dr - fr) * p.dynMix;
      bl += a * (dl - bl); bl2 += a * (bl - bl2);
      br += a * (dr - br); br2 += a * (br - br2);
      float m = 0.5f * (bl2 + br2);
      dm.process(p, dl - bl2 + m, dr - br2 + m);
      eq.process(dm.outL + sbl, dm.outR + sbr); float xl = eq.oL * p.master, xr = eq.oR * p.master;
      if (p.limiter) {   // feed-forward peak limiter, 0.4 ms attack, 120 ms release; overshoots are caught by the smooth clipper
        float pk = std::max(std::fabs(xl), std::fabs(xr));
        limEnv += (pk > limEnv ? limAtk : limRel) * (pk - limEnv);
        if (limEnv > p.limCeil) { float g = p.limCeil / limEnv; xl *= g; xr *= g; }
      }
      outL[i] = softClip(xl, p.clipKnee); outR[i] = softClip(xr, p.clipKnee);
      if (!std::isfinite(outL[i] + outR[i])) bad = true;
    }
    if (bad) {   // safety guard: a bad value anywhere resets the engine instead of blasting or hanging the host
      ++trips; for (auto& v : voices) v = Voice(); drv = DriveStage(); eq.prepare(sr); fx.prepare(sr); dyn.prepare(sr); dm.prepare(sr); bl = br = bl2 = br2 = limEnv = 0.f;
      for (int i = 0; i < n; ++i) outL[i] = outR[i] = 0.f;
    }
    { float pl = 0.f, pr = 0.f; for (int i = 0; i < n; ++i) { pl = std::max(pl, std::fabs(outL[i])); pr = std::max(pr, std::fabs(outR[i])); } mtr.l = pl; mtr.r = pr; }
  }

 private:
  Voice* allocVoice() {
    Voice* v = nullptr; const int np = std::max(1, std::min(p.polyphony, kMaxVoices));
    for (int i = 0; i < np; ++i) if (!voices[i].active) { v = &voices[i]; break; }
    if (!v) {   // steal: the oldest voice that is already releasing, else the oldest of all
      for (int i = 0; i < np; ++i) if (voices[i].amp.stage == 3 && (!v || voices[i].age < v->age)) v = &voices[i];
      if (!v) { v = &voices[0]; for (int i = 0; i < np; ++i) if (voices[i].age < v->age) v = &voices[i]; }
    }
    return v;
  }
  float sr = 48000.f; Voice voices[kMaxVoices]; Rng rng; DriveStage drv; EqStage eq; Fx fx; Dynamics dyn; Master dm; uint64_t clock = 0;
  int trips = 0;   // how many times the safety guard reset the engine
  Meters mtr;
  std::vector<float> subBusL, subBusR; float* sbL = nullptr; float* sbR = nullptr;
  float bl = 0, br = 0, bl2 = 0, br2 = 0, limEnv = 0, limAtk = 0.05f, limRel = 0.0002f;

  // sub oscillator (mono, centred). After the filter by default so the low end stays solid; before it when subPreFilter is set.
  void addSub(Voice& v, float& L, float& R, float invSr, float lvl) {
    if (lvl <= 0.f) return;
    float sf = 440.f * std::exp2(((float)(v.note - 69) + (float)p.subOctave * 12.f) / 12.f);
    float sdt = sf * invSr; if (sdt > 0.45f) return; float& sp = v.subPhase; float s;
    switch (p.subWave) {
      case Wave::Tri: s = 4.f * std::fabs(sp - 0.5f) - 1.f; break;
      case Wave::Pulse: s = (sp < 0.5f ? 1.f : -1.f) + blep(sp, sdt) - blep(sp < 0.5f ? sp + 0.5f : sp - 0.5f, sdt); s *= 0.8f; break;
      case Wave::Saw: s = (2.f * sp - 1.f) - blep(sp, sdt); break;
      default: s = std::sin(2.f * kPi * sp); break;
    }
    sp += sdt; if (sp >= 1.f) sp -= 1.f;
    L += lvl * s * 0.7f; R += lvl * s * 0.7f;
  }

  void renderVoice(Voice& v, float* oL, float* oR, int n) {
    const float invSr = 1.f / sr;
    int mxs[126], mxd[126]; float mxv[126]; int mxn = 0;   // sparse list of the non-zero matrix cells
    for (int a = 0; a < 9; ++a) for (int b = 0; b < 14; ++b) if (p.mx[a][b] != 0.f) { mxs[mxn] = a; mxd[mxn] = b; mxv[mxn] = p.mx[a][b]; ++mxn; }
    float md[14] = {}, sv[9];
    bool usedAsSrc[kLayers] = {}, anyFmSub = false, anyFmNz = false;
    for (int ly = 0; ly < kLayers; ++ly) { const LayerParams& q = p.layer[ly]; if (q.wave == Wave::Off || q.level <= 0.f) continue;
      if (q.fmAmt > 0.f && q.fmSrc >= 1 && q.fmSrc <= 3) usedAsSrc[q.fmSrc - 1] = true; if (q.amAmt > 0.f && q.amSrc >= 1 && q.amSrc <= 3) usedAsSrc[q.amSrc - 1] = true;
      if ((q.fmAmt > 0.f && q.fmSrc == 4) || (q.amAmt > 0.f && q.amSrc == 4)) anyFmSub = true; if ((q.fmAmt > 0.f && q.fmSrc == 5) || (q.amAmt > 0.f && q.amSrc == 5)) anyFmNz = true; }
    // per-block tables for the supersaw layers (pan law and spread depend only on settings, so they are not recomputed per sample)
    float tSp[kLayers][kMaxSaws], tCl[kLayers][kMaxSaws], tCr[kLayers][kMaxSaws]; bool tOn[kLayers][kMaxSaws];
    for (int ly = 0; ly < kLayers; ++ly) { const LayerParams& q = p.layer[ly];
      if (!(q.wave == Wave::SuperSaw || q.wave == Wave::SuperSquare || (q.wave == Wave::Custom && q.table && q.table->ready()))) continue;
      const int nv = std::max(1, std::min(kMaxSaws, q.voices)); const float sideFade = q.cents > 0.f ? 1.f : std::min(1.f, q.detune * 20.f), norm = 1.f / (1.f + (float)(nv - 1) * 0.62f * sideFade);
      for (int j = 0; j < nv; ++j) { const float sp = sawSpread(j, nv); const bool centre = (nv % 2 == 1 && j == nv / 2); float g = centre ? 1.f : 0.62f * sideFade; g *= norm;
        const float ang = (sp * p.spread + 1.f) * kPi * 0.25f; tSp[ly][j] = sp; tOn[ly][j] = g > 0.f; tCl[ly][j] = g * std::cos(ang) * 1.414f; tCr[ly][j] = g * std::sin(ang) * 1.414f; } }
    for (int i = 0; i < n; ++i) {
      float ampE = v.amp.tick(p.amp, sr), fE = v.fenv.tick(p.fenv, sr);
      if (v.amp.stage == 0) { v.active = false; return; }
      v.lfoPhase += p.lfoRate * invSr; if (v.lfoPhase >= 1.f) { v.lfoPhase -= 1.f; v.sh = 2.f * rng.next() - 1.f; }
      float lph = v.lfoPhase + p.lfoStart; if (lph >= 1.f) lph -= 1.f;
      float lfo = (p.lfoShape == 1 ? 4.f * std::fabs(lph - 0.5f) - 1.f : (p.lfoShape == 2 ? (lph < 0.5f ? 1.f : -1.f) : (p.lfoShape == 3 ? 2.f * lph - 1.f : (p.lfoShape == 4 ? v.sh : std::sin(2.f * kPi * lph))))) * p.lfoDepth * (p.lfoFade > 0.f ? std::min(1.f, v.t / p.lfoFade) : 1.f);
      float pAtt = p.pitchAttack > 0.f ? std::min(1.f, v.t / p.pitchAttack) : 1.f;
      const float pd = std::max(0.f, v.t - p.pitchAttack - p.pitchHold), pt = std::max(0.001f, p.pitchTime);
      float pitchEnv = p.pitchAmt * pAtt * ((1.f - p.pitchCurve) * std::exp(-pd / pt) + p.pitchCurve * std::max(0.f, 1.f - pd / (3.f * pt))) - p.pitchFall * (1.f - std::exp(-v.t / std::max(0.01f, p.pitchFallTime)));
      v.t += invSr;
      if (v.gl != 0.f) { v.gl *= std::exp(-invSr / std::max(0.001f, p.glide)); if (std::fabs(v.gl) < 1e-4f) v.gl = 0.f; }
      if ((int)(v.t * sr) % 512 == 0)
        for (int j = 0; j < kMaxSaws; ++j) v.driftT[j] = p.drift * 0.004f * (2.f * rng.next() - 1.f);
      for (int j = 0; j < kMaxSaws; ++j) v.drift[j] += 0.0005f * (v.driftT[j] - v.drift[j]);

      if (mxn) {
        for (int k = 0; k < 14; ++k) md[k] = 0.f;
        sv[0] = lfo; sv[1] = fE; sv[2] = ampE; sv[3] = std::max(-2.f, std::min(2.f, pitchEnv / 12.f)); sv[4] = p.modWheel;
        sv[5] = std::max(-2.f, std::min(2.f, (float)(v.note - 60) / 24.f)); sv[6] = v.vel; sv[7] = v.rnd;
        { const float T = std::max(0.02f, p.menvTime); float x = v.t / T; if (p.menvLoop) x -= std::floor(x); else if (x > 1.f) x = 1.f; const float fp = x * 63.f; const int i0 = std::min(62, (int)fp); sv[8] = p.menv[i0] + (p.menv[i0 + 1] - p.menv[i0]) * (fp - (float)i0); if (p.menvInv) sv[8] = 1.f - sv[8]; }
        for (int k = 0; k < mxn; ++k) md[mxd[k]] += sv[mxs[k]] * mxv[k];
      }
      float L = 0.f, R = 0.f;
      float srcV[6] = {0.f, v.lyOut[0], v.lyOut[1], v.lyOut[2], 0.f, 0.f};
      if (anyFmSub) { srcV[4] = std::sin(2.f * kPi * v.fmSubPh); v.fmSubPh += 440.f * std::exp2(((float)(v.note - 69) + (float)p.subOctave * 12.f) / 12.f) * invSr; if (v.fmSubPh >= 1.f) v.fmSubPh -= 1.f; }
      if (anyFmNz) { v.nzRaw = 2.f * rng.next() - 1.f; srcV[5] = v.nzRaw; }
      for (int ly = 0; ly < kLayers; ++ly) {
        const LayerParams& lp = p.layer[ly];
        if (lp.wave == Wave::Off || (lp.level <= 0.f && !usedAsSrc[ly])) { v.lyOut[ly] = 0.f; continue; }
        float semis = (float)(v.note - 69) + v.gl + (float)lp.octave * 12.f + lp.tune * 0.01f + pitchEnv + p.lfoPitch * lfo * 0.3f + lp.lfoPitchCents * 0.01f * lfo + v.stackCents * 0.01f + p.wheelVib * p.modWheel * 0.01f * lfo + md[0] * 12.f;
        float f = 440.f * std::exp2(semis / 12.f);
        float fmF = 1.f; if (lp.fmAmt > 0.f && lp.fmSrc >= 1 && lp.fmSrc <= 5 && lp.fmSrc != ly + 1) fmF = std::max(0.02f, 1.f + lp.fmAmt * 3.f * srcV[lp.fmSrc]);
        float dt = std::min(0.45f, f * invSr * fmF);
        float lL = 0.f, lR = 0.f;
        if (lp.wave == Wave::SuperSaw || lp.wave == Wave::SuperSquare || (lp.wave == Wave::Custom && lp.table && lp.table->ready())) {
          int nv = std::max(1, std::min(kMaxSaws, lp.voices));
          float det = lp.cents > 0.f ? std::exp2(std::max(0.f, lp.cents + md[5] * 50.f) / 1200.f) - 1.f : jpDetune(std::min(1.f, std::max(0.f, lp.detune * (1.f + p.lfoDetune * lfo) + md[5] * 0.5f))) * kMaxDetune * p.detuneRange;
          float pw = std::min(0.95f, std::max(0.05f, lp.pwm + p.lfoPwm * lfo * 0.45f + md[4] * 0.45f));
          for (int j = 0; j < nv; ++j) {
            if (!tOn[ly][j]) continue;
            const float sp = tSp[ly][j];
            float d = std::min(0.45f, dt * (1.f + det * sp + v.drift[j]));
            float& ph = v.phase[ly][j];
            float s;
            if (lp.wave == Wave::SuperSaw) s = (2.f * ph - 1.f) - blep(ph, d);
            else if (lp.wave == Wave::Custom) s = lp.table->read(ph, d);
            else {
              s = (ph < pw ? 1.f : -1.f) + blep(ph, d);
              float t2 = ph - pw; if (t2 < 0.f) t2 += 1.f;
              s -= blep(t2, d);
              s *= 0.8f;
            }
            ph += d; if (ph >= 1.f) ph -= 1.f;
            lL += tCl[ly][j] * s; lR += tCr[ly][j] * s;
          }
        } else if (lp.wave == Wave::Sample && lp.sample && !lp.sample->l.empty()) {
          const Sample& sm = *lp.sample;
          double rate = (double)(f / midiToHz((float)sm.rootNote)) * sm.sampleRate / sr;
          double& pos = v.samplePos[ly];
          size_t N = sm.l.size();
          if (sm.loop && sm.loopEnd > sm.loopStart && pos >= (double)sm.loopEnd) pos -= (double)(sm.loopEnd - sm.loopStart);
          if (pos < (double)N - 2) {
            size_t i0 = (size_t)pos; float fr = (float)(pos - (double)i0);
            auto h = [&](const std::vector<float>& a) {  // cubic hermite
              float y0 = a[i0 ? i0 - 1 : 0], y1 = a[i0], y2 = a[i0 + 1], y3 = a[std::min(N - 1, i0 + 2)];
              float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
              return ((c3 * fr + c2) * fr + c1) * fr + y1;
            };
            float sl = h(sm.l), sr_ = h(sm.r.empty() ? sm.l : sm.r);
            // width control also applies to samples (mid/side)
            float mid = 0.5f * (sl + sr_), side = 0.5f * (sl - sr_) * p.spread;
            lL = mid + side; lR = mid - side;
            pos += rate;
          }
        } else {
          float& ph = v.phase[ly][0];
          float pw = std::min(0.95f, std::max(0.05f, lp.pwm + p.lfoPwm * lfo * 0.45f + md[4] * 0.45f));
          // with sync on, the slave oscillator makes the sound and the master only restarts it
          float& sph = v.syncPhase[ly];
          float odt = dt;
          if (lp.sync) {
            float sem = std::min(48.f, std::max(0.f, lp.syncSemis + lp.syncEnv * fE + lp.syncLfo * lfo + lp.syncKey * (float)(v.note - 60) / 12.f + lp.syncWheel * p.modWheel + md[8 + ly] * 12.f));
            odt = std::min(0.45f, dt * std::exp2(sem / 12.f));
          }
          float& op = lp.sync ? sph : ph;
          auto shape = [&](float x, float d) {
            float w = 0.f;
            switch (lp.wave) {
              case Wave::Saw:   w = (2.f * x - 1.f) - blep(x, d); break;
              case Wave::Pulse: { w = (x < pw ? 1.f : -1.f) + blep(x, d); float t2 = x - pw; if (t2 < 0) t2 += 1.f; w -= blep(t2, d); w *= 0.8f; } break;
              case Wave::Tri:   w = 4.f * std::fabs(x - 0.5f) - 1.f; break;
              default:          w = std::sin(2.f * kPi * x); break;
            }
            return w;
          };
          float s = shape(op, odt);
          bool wrapped = false; ph += dt; if (ph >= 1.f) { ph -= 1.f; wrapped = true; }
          if (lp.sync) {
            float before = s;
            sph += odt;
            if (wrapped) {  // master wrapped this sample: restart the slave at the exact fractional position
              float frac = ph / dt;                       // 0..1, how far past the wrap we already are
              sph = frac * odt;
              float after = shape(sph, odt);
              s = before * frac + after * (1.f - frac);   // short crossfade over the step: first-order band limiting
            } else if (sph >= 1.f) sph -= 1.f;
          }
          lL = lR = s;
        }
        v.lyOut[ly] = 0.5f * (lL + lR);
        if (lp.amAmt > 0.f && lp.amSrc >= 1 && lp.amSrc <= 5 && lp.amSrc != ly + 1) { const float am = 1.f - lp.amAmt + lp.amAmt * srcV[lp.amSrc]; lL *= am; lR *= am; }
        float pg = (std::min(1.f, std::max(-1.f, lp.pan + p.lfoPan * lfo + md[6])) + 1.f) * kPi * 0.25f;
        L += lp.level * lL * std::cos(pg) * 1.414f; R += lp.level * lR * std::sin(pg) * 1.414f;
      }
      const float nzl = std::max(0.f, p.noiseLevel + md[12] * 0.5f), subL = std::max(0.f, p.subLevel + md[11] * 0.7f);
      if (nzl > 0.f) {  // white noise, high-passed (two poles) so only the top end is added
        float n = (2.f * rng.next() - 1.f);
        float a = 1.f - std::exp(-2.f * kPi * std::min(0.45f * sr, std::max(100.f, p.noiseHpHz)) / sr);
        v.noiseHp += a * (n - v.noiseHp); float h1 = n - v.noiseHp;
        v.noiseHp2 += a * (h1 - v.noiseHp2); float h2 = h1 - v.noiseHp2;
        L += nzl * 0.5f * h2; R += nzl * 0.5f * h2;
      }
      auto distort = [&](float& l, float& r) {
        if (p.dist <= 0.f) return;
        float k = 1.f + p.dist * 14.f, norm = 1.f / std::tanh(k * 0.6f);
        l = std::tanh(k * l + 0.15f * p.dist) * norm * 0.8f; r = std::tanh(k * r + 0.15f * p.dist) * norm * 0.8f; };
      if (p.subPreFilter) addSub(v, L, R, invSr, subL);
      if (p.distPre) distort(L, R);
      // filter
      float cut = p.cutoff * std::exp2(p.envAmt * fE * 5.f + p.keyTrack * (float)(v.note - 60) / 12.f + p.lfoCutoff * lfo * 2.f + p.wheelCutoff * p.modWheel * 5.f + p.velCutoff * v.vel * 5.f + md[1] * 5.f);
      cut = std::min(18000.f, std::max(20.f, cut));
      const float rs = std::min(1.f, std::max(0.f, p.reso + md[2] * 0.5f)), dv = std::min(1.f, std::max(0.f, p.drive + md[3] * 0.75f));
      L = v.fl.process(L, p.filterMode, cut, rs, dv, sr);
      R = v.fr.process(R, p.filterMode, cut, rs, dv, sr);
      { // low cut, then distortion
        float a = 1.f - std::exp(-2.f * kPi * std::max(5.f, std::min(2000.f, p.lowCutHz * std::exp2(md[13] * 4.f))) / sr);
        v.hpL += a * (L - v.hpL); v.hpR += a * (R - v.hpR); L -= v.hpL; R -= v.hpR;
        if (!p.distPre) distort(L, R);
      }
      float subS = 0.f; if (!p.subPreFilter && subL > 0.f) { float t2l = 0.f, t2r = 0.f; addSub(v, t2l, t2r, invSr, subL); subS = t2l; }
      float g = ampE * (0.4f + 0.6f * v.vel) * 1.3f * v.stackGain * (1.f - p.lfoAmp * 0.5f * (1.f - lfo)) * (mxn ? std::max(0.f, 1.f + md[7]) : 1.f);
      oL[i] += L * g; oR[i] += R * g; if (subS != 0.f) { sbL[i] += subS * g; sbR[i] += subS * g; }
    }
  }
};

}  // namespace supurr
