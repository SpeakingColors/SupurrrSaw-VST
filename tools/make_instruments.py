#!/usr/bin/env python3
"""Builds the SupurrSaw instrument pack (multisampled piano, FM piano, trumpet, violin, strings)
from two CC0 sample libraries by Versilian Studios / Sam Gossner:
  VCSL      https://github.com/sgossner/VCSL
  VSCO 2 CE https://github.com/sgossner/VSCO-2-CE
The raw samples are NOT part of this repository. This script converts them to short mono FLAC files,
finds each note's real pitch from the audio (the file names use mixed octave numbers), builds seamless
loops for the sustained instruments, and writes one instruments.json that the engine reads.

usage: make_instruments.py VCSL_DIR VSCO_DIR OUT_DIR [--lite]
  --lite  every second zone and shorter notes: a small set (a few MB) that is embedded in the web playground
Needs: python3, numpy, soundfile, librosa.
"""
import sys, os, re, glob, json
import numpy as np, soundfile as sf
import librosa

SR = 44100
PC = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}
CREDIT = "Samples: Versilian Studios / Sam Gossner (VCSL, VSCO 2 Community Edition), CC0 1.0"


def name_midi(fn):
    """note from the file name with standard numbering (C4 = 60); the octave digit is only trusted up to a per-instrument offset"""
    m = re.search(r'_([A-G]#?)(-?\d)(?=[_.])', os.path.basename(fn))
    return PC[m.group(1)] + 12 * (int(m.group(2)) + 1) if m else None


def pyin_midi(x):
    seg = x[int(0.2 * SR):int(1.2 * SR)].astype(np.float32)
    f0, _, _ = librosa.pyin(seg, fmin=25, fmax=4500, sr=SR, frame_length=8192)
    f0 = f0[~np.isnan(f0)]
    return None if len(f0) == 0 else 69 + 12 * np.log2(np.median(f0) / 440)


def fine_tune(x, m):
    """tune in cents of the real fundamental against equal temperament, from a spectral peak with parabolic interpolation"""
    seg = x[int(SR * 0.25):int(SR * 0.25) + int(SR * 0.9)]
    if len(seg) < SR * 0.3:
        seg = x[:int(SR * 0.9)]
    N = 1 << 17
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), N)) + 1e-12
    fr = np.fft.rfftfreq(N, 1 / SR)
    f = 440 * 2 ** ((m - 69) / 12)
    best = None
    for k in range(1, 7):
        fk = f * k
        if fk > 4000 and k > 1:
            break
        i0, i1 = np.searchsorted(fr, fk * 2 ** (-0.4 / 12)), np.searchsorted(fr, fk * 2 ** (0.4 / 12)) + 1
        j = i0 + int(np.argmax(sp[i0:i1]))
        if best is None or sp[j] > best[0] * 1.0:
            best = (sp[j], j, k)
    _, j, k = best
    a, b, c = np.log(sp[j - 1]), np.log(sp[j]), np.log(sp[j + 1])
    d = 0.5 * (a - c) / (a - 2 * b + c)
    return float(np.clip(1200 * np.log2(((j + d) * SR / N / k) / f), -50, 50))


def load(fn):
    x, sr = sf.read(fn, dtype='float64')
    if x.ndim > 1:
        x = x.mean(axis=1)
    if sr != SR:
        raise SystemExit("unexpected sample rate in " + fn)
    return x


def trim_start(x):
    thr = 10 ** (-60 / 20) * max(1e-9, np.abs(x).max())
    i = int(np.argmax(np.abs(x) > thr))
    return x[max(0, i - int(0.003 * SR)):]


