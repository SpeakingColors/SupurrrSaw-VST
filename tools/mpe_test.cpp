// Checks the pitch wheel, MPE per-note bend, pressure and slide against the engine.
// Build: g++ -O2 -std=c++17 -o mpe_test tools/mpe_test.cpp && ./mpe_test
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <memory>
#include <map>
#include <cstdint>
#include <cstring>
#include <atomic>
#define private public   // the test reads the voice list
#include "../presets/presets.h"
#undef private
using namespace supurr;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)
static double goertzel(const std::vector<float>& x, double f, double sr) {
  const double w = 2 * 3.14159265358979 * f / sr, c = 2 * std::cos(w); double s1 = 0, s2 = 0;
  for (float v : x) { double s = v + c * s1 - s2; s2 = s1; s1 = s; }
  return std::sqrt(s1 * s1 + s2 * s2 - c * s1 * s2) / (double)x.size();
}
static Engine sine() {
  Engine e; e.p.dmMix = 0.f; for (int i = 0; i < 3; ++i) e.p.layer[i].on = (i == 0);
  e.p.layer[0].wave = Wave::Sine; e.p.layer[0].level = 0.8f; e.p.layer[0].voices = 1; e.p.cutoff = 18000; e.p.amp = {0.001f, 0.1f, 1.f, 0.3f};
  e.prepare(48000); return e;
}
static std::vector<float> run(Engine& e, double sec) {
  std::vector<float> o; float l[64], r[64];
  for (int k = 0; k < (int)(sec * 48000 / 64); ++k) { e.render(l, r, 64); if (k > 300) for (int i = 0; i < 64; ++i) o.push_back(l[i]); }
  return o;
}
int main() {
  { // pitch wheel: +1 with a 2 semitone range moves A440 to 493.88 Hz, and a 12 semitone range to 880 Hz
    Engine e = sine(); e.p.pitchBend = 1.f; e.p.bendRange = 2.f; e.noteOn(69, 0.9f); auto o = run(e, 1.5);
    const double a = goertzel(o, 493.88, 48000), b = goertzel(o, 440.0, 48000); CHECK(a > 0.05); CHECK(b < a * 0.1);
    Engine f = sine(); f.p.pitchBend = 1.f; f.p.bendRange = 12.f; f.noteOn(69, 0.9f); o = run(f, 1.5);
    CHECK(goertzel(o, 880.0, 48000) > 0.05); CHECK(goertzel(o, 440.0, 48000) < 0.01);
    Engine g = sine(); g.p.pitchBend = -1.f; g.p.bendRange = 12.f; g.noteOn(69, 0.9f); o = run(g, 1.5);
    CHECK(goertzel(o, 220.0, 48000) > 0.05);
  }
  { // MPE: two notes on two channels, only the second one bends (+12 semitones)
    Engine e = sine(); e.noteOn(60, 0.9f, 1); e.noteOn(64, 0.9f, 2); e.setChanBend(2, 12.0); auto o = run(e, 1.5);
    CHECK(goertzel(o, 261.63, 48000) > 0.04);      // note 1 untouched
    CHECK(goertzel(o, 659.26, 48000) > 0.04);      // note 2 one octave up
    CHECK(goertzel(o, 329.63, 48000) < 0.01);      // where note 2 would be without the bend
  }
  { // a channel's bend set before the note starts is used from the first sample
    Engine e = sine(); e.setChanBend(3, 12.0); e.noteOn(57, 0.9f, 3); auto o = run(e, 1.5);
    CHECK(goertzel(o, 440.0, 48000) > 0.05); CHECK(goertzel(o, 220.0, 48000) < 0.01);
  }
  { // global wheel and a note's own bend add up
    Engine e = sine(); e.p.pitchBend = 1.f; e.p.bendRange = 2.f; e.noteOn(69, 0.9f, 2); e.setChanBend(2, 2.0); auto o = run(e, 1.5);
    CHECK(goertzel(o, 554.37, 48000) > 0.05);   // A + 4 semitones
  }
  { // note off by channel: the same key on two channels, only the released one stops
    Engine e = sine(); e.p.amp = {0.001f, 0.1f, 1.f, 0.01f}; e.noteOn(60, 0.9f, 1); e.noteOn(60, 0.9f, 2); float l[64], r[64];
    for (int k = 0; k < 50; ++k) e.render(l, r, 64);
    e.noteOff(60, 2); for (int k = 0; k < 400; ++k) e.render(l, r, 64);
    int alive = 0; for (auto& v : e.voices) if (v.active) { ++alive; CHECK(v.ch == 1); } CHECK(alive == 1);
  }
  { // pressure and slide reach the mod matrix: a low-passed saw gets brighter when pressure opens the cutoff
    auto bright = [&](float press, float slide, bool viaNote) {
      Engine e; e.p.dmMix = 0.f; for (int i = 1; i < 3; ++i) e.p.layer[i].on = false; e.p.layer[0].wave = Wave::Saw; e.p.layer[0].voices = 1; e.p.cutoff = 300; e.p.amp = {0.001f, 0.1f, 1.f, 0.3f};
      e.p.mx[15][1] = 1.f; e.p.mx[16][1] = 1.f; e.prepare(48000);
      if (viaNote) { e.noteOn(48, 0.9f, 2); e.setChanPress(2, press); e.setChanSlide(2, slide); } else { e.p.pressure = press; e.p.slide = slide; e.noteOn(48, 0.9f); }
      auto o = run(e, 1.2); double d = 0, t = 0; for (size_t i = 1; i < o.size(); ++i) { d += std::fabs(o[i] - o[i - 1]); t += std::fabs(o[i]); } return d / (t + 1e-12);
    };
    const double b0 = bright(0, 0, true), b1 = bright(1, 0, true), b2 = bright(0, 1, true), b3 = bright(1, 0, false), b4 = bright(0, 1, false);
    std::printf("brightness: none %.4f  note pressure %.4f  note slide %.4f  global pressure %.4f  global slide %.4f\n", b0, b1, b2, b3, b4);
    CHECK(b1 > b0 * 1.2); CHECK(b2 > b0 * 1.2); CHECK(b3 > b0 * 1.2); CHECK(b4 > b0 * 1.2);
  }
  { // bend source in the matrix: bend of one octave sent to cutoff opens the filter
    Engine e; e.p.dmMix = 0.f; for (int i = 1; i < 3; ++i) e.p.layer[i].on = false; e.p.layer[0].wave = Wave::Saw; e.p.layer[0].voices = 1; e.p.cutoff = 300; e.p.mx[17][1] = 1.f; e.prepare(48000);
    e.p.pitchBend = 1.f; e.p.bendRange = 12.f; e.noteOn(48, 0.9f); auto o = run(e, 1.2);
    CHECK(o.size() > 100); double pk = 0; for (float v : o) pk = std::max(pk, (double)std::fabs(v)); CHECK(pk > 0.01);
  }
  std::printf(fails ? "mpe_test: %d FAILED\n" : "mpe_test: all passed\n", fails); return fails ? 1 : 0;
}
