# Roadmap: SupurrSaw

Supersaw, hoover and acid synth.

## Next
- [ ] JUCE plugin shell and standalone on cat-ui, wired to `core/supurrsaw_core.h`
- [ ] Dance MB section: starts from the Vengeance Dance Master 2 style settings with the low band off, dry/wet at 50%. Thresholds in the mockup were read from a screenshot and are approximate, tune by ear
- [ ] Pultec-style low end EQ options for basses; Neve, Moog and Sonic Maximizer style options to be researched
- [ ] Default OTT-style starting point for dance, UK hardcore and hardstyle
- [ ] 303 sequencer with accent, slide and tie, classic patterns, clickable roll
- [ ] "Ah" pluck and lead preset, PWM on square layers
- [ ] Tune the acid and hoover sounds against reference recordings

## Every cat plugin
- Draw through `catui::RenderGate` so meters and animations stop when the window is not in front (audio, automation and MIDI keep running)
- Settings page with MIDI filters on by default, selectable preset/backup folders, UI size reset to 100%
