// Same checks as sampleset_test.cpp, for the browser engine: nearest zone, in tune (with the recording's tune offset), loops sustain, one-shots end.
// node tools/sampleset_test.js   (run from the repo root; uses the engine in playground/engine.js when present)
const path = require("path"), fs = require("fs");
const cand = [path.join(__dirname, "..", "playground", "engine.js"), "/home/claude/playground/engine.js"].find(fs.existsSync);
const { SupurrEngine, defaultParams } = require(cand);
function sine(root, tune, secs, loop) {
  const sr = 44100, f = 440 * Math.pow(2, (root - 69 + tune / 100) / 12), n = Math.floor(secs * sr), l = new Float32Array(n);
  for (let i = 0; i < n; i++) l[i] = 0.5 * Math.sin(2 * Math.PI * f * i / sr);
  const cyc = sr / f, ls = Math.floor(cyc * Math.floor(0.05 * sr / cyc)), le = ls + Math.round(cyc * Math.floor(0.2 * sr / cyc));
  return { l, sr, root, tune, loop, ls: loop ? ls : 0, le: loop ? le : 0 };
}
function freqOf(x, sr) { let first = -1, last = -1, n = 0; for (let i = 1; i < x.length; i++) if (x[i - 1] < 0 && x[i] >= 0) { const t = (i - 1 + (-x[i - 1]) / (x[i] - x[i - 1])) / sr; if (first < 0) first = t; last = t; n++; } return n > 2 ? (n - 1) / (last - first) : 0; }
function mk(zones) {
  const e = new SupurrEngine(48000), p = defaultParams(); p.layers = p.layers || [];
  const L = p.layer || p.layers; L[0].wave = "Sample"; L[0].instrument = "t"; L[0].level = 1; L[1].wave = "Off"; L[2].wave = "Off";
  p.cutoff = 20000; p.reso = 0; p.lowCutHz = 20; p.amp = Object.assign({}, p.amp, { a: 0.001, d: 0.2, s: 1, r: 0.2 }); p.spread = 0; p.subLevel = 0;
  e.setParams(p); e.setInstrument("t", { zones }); return e;
}
function run(e, note, secs, skip) { e.noteOn(note, 1); const out = []; const l = new Float32Array(64), r = new Float32Array(64); const nb = Math.floor(secs * 48000 / 64);
  for (let b = 0; b < nb; b++) { e.render(l, r, 64); if (b * 64 >= skip * 48000) for (let i = 0; i < 64; i++) out.push(l[i]); } e.allOff(); return out; }
const rms = (x, a, b) => { let s = 0; for (let i = a; i < b && i < x.length; i++) s += x[i] * x[i]; return Math.sqrt(s / (b - a)); };
let fails = 0; const ck = (ok, w, g, t) => { console.log(w.padEnd(44), "got", g.toFixed(3), "want", t.toFixed(3), ok ? "ok" : "FAIL"); if (!ok) fails++; };
const zs = [{ ...sine(48, 0, 3, true), lo: 0, hi: 53 }, { ...sine(60, 20, 3, true), lo: 54, hi: 65 }, { ...sine(72, 0, 3, false), lo: 66, hi: 127 }];
for (const note of [40, 48, 50, 60, 64, 65, 72, 84, 100]) { const x = run(mk(zs), note, 0.5, 0.1), want = 440 * Math.pow(2, (note - 69) / 12), got = freqOf(x, 48000); ck(got > 0 && Math.abs(1200 * Math.log2(got / want)) < 4, "pitch of note " + note + " (Hz)", got, want); }
{ const x = run(mk(zs), 60, 4, 0), early = rms(x, 24000, 48000), late = rms(x, 144000, 191999); ck(late > 0.7 * early && late > 0.05, "looped zone still sounds after 3 s (rms)", late, early); }
{ const x = run(mk([{ ...sine(72, 0, 0.3, false), lo: 0, hi: 127 }]), 72, 2, 0), late = rms(x, 48000, 95999); ck(late < 0.001, "one-shot zone has ended after 1 s (rms)", late, 0); }
console.log(fails ? "SOME TESTS FAILED" : "all multisample tests passed"); process.exit(fails ? 1 : 0);
