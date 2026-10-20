// SupurrSaw DSP core. No JUCE dependency, header only, so it can be tested on its own.
// Licence: AGPL-3.0-or-later. Author: Studio 87
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <map>
#include <vector>

namespace supurr {

constexpr int kMaxVoices = 32;  // hard limit; Params::polyphony sets how many are used
constexpr int kMaxSaws   = 15;  // unison voices per layer
constexpr int kLayers    = 3;
constexpr float kPi      = 3.14159265358979f;
constexpr double kPiD    = 3.14159265358979323846;

enum class Wave { Off, SuperSaw, SuperSquare, Saw, Pulse, Tri, Sine, Sample, Custom };
// The first six keep their old order (LP12 and LP6 take the earlier taps of the same four pole ladder); the rest are the newer types. Off lets a filter slot pass the signal untouched.
enum class FilterMode { LP24, LP18, HP, BP, LP12, LP6, Off, Moog, Jupiter, MS20, SEM, Acid, HP24, BP24, Notch, CombP, CombM, Formant };
constexpr int kFilterModes = 18;
static const char* const kFilterNames[kFilterModes] = {"LP24", "LP18", "HP", "BP", "LP12", "LP6", "Off", "Moog", "Jupiter", "MS-20", "SEM", "Acid", "HP24", "BP24", "Notch", "Comb+", "Comb-", "Formant"};   // indexed by FilterMode
// the order of the page's filter list (FILTER_MODES in the playground engine)
static const FilterMode kFilterList[kFilterModes] = {FilterMode::Off, FilterMode::LP6, FilterMode::LP12, FilterMode::LP18, FilterMode::LP24, FilterMode::Moog, FilterMode::Jupiter, FilterMode::MS20, FilterMode::SEM, FilterMode::Acid,
  FilterMode::HP, FilterMode::HP24, FilterMode::BP, FilterMode::BP24, FilterMode::Notch, FilterMode::CombP, FilterMode::CombM, FilterMode::Formant};
inline FilterMode filterModeFromName(const char* n) { for (int i = 0; i < kFilterModes; ++i) { const char* a = kFilterNames[i]; const char* b = n; while (*a && *a == *b) { ++a; ++b; } if (!*a && !*b) return (FilterMode)i; } return FilterMode::LP24; }

// Mod matrix: 15 sources x 23 destinations (same order as MX_SRC / MX_DST in the playground engine). Destinations 16 and up are global (effects, clipper, high pass, low pass, width).
constexpr int kMxSrc = 15, kMxDst = 23;
static const char* const kMxSrcNames[kMxSrc] = {"lfo", "fenv", "aenv", "penv", "whl", "key", "vel", "rnd", "men", "lfo2", "lfo3", "fenv2", "fenv3", "men2", "men3"};
static const char* const kMxDstNames[kMxDst] = {"pitch", "cut", "res", "drv", "pwm", "det", "pan", "vol", "sy1", "sy2", "sy3", "sub", "nz", "lc", "cut2", "cut3", "rvb", "dly", "drs", "clip", "hpf", "lpf", "wid"};

// Built-in procedural samples (made from maths when the engine is prepared; no recorded audio). Index 0 is Off.
constexpr int kSmpCount = 20;
static const char* const kSmpIds[kSmpCount] = {"Off", "click_soft", "click_hard", "click_tok", "kick_punch", "kick_hard", "sub_thump", "snare_crack", "snare_tight", "clap", "rim", "hat_closed", "hat_open", "pluck_soft", "pluck_hard", "pluck_pizz", "pluck_glass", "chime", "zap", "air_puff"};
inline bool smpKeyed(int i) { return i >= 13 && i <= 17; }   // the plucks and the chime follow the key
inline int smpFromId(const char* n) { for (int i = 0; i < kSmpCount; ++i) { const char* a = kSmpIds[i]; const char* b = n; while (*a && *a == *b) { ++a; ++b; } if (!*a && !*b) return i; } return 0; }

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
inline double envShape(double f, double c, bool rising) {
  if (c == 0.0) return f;
  const double p = 1.0 + 4.0 * std::fabs(c);
  const bool fastStart = rising ? (c >= 0.0) : (c < 0.0);
  return fastStart ? 1.0 - std::pow(1.0 - f, p) : std::pow(f, p);
}

struct LayerParams {
  bool on = true;              // layer switch: off keeps the wave and settings but mutes the layer
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
  int smp = 0;                 // built-in sample played with this layer (index into kSmpIds, 0 = off), pitch in semitones, volume
  float smpPitch = 0.f, smpVol = 1.f;
  std::shared_ptr<const Sample> sample;
  std::shared_ptr<const WaveTable> table;   // used by Wave::Custom (an imported or drawn cycle, played as a super stack)
};

struct Params {
  LayerParams layer[kLayers];
  // sub oscillator (always mono and centred)
  float subLevel = 0.f;
  int subVoices = 1;           // 1..5 detuned sub oscillators
  float subDetune = 0.f;       // 0..1 spread of the sub voices (up to 30 cents each side)
  float subWidth = 0.f;        // 0..1 stereo spread of the sub voices
  int noiseVoices = 1;         // 1..5 noise streams with spread high-pass points
  float noiseDetune = 0.f;     // 0..1 spread of the noise high-pass points (up to 2 octaves each side)
  float noiseWidth = 0.f;      // 0..1 how different left and right noise are
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
  float lowCutHz = 20.f;       // high pass on the whole voice after the filters (20 Hz .. 20 kHz); with the filters on HP or HP24 the page shows highCutHz here instead
  float highCutHz = 20000.f;   // low pass on the whole voice after the filters; only active when the first filter is HP or HP24 (so the two make a band), 20 kHz = off
  int auxSlope = 0;            // steepness of lowCutHz / highCutHz: 0 = 6 dB, 1 = 12, 2 = 18, 3 = 24, 4 = 48, 5 = 96 dB per octave
  float dist = 0.f;            // 0..1 distortion (acid, hoover, dirty reese, screech)
  bool distPre = false;        // true: distortion before the filter (the filter shapes the screech), false: after it
  float noiseLevel = 0.f;      // 0..1 white noise added before the filter
  float noiseHpHz = 3000.f;    // high-pass on the noise so it never muddies the low end
  Adsr amp, fenv;
  Adsr amp2, amp3, fenv2, fenv3;   // amp2 / amp3 are the volume envelopes of layers 2 and 3 when ampLink is off; fenv2 / fenv3 drive filters 2 and 3 and the mod matrix
  bool ampLink = true;             // true: the one amp envelope shapes the whole voice (the old behaviour); false: every layer has its own
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
  bool eqBypass = false;   // main EQ bypass: skips all bands, dynamics and saturation
  bool eq1On = false, eq2On = true, eq3On = true, eq4On = true, eq5On = true, eq6On = false;
  float eq1F = 30.f, eq2F = 120.f, eq2G = 0.f, eq3F = 500.f, eq3G = 0.f, eq3Q = 1.f, eq4F = 3000.f, eq4G = 0.f, eq4Q = 1.f, eq5F = 9000.f, eq5G = 0.f, eq6F = 18000.f;
  float drvDrive = 0.f, drvXs = 0.f, drvFuzz = 0.f, drvMix = 1.f;   // colour stack on the voice sum, before the effects (Drive, Transistor, Fuzz)
  float dynMix = 1.f, dmMix = 0.4f, dmDrive = 0.5f, dmClip = 0.35f, dmLow = 0.f, dmMid = 0.f, dmHigh = 0.f;
  // Full mod matrix: mx[source][destination], -1..1. Sources: LFO, filter env, amp env, pitch env, wheel, key, velocity, random, drawn mod envelope.
  // Destinations: pitch(12 st), cutoff(5 oct), reso, drive, pwm, detune, pan, volume, sync1..3 (12 st), sub, noise, low cut(4 oct).
  float mx[kMxSrc][kMxDst] = {};   // rows 8, 13 and 14 are the drawn mod envelopes 1 to 3
  float menv[64]; float menvTime = 0.5f; bool menvLoop = false, menvInv = false;   // drawn modulation envelope (0..1 over menvTime seconds, one shot or loop)
  float menv2[64], menv3[64]; float menv2Time = 0.5f, menv3Time = 0.5f; bool menvSync = false, menv2Sync = false, menv3Sync = false; int menvDiv = 6, menv2Div = 6, menv3Div = 6; bool menv2Loop = false, menv3Loop = false, menv2Inv = false, menv3Inv = false;
  // filters 2 and 3 (the first filter is filterMode / cutoff / reso / drive / envAmt / keyTrack). Off by default. fRoute 0 = in series, 1 = in parallel. Their envelope is fenv2 / fenv3.
  FilterMode f2Mode = FilterMode::Off, f3Mode = FilterMode::Off; int fRoute = 0;
  float f2Cut = 4000.f, f2Res = 0.2f, f2Drv = 0.f, f2Env = 0.f, f2Key = 0.f, f3Cut = 4000.f, f3Res = 0.2f, f3Drv = 0.f, f3Env = 0.f, f3Key = 0.f;
  // LFO 2 and 3: rate, shape (0 sine, 1 tri, 2 square, 3 saw, 4 random square, 5 random round, 6 ramp down), depth, fade-in and start phase. They feed the mod matrix only.
  float lfo2Rate = 6.f, lfo3Rate = 6.f, lfo2Depth = 1.f, lfo3Depth = 1.f, lfo2Fade = 0.f, lfo3Fade = 0.f, lfo2Start = 0.f, lfo3Start = 0.f; int lfo2Shape = 0, lfo3Shape = 0;
  // procedural sample on the noise section, and where the samples are mixed in (false: before the filters, true: after them)
  int nzSmp = 0; float nzSmpPitch = 0.f, nzSmpVol = 1.f; bool smpPost = false;
  // build-up macro (0..1): high pass up to about 2.5 kHz, more reverb and delay, harder clipper, narrower highs, a little quieter
  float build = 0.f;
  // mod effect in the chorus slot: 0 chorus, 1 flanger, 2 phaser (chorusMix is the amount); chorusFb is the feedback of the flanger and phaser
  int modFxType = 0; float chorusFb = 0.5f;
  // Stack: when on, the two types that are not picked also run, each with its own knobs (mx0 chorus, mx1 flanger, mx2 phaser)
  bool mfxMulti = false;
  float mx0Mix = 0.f, mx0Depth = 0.5f, mx0Rate = 0.6f, mx0Delay = 12.f, mx0Width = 1.f, mx0Fb = 0.5f;
  float mx1Mix = 0.f, mx1Depth = 0.6f, mx1Rate = 0.25f, mx1Delay = 6.f, mx1Width = 1.f, mx1Fb = 0.6f;
  float mx2Mix = 0.f, mx2Depth = 0.7f, mx2Rate = 0.35f, mx2Delay = 12.f, mx2Width = 1.f, mx2Fb = 0.5f;
  // crusher (bit depth and sample rate reduction) after the colour stack; crushMix 0 = off
  float crushBits = 16.f, crushRate = 1.f, crushMix = 0.f;
  // width: mid/side with separate width below and above 2 kHz, plus mid and side saturation (side only above 2 kHz)
  float widMid = 1.f, widHi = 1.f, msMid = 0.f, msSide = 0.f;
  // EQ bands 2 to 5: Dyn is the dynamic gain range in dB (0 = static), Thr threshold dB, Atk / Rel ms, Below = act when the band is quiet, Sat amount and SatT type (0 warm, 1 soft, 2 hard) of a saturator that only touches that band
  float eq2Dyn = 0.f, eq2Thr = -24.f, eq2Atk = 10.f, eq2Rel = 120.f, eq2Sat = 0.f; bool eq2Below = false; int eq2SatT = 0;
  float eq3Dyn = 0.f, eq3Thr = -24.f, eq3Atk = 10.f, eq3Rel = 120.f, eq3Sat = 0.f; bool eq3Below = false; int eq3SatT = 0;
  float eq4Dyn = 0.f, eq4Thr = -24.f, eq4Atk = 10.f, eq4Rel = 120.f, eq4Sat = 0.f; bool eq4Below = false; int eq4SatT = 0;
  float eq5Dyn = 0.f, eq5Thr = -24.f, eq5Atk = 10.f, eq5Rel = 120.f, eq5Sat = 0.f; bool eq5Below = false; int eq5SatT = 0;
  Params() { for (int i = 0; i < 64; ++i) menv[i] = menv2[i] = menv3[i] = (float)std::exp(-4.0 * i / 63.0); }
};

// smooth knee clipper: identity below T, then T + w*tanh((|x|-T)/w); C2-continuous at T, tops out at exactly 1
inline double softClip(double x, double T) {
  double a = std::fabs(x); if (a <= T) return x;
  double w = 1. - T, y = T + w * std::tanh((a - T) / w); return x < 0. ? -y : y;
}

// delay lengths in beats: 1/1, 1/2, 1/2 dotted, 1/2 triplet, 1/4, 1/4 dotted, 1/4 triplet, 1/8, 1/8 dotted, 1/8 triplet, 1/16, 1/16 dotted, 1/16 triplet, 1/32
static const double kDelayBeats[14] = {4., 2., 3., 4. / 3., 1., 1.5, 2. / 3., 0.5, 0.75, 1. / 3., 0.25, 0.375, 1. / 6., 0.125};
inline double delaySeconds(const Params& p) {
  double t = p.delaySync ? kDelayBeats[std::max(0, std::min(13, p.delayDiv))] * 60. / std::max(20., std::min(400., (double)p.bpm)) : (double)p.delayTime;
  return std::max(0.02, std::min(1.9, t));
}

inline double midiToHz(double n) { return 440. * std::exp2((n - 69.) / 12.); }

// JP-8000 detune knob response (Adam Szabo, 2010), same curve as HyperSaw
inline double jpDetune(double x) {
  double y = 10028.7312891634;
  y = y * x - 50818.8652045924;  y = y * x + 111363.4808729368;
  y = y * x - 138150.6761080548; y = y * x + 106649.6679158292;
  y = y * x - 53046.9642751875;  y = y * x + 17019.9518580080;
  y = y * x - 3425.0836591318;   y = y * x + 404.2703938388;
  y = y * x - 24.1878824391;     y = y * x + 0.6717417634;
  y = y * x + 0.0030115596;
  return std::min(1., std::max(0., y));
}
constexpr double kMaxDetune = 0.11;  // outer voice at full detune
static const double kSpread15[15] = {-1., -.78, -.58, -.41, -.26, -.14, -.05, 0.,
                                    .052, .143, .262, .413, .583, .782, 1.};
// spread position of unison voice j of n (n voices, centre voice included when n is odd)
inline double sawSpread(int j, int n) {
  if (n <= 1) return 0.;
  double t = (double)j * 14. / (double)(n - 1);
  int i = std::min(13, (int)t);
  double f = t - (double)i;
  return kSpread15[i] * (1. - f) + kSpread15[i + 1] * f;
}

inline double blep(double t, double dt) {
  if (t < dt) { t /= dt; return t + t - t * t - 1.; }
  if (t > 1. - dt) { t = (t - 1.) / dt; return t * t + t + t + 1.; }
  return 0.;
}

struct Rng { uint32_t s = 12345u; double next() { s = s * 1664525u + 1013904223u; return (double)(s >> 8) / 16777216.0; } };   // same stream as the playground engine

// The voice path runs in double precision (like the playground engine), so the two stay sample close.
struct EnvState {
  double v = 0.0, v0 = 0.0, n = 0.0; int stage = 0;  // 0 idle 1 attack 2 decay/sustain 3 release; n counts samples in the stage
  void on() { stage = 1; n = 0.0; v0 = v; }
  void off() { if (stage && stage != 3) { stage = 3; n = 0.0; v0 = v; } }
  double tick(const Adsr& e, double sr) {
    switch (stage) {
      case 1: { n += 1.0; double f = n / std::max(1.0, (double)e.a * sr * (1.0 - v0));
        if (f >= 1.0) { v = 1.0; stage = 2; n = 0.0; v0 = 1.0; } else v = v0 + (1.0 - v0) * envShape(f, e.ca, true); break; }
      case 2: { n += 1.0; double f = n / std::max(1.0, (double)e.d * sr); v = f >= 1.0 ? (double)e.s : v0 + ((double)e.s - v0) * envShape(f, e.cd, false); break; }
      case 3: { n += 1.0; double f = n / std::max(1.0, (double)e.r * sr); if (f >= 1.0) { v = 0.0; stage = 0; } else v = v0 * (1.0 - envShape(f, e.cr, false)); break; }
      default: break;
    }
    return v;
  }
};

// Filters. LP24 / LP18 / LP12 / LP6 / Jupiter: four pole ladder (zero delay feedback) with the taps. Moog and Acid: nonlinear four stage ladders, 2x oversampled. MS-20: two pole with hot resonance and clipped state.
// SEM: clean two pole. HP, BP, Notch, HP24, BP24: state variable filters. Comb+ and Comb-: feedback comb tuned by the cutoff. Formant: three vowel resonators, the cutoff sweeps A E I O U.
static const double kFormants[5][3] = {{730, 1090, 2440}, {530, 1840, 2480}, {270, 2290, 3010}, {570, 840, 2410}, {300, 870, 2240}};
struct Filter {
  double s[4] = {0, 0, 0, 0}, z1 = 0, z2 = 0, z3 = 0, z4 = 0, y[4] = {0, 0, 0, 0}, inE = 0, outE = 0, t[4] = {0, 0, 0, 0}, yy[4] = {0, 0, 0, 0}, fbl = 0, cd = 0, fz[6] = {0, 0, 0, 0, 0, 0}, hp_ = 0, bp_ = 0, lp_ = 0;
  double lc = -1, lsr = -1, lg = 0; int ci = 0; float* cb = nullptr; bool cbInit = false;   // cb: 4096 floats for the comb modes, lent by the engine
  void svf(double x, double g, double R, bool st) {
    const double a1 = st ? z3 : z1, a2 = st ? z4 : z2;
    const double hp = (x - (2 * R + g) * a1 - a2) / (1 + 2 * R * g + g * g), bp = g * hp + a1, lp = g * bp + a2;
    if (st) { z3 = g * hp + bp; z4 = g * bp + lp; } else { z1 = g * hp + bp; z2 = g * bp + lp; }
    hp_ = hp; bp_ = bp; lp_ = lp;
  }
  double process(double x, FilterMode m, double cutoffHz, double reso, double drive, double sr) {
    if (m == FilterMode::Off) return x;
    double g; if (cutoffHz == lc && sr == lsr) g = lg; else { const double fc = std::min(cutoffHz, 0.45 * sr); g = std::tan(kPiD * fc / sr); lc = cutoffHz; lsr = sr; lg = g; }
    switch (m) {
      case FilterMode::LP24: case FilterMode::LP18: case FilterMode::LP12: case FilterMode::LP6: case FilterMode::Jupiter: {
        const double G = g / (1 + g), G2 = G * G, G4 = G2 * G2, k = 3.95 * reso, ig = 1 / (1 + g);
        const double S = G2 * G * s[0] * ig + G2 * s[1] * ig + G * s[2] * ig + s[3] * ig;
        const double dg = 0.5 + 3 * drive; double inp = std::tanh(x * dg) / dg; if (m == FilterMode::Jupiter) inp *= 1 + 0.2 * k;
        const double fa = 1 - std::exp(-1 / (0.03 * sr)); inE += fa * (std::fabs(x) - inE); outE += fa * (std::fabs(inp) - outE);
        if (outE > 1e-5 && drive > 0) inp *= std::min(4.0, std::max(1.0, inE / outE));
        const double y4 = (G4 * inp + S) / (1 + k * G4), u = inp - k * y4; double xi = u;
        for (int i = 0; i < 4; ++i) { const double v = G * (xi - s[i]); y[i] = v + s[i]; s[i] = y[i] + v; xi = y[i]; }
        return (m == FilterMode::LP24 || m == FilterMode::Jupiter) ? y[3] : (m == FilterMode::LP18 ? y[2] : (m == FilterMode::LP12 ? y[1] : y[0]));
      }
      case FilterMode::Moog: {
        const double ga = 1 - std::exp(-kPiD * std::min(cutoffHz, 0.45 * sr) / sr), k = 4 * reso, dg = 1 + 3 * drive; double o = 0;
        for (int p = 0; p < 2; ++p) { const double u = std::tanh(x * dg * 0.8 - k * yy[3]);
          yy[0] += ga * (u - t[0]); t[0] = std::tanh(yy[0]); yy[1] += ga * (t[0] - t[1]); t[1] = std::tanh(yy[1]); yy[2] += ga * (t[1] - t[2]); t[2] = std::tanh(yy[2]); yy[3] += ga * (t[2] - t[3]); t[3] = std::tanh(yy[3]); o += yy[3]; }
        return 0.5 * o * (1 + 0.3 * k) / std::sqrt(dg);
      }
      case FilterMode::MS20: {
        const double R = 0.5 * (1 - 0.975 * reso) + 0.012, dg = 1 + 2 * drive; svf(std::tanh(x * dg * 1.2) / dg, g, R, false);
        z1 = std::tanh(z1 * 0.7) / 0.7; z2 = std::tanh(z2 * 0.7) / 0.7; return std::tanh(lp_ * 1.1) * 0.95;
      }
      case FilterMode::SEM: { const double R = 0.5 * (1 - 0.9 * reso) + 0.06; svf(x, g, R, false); return lp_; }
      case FilterMode::Acid: {
        const double ga = 1 - std::exp(-kPiD * std::min(cutoffHz, 0.45 * sr) / sr), k = 4.1 * reso, dg = 1 + 5 * drive; double o = 0;
        for (int p = 0; p < 2; ++p) { const double fbv = yy[3] - fbl; fbl += 0.003 * (yy[3] - fbl);
          const double u = std::tanh(x * dg * 0.9 - k * fbv * 0.97);
          yy[0] += ga * (u - t[0]); t[0] = std::tanh(yy[0]); yy[1] += ga * (t[0] - t[1]); t[1] = std::tanh(yy[1]); yy[2] += ga * (t[1] - t[2]); t[2] = std::tanh(yy[2]); yy[3] += ga * (t[2] - t[3]); t[3] = std::tanh(yy[3]); o += 0.6 * yy[2] + 0.4 * yy[3]; }
        return 0.5 * o * (1 + 0.25 * k) / std::sqrt(dg);
      }
      case FilterMode::CombP: case FilterMode::CombM: {
        if (!cb) return x;
        if (!cbInit) { for (int i = 0; i < 4096; ++i) cb[i] = 0.f; cbInit = true; }
        const double f = std::max(30.0, std::min(0.45 * sr, cutoffHz)), D = std::max(2.0, std::min(4090.0, sr / f)), fb = (m == FilterMode::CombP ? 1.0 : -1.0) * std::min(0.96, reso * 0.96); const int n = 4096;
        double rp = (double)ci - D; if (rp < 0) rp += n; const int i0 = (int)std::floor(rp); const double fr = rp - i0, a = cb[i0 % n], b = cb[(i0 + 1) % n]; double d = a * (1 - fr) + b * fr;
        cd += 0.55 * (d - cd); d = cd;
        const double yv = x + fb * d; cb[ci] = (float)yv; ci = (ci + 1) % n; return yv * (1 - 0.5 * std::fabs(fb)) * 0.8;
      }
      case FilterMode::Formant: {
        const double pos = std::min(0.9999, std::max(0.0, std::log(std::max(20.0, cutoffHz) / 20) / std::log(1000.0))) * 4; const int i = (int)std::floor(pos); const double fr = pos - i, Q = 3 + reso * 20; double o = 0;
        const double* A = kFormants[i]; const double* B = kFormants[i + 1];
        for (int j = 0; j < 3; ++j) { const double f = A[j] + (B[j] - A[j]) * fr, gg = std::tan(kPiD * std::min(f, 0.45 * sr) / sr), R = 0.5 / Q, a1 = fz[j * 2], a2 = fz[j * 2 + 1];
          const double hp = (x - (2 * R + gg) * a1 - a2) / (1 + 2 * R * gg + gg * gg), bp = gg * hp + a1; fz[j * 2] = gg * hp + bp; fz[j * 2 + 1] = gg * bp + (gg * bp + a2); o += bp * (j == 0 ? 1 : (j == 1 ? 0.7 : 0.4)) * 2 * R * Q * 0.5; }
        return o * 1.6;
      }
      default: break;
    }
    const double R = 0.5 * (1 - 0.95 * reso) + 0.02;
    svf(x, g, R, false); const double hp = hp_, bp = bp_, lp = lp_;
    if (m == FilterMode::HP) return hp;
    if (m == FilterMode::BP) return bp * (2 * R);
    if (m == FilterMode::Notch) return hp + lp;
    const double R2 = 0.55;
    if (m == FilterMode::HP24) { svf(hp, g, R2, true); return hp_; }
    svf(bp * (2 * R), g, R2, true); return bp_ * (2 * R2);   // BP24
  }
};

// LFO shapes 0..6: sine, triangle, square, saw up, random square (new value every cycle), random round (glides between random values), ramp down
static const double kMenvBeats[16] = {16, 8, 4, 2, 3, 4. / 3, 1, 1.5, 2. / 3, 0.5, 0.75, 1. / 3, 0.25, 0.375, 1. / 6, 0.125};   // drawn envelope length in beats for menvDiv
inline double lfoWave(int sh, double ph, double cur, double prev) {
  switch (sh) { case 1: return 4 * std::fabs(ph - 0.5) - 1; case 2: return ph < 0.5 ? 1.0 : -1.0; case 3: return 2 * ph - 1; case 4: return cur; case 5: return prev + (cur - prev) * (1 - std::cos(kPiD * ph)) * 0.5; case 6: return 1 - 2 * ph; default: return std::sin(2 * kPiD * ph); }
}

// steep high pass / low pass: Butterworth cascades of biquads (48 dB = 8th order, 96 dB = 16th order)
inline void bwDesign(double* c, int sections, double fc, double sr, bool hp) {
  const int order = sections * 2; const double w = std::tan(kPiD * std::min(fc, 0.45 * sr) / sr);
  for (int k = 0; k < sections; ++k) { const double q = 1.0 / (2.0 * std::cos(kPiD * (2 * k + 1) / (2.0 * order))), n = 1.0 / (1.0 + w / q + w * w); const int o = k * 5;
    if (hp) { c[o] = n; c[o + 1] = -2.0 * n; c[o + 2] = n; } else { c[o] = w * w * n; c[o + 1] = 2.0 * c[o]; c[o + 2] = c[o]; }
    c[o + 3] = 2.0 * (w * w - 1.0) * n; c[o + 4] = (1.0 - w / q + w * w) * n; }
}
inline double bwRun(const double* c, double* z, int sections, double x) {
  for (int k = 0; k < sections; ++k) { const int o = k * 5, zi = 2 * k; const double y = c[o] * x + z[zi]; z[zi] = c[o + 1] * x - c[o + 3] * y + z[zi + 1]; z[zi + 1] = c[o + 2] * x - c[o + 4] * y; x = y; }
  return x;
}
struct Voice {
  bool active = false; int note = 60; double vel = 1.0; uint64_t age = 0;
  double phase[kLayers][kMaxSaws] = {};
  double samplePos[kLayers] = {};
  double syncPhase[kLayers] = {}, noiseHp = 0, noiseHp2 = 0, gl = 0, subPh[5] = {}, nzS[20] = {};
  double rnd = 0, sh = 0, sh0 = 0, subPhase = 0, drift[kMaxSaws] = {}, driftT[kMaxSaws] = {};
  double lfoPhase = 0, t = 0, stackCents = 0, stackGain = 1;
  double lyOut[kLayers] = {}, fmSubPh = 0;
  // control rate values (updated every 4th sample), the second and third LFO, their own random stream (lfoRnd), procedural sample positions and steps
  int ctr = 0; double lfoC = 0, pEnv = 0, pcC[kLayers] = {1, 1, 1}, psC[kLayers] = {0, 0, 0}, fC[kLayers] = {440, 440, 440}, cc[3] = {1000, 1000, 1000}, laC = 0.01, hcC = 0.9;
  double lp2[2] = {0, 0}, sa[2] = {0, 0}, sb[2] = {0, 0}, lc2[2] = {0, 0}; uint32_t rs = 1;
  double sp[4] = {0, 0, 0, 0}, sq[4] = {1, 1, 1, 1}, ax[8] = {0, 0, 0, 0, 0, 0, 0, 0}, svL[kMxSrc] = {}, bw[40] = {}, bwz[32] = {}, bwF = -1.0; int bwN = 0; bool bwH = false;
  EnvState amp, amp2, amp3, fenv, fenv2, fenv3; Filter fl[3], fr[3]; double hpL = 0, hpR = 0; bool oneShot = false;
  double lfoRnd() { rs = rs * 1664525u + 1013904223u; return (double)(rs >> 8) / 16777216.0 * 2.0 - 1.0; }
};


// ---- effects ----
struct DelayLine {
  std::vector<float> b; size_t w = 0;   // float storage, like the playground engine's Float32Array
  void init(size_t n) { b.assign(n + 4, 0.f); w = 0; }
  void push(double x) { b[w] = (float)x; if (++w >= b.size()) w = 0; }
  double read(double d) const {  // d samples back, linear interpolation
    double rp = (double)w - d; while (rp < 0) rp += (double)b.size(); if (rp >= (double)b.size()) rp -= (double)b.size(); size_t i0 = (size_t)rp; double f = rp - (double)i0;
    size_t i1 = i0 + 1 >= b.size() ? 0 : i0 + 1; return b[i0] * (1. - f) + b[i1] * f; }
};
struct Reverb {
  struct Comb { std::vector<float> b; size_t i = 0; double lp = 0; double run(double x, double fb, double damp) { double o = b[i]; lp = o * (1. - damp) + lp * damp; b[i] = (float)(x + lp * fb); if (++i >= b.size()) i = 0; return o; } };
  struct Ap { std::vector<float> b; size_t i = 0; double run(double x) { double o = b[i]; double y = -x + o; b[i] = (float)(x + o * 0.5); if (++i >= b.size()) i = 0; return y; } };
  Comb c[2][8]; Ap a[2][4];
  void init(double sr) {
    const int cl[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617}, al[4] = {556, 441, 341, 225};
    double k = sr / 44100.;
    for (int ch = 0; ch < 2; ++ch) { for (int j = 0; j < 8; ++j) { c[ch][j].b.assign((size_t)((cl[j] + ch * 23) * k), 0.f); c[ch][j].i = 0; c[ch][j].lp = 0; }
      for (int j = 0; j < 4; ++j) { a[ch][j].b.assign((size_t)((al[j] + ch * 11) * k), 0.f); a[ch][j].i = 0; } }
  }
  // input mono, outputs wet L/R
  void run(double in, double size, double damp, double& wl, double& wr) {
    double fb = 0.70 + 0.27 * size, o[2];
    for (int ch = 0; ch < 2; ++ch) { double s = 0; for (int j = 0; j < 8; ++j) s += c[ch][j].run(in * 0.15, fb, damp); for (int j = 0; j < 4; ++j) s = a[ch][j].run(s); o[ch] = s; }
    wl = o[0]; wr = o[1];
  }
};
// ---- colour stack: Drive, Transistor, Fuzz (PurrOne stages), 2x oversampled with a 33 tap half-band, fixed 16 sample latency ----
inline double sstep(double t) { t = t < 0. ? 0. : t > 1. ? 1. : t; return t * t * (3. - 2. * t); }
inline double asymTanh(double u, double nl) { return u >= 0. ? std::tanh(u) : nl * std::tanh(u / nl); }
inline double purrShaper(double x, double drive, double xs, double fuzz) {
  const double dn = sstep((drive - 0.8) * 5.), xn = sstep((xs - 0.8) * 5.), fn = sstep((fuzz - 0.8) * 5.); double y = x;
  if (drive > 0.001) { y = std::tanh(x * (1. + drive * 5. + dn * 30.)); if (dn > 0.) y += dn * (std::tanh(y * (2. + 10. * dn)) - y); }
  if (xs > 0.001) { const double nl = std::max(0.25, 1. - 0.3 * xs - 0.25 * xn), u = y * (1. + xs * 5. + xn * 30.); y = asymTanh(u, nl); y += (0.3 * xs + 0.25 * xn) * y * y; if (xn > 0.) { const double y2 = asymTanh(y * (2. + 10. * xn) + 0.15 * xn, nl); y += xn * (y2 - y); } }
  if (fuzz > 0.001) { const double fz = 1. + fuzz * 14. + fn * 70., bf = 0.5 * std::min(1., fuzz); y = std::tanh(y * fz + bf) - std::tanh(bf); if (fn > 0.) { const double r = std::sqrt(y * y + 0.02) - 0.1414; y += fn * 0.7 * (r - 0.3); y = std::tanh(y * (1. + 0.8 * fn)); } }
  return y;
}
inline double purrMakeup(double drive, double xs, double fuzz, double ref) { double a = 0., b = 0.; for (int i = 0; i < 64; ++i) { const double x = ref * std::sin(2. * kPiD * (double)i / 64.); a += x * x; const double y = purrShaper(x, drive, xs, fuzz); b += y * y; } const double m = b > 1e-9 ? std::sqrt(a / b) : 1.; return std::min(4., std::max(0.25, m)); }
struct HalfBand33 { double h[33], o[16];
  HalfBand33() { auto i0 = [](double x) { double s = 1, t = 1; for (int k = 1; k < 30; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; };
    for (int j = 0; j < 33; ++j) { double r = 2.0 * j / 32.0 - 1.0, w = i0(8.0 * std::sqrt(1.0 - r * r)) / i0(8.0); int k = j - 16; h[j] = (k == 0 ? 0.5 : std::sin(3.14159265358979323846 * k / 2.0) / (3.14159265358979323846 * k)) * w; }
    for (int k = 0; k < 16; ++k) o[k] = h[2 * k + 1]; } };
inline const HalfBand33& halfBand33() { static const HalfBand33 hb; return hb; }
struct DriveCh {
  double x[32] = {}, s[64] = {}, dd[16] = {}; int xi = 0, si = 0, di = 0; double dc = 0.;
  double run(double v, bool on, double drive, double xs, double fuzz, double mk, double mix) {
    const HalfBand33& hb = halfBand33(); x[xi] = v; const int xc = xi; xi = (xi + 1) & 31;
    const double dry = dd[di]; dd[di] = v; di = (di + 1) & 15;
    if (!on) { si = (si + 1) & 63; s[si] = 0.; si = (si + 1) & 63; s[si] = 0.; dc = 0.; return dry; }
    const double u0 = x[(xc - 8) & 31]; double u1 = 0; for (int k = 0; k < 16; ++k) u1 += hb.o[k] * x[(xc - k) & 31]; u1 *= 2.0;
    si = (si + 1) & 63; s[si] = purrShaper(u0 * 1.5, drive, xs, fuzz) * mk / 1.5;
    const int p = si; double y = 0.5 * s[(p - 16) & 63]; for (int k = 0; k < 16; ++k) y += hb.o[k] * s[(p - 2 * k - 1) & 63];
    si = (si + 1) & 63; s[si] = purrShaper((double)u1 * 1.5, drive, xs, fuzz) * mk / 1.5;
    dc += 0.0005 * ((double)y - dc); return dry * (1. - mix) + ((double)y - dc) * mix; } };
struct DriveStage {
  DriveCh c[2]; double kd = -1., kx = -1., kf = -1., mk = 1.; double sl[16] = {}, sr[16] = {}; int si = 0; double oL = 0., oR = 0., sL = 0., sR = 0.;
  void process(const Params& p, double L, double R, double sbl, double sbr) {
    const double d = p.drvDrive, xs = p.drvXs, fz = p.drvFuzz; const bool on = d > 0.001 || xs > 0.001 || fz > 0.001;
    if (on && (d != kd || xs != kx || fz != kf)) { kd = d; kx = xs; kf = fz; mk = purrMakeup(d, xs, fz, 0.75); }
    oL = c[0].run(L, on, d, xs, fz, mk, p.drvMix); oR = c[1].run(R, on, d, xs, fz, mk, p.drvMix);
    sL = sl[si]; sR = sr[si]; sl[si] = sbl; sr[si] = sbr; si = (si + 1) & 15; } };
// ---- 6 band EQ: low cut, low shelf, 2 bells, high shelf, high cut (RBJ biquads, double precision).
// Bands 2 to 5 can also be dynamic (their gain moves with the level inside the band) and can saturate only their own band. ----
enum class EqType { HP, LS, PK, HS, LP, BP };
inline void eqCoef(EqType type, double f, double g, double q, double sr, double* o) {
  const double w = 2.0 * kPiD * std::min(std::max(f, 10.0), 0.45 * sr) / sr, c = std::cos(w), sn = std::sin(w), al = sn / (2.0 * std::max(0.1, q));
  double b0, b1, b2, a0, a1, a2;
  if (type == EqType::HP) { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = b0; a0 = 1 + al; a1 = -2 * c; a2 = 1 - al; }
  else if (type == EqType::LP) { b0 = (1 - c) / 2; b1 = 1 - c; b2 = b0; a0 = 1 + al; a1 = -2 * c; a2 = 1 - al; }
  else if (type == EqType::BP) { b0 = al; b1 = 0; b2 = -al; a0 = 1 + al; a1 = -2 * c; a2 = 1 - al; }
  else { const double A = std::pow(10.0, g / 40.0);
    if (type == EqType::PK) { b0 = 1 + al * A; b1 = -2 * c; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * c; a2 = 1 - al / A; }
    else { const double be = 2 * std::sqrt(A) * al;
      if (type == EqType::LS) { b0 = A * ((A + 1) - (A - 1) * c + be); b1 = 2 * A * ((A - 1) - (A + 1) * c); b2 = A * ((A + 1) - (A - 1) * c - be); a0 = (A + 1) + (A - 1) * c + be; a1 = -2 * ((A - 1) + (A + 1) * c); a2 = (A + 1) + (A - 1) * c - be; }
      else { b0 = A * ((A + 1) + (A - 1) * c + be); b1 = -2 * A * ((A - 1) + (A + 1) * c); b2 = A * ((A + 1) + (A - 1) * c - be); a0 = (A + 1) - (A - 1) * c + be; a1 = 2 * ((A - 1) - (A + 1) * c); a2 = (A + 1) - (A - 1) * c - be; } } }
  o[0] = b0 / a0; o[1] = b1 / a0; o[2] = b2 / a0; o[3] = a1 / a0; o[4] = a2 / a0;
}
inline double lcosh(double x) { const double a = std::fabs(x); return a > 15 ? a - 0.693147 : std::log(std::cosh(x)); }
inline double adaaTanh(double v, double v1) { const double d = v - v1; return std::fabs(d) < 1e-4 ? std::tanh(0.5 * (v + v1)) : (lcosh(v) - lcosh(v1)) / d; }   // antiderivative anti-aliased tanh
// saturation types: 0 Tube, 1 Tape, 2 Fuzz, 3 Soft, 4 Hard clip, 5 Fold, 6 Diode, 7 Warm
static const double kSatDr[8] = {7, 4.5, 16, 3, 6, 5, 5, 2.5}, kSatBias[8] = {0.3, 0, 0.55, 0, 0, 0, 0.8, 0.5}, kSatOut[8] = {1, 1, 0.85, 1, 0.9, 0.9, 0.9, 1};
inline double satF(int t, double v) { return t == 4 ? (v > 1 ? 1.0 : v < -1 ? -1.0 : v) : t == 5 ? std::sin(v) : std::tanh(v); }
inline double satAD(int t, double v, double v1) {
  const double d = v - v1;
  if (t == 4) { auto F = [](double x) { const double a = std::fabs(x); return a <= 1 ? 0.5 * x * x : a - 0.5; }; return std::fabs(d) < 1e-4 ? satF(4, 0.5 * (v + v1)) : (F(v) - F(v1)) / d; }
  if (t == 5) return std::fabs(d) < 1e-4 ? std::sin(0.5 * (v + v1)) : (std::cos(v1) - std::cos(v)) / d;
  return adaaTanh(v, v1); }
struct EqStage {
  double sr = 48000.0, z[24] = {}, co[30] = {}, bz[24] = {}, bco[30] = {}; bool act[6] = {}, any = false, need = false; double oL = 0., oR = 0.;
  std::vector<double> key; unsigned char dyn[6] = {}, satT[6] = {}, below[6] = {}; double sat[6] = {}, nrm[6] = {}, env[6] = {}, gcur[6] = {}, f[6] = {}, g0[6] = {}, q[6] = {}, thr[6] = {}, rng[6] = {}, ac[6] = {}, rc[6] = {}, u1[12] = {}, dc[12] = {}; unsigned ctr = 0;
  void prepare(double s) { sr = s; for (double& v : z) v = 0; for (double& v : bz) v = 0; key.clear(); any = false; need = false; ctr = 0;
    for (int i = 0; i < 6; ++i) { dyn[i] = 0; sat[i] = 0; env[i] = 0; gcur[i] = 0; } for (int i = 0; i < 12; ++i) u1[i] = dc[i] = 0; }
  void update(const Params& p) {
    const bool on[6] = {p.eq1On, p.eq2On, p.eq3On, p.eq4On, p.eq5On, p.eq6On}; const double fr[6] = {p.eq1F, p.eq2F, p.eq3F, p.eq4F, p.eq5F, p.eq6F}, gn[6] = {0, p.eq2G, p.eq3G, p.eq4G, p.eq5G, 0}, qq[6] = {0.7071, 0.7071, p.eq3Q, p.eq4Q, 0.7071, 0.7071};
    const double dy[6] = {0, p.eq2Dyn, p.eq3Dyn, p.eq4Dyn, p.eq5Dyn, 0}, th[6] = {0, p.eq2Thr, p.eq3Thr, p.eq4Thr, p.eq5Thr, 0}, at[6] = {0, p.eq2Atk, p.eq3Atk, p.eq4Atk, p.eq5Atk, 0}, rl[6] = {0, p.eq2Rel, p.eq3Rel, p.eq4Rel, p.eq5Rel, 0};
    const double sa[6] = {0, p.eq2Sat, p.eq3Sat, p.eq4Sat, p.eq5Sat, 0}, st[6] = {0, (double)p.eq2SatT, (double)p.eq3SatT, (double)p.eq4SatT, (double)p.eq5SatT, 0}; const bool bl[6] = {false, p.eq2Below, p.eq3Below, p.eq4Below, p.eq5Below, false};
    std::vector<double> k; k.reserve(48); for (int b = 0; b < 6; ++b) { k.push_back(on[b]); k.push_back(fr[b]); k.push_back(gn[b]); k.push_back(qq[b]); k.push_back(dy[b]); k.push_back(th[b]); k.push_back(at[b]); k.push_back(rl[b]); k.push_back(bl[b]); k.push_back(sa[b]); k.push_back(st[b]); } k.push_back(p.eqBypass);
    if (k == key) return;
    key = k;
    static const EqType t[6] = {EqType::HP, EqType::LS, EqType::PK, EqType::PK, EqType::HS, EqType::LP}; bool anyB = false, needB = false;
    for (int b = 0; b < 6; ++b) { const bool gb = b >= 1 && b <= 4; const double g = gn[b]; const bool dn = gb && on[b] && std::fabs(dy[b]) > 0.01; const double sv = gb && on[b] ? sa[b] : 0;
      dyn[b] = dn ? 1 : 0; sat[b] = sv > 0.001 ? sv : 0; f[b] = fr[b]; g0[b] = g; q[b] = qq[b];
      const bool a = on[b] && !(gb && std::fabs(g) < 0.01 && !dn && !(sv > 0.001)); act[b] = a;
      if (a) { anyB = true; if (!dn) gcur[b] = g; else if (gcur[b] == 0 && !env[b]) gcur[b] = g; eqCoef(t[b], fr[b], gcur[b], qq[b], sr, &co[b * 5]); }
      if (dn || sat[b] > 0) { needB = true; const EqType bt = b == 1 ? EqType::LP : (b == 4 ? EqType::HP : EqType::BP); eqCoef(bt, fr[b], 0, b == 1 || b == 4 ? 0.7071 : std::max(0.5, qq[b]), sr, &bco[b * 5]); }
      if (dn) { thr[b] = th[b]; rng[b] = dy[b]; ac[b] = 1 - std::exp(-1 / std::max(1.0, at[b] * 0.001 * sr)); rc[b] = 1 - std::exp(-1 / std::max(1.0, rl[b] * 0.001 * sr)); below[b] = bl[b] ? 1 : 0; }
      if (sat[b] > 0) { const int ty = std::max(0, std::min(7, (int)st[b])); satT[b] = (unsigned char)ty; const double dr = 1 + sat[b] * kSatDr[ty], r = 0.25, y = satF(ty, dr * r + kSatBias[ty]) - satF(ty, kSatBias[ty]); nrm[b] = y > 1e-6 ? r / y * kSatOut[ty] : 1; } }
    any = anyB && !p.eqBypass; need = needB; }
  double satOne(int b, double c, int ch) { const int t = satT[b]; const double dr = 1 + sat[b] * kSatDr[t], bi = kSatBias[t], v = dr * c + bi; const int i = b * 2 + ch; const double v1 = u1[i]; u1[i] = v;
    double y = (satAD(t, v, v1) - satF(t, bi)) * nrm[b]; const double mix = std::min(1.0, sat[b] * 5); y = c + (y - c) * mix; dc[i] += 0.0008 * (y - dc[i]); return y - dc[i]; }
  void process(double L, double R) {
    if (!any) { oL = L; oR = R; return; }
    double l = L, r = R; const bool tick = need && ((++ctr & 15u) == 0);
    for (int b = 0; b < 6; ++b) { if (!act[b]) continue; const int dy = dyn[b]; const bool st = sat[b] > 0;
      if (dy || st) { const int c = b * 5, i = b * 4; const double b0 = bco[c], b1 = bco[c + 1], b2 = bco[c + 2], a1 = bco[c + 3], a2 = bco[c + 4];
        const double cl = b0 * l + bz[i]; bz[i] = b1 * l - a1 * cl + bz[i + 1]; bz[i + 1] = b2 * l - a2 * cl; const double cr = b0 * r + bz[i + 2]; bz[i + 2] = b1 * r - a1 * cr + bz[i + 3]; bz[i + 3] = b2 * r - a2 * cr;
        if (dy) { const double lv = 0.5 * (std::fabs(cl) + std::fabs(cr)); env[b] += (lv > env[b] ? ac[b] : rc[b]) * (lv - env[b]); }
        if (st) { l += satOne(b, cl, 0) - cl; r += satOne(b, cr, 1) - cr; } }
      if (tick && dy) { const double db = 20 * std::log10(env[b] + 1e-9); double over = below[b] ? thr[b] - db : db - thr[b]; if (over < 0) over = 0; const double rg = rng[b], dg = rg > 0 ? std::min(rg, 0.75 * over) : std::max(rg, -0.75 * over);
        gcur[b] += 0.35 * (g0[b] + dg - gcur[b]); static const EqType t[6] = {EqType::HP, EqType::LS, EqType::PK, EqType::PK, EqType::HS, EqType::LP}; eqCoef(t[b], f[b], gcur[b], q[b], sr, &co[b * 5]); }
      const double* c = &co[b * 5]; double* zz = &z[b * 4];
      double y = c[0] * l + zz[0]; zz[0] = c[1] * l - c[3] * y + zz[1]; zz[1] = c[2] * l - c[4] * y; l = y;
      y = c[0] * r + zz[2]; zz[2] = c[1] * r - c[3] * y + zz[3]; zz[3] = c[2] * r - c[4] * y; r = y; }
    oL = l; oR = r; } };
// ---- crush: bit depth and sample rate reduction ----
struct Crush {
  double h[2] = {0, 0}, c = 0, oL = 0, oR = 0;
  void process(const Params& p, double L, double R) {
    const double mix = p.crushMix, bits = p.crushBits, rate = p.crushRate; if (mix <= 0 || (bits >= 15.99 && rate <= 1.001)) { oL = L; oR = R; return; }
    c += 1; if (c >= rate) { c -= rate; const double q = std::pow(2.0, std::max(1.0, bits) - 1), step = 1 / q; h[0] = js_round(L * q) * step; h[1] = js_round(R * q) * step; }
    oL = L + (h[0] - L) * mix; oR = R + (h[1] - R) * mix; }
  static double js_round(double x) { return std::floor(x + 0.5); }   // Math.round: halves go up
};
// ---- width: mid/side with separate width above and below 2 kHz, plus mid and side saturation (side only above 2 kHz) ----
struct WidthStage {
  double sr = 48000, sl = 0, a = 0, u[2] = {0, 0}, oL = 0, oR = 0;
  void prepare(double s) { sr = s; sl = 0; a = 1 - std::exp(-2 * kPiD * 2000 / s); u[0] = u[1] = 0; }
  double sat(int i, double x, double amt) { const double dr = 1 + amt * 5, v = dr * x, y = adaaTanh(v, u[i]); u[i] = v; const double nrm = 0.25 / std::tanh(dr * 0.25), o = y * nrm; return x + (o - x) * std::min(1.0, amt * 4); }
  void process(const Params& p, double L, double R) {
    const double wm = p.widMid, wh = p.widHi, ms = p.msMid, ss = p.msSide; if (wm == 1 && wh == 1 && ms <= 0 && ss <= 0) { oL = L; oR = R; return; }
    double m = 0.5 * (L + R); const double sd = 0.5 * (L - R); sl += a * (sd - sl); const double sh = sd - sl; double hi = sh;
    if (ss > 0) { hi = sat(1, sh, ss); }
    if (ms > 0) { m = sat(0, m, ms); }
    const double s2 = sl * wm + hi * wh; oL = m + s2; oR = m - s2; } };
struct Fx {
  double sr = 48000.; DelayLine chor, dl, dr, pre, fl[2]; Reverb rev; double lfo = 0., lfoT[3] = {0, 0, 0}, dlp[2] = {0, 0}, rhp = 0., dtSm = -1., ph[2][6] = {}, phFb[2] = {0, 0};
  double wt[8] = {1e9, -1e9, 1e9, -1e9, 1e9, -1e9, 1e9, -1e9};   // min/max of the delay and reverb wet signal, for the live display
  void wetTake(float* o) { for (int i = 0; i < 8; ++i) { o[i] = std::fabs(wt[i]) > 1e8 ? 0.f : (float)wt[i]; wt[i] = (i & 1) ? -1e9 : 1e9; } }
  void prepare(double s) { sr = s; chor.init((size_t)(0.05 * s)); dl.init((size_t)(2. * s)); dr.init((size_t)(2. * s)); rev.init(s); pre.init((size_t)(0.26 * s)); fl[0].init((size_t)(0.02 * s)); fl[1].init((size_t)(0.02 * s));
    lfo = 0.; lfoT[0] = lfoT[1] = lfoT[2] = 0.; dlp[0] = dlp[1] = 0.; rhp = 0.; dtSm = -1.; for (auto& a : ph) for (double& v : a) v = 0; phFb[0] = phFb[1] = 0; }
  void process(const Params& p, double& L, double& R) {
    const double m = 0.5 * (L + R); const int mt = p.modFxType;
    static const int kOrder[3] = {1, 2, 0};   // stages run flanger, phaser, chorus; the picked type uses the main knobs, the others only run when Stack is on
    for (int oi = 0; oi < 3; ++oi) {
      const int t = kOrder[oi]; double mix, depth, rate, delay, width, fbk;
      if (t == mt) { mix = p.chorusMix; depth = p.chorusDepth; rate = p.chorusRate; delay = p.chorusDelay; width = p.chorusWidth; fbk = p.chorusFb; }
      else if (p.mfxMulti) {
        switch (t) {
          case 0: mix = p.mx0Mix; depth = p.mx0Depth; rate = p.mx0Rate; delay = p.mx0Delay; width = p.mx0Width; fbk = p.mx0Fb; break;
          case 1: mix = p.mx1Mix; depth = p.mx1Depth; rate = p.mx1Rate; delay = p.mx1Delay; width = p.mx1Width; fbk = p.mx1Fb; break;
          default: mix = p.mx2Mix; depth = p.mx2Depth; rate = p.mx2Rate; delay = p.mx2Delay; width = p.mx2Width; fbk = p.mx2Fb; break;
        }
      } else continue;
      if (!(mix > 0)) continue;
      lfoT[t] += rate / sr; if (lfoT[t] >= 1) lfoT[t] -= 1; const double lf = lfoT[t];
      if (t == 1) {   // flanger: short swept delay with feedback
        const double base = std::max(0.0004, delay * 0.00025), fb = std::max(-0.95, std::min(0.95, fbk)), wd = 0.25 * width; double out[2] = {L, R};
        for (int c = 0; c < 2; ++c) { const double x = c ? R : L, phs = lf + (c ? wd : 0), d = (base * (1 + 0.9 * depth * std::sin(2 * kPiD * phs))) * sr, y = fl[c].read(std::max(1.0, d)); fl[c].push(x + fb * y); out[c] = x + mix * y; }
        L = out[0]; R = out[1];
      } else if (t == 2) {   // phaser: 6 allpass stages swept in frequency, with feedback
        const double fb = std::max(-0.95, std::min(0.95, fbk)), wd = 0.25 * width; double out[2] = {L, R};
        for (int c = 0; c < 2; ++c) { const double x = c ? R : L, phs = lf + (c ? wd : 0), f = std::min(0.45 * sr, 220 * std::pow(2.0, depth * 4.2 * (0.5 + 0.5 * std::sin(2 * kPiD * phs)))), tg = std::tan(kPiD * f / sr), a = (tg - 1) / (tg + 1); double* z = ph[c]; double v = x + fb * phFb[c];
          for (int k = 0; k < 6; ++k) { const double y = a * v + z[k]; z[k] = v - a * y; v = y; } phFb[c] = v; out[c] = x * (1 - 0.5 * mix) + v * 0.5 * mix; }
        L = out[0]; R = out[1];
      } else {   // chorus, side-only: the left and right wet signals are opposite, so they cancel in mono
        chor.push(0.5 * (L + R)); const double cd = delay * 0.001; const double d = (cd + 0.006 * depth * std::sin(2. * kPiD * lf)) * sr;
        const double d2 = (cd + 0.005 + 0.006 * depth * std::sin(2. * kPiD * (lf + 0.5))) * sr;
        const double w = chor.read(d) - chor.read(d2);   // adds width without shifting the left/right balance
        L += mix * w; R -= mix * w * width;
      }
    }
    (void)m;
    if (p.delayMix > 0.) {  // ping-pong, darker on every repeat
      const double tt = delaySeconds(p) * sr;
      if (dtSm < 0.) dtSm = tt; dtSm += (tt - dtSm) * (1. - std::exp(-1. / (0.06 * sr)));   // time changes glide like tape instead of clicking
      const double t = dtSm;
      const double a = dl.read(t), b = dr.read(t);
      const double dc = 1. - 0.93 * p.delayDamp, pg = p.delayPing; dlp[0] += dc * (a - dlp[0]); dlp[1] += dc * (b - dlp[1]);
      dl.push(m + (pg * dlp[1] + (1. - pg) * dlp[0]) * p.delayFb); dr.push((pg * dlp[0] + (1. - pg) * dlp[1]) * p.delayFb + (1. - pg) * m);
      L += p.delayMix * dlp[0]; R += p.delayMix * dlp[1];
      { const double x = p.delayMix * dlp[0], y = p.delayMix * dlp[1]; wt[0] = std::min(wt[0], x); wt[1] = std::max(wt[1], x); wt[2] = std::min(wt[2], y); wt[3] = std::max(wt[3], y); }
    }
    if (p.reverbMix > 0.) {  // mid and side are scaled separately: narrow centre, wide sides
      const double ra = 1. - std::exp(-2. * kPiD * std::max(20., (double)p.reverbLowCutHz) / sr);
      rhp += ra * (m - rhp); const double rin = m - rhp;
      double rin2 = rin; if (p.reverbPre > 0.) { pre.push(rin); rin2 = pre.read(std::max(1., p.reverbPre * 0.001 * sr)); }
      double wl, wr; rev.run(rin2, p.reverbSize, p.reverbDamp, wl, wr);
      const double wm = 0.5 * (wl + wr), ws = 0.5 * (wl - wr);
      const double outM = wm * 0.8, outS = ws * (0.4 + 1.2 * p.reverbWidth);
      const double k = 0.29;   // wet level matches the dry level at mix = 1 (measured with noise), so the knob reads as % wet
      const double mx = std::min(1., (double)p.reverbMix);
      { const double x = mx * (outM + outS) * k, y = mx * (outM - outS) * k; wt[4] = std::min(wt[4], x); wt[5] = std::max(wt[5], x); wt[6] = std::min(wt[6], y); wt[7] = std::max(wt[7], y); }
      L = L * (1. - mx) + mx * (outM + outS) * k; R = R * (1. - mx) + mx * (outM - outS) * k;
    }
  }
};

// ---- dynamics after the effects ----
inline double dbToLin(double d) { return std::pow(10., d * 0.05); }
inline double linToDb(double x) { return 20. * std::log10(std::max(x, 1e-6)); }
// 2nd-order Butterworth section; two in a row give a Linkwitz-Riley 4th-order crossover
struct Bq {
  double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  void set(bool hp, double fc, double sr) {
    double w = std::tan(kPiD * std::min(fc, 0.45 * sr) / sr), q = 0.70710678, n = 1. / (1. + w / q + w * w);
    if (hp) { b0 = n; b1 = -2. * n; b2 = n; } else { b0 = w * w * n; b1 = 2. * b0; b2 = b0; }
    a1 = 2. * (w * w - 1.) * n; a2 = (1. - w / q + w * w) * n;
  }
  double tick(double x) { double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};
struct Lr4 { Bq a, b; void set(bool hp, double fc, double sr) { a.set(hp, fc, sr); b.set(hp, fc, sr); } double tick(double x) { return b.tick(a.tick(x)); } };

struct Dynamics {
  double sr = 48000.;
  // opto (LA-2A style leveller): peak detector, soft knee 3:1, two release speeds (fast 60 ms, slow 1.5 s) so long compression lets go slowly
  double oEnv = 0., oFast = 0., oSlow = 0.;
  // squeeze: one-band glue compressor
  double sEnv = 0., sGr = 0.;
  double gr[3] = {0., 0., 0.};   // meters: opto, OTT average and squeeze gain reduction in dB (for the interface)
  // OTT: three bands, each with a level follower; crossovers at 120 Hz and 2.5 kHz
  Lr4 lo[2], hiSplit1[2], midLo[2], hiHp[2], apLo1[2], apLo2[2];
  double bEnv[3] = {0, 0, 0}, bGain[3] = {1, 1, 1};
  void prepare(double s) {
    sr = s; oEnv = oFast = oSlow = sEnv = sGr = 0.; for (int i = 0; i < 3; ++i) { bEnv[i] = 0.; bGain[i] = 1.; }
    for (int c = 0; c < 2; ++c) { lo[c] = Lr4(); hiSplit1[c] = Lr4(); midLo[c] = Lr4(); hiHp[c] = Lr4(); apLo1[c] = Lr4(); apLo2[c] = Lr4();
      lo[c].set(false, 120., sr); hiSplit1[c].set(true, 120., sr); midLo[c].set(false, 2500., sr); hiHp[c].set(true, 2500., sr);
      apLo1[c].set(false, 2500., sr); apLo2[c].set(true, 2500., sr); }
  }
  static double coef(double seconds, double sr) { return 1. - std::exp(-1. / std::max(1., seconds * sr)); }
  void process(const Params& p, float& L, float& R) { double l = L, r = R; process(p, l, r); L = (float)l; R = (float)r; }
  void process(const Params& p, double& L, double& R) {
    if (p.optoAmt > 0.) {
      double pk = std::max(std::fabs(L), std::fabs(R));
      oEnv += (pk > oEnv ? coef(0.010, sr) : coef(0.040, sr)) * (pk - oEnv);
      double db = linToDb(oEnv), T = -4. - 26. * p.optoAmt, over = db - T, knee = 12., gr;
      if (over <= -knee * 0.5) gr = 0.; else if (over >= knee * 0.5) gr = over * (1. - 1. / 3.);
      else { double x = over + knee * 0.5; gr = (1. - 1. / 3.) * x * x / (2. * knee); }
      oFast += (gr > oFast ? coef(0.010, sr) : coef(0.060 * p.optoRel, sr)) * (gr - oFast);
      oSlow += (gr > oSlow ? coef(0.300, sr) : coef(1.500 * p.optoRel, sr)) * (gr - oSlow);
      double g = dbToLin(-std::max(oFast, 0.8 * oSlow) + 0.45 * (-T) * (1. - 1. / 3.) + p.optoGain);   // auto makeup
      L *= g; R *= g;
    }
    if (p.ottMix > 0.) {
      double in[2] = {L, R}, band[2][3], depth = p.ottDepth;
      for (int c = 0; c < 2; ++c) {
        double low = lo[c].tick(in[c]), rest = hiSplit1[c].tick(in[c]);
        double mid = midLo[c].tick(rest), high = hiHp[c].tick(rest);
        double lowAp = apLo1[c].tick(low) + apLo2[c].tick(low);   // keep the low band in phase with the other two
        band[c][0] = lowAp; band[c][1] = mid; band[c][2] = high;
      }
      const double atk[3] = {0.0218, 0.0135, 0.0104}, rel[3] = {0.247, 0.132, 0.072}, tm = p.ottTime, trimB[3] = {p.ottLow, p.ottMid, p.ottHigh};   // seconds per band (mine, in the range such pedals use)
      double outL = 0., outR = 0.;
      for (int b = 0; b < 3; ++b) {
        double pk = std::max(std::fabs(band[0][b]), std::fabs(band[1][b]));
        bEnv[b] += (pk > bEnv[b] ? coef(atk[b] * tm, sr) : coef(rel[b] * tm, sr)) * (pk - bEnv[b]);
        double db = linToDb(bEnv[b]), gdb = 0.;
        const double Th = -14., Tl = -38., rD = 1. + 5. * depth, rU = 1. + 3. * depth;
        if (db > Th) gdb = -(db - Th) * (1. - 1. / rD);
        else if (db < Tl) gdb = std::min(18. * depth, (Tl - db) * (1. - 1. / rU)) * std::min(1., std::max(0., (db + 80.) / 20.));   // no boosting of silence
        double tg = dbToLin(gdb + 3. * depth + trimB[b]); bGain[b] += (tg - bGain[b]) * 0.05;
        outL += band[0][b] * bGain[b]; outR += band[1][b] * bGain[b];
      }
      L = L * (1. - p.ottMix) + outL * p.ottMix; R = R * (1. - p.ottMix) + outR * p.ottMix;
    }
    if (p.sqzAmt > 0.) {
      double pk = std::max(std::fabs(L), std::fabs(R));
      sEnv += (pk > sEnv ? coef(p.sqzAtk * 0.001, sr) : coef(p.sqzRel * 0.001, sr)) * (pk - sEnv);
      double db = linToDb(sEnv), T = -8. - 14. * p.sqzAmt, R_ = 1. + 3. * p.sqzAmt, over = db - T, knee = 6., gr;
      if (over <= -knee * 0.5) gr = 0.; else if (over >= knee * 0.5) gr = over * (1. - 1. / R_);
      else { double x = over + knee * 0.5; gr = (1. - 1. / R_) * x * x / (2. * knee); }
      sGr += (gr - sGr) * 0.01;
      double g = dbToLin(-sGr + 0.5 * (-T) * (1. - 1. / R_) + p.sqzMakeup);
      L *= g; R *= g;
    }
    gr[0] = p.optoAmt > 0. ? std::max(oFast, 0.8 * oSlow) : 0.;
    gr[1] = p.ottMix > 0. ? 20. * std::log10((bGain[0] + bGain[1] + bGain[2]) / 3. + 1e-9) : 0.;
    gr[2] = p.sqzAmt > 0. ? sGr : 0.;
  }
};

struct Master {
  double sr = 48000., env[3] = {0, 0, 0}, gn[3] = {1, 1, 1}, gr[3] = {0, 0, 0}, xl = 0., xr = 0., outL = 0., outR = 0.;
  Lr4 lo[2], hi1[2], midLo[2], hiHp[2], ap1[2], ap2[2];
  void prepare(double s) {
    sr = s; for (int i = 0; i < 3; ++i) { env[i] = 0.; gn[i] = 1.; gr[i] = 0.; } xl = xr = 0.;
    for (int c = 0; c < 2; ++c) { lo[c].set(false, 150., sr); hi1[c].set(true, 150., sr); midLo[c].set(false, 3000., sr); hiHp[c].set(true, 3000., sr); ap1[c].set(false, 3000., sr); ap2[c].set(true, 3000., sr); }
  }
  static double coef(double seconds, double sr) { return 1. - std::exp(-1. / std::max(1., seconds * sr)); }
  static double lncosh(double u) { double a = std::fabs(u); return a + std::log1p(std::exp(-2. * a)) - 0.6931471805599453; }
  static double clip(double x, double prev, double c) { double d = x - prev; if (std::fabs(d) < 1e-5) return c * std::tanh(0.5 * (x + prev) / c); return c * c * (lncosh(x / c) - lncosh(prev / c)) / d; }
  void process(const Params& p, double L, double R) {
    const double mix = p.dmMix;
    if (!(mix > 0.)) { gr[0] = gr[1] = gr[2] = 0.; outL = L; outR = R; return; }
    double in[2] = {L, R}, band[2][3];
    for (int c = 0; c < 2; ++c) {
      double low = lo[c].tick(in[c]), rest = hi1[c].tick(in[c]), mid = midLo[c].tick(rest), high = hiHp[c].tick(rest);
      band[c][0] = ap1[c].tick(low) + ap2[c].tick(low); band[c][1] = mid; band[c][2] = high;
    }
    const double atk[3] = {0.014, 0.007, 0.0025}, rel[3] = {0.16, 0.09, 0.05}, T0[3] = {-9., -10., -12.}, trim[3] = {p.dmLow, p.dmMid, p.dmHigh};
    const double R_ = 1.5 + 3.5 * p.dmDrive; double wl = 0., wr = 0.;
    for (int b = 0; b < 3; ++b) {
      double pk = std::max(std::fabs(band[0][b]), std::fabs(band[1][b]));
      env[b] += (pk > env[b] ? coef(atk[b], sr) : coef(rel[b], sr)) * (pk - env[b]);
      double T = T0[b] - 12. * p.dmDrive, over = linToDb(env[b]) - T, knee = 8., g;
      if (over <= -knee * 0.5) g = 0.; else if (over >= knee * 0.5) g = over * (1. - 1. / R_); else { double x = over + knee * 0.5; g = (1. - 1. / R_) * x * x / (2. * knee); }
      gn[b] += (dbToLin(-g + 0.85 * (-T) * (1. - 1. / R_) + trim[b] * 6.) - gn[b]) * 0.2; gr[b] = g;
      wl += band[0][b] * gn[b]; wr += band[1][b] * gn[b];
    }
    const double dl0 = band[0][0] + band[0][1] + band[0][2], dr0 = band[1][0] + band[1][1] + band[1][2];
    const double gc = 1. + p.dmClip * 1.2, c = 0.85, a = wl * gc, b2 = wr * gc;
    const double yl = clip(a, xl, c), yr = clip(b2, xr, c); xl = a; xr = b2;
    outL = dl0 * (1. - mix) + yl * mix; outR = dr0 * (1. - mix) + yr * mix;
  }
};

// ---- built-in samples: made from maths when the engine is prepared (no recorded or vendor audio). Same generator as makeSamples() in the playground engine. ----
struct SampleBank { std::vector<float> d[kSmpCount]; };
inline std::shared_ptr<const SampleBank> makeSamples(double sr) {
  auto bank = std::make_shared<SampleBank>();
  uint32_t seed = 987654321u;
  auto rnd = [&]() { seed = seed * 1664525u + 1013904223u; return (double)(seed >> 8) / 16777216.0 * 2.0 - 1.0; };
  const double TP = 2.0 * kPiD;
  auto N = [&](double sec) { return (int)std::floor(sec * sr); };
  auto sgn = [](double x) { return x > 0 ? 1.0 : (x < 0 ? -1.0 : 0.0); };
  typedef std::vector<float> Buf;
  auto fin = [&](int id, Buf a) { double pk = 1e-9; for (float v : a) pk = std::max(pk, (double)std::fabs(v)); const double g = 0.9 / pk; const int n = std::min(64, (int)a.size() >> 2);
    for (int i = 0; i < n; ++i) { float& v = a[a.size() - 1 - i]; v = (float)(v * ((double)i / n)); } for (float& v : a) v = (float)(v * g); bank->d[id] = std::move(a); };
  auto lpn = [&](Buf& a, double c) -> Buf& { double y = 0; for (float& v : a) { y += c * (v - y); v = (float)y; } return a; };
  auto hpn = [&](Buf& a, double c) -> Buf& { double y = 0; for (float& v : a) { y += c * (v - y); v = (float)(v - y); } return a; };
  auto bpn = [&](Buf& a, double f0, double f1) -> Buf& { lpn(a, 1 - std::exp(-TP * f1 / sr)); return hpn(a, 1 - std::exp(-TP * f0 / sr)); };
  auto sweep = [&](double len, double f0, double f1, double tau, double dec, double sat) { Buf a(N(len), 0.f); double ph = 0; for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr, f = f1 + (f0 - f1) * std::exp(-t / tau); ph += f / sr; const double v = std::sin(TP * ph) * std::exp(-t / dec); a[i] = (float)(sat != 0 ? std::tanh(v * sat) : v); } return a; };
  auto ks = [&](double len, double f, double damp, double bright) { const int D = std::max(2, (int)std::floor(sr / f + 0.5)); Buf buf(D, 0.f), a(N(len), 0.f); double lp = 0;
    for (int i = 0; i < D; ++i) { lp += bright * (rnd() - lp); const double r2 = rnd(); buf[i] = (float)(lp * 1.5 + r2 * 0.3 * bright); }
    int p = 0; double prev = 0; for (size_t i = 0; i < a.size(); ++i) { const double x = buf[p], y = (x + prev) * 0.5 * (1 - damp) + x * damp; prev = x; buf[p] = (float)(y * 0.9995); a[i] = (float)x; if (++p >= D) p = 0; } return a; };
  { Buf a(N(0.012), 0.f); for (size_t i = 0; i < a.size(); ++i) a[i] = (float)(rnd() * std::exp(-(double)i / sr / 0.0012)); fin(1, lpn(a, 0.25)); }
  { Buf a(N(0.01), 0.f); for (size_t i = 0; i < a.size(); ++i) a[i] = (float)(rnd() * std::exp(-(double)i / sr / 0.0007)); a[0] = 1.f; fin(2, hpn(a, 0.06)); }
  { Buf a(N(0.02), 0.f); for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr; const double r = rnd(); a[i] = (float)((std::sin(TP * 1900 * t) * 0.8 + r * 0.4) * std::exp(-t / 0.0022)); } fin(3, a); }
  { Buf a = sweep(0.4, 240, 48, 0.035, 0.12, 0); for (int i = 0; i < N(0.004); ++i) { const double r = rnd(); a[i] = (float)(a[i] + r * 0.3 * (1 - (double)i / N(0.004))); } fin(4, a); }
  fin(5, sweep(0.45, 520, 52, 0.022, 0.16, 3.2));
  fin(6, sweep(0.6, 70, 38, 0.06, 0.22, 0));
  { Buf a(N(0.35), 0.f), t1 = sweep(0.2, 260, 185, 0.02, 0.05, 0), nz(a.size(), 0.f); for (size_t i = 0; i < nz.size(); ++i) nz[i] = (float)(rnd() * std::exp(-(double)i / sr / 0.07)); bpn(nz, 900, 9000); for (size_t i = 0; i < a.size(); ++i) a[i] = (float)(nz[i] * 1.1 + (i < t1.size() ? t1[i] * 0.7 : 0.0)); fin(7, a); }
  { Buf a(N(0.2), 0.f), t1 = sweep(0.1, 300, 210, 0.012, 0.03, 0), nz(a.size(), 0.f); for (size_t i = 0; i < nz.size(); ++i) nz[i] = (float)(rnd() * std::exp(-(double)i / sr / 0.035)); bpn(nz, 1200, 8000); for (size_t i = 0; i < a.size(); ++i) a[i] = (float)(nz[i] * 1.2 + (i < t1.size() ? t1[i] * 0.5 : 0.0)); fin(8, a); }
  { Buf a(N(0.32), 0.f); for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr; double e = 0; for (int k = 0; k < 4; ++k) { const double tk = t - k * 0.011; if (tk >= 0) e += (k < 3 ? std::exp(-tk / 0.003) : std::exp(-tk / 0.09)); } a[i] = (float)(rnd() * e); } fin(9, bpn(a, 800, 3500)); }
  { Buf a(N(0.08), 0.f); for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr; const double r = rnd(); a[i] = (float)((std::sin(TP * 1750 * t) * 0.7 + std::sin(TP * 480 * t) * 0.6 + r * 0.3) * std::exp(-t / 0.012)); } fin(10, a); }
  { Buf a(N(0.12), 0.f); for (size_t i = 0; i < a.size(); ++i) a[i] = (float)(rnd() * std::exp(-(double)i / sr / 0.016)); hpn(a, 0.55); fin(11, hpn(a, 0.5)); }
  { Buf a(N(0.6), 0.f); const double r6[6] = {1, 1.4471, 1.6170, 1.9265, 2.5028, 2.6637}; for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr; double v = 0; for (double q : r6) v += sgn(std::sin(TP * 330 * q * t)); const double r = rnd(); a[i] = (float)((v / 6 * 0.7 + r * 0.5) * std::exp(-t / 0.11)); } hpn(a, 0.5); fin(12, hpn(a, 0.45)); }
  fin(13, ks(1.4, 261.63, 0.55, 0.35));
  fin(14, ks(1.6, 261.63, 0.12, 1.0));
  { Buf a = ks(0.5, 261.63, 0.7, 0.8); for (size_t i = 0; i < a.size(); ++i) a[i] = (float)(a[i] * std::exp(-(double)i / sr / 0.14)); fin(15, a); }
  { Buf a(N(1.8), 0.f); const double f = 261.63; for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr, m = std::sin(TP * f * 3.5 * t) * 2.6 * std::exp(-t / 0.18); a[i] = (float)(std::sin(TP * f * t + m) * std::exp(-t / 0.5)); } fin(16, a); }
  { Buf a(N(2.4), 0.f); const double f = 523.25, pr[5] = {1, 2.76, 5.4, 8.93, 11.34}, am[5] = {1, 0.55, 0.3, 0.18, 0.1}, dc[5] = {1.4, 0.7, 0.4, 0.25, 0.15}; for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr; double v = 0; for (int k = 0; k < 5; ++k) v += am[k] * std::sin(TP * f * pr[k] * t) * std::exp(-t / dc[k]); a[i] = (float)v; } fin(17, a); }
  { Buf a(N(0.22), 0.f); double ph = 0; for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr, f = 160 + 4200 * std::exp(-t / 0.03); ph += f / sr; a[i] = (float)(sgn(std::sin(TP * ph)) * 0.6 * std::exp(-t / 0.07) + std::sin(TP * ph) * 0.4 * std::exp(-t / 0.07)); } fin(18, a); }
  { Buf a(N(0.5), 0.f); for (size_t i = 0; i < a.size(); ++i) { const double t = (double)i / sr; a[i] = (float)(rnd() * std::pow(std::sin(kPiD * std::min(1.0, t / 0.5)), 1.5) * std::exp(-t / 0.3)); } fin(19, bpn(a, 500, 6500)); }
  return bank;
}
inline std::shared_ptr<const SampleBank> sampleBank(double sr) {   // one bank per sample rate, shared by all engines
  static std::mutex mu; static std::map<int, std::shared_ptr<const SampleBank>> cache; std::lock_guard<std::mutex> lk(mu);
  auto& e = cache[(int)std::lround(sr)]; if (!e) e = makeSamples(sr); return e;
}

