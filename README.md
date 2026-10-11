# Open Synth

[![Build](https://github.com/synthalorian/open-synth/actions/workflows/build.yml/badge.svg)](https://github.com/synthalorian/open-synth/actions/workflows/build.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-green)](LICENSE)
[![C++20](https://img.shields.io/badge/standard-C%2B%2B20-blue)](README.md)

![Open Synth](assets/social_preview.png)

A free, open-source software synthesizer built with JUCE and C++20. Inspired by the Roland Juno-Di workflow — instant playability, rich sound, zero menu diving.

## What It Is

Open Synth is a **standalone + VST3 synthesizer plugin** designed for musicians who want to plug in and play. It combines a modern C++ DSP engine with a clean JUCE-based UI, delivering everything from classic analog-style leads to evolving wavetable pads — without forcing you through nested menus.

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

## Download

Prebuilt binaries for **Windows (x86_64)**, **macOS (universal)**, and **Linux (x86_64)** are attached to every
[GitHub Release](https://github.com/synthalorian/open-synth/releases/latest), built by the
[Build workflow](.github/workflows/build.yml). Each platform ships three archives: `standalone`, `vst3`, and `clap`.
Bleeding-edge builds from `master` are available as workflow artifacts on the [Actions tab](https://github.com/synthalorian/open-synth/actions/workflows/build.yml) (requires a GitHub login, kept 14 days).

### Windows install

1. Download `open-synth-windows-x86_64-standalone.zip` (and/or the `-vst3.zip` / `-clap.zip`) from the latest release.
2. **Standalone:** extract anywhere and run `Open Synth.exe`. Keep the `samples\` folder next to the `.exe`.
3. **VST3:** extract `Open Synth.vst3` into `C:\Program Files\Common Files\VST3\`.
4. **CLAP:** extract `Open Synth.clap` into `C:\Program Files\Common Files\CLAP\`.
5. **Samples for plugins:** in a DAW, Open Synth looks for its sample library in `%APPDATA%\OpenSynth\samples\`
   (the VST3/CLAP archives do not include samples). Copy the `samples\` folder from the standalone zip there,
   so you end up with `%APPDATA%\OpenSynth\samples\manifests\...`. Without it, sample-based presets
   (pianos, drums, bass, guitars) are silent; synth presets work regardless.

Notes: the binaries are not code-signed, so Windows SmartScreen may warn on first launch ("More info" → "Run anyway").
Windows 10/11 x64 only; there is no ARM64 build or installer yet.

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

Apache License 2.0 — see [LICENSE](LICENSE).

Bundled sample libraries are CC0 / public domain (see `samples/` submodule sources: freepats, sfzinstruments).

## Credits


---

*"This is the wave."* ⚫🦞

---

## ☕ Support the Developer

If this project saved you time, solved a problem, or just made your day a little more neon, you can fuel the next one:

[![Buy Me A Coffee](https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png)](https://buymeacoffee.com/synthalorian)
