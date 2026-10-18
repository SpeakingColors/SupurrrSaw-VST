# Tuned starting points (measured against the author's own reference renders)

These are engine settings for `tools/tune.cpp` (key=value). They will become real presets in the plugin.

- **Acid Modern:** `tune acid out.wav root=63 cut=1500 reso=0.75 env=0.4 dist=0.85 drive=0.3 sub=0 m18=0 wave=1 fdec=0.15 lc=150`
- **Hoover:** `tune hoover out.wav root=43 saw=0 voices=5 det=0.6 range=3 oct1=1 oct2=2 cut=9000 reso=0.45 drive=0.3 dist=0.3 lc=40 sub=0 layers=3` (three pulse layers an octave apart, slow bend down about 4 semitones over 1 s, 5 Hz LFO on pulse width)
- **Reese Clean / Dirty and Ah Stab Classic:** see `tools/demo.cpp`.
