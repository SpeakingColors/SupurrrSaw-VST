// Offline renderer + checks for the SupurrSaw core. Usage: demo [vec1.wav]
#include "../core/supurrsaw_core.h"
#include <cstdio>
#include <cstring>
using namespace supurr;
static std::shared_ptr<Sample> loadWav(const char* fn, int root) {
  FILE* f = fopen(fn, "rb"); if (!f) return nullptr;
  std::vector<unsigned char> b; unsigned char tmp[65536]; size_t n; while ((n = fread(tmp, 1, sizeof tmp, f)) > 0) b.insert(b.end(), tmp, tmp + n); fclose(f);
  size_t i = 12; int ch = 2, bits = 16; bool isFloat = false; uint32_t sr = 44100; auto sm = std::make_shared<Sample>();
  while (i + 8 <= b.size()) {
    uint32_t sz = b[i+4] | b[i+5] << 8 | b[i+6] << 16 | (uint32_t)b[i+7] << 24;
    if (!memcmp(&b[i], "fmt ", 4)) { ch = b[i+10] | b[i+11] << 8; sr = b[i+12] | b[i+13] << 8 | b[i+14] << 16 | (uint32_t)b[i+15] << 24; bits = b[i+22] | b[i+23] << 8; isFloat = (b[i+8] | b[i+9] << 8) == 3; }
    if (!memcmp(&b[i], "data", 4) && (bits == 16 || bits == 24 || bits == 32)) {
      int bps = bits / 8; size_t frames = std::min<size_t>(sz, b.size() - i - 8) / (bps * ch);
      auto rd = [&](const unsigned char* q) -> float {
        if (bits == 16) return (int16_t)(q[0] | q[1] << 8) / 32768.f;
        if (bits == 24) { int32_t v = (q[0] << 8 | q[1] << 16 | q[2] << 24); return (v >> 8) / 8388608.f; }
        if (isFloat) { float fv; memcpy(&fv, q, 4); return fv; }
        return (int32_t)(q[0] | q[1] << 8 | q[2] << 16 | (uint32_t)q[3] << 24) / 2147483648.f; };
      for (size_t k = 0; k < frames; ++k) { const unsigned char* q = &b[i + 8 + k * bps * ch];
        float a = rd(q), c = ch > 1 ? rd(q + bps) : a; sm->l.push_back(a); sm->r.push_back(c); }
    }
    i += 8 + sz + (sz & 1);
  }
  sm->sampleRate = sr; sm->rootNote = root; return sm->l.empty() ? nullptr : sm;
}
struct Verb {  // small Freeverb-style reverb for listening tests only
  struct Comb { std::vector<float> b; size_t i = 0; float s = 0; float run(float x, float fb, float d) { float o = b[i]; s = o * (1 - d) + s * d; b[i] = x + s * fb; if (++i >= b.size()) i = 0; return o; } };
  struct AP { std::vector<float> b; size_t i = 0; float run(float x) { float o = b[i]; float y = -x + o; b[i] = x + o * 0.5f; if (++i >= b.size()) i = 0; return y; } };
  Comb c[2][4]; AP a[2][2];
  Verb() { int cl[4] = {1116, 1188, 1277, 1356}, al[2] = {556, 441};
    for (int ch = 0; ch < 2; ++ch) { for (int k = 0; k < 4; ++k) c[ch][k].b.assign(cl[k] + ch * 23, 0.f); for (int k = 0; k < 2; ++k) a[ch][k].b.assign(al[k] + ch * 23, 0.f); } }
  void run(float& l, float& r, float mix, float fb) { float in[2] = {l, r}, o[2];
    for (int ch = 0; ch < 2; ++ch) { float s = 0; for (int k = 0; k < 4; ++k) s += c[ch][k].run(in[ch] * 0.3f, fb, 0.3f); s = a[ch][0].run(s); s = a[ch][1].run(s); o[ch] = s; }
    l = l * (1 - mix) + o[0] * mix; r = r * (1 - mix) + o[1] * mix; }
};
static void writeWav(const char* fn, const std::vector<float>& l, const std::vector<float>& r) {
  FILE* f = fopen(fn, "wb"); int n = (int)l.size(), sr = 48000, b = n * 4;
  fwrite("RIFF", 1, 4, f); int c = 36 + b; fwrite(&c, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); int s16 = 16; short pcm = 1, ch = 2, bps = 16, ba = 4; int br = sr * 4;
  fwrite(&s16, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bps, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&b, 4, 1, f);
  for (int i = 0; i < n; ++i) { short a = (short)(std::max(-1.f, std::min(1.f, l[i])) * 30000), d = (short)(std::max(-1.f, std::min(1.f, r[i])) * 30000); fwrite(&a, 2, 1, f); fwrite(&d, 2, 1, f); } fclose(f);
}
struct Note { int n; float at, len; };
static void play(const char* name, Engine& e, const std::vector<Note>& seq, float total, float verbMix = 0.f) {
  const int sr = 48000; e.prepare(sr); std::vector<float> L, R; Verb vb; size_t N = (size_t)(total * sr); L.reserve(N); R.reserve(N);
  size_t si = 0; std::vector<std::pair<size_t,int>> offs; float lb[64], rb[64];
  double sumL = 0, sumR = 0, sumM = 0, sumLR = 0, peak = 0; bool bad = false;
  for (size_t pos = 0; pos < N; pos += 64) {
    while (si < seq.size() && seq[si].at * sr <= pos) { e.noteOn(seq[si].n, 0.9f); offs.push_back({(size_t)((seq[si].at + seq[si].len) * sr), seq[si].n}); ++si; }
    for (auto it = offs.begin(); it != offs.end();) { if (it->first <= pos) { e.noteOff(it->second); it = offs.erase(it); } else ++it; }
    e.render(lb, rb, 64);
    for (int i = 0; i < 64; ++i) { float l = lb[i], r = rb[i]; if (verbMix > 0) vb.run(l, r, verbMix, 0.82f);
      if (l != l || r != r) bad = true; peak = std::max(peak, (double)std::max(std::fabs(l), std::fabs(r)));
      sumL += l * l; sumR += r * r; sumM += 0.25 * (l + r) * (l + r); sumLR += l * r; L.push_back(l); R.push_back(r); }
  }
  double corr = sumLR / std::sqrt(sumL * sumR + 1e-12), monoDb = 10 * std::log10((sumM + 1e-12) / (0.5 * (sumL + sumR) + 1e-12) );
  printf("%-22s peak %.2f  corr %+.2f  mono-sum vs stereo %+.1f dB %s\n", name, peak, corr, monoDb, bad ? "NaN!" : "ok");
  char fn[128]; snprintf(fn, sizeof fn, "%s.wav", name); writeWav(fn, L, R);
}
int main(int argc, char** argv) {
  auto vec = argc > 1 ? loadWav(argv[1], 66) : nullptr;  // F#4
  if (vec) vec->oneShot = true;
  std::vector<Note> riff; int mel[] = {69, 69, 72, 76, 74, 72, 71, 72, 77, 76, 74, 72, 71, 72, 74, 76}; for (int k = 0; k < 16; ++k) riff.push_back({mel[k], k * 0.30f, 0.22f});
  { // 1. Ah stab: the sample IS the sound. A little supersaw underneath for body. Filter wide open.
    Engine e; Params& p = e.p;
    if (vec) { p.layer[0].wave = Wave::Sample; p.layer[0].sample = vec; p.layer[0].level = 1.0f; }
    else { p.layer[0].wave = Wave::SuperSquare; p.layer[0].level = 0.7f; }
    p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 7; p.layer[1].detune = 0.35f; p.layer[1].level = 0.12f;
    p.cutoff = 18000; p.reso = 0.f; p.lowCutHz = 60; p.amp = {0.001f, 1.2f, 0.f, 0.3f}; p.spread = 1.f;
    play("ah_stab_classic", e, riff, 5.6f);
  }
  { // 2. Hoover, an octave higher, low rumble cut, less drive
    Engine e; Params& p = e.p;
    for (int l = 0; l < 3; ++l) { p.layer[l].wave = Wave::Pulse; p.layer[l].octave = l; p.layer[l].level = 0.45f; }
    p.lfoRate = 6.5f; p.lfoPwm = 0.9f; p.lfoPitch = 0.1f; p.pitchAmt = 12.f; p.pitchTime = 0.09f; p.cutoff = 2600; p.reso = 0.45f; p.drive = 0.1f; p.dist = 0.1f; p.lowCutHz = 150.f;
    p.amp = {0.002f, 0.5f, 0.8f, 0.25f}; p.spread = 0.8f;
    play("hoover", e, {{57, 0.f, 0.9f}, {60, 1.f, 0.9f}, {55, 2.f, 1.2f}}, 4.0f, 0.12f);
  }
  { // 3. mono-compatibility sweep: worst case settings across the keyboard
    double worst = 0; for (float d : {0.f, 0.5f, 1.f}) for (float sp : {0.7f, 1.f}) for (int note : {36, 48, 60, 72}) {
      Engine e; Params& p = e.p; p.layer[0].wave = Wave::SuperSaw; p.layer[0].detune = d; p.layer[0].voices = 15; p.spread = sp; p.detuneRange = 1.f; p.cutoff = 12000; p.amp = {0.002f, 2.f, 1.f, 0.3f};
      e.prepare(48000); e.noteOn(note, 0.9f); float l[64], r[64]; double sL = 0, sR = 0, sM = 0;
      for (int k = 0; k < 750; ++k) { e.render(l, r, 64); for (int i = 0; i < 64; ++i) { sL += l[i] * l[i]; sR += r[i] * r[i]; sM += 0.25 * (l[i] + r[i]) * (l[i] + r[i]); } }
      double db = 10 * std::log10(sM / (0.5 * (sL + sR) + 1e-12)); worst = std::min(worst, db); }
    printf("worst mono-sum loss over 15-voice detune/spread/note sweep: %.2f dB\n", worst);
  }
  { // 4. Reese bass, clean and dirty
    for (int dirty = 0; dirty < 2; ++dirty) { Engine e; Params& p = e.p;
      if (!dirty) { p.layer[0].wave = Wave::Saw; p.layer[1].wave = Wave::Saw; p.layer[1].tune = 14.f; p.layer[0].tune = -14.f; p.layer[0].level = p.layer[1].level = 0.5f; p.layer[0].pan = -0.6f; p.layer[1].pan = 0.6f; p.cutoff = 500; p.reso = 0.3f; p.subLevel = 0.5f; }
      else { p.layer[0].wave = Wave::SuperSaw; p.layer[0].voices = 5; p.layer[0].detune = 0.3f; p.layer[1].wave = Wave::SuperSaw; p.layer[1].voices = 5; p.layer[1].detune = 0.3f; p.layer[1].tune = 19.f; p.layer[2].wave = Wave::Saw; p.layer[2].tune = -19.f;
        for (auto& l : p.layer) l.level = 0.4f; p.cutoff = 700; p.reso = 0.4f; p.drive = 0.4f; p.dist = 0.55f; p.lfoRate = 0.35f; p.lfoCutoff = 0.35f; p.subLevel = 0.35f; p.detuneRange = 1.5f; }
      p.bassMonoHz = 150; p.amp = {0.005f, 0.5f, 0.9f, 0.2f};
      play(dirty ? "reese_dirty" : "reese_clean", e, {{36, 0.f, 1.5f}, {39, 1.6f, 1.5f}}, 3.4f); }
  }
  { // 5. Acid: saw, LP18, high resonance, hard distorted
    Engine e; Params& p = e.p; p.layer[0].wave = Wave::Saw; p.filterMode = FilterMode::LP18; p.cutoff = 450; p.reso = 0.85f; p.envAmt = 0.75f; p.fenv = {0.001f, 0.2f, 0.f, 0.1f}; p.amp = {0.001f, 0.3f, 0.7f, 0.05f}; p.subLevel = 0.25f; p.spread = 0.f;
    p.drive = 0.6f; p.dist = 0.75f; p.lowCutHz = 35.f; p.master = 0.55f;
    std::vector<Note> sq; int nn[] = {36, 36, 48, 39, 36, 36, 43, 46, 36, 48, 36, 51, 36, 43, 39, 48}; for (int k = 0; k < 16; ++k) sq.push_back({nn[k], k * 0.15f, 0.12f});
    play("acid_dirty", e, sq, 2.6f, 0.08f);
  }
  return 0;
}
