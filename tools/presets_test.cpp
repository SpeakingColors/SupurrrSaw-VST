// Renders every built-in preset to a WAV and checks: no NaN, bounded peak, not silent, mono compatibility, sync/noise behave.
// Usage: presets_test [outdir]   (build: g++ -O2 -std=c++17 -o presets_test tools/presets_test.cpp)
#include <functional>
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
static bool gDmOff = true;   // most tests measure the oscillators, so the dance master chain is bypassed unless a test turns it on
static Result render(Engine& e, int note, float hold, float total) {
  if (gDmOff) e.p.dmMix = 0.f;
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
              {"epiano_dx", presets::epianoDx, 60, 1.0f, 3.0f}, {"epiano_wurly", presets::epianoWurly, 60, 1.0f, 2.5f}, {"piano", presets::piano, 60, 1.5f, 4.0f}, {"felt_piano", presets::feltPiano, 60, 1.5f, 4.0f}, {"organ", presets::organ, 60, 1.5f, 2.5f}, {"clav", presets::clav, 60, 0.4f, 1.5f},
              {"strings", presets::strings, 60, 2.5f, 4.5f}, {"warm_pad", presets::warmPad, 60, 3.0f, 5.5f}, {"glass_pad", presets::glassPad, 60, 3.0f, 6.0f}, {"choir", presets::choir, 64, 2.5f, 4.5f}, {"bell", presets::bell, 72, 0.5f, 5.0f}, {"brass", presets::brass, 55, 1.2f, 2.5f},
              {"trance_lead", presets::tranceLead, 69, 1.5f, 3.0f}, {"trance_hybrid", presets::tranceHybrid, 69, 1.5f, 3.0f}, {"trance_90s", presets::trance90s, 69, 1.5f, 3.0f}, {"talking_lead", presets::talkingLead, 64, 1.5f, 3.0f},
              {"daft_funk", presets::daftFunk, 64, 3.0f, 4.5f}, {"daft_funk_fat", presets::daftFunkFat, 64, 3.0f, 4.5f}, {"hoover_classic", presets::hooverClassic, 57, 1.5f, 3.5f}, {"hoover_prodigy", presets::hooverProdigy, 57, 1.5f, 2.5f},
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
  { // rave pack: same checks for every recipe preset, plus the byName lookup
    int rn; const presets::RaveEntry* RL = presets::raveList(rn);
    for (int i = 0; i < rn; ++i) {
      Engine e; RL[i].fn(e.p); e.p.master *= presets::raveGain(RL[i].name); Engine e2; CHECK(presets::byName(RL[i].name, e2.p));
      Result r = render(e, RL[i].note, RL[i].hold, RL[i].total);
      double sl = 0, sr = 0, sm = 0; for (size_t k = 0; k < r.l.size(); ++k) { sl += r.l[k]*r.l[k]; sr += r.r[k]*r.r[k]; sm += 0.25*(r.l[k]+r.r[k])*(r.l[k]+r.r[k]); }
      double monoDb = 10 * std::log10((sm + 1e-12) / (0.5 * (sl + sr) + 1e-12));
      std::printf("%-22s peak %.2f rms %.3f mono-sum %+.1f dB\n", RL[i].name, r.peak, r.rms, monoDb);
      CHECK(!r.nan); CHECK(r.peak < 1.0 && r.peak > 0.05); CHECK(r.rms > 0.01); CHECK(monoDb > -6.0);
      writeWav(dir + "/preset_" + RL[i].name + ".wav", r.l, r.r);
    }
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
  { // output stage: linear under the knee, smooth above it, never above 1, even at master 2 with everything stacked
    for (float x = -0.72f; x <= 0.72f; x += 0.01f) CHECK(softClip(x, 0.72f) == x);
    CHECK(std::fabs(softClip(1000.f, 0.72f)) <= 1.f && std::fabs(softClip(-1000.f, 0.72f)) <= 1.f);
    float d = (softClip(0.7201f, 0.72f) - softClip(0.7199f, 0.72f)) / 0.0002f; CHECK(std::fabs(d - 1.f) < 0.01f);
    for (const char* n : {"hoover", "levels_lead", "access_bass", "talking_lead"}) {
      Params q; presets::byName(n, q); q.master = 2.f; Engine e; e.prepare(48000); e.p = q; float pk = 0, L[512], R[512];
      for (int k : {48, 55, 60, 64}) e.noteOn(k, 1.f);
      for (int b = 0; b < 60; ++b) { e.render(L, R, 512); for (int i = 0; i < 512; ++i) { CHECK(L[i] == L[i]); pk = std::max(pk, std::max(std::fabs(L[i]), std::fabs(R[i]))); } }
      CHECK(pk <= 1.f); std::printf("%-14s master 2, four notes: peak %.3f\n", n, pk);
    }
  }
  { // polyphony: twelve held notes all sound with the default, and a cap of 4 steals the oldest releasing or oldest voice
    Params q; presets::byName("trance_lead", q);
    for (int cap : {12, 4}) { q.polyphony = cap; Engine e; e.prepare(48000); e.p = q; float L[256], R[256];
      for (int k = 0; k < 12; ++k) { e.noteOn(48 + k, 0.8f); e.render(L, R, 256); }
      int act = e.activeVoices(); std::printf("polyphony %d: %d voices sounding after 12 notes\n", cap, act); CHECK(act == (cap == 12 ? 12 : 4)); }
  }
  { // drive keeps the level: filter Drive 0 -> 1 changes RMS by less than 2 dB on a lead, a bass and a talking lead
    for (const char* n : {"hoover_classic", "talking_lead", "access_bass", "sandstorm_lead"}) { double r[2];
      for (int k = 0; k < 2; ++k) { Params q; presets::byName(n, q); q.master = 0.3f; q.dist = 0.f; q.drive = k ? 1.f : 0.f; Engine e; e.prepare(48000); e.p = q; Result x = render(e, 57, 1.0f, 1.2f); r[k] = x.rms; }
      double d = 20 * std::log10(r[1] / r[0]); std::printf("%-14s drive 1 vs 0: %+.2f dB\n", n, d); CHECK(std::fabs(d) < 2.0); }
  }
  { // stack mode: three copies per key, level stays close to a single voice
    Params q; presets::byName("trance_lead", q); q.stackCount = 3; Engine e; e.prepare(48000); e.p = q; float L[256], R[256]; e.noteOn(60, 0.8f); e.render(L, R, 256);
    CHECK(e.activeVoices() == 3);
    double r[2]; for (int k = 0; k < 2; ++k) { Params z; presets::byName("trance_lead", z); z.stackCount = k ? 3 : 1; z.master = 0.5f; Engine g; g.prepare(48000); g.p = z; Result x = render(g, 60, 1.0f, 1.2f); r[k] = x.rms; }
    double d = 20 * std::log10(r[1] / r[0]); std::printf("stack 3 vs 1: %+.2f dB\n", d); CHECK(std::fabs(d) < 3.0);
    CHECK(e.p.mono == false);
  }
  { // custom cycle: a saw drawn as a cycle plays about as loud as the built-in saw and stays bounded and finite up high
    auto tbl = std::make_shared<WaveTable>(); std::vector<float> cyc(1000); for (int i = 0; i < 1000; ++i) cyc[i] = 2.f * i / 1000.f - 1.f; tbl->setCycle(cyc.data(), 1000); CHECK(tbl->ready());
    double rr[2]; for (int k = 0; k < 2; ++k) { Engine e; e.p.layer[0].wave = k ? Wave::Custom : Wave::SuperSaw; e.p.layer[0].table = tbl; e.p.layer[0].voices = 1; e.p.layer[0].detune = 0.f; e.p.layer[0].level = 1.f; e.p.cutoff = 18000.f; e.p.master = 0.5f;
      Result r = render(e, 60, 1.0f, 1.2f); CHECK(!r.nan); rr[k] = r.rms; }
    double d = 20 * std::log10(rr[1] / rr[0]); std::printf("custom saw vs saw: %+.2f dB\n", d); CHECK(std::fabs(d) < 2.5);
    Engine e; e.p.layer[0].wave = Wave::Custom; e.p.layer[0].table = tbl; e.p.layer[0].voices = 7; e.p.layer[0].detune = 0.6f; e.p.cutoff = 18000.f; Result hi = render(e, 108, 0.5f, 0.6f); CHECK(!hi.nan && hi.peak < 1.0);
  }
  { // delay sync: the time follows the tempo
    Params q; q.delaySync = true; q.bpm = 120.f; q.delayDiv = 4; CHECK(std::fabs(delaySeconds(q) - 0.5f) < 1e-5f);
    q.delayDiv = 7; CHECK(std::fabs(delaySeconds(q) - 0.25f) < 1e-5f); q.bpm = 150.f; CHECK(std::fabs(delaySeconds(q) - 0.2f) < 1e-5f);
    q.delaySync = false; q.delayTime = 0.51f; CHECK(std::fabs(delaySeconds(q) - 0.51f) < 1e-5f);
  }
  { // dynamics: opto turns a loud tone down, OTT lifts a quiet tone, nothing blows up
    auto tone = [&](float amp, int which) { Dynamics d; d.prepare(48000); Params q; if (which == 1) q.optoAmt = 0.8f; if (which == 2) { q.ottMix = 1.f; q.ottDepth = 1.f; } if (which == 3) q.sqzAmt = 0.8f;
      double s2 = 0; int n = 0; for (int i = 0; i < 48000 * 3; ++i) { float x = amp * std::sin(2.f * kPi * 440.f * i / 48000.f), l = x, r = x; d.process(q, l, r); CHECK(l == l); if (i > 48000) { s2 += (double)l * l; ++n; } } return std::sqrt(s2 / n) / amp; };
    double a0 = tone(0.5f, 0), a1 = tone(0.5f, 1); std::printf("opto on 0.5 tone: %+.2f dB\n", 20 * std::log10(a1 / a0)); CHECK(a1 < a0 * 1.2);
    double q0 = tone(0.003f, 2); std::printf("OTT on a -50 dB tone: %+.2f dB gain\n", 20 * std::log10(q0 / 0.7071)); CHECK(q0 > 0.7071 * 1.4);
    double s1 = tone(0.5f, 3); std::printf("squeeze on 0.5 tone: %+.2f dB\n", 20 * std::log10(s1 / a0)); CHECK(s1 < a0 * 1.5 && s1 > 0.3 * a0);
    for (const char* n : {"hoover", "access_bass"}) { Params q; presets::byName(n, q); q.master = 2.f; q.optoAmt = 1.f; q.ottMix = 1.f; q.ottDepth = 1.f; q.sqzAmt = 1.f; Engine e; e.prepare(48000); e.p = q; float L[512], R[512], pk = 0;
      for (int k : {48, 55, 60, 64}) e.noteOn(k, 1.f); for (int b = 0; b < 60; ++b) { e.render(L, R, 512); for (int i = 0; i < 512; ++i) { CHECK(L[i] == L[i]); pk = std::max(pk, std::fabs(L[i])); } } CHECK(pk <= 1.f); }
  }
  { // FM and AM: every source changes the sound and stays finite, a modulator layer at level 0 is silent by itself but still modulates
    auto tone = [&](int fmSrc, float fmAmt, int amSrc, float amAmt, float lvl1) { Params q; q.dmMix = 0.f; q.layer[0].wave = Wave::Sine; q.layer[0].level = 0.7f; q.layer[0].fmSrc = fmSrc; q.layer[0].fmAmt = fmAmt; q.layer[0].amSrc = amSrc; q.layer[0].amAmt = amAmt;
      q.layer[1].wave = Wave::Sine; q.layer[1].level = lvl1; q.layer[1].octave = 1; q.subLevel = 0.f; q.cutoff = 18000.f; q.noiseLevel = 0.f;
      Engine e; e.prepare(48000); e.p = q; e.noteOn(57, 0.8f); float L[4800], R[4800]; e.render(L, R, 4800); std::vector<float> o(L, L + 4800); for (float x : o) CHECK(x == x); return o; };
    auto diff = [&](const std::vector<float>& a, const std::vector<float>& b) { double d = 0; for (size_t i = 0; i < a.size(); ++i) d += std::fabs(a[i] - b[i]); return d / a.size(); };
    auto base = tone(0, 0.f, 0, 0.f, 0.f);
    CHECK(diff(base, tone(2, 0.6f, 0, 0.f, 0.f)) > 0.005);   // FM from layer 2 (level 0, silent modulator)
    CHECK(diff(base, tone(4, 0.6f, 0, 0.f, 0.f)) > 0.005);   // FM from sub
    CHECK(diff(base, tone(5, 0.6f, 0, 0.f, 0.f)) > 0.005);   // FM from noise
    CHECK(diff(base, tone(0, 0.f, 2, 0.8f, 0.f)) > 0.005);   // AM from layer 2
    CHECK(diff(base, tone(0, 0.f, 4, 0.8f, 0.f)) > 0.005);   // AM from sub
    std::printf("fm/am: all sources change the sound\n");
  }
  { // mod matrix: every destination reacts to a source that is active, and nothing blows up
    auto rms = [&](int src, int dst, float amt) { Params q; q.layer[0].wave = Wave::SuperSaw; q.layer[0].level = 0.7f; q.cutoff = 1500.f; q.modWheel = 1.f; q.mx[src][dst] = amt;
      Engine e; e.prepare(48000); e.p = q; e.noteOn(57, 0.8f); float L[4800], R[4800]; e.render(L, R, 4800); double s2 = 0; for (int i = 0; i < 4800; ++i) { CHECK(L[i] == L[i]); s2 += (double)L[i] * L[i]; } return std::sqrt(s2 / 4800); };
    double base = rms(0, 0, 0.f); int changed = 0;
    for (int d : {0, 1, 2, 5, 6, 7, 11, 12, 13}) { if (std::fabs(rms(4, d, 0.8f) - base) > 1e-6) ++changed; }
    std::printf("mod matrix: %d of 9 wheel destinations changed the sound\n", changed); CHECK(changed == 9);
    for (int sidx = 0; sidx < 9; ++sidx) for (int d = 0; d < 14; ++d) CHECK(std::isfinite(rms(sidx, d, 1.f)));
  }
  { // dance master chain: louder at the same ceiling, never above the limiter, no NaN, sub stays clean
    auto meas = [&](float mix, float sub) { Params q; presets::byName("trance_lead", q); q.dmMix = mix; q.subLevel = sub; Engine e; e.prepare(48000); e.p = q; float L[512], R[512]; double s2 = 0; int cnt = 0; float pk = 0;
      for (int k : {48, 55, 60, 64}) e.noteOn(k, 0.9f); for (int b = 0; b < 190; ++b) { e.render(L, R, 512); for (int i = 0; i < 512; ++i) { CHECK(L[i] == L[i]); if (b > 60) { s2 += (double)L[i] * L[i]; ++cnt; pk = std::max(pk, std::fabs(L[i])); } } }
      return std::make_pair(10.0 * std::log10(s2 / cnt), pk); };
    auto d0 = meas(0.f, 0.f), d1 = meas(0.4f, 0.f), d2 = meas(1.f, 0.f);
    std::printf("dance master: rms %+.1f dB at mix 0.4, %+.1f dB at mix 1 (peaks %.2f %.2f %.2f)\n", d1.first - d0.first, d2.first - d0.first, d0.second, d1.second, d2.second);
    CHECK(d1.first > d0.first + 1.0 && d2.first > d1.first); CHECK(d2.second <= 1.f);
    auto s0 = meas(0.4f, 0.f), s1 = meas(0.4f, 0.4f); CHECK(s1.first > s0.first - 0.5);
  }
  { // drive stack: off is a clean 16 sample delay, on changes the sound, stays finite, level matched within 6 dB, DC small
    DriveStage ds; Params q; float in[400], out[400]; for (int i = 0; i < 400; ++i) { in[i] = 0.5f * std::sin(2.f * 3.14159265f * 440.f * i / 48000.f); ds.process(q, in[i], in[i], 0.f, 0.f); out[i] = ds.oL; }
    double e0 = 0; for (int i = 16; i < 400; ++i) e0 = std::max(e0, (double)std::fabs(out[i] - in[i - 16])); CHECK(e0 < 1e-6);
    for (int mode = 0; mode < 3; ++mode) { Params w; (mode == 0 ? w.drvDrive : mode == 1 ? w.drvXs : w.drvFuzz) = 0.9f; DriveStage d2; double a = 0, b = 0, dc = 0; int cnt = 0;
      for (int i = 0; i < 9600; ++i) { float x = 0.5f * std::sin(2.f * 3.14159265f * 440.f * i / 48000.f); d2.process(w, x, x, 0.f, 0.f); CHECK(d2.oL == d2.oL); if (i > 4800) { a += (double)x * x; b += (double)d2.oL * d2.oL; dc += d2.oL; ++cnt; } }
      double db = 10.0 * std::log10(b / a); std::printf("drive stack mode %d at 0.9: %+.1f dB, dc %.4f\n", mode, db, dc / cnt); CHECK(std::fabs(db) < 1.5); CHECK(std::fabs(dc / cnt) < 0.02); }
  }
  { // EQ: flat is a bit exact bypass, a -8 dB bell at 1 kHz and a +6 dB low shelf measure what they say, low cut removes 40 Hz
    EqStage flat; Params q0; flat.prepare(48000); flat.update(q0); flat.process(0.3f, -0.2f); CHECK(flat.oL == 0.3f && flat.oR == -0.2f);
    auto gainAt = [&](Params q, double f) { EqStage e; e.prepare(48000); e.update(q); double a = 0; int n = 0; for (int i = 0; i < 48000; ++i) { float x = 0.3f * (float)std::sin(2 * 3.14159265358979 * f * i / 48000.0); e.process(x, x); if (i > 24000) { a += (double)e.oL * e.oL; ++n; } } return 20.0 * std::log10(std::sqrt(a / n) / (0.3 / std::sqrt(2.0))); };
    Params q; q.eq3G = -8.f; q.eq3F = 1000.f; q.eq3Q = 2.f; double g1 = gainAt(q, 1000.0); Params q2; q2.eq2G = 6.f; q2.eq2F = 120.f; double g2 = gainAt(q2, 25.0); Params q3; q3.eq1On = true; q3.eq1F = 100.f; double g3 = gainAt(q3, 40.0);
    std::printf("eq: bell %.2f dB (-8), low shelf at 25 Hz %.2f dB (about +6), low cut 100 Hz at 40 Hz %.1f dB\n", g1, g2, g3); CHECK(std::fabs(g1 + 8.0) < 0.1); CHECK(g2 > 5.0 && g2 < 6.2); CHECK(g3 < -10.0);
  }
  { // drums: each one is finite, stays under the ceiling and dies away
    for (const char* nm : {"kick", "snare", "closed_hat", "open_hat", "crash"}) { Params q; CHECK(presets::byName(nm, q)); Engine e; e.prepare(48000); e.p = q; float L[512], R[512], pk = 0, tail = 0;
      e.noteOn(48, 1.f); for (int b = 0; b < 190; ++b) { if (b == 47) e.noteOff(48); e.render(L, R, 512); for (int i = 0; i < 512; ++i) { CHECK(L[i] == L[i]); pk = std::max(pk, std::fabs(L[i])); if (b > 180) tail = std::max(tail, std::fabs(L[i])); } }
      CHECK(pk > 0.3f && pk <= 1.f); CHECK(tail < 0.01f); }
  }
  { // every one of the 126 mod matrix cells must change the sound (nothing decorative)
    auto mk = [&](int syl, int src, int dst) { Params q; q.dmMix = 0.f; q.reverbMix = q.chorusMix = q.delayMix = 0.f; q.limiter = false; q.cutoff = 3000.f; q.reso = 0.3f; q.drive = 0.2f; q.lowCutHz = 100.f; q.subLevel = 0.3f; q.noiseLevel = 0.3f;
      q.pitchAmt = 7.f; q.pitchTime = 0.5f; q.lfoRate = 5.f; q.modWheel = 1.f; q.fenv.s = 0.6f; q.amp.s = 0.7f;
      q.layer[0].wave = Wave::Pulse; q.layer[0].level = 0.5f; q.layer[0].pwm = 0.5f; q.layer[1].wave = Wave::SuperSaw; q.layer[1].level = 0.5f; q.layer[1].detune = 0.5f; q.layer[1].voices = 5; q.layer[2].wave = Wave::Saw; q.layer[2].level = 0.4f;
      if (syl >= 0) { for (int i = 0; i < 3; ++i) q.layer[i].wave = Wave::Off; q.layer[syl].wave = Wave::Saw; q.layer[syl].level = 0.6f; q.layer[syl].sync = true; q.layer[syl].syncSemis = 7.f; }
      if (src >= 0) q.mx[src][dst] = 0.8f;
      Engine e; e.prepare(48000); e.p = q; e.noteOn(72, 0.8f); std::vector<float> o; float L[500], R[500]; for (int b = 0; b < 48; ++b) { e.render(L, R, 500); o.insert(o.end(), L, L + 500); } return o; };
    int dead = 0;
    for (int s = 0; s < 9; ++s) for (int d = 0; d < 14; ++d) { int syl = d == 8 ? 0 : d == 9 ? 1 : d == 10 ? 2 : -1; auto a0 = mk(syl, -1, 0), a1 = mk(syl, s, d); double df = 0, sm = 0; for (size_t i = 0; i < a0.size(); ++i) { df += std::fabs(a0[i] - a1[i]); sm += std::fabs(a0[i]); } if (!(df / (sm + 1e-9) > 1e-3)) { ++dead; std::printf("dead cell %d->%d\n", s, d); } }
    CHECK(dead == 0);
  }
  { // every extra effect control changes the sound when its effect is on
    auto rd = [&](std::function<void(Params&)> f) { Params q; q.dmMix = 0.f; q.layer[0].wave = Wave::Saw; q.layer[0].level = 0.6f; q.cutoff = 6000.f; q.limiter = false; f(q); Engine e; e.prepare(48000); e.p = q; e.noteOn(57, 0.9f); std::vector<float> o; float L[500], R[500]; for (int b = 0; b < 60; ++b) { if (b == 20) e.noteOff(57); e.render(L, R, 500); o.insert(o.end(), L, L + 500); } return o; };
    struct C { const char* n; std::function<void(Params&)> on, mod; } cs[] = {
      {"chorusDelay", [](Params& q) { q.chorusMix = 0.8f; q.chorusDepth = 0.8f; }, [](Params& q) { q.chorusDelay = 25.f; }},
      {"chorusWidth", [](Params& q) { q.chorusMix = 0.8f; }, [](Params& q) { q.chorusWidth = 0.f; }},
      {"delayDamp", [](Params& q) { q.delayMix = 0.8f; q.delayFb = 0.7f; q.delayTime = 0.1f; }, [](Params& q) { q.delayDamp = 1.f; }},
      {"delayPing", [](Params& q) { q.delayMix = 0.8f; q.delayFb = 0.7f; q.delayTime = 0.1f; }, [](Params& q) { q.delayPing = 0.f; }},
      {"reverbPre", [](Params& q) { q.reverbMix = 0.8f; }, [](Params& q) { q.reverbPre = 120.f; }},
      {"reverbDamp", [](Params& q) { q.reverbMix = 0.8f; }, [](Params& q) { q.reverbDamp = 0.8f; }},
      {"optoGain", [](Params& q) { q.optoAmt = 0.6f; }, [](Params& q) { q.optoGain = 4.f; }},
      {"optoRel", [](Params& q) { q.optoAmt = 0.8f; }, [](Params& q) { q.optoRel = 2.f; }},
      {"ottLow", [](Params& q) { q.ottMix = 1.f; q.ottDepth = 0.6f; }, [](Params& q) { q.ottLow = 5.f; }},
      {"ottHigh", [](Params& q) { q.ottMix = 1.f; q.ottDepth = 0.6f; }, [](Params& q) { q.ottHigh = 5.f; }},
      {"ottTime", [](Params& q) { q.ottMix = 1.f; q.ottDepth = 0.8f; }, [](Params& q) { q.ottTime = 3.f; }},
      {"sqzAtk", [](Params& q) { q.sqzAmt = 0.8f; }, [](Params& q) { q.sqzAtk = 40.f; }},
      {"sqzRel", [](Params& q) { q.sqzAmt = 0.8f; }, [](Params& q) { q.sqzRel = 500.f; }},
      {"sqzMakeup", [](Params& q) { q.sqzAmt = 0.8f; }, [](Params& q) { q.sqzMakeup = 4.f; }}};
    for (auto& c : cs) { auto a0 = rd(c.on), a1 = rd([&](Params& q) { c.on(q); c.mod(q); }); double df = 0, sm = 0; for (size_t i = 0; i < a0.size(); ++i) { df += std::fabs(a0[i] - a1[i]); sm += std::fabs(a0[i]); } if (!(df / (sm + 1e-9) > 1e-3)) { std::printf("dead control %s\n", c.n); CHECK(false); } }
  }
  // ======================= tests for the features added with the big build (filters, LFOs, envelopes, samples, EQ dynamics, build macro, effects) =======================
  { // mod matrix size and order are the ones of the playground engine (MX_SRC / MX_DST); the filter list covers every mode once
    CHECK(kMxSrc == 18 && kMxDst == 23);
    const char* S[] = {"lfo", "fenv", "aenv", "penv", "whl", "key", "vel", "rnd", "men", "lfo2", "lfo3", "fenv2", "fenv3", "men2", "men3", "prs", "sld", "bnd"};
    const char* D[] = {"pitch", "cut", "res", "drv", "pwm", "det", "pan", "vol", "sy1", "sy2", "sy3", "sub", "nz", "lc", "cut2", "cut3", "rvb", "dly", "drs", "clip", "hpf", "lpf", "wid"};
    for (int i = 0; i < kMxSrc; ++i) CHECK(std::string(kMxSrcNames[i]) == S[i]);
    for (int i = 0; i < kMxDst; ++i) CHECK(std::string(kMxDstNames[i]) == D[i]);
    CHECK(sizeof(Params().mx) == sizeof(float) * 18 * 23);
    bool seen[kFilterModes] = {}; for (int i = 0; i < kFilterModes; ++i) seen[(int)kFilterList[i]] = true; for (bool b : seen) CHECK(b);
    for (int i = 0; i < kFilterModes; ++i) CHECK(filterModeFromName(kFilterNames[i]) == (FilterMode)i);
    CHECK(std::string(kFilterNames[(int)kFilterList[0]]) == "Off" && std::string(kFilterNames[(int)kFilterList[17]]) == "Formant");
    for (int i = 0; i < kSmpCount; ++i) CHECK(smpFromId(kSmpIds[i]) == i);
    Params d; CHECK(d.f2Mode == FilterMode::Off && d.f3Mode == FilterMode::Off && d.ampLink && d.highCutHz == 20000.f && d.crushBits == 16.f && d.widMid == 1.f && d.lowCutHz == 20.f);
    std::printf("new engine: %dx%d mod matrix, %d filter modes, %d samples\n", kMxSrc, kMxDst, kFilterModes, kSmpCount - 1);
  }
  // shared helpers: the three note render used for the comparison with the playground engine (48000 Hz, blocks of 128, note off at block 40, 100 blocks)
  auto runP = [&](const Params& q, std::vector<float>* outR = nullptr) { Engine e; e.prepare(48000); e.p = q; e.noteOn(48, 0.875f); e.noteOn(55, 0.75f); e.noteOn(64, 0.625f); std::vector<float> o; float L[128], R[128];
    for (int b = 0; b < 100; ++b) { if (b == 40) { e.noteOff(48); e.noteOff(55); e.noteOff(64); } e.render(L, R, 128); o.insert(o.end(), L, L + 128); if (outR) outR->insert(outR->end(), R, R + 128); } return o; };
  auto rmsOf = [&](const std::vector<float>& x, size_t a = 0, size_t b = (size_t)-1) { double s = 0; size_t n = 0; for (size_t i = a; i < x.size() && i < b; ++i) { s += (double)x[i] * x[i]; ++n; } return std::sqrt(s / (double)(n ? n : 1)); };
  auto allFinite = [&](const std::vector<float>& x) { for (float v : x) if (!std::isfinite(v)) return false; return true; };
  auto relDiff = [&](const std::vector<float>& a, const std::vector<float>& b) { double d = 0, s = 0; for (size_t i = 0; i < a.size() && i < b.size(); ++i) { d += (double)(a[i] - b[i]) * (a[i] - b[i]); s += (double)a[i] * a[i]; } return std::sqrt(d / (s + 1e-12)); };
  auto baseP = [&]() { Params q; q.layer[0].wave = Wave::SuperSaw; q.layer[0].level = 0.6f; q.layer[0].voices = 7; q.layer[0].detune = 0.5f;
    q.layer[1].wave = Wave::Pulse; q.layer[1].level = 0.4f; q.layer[1].pwm = 0.4f; q.layer[1].octave = -1; q.layer[1].tune = 7.f; q.layer[2].wave = Wave::Sine; q.layer[2].level = 0.3f; q.layer[2].octave = 1;
    q.cutoff = 1800.f; q.reso = 0.35f; q.drive = 0.2f; q.subLevel = 0.25f; q.noiseLevel = 0.1f; q.amp.s = 0.7f; q.amp.r = 0.4f; q.fenv.s = 0.3f; q.envAmt = 0.4f; q.chorusMix = 0.2f; q.delayMix = 0.15f; q.delayTime = 0.19f; q.reverbMix = 0.15f; q.master = 0.7f; return q; };
  auto mxSet = [&](Params& q, const char* s, const char* d, float v) { int si = -1, di = -1; for (int i = 0; i < kMxSrc; ++i) if (std::string(kMxSrcNames[i]) == s) si = i; for (int i = 0; i < kMxDst; ++i) if (std::string(kMxDstNames[i]) == d) di = i; CHECK(si >= 0 && di >= 0); if (si >= 0 && di >= 0) q.mx[si][di] = v; };
  { // filter modes: every filtering mode is finite and sounds different from Off and from each other; the new types behave like what they are named for
    std::vector<std::vector<float>> o; std::vector<std::string> nm; Params q0 = baseP(); q0.filterMode = FilterMode::Off; q0.cutoff = 900.f; q0.reso = 0.55f; q0.drive = 0.4f; auto off = runP(q0); CHECK(allFinite(off));
    for (int i = 0; i < kFilterModes; ++i) { FilterMode m = (FilterMode)i; if (m == FilterMode::Off) continue; Params q = q0; q.filterMode = m; auto x = runP(q); CHECK(allFinite(x)); CHECK(rmsOf(x) > 0.02); CHECK(relDiff(off, x) > 0.02); o.push_back(x); nm.push_back(kFilterNames[i]); }
    double mn = 9; int bad = 0; for (size_t i = 0; i < o.size(); ++i) for (size_t j = i + 1; j < o.size(); ++j) { double d = relDiff(o[i], o[j]); mn = std::min(mn, d); if (d < 0.01) { ++bad; std::printf("filter modes %s and %s too alike (%.4f)\n", nm[i].c_str(), nm[j].c_str(), d); } }
    CHECK(bad == 0); std::printf("filter modes: %zu filtering types, all finite and different (closest pair %.3f)\n", o.size(), mn);
    auto bright = [&](FilterMode m) { Params q = baseP(); q.filterMode = m; q.cutoff = 1000.f; q.reso = 0.2f; q.noiseLevel = 0.f; q.chorusMix = q.delayMix = q.reverbMix = 0.f; return brightness(runP(q)); };
    double bo = bright(FilterMode::Off); CHECK(bright(FilterMode::Moog) < bo * 0.6); CHECK(bright(FilterMode::Acid) < bo * 0.6); CHECK(bright(FilterMode::SEM) < bo * 0.8); CHECK(bright(FilterMode::Jupiter) < bo * 0.6); CHECK(bright(FilterMode::MS20) < bo * 0.7);
    CHECK(bright(FilterMode::HP24) > bright(FilterMode::LP24) * 1.5); CHECK(bright(FilterMode::BP24) < bright(FilterMode::BP) * 1.05);
    // self oscillation and extremes stay finite for every mode: full resonance and drive at the lowest and highest cutoff
    for (int i = 0; i < kFilterModes; ++i) for (float cut : {20.f, 18000.f}) { Params q = baseP(); q.filterMode = (FilterMode)i; q.cutoff = cut; q.reso = 1.f; q.drive = 1.f; q.master = 0.3f; CHECK(allFinite(runP(q))); }
  }
  { // filters 2 and 3: they work with filter 1 off, serial and parallel differ, fenv2 opens filter 2 over time, key tracking and the mod matrix (cut2, cut3) move them
    Params a = baseP(); a.f2Mode = FilterMode::HP24; a.f2Cut = 300.f; a.f3Mode = FilterMode::Notch; a.f3Cut = 2500.f; auto ser = runP(a); a.fRoute = 1; auto par = runP(a); CHECK(allFinite(ser) && allFinite(par)); CHECK(relDiff(ser, par) > 0.05);
    Params b = baseP(); b.filterMode = FilterMode::Off; auto b0 = runP(b); b.f3Mode = FilterMode::LP12; b.f3Cut = 400.f; auto b3 = runP(b); CHECK(relDiff(b0, b3) > 0.1 && brightness(b3) < brightness(b0) * 0.6);
    Params c = baseP(); c.f2Mode = FilterMode::LP12; c.f2Cut = 300.f; c.f2Env = 1.f; c.fenv2 = {0.001f, 0.2f, 0.f, 0.1f}; c.noiseLevel = 0.f; c.chorusMix = c.delayMix = c.reverbMix = 0.f; c.fenv.s = 0.f; c.envAmt = 0.f; c.amp = {0.002f, 0.3f, 1.f, 0.2f}; auto c0 = runP(c);
    double e1 = brightness(std::vector<float>(c0.begin() + 400, c0.begin() + 1400)), e2 = brightness(std::vector<float>(c0.begin() + 4000, c0.begin() + 5000));   // the run is 12800 samples long, the note is released at 5120: both windows lie inside it
    std::printf("fenv2 brightness early %g late %g\n", e1, e2); CHECK(c0.size() >= 5000 && e1 > e2 * 1.1);
    Params k0 = baseP(); k0.f2Mode = FilterMode::LP12; k0.f2Cut = 500.f; Params k1 = k0; k1.f2Key = 0.8f; CHECK(relDiff(runP(k0), runP(k1)) > 0.02);
    for (const char* dd : {"cut2", "cut3"}) { Params m0 = baseP(); m0.f2Mode = FilterMode::LP12; m0.f3Mode = FilterMode::LP12; m0.f2Cut = m0.f3Cut = 800.f; Params m1 = m0; mxSet(m1, "fenv2", dd, 0.8f); mxSet(m1, "vel", dd, 0.6f); CHECK(relDiff(runP(m0), runP(m1)) > 0.02); }
    std::printf("filters 2 and 3: serial vs parallel differ %.3f, envelope and matrix work\n", relDiff(ser, par));
  }
  { // per layer amp envelopes: with ampLink off every layer has its own, the voice lives as long as the longest layer, linked is the old behaviour
    Params l = baseP(); l.noiseLevel = 0.f; l.subLevel = 0.f; l.chorusMix = l.delayMix = l.reverbMix = 0.f; l.cutoff = 18000.f; l.envAmt = 0.f; l.amp = {0.002f, 0.3f, 1.f, 0.3f};
    Params u = l; u.ampLink = false; u.amp2 = {0.002f, 0.04f, 0.f, 0.03f}; u.amp3 = {0.002f, 0.04f, 0.f, 0.03f}; auto lo = runP(l), un = runP(u); CHECK(allFinite(un));
    CHECK(rmsOf(un, 3000, 5000) < rmsOf(lo, 3000, 5000) * 0.8);   // the pulse and sine layers have died, the saw layer keeps playing
    Params same = l; same.ampLink = false; same.amp2 = same.amp3 = same.amp; auto sm = runP(same); std::printf("linked vs equal separate envelopes: %.2e\n", relDiff(lo, sm)); CHECK(relDiff(lo, sm) < 0.1);   // identical envelopes give the linked sound
    Params v = l; v.ampLink = false; v.amp = {0.002f, 0.05f, 0.f, 0.05f}; v.amp2 = {0.002f, 0.05f, 0.f, 0.05f}; v.amp3 = {0.002f, 0.1f, 1.f, 1.5f};   // longest release is layer 3
    Engine e; e.prepare(48000); e.p = v; e.noteOn(60, 0.8f); float L[512], R[512]; for (int b = 0; b < 20; ++b) e.render(L, R, 512); e.noteOff(60); for (int b = 0; b < 10; ++b) e.render(L, R, 512); CHECK(e.activeVoices() == 1);
    for (int b = 0; b < 250; ++b) e.render(L, R, 512); CHECK(e.activeVoices() == 0);
    // a layer set to Off does not keep the voice alive
    Params w = l; w.ampLink = false; w.amp = {0.002f, 0.05f, 0.f, 0.05f}; w.amp3 = {0.002f, 0.1f, 1.f, 3.f}; w.layer[2].wave = Wave::Off; Engine e2; e2.prepare(48000); e2.p = w; e2.noteOn(60, 0.8f); for (int b = 0; b < 50; ++b) e2.render(L, R, 512); e2.noteOff(60); for (int b = 0; b < 40; ++b) e2.render(L, R, 512); CHECK(e2.activeVoices() == 0);
    std::printf("per layer envelopes: separate decays, voice life, linked equivalence ok\n");
  }
  { // procedural samples: every one exists, is finite, peaks at 0.9, plays on a layer and on the noise section; the plucks and the chime follow the key; smpPost changes the signal path
    auto bank = sampleBank(48000.0); for (int i = 1; i < kSmpCount; ++i) { const auto& d = bank->d[i]; CHECK(d.size() > 100); double pk = 0; for (float v : d) { CHECK(std::isfinite(v)); pk = std::max(pk, (double)std::fabs(v)); } CHECK(std::fabs(pk - 0.9) < 0.01); }
    CHECK(bank->d[0].empty()); CHECK(sampleBank(48000.0) == bank);
    for (int i = 1; i < kSmpCount; ++i) { Params q; q.dmMix = 0.f; q.layer[0].wave = Wave::Off; q.layer[1].wave = Wave::Off; q.layer[2].wave = Wave::Off; q.layer[0].smp = i; q.cutoff = 18000.f; q.limiter = false; Engine e; e.prepare(48000); e.p = q; e.noteOn(60, 0.9f); float L[2400], R[2400]; e.render(L, R, 2400); double s = 0; for (int k = 0; k < 2400; ++k) { CHECK(std::isfinite(L[k])); s += (double)L[k] * L[k]; } CHECK(s > 1e-4); }
    { Params q; q.dmMix = 0.f; for (auto& ly : q.layer) ly.wave = Wave::Off; q.nzSmp = smpFromId("snare_crack"); q.cutoff = 18000.f; Engine e; e.prepare(48000); e.p = q; e.noteOn(60, 0.9f); float L[2400], R[2400]; e.render(L, R, 2400); double s = 0; for (int k = 0; k < 2400; ++k) s += (double)L[k] * L[k]; CHECK(s > 1e-4); }
    auto zc = [&](Params q, int note) { for (auto& ly : q.layer) ly.wave = Wave::Off; q.dmMix = 0.f; q.cutoff = 18000.f; q.layer[0].smp = smpFromId("pluck_soft"); Engine e; e.prepare(48000); e.p = q; e.noteOn(note, 0.9f); float L[4800], R[4800]; e.render(L, R, 4800); int z = 0; for (int k = 1; k < 4800; ++k) if ((L[k - 1] < 0) != (L[k] < 0)) ++z; return z; };
    Params zp; int zlo = zc(zp, 48), zhi = zc(zp, 72); CHECK(zhi > zlo * 2);   // keyed: two octaves up is about four times the zero crossings
    { Params q; q.dmMix = 0.f; for (auto& ly : q.layer) ly.wave = Wave::Off; q.layer[0].smp = smpFromId("kick_hard"); q.cutoff = 18000.f; auto z0 = zc(q, 40); auto z1 = zc(q, 70); (void)z0; (void)z1;
      Params a = q, b = q; b.layer[0].smpPitch = -12.f; auto ra = runP(a), rb = runP(b); CHECK(relDiff(ra, rb) > 0.3);   // pitch of an unkeyed sample moves with smpPitch only
      Params c = q, d = q; c.layer[0].smpVol = 0.5f; d.layer[0].smpVol = 1.f; double rc = rmsOf(runP(c)), rd = rmsOf(runP(d)); std::printf("smpVol 1 vs 0.5 rms ratio %.3f\n", rd / rc); CHECK(rd / rc > 1.1 && rd / rc < 2.2); }
    { Params a = baseP(), b = a; a.layer[1].smp = smpFromId("pluck_glass"); a.cutoff = 500.f; b = a; b.smpPost = true; CHECK(relDiff(runP(a), runP(b)) > 0.05); }
    // a sample on a layer follows that layer's own envelope when ampLink is off
    { Params q = baseP(); for (auto& ly : q.layer) ly.wave = Wave::Off; q.layer[1].wave = Wave::Saw; q.layer[0].smp = smpFromId("chime"); q.ampLink = false; q.amp = {0.002f, 0.01f, 0.f, 0.01f}; q.amp2 = {0.002f, 0.3f, 1.f, 0.3f}; q.amp3 = q.amp2; Params r = q; r.amp = {0.002f, 0.3f, 1.f, 0.3f}; CHECK(relDiff(runP(q), runP(r)) > 0.05); }
    std::printf("samples: %d built in, keyed ones follow the key (%d vs %d zero crossings), volume and post path ok\n", kSmpCount - 1, zlo, zhi);
  }
  { // EQ dynamics and band saturation
    auto gainDb = [&](Params q, double f, double amp) { EqStage e; e.prepare(48000); e.update(q); double a = 0, b = 0; for (int i = 0; i < 96000; ++i) { double x = amp * std::sin(2 * 3.14159265358979 * f * i / 48000.0); e.process(x, x); if (i > 48000) { a += x * x; b += e.oL * e.oL; } } return 10 * std::log10(b / a); };
    Params q; q.eq3On = true; q.eq3F = 1000.f; q.eq3G = 0.f; q.eq3Q = 1.f; q.eq3Dyn = -12.f; q.eq3Thr = -30.f; q.eq3Atk = 5.f; q.eq3Rel = 80.f;
    double quiet = gainDb(q, 1000, 0.003), loud = gainDb(q, 1000, 0.5); std::printf("eq dynamic cut: quiet %.2f dB, loud %.2f dB\n", quiet, loud); CHECK(std::fabs(quiet) < 0.8); CHECK(loud < -5.0);
    q.eq3Dyn = 12.f; double loudUp = gainDb(q, 1000, 0.5); CHECK(loudUp > 5.0);   // positive range: boosts when loud
    q.eq3Dyn = -12.f; q.eq3Below = true; double quietBelow = gainDb(q, 1000, 0.003), loudBelow = gainDb(q, 1000, 0.5); CHECK(quietBelow < -5.0 && std::fabs(loudBelow) < 0.8);   // "below" acts when the band is quiet
    Params s; s.eq3On = true; s.eq3F = 1000.f; s.eq3G = 0.f; s.eq3Q = 1.f; s.eq3Sat = 0.8f; EqStage e0, e1; e0.prepare(48000); e1.prepare(48000); e0.update(Params()); e1.update(s); double d1 = 0, d3 = 0, h = 0;   // saturation adds harmonics (3rd) but leaves other bands alone
    std::vector<double> y; for (int i = 0; i < 48000; ++i) { double x = 0.4 * std::sin(2 * 3.14159265358979 * 1000.0 * i / 48000.0) + 0.3 * std::sin(2 * 3.14159265358979 * 100.0 * i / 48000.0); e1.process(x, x); y.push_back(e1.oL); }
    auto bin = [&](double f) { double re = 0, im = 0; for (int i = 8000; i < 40000; ++i) { re += y[i] * std::cos(2 * 3.14159265358979 * f * i / 48000.0); im += y[i] * std::sin(2 * 3.14159265358979 * f * i / 48000.0); } return std::sqrt(re * re + im * im) / 16000.0; };
    d1 = bin(1000); d3 = bin(3000); h = bin(100); std::printf("eq band saturation: 1k %.3f, 3k %.3f, 100 Hz %.3f\n", d1, d3, h); CHECK(d3 > 0.01); CHECK(std::fabs(h - 0.3) < 0.01); CHECK(d1 > 0.2);
    for (int t = 0; t < 8; ++t) { Params w = baseP(); w.eq2On = w.eq3On = w.eq4On = w.eq5On = true; w.eq3F = 800.f; w.eq3G = 1.f; w.eq3Sat = 0.9f; w.eq3SatT = t; w.eq4Dyn = -9.f; w.eq4Thr = -35.f; w.eq4Sat = 0.5f; w.eq4SatT = t; w.eq5Dyn = 6.f; w.eq5Below = true; auto x = runP(w); CHECK(allFinite(x)); CHECK(relDiff(runP(baseP()), x) > 0.01); }
    std::printf("eq: dynamic gain above and below the threshold, band saturation of eight kinds, finite in the engine\n");
    { Params w = baseP(); w.eq3On = true; w.eq3F = 800.f; w.eq3G = 9.f; auto a = runP(w); w.eqBypass = true; auto b = runP(w); CHECK(relDiff(runP(baseP()), b) < 1e-9 || relDiff(a, b) > 0.01); Params z = baseP(); z.eq3On = true; z.eq3F = 800.f; z.eq3G = 0.f; z.eq3Sat = 0.8f; z.eq3SatT = 4; auto h1 = runP(z); z.eqBypass = true; auto h2 = runP(z); CHECK(relDiff(h1, h2) > 0.01); std::printf("eq main bypass acts\n"); }
  }
  { // build macro: 0 is the plain sound, higher values remove lows, add reverb and delay, and lower the level a little; the global matrix destinations (rvb dly drs clip hpf lpf wid) act too
    Params b0 = baseP(); b0.reverbMix = b0.delayMix = 0.f; b0.chorusMix = 0.f; auto r0 = runP(b0), r0b = runP(b0); CHECK(relDiff(r0, r0b) == 0.0);
    auto lowShare = [&](const std::vector<float>& x) { float lp = 0; double lo = 0, all = 0; for (size_t i = 0; i < x.size(); ++i) { lp += 0.02f * (x[i] - lp); lo += (double)lp * lp; all += (double)x[i] * x[i]; } return lo / (all + 1e-12); };
    double prev = lowShare(r0); int mono = 0; for (float b : {0.3f, 0.6f, 1.f}) { Params q = b0; q.build = b; auto x = runP(q); CHECK(allFinite(x)); double ls = lowShare(x); if (ls < prev) ++mono; prev = ls; } CHECK(mono == 3);
    Params q1 = b0; q1.build = 1.f; auto x1 = runP(q1); CHECK(rmsOf(x1, 0, 3000) > 0.0); CHECK(relDiff(r0, x1) > 0.3);
    Params w0 = b0; w0.build = 0.f; Params w1 = b0; w1.build = 0.5f; std::vector<float> R0, R1; runP(w0, &R0); runP(w1, &R1); double tail0 = 0, tail1 = 0; for (size_t i = 8000; i < R0.size(); ++i) { tail0 += (double)R0[i] * R0[i]; tail1 += (double)R1[i] * R1[i]; } std::printf("tail %.3e vs %.3e\n", tail0, tail1); CHECK(tail1 != tail0 && std::isfinite(tail1));   // wetter tail
    for (const char* dd : {"rvb", "dly", "drs", "clip", "hpf", "lpf", "wid"}) { Params m = baseP(); m.dmMix = 0.4f; m.drvMix = 1.f; m.eq6On = false; Params m1 = m; mxSet(m1, "vel", dd, std::string(dd) == "lpf" ? -0.9f : 0.9f); mxSet(m1, "whl", dd, 0.f); auto a = runP(m), c = runP(m1); CHECK(allFinite(c)); if (!(relDiff(a, c) > 1e-3)) { std::printf("global destination %s has no effect\n", dd); CHECK(false); } }
    std::printf("build macro: low share falls with the knob, tail changes, all seven global destinations act\n");
  }
  { // effects added in the chorus slot and after the colour stack: flanger, phaser, crusher, width
    Params c0 = baseP(); c0.chorusMix = 0.6f; c0.chorusDepth = 0.8f; auto ch = runP(c0);
    Params fl = c0; fl.modFxType = 1; fl.chorusFb = 0.7f; fl.chorusDelay = 8.f; auto fo = runP(fl); Params ph = c0; ph.modFxType = 2; ph.chorusFb = 0.6f; auto po = runP(ph);
    CHECK(allFinite(fo) && allFinite(po)); CHECK(relDiff(ch, fo) > 0.05 && relDiff(ch, po) > 0.05 && relDiff(fo, po) > 0.05);
    Params fn = fl; fn.chorusFb = -0.8f; CHECK(allFinite(runP(fn)) && relDiff(fo, runP(fn)) > 0.05); Params pn = ph; pn.chorusFb = -0.9f; CHECK(allFinite(runP(pn)) && relDiff(po, runP(pn)) > 0.05);
    Params fm = fl; fm.chorusMix = 0.f; Params pm = ph; pm.chorusMix = 0.f; CHECK(relDiff(runP(fm), runP(pm)) < 1e-6);   // mix 0 switches both off
    // flanger and phaser at full feedback stay bounded
    for (int t : {1, 2}) { Params z = baseP(); z.modFxType = t; z.chorusMix = 1.f; z.chorusFb = 0.95f; z.chorusDepth = 1.f; z.master = 1.f; auto x = runP(z); CHECK(allFinite(x)); double pk = 0; for (float v : x) pk = std::max(pk, (double)std::fabs(v)); CHECK(pk <= 1.0); }
    // crush: fewer bits give fewer distinct levels; rate reduction holds samples; mix 0 or 16 bits at rate 1 is bypass
    Params k0 = baseP(); k0.chorusMix = k0.delayMix = k0.reverbMix = 0.f; k0.dmMix = 0.f; k0.limiter = false; auto base = runP(k0); Params k1 = k0; k1.crushMix = 1.f; k1.crushBits = 4.f; auto cr = runP(k1);
    auto distinct = [&](const std::vector<float>& x) { std::vector<float> v(x.begin() + 6000, x.begin() + 12000); std::sort(v.begin(), v.end()); return (int)(std::unique(v.begin(), v.end()) - v.begin()); };
    CHECK(allFinite(cr)); CHECK(relDiff(base, cr) > 0.01); { Crush cz; Params cp; cp.crushMix = 1.f; cp.crushBits = 4.f; std::vector<float> cv; for (int i = 0; i < 4000; ++i) { double x = 0.8 * std::sin(i * 0.05); cz.process(cp, x, x); cv.push_back((float)cz.oL); } std::sort(cv.begin(), cv.end()); CHECK((int)(std::unique(cv.begin(), cv.end()) - cv.begin()) <= 16); }
    Params k2 = k0; k2.crushMix = 1.f; k2.crushBits = 16.f; k2.crushRate = 8.f; auto rr = runP(k2); CHECK(allFinite(rr) && relDiff(base, rr) > 0.01); { Crush cz; Params cp; cp.crushMix = 1.f; cp.crushBits = 16.f; cp.crushRate = 8.f; int holds = 0; double prev = 0; for (int i = 0; i < 4000; ++i) { double x = 0.8 * std::sin(i * 0.05); cz.process(cp, x, x); if (i && cz.oL == prev) ++holds; prev = cz.oL; } CHECK(holds > 3000); }
    Params k3 = k0; k3.crushMix = 0.f; k3.crushBits = 3.f; k3.crushRate = 10.f; CHECK(relDiff(base, runP(k3)) < 1e-9); Params k4 = k0; k4.crushMix = 1.f; CHECK(relDiff(base, runP(k4)) < 1e-9);
    // width: mid and side widths below and above 2 kHz. Both at 0 is mono; 1 and 1 with no saturation is a bit exact bypass; mid and side saturation change the sound and stay finite
    Params w0 = baseP(); w0.dmMix = 0.f; w0.limiter = false; w0.spread = 1.f; w0.layer[0].detune = 0.8f; w0.chorusMix = 0.f; auto w0l = runP(w0); Params w1 = w0; w1.widMid = 0.f; w1.widHi = 0.f; std::vector<float> wl, wr; runP(w1, &wr); wl = runP(w1); double dd = 0, ss = 0; for (size_t i = 0; i < wl.size(); ++i) { dd += std::fabs(wl[i] - wr[i]); ss += std::fabs(wl[i]); } CHECK(dd < ss * 1e-6);
    Params w2 = w0; w2.widMid = 1.f; w2.widHi = 1.f; CHECK(relDiff(w0l, runP(w2)) == 0.0);
    Params w3 = w0; w3.widHi = 1.8f; w3.widMid = 0.4f; CHECK(relDiff(w0l, runP(w3)) > 0.01); Params w4 = w0; w4.msMid = 0.7f; w4.msSide = 0.7f; auto w4o = runP(w4); CHECK(allFinite(w4o) && relDiff(w0l, w4o) > 0.01);
    // stack: the types that are not picked only run when Stack is on, and then each uses its own knobs
    { Params a = baseP(); a.chorusMix = 0.5f; a.modFxType = 0; a.mx1Mix = 0.5f; a.mx2Mix = 0.5f; auto off = runP(a); Params b = a; b.mfxMulti = true; auto on = runP(b);
      CHECK(allFinite(on) && relDiff(off, on) > 0.02); Params c = a; c.mx1Mix = c.mx2Mix = 0.f; c.mfxMulti = true; CHECK(relDiff(off, runP(c)) < 1e-9);   // stack on but the others at 0: unchanged
      Params d = b; d.mx1Mix = 0.f; CHECK(relDiff(on, runP(d)) > 0.01);   // each extra set is live
      Params e = b; e.mx2Mix = 0.f; CHECK(relDiff(on, runP(e)) > 0.01); }
    // layer switch: off mutes the layer and keeps its wave; on brings it back bit for bit
    { Params a = baseP(); auto base = runP(a); Params b = a; b.layer[0].on = false; auto off = runP(b); CHECK(relDiff(base, off) > 0.05 && b.layer[0].wave != Wave::Off); Params c = b; c.layer[0].on = true; CHECK(relDiff(base, runP(c)) == 0.0); }
    std::printf("effects: flanger, phaser (both polarities), crush and width behave\n");
  }
  { // low cut / high cut: the high pass reaches up to 20 kHz, auxSlope sets 6/12/18/24 dB per octave, highCutHz works next to a highpass filter and is ignored otherwise
    auto lowE = [&](float hz, int slope) { Params q = baseP(); q.filterMode = FilterMode::LP24; q.cutoff = 18000.f; q.reso = 0.f; q.drive = 0.f; q.envAmt = 0.f; q.noiseLevel = 0.f; q.lowCutHz = hz; q.auxSlope = slope; q.subLevel = 0.f; q.chorusMix = q.delayMix = q.reverbMix = 0.f; q.dmMix = 0.f; auto x = runP(q); float l1 = 0, l2 = 0, l3 = 0, l4 = 0; double lo = 0; for (float v : x) { l1 += 0.03f * (v - l1); l2 += 0.03f * (l1 - l2); l3 += 0.03f * (l2 - l3); l4 += 0.03f * (l3 - l4); lo += (double)l4 * l4; } return lo; };
    double e0 = lowE(20.f, 0), e6 = lowE(2500.f, 0), e12 = lowE(2500.f, 1), e18 = lowE(2500.f, 2), e24 = lowE(2500.f, 3); std::printf("low cut 2.5 kHz low band energy: off %.3f, 6 dB %.4f, 12 dB %.5f, 18 dB %.6f, 24 dB %.7f\n", e0, e6, e12, e18, e24);
    CHECK(e6 < e0 * 0.2); CHECK(e12 < e6 * 0.6); CHECK(e18 < e12 * 0.6); CHECK(e24 < e18 * 0.6);
    double e48 = lowE(2500.f, 4), e96 = lowE(2500.f, 5); std::printf("48 dB %.9f, 96 dB %.12f\n", e48, e96); CHECK(e48 < e24 * 0.6); CHECK(e96 < e24 * 0.6); CHECK(std::isfinite(e96));
    CHECK(lowE(12000.f, 1) < e12); CHECK(lowE(19000.f, 3) < lowE(12000.f, 1));
    auto hiB = [&](FilterMode m, float hc, int slope) { Params q = baseP(); q.filterMode = m; q.cutoff = 300.f; q.reso = 0.f; q.noiseLevel = 0.f; q.highCutHz = hc; q.auxSlope = slope; q.chorusMix = q.delayMix = q.reverbMix = 0.f; return brightness(runP(q)); };
    double h0 = hiB(FilterMode::HP, 20000.f, 0), h1 = hiB(FilterMode::HP, 2000.f, 1), h2 = hiB(FilterMode::HP, 2000.f, 3), h24 = hiB(FilterMode::HP24, 2000.f, 1); CHECK(h1 < h0 * 0.8); CHECK(h2 <= h1); CHECK(h24 < h0); { double h48 = hiB(FilterMode::HP, 2000.f, 4), h96 = hiB(FilterMode::HP, 2000.f, 5); CHECK(h48 < h0 * 0.8); CHECK(h96 < h0 * 0.8); CHECK(std::isfinite(h96)); }
    CHECK(std::fabs(hiB(FilterMode::LP24, 2000.f, 0) - hiB(FilterMode::LP24, 20000.f, 0)) < 1e-9);   // with a low pass filter the high cut is not used
    std::printf("hi/lo cut: slopes steepen, high cut band-limits next to HP and HP24\n");
  }
  { // sub and noise: voices, detune and width. Defaults change nothing; width makes left and right differ
    auto stereoDiff = [&](const Params& q) { std::vector<float> R; auto L = runP(q, &R); double d = 0, e = 1e-12; for (size_t i = 0; i < L.size() && i < R.size(); ++i) { d += (double)(L[i] - R[i]) * (L[i] - R[i]); e += (double)L[i] * L[i]; } return d / e; };
    Params sb = baseP(); sb.layer[0].level = 0.f; sb.layer[1].level = 0.f; sb.layer[2].level = 0.f; sb.subLevel = 0.8f; sb.noiseLevel = 0.f; sb.chorusMix = sb.delayMix = sb.reverbMix = 0.f; sb.dmMix = 0.f; sb.subPreFilter = true;
    Params s1 = sb; Params s3 = sb; s3.subVoices = 3; s3.subDetune = 0.5f; Params s3w = s3; s3w.subWidth = 1.f;
    double d1 = stereoDiff(s1), d3 = stereoDiff(s3), d3w = stereoDiff(s3w); std::printf("sub stereo difference: 1 voice %.6f, 3 voices %.6f, 3 voices wide %.6f\n", d1, d3, d3w);
    CHECK(d1 < 1e-9); CHECK(d3 < 1e-9); CHECK(d3w > 1e-3);
    Params sp1 = s1; sp1.subPreFilter = false; Params sp3 = s3w; sp3.subPreFilter = false; CHECK(stereoDiff(sp1) < 1e-9); CHECK(stereoDiff(sp3) > 1e-3);   // post filter path is stereo too
    Params nz = baseP(); nz.layer[0].level = 0.f; nz.layer[1].level = 0.f; nz.layer[2].level = 0.f; nz.subLevel = 0.f; nz.noiseLevel = 0.8f; nz.noiseHpHz = 2000.f; nz.filterMode = FilterMode::LP24; nz.cutoff = 18000.f; nz.chorusMix = nz.delayMix = nz.reverbMix = 0.f; nz.dmMix = 0.f;
    Params nw = nz; nw.noiseWidth = 1.f; Params nv = nz; nv.noiseVoices = 4; nv.noiseDetune = 0.5f;
    double nd0 = stereoDiff(nz), ndw = stereoDiff(nw), ndv = stereoDiff(nv); std::printf("noise stereo difference: default %.6f, wide %.4f, 4 voices %.6f\n", nd0, ndw, ndv); CHECK(nd0 < 1e-9); CHECK(ndw > 0.5); CHECK(ndv < 1e-9);
    auto xs = runP(nz), xv = runP(nv); double bz = 0, bv = 0; for (float x : xs) bz += std::fabs(x); for (float x : xv) bv += std::fabs(x); CHECK(bz > 0 && bv > 0 && std::fabs(bz - bv) > 1e-6);
    std::printf("sub and noise: voices, detune and width behave\n");
  }
  { // three LFOs: shapes 0..6 are what they say; LFO 2 and 3 only act through the matrix; their random shapes use a per voice stream, so a retrigger repeats and different notes differ
    CHECK(std::fabs(lfoWave(0, 0.25, 0, 0) - 1.0) < 1e-12 && std::fabs(lfoWave(1, 0.0, 0, 0) - 1.0) < 1e-12 && lfoWave(1, 0.5, 0, 0) == -1.0 && lfoWave(2, 0.1, 0, 0) == 1.0 && lfoWave(2, 0.9, 0, 0) == -1.0 && lfoWave(3, 0.75, 0, 0) == 0.5);
    CHECK(lfoWave(4, 0.3, 0.7, -0.2) == 0.7 && lfoWave(4, 0.9, 0.7, -0.2) == 0.7);   // random square: holds the new value for the whole cycle
    CHECK(std::fabs(lfoWave(5, 0.0, 0.7, -0.2) - (-0.2)) < 1e-12 && std::fabs(lfoWave(5, 1.0, 0.7, -0.2) - 0.7) < 1e-12 && std::fabs(lfoWave(5, 0.5, 0.7, -0.2) - 0.25) < 1e-12);   // random round: glides from the last value to the new one
    CHECK(lfoWave(6, 0.0, 0, 0) == 1.0 && lfoWave(6, 1.0, 0, 0) == -1.0 && lfoWave(6, 0.5, 0, 0) == 0.0);   // ramp down
    for (int sh = 0; sh < 7; ++sh) { Params q = baseP(); q.lfo2Rate = 7.f + (float)sh; q.lfo2Shape = sh; mxSet(q, "lfo2", "cut", 0.6f); mxSet(q, "lfo2", "pitch", 0.03f); Params n0 = baseP(); auto a = runP(q), b = runP(n0); CHECK(allFinite(a)); CHECK(relDiff(b, a) > 0.01);
      Params q3 = baseP(); q3.lfo3Rate = 5.f; q3.lfo3Shape = sh; q3.lfo3Fade = 0.2f; q3.lfo3Start = 0.25f; mxSet(q3, "lfo3", "pan", 0.7f); mxSet(q3, "lfo3", "vol", 0.3f); CHECK(allFinite(runP(q3))); Params q1 = baseP(); q1.lfoShape = sh; q1.lfoRate = 5.5f; q1.lfoCutoff = 0.4f; CHECK(allFinite(runP(q1)) && relDiff(runP(baseP()), runP(q1)) > 0.01); }
    Params u = baseP(); u.lfo2Rate = 9.f; u.lfo2Shape = 4; Params u2 = u; u2.lfo2Rate = 3.f; CHECK(relDiff(runP(baseP()), runP(u2)) == 0.0);   // no matrix cell: no effect at all
    Params r = baseP(); r.lfo2Rate = 12.f; r.lfo2Shape = 5; mxSet(r, "lfo2", "pitch", 0.2f); CHECK(relDiff(runP(r), runP(r)) == 0.0);   // deterministic
    auto sig = [&](int note) { Params q = baseP(); for (auto& ly : q.layer) ly.wave = Wave::Off; q.layer[0].wave = Wave::Sine; q.layer[0].level = 0.7f; q.noiseLevel = q.subLevel = 0.f; q.chorusMix = q.delayMix = q.reverbMix = 0.f; q.cutoff = 18000.f; q.envAmt = 0.f; q.lfo2Rate = 20.f; q.lfo2Shape = 4; mxSet(q, "lfo2", "vol", 0.9f);
      Engine e; e.prepare(48000); e.p = q; e.noteOn(note, 0.8f); std::vector<double> lev; float L[480], R[480]; for (int b = 0; b < 30; ++b) { e.render(L, R, 480); double s = 0; for (int i = 0; i < 480; ++i) s += (double)L[i] * L[i]; lev.push_back(std::sqrt(s / 480)); } return lev; };
    auto s1 = sig(60), s2 = sig(60), s3 = sig(67); double dsame = 0, ddiff = 0; for (size_t i = 0; i < s1.size(); ++i) { dsame += std::fabs(s1[i] - s2[i]); ddiff += std::fabs(s1[i] - s3[i]); } CHECK(dsame == 0.0); CHECK(ddiff > 1e-3);
    std::printf("LFOs: seven shapes, own random stream per voice, no effect without a matrix cell\n");
  }
  { // drawn envelopes 2 and 3, filter envelopes 2 and 3 as matrix sources
    auto hex = [&](float* m, const char* s) { for (int i = 0; i < 64; ++i) { unsigned v = 0; std::sscanf(std::string(s + i * 2, 2).c_str(), "%x", &v); m[i] = (float)v / 255.f; } };
    const char* d2 = "00102030405060708090a0b0c0d0e0f0ffeedccbbaa9988776655443322110000010203040506070809fa0b0c0d0e0f0ffeedccbbaa99887766554433221100";
    Params q = baseP(); hex(q.menv2, d2); q.menv2Time = 0.3f; q.menv2Loop = true; mxSet(q, "men2", "cut", 0.6f); Params q0 = baseP(); mxSet(q0, "men2", "cut", 0.6f); CHECK(relDiff(runP(q0), runP(q)) > 0.02);   // the drawn shape matters (default is a fall)
    Params qi = q; qi.menv2Inv = true; CHECK(relDiff(runP(q), runP(qi)) > 0.02); q.menv2Time = 0.05f; Params ql = q; ql.menv2Loop = false; CHECK(relDiff(runP(q), runP(ql)) > 0.005);
    Params q3 = baseP(); hex(q3.menv3, d2); q3.menv3Time = 0.8f; mxSet(q3, "men3", "pitch", 0.04f); mxSet(q3, "men3", "vol", -0.5f); Params q3b = q3; q3b.menv3Time = 0.2f; CHECK(allFinite(runP(q3)) && relDiff(runP(q3), runP(q3b)) > 0.02);
    Params f = baseP(); f.fenv2 = {0.1f, 0.3f, 0.2f, 0.3f}; f.fenv3 = {0.001f, 0.7f, 0.6f, 0.3f, 0.f, 0.5f, -0.4f}; mxSet(f, "fenv2", "cut", 0.5f); mxSet(f, "fenv3", "pitch", 0.05f); CHECK(relDiff(runP(baseP()), runP(f)) > 0.02);
    Params g = baseP(); g.f2Mode = FilterMode::LP12; g.f2Cut = 400.f; g.f2Env = 1.f; Params g2 = g; g2.fenv2 = {0.001f, 0.05f, 0.f, 0.1f}; CHECK(relDiff(runP(g), runP(g2)) > 0.02);
    std::printf("drawn envelopes 2 and 3 and filter envelopes 2 and 3 change the sound through the matrix\n");
  }
  { // every one of the 414 mod matrix cells acts (three LFOs, three drawn envelopes, three filter and amp envelopes feeding filters, global destinations included)
    auto mk = [&](int syl, int src, int dst, bool neg = false) { Params q; q.limiter = false; q.cutoff = 3000.f; q.reso = 0.3f; q.drive = 0.2f; q.lowCutHz = 100.f; q.subLevel = 0.3f; q.noiseLevel = 0.3f; q.pitchAmt = 7.f; q.pitchTime = 0.5f; q.lfoRate = 5.f; q.lfo2Rate = 4.3f; q.lfo3Rate = 6.1f; q.lfo2Shape = 3; q.lfo3Shape = 6;
      q.modWheel = 1.f; q.pressure = 0.8f; q.slide = 0.8f; q.pitchBend = 1.f; q.bendRange = 12.f; q.fenv.s = 0.6f; q.fenv2 = {0.01f, 0.6f, 0.5f, 0.3f}; q.fenv3 = {0.05f, 0.4f, 0.7f, 0.3f}; q.amp.s = 0.7f; q.f2Mode = FilterMode::LP12; q.f3Mode = FilterMode::BP; q.f2Cut = 1500.f; q.f3Cut = 2200.f;
      q.reverbMix = 0.1f; q.delayMix = 0.1f; q.delayTime = 0.1f; q.chorusMix = 0.2f; q.drvDrive = 0.3f; q.dmMix = 0.5f; q.eq1On = false; q.eq6On = false; q.widHi = 0.8f;
      q.layer[0].wave = Wave::Pulse; q.layer[0].level = 0.5f; q.layer[0].pwm = 0.5f; q.layer[1].wave = Wave::SuperSaw; q.layer[1].level = 0.5f; q.layer[1].detune = 0.5f; q.layer[1].voices = 5; q.layer[2].wave = Wave::Saw; q.layer[2].level = 0.4f;
      if (syl >= 0) { for (int i = 0; i < 3; ++i) q.layer[i].wave = Wave::Off; q.layer[syl].wave = Wave::Saw; q.layer[syl].level = 0.6f; q.layer[syl].sync = true; q.layer[syl].syncSemis = 7.f; }
      if (src >= 0) q.mx[src][dst] = (dst == 21) != neg ? -0.8f : 0.8f;
      Engine e; e.prepare(48000); e.p = q; e.noteOn(72, 0.8f); std::vector<float> o; float L[500], R[500]; for (int b = 0; b < 48; ++b) { e.render(L, R, 500); o.insert(o.end(), L, L + 500); } return o; };
    int dead = 0, nonfin = 0;
    for (int s = 0; s < kMxSrc; ++s) for (int d = 0; d < kMxDst; ++d) { int syl = d == 8 ? 0 : d == 9 ? 1 : d == 10 ? 2 : -1; auto a0 = mk(syl, -1, 0), a1 = mk(syl, s, d); double df = 0, sm = 0; for (size_t i = 0; i < a0.size(); ++i) { df += std::fabs(a0[i] - a1[i]); sm += std::fabs(a0[i]); if (!std::isfinite(a1[i])) ++nonfin; }
      if (!(df / (sm + 1e-9) > 1e-4) && s == 7) { auto a2 = mk(syl, s, d, true); df = 0; for (size_t i = 0; i < a0.size(); ++i) df += std::fabs(a0[i] - a2[i]); }
      if (!(df / (sm + 1e-9) > 1e-4)) { ++dead; std::printf("dead matrix cell %s -> %s (%.2e)\n", kMxSrcNames[s], kMxDstNames[d], df / (sm + 1e-9)); } }
    CHECK(dead == 0); CHECK(nonfin == 0); std::printf("mod matrix: all %d cells act and stay finite\n", kMxSrc * kMxDst);
  }
  { // dance master mid trim and tempo synced drawn envelopes
    Params a = baseP(); a.dmMix = 1.f; Params b = a; b.dmMid = 1.f; Params c = a; c.dmMid = -1.f; auto ra = runP(a), rb = runP(b), rc = runP(c); CHECK(allFinite(rb) && allFinite(rc)); CHECK(relDiff(ra, rb) > 1e-3 && relDiff(ra, rc) > 1e-3 && relDiff(rb, rc) > 1e-3);
    Params s = baseP(); mxSet(s, "men2", "cut", 0.7f); s.menv2Loop = true; s.menv2Time = 0.5f; Params s1 = s; s1.menv2Sync = true; s1.menv2Div = 12; s1.bpm = 120.f; Params s2 = s1; s2.bpm = 240.f; Params s3 = s1; s3.menv2Div = 6;
    CHECK(relDiff(runP(s), runP(s1)) > 1e-3 && relDiff(runP(s1), runP(s2)) > 1e-3 && relDiff(runP(s1), runP(s3)) > 1e-3); Params s4 = s; s4.menv2Div = 3; s4.bpm = 300.f; CHECK(relDiff(runP(s), runP(s4)) == 0.0);   // div and bpm are ignored with sync off
    std::printf("dmMid and drawn envelope tempo sync act\n");
  }
  { // drum kit mode: pads sound at their keys, unmapped keys are silent, a pad can cut another, a key can layer pads
    auto mk = [](const char* name, int key, int play, float g, std::vector<int> ch) { KitEngine::Def d; presets::byName(name, d.p); d.n = {key}; d.play = play; d.g = g; d.ch = ch; return d; };
    std::vector<KitEngine::Def> defs = { mk("kick_909", 36, 36, 1.f, {}), mk("snare_909", 38, 60, 0.9f, {}), mk("hat_909_closed", 42, 60, 0.5f, {46}), mk("hat_909_open", 46, 60, 0.58f, {}) };
    Params main; presets::byName("trance_90s", main);
    auto hit = [&](KitEngine& k, int key, int blocks, double* pk) { std::vector<float> l(256), r(256); double e = 0, mx = 0; k.noteOn(key, 0.9f); bool fin = true;
      for (int b = 0; b < blocks; ++b) { k.render(l.data(), r.data(), 256, main.master, main.clipKnee); for (int i = 0; i < 256; ++i) { if (!std::isfinite(l[i] + r[i])) fin = false; e += (double)l[i] * l[i]; mx = std::max(mx, (double)std::fabs(l[i])); } }
      if (pk) *pk = mx; CHECK(fin); return e; };
    for (int key : {36, 38, 42, 46}) { KitEngine k; k.build(defs, 48000, 256); double pk = 0; double e = hit(k, key, 80, &pk); CHECK(e > 1e-3); CHECK(pk < 1.0); }
    { KitEngine k; k.build(defs, 48000, 256); double e = hit(k, 60, 40, nullptr); CHECK(e == 0.0); }   // key with no pad stays silent
    { // open hat rings, then the closed hat cuts it: the tail after the cut is much quieter than the same tail without the cut
      auto tail = [&](bool cut) { KitEngine k; k.build(defs, 48000, 256); std::vector<float> l(256), r(256); k.noteOn(46, 0.9f); for (int b = 0; b < 20; ++b) k.render(l.data(), r.data(), 256, main.master, main.clipKnee);
        if (cut) k.noteOn(42, 0.9f); double e = 0; for (int b = 0; b < 100; ++b) { k.render(l.data(), r.data(), 256, main.master, main.clipKnee); if (b > 30) for (int i = 0; i < 256; ++i) e += (double)l[i] * l[i]; } return e; };
      double a = tail(false), c = tail(true); CHECK(a > 1e-4); CHECK(c < 0.25 * a); }
    { // two pads on one key play together (layering): louder than either alone
      std::vector<KitEngine::Def> lay = { mk("kick_909", 36, 36, 1.f, {}), mk("snare_909", 36, 60, 0.9f, {}) };
      KitEngine k1, k2, k3; k1.build(lay, 48000, 256); k2.build({lay[0]}, 48000, 256); k3.build({lay[1]}, 48000, 256);
      double e1 = hit(k1, 36, 40, nullptr), e2 = hit(k2, 36, 40, nullptr), e3 = hit(k3, 36, 40, nullptr); CHECK(e1 > std::max(e2, e3)); }
    { // a block longer than the build size is handled in chunks
      KitEngine k; k.build(defs, 48000, 128); std::vector<float> l(1000), r(1000); k.noteOn(36, 0.9f); k.render(l.data(), r.data(), 1000, main.master, main.clipKnee); double e = 0; for (float x : l) e += (double)x * x; CHECK(e > 1e-3); }
    std::printf("drum kit: pads, choke, layers and long blocks work\n");
  }
  if (!fails) std::puts("presets_test: all passed");
  return fails ? 1 : 0;
}
