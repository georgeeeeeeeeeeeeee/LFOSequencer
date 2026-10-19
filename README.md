# LFO Sequencer VST3

An 8-channel LFO-to-MIDI sequencer plugin for Ableton Live (and any VST3 host). Each channel generates a low-frequency oscillator that sends MIDI note triggers at wave peaks and continuous MIDI CC values.

## Features

- **8 independent LFO channels**, each with:
  - 6 wave shapes: Sine, Square, Triangle, Saw, Random (smoothly interpolated), Sample & Hold
  - Frequency range: 0.001 Hz (16 min cycle) to 20 Hz
  - Configurable MIDI note output (note number, channel, velocity range, note length)
  - Configurable MIDI CC output (CC number, channel)
  - Independent enable/disable for note and CC output
  - Peak threshold control for note trigger sensitivity
  - Real-time waveform visualisation

- **State saving** — all parameters saved with your Ableton project
- **MIDI effect** — routes through Ableton's MIDI chain, no audio I/O

## Architecture

```
LFOChannel.h     — Single LFO: wave generation, peak detection, MIDI output
LFOEngine.h      — Manages 8 channels, delegates processing
PluginProcessor  — JUCE AudioProcessor, state save/load
PluginEditor     — UI: waveform displays, rotary knobs, toggles
```

## Building

### Prerequisites

1. **JUCE Framework** (v7.x recommended)
   ```bash
   git clone https://github.com/juce-framework/JUCE.git ~/JUCE
   ```

2. **CMake** 3.22 or later

3. **C++ compiler** with C++17 support:
   - macOS: Xcode command line tools (`xcode-select --install`)
   - Windows: Visual Studio 2019+ with C++ workload
   - Linux: GCC 9+ or Clang 10+

### Build

```bash
# Set JUCE path (default: ~/JUCE)
export JUCE_PATH=~/JUCE

# Run the build script
./build.sh
```

Or manually:

```bash
mkdir build && cd build
cmake .. -DJUCE_PATH=~/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Install

Copy the generated `.vst3` to your plugin folder:

| Platform | Path |
|----------|------|
| macOS    | `~/Library/Audio/Plug-Ins/VST3/` |
| Windows  | `C:\Program Files\Common Files\VST3\` |
| Linux    | `~/.vst3/` |

Then rescan plugins in Ableton Live (Options → Preferences → Plug-ins).

## Usage in Ableton Live

1. Create a MIDI track
2. Add "LFO Sequencer" as a MIDI effect
3. Route the MIDI output to another track's instrument
4. Configure each LFO channel:
   - Set **Hz** for cycle speed (try 0.05–0.5 for slow generative patterns)
   - Choose a **wave shape**
   - Set the **Note** to trigger and **CC#** to modulate
   - Use **MIDI channel** to target different instruments
   - Adjust **velocity range** for dynamic variation
   - Set **note length** for how long each trigger sustains

### Routing Tips

- **Single instrument modulation**: Set all channels to the same MIDI channel, different CC numbers → modulate multiple parameters of one synth
- **Multi-instrument triggering**: Set channels to different MIDI channels → trigger different instruments from one plugin
- **Generative patterns**: Use prime-number frequency ratios (e.g., 0.1, 0.13, 0.17, 0.23 Hz) for non-repeating polyrhythmic patterns
- **Ambient drones**: Very low frequencies (0.005–0.02 Hz) with long note lengths create slowly evolving harmonic fields

## Parameters Per Channel

| Parameter | Range | Description |
|-----------|-------|-------------|
| Active    | On/Off | Enable/disable this channel |
| Wave      | Sine/Square/Triangle/Saw/Random/S&H | LFO wave shape |
| Hz        | 0.001–20 | LFO frequency |
| Note      | 0–127 (C-1 to G9) | MIDI note to trigger at peak |
| Ch        | 1–16 | MIDI channel |
| CC#       | 0–127 | MIDI CC number for continuous output |
| VelMin    | 0–127 | Minimum velocity |
| VelMax    | 0–127 | Maximum velocity |
| NoteLen   | 10–5000 ms | Note on duration |
| Thresh    | 0.1–1.0 | Peak detection threshold |
| Note enable | On/Off | Enable MIDI note output |
| CC enable | On/Off | Enable MIDI CC output |

## License

Personal use. Built with JUCE under their personal license terms.
