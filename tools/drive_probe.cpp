// Measures how loud a preset is as the filter Drive goes 0 -> 1 (pre-clip RMS and peak). Used to level-match the drive stage.
#include "../presets/presets.h"
#include <cstdio>
using namespace supurr;
int main() {
  const char* names[] = {"hoover", "hoover_classic", "trance_lead", "talking_lead", "daft_funk", "trance_bass", "access_bass", "pendulum_lead", "sandstorm_lead", "pluck"};
  const int notes[] = {57, 57, 69, 64, 64, 36, 40, 69, 69, 69};
  double tot[5] = {0}; int cnt = 0;
  for (int k = 0; k < 10; ++k) {
    std::printf("%-15s", names[k]);
    double base = 0, basePk = 0;
    for (int di = 0; di < 5; ++di) {
      Params q; presets::byName(names[k], q); q.master = 0.01f; q.limiter = false; q.chorusMix = q.delayMix = q.reverbMix = 0; q.drive = di * 0.25f; q.dist = 0.f;
      Engine e; e.prepare(48000); e.p = q; e.noteOn(notes[k], 0.9f); float L[64], R[64]; double s2 = 0, pk = 0; int n = 0;
      for (int b = 0; b < 1500; ++b) { e.render(L, R, 64); if (b > 300) for (int i = 0; i < 64; ++i) { s2 += (double)L[i] * L[i]; pk = std::max(pk, (double)std::fabs(L[i])); ++n; } }
      double rms = std::sqrt(s2 / n) * 100, peak = pk * 100; if (di == 0) { base = rms; basePk = peak; }
      std::printf("  d%.2f rms %+5.1f dB pk %+5.1f dB", q.drive, 20 * std::log10(rms / base), 20 * std::log10(peak / basePk));
      tot[di] += 20 * std::log10(rms / base);
    }
    ++cnt; std::printf("\n");
  }
  std::printf("mean rms change:"); for (int di = 0; di < 5; ++di) std::printf("  d%.2f %+5.2f dB", di * 0.25, tot[di] / cnt); std::printf("\n");
}
