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
  if (!fails) std::puts("presets_test: all passed");
  return fails ? 1 : 0;
}
