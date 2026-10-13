# Changelog

All notable changes to Open Synth are documented here.

## [Unreleased]

### Changed
- **License changed from Apache-2.0 to AGPL-3.0-only** as of the next release, because JUCE is used under its AGPLv3 option. Releases up to and including 2.2.0 remain under Apache-2.0. Third-party licenses are listed in the new NOTICE file.

## [2.2.0] — 2026-10-12

### Fixed
- **User presets are keyed by parameter ID.** `user_presets.json` used to key values by display name, and 68 parameters share only 45 names, so one value drove several parameters (`On` toggled all three FX slots and the arpeggiator; osc waveforms/volumes and the amp/filter envelopes were coupled). Saving now writes stable parameter IDs. Old display-name presets still load.

### Added
- **Import presets** button in the preset browser: pick a JSON file (an array of presets, one preset, or `{"presets": [...]}`) and it is merged into your user presets. Presets with the same name are replaced.
- `user_preset_test`: ID round trip, legacy loading, and import merge.

### Downloads are unsigned
Windows SmartScreen: click **More info**, then **Run anyway**. macOS Gatekeeper: right-click the .pkg and choose **Open**, or run `xattr -dr com.apple.quarantine` on the installed bundles. See the README for details. Check downloads with `sha256sum -c SHA256SUMS --ignore-missing`.

## [2.1.0] — 2026-10-12

Installers, signing hooks, and a demo.

### Added
- **Installers built in CI for every release tag:**
  - Windows: an Inno Setup `setup.exe`. VST3 goes to `Common Files\VST3`, CLAP to `Common Files\CLAP` and the standalone to `Program Files\Open Synth`, and each component is optional.
  - macOS: a universal (arm64 + x86_64) `.pkg` inside a `.dmg`, with **AU** (new), VST3, CLAP and the standalone app.
  - Linux: a `.tar.gz` with `install.sh` (`~/.vst3`, `~/.clap`, `~/.local/opt/open-synth`, launcher, desktop entry, `--uninstall`), plus an AppImage of the standalone.
  - `SHA256SUMS` for all release assets, published by a single release job.
- **Optional code signing:** Authenticode on Windows, and Developer ID with the hardened runtime, a signed pkg and notarytool notarization on macOS. These steps run only when the signing secrets are configured, and are skipped cleanly otherwise. The README lists the secrets.
- **Demo:** `demo_render`, a headless offline renderer that plays a scripted phrase through six factory presets. It produced the demo in `docs/media/open-synth-demo.{mp3,ogg}`. There is also a README screenshot of the standalone running on Linux.
- CI smoke test on Linux: installs the tarball into a temporary home and runs the standalone under Xvfb.
- **Instrument browser quick categories:** one-touch Juno-Di style buttons (All / Piano / Organ / Drums / Bass / Pads) above the preset list; the dropdown now lists the real library categories, derived from the 5,600-preset library instead of a stale hardcoded list.
- **Keyboard navigation in the preset browser:** Up/Down auditions presets live, Enter loads and closes, Escape closes, Left/Right cycles categories when the list has focus.
- **Now-playing state:** the browser header shows the currently loaded instrument, its row is highlighted in neon yellow with a ▶ marker, and opening the browser preselects and scrolls to it.
- `preset_browser_test`: headless UI-logic tests covering category switching, keyboard navigation, and loaded-instrument state.

### Changed
- Plugins now find the samples bundled inside or beside their own VST3/CLAP/AU file, as well as in a shared `ProgramData`/`/Library` location. Previously the samples had to be copied by hand into the user data folder.
- On macOS the samples live in `Contents/Resources` instead of `Contents/MacOS`, so the bundles can be code-signed.

### Fixed
- Category filter matched almost nothing: the dropdown used title-case names ("Pads", "Keys") while the library uses snake_case ("synth_pad"). Filters now match whole words against normalized categories, so "Pad" covers `synth_pad`, "Piano" covers `upright_piano`/`synth_piano`, etc.
- Filtered user-preset lists loaded the wrong preset — the selected display row was used as an index into the unfiltered user-preset array.
- Category dropdown opened with nothing selected; it now defaults to "All Categories".

## [2.0.2] — 2026-07-25

Split Personality — performance mode, new brand, and true cross-platform builds.

### Added
- **Split keyboard:** notes below the split point route to a dedicated part auto-configured as a plucked Karplus-Strong bass. Adjustable split point (slider + draggable overlay on the virtual keyboard). ROMpler samples stay on the upper zone.
- **Layer (Dual):** every note doubles on a second part mirroring the current preset with ±8 cent detune — classic Juno Dual thickness. Tracks preset and panel changes live.
- **Transpose:** ±12 semitones at the engine input.
- Per-note routing records: flipping split/layer/transpose mid-chord never leaves stuck notes.
- New brand identity: synthwave key art as the app icon on all platforms, README banner, and social preview.
- `scripts/install_local.sh`: freedesktop install (binary + samples + icon + desktop entry).
- GitHub Actions CI: Linux, Windows, and macOS builds + test suite on every push; tag builds bundle samples and attach artifacts to the release automatically.

### Fixed
- MSVC portability: `_USE_MATH_DEFINES` / `NOMINMAX`; `std::tanh` instead of non-standard `std::tanhf`; namespace forward-declaration bug that broke the Windows link.