class Engine {
 public:
  Params p;
  void prepare(double sampleRate) { sr = sampleRate; for (auto& v : voices) v = Voice(); drv = DriveStage(); crush = Crush(); wid = WidthStage(); wid.prepare(sr); eq.prepare(sr); bl = br = 0.0; bl2 = br2 = 0.0; limEnv = 0.0; limAtk = 1.0 - std::exp(-1.0 / (0.0004 * sr)); limRel = 1.0 - std::exp(-1.0 / (0.12 * sr));
    fx.prepare(sr); dyn.prepare(sr); dm.prepare(sr); bank = sampleBank(sr); if (combPool.size() != (size_t)kMaxVoices * 6 * 4096) combPool.assign((size_t)kMaxVoices * 6 * 4096, 0.f); for (double& g : mgS) g = 0; }
  void envsOn(Voice& v) { v.amp.on(); v.amp2.on(); v.amp3.on(); v.fenv.on(); v.fenv2.on(); v.fenv3.on(); }
  void envsOff(Voice& v) { v.amp.off(); v.amp2.off(); v.amp3.off(); v.fenv.off(); v.fenv2.off(); v.fenv3.off(); }
  void noteOn(int note, float vel) {
    if (p.mono) {  // reuse the one sounding voice so tails never pile up
      Voice* v = nullptr;
      for (auto& c : voices) if (c.active) { if (!v || c.age > v->age) v = &c; }
      if (v) { for (auto& c : voices) if (&c != v) c.active = false; v->gl += (double)(v->note - note); v->note = note; v->vel = vel; v->age = ++clock; v->t = 0.0; v->rnd = 2.0 * rng.next() - 1.0; v->sh = v->rnd; envsOn(*v); v->sp[0] = v->sp[1] = v->sp[2] = v->sp[3] = 0.0; return; }
    }
    const int n = p.mono ? 1 : std::max(1, std::min(4, p.stackCount));   // Stack mode plays several detuned copies per key
    for (int k = 0; k < n; ++k) {
      Voice* v = allocVoice();
      *v = Voice(); v->active = true; v->note = note; v->vel = vel; v->rnd = 2.0 * rng.next() - 1.0; v->sh = v->rnd; v->sh0 = 0.0; v->age = ++clock;
      v->rs = ((uint32_t)(note + 1) * 2654435761u) ^ ((uint32_t)v->age * 40503u); v->sa[0] = v->lfoRnd(); v->sa[1] = v->lfoRnd(); v->sb[0] = v->sb[1] = 0.0;   // each voice has its own random stream for LFO 2 and 3
      v->stackCents = n > 1 ? ((double)k / (double)(n - 1) * 2.0 - 1.0) * p.stackCents : 0.0; v->stackGain = 1.0 / std::sqrt((double)n);
      for (int l = 0; l < kLayers; ++l) for (int j = 0; j < kMaxSaws; ++j) v->phase[l][j] = p.phaseReset ? 0.0 : rng.next();
      envsOn(*v);
      for (int l = 0; l < kLayers; ++l) if (p.layer[l].wave == Wave::Sample && p.layer[l].sample && p.layer[l].sample->oneShot) v->oneShot = true;
    }
  }
  void noteOff(int note) { for (auto& v : voices) if (v.active && v.note == note && v.amp.stage != 3 && !v.oneShot) envsOff(v); }
  void allOff() { for (auto& v : voices) v.active = false; }
  struct Meters { float l = 0.f, r = 0.f, lim = 0.f; int active = 0, trips = 0; float gr[3] = {0, 0, 0}, dm[3] = {0, 0, 0}; };
  void wetTake(float* o) { fx.wetTake(o); }
  Meters meters() const { Meters m = mtr; m.lim = (float)limEnv; m.trips = trips; for (int i = 0; i < 3; ++i) { m.gr[i] = (float)dyn.gr[i]; m.dm[i] = (float)dm.gr[i]; } return m; }
  void reserve(int maxBlock) { if ((int)subBusL.size() < maxBlock) { subBusL.assign(maxBlock, 0.0); subBusR.assign(maxBlock, 0.0); } }
  int activeVoices() const { int n = 0; for (const auto& v : voices) if (v.active) ++n; return n; }

