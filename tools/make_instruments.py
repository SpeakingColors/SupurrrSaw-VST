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
Needs: python3, numpy, scipy, soundfile, librosa.
Also adds "Moist Keys", a processed grand piano (bright house piano).
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


def biquad_peak(x, f0, gain_db, q):
    from scipy.signal import lfilter
    A = 10 ** (gain_db / 40); w = 2 * np.pi * f0 / SR; al = np.sin(w) / (2 * q)
    b = [1 + al * A, -2 * np.cos(w), 1 - al * A]; a = [1 + al / A, -2 * np.cos(w), 1 - al / A]
    return lfilter(np.array(b) / a[0], np.array(a) / a[0], x)


def derive_moist_keys(out, name="Moist Keys", source="Grand Piano", ext="flac"):
    """A made-up house piano in the spirit of the classic dance PCM pianos (the thin, hard, hollow ones that sit in a house mix).
    Built from the Kawai grand zones already in OUT. What it copies is the character, not a recording of any keyboard:
      thin low end, a hard hammer front, a presence hump at 2 to 4 kHz, a band-limited top, a lower sample rate with a little grit,
      and a short looped sustain (a few whole periods repeated) that gives the static hollow 'ahh' after the hit.
    Adds one instrument to instruments.json. ext is flac for the plugin pack, wav for the web page."""
    from scipy.signal import butter, sosfilt, resample_poly
    jf = os.path.join(out, "instruments.json")
    J = json.load(open(jf))
    J["instruments"] = [i for i in J["instruments"] if i["name"] != name]
    src = next(i for i in J["instruments"] if i["name"] == source)
    os.makedirs(os.path.join(out, name), exist_ok=True)
    hp = butter(2, 190 / (SR / 2), "high", output="sos")
    lp = butter(4, 11500 / (SR / 2), "low", output="sos")
    zones = []
    for idx, z in enumerate(src["zones"]):
        x, sr = sf.read(os.path.join(out, z["file"]), dtype="float64")
        if x.ndim > 1:
            x = x.mean(axis=1)
        f0 = 440.0 * 2 ** ((z["root"] - 69) / 12.0)
        per = SR / f0
        # 1. keep the real hit, then rebuild the tail from a short loop of whole periods taken just after the hit
        hit = int(0.55 * SR)
        x = x[: int(2.0 * SR)].copy()
        pk = np.abs(x).max()
        L = max(1, int(round(0.09 * f0))) * per            # about 90 ms, a whole number of periods
        L = int(round(L))
        a0 = int(0.40 * SR)
        loop = x[a0:a0 + L].copy()
        xf = min(int(per), L // 3)
        if xf > 8:                                          # seam: fade the loop end into its own start
            w = np.linspace(0, 1, xf)
            loop[-xf:] = loop[-xf:] * (1 - w) + x[a0 - xf:a0] * w
        n = int(3.4 * SR)
        y = np.zeros(n)
        y[:hit] = x[:hit]
        pos = hit
        # match the level at the join, then repeat the loop with a slow fade
        g = np.sqrt(np.mean(x[hit - L:hit] ** 2) / max(np.mean(loop ** 2), 1e-12))
        k = 0
        while pos < n:
            m = min(L, n - pos)
            seg = loop[:m] * g
            if k == 0 and m > 0:
                c = min(int(0.02 * SR), m)
                seg[:c] = seg[:c] * np.linspace(0, 1, c) + x[hit:hit + c] * np.linspace(1, 0, c) if hit + c <= len(x) else seg[:c]
            y[pos:pos + m] = seg
            pos += m
            k += 1
        t = np.arange(n) / SR
        y = y * (0.28 + 0.72 * np.exp(-t / 0.35)) * np.exp(-t / 1.7)    # quick fall into a quiet, steady sustain
        # 2. the voicing
        y = sosfilt(hp, y)
        y = biquad_peak(y, 320, -3.5, 0.9)
        y = biquad_peak(y, 2400, 7.0, 0.9)
        y = biquad_peak(y, 4200, 4.5, 0.8)
        y = sosfilt(lp, y)
        # 3. lower sample rate, a little grit (32 kHz worth of band, 13 bit steps)
        d = resample_poly(y, 2, 3)
        d = np.round(d * 4096.0) / 4096.0
        y = np.interp(np.arange(n) / 1.0, np.arange(len(d)) * 1.5, d, right=0.0)
        # 4. pushed hard front, then limited
        front = 1 + 0.9 * np.exp(-t / 0.03)
        y = np.tanh(1.5 * y * front / max(pk, 1e-9)) / np.tanh(1.5)
        fo = int(0.6 * SR)
        y[n - fo:] *= np.linspace(1, 0, fo) ** 1.5
        y = y / max(np.abs(y).max(), 1e-9) * min(0.85, pk * 1.15)
        fl = "%s/%03d_n%d.%s" % (name, idx, z["root"], ext)
        if ext == "flac":
            sf.write(os.path.join(out, fl), y.astype(np.float32), SR, subtype="PCM_16")
        else:
            sf.write(os.path.join(out, fl), y.astype(np.float32), SR, subtype="PCM_16")
        zones.append({**z, "file": fl, "len": int(n)})
    J["instruments"].append({"name": name, "loop": False, "zones": zones})
    json.dump(J, open(jf, "w"), indent=1)


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
    derive_moist_keys(out)
    for i in inst:
        print("%-16s %2d zones, roots %s" % (i["name"], len(i["zones"]), [z["root"] for z in i["zones"]]))


if __name__ == "__main__":
    main()