def build(name, files, outdir, loop, maxlen, fadeout=0.6, gain_to=0.7, thin=1):
    os.makedirs(os.path.join(outdir, name), exist_ok=True)
    loaded = []
    for fn in sorted(files):
        nm = name_midi(fn)
        if nm is None:
            continue
        x = trim_start(load(fn))
        loaded.append((nm, x, fn, pyin_midi(x)))
    offs = [int(round((p - nm) / 12)) * 12 for nm, x, fn, p in loaded if p is not None]
    off = max(set(offs), key=offs.count)   # the file names use a shifted octave number; the most common shift wins
    items = []
    for nm, x, fn, p in loaded:
        m = nm + off
        if p is not None and 20 < p < 100 and 0.5 < abs(p - m) <= 1.5:
            m = int(round(p))   # a file name that is one semitone off (the lowest Kawai note is called A#-1 but is an A0)
        items.append((m, fine_tune(x, m), x, fn))
    items.sort(key=lambda t: t[0])
    seen, keep = set(), []
    for it in items:   # one sample per detected note
        if it[0] in seen:
            continue
        seen.add(it[0])
        keep.append(it)
    peak = max(np.abs(it[2][:SR]).max() for it in keep)
    if thin > 1:
        keep = keep[::thin]
    zones = []
    for idx, (m, c, x, fn) in enumerate(keep):
        n = min(len(x), int(maxlen * SR))
        x = x[:n].copy()
        ls = le = 0
        if loop:
            xf = int(0.35 * SR)
            le = n - 1 - xf // 4
            ls = max(int(n * 0.40), xf + 1)
            t = np.linspace(0, 1, xf)
            x[le - xf:le] = x[le - xf:le] * np.cos(t * np.pi / 2) + x[ls - xf:ls] * np.sin(t * np.pi / 2)
            x = np.concatenate([x[:le], x[ls:ls + 8]])   # a few guard samples past the loop end that continue from the loop start, so interpolation at the seam is clean
        else:
            fo = min(int(fadeout * SR), n // 2)
            x[n - fo:] *= np.linspace(1, 0, fo) ** 1.5
        x = x / peak * gain_to
        fl = "%s/%03d_n%d.flac" % (name, idx, m)
        sf.write(os.path.join(outdir, fl), x.astype(np.float32), SR, subtype='PCM_16')
        zones.append({"file": fl, "root": int(m), "tune": round(c, 1), "loopStart": int(ls), "loopEnd": int(le), "len": int(len(x))})
    for i, z in enumerate(zones):   # key ranges: split halfway between neighbouring roots
        z["lo"] = 0 if i == 0 else (zones[i - 1]["root"] + z["root"] + 1) // 2
        z["hi"] = 127 if i == len(zones) - 1 else (z["root"] + zones[i + 1]["root"]) // 2
    return {"name": name, "loop": bool(loop), "zones": zones}


def main():
    vcsl, vsco, out = sys.argv[1:4]
    lite = '--lite' in sys.argv
    th = 2 if lite else 1
    sc = 0.6 if lite else 1.0
    G = lambda base, pat: glob.glob(os.path.join(base, pat))
    inst = [
        build("Grand Piano", G(vcsl, "Chordophones/Zithers/Grand Piano, Kawai/Sustains/*_v2_rr1_*.wav"), out, False, 7.0*sc, 0.8, thin=th),
        build("FM Piano", G(vcsl, "Electrophones/TX81Z/FM Piano/*_vl2.wav"), out, False, 6.0*sc, 0.6, thin=th),
        build("Trumpet", G(vsco, "Brass/Trumpet/sus/*_v3_*.wav"), out, True, 3.5*sc, thin=th),
        build("Solo Violin", G(vsco, "Strings/Solo Violin/Arco Vib/*_f.wav"), out, True, 4.0*sc, thin=th),
        build("Violin Ensemble", G(vsco, "Strings/Violin Section/susVib/*_v2.wav"), out, True, 4.0*sc, thin=th),
        build("Viola Ensemble", G(vsco, "Strings/Viola Section/susvib/*_v2_*.wav"), out, True, 4.0*sc, thin=th),
        build("Cello Ensemble", G(vsco, "Strings/Cello Section/susvib/*_v3_*.wav"), out, True, 4.0*sc, thin=th),
    ]
    json.dump({"version": 1, "credit": CREDIT, "instruments": inst}, open(os.path.join(out, "instruments.json"), "w"), indent=1)
    for i in inst:
        print("%-16s %2d zones, roots %s" % (i["name"], len(i["zones"]), [z["root"] for z in i["zones"]]))


if __name__ == "__main__":
    main()