  // adds nothing, overwrites outL/outR
  void render(float* outL, float* outR, int n) {
    for (int i = 0; i < n; ++i) outL[i] = outR[i] = 0.f;
    if ((int)subBusL.size() < n) { subBusL.assign(n, 0.0); subBusR.assign(n, 0.0); }
    std::fill(subBusL.begin(), subBusL.begin() + n, 0.0); std::fill(subBusR.begin(), subBusR.begin() + n, 0.0); sbL = subBusL.data(); sbR = subBusR.data();
    if (combPool.empty()) combPool.assign((size_t)kMaxVoices * 6 * 4096, 0.f);
    if (!bank) bank = sampleBank(sr);
    mxn = 0; for (int a = 0; a < kMxSrc; ++a) for (int b = 0; b < kMxDst; ++b) if (p.mx[a][b] != 0.f) { mxs[mxn] = a; mxd[mxn] = b; mxv[mxn] = p.mx[a][b]; ++mxn; }
    for (int vi = 0; vi < kMaxVoices; ++vi) if (voices[vi].active) renderVoice(voices[vi], vi, outL, outR, n);
    mtr.active = activeVoices();
    globals();
    // bass mono: sum everything below bassMonoHz, keep the rest stereo
    eq.update(pe); const double a = 1.0 - std::exp(-2.0 * kPiD * std::max(10.0, (double)p.bassMonoHz) / sr); bool bad = false;
    for (int i = 0; i < n; ++i) {
      drv.process(pe, outL[i], outR[i], sbL[i], sbR[i]); crush.process(pe, drv.oL, drv.oR); const double sbl = drv.sL, sbr = drv.sR;
      double fl = crush.oL, fr = crush.oR; fx.process(pe, fl, fr);
      double dl = fl, dr = fr;
      dyn.process(p, dl, dr);
      dl = fl + (dl - fl) * p.dynMix; dr = fr + (dr - fr) * p.dynMix;
      bl += a * (dl - bl); bl2 += a * (bl - bl2);
      br += a * (dr - br); br2 += a * (br - br2);
      const double m = 0.5 * (bl2 + br2);
      wid.process(pe, dl - bl2 + m, dr - br2 + m);
      dm.process(pe, wid.oL, wid.oR);
      eq.process(dm.outL + sbl, dm.outR + sbr); double xl = eq.oL * pe.master, xr = eq.oR * pe.master;
      if (p.limiter) {   // feed-forward peak limiter, 0.4 ms attack, 120 ms release; overshoots are caught by the smooth clipper
        const double pk = std::max(std::fabs(xl), std::fabs(xr));
        limEnv += (pk > limEnv ? limAtk : limRel) * (pk - limEnv);
        if (limEnv > p.limCeil) { const double g = p.limCeil / limEnv; xl *= g; xr *= g; }
      }
      outL[i] = (float)softClip(xl, p.clipKnee); outR[i] = (float)softClip(xr, p.clipKnee);
      if (!std::isfinite(outL[i] + outR[i])) bad = true;
    }
    if (bad) {   // safety guard: a bad value anywhere resets the engine instead of blasting or hanging the host
      ++trips; for (auto& v : voices) v = Voice(); drv = DriveStage(); crush = Crush(); wid = WidthStage(); wid.prepare(sr); eq.prepare(sr); fx.prepare(sr); dyn.prepare(sr); dm.prepare(sr); bl = br = bl2 = br2 = limEnv = 0.0; for (double& g : mgS) g = 0;
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
  double sr = 48000.0; Voice voices[kMaxVoices]; Rng rng; DriveStage drv; Crush crush; WidthStage wid; EqStage eq; Fx fx; Dynamics dyn; Master dm; uint64_t clock = 0;
  int trips = 0;   // how many times the safety guard reset the engine
  Meters mtr; Params pe;   // pe: the settings the effects really use this block (p plus global modulation and the Build macro)
  std::shared_ptr<const SampleBank> bank; std::vector<float> combPool;
  std::vector<double> subBusL, subBusR; double* sbL = nullptr; double* sbR = nullptr;
  double bl = 0, br = 0, bl2 = 0, br2 = 0, limEnv = 0, limAtk = 0.05, limRel = 0.0002;
  int mxs[kMxSrc * kMxDst], mxd[kMxSrc * kMxDst], mxn = 0; double mxv[kMxSrc * kMxDst]; double mgS[kMxDst] = {};

  // drawn modulation envelope k (0..2) at time t
  double menvAt(int k, double t) const {
    const float* m = k == 0 ? p.menv : (k == 1 ? p.menv2 : p.menv3); const bool sy = k == 0 ? p.menvSync : (k == 1 ? p.menv2Sync : p.menv3Sync); const int dv = std::max(0, std::min(15, k == 0 ? p.menvDiv : (k == 1 ? p.menv2Div : p.menv3Div)));
    const double T = std::max(0.02, sy ? kMenvBeats[dv] * 60. / std::max(20., std::min(400., (double)p.bpm)) : (double)(k == 0 ? p.menvTime : (k == 1 ? p.menv2Time : p.menv3Time))); const bool lp = k == 0 ? p.menvLoop : (k == 1 ? p.menv2Loop : p.menv3Loop);
    double x = t / T; if (lp) x -= std::floor(x); else if (x > 1) x = 1; const double f = x * 63; const int i = std::min(62, (int)f); return m[i] + (m[i + 1] - m[i]) * (f - i);
  }
  // global modulation (reverb, delay, drive, clipper, high pass, low pass, width) comes from the newest sounding voice; the Build macro adds to it
  void globals() {
    pe = p; double mg[kMxDst] = {}; const double b = std::max(0.0, std::min(1.0, (double)p.build));
    const Voice* y = nullptr; if (mxn) { for (const auto& v : voices) if (v.active && (!y || v.age > y->age)) y = &v; }
    if (y) for (int k = 0; k < mxn; ++k) { const int d = mxd[k]; if (d >= 16) mg[d] += y->svL[mxs[k]] * mxv[k]; }
    for (int d = 16; d < kMxDst; ++d) mgS[d] += 0.35 * (mg[d] - mgS[d]);
    const double* g = mgS; auto c01 = [](double x) { return x < 0 ? 0.0 : x > 1 ? 1.0 : x; };
    const double hpB = b <= 0 ? 0 : (b < 0.5 ? 20 + 130 * std::pow(2 * b, 2.0) : 150 * std::pow(2500.0 / 150.0, (b - 0.5) * 2));
    pe.reverbMix = (float)c01(p.reverbMix + g[16] + 0.5 * b); pe.delayMix = (float)c01(p.delayMix + g[17] + 0.2 * b);
    pe.reverbLowCutHz = (float)(b > 0 ? p.reverbLowCutHz + std::max(0.0, 500.0 - p.reverbLowCutHz) * b : (double)p.reverbLowCutHz);
    pe.drvDrive = (float)c01(p.drvDrive + g[18]); pe.dmClip = (float)c01(p.dmClip + g[19] + 0.5 * std::log10(1 + 9 * b));
    const double hm = g[20], f1 = p.eq1F * std::pow(2.0, 6 * hm); pe.eq1On = p.eq1On || hm > 0.002 || b > 0.001;
    pe.eq1F = (float)std::min(20000.0, std::max(20.0, b > 0.001 ? std::max((p.eq1On || hm > 0.002) ? f1 : 0.0, hpB) : f1));
    const double lm = g[21]; pe.eq6On = p.eq6On || lm < -0.002; pe.eq6F = (float)std::min(20000.0, std::max(20.0, p.eq6F * std::pow(2.0, 6 * std::min(0.0, lm))));
    pe.widHi = (float)(std::min(2.0, std::max(0.0, p.widHi + g[22])) * (1 - 0.3 * b)); pe.widMid = (float)(std::min(2.0, std::max(0.0, p.widMid + 0.5 * g[22])) * (1 - 0.15 * b));
    pe.master = (float)(p.master * std::pow(10.0, -2.5 * b / 20));
  }

  // sub oscillator (mono, centred). After the filter by default so the low end stays solid; before it when subPreFilter is set.
  void addSub(Voice& v, double& L, double& R, double invSr, double lvl) {
    if (lvl <= 0.0) return;
    const int nv = std::max(1, std::min(5, p.subVoices));
    if (nv > 1) {   // several detuned sub oscillators, spread across the stereo field
      const double sf0 = 440.0 * std::pow(2.0, ((double)(v.note - 69) + (double)p.subOctave * 12.0) / 12.0); double sl = 0.0, srr = 0.0;
      for (int k = 0; k < nv; ++k) {
        const double x = 2.0 * k / (nv - 1) - 1.0, sdt = sf0 * std::pow(2.0, x * p.subDetune * 30.0 / 1200.0) * invSr; if (sdt > 0.45) continue; double sp = v.subPh[k], s;
        switch (p.subWave) {
          case Wave::Tri: s = 4.0 * std::fabs(sp - 0.5) - 1.0; break;
          case Wave::Pulse: s = (sp < 0.5 ? 1.0 : -1.0) + blep(sp, sdt) - blep(sp < 0.5 ? sp + 0.5 : sp - 0.5, sdt); s *= 0.8; break;
          case Wave::Saw: s = (2.0 * sp - 1.0) - blep(sp, sdt); break;
          default: s = std::sin(2.0 * kPiD * sp); break;
        }
        sp += sdt; if (sp >= 1.0) sp -= 1.0; v.subPh[k] = sp;
        const double pa = (x * p.subWidth + 1.0) * kPiD / 4.0; sl += s * 1.41421356 * std::cos(pa); srr += s * 1.41421356 * std::sin(pa);
      }
      const double nrm = lvl * 0.7 / std::sqrt((double)nv); L += sl * nrm; R += srr * nrm; return;
    }
    const double sf = 440.0 * std::pow(2.0, ((double)(v.note - 69) + (double)p.subOctave * 12.0) / 12.0), sdt = sf * invSr; if (sdt > 0.45) return; double sp = v.subPhase, s;
    switch (p.subWave) {
      case Wave::Tri: s = 4.0 * std::fabs(sp - 0.5) - 1.0; break;
      case Wave::Pulse: s = (sp < 0.5 ? 1.0 : -1.0) + blep(sp, sdt) - blep(sp < 0.5 ? sp + 0.5 : sp - 0.5, sdt); s *= 0.8; break;
      case Wave::Saw: s = (2.0 * sp - 1.0) - blep(sp, sdt); break;
      default: s = std::sin(2.0 * kPiD * sp); break;
    }
    sp += sdt; if (sp >= 1.0) sp -= 1.0; v.subPhase = sp;
    L += lvl * s * 0.7; R += lvl * s * 0.7;
  }

  void renderVoice(Voice& v, int vi, float* oL, float* oR, int n) {
    const double invSr = 1.0 / sr; const bool link = p.ampLink, hasMx = mxn > 0;
    double sv[kMxSrc] = {}, md[kMxDst] = {};
    for (int q = 0; q < 3; ++q) { v.fl[q].cb = &combPool[((size_t)vi * 6 + q * 2) * 4096]; v.fr[q].cb = &combPool[((size_t)vi * 6 + q * 2 + 1) * 4096]; }
    bool usedAsSrc[kLayers] = {}, anyFmSub = false, anyFmNz = false;
    for (int ly = 0; ly < kLayers; ++ly) { const LayerParams& q = p.layer[ly]; if (!q.on || q.wave == Wave::Off || q.level <= 0.f) continue;
      if (q.fmAmt > 0.f && q.fmSrc >= 1 && q.fmSrc <= 3) usedAsSrc[q.fmSrc - 1] = true; if (q.amAmt > 0.f && q.amSrc >= 1 && q.amSrc <= 3) usedAsSrc[q.amSrc - 1] = true;
      if ((q.fmAmt > 0.f && q.fmSrc == 4) || (q.amAmt > 0.f && q.amSrc == 4)) anyFmSub = true; if ((q.fmAmt > 0.f && q.fmSrc == 5) || (q.amAmt > 0.f && q.amSrc == 5)) anyFmNz = true; }
    // per-block tables for the supersaw layers (pan law and spread depend only on settings, so they are not recomputed per sample)
    double tSp[kLayers][kMaxSaws], tCl[kLayers][kMaxSaws], tCr[kLayers][kMaxSaws]; bool tOn[kLayers][kMaxSaws];
    for (int ly = 0; ly < kLayers; ++ly) { const LayerParams& q = p.layer[ly];
      if (!(q.wave == Wave::SuperSaw || q.wave == Wave::SuperSquare || (q.wave == Wave::Custom && q.table && q.table->ready()))) continue;
      const int nv = std::max(1, std::min(kMaxSaws, q.voices)); const double sideFade = q.cents > 0.f ? 1.0 : std::min(1.0, q.detune * 20.0), norm = 1.0 / (1.0 + (double)(nv - 1) * 0.62 * sideFade);
      for (int j = 0; j < nv; ++j) { const double sp = sawSpread(j, nv); const bool centre = (nv % 2 == 1 && j == nv / 2); double g = centre ? 1.0 : 0.62 * sideFade; g *= norm;
        const double ang = (sp * p.spread + 1.0) * kPiD * 0.25; tSp[ly][j] = sp; tOn[ly][j] = g > 0.0; tCl[ly][j] = g * std::cos(ang) * 1.414; tCr[ly][j] = g * std::sin(ang) * 1.414; } }
    const double dk = 1.0 + p.dist * 14.0, dnm = 1.0 / std::tanh(dk * 0.6);
    const FilterMode modes[3] = {p.filterMode, p.f2Mode, p.f3Mode}; const bool parallel = p.fRoute == 1, post = p.smpPost; const int lfoS[3] = {p.lfoShape, p.lfo2Shape, p.lfo3Shape};
    const double rr[3] = {0, p.f2Res, p.f3Res}, dd[3] = {0, p.f2Drv, p.f3Drv}; double srcV[6];
    for (int i = 0; i < n; ++i) {
      const double ampE = v.amp.tick(p.amp, sr); double aE2 = ampE, aE3 = ampE;
      if (!link) { aE2 = v.amp2.tick(p.amp2, sr); aE3 = v.amp3.tick(p.amp3, sr); }
      const double fE = v.fenv.tick(p.fenv, sr), fE2 = v.fenv2.tick(p.fenv2, sr), fE3 = v.fenv3.tick(p.fenv3, sr);
      if (v.amp.stage == 0 && (link || ((!p.layer[1].on || p.layer[1].wave == Wave::Off || v.amp2.stage == 0) && (!p.layer[2].on || p.layer[2].wave == Wave::Off || v.amp3.stage == 0) && (!p.layer[0].on || p.layer[0].wave == Wave::Off || v.amp.stage == 0)))) { v.active = false; return; }
      const bool ctl = (v.ctr++ & 3) == 0;
      v.lfoPhase += p.lfoRate * invSr; if (v.lfoPhase >= 1.0) { v.lfoPhase -= 1.0; v.sh0 = v.sh; v.sh = 2.0 * rng.next() - 1.0; }
      for (int k = 0; k < 2; ++k) { v.lp2[k] += (k ? p.lfo3Rate : p.lfo2Rate) * invSr; if (v.lp2[k] >= 1.0) { v.lp2[k] -= 1.0; v.sb[k] = v.sa[k]; v.sa[k] = v.lfoRnd(); } }
      double lfo, lfo2, lfo3;
      if (ctl) {
        double lph = v.lfoPhase + p.lfoStart; if (lph >= 1.0) lph -= 1.0;
        lfo = lfoWave(lfoS[0], lph, v.sh, v.sh0) * p.lfoDepth * (p.lfoFade > 0.f ? std::min(1.0, v.t / p.lfoFade) : 1.0); v.lfoC = lfo;
        double q2 = v.lp2[0] + p.lfo2Start; if (q2 >= 1.0) q2 -= 1.0; lfo2 = lfoWave(lfoS[1], q2, v.sa[0], v.sb[0]) * p.lfo2Depth * (p.lfo2Fade > 0.f ? std::min(1.0, v.t / p.lfo2Fade) : 1.0); v.lc2[0] = lfo2;
        double q3 = v.lp2[1] + p.lfo3Start; if (q3 >= 1.0) q3 -= 1.0; lfo3 = lfoWave(lfoS[2], q3, v.sa[1], v.sb[1]) * p.lfo3Depth * (p.lfo3Fade > 0.f ? std::min(1.0, v.t / p.lfo3Fade) : 1.0); v.lc2[1] = lfo3;
      } else { lfo = v.lfoC; lfo2 = v.lc2[0]; lfo3 = v.lc2[1]; }
      double pitchEnv;
      if (ctl) {
        const double pAtt = p.pitchAttack > 0.f ? std::min(1.0, v.t / p.pitchAttack) : 1.0;
        const double pd = std::max(0.0, v.t - p.pitchAttack - p.pitchHold), pt = std::max(0.001, (double)p.pitchTime);
        pitchEnv = p.pitchAmt * pAtt * ((1.0 - p.pitchCurve) * std::exp(-pd / pt) + p.pitchCurve * std::max(0.0, 1.0 - pd / (3.0 * pt))) - p.pitchFall * (1.0 - std::exp(-v.t / std::max(0.01, (double)p.pitchFallTime)));
        v.pEnv = pitchEnv;
      } else pitchEnv = v.pEnv;
      v.t += invSr;
      if (v.gl != 0.0) { v.gl *= std::exp(-invSr / std::max(0.001, (double)p.glide)); if (std::fabs(v.gl) < 1e-4) v.gl = 0.0; }
      if ((long long)std::floor(v.t * sr) % 512 == 0)
        for (int j = 0; j < kMaxSaws; ++j) v.driftT[j] = p.drift * 0.004 * (2.0 * rng.next() - 1.0);
      if (ctl) for (int j = 0; j < kMaxSaws; ++j) v.drift[j] += 0.002 * (v.driftT[j] - v.drift[j]);

      if (hasMx) {
        for (int k = 0; k < kMxDst; ++k) md[k] = 0.0;
        sv[0] = lfo; sv[1] = fE; sv[2] = ampE; sv[3] = std::max(-2.0, std::min(2.0, pitchEnv / 12.0)); sv[4] = p.modWheel;
        sv[5] = std::max(-2.0, std::min(2.0, (double)(v.note - 60) / 24.0)); sv[6] = v.vel; sv[7] = v.rnd;
        sv[8] = p.menvInv ? 1.0 - menvAt(0, v.t) : menvAt(0, v.t); sv[9] = lfo2; sv[10] = lfo3; sv[11] = fE2; sv[12] = fE3;
        sv[13] = p.menv2Inv ? 1.0 - menvAt(1, v.t) : menvAt(1, v.t); sv[14] = p.menv3Inv ? 1.0 - menvAt(2, v.t) : menvAt(2, v.t);
        for (int k = 0; k < mxn; ++k) md[mxd[k]] += sv[mxs[k]] * mxv[k];
        if (i == n - 1) for (int k = 0; k < kMxSrc; ++k) v.svL[k] = sv[k];
      }
      double L = 0.0, R = 0.0, smL = 0.0, smR = 0.0;
      srcV[0] = 0.0; srcV[1] = v.lyOut[0]; srcV[2] = v.lyOut[1]; srcV[3] = v.lyOut[2]; srcV[4] = 0.0; srcV[5] = 0.0;
      if (anyFmSub) { srcV[4] = std::sin(2.0 * kPiD * v.fmSubPh); v.fmSubPh += 440.0 * std::pow(2.0, ((double)(v.note - 69) + (double)p.subOctave * 12.0) / 12.0) * invSr; if (v.fmSubPh >= 1.0) v.fmSubPh -= 1.0; }
      if (anyFmNz) srcV[5] = 2.0 * rng.next() - 1.0;
      for (int ly = 0; ly < kLayers; ++ly) {
        const LayerParams& lp = p.layer[ly]; const double le = link ? 1.0 : (ly == 0 ? ampE : (ly == 1 ? aE2 : aE3));
        if (ctl) { const double pg = (std::min(1.0, std::max(-1.0, lp.pan + p.lfoPan * lfo + md[6])) + 1.0) * kPiD * 0.25; v.pcC[ly] = std::cos(pg) * 1.414; v.psC[ly] = std::sin(pg) * 1.414; }
        if (lp.smp != 0 && bank) { const std::vector<float>& d = bank->d[lp.smp]; if (!d.empty()) {
          if (ctl) v.sq[ly] = std::pow(2.0, (lp.smpPitch + (smpKeyed(lp.smp) ? v.note - 60 + v.gl : 0.0)) / 12.0);
          const double pos = v.sp[ly]; const int i0 = (int)pos;
          if (i0 < (int)d.size() - 1) { const double f = pos - i0, sm = (d[i0] * (1.0 - f) + d[i0 + 1] * f) * lp.smpVol * le; v.sp[ly] = pos + v.sq[ly]; smL += sm * v.pcC[ly]; smR += sm * v.psC[ly]; } } }
        if (!lp.on || lp.wave == Wave::Off || (lp.level <= 0.f && !usedAsSrc[ly])) { v.lyOut[ly] = 0.0; continue; }
        double f;
        if (ctl) { const double semis = (double)(v.note - 69) + v.gl + (double)lp.octave * 12.0 + lp.tune * 0.01 + pitchEnv + p.lfoPitch * lfo * 0.3 + lp.lfoPitchCents * 0.01 * lfo + v.stackCents * 0.01 + p.wheelVib * p.modWheel * 0.01 * lfo + md[0] * 12.0;
          f = 440.0 * std::pow(2.0, semis / 12.0); v.fC[ly] = f; } else f = v.fC[ly];
        double fmF = 1.0; if (lp.fmAmt > 0.f && lp.fmSrc >= 1 && lp.fmSrc <= 5 && lp.fmSrc != ly + 1) fmF = std::max(0.02, 1.0 + lp.fmAmt * 3.0 * srcV[lp.fmSrc]);
        const double dt = std::min(0.45, f * invSr * fmF);
        double lL = 0.0, lR = 0.0;
        if (lp.wave == Wave::SuperSaw || lp.wave == Wave::SuperSquare || (lp.wave == Wave::Custom && lp.table && lp.table->ready())) {
          const int nv = std::max(1, std::min(kMaxSaws, lp.voices));
          const double det = lp.cents > 0.f ? std::pow(2.0, std::max(0.0, lp.cents + md[5] * 50.0) / 1200.0) - 1.0 : jpDetune(std::min(1.0, std::max(0.0, lp.detune * (1.0 + p.lfoDetune * lfo) + md[5] * 0.5))) * kMaxDetune * p.detuneRange;
          const double pw = std::min(0.95, std::max(0.05, lp.pwm + p.lfoPwm * lfo * 0.45 + md[4] * 0.45));
          for (int j = 0; j < nv; ++j) {
            if (!tOn[ly][j]) continue;
            const double d = std::min(0.45, dt * (1.0 + det * tSp[ly][j] + v.drift[j]));
            double x = v.phase[ly][j], s;
            if (lp.wave == Wave::SuperSaw) s = (2.0 * x - 1.0) - blep(x, d);
            else if (lp.wave == Wave::Custom) s = lp.table->read((float)x, (float)d);
            else {
              s = (x < pw ? 1.0 : -1.0) + blep(x, d);
              double t2 = x - pw; if (t2 < 0.0) t2 += 1.0;
              s -= blep(t2, d);
              s *= 0.8;
            }
            x += d; if (x >= 1.0) x -= 1.0; v.phase[ly][j] = x;
            lL += tCl[ly][j] * s; lR += tCr[ly][j] * s;
          }
        } else if (lp.wave == Wave::Sample && lp.sample && !lp.sample->l.empty()) {
          const Sample& sm = *lp.sample;
          double rate = (double)(f / midiToHz((double)sm.rootNote)) * sm.sampleRate / sr;
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
            double mid = 0.5 * (sl + sr_), side = 0.5 * (sl - sr_) * p.spread;
            lL = mid + side; lR = mid - side;
            pos += rate;
          }
        } else {
          const double pw = std::min(0.95, std::max(0.05, lp.pwm + p.lfoPwm * lfo * 0.45 + md[4] * 0.45));
          double odt = dt;
          if (lp.sync) {   // with sync on, the slave oscillator makes the sound and the master only restarts it
            const double sem = std::min(48.0, std::max(0.0, lp.syncSemis + lp.syncEnv * fE + lp.syncLfo * lfo + lp.syncKey * (double)(v.note - 60) / 12.0 + lp.syncWheel * p.modWheel + md[8 + ly] * 12.0));
            odt = std::min(0.45, dt * std::pow(2.0, sem / 12.0));
          }
          auto shape = [&](double x, double d) {
            double w = 0.0;
            switch (lp.wave) {
              case Wave::Saw:   w = (2.0 * x - 1.0) - blep(x, d); break;
              case Wave::Pulse: { w = (x < pw ? 1.0 : -1.0) + blep(x, d); double t2 = x - pw; if (t2 < 0) t2 += 1.0; w -= blep(t2, d); w *= 0.8; } break;
              case Wave::Tri:   w = 4.0 * std::fabs(x - 0.5) - 1.0; break;
              default:          w = std::sin(2.0 * kPiD * x); break;
            }
            return w;
          };
          double& ph0 = v.phase[ly][0]; const double op = lp.sync ? v.syncPhase[ly] : ph0;
          double s = shape(op, odt); bool wrapped = false; ph0 += dt; if (ph0 >= 1.0) { ph0 -= 1.0; wrapped = true; }
          if (lp.sync) {
            const double before = s; double sph = v.syncPhase[ly] + odt;
            if (wrapped) { const double frac = ph0 / dt; sph = frac * odt; const double after = shape(sph, odt); s = before * frac + after * (1.0 - frac); }   // short crossfade over the step: first-order band limiting
            else if (sph >= 1.0) sph -= 1.0;
            v.syncPhase[ly] = sph;
          }
          lL = lR = s;
        }
        v.lyOut[ly] = 0.5 * (lL + lR);
        if (lp.amAmt > 0.f && lp.amSrc >= 1 && lp.amSrc <= 5 && lp.amSrc != ly + 1) { const double am = 1.0 - lp.amAmt + lp.amAmt * srcV[lp.amSrc]; lL *= am; lR *= am; }
        L += lp.level * lL * v.pcC[ly] * le; R += lp.level * lR * v.psC[ly] * le;
      }
      const double nzl = std::max(0.0, p.noiseLevel + md[12] * 0.5), subL = std::max(0.0, p.subLevel + md[11] * 0.7), en = link ? 1.0 : ampE;
      if (nzl > 0.0) {  // white noise, high-passed (two poles) so only the top end is added
        const int nvz = std::max(1, std::min(5, p.noiseVoices));
        if (nvz == 1 && p.noiseWidth <= 0.f && p.noiseDetune <= 0.f) {
          const double nz = 2.0 * rng.next() - 1.0, a = 1.0 - std::exp(-2.0 * kPiD * std::min(0.45 * sr, std::max(100.0, (double)p.noiseHpHz)) / sr);
          v.noiseHp += a * (nz - v.noiseHp); const double h1 = nz - v.noiseHp; v.noiseHp2 += a * (h1 - v.noiseHp2); const double h2 = h1 - v.noiseHp2;
          L += nzl * 0.5 * h2 * en; R += nzl * 0.5 * h2 * en;
        } else {   // several noise streams with spread high-pass points, and a decorrelated left and right
          const double w = std::min(1.0, (double)p.noiseWidth), nrm = nzl * 0.5 / std::sqrt((double)nvz); double sl = 0.0, srr = 0.0; double* z = v.nzS;
          for (int k = 0; k < nvz; ++k) {
            const double x = nvz > 1 ? 2.0 * k / (nvz - 1) - 1.0 : 0.0, hz = std::min(0.45 * sr, std::max(100.0, p.noiseHpHz * std::pow(2.0, x * p.noiseDetune * 2.0))), a = 1.0 - std::exp(-2.0 * kPiD * hz / sr);
            const double c = 2.0 * rng.next() - 1.0, nl = c * (1.0 - w) + w * (2.0 * rng.next() - 1.0), nr = c * (1.0 - w) + w * (2.0 * rng.next() - 1.0); const int o = k * 4;
            z[o] += a * (nl - z[o]); const double l1 = nl - z[o]; z[o + 1] += a * (l1 - z[o + 1]); sl += l1 - z[o + 1];
            z[o + 2] += a * (nr - z[o + 2]); const double r1 = nr - z[o + 2]; z[o + 3] += a * (r1 - z[o + 3]); srr += r1 - z[o + 3];
          }
          L += nrm * sl * en; R += nrm * srr * en;
        }
      }
      if (p.nzSmp != 0 && bank) { const std::vector<float>& d = bank->d[p.nzSmp]; if (!d.empty()) {
        if (ctl) v.sq[3] = std::pow(2.0, (p.nzSmpPitch + (smpKeyed(p.nzSmp) ? v.note - 60 + v.gl : 0.0)) / 12.0);
        const double pos = v.sp[3]; const int i0 = (int)pos;
        if (i0 < (int)d.size() - 1) { const double f = pos - i0, sm = (d[i0] * (1.0 - f) + d[i0 + 1] * f) * p.nzSmpVol * en; v.sp[3] = pos + v.sq[3]; smL += sm; smR += sm; } } }
      if (!post) { L += smL; R += smR; }
      auto distort = [&]() { if (p.dist <= 0.f) return; L = std::tanh(dk * L + 0.15 * p.dist) * dnm * 0.8; R = std::tanh(dk * R + 0.15 * p.dist) * dnm * 0.8; };
      if (p.subPreFilter) addSub(v, L, R, invSr, subL * en);
      if (p.distPre) distort();
      if (ctl) {
        const double cut = p.cutoff * std::pow(2.0, p.envAmt * fE * 5.0 + p.keyTrack * (double)(v.note - 60) / 12.0 + p.lfoCutoff * lfo * 2.0 + p.wheelCutoff * p.modWheel * 5.0 + p.velCutoff * v.vel * 5.0 + md[1] * 5.0); v.cc[0] = std::min(18000.0, std::max(20.0, cut));
        v.cc[1] = std::min(18000.0, std::max(20.0, p.f2Cut * std::pow(2.0, p.f2Env * fE2 * 5.0 + p.f2Key * (double)(v.note - 60) / 12.0 + md[14] * 5.0)));
        v.cc[2] = std::min(18000.0, std::max(20.0, p.f3Cut * std::pow(2.0, p.f3Env * fE3 * 5.0 + p.f3Key * (double)(v.note - 60) / 12.0 + md[15] * 5.0)));
        v.laC = 1.0 - std::exp(-2.0 * kPiD * std::max(5.0, std::min(0.45 * sr, p.lowCutHz * std::pow(2.0, md[13] * 4.0))) / sr); v.hcC = 1.0 - std::exp(-2.0 * kPiD * std::max(20.0, std::min(0.45 * sr, (double)p.highCutHz)) / sr);
        { const int ns0 = p.auxSlope; if (ns0 >= 4) { const bool hp0 = modes[0] == FilterMode::HP || modes[0] == FilterMode::HP24; const double f0 = hp0 ? std::max(20.0, std::min(0.45 * sr, (double)p.highCutHz)) : std::max(5.0, std::min(0.45 * sr, p.lowCutHz * std::pow(2.0, md[13] * 4.0))); const int sec0 = ns0 == 4 ? 4 : 8; if (f0 != v.bwF || sec0 != v.bwN || hp0 != v.bwH) { v.bwF = f0; v.bwN = sec0; v.bwH = hp0; bwDesign(v.bw, sec0, f0, sr, !hp0); } } }
      }
      const double rs0 = std::min(1.0, std::max(0.0, p.reso + md[2] * 0.5)), dv = std::min(1.0, std::max(0.0, p.drive + md[3] * 0.75));
      const double rrr[3] = {rs0, rr[1], rr[2]}, ddd[3] = {dv, dd[1], dd[2]};
      if (!parallel) { for (int q = 0; q < 3; ++q) { if (modes[q] == FilterMode::Off) continue; L = v.fl[q].process(L, modes[q], v.cc[q], rrr[q], ddd[q], sr); R = v.fr[q].process(R, modes[q], v.cc[q], rrr[q], ddd[q], sr); } }
      else { double al = 0.0, ar = 0.0; int cn = 0; for (int q = 0; q < 3; ++q) { if (modes[q] == FilterMode::Off) continue; al += v.fl[q].process(L, modes[q], v.cc[q], rrr[q], ddd[q], sr); ar += v.fr[q].process(R, modes[q], v.cc[q], rrr[q], ddd[q], sr); ++cn; } if (cn) { const double k = 1.0 / std::sqrt((double)cn); L = al * k; R = ar * k; } }
      if (post) { L += smL; R += smR; }
      { // high pass (and, next to a highpass filter, a low pass) on the whole voice, then distortion
        const double la = v.laC; const bool hpT = modes[0] == FilterMode::HP || modes[0] == FilterMode::HP24; const int ns = std::max(0, std::min(5, p.auxSlope));
        if (ns >= 4) {
          const int sec = ns == 4 ? 4 : 8;
          if (!hpT) { L = bwRun(v.bw, v.bwz, sec, L); R = bwRun(v.bw, v.bwz + 16, sec, R); }
          else { v.hpL += la * (L - v.hpL); v.hpR += la * (R - v.hpR); L -= v.hpL; R -= v.hpR; if (p.highCutHz < 19990.f) { L = bwRun(v.bw, v.bwz, sec, L); R = bwRun(v.bw, v.bwz + 16, sec, R); } }
        } else {
        v.hpL += la * (L - v.hpL); v.hpR += la * (R - v.hpR); L -= v.hpL; R -= v.hpR;
        if (!hpT) { if (ns > 0) { double* ax = v.ax; for (int s2 = 0; s2 < ns; ++s2) { ax[s2] += la * (L - ax[s2]); L -= ax[s2]; ax[4 + s2] += la * (R - ax[4 + s2]); R -= ax[4 + s2]; } } }
        else if (p.highCutHz < 19990.f) { double* ax = v.ax; const double hc = v.hcC; for (int s2 = 0; s2 <= ns; ++s2) { ax[s2] += hc * (L - ax[s2]); L = ax[s2]; ax[4 + s2] += hc * (R - ax[4 + s2]); R = ax[4 + s2]; } }
        }
        if (!p.distPre) distort();
      }
      double subS = 0.0, subSR = 0.0; if (!p.subPreFilter && subL > 0.0) { double t2l = 0.0, t2r = 0.0; addSub(v, t2l, t2r, invSr, subL * en); subS = t2l; subSR = t2r; }
      const double g = (link ? ampE : 1.0) * (0.4 + 0.6 * v.vel) * 1.3 * v.stackGain * (1.0 - p.lfoAmp * 0.5 * (1.0 - lfo)) * (hasMx ? std::max(0.0, 1.0 + md[7]) : 1.0);
      oL[i] += (float)(L * g); oR[i] += (float)(R * g); if (subS != 0.0 || subSR != 0.0) { sbL[i] += subS * g; sbR[i] += subSR * g; }
    }
  }
};


// Drum kit mode: every pad is its own small Engine, so each drum keeps its own sound and effects. Keys map to pads,
// a pad can cut another pad short (closed hat cuts open hat) and a key can play several pads at once (layers).
// Mirrors the JS engine in the playground (setKit / renderKit).
class KitEngine {
 public:
  struct Def { std::vector<int> n, ch; int play = -1; float g = 1.f; Params p; };   // play < 0: the pad plays at the key itself
  void build(const std::vector<Def>& defs, double sampleRate, int maxBlock) {
    sr = sampleRate; slots.clear(); block = std::max(64, maxBlock);
    for (const auto& d : defs) {
      if (d.n.empty()) continue;
      Slot s; s.n = d.n; s.ch = d.ch; s.play = d.play; s.g = d.g; s.e.reset(new Engine()); s.e->p = d.p; s.e->prepare(sr); s.e->reserve(block);
      slots.push_back(std::move(s));
    }
    kL.assign((size_t)block, 0.f); kR.assign((size_t)block, 0.f); aL.assign((size_t)block, 0.0); aR.assign((size_t)block, 0.0);
  }
  bool empty() const { return slots.empty(); }
  int pads() const { return (int)slots.size(); }
  void noteOn(int note, float vel) {
    for (auto& s : slots) {
      if (std::find(s.n.begin(), s.n.end(), note) == s.n.end()) continue;
      for (auto& t : slots) if (&t != &s && !s.ch.empty() && std::find(s.ch.begin(), s.ch.end(), t.n[0]) != s.ch.end()) t.e->allOff();
      s.e->noteOn(s.play < 0 ? note : s.play, vel); s.hot = (long long)(4.0 * sr);
    }
  }
  void noteOff(int) {}   // drums are one shots
  void allOff() { for (auto& s : slots) { s.e->allOff(); s.hot = 0; } }
  int activePads() const { return act; }
  // master and knee come from the main patch, like the JS engine
  void render(float* outL, float* outR, int n, float master, float knee) {
    act = 0; bool bad = false;
    for (int pos = 0; pos < n; pos += block) {
      const int m = std::min(block, n - pos);
      std::fill(aL.begin(), aL.begin() + m, 0.0); std::fill(aR.begin(), aR.begin() + m, 0.0);
      for (auto& s : slots) {
        if (s.hot <= 0) continue;
        s.e->render(kL.data(), kR.data(), m); s.hot -= m; if (s.e->activeVoices() > 0) ++act;
        for (int i = 0; i < m; ++i) { aL[(size_t)i] += (double)kL[(size_t)i] * s.g; aR[(size_t)i] += (double)kR[(size_t)i] * s.g; }
      }
      for (int i = 0; i < m; ++i) {
        const float l = (float)softClip(aL[(size_t)i] * master, knee), r = (float)softClip(aR[(size_t)i] * master, knee);
        outL[pos + i] = l; outR[pos + i] = r; if (!std::isfinite(l + r)) bad = true;
      }
    }
    if (bad) { allOff(); for (int i = 0; i < n; ++i) outL[i] = outR[i] = 0.f; }
  }
 private:
  struct Slot { std::vector<int> n, ch; int play = -1; float g = 1.f; std::unique_ptr<Engine> e; long long hot = 0; };
  std::vector<Slot> slots; std::vector<float> kL, kR; std::vector<double> aL, aR; double sr = 48000.0; int block = 8192; int act = 0;
};

}  // namespace supurr