## [2.0.1] — 2026-07-25

The "static" hotfix. Three compounding audio bugs that produced noise/drone under presets — most audibly a harsh static behind the Grand Piano.

### Fixed
- **Filter explosion (the static):** the Chamberlin SVF went unstable above sr/6 (8kHz @ 48kHz). Any preset with cutoff > 8kHz (piano @ 12kHz) drove the filter into runaway feedback, railing the mix into square-wave noise. Replaced with a TPT (Zavalishin) SVF — stable up to Nyquist.
- **Voice envelopes ignored presets:** `noteOn` never copied the part's envelope into the voice, so every voice used default sustain 0.8 and droned under the decaying samples. Envelopes (amp/filter/pitch + curves) now sync from each voice's part every block — covers direct, arpeggiator, and queue-triggered notes, plus live parameter edits.
- **Mix clipping:** raw voice accumulation (up to ±8 at full polyphony) slammed the tanh limiter. Added 1/√N polyphonic headroom scaling and a 0.85 sample-layer trim.
- **Acoustic preset envelopes:** piano, upright, guitars, and basses had pad-style envelopes (sustain 0.6–0.7) that droned beneath the samples. Now sustain 0–0.15 with instrument-natural decays (piano 2400ms). All 5,600 presets regenerated.

### Added
- `engine_render_test`: headless full-engine render harness (layer isolation modes, RMS/peak profiling, WAV dump) — the tool that found all of the above.

## [2.0.0] — 2026-07-25

**The C++ Rewrite.** The engine was rebuilt from the Flutter/Dart prototype (v1.1.0 and earlier) into a native JUCE 8 + C++20 application: sub-10ms latency, 16-part multitimbrality, full MFX chain, and a streaming sample ROMpler. Standalone + VST3 + CLAP.

### Sound Engine
- Dual oscillators per part — sine, saw, square, triangle, pulse, noise, wavetable
- Wavetable synthesis with 100+ built-in tables and morphing
- Physical modeling mode (body resonance, key click, sympathetic strings)
- Unison voices with detune and stereo spread; FM on both oscillators; sub-oscillator
- Multi-mode filter (lowpass, highpass, bandpass, notch, comb)
- 3 envelopes (amp with delay/hold, filter with key tracking, pitch) with per-stage curves
- 2 LFOs per part — tempo-syncable, fade-in, routable to pitch/filter/amp/pan
- Arpeggiator with 8 patterns, octave range, swing, gate, hold
- 16-part multitimbrality with full MIDI channel separation
- Drum kit with 8 preset kits and a rhythm pattern player (29 patterns across 9 categories, including Afrobeat and Reggae)

### Sample Engine (ROMpler)
- Streaming sample player with cubic interpolation, per-voice anti-aliasing, ADSR, pitch bend
- Multi-zone multisample manifests with velocity layers, round-robin, release samples, loop crossfades
- Supports both multi-layer and flat manifest formats with robust sample path resolution
- 8 CC0 sample libraries wired in as git submodules (1,419 zones): 2 pianos, 2 drum kits, 2 basses, 2 guitars

### Effects
- 5-slot FX engine (3 MFX + Reverb + Chorus, Juno-Di parity) with 60+ processor types

### Presets
- 5,600 factory presets across 20+ categories (generated library with per-category synthesis profiles)
- User preset manager with categories; setlist mode for live performance

### App
- Standalone, VST3, and CLAP builds (desktop); synthwave '84 UI
- Real-time stereo oscilloscope, zone editor overlay, phrase sampler, audio recording
- MIDI learn on all parameters, app state persistence, CPU load meter

### Fixed for 1.0.0
- **Sample manifests silently failed to load** — loader only understood the multi-layer format; all 8 shipped manifests use the flat format. The entire ROMpler layer is now functional and test-verified.
- **MIDI CC 123 (All Notes Off) left sample voices hanging** — sample player now releases; added CC 120 (All Sound Off) instant kill wired through the engine and MIDI wrapper
- ADSR attack test contaminated by prior voices' release tails — new `allSoundOff()` gives clean scenario isolation
- `native/oboe` stale gitlink broke `git submodule update --init` (and fresh `--recurse-submodules` clones)
- Cabinet simulator potential uninitialized values on out-of-range/NaN parameters
- Member initialization order mismatch in the plugin editor (`-Wreorder`, UB-class)
- Constructor parameter shadowing JUCE `AudioProcessorEditor::processor`
- Afrobeat and Reggae rhythm patterns implemented but unreachable — now in the library

### Build & Tooling
- JUCE located via `-DJUCE_DIR`, `JUCE_DIR` env var, `libs/JUCE` submodule, or `~/.juce` — no more hardcoded absolute paths
- Wired `fx_tests` (1,021 assertions) and `sample_static_test` into CMake; all test targets get FLAC support
- Generated preset library header silences expected conversion warnings via diagnostic pragmas (122k → ~800 warnings)
- Removed legacy Flutter/FFI-era dead code (`lib/`, `native/`, PortAudio/Oboe FFI sources)
- Tests: 83/83 sample decode, block processing + ADSR + pitch bend, 8/8 manifest renders, 1,021 FX assertions

---

