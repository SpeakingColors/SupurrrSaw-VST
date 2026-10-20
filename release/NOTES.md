Free and open source supersaw, hoover and acid synth by Studio 87. VST3 and standalone. Short manual attached.

**Download the installer for your system and run it. Nothing to copy by hand.**

- **Windows 10 or 11:** `SupurrSaw_..._windows_setup.exe`. Double click, Next, Install. The plugin goes where your DAW looks for it, and the standalone app gets a Start menu entry.
- **macOS (Apple silicon):** `SupurrSaw_..._macos.pkg`. Double click and Continue until done. The plugin goes into your plugin folder, the app into Applications.
- **Linux, DAW installed normally:** `SupurrSaw_..._linux.deb`. Double click it (or `sudo apt install ./file.deb`). Installs the plugin to `/usr/lib/vst3` and the standalone app.
- **Linux, standalone app only (Flatpak):** `SupurrSaw_..._linux.flatpak`. Double click it in your software center (or `flatpak install file.flatpak`). Needs the internet once to fetch the runtime.

Then restart your DAW or rescan plugins.

The builds are not signed yet, so the first run shows a warning. Windows: click More info, then Run anyway. macOS: System Settings, Privacy and Security, Open Anyway.

Overwrote a factory bank? The factory banks A to D are in the repo under `presets/factory/` as `.syx` files. Import one on the Presets page (Banks, Import .syx), or press **Restore factory** there.

Found a problem? Open an issue and, if you can, use **Files > Copy diagnostics** inside the plugin and paste the result.
