# Tuned starting points (measured against the author's own reference renders)

These are engine settings for `tools/tune.cpp` (key=value). They will become real presets in the plugin.

- **Acid Modern:** `tune acid out.wav root=48 cut=900 reso=0.75 env=0.7 dist=1.0 drive=0.8 sub=0 m18=1 wave=1 fdec=0.2 lc=80 dmix=0.22 dtime=0.2 dfb=0.35 rmix=0.1 rsize=0.5 rwidth=0.3 cmix=0.15` (pulse, 18 dB ladder, heavy distortion, ping-pong delay, small reverb)
- **Hoover (smooth):** `tune hoover out.wav root=57 saw=2 voices=5 det=0.7 range=0.6 oct1=1 oct2=1 layers=2 cut=6000 reso=0.3 drive=0.6 dist=0.1 lc=60 sub=0 pamt=3 ptime=0.1 cmix=0.2 rmix=0.1 rsize=0.7 rwidth=0.3` (two super-square layers an octave apart, a quick 3 semitone dive that settles in about 0.1 s, no LFO on volume or cutoff)
- **Reese Clean / Dirty and Ah Stab Classic:** see `tools/demo.cpp`.
