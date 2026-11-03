#!/usr/bin/env python3
"""Cuts a long recording of one note per bar into instrument zones (FLAC, mono or stereo, up to 3.2 s each) and writes an instruments.json fragment.

The recordings are made from a MIDI file with notes 36, 42, 48 ... 120 (one every tritone, one per bar, 60 BPM so a bar is 4 s,
note held 3.75 s). The real sounding pitch of every bar is measured from the audio and snapped to the written note plus a whole number
of octaves (a 16' patch sounds an octave lower than written). Raw recordings are not part of the repository.

usage: cut_sampled_bars.py OUT_DIR "Instrument Name=path/to/file.wav" ["Other Name=file2.wav" ...]
Needs: python3, numpy, scipy, soundfile.
"""
import sys, os, json
import numpy as np, soundfile as sf
from scipy.signal import resample_poly

SR = 44100
BAR = 4.0
MAXLEN = 3.2   # seconds kept per zone
FIRST, STEP = 36, 6
NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def hps_midi(seg):
    n = 1 << 18
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), n))
    fr = np.fft.rfftfreq(n, 1 / SR)
    h = sp.copy()
    for k in (2, 3, 4):
        d = sp[::k]
        h[:len(d)] *= d
    sel = (fr > 25) & (fr < 4500)
    i = int(np.argmax(h[sel]))
    return 69 + 12 * np.log2(fr[sel][i] / 440.0)


def fine_tune(x, m):
    """cents of the real fundamental against equal temperament, from a spectral peak with parabolic interpolation"""
    seg = x[int(SR * 0.1):int(SR * 1.0)]
    if len(seg) < SR * 0.3:
        seg = x[:int(SR * 0.9)]
    n = 1 << 17
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), n)) + 1e-12
    fr = np.fft.rfftfreq(n, 1 / SR)
    f = 440 * 2 ** ((m - 69) / 12)
    best = None
    for k in range(1, 7):
        fk = f * k
        if fk > 4000 and k > 1:
            break
        i0, i1 = np.searchsorted(fr, fk * 2 ** (-0.4 / 12)), np.searchsorted(fr, fk * 2 ** (0.4 / 12)) + 1
        if i1 - i0 < 3:
            continue
        j = i0 + int(np.argmax(sp[i0:i1]))
        if best is None or sp[j] > best[0]:
            best = (sp[j], j, k)
    if best is None or best[1] < 1:
        return 0.0
    _, j, k = best
    a, b, c = np.log(sp[j - 1]), np.log(sp[j]), np.log(sp[j + 1])
    den = a - 2 * b + c
    d = 0.5 * (a - c) / den if den != 0 else 0.0
    return float(np.clip(1200 * np.log2(((j + d) * SR / n / k) / f), -50, 50))


def cut(name, path, out):
    x, sr = sf.read(path, dtype='float64', always_2d=True)
    if sr != SR:
        g = np.gcd(sr, SR)
        x = resample_poly(x, SR // g, sr // g, axis=0)
    mono_mix = x.mean(1)
    side = (x[:, 0] - x[:, 1]) * 0.5 if x.shape[1] > 1 else np.zeros(len(x))
    stereo = float(np.sqrt((side ** 2).mean() + 1e-30) / np.sqrt((mono_mix ** 2).mean() + 1e-30)) > 0.003
    bars = []
    for b in range(int(len(x) / (BAR * SR))):
        s = int(b * BAR * SR)
        seg = mono_mix[s:s + int(BAR * SR)]
        if np.abs(seg).max() < 0.002:
            continue
        d = hps_midi(seg[int(0.05 * SR):int(1.0 * SR)])
        written = 125 if b == 15 else FIRST + STEP * b   # the last note of the MIDI file is 125
        off = 12 * round((d - written) / 12)
        ok = abs(d - (written + off)) < 1.2
        bars.append(dict(b=b, s=s, written=written, d=d, off=off, ok=ok))
    good = [z['off'] for z in bars if z['ok']]
    if good:
        vals, cnt = np.unique(good, return_counts=True)
        off = int(vals[np.argmax(cnt)])
    else:
        off = 0
    for z in bars:
        z['root'] = z['written'] + off
    # drop bars whose root is out of the MIDI range
    bars = [z for z in bars if 0 <= z['root'] <= 127]
    bars = [z for z in bars if not (z['root'] > 84 and z['root'] % 12 == 6)]   # above C6 only every octave: keeps the pack small, the stretch is still small up there
    peak = 0.0
    zones = []
    for z in bars:
        seg = x[z['s']:z['s'] + int(MAXLEN * SR)]
        m = np.abs(seg).max(1)
        idx = np.where(m > 0.0003)[0]
        end = min(len(seg), (idx[-1] + int(0.15 * SR)) if len(idx) else len(seg))
        end = max(end, int(0.4 * SR))
        seg = seg[:end].copy()
        fl = min(int(0.06 * SR), len(seg) // 4)
        seg[-fl:] *= (0.5 + 0.5 * np.cos(np.linspace(0, np.pi, fl)))[:, None]
        z['seg'] = seg
        z['tune'] = fine_tune(seg.mean(1), z['root'])
        peak = max(peak, np.abs(seg).max())
    gain = 0.89 / peak if peak > 0 else 1.0
    os.makedirs(os.path.join(out, name), exist_ok=True)
    roots = [z['root'] for z in bars]
    entry = dict(name=name, loop=False, zones=[])
    for i, z in enumerate(bars):
        lo = 0 if i == 0 else int(np.floor((roots[i - 1] + roots[i]) / 2)) + 1
        hi = 127 if i == len(bars) - 1 else int(np.floor((roots[i] + roots[i + 1]) / 2))
        data = z['seg'] * gain
        if not stereo:
            data = data.mean(1)
        fn = '%s/%03d_n%d.flac' % (name, i, z['root'])
        sf.write(os.path.join(out, fn), data, SR, subtype='PCM_16')
        entry['zones'].append(dict(file=fn, root=int(z['root']), tune=round(z['tune'], 1), loopStart=0, loopEnd=0, len=int(len(z['seg'])), lo=lo, hi=hi))
    n_ok = len(good)
    print('%-22s %s offset %+d st, pitch matched %d of %d bars, %d zones, %s, roots %s..%s' % (
        name, 'stereo' if stereo else 'mono  ', off, n_ok, len(bars), len(entry['zones']), 'gain %.2f' % gain,
        NAMES[roots[0] % 12] + str(roots[0] // 12 - 1), NAMES[roots[-1] % 12] + str(roots[-1] // 12 - 1)))
    return entry


def main():
    out = sys.argv[1]
    insts = []
    for a in sys.argv[2:]:
        name, path = a.split('=', 1)
        insts.append(cut(name, path, out))
    json.dump(dict(version=1, credit='M1 recordings by the author', instruments=insts), open(os.path.join(out, 'instruments.json'), 'w'), separators=(',', ':'))
    tot = sum(os.path.getsize(os.path.join(out, z['file'])) for i in insts for z in i['zones'])
    print('total %.1f MB' % (tot / 1e6))


if __name__ == '__main__':
    main()
