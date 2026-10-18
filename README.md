<p align="center"><img src="assets/supurrsaw-cat-logo.png" width="160" alt="SupurrSaw cat logo"></p>

# SupurrSaw 🐈🎹

A free, open source supersaw, hoover and acid synth for **Linux, Windows and macOS** (VST3 and standalone app, built from one codebase).

> Status: early. The DSP engine works and is tested offline. The plugin shell and the interface are next.

## What it does

- **3 layers.** Each one is an oscillator or a loaded sample: Super Saw (JP-8000 style), Super Square (PWM), Saw, Pulse, Triangle, Sine, Sample.
- **0% detune gives one clean oscillator.** Push the detune range further than the original when you want.
- **Sub oscillator** you can switch on for any sound.
- **Hoover:** pitch envelope dive, PWM and an LFO on the pulse width and on the detune.
- **Hard sync** with a sweepable slave (envelope and LFO) for talking leads.
- **Noise** with its own high-pass, and **distortion before or after the filter** (screech).
- **Acid:** 18 dB and 24 dB ladder filters, plus HP and BP. A piano roll sequencer with accent, slide and tie is planned.
- **Mono safe:** low frequencies are summed to mono, and the engine checks the mono sum.
- **Waveform library** (planned): import single-cycle WAVs or draw your own, stored locally.
- **Presets** as starting points: Hoover, Pick, Pluck, Trance Lead, Talking Lead (more: basses, reese, acid).

## Try the engine today

```
g++ -O2 -std=c++17 -o demo tools/demo.cpp
./demo            # renders hoover, acid, reese and supersaw demos as WAV files
g++ -O2 -std=c++17 -o presets_test tools/presets_test.cpp && ./presets_test .   # renders and checks the built-in presets
```

## Samples and copyright

The built-in `assets/samples/choir_stab_classic.wav` was made by SpeakingColors and ships with the synth. Third-party sample packs must never be committed; put those in your local waveform library.

## Licence

AGPL-3.0-or-later (the JUCE framework is used under the AGPLv3). You can use, change and share it, and anyone who shares a modified version must share the source under the same terms.

Free to use. Donations welcome (link coming soon).

Not affiliated with or endorsed by Roland, Korg or any other brand mentioned. Names are used only to describe sounds and compatibility.

## Docs
- [Interactive mockups](docs/README.md)
- [Roadmap](ROADMAP.md)

## Demo MIDI
`demo-midi/` holds MIDI files made by SpeakingColors for trying presets (A minor, 150 BPM). Play one through two presets:
```
g++ -O2 -std=c++17 -o midi_demo tools/midi_demo.cpp
./midi_demo demo-midi/Amin_-_Sylenth1_-_JP8080_LEAD_2.mid out.wav 150 55 trance_bass trance_lead
```
Notes below the split note (55) use the first preset, the rest the second. Presets: hoover, pick, pluck, trance_lead, talking_lead, trance_bass, funk_bass.
