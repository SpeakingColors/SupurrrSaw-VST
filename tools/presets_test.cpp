// Renders every built-in preset to a WAV and checks: no NaN, bounded peak, not silent, mono compatibility, sync/noise behave.
// Usage: presets_test [outdir]   (build: g++ -O2 -std=c++17 -o presets_test tools/presets_test.cpp)
#include "../presets/presets.h"
#include <cstdio>
#include <string>
using namespace supurr;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)
static void writeWav(const std::string& fn, const std::vector<float>& l, const std::vector<float>& r) {
  FILE* f = fopen(fn.c_str(), "wb"); if (!f) return; int n = (int)l.size(), sr = 48000, b = n * 4;
  fwrite("RIFF", 1, 4, f); int c = 36 + b; fwrite(&c, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); int s16 = 16; short pcm = 1, ch = 2, bps = 16, ba = 4; int br = sr * 4;
  fwrite(&s16, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bps, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&b, 4, 1, f);
  for (int i = 0; i < n; ++i) { short a = (short)(std::max(-1.f, std::min(1.f, l[i])) * 30000), d = (short)(std::max(-1.f, std::min(1.f, r[i])) * 30000); fwrite(&a, 2, 1, f); fwrite(&d, 2, 1, f); }
  fclose(f);
}
struct Result { std::vector<float> l, r; double peak = 0, rms = 0; bool nan = false; };
static Result render(Engine& e, int note, float hold, float total) {
  Result res; e.prepare(48000); e.noteOn(note, 0.9f); float lb[64], rb[64]; size_t N = (size_t)(total * 48000); double s2 = 0;
  for (size_t pos = 0; pos < N; pos += 64) {
    if (pos >= (size_t)(hold * 48000) && pos < (size_t)(hold * 48000) + 64) e.noteOff(note);
    e.render(lb, rb, 64);
    for (int i = 0; i < 64; ++i) { if (lb[i] != lb[i] || rb[i] != rb[i]) res.nan = true; res.peak = std::max(res.peak, (double)std::max(std::fabs(lb[i]), std::fabs(rb[i]))); s2 += lb[i] * lb[i]; res.l.push_back(lb[i]); res.r.push_back(rb[i]); }
  }
  res.rms = std::sqrt(s2 / (double)N); return res;
}
// share of energy above 6 kHz via a simple difference measure
static double brightness(const std::vector<float>& x) { double d = 0, t = 0; for (size_t i = 1; i < x.size(); ++i) { d += std::fabs(x[i] - x[i-1]); t += std::fabs(x[i]) ; } return d / (t + 1e-12); }
int main(int argc, char** argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  struct P { const char* name; void (*fn)(Params&); int note; float hold, total; };
  P list[] = {{"hoover", presets::hoover, 57, 1.2f, 2.5f}, {"pick", presets::pick, 69, 0.15f, 1.0f}, {"pluck", presets::pluck, 69, 0.2f, 1.2f},
              {"trance_lead", presets::tranceLead, 69, 1.5f, 3.0f}, {"talking_lead", presets::talkingLead, 64, 1.5f, 3.0f}};
  for (auto& pr : list) {
    Engine e; pr.fn(e.p); Result r = render(e, pr.note, pr.hold, pr.total);
    double sl = 0, sr = 0, sm = 0; for (size_t i = 0; i < r.l.size(); ++i) { sl += r.l[i]*r.l[i]; sr += r.r[i]*r.r[i]; sm += 0.25*(r.l[i]+r.r[i])*(r.l[i]+r.r[i]); }
    double monoDb = 10 * std::log10((sm + 1e-12) / (0.5 * (sl + sr) + 1e-12));
    std::printf("%-13s peak %.2f rms %.3f mono-sum %+.1f dB\n", pr.name, r.peak, r.rms, monoDb);
    CHECK(!r.nan); CHECK(r.peak < 1.0 && r.peak > 0.05); CHECK(r.rms > 0.01); CHECK(monoDb > -6.0);
    writeWav(dir + "/preset_" + pr.name + ".wav", r.l, r.r);
  }
  { // pick: pitch rises into the note like a bubble (negative pitch envelope)
    Engine e; presets::pick(e.p); e.p.chorusMix = e.p.delayMix = e.p.reverbMix = 0; e.p.dist = 0; e.p.filterMode = FilterMode::LP24; e.p.cutoff = 18000;
    Result r = render(e, 57, 0.4f, 0.5f);
    auto zc = [&](size_t a, size_t b) { int z = 0; for (size_t i = a + 1; i < b; ++i) if ((r.l[i-1] < 0) != (r.l[i] < 0)) ++z; return z; };
    CHECK(zc(9000, 10200) > zc(200, 1400) * 1.15);
  }
  { // sync: slave pitch sets the timbre. Same master pitch, higher sync interval gives a brighter sound
    double b[2];
    for (int k = 0; k < 2; ++k) { Engine e; e.p = Params(); e.p.layer[0].wave = Wave::Saw; e.p.layer[0].sync = true; e.p.layer[0].syncSemis = k ? 19.f : 3.f; e.p.cutoff = 18000; e.p.master = 0.5f;
      Result r = render(e, 52, 0.8f, 1.0f); CHECK(!r.nan); b[k] = brightness(r.l); }
    CHECK(b[1] > b[0] * 1.2);
    // sync off must equal the plain saw (nothing changes unless the switch is on)
    Engine a, c; a.p.layer[0].wave = c.p.layer[0].wave = Wave::Saw; c.p.layer[0].syncSemis = 12.f;
    Result ra = render(a, 52, 0.5f, 0.6f), rc = render(c, 52, 0.5f, 0.6f); double d = 0; for (size_t i = 0; i < ra.l.size(); ++i) d += std::fabs(ra.l[i] - rc.l[i]); CHECK(d < 1e-3);
  }
  { // sync follows the note: with syncKey the slave interval grows with pitch, so high notes get a bigger timbre change
    double r[2];
    for (int k = 0; k < 2; ++k) { double b[2];
      for (int n = 0; n < 2; ++n) { Engine e; e.p.layer[0].wave = Wave::Saw; e.p.layer[0].sync = true; e.p.layer[0].syncSemis = 6.f; e.p.layer[0].syncKey = k ? 8.f : 0.f; e.p.cutoff = 18000;
        Result x = render(e, n ? 72 : 48, 0.8f, 1.0f); b[n] = brightness(x.l); }
      r[k] = b[1] / b[0]; }
    CHECK(r[1] > r[0] * 1.15);
    // mod wheel opens the sync
    double bw[2]; for (int k = 0; k < 2; ++k) { Engine e; e.p.layer[0].wave = Wave::Saw; e.p.layer[0].sync = true; e.p.layer[0].syncSemis = 3.f; e.p.layer[0].syncWheel = 16.f; e.p.modWheel = k; e.p.cutoff = 18000; Result x = render(e, 52, 0.8f, 1.0f); bw[k] = brightness(x.l); }
    CHECK(bw[1] > bw[0] * 1.2);
  }
  { // noise: adds high-frequency content, nothing below its high-pass (low end stays clean)
    double b[2], lo[2];
    for (int k = 0; k < 2; ++k) { Engine e; e.p.layer[0].wave = Wave::Sine; e.p.cutoff = 18000; e.p.noiseLevel = k ? 0.3f : 0.f; e.p.noiseHpHz = 5000.f;
      Result r = render(e, 40, 0.8f, 1.0f); b[k] = brightness(r.l);
      double s = 0, m = 0; float lp = 0; for (float x : r.l) { lp += 0.01f * (x - lp); s += lp * lp; m += x * x; } lo[k] = s; (void)m; }
    CHECK(b[1] > b[0] * 1.5);
    CHECK(lo[1] < lo[0] * 1.2);
  }
  { // distortion order: pre vs post the filter must sound different
    double d = 0; Result a, b;
    for (int k = 0; k < 2; ++k) { Engine e; e.p.layer[0].wave = Wave::Saw; e.p.cutoff = 900; e.p.reso = 0.5f; e.p.dist = 0.8f; e.p.distPre = k == 1; Result r = render(e, 57, 0.4f, 0.5f); (k ? b : a) = r; }
    for (size_t i = 0; i < a.l.size(); ++i) d += std::fabs(a.l[i] - b.l[i]); CHECK(d > 1.0);
  }
  if (!fails) std::puts("presets_test: all passed");
  return fails ? 1 : 0;
}
