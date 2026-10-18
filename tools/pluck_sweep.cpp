// Renders the Pluck preset eight times with the filter going from fully closed to fully open.
// Usage: pluck_sweep out.wav
#include "../presets/presets.h"
#include <cstdio>
using namespace supurr;
int main(int argc, char** argv) {
  Engine e; presets::pluck(e.p); e.prepare(48000); std::vector<float> L, R; float lb[64], rb[64];
  const float cuts[8] = {40.f, 150.f, 400.f, 700.f, 1500.f, 3500.f, 9000.f, 18000.f};
  for (int k = 0; k < 8; ++k) {
    e.p.cutoff = cuts[k]; e.noteOn(69, 0.9f);
    for (int pos = 0; pos < 48000; pos += 64) { if (pos == 9600) e.noteOff(69); e.render(lb, rb, 64); for (int i = 0; i < 64; ++i) { L.push_back(lb[i]); R.push_back(rb[i]); } }
  }
  FILE* f = fopen(argc > 1 ? argv[1] : "pluck_sweep.wav", "wb"); int n = (int)L.size(), sr = 48000, b = n * 4;
  fwrite("RIFF", 1, 4, f); int c = 36 + b; fwrite(&c, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); int s16 = 16; short pcm = 1, ch = 2, bps = 16, ba = 4; int br = sr * 4;
  fwrite(&s16, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bps, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&b, 4, 1, f);
  for (int i = 0; i < n; ++i) { short a = (short)(std::max(-1.f, std::min(1.f, L[i])) * 30000), d = (short)(std::max(-1.f, std::min(1.f, R[i])) * 30000); fwrite(&a, 2, 1, f); fwrite(&d, 2, 1, f); }
  fclose(f); return 0;
}
