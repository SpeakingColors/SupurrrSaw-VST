// Measures the pre-clip peak of every preset (master = 1, limiter off, knee 1) and prints the master that gives a 0.6 peak.
#include "../presets/presets.h"
#include <cstdio>
#include <cmath>
#include <string>
using namespace supurr;
int main() {
  const char* names[] = {"hoover","hoover_classic","hoover_prodigy","pick","pluck","trance_lead","talking_lead","daft_funk","trance_bass","funk_bass","freeway_bass","better_off_alone","gimme_lead","levels_lead","sandstorm_lead","gigi_lead","alexf_lead","mgmt_lead","pendulum_lead","nightcall_keys","brejcha_bass","feellove_bass","nboy_303","spaghetti_bass","access_bass"};
  for (const char* n : names) {
    Params p; presets::byName(n, p);
    std::string s = n; int note = s.find("bass") != std::string::npos || s == "nboy_303" ? 36 : 60;
    p.master = 0.01f; p.limiter = false;
    Engine e; e.prepare(48000); e.p = p;
    e.noteOn(note, 0.8f); float L[512], R[512], pk = 0;
    for (int b = 0; b < 190; ++b) { e.render(L, R, 512); if (b == 150) e.noteOff(note); for (int i = 0; i < 512; ++i) pk = std::max(pk, 100.f * std::max(std::fabs(L[i]), std::fabs(R[i]))); }
    std::printf("%s %.3f %.3f\n", n, pk, 0.6f / pk);
  }
}
