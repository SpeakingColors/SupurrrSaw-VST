// Plays a .mid file through two presets (split by note) and writes a WAV.
// Usage: midi_demo in.mid out.wav bpm splitNote lowPreset highPreset [transposeHigh [transposeLow]]
//   notes below splitNote use lowPreset, the rest use highPreset ("none" silences a part).
#include "../presets/presets.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iterator>
using namespace supurr;
struct Ev { double beat; int note; bool on; int vel; };
static bool readMidi(const char* fn, std::vector<Ev>& out) {
  std::ifstream f(fn, std::ios::binary); if (!f) return false; std::vector<unsigned char> b((std::istreambuf_iterator<char>(f)), {});
  auto u32 = [&](size_t i) { return (uint32_t)b[i] << 24 | b[i+1] << 16 | b[i+2] << 8 | b[i+3]; }; auto u16 = [&](size_t i) { return b[i] << 8 | b[i+1]; };
  if (b.size() < 14 || memcmp(b.data(), "MThd", 4)) return false; int ntr = u16(10), tpb = u16(12); size_t i = 8 + u32(4);
  for (int t = 0; t < ntr && i + 8 <= b.size(); ++t) {
    uint32_t len = u32(i + 4); size_t p = i + 8, end = p + len; double tick = 0; int run = 0;
    while (p < end) {
      uint32_t d = 0; while (p < end) { unsigned char c = b[p++]; d = d << 7 | (c & 0x7f); if (!(c & 0x80)) break; } tick += d;
      unsigned char st = b[p]; if (st == 0xff) { p += 2; uint32_t l = 0; while (p < end) { unsigned char c = b[p++]; l = l << 7 | (c & 0x7f); if (!(c & 0x80)) break; } p += l; continue; }
      if (st == 0xf0 || st == 0xf7) { ++p; uint32_t l = 0; while (p < end) { unsigned char c = b[p++]; l = l << 7 | (c & 0x7f); if (!(c & 0x80)) break; } p += l; continue; }
      if (st & 0x80) { run = st; ++p; }
      int type = run & 0xf0, n1 = b[p++], n2 = (type == 0xc0 || type == 0xd0) ? 0 : b[p++];
      if (type == 0x90 && n2 > 0) out.push_back({tick / tpb, n1, true, n2}); else if (type == 0x80 || type == 0x90) out.push_back({tick / tpb, n1, false, 0});
    }
    i = end;
  }
  std::stable_sort(out.begin(), out.end(), [](const Ev& a, const Ev& c) { return a.beat < c.beat; }); return true;
}
int main(int argc, char** argv) {
  if (argc < 7) { std::puts("usage: midi_demo in.mid out.wav bpm splitNote lowPreset highPreset [transposeHigh [transposeLow]]"); return 1; }
  std::vector<Ev> ev; if (!readMidi(argv[1], ev) || ev.empty()) { std::puts("no notes in the file"); return 2; }
  double bpm = atof(argv[3]); int split = atoi(argv[4]); int trH = argc > 7 ? atoi(argv[7]) : 0, trL = argc > 8 ? atoi(argv[8]) : 0; const int sr = 48000;
  Engine eng[2]; bool use[2] = {true, true}; for (int k = 0; k < 2; ++k) { std::string n = argv[5 + k]; if (n == "none") use[k] = false; else if (!presets::byName(n, eng[k].p)) { std::printf("unknown preset %s\n", n.c_str()); return 3; } eng[k].prepare(sr); }
  double endBeat = ev.back().beat + 2.0; size_t N = (size_t)(endBeat * 60.0 / bpm * sr), si = 0; std::vector<float> L, R; L.reserve(N); R.reserve(N);
  float lb[2][64], rb[2][64];
  for (size_t pos = 0; pos < N; pos += 64) {
    double beat = (double)pos / sr * bpm / 60.0;
    while (si < ev.size() && ev[si].beat <= beat) { int k = ev[si].note < split ? 0 : 1; if (use[k]) { int tr = k ? trH : trL; if (ev[si].on) eng[k].noteOn(ev[si].note + tr, ev[si].vel / 127.f); else eng[k].noteOff(ev[si].note + tr); } ++si; }
    for (int k = 0; k < 2; ++k) { if (use[k]) eng[k].render(lb[k], rb[k], 64); else for (int i = 0; i < 64; ++i) lb[k][i] = rb[k][i] = 0; }
    for (int i = 0; i < 64; ++i) { L.push_back(lb[0][i] + lb[1][i]); R.push_back(rb[0][i] + rb[1][i]); }
  }
  double pk = 0; for (size_t i = 0; i < L.size(); ++i) pk = std::max(pk, (double)std::max(std::fabs(L[i]), std::fabs(R[i]))); float g = pk > 0.95 ? (float)(0.95 / pk) : 1.f;
  FILE* f = fopen(argv[2], "wb"); int n = (int)L.size(), sr2 = sr, bytes = n * 4;
  fwrite("RIFF", 1, 4, f); int c = 36 + bytes; fwrite(&c, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); int s16 = 16; short pcm = 1, ch = 2, bps = 16, ba = 4; int br = sr2 * 4;
  fwrite(&s16, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr2, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bps, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
  for (int i = 0; i < n; ++i) { short a = (short)(L[i] * g * 30000), d = (short)(R[i] * g * 30000); fwrite(&a, 2, 1, f); fwrite(&d, 2, 1, f); }
  fclose(f); std::printf("%s: %d notes events, %.1f s, peak %.2f\n", argv[2], (int)ev.size(), n / (double)sr, pk); return 0;
}
