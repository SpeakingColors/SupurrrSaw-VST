// SupurrSaw DSP core. No JUCE dependency, header only, so it can be tested on its own.
// Licence: AGPL-3.0-or-later. Author: SpeakingColors
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace supurr {

constexpr int kMaxVoices = 8;   // polyphony
constexpr int kMaxSaws   = 15;  // unison voices per layer
constexpr int kLayers    = 3;
constexpr float kPi      = 3.14159265358979f;

enum class Wave { Off, SuperSaw, SuperSquare, Saw, Pulse, Tri, Sine, Sample };
enum class FilterMode { LP24, LP18, HP, BP };

// A loaded sample (WAV data already decoded). Shared between layers and voices, read only while playing.
struct Sample {
  std::vector<float> l, r;
  double sampleRate = 44100.0;
  int rootNote = 60;
  bool loop = false;
  bool oneShot = false;  // plays to the end, key release does not cut it (stabs)
  size_t loopStart = 0, loopEnd = 0;
};

struct Adsr { float a = 0.002f, d = 0.3f, s = 0.8f, r = 0.3f; };  // seconds, sustain 0..1

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
  std::shared_ptr<const Sample> sample;
};

struct Params {
  LayerParams layer[kLayers];
  // sub oscillator (always mono and centred)
  float subLevel = 0.f;
  Wave subWave = Wave::Sine;   // Sine, Tri, Pulse, Saw
  int subOctave = -1;          // -1 or -2
  // filter
  FilterMode filterMode = FilterMode::LP24;
  float cutoff = 4000.f, reso = 0.2f, drive = 0.f, envAmt = 0.f, keyTrack = 0.f;
  float lowCutHz = 20.f;       // removes low rumble before the sub is added
  float dist = 0.f;            // 0..1 distortion (acid, hoover, dirty reese, screech)
  bool distPre = false;        // true: distortion before the filter (the filter shapes the screech), false: after it
  float noiseLevel = 0.f;      // 0..1 white noise added before the filter
  float noiseHpHz = 3000.f;    // high-pass on the noise so it never muddies the low end
  Adsr amp, fenv;
  // pitch envelope (hoover dive): starts at +pitchAmt semitones, decays to 0
  float pitchAmt = 0.f, pitchTime = 0.09f;
  // pitch fall (slow hoover bend down): drops by pitchFall semitones, reaching about 63% of it after pitchFallTime
  float pitchFall = 0.f, pitchFallTime = 1.f;
  // LFO
  float lfoRate = 6.f, lfoPwm = 0.f, lfoPitch = 0.f, lfoCutoff = 0.f;
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
  float reverbMix = 0.f, reverbSize = 0.6f, reverbWidth = 0.8f;
  float master = 0.8f;
};

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
  float v = 0.f; int stage = 0;  // 0 idle 1 attack 2 decay/sustain 3 release
  void on() { stage = 1; }
  void off() { if (stage) stage = 3; }
  float tick(const Adsr& e, float sr) {
    switch (stage) {
      case 1: v += 1.f / std::max(1.f, e.a * sr); if (v >= 1.f) { v = 1.f; stage = 2; } break;
      case 2: v += (e.s - v) * (1.f - std::exp(-1.f / std::max(1.f, e.d * sr * 0.3f))); break;
      case 3: v += (0.f - v) * (1.f - std::exp(-1.f / std::max(1.f, e.r * sr * 0.3f))); if (v < 1e-4f) { v = 0.f; stage = 0; } break;
      default: break;
    }
    return v;
  }
};

