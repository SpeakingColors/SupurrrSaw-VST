// Param-driven renderer for matching reference sounds. Usage: tune <acid|hoover> out.wav key=value ...
#include "../core/supurrsaw_core.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
using namespace supurr;
static void writeWav(const char* fn, const std::vector<float>& l, const std::vector<float>& r) {
  FILE* f = fopen(fn, "wb"); int n = (int)l.size(), sr = 48000, b = n * 4;
  fwrite("RIFF", 1, 4, f); int c = 36 + b; fwrite(&c, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); int s16 = 16; short pcm = 1, ch = 2, bps = 16, ba = 4; int br = sr * 4;
  fwrite(&s16, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bps, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&b, 4, 1, f);
  for (int i = 0; i < n; ++i) { short a = (short)(std::max(-1.f, std::min(1.f, l[i])) * 30000), d = (short)(std::max(-1.f, std::min(1.f, r[i])) * 30000); fwrite(&a, 2, 1, f); fwrite(&d, 2, 1, f); } fclose(f);
}
int main(int argc, char** argv) {
  std::string kind = argv[1]; const char* out = argv[2]; std::map<std::string, float> k;
  for (int i = 3; i < argc; ++i) { std::string a = argv[i]; auto e = a.find('='); k[a.substr(0, e)] = (float)atof(a.substr(e + 1).c_str()); }
  auto G = [&](const char* n, float d) { return k.count(n) ? k[n] : d; };
  Engine e; Params& p = e.p; struct N { int n; float at, len; }; std::vector<N> seq; float total = 0;
  if (kind == "acid") {
    p.layer[0].wave = G("wave", 0) > 0.5f ? Wave::Pulse : Wave::Saw; p.layer[0].pwm = G("pw", 0.5f);
    p.filterMode = G("m18", 1) > 0.5f ? FilterMode::LP18 : FilterMode::LP24; p.cutoff = G("cut", 600); p.reso = G("reso", 0.8f); p.envAmt = G("env", 0.6f); p.fenv = {0.001f, G("fdec", 0.2f), 0.f, 0.1f};
    p.amp = {0.001f, G("adec", 0.3f), G("asus", 0.7f), 0.05f}; p.subLevel = G("sub", 0.f); p.spread = 0.f; p.drive = G("drive", 0.5f); p.dist = G("dist", 0.7f); p.lowCutHz = G("lc", 35); p.master = G("master", 0.55f);
    int root = (int)G("root", 44); int pat[] = {0, 0, 12, 0, 3, 0, 0, 7, 0, 12, 0, 0, 10, 0, 3, 0}; for (int i = 0; i < 16; ++i) seq.push_back({root + pat[i], i * 0.1f, 0.085f}); total = 1.8f;
  } else {
    int nl = (int)G("layers", 3); for (int l = 0; l < nl; ++l) { p.layer[l].wave = G("saw", 1) > 0.5f ? Wave::SuperSaw : Wave::Pulse; p.layer[l].voices = (int)G("voices", 7); p.layer[l].detune = G("det", 0.7f); p.layer[l].octave = (int)G("oct", 0) + (l == 1 ? (int)G("oct1", 0) : 0) + (l == 2 ? (int)G("oct2", 0) : 0); p.layer[l].level = G("lvl", 0.4f); p.layer[l].pwm = 0.5f; }
    p.detuneRange = G("range", 1.5f); p.lfoRate = G("lfo", 5.f); p.lfoPwm = G("lfopwm", 0.8f); p.lfoCutoff = G("lfocut", 0.3f); p.pitchFall = G("fall", 4.f); p.pitchFallTime = G("falltime", 1.0f);
    p.cutoff = G("cut", 2500); p.reso = G("reso", 0.4f); p.drive = G("drive", 0.2f); p.dist = G("dist", 0.2f); p.lowCutHz = G("lc", 40); p.spread = G("spread", 1.f); p.amp = {0.005f, 1.0f, 0.9f, 0.3f}; p.subLevel = G("sub", 0.f); p.master = G("master", 0.6f);
    int root = (int)G("root", 43); seq = {{root, 0.f, 1.5f}, {root + 5, 2.f, 1.5f}, {root - 2, 4.f, 1.5f}}; total = 6.f;
  }
  e.prepare(48000); std::vector<float> L, R; size_t N = (size_t)(total * 48000), si = 0; std::vector<std::pair<size_t, int>> offs; float lb[64], rb[64];
  for (size_t pos = 0; pos < N; pos += 64) {
    while (si < seq.size() && seq[si].at * 48000 <= pos) { e.noteOn(seq[si].n, 0.9f); offs.push_back({(size_t)((seq[si].at + seq[si].len) * 48000), seq[si].n}); ++si; }
    for (auto it = offs.begin(); it != offs.end();) { if (it->first <= pos) { e.noteOff(it->second); it = offs.erase(it); } else ++it; }
    e.render(lb, rb, 64); for (int i = 0; i < 64; ++i) { L.push_back(lb[i]); R.push_back(rb[i]); }
  }
  writeWav(out, L, R); return 0;
}
