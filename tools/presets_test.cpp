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
              {"trance_lead", presets::tranceLead, 69, 1.5f, 3.0f}, {"talking_lead", presets::talkingLead, 64, 1.5f, 3.0f},
              {"daft_funk", presets::daftFunk, 64, 3.0f, 4.5f}, {"hoover_classic", presets::hooverClassic, 57, 1.5f, 3.5f}, {"hoover_prodigy", presets::hooverProdigy, 57, 1.5f, 2.5f},
              {"better_off_alone", presets::betterOffAlone, 69, 1.5f, 3.0f}, {"gimme_lead", presets::gimmeLead, 69, 1.0f, 2.5f}, {"levels_lead", presets::levelsLead, 69, 1.0f, 2.5f}, {"sandstorm_lead", presets::sandstormLead, 69, 2.0f, 3.5f},
              {"gigi_lead", presets::gigiLead, 69, 1.0f, 2.5f}, {"alexf_lead", presets::alexFLead, 69, 1.0f, 2.5f}, {"mgmt_lead", presets::mgmtLead, 69, 1.0f, 2.5f}, {"pendulum_lead", presets::pendulumLead, 69, 1.0f, 3.0f},
              {"nightcall_keys", presets::nightCallKeys, 60, 1.0f, 4.5f}, {"freeway_bass", presets::freewayBass, 40, 0.5f, 1.5f},
              {"brejcha_bass", presets::brejchaBass, 36, 1.0f, 2.0f}, {"feellove_bass", presets::feelLoveBass, 40, 0.8f, 1.5f}, {"nboy_303", presets::nboy303, 36, 0.8f, 1.5f}, {"spaghetti_bass", presets::spaghettiBass, 40, 0.5f, 1.2f}, {"access_bass", presets::accessBass, 40, 1.0f, 2.0f}};
  for (auto& pr : list) {
    Engine e; pr.fn(e.p); Result r = render(e, pr.note, pr.hold, pr.total);
    double sl = 0, sr = 0, sm = 0; for (size_t i = 0; i < r.l.size(); ++i) { sl += r.l[i]*r.l[i]; sr += r.r[i]*r.r[i]; sm += 0.25*(r.l[i]+r.r[i])*(r.l[i]+r.r[i]); }
    double monoDb = 10 * std::log10((sm + 1e-12) / (0.5 * (sl + sr) + 1e-12));
    std::printf("%-13s peak %.2f rms %.3f mono-sum %+.1f dB\n", pr.name, r.peak, r.rms, monoDb);
    CHECK(!r.nan); CHECK(r.peak < 1.0 && r.peak > 0.05); CHECK(r.rms > 0.01); CHECK(monoDb > -6.0);
    writeWav(dir + "/preset_" + pr.name + ".wav", r.l, r.r);
  }
  { // reverb mix is a true crossfade: at 0.1 the wet level is far below the dry level; the reverb low cut removes low end from the wet only
    double lowWet[2];
    for (int k = 0; k < 2; ++k) { Engine e; e.p.layer[0].wave = Wave::Saw; e.p.cutoff = 18000; e.p.reverbMix = 1.f; e.p.reverbLowCutHz = k ? 400.f : 20.f; Result r = render(e, 33, 0.3f, 1.5f);
      float lp = 0; double en = 0; for (size_t i = 24000; i < r.l.size(); ++i) { lp += 0.02f * (r.l[i] - lp); en += lp * lp; } lowWet[k] = en; }
    CHECK(lowWet[1] < lowWet[0] * 0.7);
    Engine a, b; a.p.layer[0].wave = b.p.layer[0].wave = Wave::Saw; a.p.reverbMix = 0.1f; b.p.reverbMix = 0.5f; Result ra = render(a, 57, 0.2f, 1.0f), rb = render(b, 57, 0.2f, 1.0f);
    double ea = 0, eb = 0, da = 0; for (size_t i = 30000; i < ra.l.size(); ++i) { ea += ra.l[i] * ra.l[i]; eb += rb.l[i] * rb.l[i]; } (void)da; CHECK(eb > ea * 2.0);   // tail after the note: more wet = more reverb tail
  }
  { // pitch envelope with an attack: starts at the played pitch, rises, then falls back (hoover recipe)
    Engine e; e.p.layer[0].wave = Wave::Saw; e.p.cutoff = 18000; e.p.pitchAmt = 12.f; e.p.pitchAttack = 0.2f; e.p.pitchTime = 0.3f; e.p.mono = true;
    Result r = render(e, 45, 2.0f, 2.0f);
    auto zc = [&](size_t a, size_t b) { int z = 0; for (size_t i = a + 1; i < b; ++i) if ((r.l[i-1] < 0) != (r.l[i] < 0)) ++z; return z; };
    int start = zc(100, 2100), peak = zc(8800, 10800), late = zc(80000, 82000);
    CHECK(peak > start * 1.4); CHECK(peak > late * 1.4);
  }
  { // unison cents: 4 voices at 36 cents beat harder than 4 voices at 3 cents (more amplitude movement over a short window)
    double var[2];
    for (int k = 0; k < 2; ++k) { Engine e; e.p.layer[0].wave = Wave::SuperSaw; e.p.layer[0].voices = 4; e.p.layer[0].cents = k ? 36.f : 3.f; e.p.cutoff = 600; e.p.spread = 0;
      Result r = render(e, 40, 1.5f, 1.6f); double m = 0, v = 0; int n = 0; std::vector<double> env; for (size_t i = 12000; i + 480 < r.l.size(); i += 480) { double a = 0; for (int j = 0; j < 480; ++j) a += std::fabs(r.l[i+j]); env.push_back(a); }
      for (double x : env) { m += x; ++n; } m /= n; for (double x : env) v += (x - m) * (x - m); var[k] = v / n / (m * m + 1e-12); }
    CHECK(var[1] > var[0]);
  }
  { // mono mode: two overlapping notes never produce more than one voice of level (no pile-up of long releases)
    Engine e; e.p.layer[0].wave = Wave::Saw; e.p.cutoff = 18000; e.p.mono = true; e.p.amp = {0.001f, 0.1f, 1.f, 3.f}; e.prepare(48000);
    float lb[64], rb[64]; double e1 = 0, e2 = 0;
    e.noteOn(48, 0.9f); for (int k = 0; k < 200; ++k) e.render(lb, rb, 64);
    e.noteOff(48); e.noteOn(55, 0.9f); for (int k = 0; k < 400; ++k) { e.render(lb, rb, 64); for (int i = 0; i < 64; ++i) e1 += lb[i] * lb[i]; }
    Engine f; f.p.layer[0].wave = Wave::Saw; f.p.cutoff = 18000; f.p.amp = {0.001f, 0.1f, 1.f, 3.f}; f.prepare(48000);
    f.noteOn(48, 0.9f); for (int k = 0; k < 200; ++k) f.render(lb, rb, 64);
    f.noteOff(48); f.noteOn(55, 0.9f); for (int k = 0; k < 400; ++k) { f.render(lb, rb, 64); for (int i = 0; i < 64; ++i) e2 += lb[i] * lb[i]; }
    CHECK(e1 < e2 * 0.8);
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