// 4-pole ladder (zero delay feedback) with LP24 and LP18 taps, plus a 12 dB state variable filter for HP and BP.
struct Filter {
  float s[4] = {0, 0, 0, 0}, z1 = 0, z2 = 0;
  void reset() { s[0] = s[1] = s[2] = s[3] = z1 = z2 = 0; }
  float process(float x, FilterMode m, float cutoffHz, float reso, float drive, float sr) {
    float fc = std::min(cutoffHz, 0.45f * sr);
    float g = std::tan(kPi * fc / sr);
    if (m == FilterMode::LP24 || m == FilterMode::LP18) {
      float G = g / (1.f + g), G2 = G * G, G4 = G2 * G2;
      float k = 3.95f * reso;
      float S = G2 * G * s[0] / (1.f + g) + G2 * s[1] / (1.f + g) + G * s[2] / (1.f + g) + s[3] / (1.f + g);
      float in = std::tanh(x * (1.f + 3.f * drive));
      float y4 = (G4 * in + S) / (1.f + k * G4);
      float u = in - k * y4;
      float y[4]; float xi = u;
      for (int i = 0; i < 4; ++i) { float v = G * (xi - s[i]); y[i] = v + s[i]; s[i] = y[i] + v; xi = y[i]; }
      return (m == FilterMode::LP24) ? y[3] : y[2];
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
  float syncPhase[kLayers] = {}; float noiseHp = 0.f, noiseHp2 = 0.f;
  float subPhase = 0.f, drift[kMaxSaws] = {}, driftT[kMaxSaws] = {};
  float lfoPhase = 0.f, t = 0.f;
  EnvState amp, fenv; Filter fl, fr; float hpL = 0.f, hpR = 0.f; bool oneShot = false;
};


// ---- effects ----
struct DelayLine {
  std::vector<float> b; size_t w = 0;
  void init(size_t n) { b.assign(n + 4, 0.f); w = 0; }
  void push(float x) { b[w] = x; if (++w >= b.size()) w = 0; }
  float read(float d) const {  // d samples back, linear interpolation
    float rp = (float)w - d; while (rp < 0) rp += (float)b.size(); size_t i0 = (size_t)rp; float f = rp - (float)i0;
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
  void run(float in, float size, float& wl, float& wr) {
    float fb = 0.70f + 0.27f * size, o[2];
    for (int ch = 0; ch < 2; ++ch) { float s = 0; for (int j = 0; j < 8; ++j) s += c[ch][j].run(in * 0.15f, fb, 0.25f); for (int j = 0; j < 4; ++j) s = a[ch][j].run(s); o[ch] = s; }
    wl = o[0]; wr = o[1];
  }
};
struct Fx {
  float sr = 48000.f; DelayLine chor, dl, dr; Reverb rev; float lfo = 0.f, dlp[2] = {0, 0};
  void prepare(float s) { sr = s; chor.init((size_t)(0.05f * s)); dl.init((size_t)(2.f * s)); dr.init((size_t)(2.f * s)); rev.init(s); lfo = 0.f; dlp[0] = dlp[1] = 0.f; }
  void process(const Params& p, float& L, float& R) {
    float m = 0.5f * (L + R);
    if (p.chorusMix > 0.f) {  // side-only: the left and right wet signals are opposite, so they cancel in mono
      lfo += p.chorusRate / sr; if (lfo >= 1.f) lfo -= 1.f;
      chor.push(m); float d = (0.012f + 0.006f * p.chorusDepth * std::sin(2.f * kPi * lfo)) * sr;
      float d2 = (0.017f + 0.006f * p.chorusDepth * std::sin(2.f * kPi * (lfo + 0.5f))) * sr;
      float w = 0.5f * (chor.read(d) + chor.read(d2)) - m;
      L += p.chorusMix * w; R -= p.chorusMix * w;
    }
    if (p.delayMix > 0.f) {  // ping-pong, darker on every repeat
      float t = std::max(0.02f, std::min(1.9f, p.delayTime)) * sr;
      float a = dl.read(t), b = dr.read(t);
      dlp[0] += 0.35f * (a - dlp[0]); dlp[1] += 0.35f * (b - dlp[1]);
      dl.push(m + dlp[1] * p.delayFb); dr.push(dlp[0] * p.delayFb);
      L += p.delayMix * dlp[0]; R += p.delayMix * dlp[1];
    }
    if (p.reverbMix > 0.f) {  // mid and side are scaled separately: narrow centre, wide sides
      float wl, wr; rev.run(m, p.reverbSize, wl, wr);
      float wm = 0.5f * (wl + wr), ws = 0.5f * (wl - wr);
      float outM = wm * 0.8f, outS = ws * (0.4f + 1.2f * p.reverbWidth);
      L += p.reverbMix * (outM + outS) * 3.f; R += p.reverbMix * (outM - outS) * 3.f;
    }
  }
};

class Engine {
 public:
  Params p;
  void prepare(double sampleRate) { sr = (float)sampleRate; for (auto& v : voices) v = Voice(); bl = br = 0.f; bl2 = br2 = 0.f; fx.prepare(sr); }
  void noteOn(int note, float vel) {
    Voice* v = nullptr;
    for (auto& c : voices) if (!c.active) { v = &c; break; }
    if (!v) { v = &voices[0]; for (auto& c : voices) if (c.age < v->age) v = &c; }
    *v = Voice(); v->active = true; v->note = note; v->vel = vel; v->age = ++clock;
    for (int l = 0; l < kLayers; ++l) for (int j = 0; j < kMaxSaws; ++j) v->phase[l][j] = rng.next();
    v->amp.on(); v->fenv.on();
    for (int l = 0; l < kLayers; ++l) if (p.layer[l].wave == Wave::Sample && p.layer[l].sample && p.layer[l].sample->oneShot) v->oneShot = true;
  }
  void noteOff(int note) { for (auto& v : voices) if (v.active && v.note == note && v.amp.stage != 3 && !v.oneShot) { v.amp.off(); v.fenv.off(); } }
  void allOff() { for (auto& v : voices) v.active = false; }

  // adds nothing, overwrites outL/outR
  void render(float* outL, float* outR, int n) {
    for (int i = 0; i < n; ++i) outL[i] = outR[i] = 0.f;
    for (auto& v : voices) if (v.active) renderVoice(v, outL, outR, n);
    // bass mono: sum everything below bassMonoHz, keep the rest stereo
    float a = 1.f - std::exp(-2.f * kPi * std::max(10.f, p.bassMonoHz) / sr);
    for (int i = 0; i < n; ++i) {
      fx.process(p, outL[i], outR[i]);
      bl += a * (outL[i] - bl); bl2 += a * (bl - bl2);
      br += a * (outR[i] - br); br2 += a * (br - br2);
      float m = 0.5f * (bl2 + br2);
      float l = outL[i] - bl2 + m, r = outR[i] - br2 + m;
      outL[i] = std::tanh(l * p.master); outR[i] = std::tanh(r * p.master);
    }
  }

 private:
  float sr = 48000.f; Voice voices[kMaxVoices]; Rng rng; Fx fx; uint64_t clock = 0;
  float bl = 0, br = 0, bl2 = 0, br2 = 0;

  void renderVoice(Voice& v, float* oL, float* oR, int n) {
    const float invSr = 1.f / sr;
    for (int i = 0; i < n; ++i) {
      float ampE = v.amp.tick(p.amp, sr), fE = v.fenv.tick(p.fenv, sr);
      if (v.amp.stage == 0) { v.active = false; return; }
      v.lfoPhase += p.lfoRate * invSr; if (v.lfoPhase >= 1.f) v.lfoPhase -= 1.f;
      float lfo = std::sin(2.f * kPi * v.lfoPhase);
      float pitchEnv = p.pitchAmt * std::exp(-v.t / std::max(0.001f, p.pitchTime)) - p.pitchFall * (1.f - std::exp(-v.t / std::max(0.01f, p.pitchFallTime)));
      v.t += invSr;
      if ((int)(v.t * sr) % 512 == 0)
        for (int j = 0; j < kMaxSaws; ++j) v.driftT[j] = p.drift * 0.004f * (2.f * rng.next() - 1.f);
      for (int j = 0; j < kMaxSaws; ++j) v.drift[j] += 0.0005f * (v.driftT[j] - v.drift[j]);

      float L = 0.f, R = 0.f;
      for (int ly = 0; ly < kLayers; ++ly) {
        const LayerParams& lp = p.layer[ly];
        if (lp.wave == Wave::Off || lp.level <= 0.f) continue;
        float semis = (float)(v.note - 69) + (float)lp.octave * 12.f + lp.tune * 0.01f + pitchEnv + p.lfoPitch * lfo * 0.3f;
        float f = 440.f * std::exp2(semis / 12.f);
        float dt = std::min(0.45f, f * invSr);
        float lL = 0.f, lR = 0.f;
        if (lp.wave == Wave::SuperSaw || lp.wave == Wave::SuperSquare) {
          int nv = std::max(1, std::min(kMaxSaws, lp.voices));
          float det = jpDetune(std::min(1.f, std::max(0.f, lp.detune * (1.f + p.lfoDetune * lfo)))) * kMaxDetune * p.detuneRange;
          float sideFade = std::min(1.f, lp.detune * 20.f);  // detune 0 = one clean voice
          float pw = std::min(0.95f, std::max(0.05f, lp.pwm + p.lfoPwm * lfo * 0.45f));
          float norm = 1.f / (1.f + (float)(nv - 1) * 0.62f * sideFade);
          for (int j = 0; j < nv; ++j) {
            float sp = sawSpread(j, nv);
            bool centre = (nv % 2 == 1 && j == nv / 2);
            float g = centre ? 1.f : 0.62f * sideFade; g *= norm;
            if (g <= 0.f) continue;
            float d = std::min(0.45f, dt * (1.f + det * sp + v.drift[j]));
            float& ph = v.phase[ly][j];
            float s;
            if (lp.wave == Wave::SuperSaw) s = (2.f * ph - 1.f) - blep(ph, d);
            else {
              s = (ph < pw ? 1.f : -1.f) + blep(ph, d);
              float t2 = ph - pw; if (t2 < 0.f) t2 += 1.f;
              s -= blep(t2, d);
              s *= 0.8f;
            }
            ph += d; if (ph >= 1.f) ph -= 1.f;
            float pos = sp * p.spread;  // -1..1
            float ang = (pos + 1.f) * kPi * 0.25f;
            lL += g * s * std::cos(ang) * 1.414f; lR += g * s * std::sin(ang) * 1.414f;
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
          float pw = std::min(0.95f, std::max(0.05f, lp.pwm + p.lfoPwm * lfo * 0.45f));
          // with sync on, the slave oscillator makes the sound and the master only restarts it
          float& sph = v.syncPhase[ly];
          float odt = dt;
          if (lp.sync) {
            float sem = std::min(48.f, std::max(0.f, lp.syncSemis + lp.syncEnv * fE + lp.syncLfo * lfo));
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
        float pg = (lp.pan + 1.f) * kPi * 0.25f;
        L += lp.level * lL * std::cos(pg) * 1.414f; R += lp.level * lR * std::sin(pg) * 1.414f;
      }
      if (p.noiseLevel > 0.f) {  // white noise, high-passed (two poles) so only the top end is added
        float n = (2.f * rng.next() - 1.f);
        float a = 1.f - std::exp(-2.f * kPi * std::min(0.45f * sr, std::max(100.f, p.noiseHpHz)) / sr);
        v.noiseHp += a * (n - v.noiseHp); float h1 = n - v.noiseHp;
        v.noiseHp2 += a * (h1 - v.noiseHp2); float h2 = h1 - v.noiseHp2;
        L += p.noiseLevel * 0.5f * h2; R += p.noiseLevel * 0.5f * h2;
      }
      auto distort = [&](float& l, float& r) {
        if (p.dist <= 0.f) return;
        float k = 1.f + p.dist * 14.f, norm = 1.f / std::tanh(k * 0.6f);
        l = std::tanh(k * l + 0.15f * p.dist) * norm * 0.8f; r = std::tanh(k * r + 0.15f * p.dist) * norm * 0.8f; };
      if (p.distPre) distort(L, R);
      // filter
      float cut = p.cutoff * std::exp2(p.envAmt * fE * 5.f + p.keyTrack * (float)(v.note - 60) / 12.f + p.lfoCutoff * lfo * 2.f);
      cut = std::min(18000.f, std::max(20.f, cut));
      L = v.fl.process(L, p.filterMode, cut, p.reso, p.drive, sr);
      R = v.fr.process(R, p.filterMode, cut, p.reso, p.drive, sr);
      { // low cut, then distortion
        float a = 1.f - std::exp(-2.f * kPi * std::max(5.f, p.lowCutHz) / sr);
        v.hpL += a * (L - v.hpL); v.hpR += a * (R - v.hpR); L -= v.hpL; R -= v.hpR;
        if (!p.distPre) distort(L, R);
      }
      // sub (mono, centred, after the filter so the low end stays solid)
      if (p.subLevel > 0.f) {
        float sf = 440.f * std::exp2(((float)(v.note - 69) + (float)p.subOctave * 12.f) / 12.f);
        float sdt = sf * invSr; float& sp = v.subPhase; float s;
        switch (p.subWave) {
          case Wave::Tri: s = 4.f * std::fabs(sp - 0.5f) - 1.f; break;
          case Wave::Pulse: s = (sp < 0.5f ? 1.f : -1.f) + blep(sp, sdt) - blep(sp < 0.5f ? sp + 0.5f : sp - 0.5f, sdt); s *= 0.8f; break;
          case Wave::Saw: s = (2.f * sp - 1.f) - blep(sp, sdt); break;
          default: s = std::sin(2.f * kPi * sp); break;
        }
        sp += sdt; if (sp >= 1.f) sp -= 1.f;
        L += p.subLevel * s * 0.7f; R += p.subLevel * s * 0.7f;
      }
      float g = ampE * (0.4f + 0.6f * v.vel) * 1.3f;
      oL[i] += L * g; oR[i] += R * g;
    }
  }
};

}  // namespace supurr
