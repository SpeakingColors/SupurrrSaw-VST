// Tests multisample playback: the nearest zone is chosen, it plays in tune (including the recording's own tune offset),
// a looped zone sustains and a one-shot zone ends. Uses synthetic sine zones, no recordings needed.
// g++ -O2 -std=c++17 -o sampleset_test tools/sampleset_test.cpp && ./sampleset_test
#include "../core/supurrsaw_core.h"
#include <cstdio>
using namespace supurr;

static std::shared_ptr<Sample> sine(int root, double tuneCents, double seconds, bool loop) {
  auto s = std::make_shared<Sample>(); s->sampleRate = 44100.0; s->rootNote = root; s->tune = tuneCents; s->loop = loop;
  const double f = 440.0 * std::pow(2.0, (root - 69 + tuneCents / 100.0) / 12.0);   // the recording is tuneCents sharp of its root
  const size_t n = (size_t)(seconds * 44100.0); s->l.resize(n);
  // loop length is a whole number of cycles so the loop is seamless
  for (size_t i = 0; i < n; ++i) s->l[i] = 0.5f * (float)std::sin(2.0 * 3.14159265358979 * f * (double)i / 44100.0);
  if (loop) { const double cyc = 44100.0 / f; s->loopStart = (size_t)(cyc * std::floor(0.05 * 44100.0 / cyc)); s->loopEnd = s->loopStart + (size_t)std::llround(cyc * std::floor(0.2 * 44100.0 / cyc)); }
  return s;
}
static double freqOf(const std::vector<float>& x, double sr) {
  double first = -1, last = -1; int n = 0;
  for (size_t i = 1; i < x.size(); ++i) if (x[i - 1] < 0.f && x[i] >= 0.f) { const double t = ((double)i - 1.0 + (-x[i - 1]) / (x[i] - x[i - 1])) / sr; if (first < 0) first = t; last = t; ++n; }
  return n > 2 ? (n - 1) / (last - first) : 0.0;
}
static std::vector<float> run(Engine& e, int note, double seconds, double skip) {
  e.noteOn(note, 1.0f); std::vector<float> out; float l[64], r[64]; const int blocks = (int)(seconds * 48000 / 64);
  for (int b = 0; b < blocks; ++b) { e.render(l, r, 64); if (b * 64 >= (int)(skip * 48000)) out.insert(out.end(), l, l + 64); }
  e.allOff(); return out;
}
static double rms(const std::vector<float>& x, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b && i < x.size(); ++i) s += x[i] * x[i]; return std::sqrt(s / (double)(b - a)); }

int main() {
  int fails = 0; auto check = [&](bool ok, const char* what, double got, double want) { printf("%-44s got %.3f want %.3f %s\n", what, got, want, ok ? "ok" : "FAIL"); if (!ok) ++fails; };
  auto set = std::make_shared<SampleSet>(); set->name = "test";
  set->zones.push_back({sine(48, 0, 3.0, true), 0, 53});
  set->zones.push_back({sine(60, 20, 3.0, true), 54, 65});      // recorded 20 cents sharp
  set->zones.push_back({sine(72, 0, 3.0, false), 66, 127});
  auto mk = [&]() { auto e = std::make_unique<Engine>(); Params& p = e->p; p.layer[0].wave = Wave::Sample; p.layer[0].sampleSet = set; p.layer[0].level = 1.f;
    p.cutoff = 20000; p.reso = 0.f; p.lowCutHz = 20; p.amp = {0.001f, 0.2f, 1.f, 0.2f}; p.spread = 0.f; p.subLevel = 0.f; e->prepare(48000); return e; };
  for (int note : {40, 48, 50, 60, 64, 65, 72, 84, 100}) {
    auto e = mk(); auto x = run(*e, note, 0.5, 0.1); const double want = 440.0 * std::pow(2.0, (note - 69) / 12.0), got = freqOf(x, 48000.0);
    char what[64]; snprintf(what, sizeof what, "pitch of note %d (Hz)", note);
    check(got > 0.0 && std::fabs(1200.0 * std::log2(got / want)) < 4.0, what, got, want);
  }
  { auto e = mk(); auto x = run(*e, 60, 4.0, 0.0); const double early = rms(x, 48000 / 2, 48000), late = rms(x, 3 * 48000, 4 * 48000 - 1);
    check(late > 0.7 * early && late > 0.05, "looped zone still sounds after 3 s (rms)", late, early); }
  { set = std::make_shared<SampleSet>(); set->zones.push_back({sine(72, 0, 0.3, false), 0, 127});   // a short one-shot
    auto e = mk(); auto x = run(*e, 72, 2.0, 0.0); const double late = rms(x, 48000, 2 * 48000 - 1);
    check(late < 0.001, "one-shot zone has ended after 1 s (rms)", late, 0.0); }
  { auto e = mk(); auto x = run(*e, 60, 1.0, 0.0); bool bad = false; for (float v : x) if (!std::isfinite(v) || std::fabs(v) > 2.f) bad = true; check(!bad, "output finite and not clipping hard", bad ? 1 : 0, 0); }
  printf("%s\n", fails ? "SOME TESTS FAILED" : "all multisample tests passed");
  return fails ? 1 : 0;
}
