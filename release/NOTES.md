> **macOS is not in this release yet (known bug, [issue #1](https://github.com/SpeakingColors/SupurrrSaw-VST/issues/1)).** GitHub's hosted macOS runners are not starting our macOS build job, so this zip has the Windows and Linux installers only. We will re-release the macOS version as soon as GitHub builds work again. If you know how to fix this, please tell us in the issue.

**New in 0.3.3:** sampled instruments are bundled in the plugin (pianos, strings, brass, voices and more), "Aah stab" and "AHH wave" are in the Voices group of the wave list, and there are two new drum kits. Several presets have new names.

Free and open source supersaw, hoover and acid synth by Studio 87. VST3 and standalone.

**Download the zip, unzip it, and open `INSTALL.txt`.** It has a folder per system (`windows`, `macos`, `linux`) with a real installer in each, plus the manual (PDF) and checksums. Run the installer for your system, nothing to copy by hand. Windows has an uninstaller in Settings > Apps, macOS has an Uninstall app.

**New in 0.3.2:** drag to modulate: drag a source from the Mod tray onto a knob to set up the modulation. **New in 0.3.1:** on-screen pitch and mod wheels with separate up and down bend ranges. **New in 0.3.0:** pitch wheel with a bend range, MPE (bend, slide and press a single note of a chord), and Pressure, Slide and Bend as mod matrix sources. The plugin also reads polyphonic key pressure.

The installers are not signed yet, so the first run may show a warning. `INSTALL.txt` says which button to click.

Overwrote a factory bank? The factory banks A to D are in the repo under `presets/factory/` as `.syx` files. Import one on the Presets page (Banks, Import .syx), or press **Restore factory** there.

Found a problem? Open an issue and, if you can, use **Files > Copy diagnostics** inside the plugin and paste the result.
