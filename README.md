<p align="center"><img src="assets/supurrsaw-cat-logo.png" width="160" alt="SupurrSaw cat logo"></p>

# SupurrSaw 🐈🎹

A free, open source supersaw, hoover and acid synth for **Linux, Windows and macOS** (VST3 and standalone app, built from one codebase).

> Status: early. The DSP engine works and is tested offline. The plugin shell and the interface are next.

## What it does

- **3 layers.** Each one is an oscillator or a loaded sample: Super Saw (JP-8000 style), Super Square (PWM), Saw, Pulse, Triangle, Sine, Sample.
- **0% detune gives one clean oscillator.** Push the detune range further than the original when you want.
- **Sub oscillator** you can switch on for any sound.
- **Hoover:** pitch envelope dive, PWM and an LFO.
- **Acid:** 18 dB and 24 dB ladder filters, plus HP and BP. A piano roll sequencer with accent, slide and tie is planned.
- **Mono safe:** low frequencies are summed to mono, and the engine checks the mono sum.
- **Waveform library** (planned): import single-cycle WAVs or draw your own, stored locally.
- **Presets** as starting points: trance leads and plucks, basses, hoover, reese, acid.

## Try the engine today

```
g++ -O2 -std=c++17 -o demo tools/demo.cpp
./demo            # renders hoover, acid, reese and supersaw demos as WAV files
```

## Samples and copyright

Commercial sample packs are not included and must not be committed. Put your own samples in your local waveform library.

## Licence

AGPL-3.0-or-later (the JUCE framework is used under the AGPLv3). You can use, change and share it, and anyone who shares a modified version must share the source under the same terms.

Free to use. Donations welcome (link coming soon).

Not affiliated with or endorsed by Roland, Korg or any other brand mentioned. Names are used only to describe sounds and compatibility.
