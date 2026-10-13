# Open Synth

[![Build](https://github.com/Blackshield-Company/open-synth/actions/workflows/build.yml/badge.svg)](https://github.com/Blackshield-Company/open-synth/actions/workflows/build.yml)
[![License](https://img.shields.io/badge/license-AGPL--3.0--only-blue)](LICENSE)
[![C++20](https://img.shields.io/badge/standard-C%2B%2B20-blue)](README.md)

![Open Synth](assets/social_preview.png)

A free, open-source software synthesizer built with JUCE and C++20. Inspired by the Roland Juno-Di workflow — instant playability, rich sound, zero menu diving.

## What It Is

Open Synth is a **standalone app plus VST3, CLAP and (on macOS) AU synthesizer plugin** designed for musicians who want to plug in and play. It combines a modern C++ DSP engine with a clean JUCE-based UI, delivering everything from classic analog-style leads to evolving wavetable pads — without forcing you through nested menus.

Originally prototyped in Flutter/Dart, the engine was rebuilt in native C++ for pro audio performance: sub-10ms latency, 16-part multitimbrality, and a full MFX (multi-effects) chain.

## Features

### Sound Engine
- **Dual oscillators** per part — sine, saw, square, triangle, pulse, noise, wavetable
- **Wavetable synthesis** with 100+ built-in tables and morphing
- **Physical modeling** mode for acoustic-style realism (body resonance, key click, sympathetic strings)
- **Unison voices** with detune and stereo spread
- **FM synthesis** on both oscillators
- **Sub-oscillator** with multiple octave modes

### Filter & Envelopes
- **Multi-mode filter**: lowpass, highpass, bandpass, notch, comb
- **3 envelopes**: amp (with delay/hold), filter (with key tracking), pitch
- **Per-stage curve shaping** (linear, exponential, logarithmic)

### Modulation
- **2 LFOs** per part — tempo-syncable, fade-in, routable to pitch/filter/amp/pan
- **Pitch bend**, mod wheel, aftertouch, polyphonic aftertouch
- **Arpeggiator** with 8 patterns, octave range, swing, gate, and hold

### Effects (MFX Chain)
4-slot series effects engine with 22 processor types:

| Legacy (Slot 0) | MFX Expansion (Slots 1-3) |
|-----------------|---------------------------|
| Chorus | Equalizer |
| Delay | Limiter |
| Reverb | Rotary |
| Phaser | Tremolo |
| Flanger | Auto-Wah |
| Compressor | Bitcrusher |
| Drive | Ring Modulator |
| | Pitch Shifter |
| | Multitap Delay |
| | Ping-Pong Delay |
| | Spring Reverb |
| | Gated Reverb |
| | Amp Simulator |
| | Stereo Widener |
| | Vocoder |

### Performance & Workflow
- **16-part multitimbral** — full MIDI channel separation
- **Voice allocation**: polyphonic, monophonic, legato, unison modes
- **Drum kit** with 8 preset kits and rhythm pattern player
- **User preset manager** — save/load with categories
- **Setlist mode** — chain presets for live performance
- **MIDI learn** on all parameters
- **Audio recording** built-in
- **CPU load meter** and oscilloscope

## Demo

![Open Synth standalone running on Linux](docs/media/screenshot-linux.png)

🎧 **[Listen to the demo (MP3)](docs/media/open-synth-demo.mp3)** · [OGG](docs/media/open-synth-demo.ogg) — about 50 seconds through
*Sunset Pad*, *Acid Bass Modern*, *FM Bell Layered*, *Super Saw Soft*, *Synth Brass Dark* and *Grand Piano*, at 110 BPM.
It was rendered offline by the real engine with `demo_render` (see [Rendering the demo](#rendering-the-demo)), so nothing
was recorded from a soundcard. The screenshot is the standalone app running under Xvfb.

## Download

Every [GitHub Release](https://github.com/synthalorian/open-synth/releases/latest) has installers built by the
[Build workflow](.github/workflows/build.yml), plus a `SHA256SUMS` file:

| Platform | File | Installs |
|----------|------|----------|
| Windows 10/11 x64 | `open-synth-<ver>-windows-x64-setup.exe` | VST3 → `C:\Program Files\Common Files\VST3`, CLAP → `C:\Program Files\Common Files\CLAP`, standalone → `C:\Program Files\Open Synth` (each one optional) |
| macOS 11+ (Apple Silicon + Intel, universal) | `open-synth-<ver>-macos-universal.dmg` | A `.pkg` with AU → `/Library/Audio/Plug-Ins/Components`, VST3 → `/Library/Audio/Plug-Ins/VST3`, CLAP → `/Library/Audio/Plug-Ins/CLAP`, app → `/Applications` |
| Linux x86_64 | `open-synth-<ver>-linux-x86_64.tar.gz` | Extract it and run `./install.sh`: VST3 → `~/.vst3`, CLAP → `~/.clap`, standalone → `~/.local/opt/open-synth` with an `open-synth` launcher and a desktop entry. `./install.sh --uninstall` removes it all |
| Linux x86_64 | `open-synth-<ver>-x86_64.AppImage` | Standalone only: `chmod +x` it, then run it |

Check a download with `sha256sum -c SHA256SUMS --ignore-missing` (on macOS, `shasum -a 256 -c SHA256SUMS --ignore-missing`).

The sample library (pianos, drums, bass, guitars) ships inside each plugin and app, so nothing has to be copied by hand.
Open Synth also looks in `%APPDATA%\OpenSynth\samples` (Windows), `~/.config/OpenSynth/samples` (Linux),
`~/Library/OpenSynth/samples` (macOS) and in the shared `C:\ProgramData\OpenSynth\samples` and `/Library/OpenSynth/samples`.

Development builds from `master` are on the [Actions tab](https://github.com/synthalorian/open-synth/actions/workflows/build.yml) as workflow artifacts. You need a GitHub login to download them, and they are kept for 14 days.

### Unsigned builds: first launch

Releases are **unsigned** until signing secrets are configured (see below), so the OS will warn you the first time:

- **Windows SmartScreen:** "Windows protected your PC" → click **More info** → **Run anyway**.
- **macOS Gatekeeper:** right-click (or Control-click) the `.pkg` → **Open** → **Open**. On macOS 15 and later, go to
  **System Settings → Privacy & Security** and click **Open Anyway**. If a plugin or the app is still blocked after
  installing, clear the quarantine flag:
  ```sh
  xattr -dr com.apple.quarantine "/Applications/Open Synth.app" \
    "/Library/Audio/Plug-Ins/Components/Open Synth.component" \
    "/Library/Audio/Plug-Ins/VST3/Open Synth.vst3" "/Library/Audio/Plug-Ins/CLAP/Open Synth.clap"
  ```
- **Linux:** no signing is involved.

### Enabling code signing (maintainers)

The signing steps in CI only run when the matching repository secrets exist (Settings → Secrets and variables → Actions).
Without them the steps are skipped and the build stays green.

| Secret | Used for |
|--------|----------|
| `WINDOWS_CERT_PFX` | Base64 of the Authenticode code-signing `.pfx` (`base64 -w0 cert.pfx`) |
| `WINDOWS_CERT_PASSWORD` | Password for that `.pfx` |
| `APPLE_DEVELOPER_ID_CERT_P12` | Base64 of a `.p12` holding the *Developer ID Application* and *Developer ID Installer* certificates and their keys |
| `APPLE_DEVELOPER_ID_CERT_PASSWORD` | Password for that `.p12` |
| `APPLE_DEVELOPER_ID_APPLICATION` | Signing identity, e.g. `Developer ID Application: Your Name (TEAMID)` |
| `APPLE_DEVELOPER_ID_INSTALLER` | Installer identity, e.g. `Developer ID Installer: Your Name (TEAMID)` |
| `APPLE_ID`, `APPLE_TEAM_ID`, `APPLE_APP_PASSWORD` | `notarytool` credentials (Apple ID email, team ID, app-specific password). When all three are set, the DMG is notarized and stapled |

On Windows, the exe, VST3, CLAP and the installer are signed with `signtool` (SHA-256, DigiCert timestamp). On macOS, each
bundle is signed with the hardened runtime, the `.pkg` is signed with the Installer identity, and the `.dmg` is signed and,
when credentials are present, notarized.

## Architecture

```
┌─────────────────────────────────────────┐
│           JUCE Plugin Editor            │
│  (Knobs, keyboard, scope, preset UI)    │
└─────────────────────────────────────────┘
                   │
┌─────────────────────────────────────────┐
│        OpenSynthProcessor               │
│  (APVTS, MIDI routing, state mgmt)      │
└─────────────────────────────────────────┘
                   │
┌─────────────────────────────────────────┐
│        SynthEngineWrapper               │
│  (JUCE audio buffer bridge)             │
└─────────────────────────────────────────┘
                   │
┌─────────────────────────────────────────┐
│           SynthEngine                   │
│  16 parts → VoiceAllocator → FX Engine  │
│  Arpeggiator + DrumKit + Recorder       │
└─────────────────────────────────────────┘
```

## Building

### Prerequisites
- CMake 3.22+
- C++20 compiler (GCC 12+, Clang 15+, MSVC 2022+)
- [JUCE](https://github.com/juce-framework/JUCE) — resolved in this order: `-DJUCE_DIR=/path/to/JUCE`, the `JUCE_DIR` environment variable, a `libs/JUCE` submodule, or `~/.juce`

### Linux
```bash
git clone --recurse-submodules https://github.com/synthalorian/open-synth.git
cd open-synth
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR=/path/to/JUCE
make -j$(nproc)
```

**Important:** use `--recurse-submodules` (or `git submodule update --init` after cloning). The CC0 sample libraries (pianos, drums, bass, guitars) live in git submodules under `samples/` — without them, sample-based presets will be silent.

The standalone binary, VST3, and CLAP plugins will be in `build/OpenSynth_artefacts/`.

### Tests
```bash
cd build
./sample_load_test ../samples          # all library WAVs decode (83/83)
./sample_player_block_test ../samples  # block processing, pitch bend, ADSR
./sample_static_test ../samples/manifests/splendid-grand-piano.json  # manifest load + clean audio
./fx_tests                             # 1,021 FX engine unit tests
./preset_browser_test                  # instrument browser UI logic (headless)
```

### Rendering the demo

`demo_render` drives the synth engine offline: it needs no audio device and no display. Run it from the repo root so
`samples/` resolves (`git submodule update --init samples/piano-splendid-grand` brings in the piano samples):
```bash
cmake --build build --target demo_render
./build/demo_render /tmp/demo.wav
ffmpeg -i /tmp/demo.wav -codec:a libmp3lame -q:a 2 docs/media/open-synth-demo.mp3
ffmpeg -i /tmp/demo.wav -codec:a libvorbis -q:a 5 docs/media/open-synth-demo.ogg
```

### macOS
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR=/path/to/JUCE
cmake --build . --config Release
```

### Windows
Requires Visual Studio 2022 (the "Desktop development with C++" workload), CMake 3.22+, and Git.
From a "Developer PowerShell for VS 2022":
```powershell
git clone --recurse-submodules https://github.com/synthalorian/open-synth.git
cd open-synth
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git ..\JUCE
cmake -S . -B build -DJUCE_DIR="$PWD\..\JUCE"
cmake --build build --config Release
.\build\Release\fx_tests.exe
```
Outputs land in `build\OpenSynth_artefacts\Release\` (`Standalone\Open Synth.exe`, `VST3\Open Synth.vst3`,
`CLAP\Open Synth.clap`). With the Visual Studio generator, test executables are under `build\Release\`.
This is the same configuration the `windows-x86_64` CI job builds on every push.

## Roadmap

- [x] Core synth engine (dual osc, filter, envelopes)
- [x] Wavetable + physical modeling
- [x] Full MFX chain (22 FX types)
- [x] Arpeggiator + drum kit + rhythm patterns
- [x] 16-part multitimbrality
- [x] User presets + setlist mode
- [x] App state persistence + MIDI input
- [x] Preset library — 5,600 presets across 20+ categories
- [x] Sample playback / ROMpler layer (CC0 library: pianos, drums, bass, guitars)
- [x] CLAP plugin format
- [ ] **In Progress**: Instrument realism system (per-category body models)
- [ ] MPE (MIDI Polyphonic Expression) support
- [ ] iOS / Android standalone apps

## License

Copyright (C) 2026 Blackshield Company.

Open Synth is licensed under the GNU Affero General Public License v3.0 only (SPDX: AGPL-3.0-only) — see [LICENSE](LICENSE). This is because JUCE is used under its AGPLv3 option. Releases up to and including 2.2.0 were published under Apache-2.0.

Third-party components and their licenses are listed in [NOTICE](NOTICE).

Bundled sample libraries (git submodules under `samples/`) keep their own licenses (CC0, public domain, CC-BY-4.0, GPL-2.0-or-later with a composition exception); see [NOTICE](NOTICE).

## Credits


---

*"This is the wave."* ⚫🦞

---

## ☕ Support the Developer

If this project saved you time, solved a problem, or just made your day a little more neon, you can fuel the next one:

[![Buy Me A Coffee](https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png)](https://buymeacoffee.com/synthalorian)
