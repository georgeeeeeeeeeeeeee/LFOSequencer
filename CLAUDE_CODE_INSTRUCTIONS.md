# LFO Sequencer VST3 — Claude Code Build Instructions

## What This Is

An 8-channel LFO-to-MIDI sequencer VST3 plugin for Ableton Live on macOS. Each channel runs an independent low-frequency oscillator that sends MIDI note triggers at wave peaks and continuous MIDI CC values. Designed for generative ambient music — slow evolving patterns from sub-Hz oscillators.

## Project Status

Complete JUCE C++ source code, ready to compile into a `.vst3` bundle for macOS.

## Architecture

```
CMakeLists.txt       — CMake build config, targets VST3 + Standalone
Source/
  LFOChannel.h       — Single LFO: 6 wave shapes, phase offset, peak detection, MIDI output
  LFOChannel.cpp     — (header-only)
  LFOEngine.h        — 8 channels, sync, spread, MIDI learn
  LFOEngine.cpp      — (header-only)
  PluginProcessor.h/cpp — Audio processor, state save/restore, MIDI learn passthrough
  PluginEditor.h/cpp    — UI: waveforms, rotary knobs, sync/spread/learn buttons
```

## Features Per Channel

- 6 wave shapes: Sine, Square, Triangle, Saw, Random (smooth interpolated), Sample & Hold
- Frequency: 0.001 Hz to 20 Hz
- **Phase offset**: 0° to 360° — stagger timing between channels
- MIDI note output: note number, channel 1-16, velocity min/max, note length
- MIDI CC output: CC number, channel, continuous 0-127
- Independent note/CC enable toggles
- Peak threshold for note trigger sensitivity
- **MIDI Learn**: click Learn, move a CC on your controller → auto-assigns CC number + channel
- Real-time waveform visualisation with peak flash and offset indicator

## Global Controls

- **Sync**: Resets all channel phases to zero simultaneously (offsets preserved, so channels restart at their offset position)
- **Spread**: Auto-distributes phase offsets evenly across active channels (e.g. 4 active → 0°, 90°, 180°, 270°)
- **Reset All**: Full reset of all phases and state

## Build Steps for macOS

### Prerequisites

1. Xcode command line tools:
   ```bash
   xcode-select --install
   ```

2. CMake:
   ```bash
   brew install cmake
   ```
   Or verify: `cmake --version`

3. Clone JUCE:
   ```bash
   git clone https://github.com/juce-framework/JUCE.git ~/JUCE
   ```

### Build

```bash
cd /path/to/LFOSequencer
mkdir -p build && cd build
cmake .. -DJUCE_PATH=~/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Install

```bash
find build -name "*.vst3" -type d
cp -r build/LFOSequencer_artefacts/Release/VST3/LFO\ Sequencer.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

### Verify in Ableton

1. Open Ableton Live → Settings → Plug-ins
2. Ensure "Use VST3" is enabled
3. Rescan or restart Ableton
4. Find "LFO Sequencer" in Plug-ins → VST3

## Ableton MIDI Routing

### Basic setup
1. Track 1: Load "LFO Sequencer" as MIDI effect
2. Track 2: Load an instrument (synth, sampler, etc.)
3. Set Track 1's "MIDI To" → Track 2

### MIDI CC mapping in Ableton
1. In LFO Sequencer, each channel outputs on a CC number (default CC 20-27)
2. In Ableton, press Cmd+M to enter MIDI Map mode
3. Click the parameter you want to control
4. Wiggle the CC on a hardware controller, OR
5. Use the LFO Sequencer's output CC → Ableton auto-detects it

### Using MIDI Learn (in-plugin)
1. Click "Learn" on a channel (button flashes)
2. Move a knob/slider on your MIDI controller
3. The channel auto-assigns to that CC number and MIDI channel
4. Now that channel's LFO controls that CC

### Multi-instrument routing
- Set channels to different MIDI channels (1-16)
- Route each MIDI channel to different instrument tracks
- Or use Ableton's "MIDI To" with channel filtering

## Design Direction

**Windows 95 meets clouds** — utilitarian 90s desktop chrome with dreamy atmospheric visuals.

- Win95 chrome: 2px beveled borders, system grey (#C0C0C0), navy title bar (#000080), sunken fields
- Clouds: sky-blue gradient hints bleeding through panels
- Waveforms: navy blue on light fields, rose/red peak flash
- Typography: monospace, small caps labels
- An HTML prototype is included as `lfo-sequencer-win95.html`

To implement in JUCE: create a custom `LookAndFeel` class overriding button, slider, and combobox drawing methods with Win95 border styles.

## Potential Build Issues

### "JUCE not found"
→ Verify `-DJUCE_PATH=~/JUCE` points to JUCE repo root (should contain `CMakeLists.txt`)

### Code signing errors
→ Add to CMakeLists.txt:
```cmake
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
```

### "No member named 'getChildWithTagNameIterator'"
→ Requires JUCE 7.x. For JUCE 6, replace with:
```cpp
forEachXmlChildElementWithTagName(*xml, chXml, "Channel")
```

### Plugin doesn't appear in Ableton
→ VST3 is a directory bundle on macOS. Ensure the whole `.vst3` directory was copied.
→ Try the Standalone build first: `open build/LFOSequencer_artefacts/Release/Standalone/LFO\ Sequencer.app`

### MIDI Learn not working
→ The plugin needs MIDI input routed to it. In Ableton, check that a MIDI controller is sending to the track. The plugin accepts MIDI input (acceptsMidi = true) and scans for CC messages when a channel is in learn mode.

## Extension Ideas

- Tempo sync (LFO rate locked to Ableton BPM — quarter notes, bars, etc.)
- Probability per channel (% chance of note firing at peak)
- Waveform drawing (custom user-drawn shapes)
- Preset save/load system
- Polyphonic mode (trigger chords instead of single notes)
- Swing/humanise timing offset
- BPM-synced delay on note triggers
