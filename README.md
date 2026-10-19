# LFO Sequencer VST3

A 4-channel LFO-to-MIDI sequencer plugin for Ableton Live (and any VST3 host). Each channel runs a low-frequency oscillator that sends MIDI note triggers when the wave crosses a threshold, plus continuous MIDI CC.

## Features

- **4 independent LFO channels**, each with:
  - 6 wave shapes: Sine, Square, Triangle, Saw, Random (smoothly interpolated), Sample & Hold
  - Frequency from 0.01 Hz (100-second cycle) to 100 Hz
  - Phase offset, to stagger channels against each other
  - MIDI note output: note, channel, velocity range, note length
  - MIDI CC output: CC number, scaled to the same V Min–V Max range
  - Independent on/off for note and CC output
  - Threshold that sets where on the wave notes fire
  - Live waveform display with playhead and threshold line
- **Sync** restarts every channel at its phase offset; **Spread** spaces the active channels' offsets evenly; **Reset All** restarts everything
- **State saving**: all settings are saved with your project

LFOs run freely in Hz; they don't follow the host tempo or transport.

## Architecture

```
LFOChannel.h     — Single LFO: wave generation, threshold detection, MIDI output
LFOEngine.h      — Manages the channels, Sync/Spread/Reset
PluginProcessor  — JUCE AudioProcessor, state save/load
PluginEditor     — UI: waveform displays, editable value boxes, Win95 look
```

## Building

### Prerequisites

1. **CMake** 3.22 or later, and **git**
2. **C++ compiler** with C++17 support:
   - macOS: Xcode command line tools (`xcode-select --install`)
   - Windows: Visual Studio 2022 with the C++ workload

JUCE 8.0.12 is downloaded automatically during configuration.

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

To build against a local JUCE checkout instead, add `-DJUCE_PATH=/path/to/JUCE` to the first command.

On macOS this produces a universal (arm64 + x86_64) binary for macOS 12 and later, ad-hoc signed.

### Install

Copy the generated `LFO Sequencer.vst3` from `build/LFOSequencer_artefacts/Release/VST3/` to your plugin folder:

| Platform | Path |
|----------|------|
| macOS    | `~/Library/Audio/Plug-Ins/VST3/` |
| Windows  | `C:\Program Files\Common Files\VST3\` |

Then rescan plugins in your host.

## Using it in Ableton Live

1. Load **LFO Sequencer** on a MIDI track.
2. Create a second MIDI track with the instrument you want to play.
3. In that track's MIDI settings, set **MIDI From** to the LFO Sequencer track, and pick **LFO Sequencer** in the box below it.
4. Arm or monitor the instrument track and the notes will play.

## Usage

Configure each channel by double-clicking a value box, typing a value and pressing Enter:

- Set **Hz** for cycle speed (try 0.05–0.5 for slow generative patterns)
- Choose a **wave shape**
- Set the **Note** to trigger (a number or a name like `C4` or `F#2`) and the **CC#** to modulate
- Use **Chan** to target different instruments
- Adjust **V Min / V Max** for velocity and CC range
- Set **Len ms** for how long each note sustains

### Tips

- **Single instrument modulation**: set all channels to the same MIDI channel with different CC numbers to modulate several parameters of one synth
- **Multi-instrument triggering**: set channels to different MIDI channels
- **Generative patterns**: use prime-number frequency ratios (e.g. 0.1, 0.13, 0.17, 0.23 Hz) for patterns that take a long time to repeat
- **Ambient drones**: very low frequencies (0.01–0.02 Hz) with long note lengths give slowly evolving harmonic fields

## Parameters Per Channel

| Parameter | Range | Description |
|-----------|-------|-------------|
| ON        | On/Off | Enable/disable this channel |
| Wave      | Sine/Square/Triangle/Saw/Random/S&H | LFO wave shape |
| Hz        | 0.01–100 | LFO frequency |
| Phase     | 0–360° | Phase offset |
| Note      | 0–127 (C-1 to G9) | MIDI note to trigger |
| Chan      | 1–16 | MIDI channel for notes and CC |
| Thr       | 0.00–1.00 | Threshold the wave must rise through to fire a note (Sine, Triangle, Random). Square fires on its rising edge; Saw and S&H fire once per cycle |
| CC#       | 0–127 | MIDI CC number for continuous output |
| V Min     | 0–127 | Minimum note velocity and CC value |
| V Max     | 0–127 | Maximum note velocity and CC value |
| Len ms    | 1–5000 | Note length |
| Vel       | display | Velocity a note would get right now |
| Note / CC | On/Off | Enable note output / CC output |

## License

Copyright © 2026 Lever Studios. Licensed under the GNU Affero General Public License v3.0 (AGPLv3). See `LICENSE`.

Built with [JUCE](https://juce.com), used under the AGPLv3.
