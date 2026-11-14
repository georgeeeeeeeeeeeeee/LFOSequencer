# LFO Sequencer

A free VST3 plugin for macOS and Windows: up to 4 slow LFOs that play MIDI notes when the wave crosses a threshold and send MIDI CC as it moves. Point it at your synths for evolving, generative patterns.

![Platforms](https://img.shields.io/badge/platforms-macOS%20%7C%20Windows-lightgrey) ![Format](https://img.shields.io/badge/format-VST3-blue) ![License](https://img.shields.io/badge/license-AGPLv3-green)

## Download

Get the latest zip for your computer from the **[Releases page](https://github.com/georgeeeeeeeeeeeeee/LFOSequencer/releases/latest)**:

- **macOS** (12 or later, Apple silicon and Intel): `LFOSequencer-1.0.0-mac.zip`
- **Windows** (10 or 11, 64-bit): `LFOSequencer-1.0.0-win.zip`

Each zip holds the plugin, a short install guide (`README.txt`) and the licence. There's nothing to compile.

## Install on macOS

1. Unzip, then copy **LFO Sequencer.vst3** into `~/Library/Audio/Plug-Ins/VST3`.
   In Finder, choose **Go › Go to Folder…**, paste that path and press Return, then drag the plugin in. Create the `VST3` folder if it doesn't exist.
2. **Let macOS open it.** Do this before you open your music software (see below).
3. Open your music software and rescan plugins.

### The macOS security warning

LFO Sequencer is **not notarised by Apple and not signed with an Apple Developer ID**. It's a free release with an ad-hoc signature. So the first time it loads, macOS blocks it with a message like *"LFO Sequencer.vst3" Not Opened* or *Apple could not verify…*. That's expected. Unblock it once with either:

**A. Terminal (quickest).** Open Terminal (Applications › Utilities), paste this line and press Return. Nothing is printed when it works.

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"LFO Sequencer.vst3"
```

**B. Without Terminal.** Open your music software and let it scan. When macOS says the plugin wasn't opened, click **Done**. Then go to **System Settings › Privacy & Security**, scroll to **Security**, and click **Open Anyway** next to the LFO Sequencer message. Confirm with your password, then quit and reopen your music software and rescan.

If you've already seen the warning and the plugin still won't load: delete it from the VST3 folder, copy it again from the zip, run the line in A, then open your music software.

Only do this for software you trust. The full source is in this repository.

## Install on Windows

1. Right-click the zip and choose **Extract All…**.
2. Copy the whole **LFO Sequencer.vst3** folder into `C:\Program Files\Common Files\VST3`. Windows asks for administrator permission; click **Continue**.
3. Open your music software and rescan plugins.

No installer and no extra runtimes are needed.

## Using it in Ableton Live

1. In **Settings › Plug-Ins** (Preferences in older versions), turn on **Use VST3 Plug-In System Folders** and click **Rescan**.
2. Load **LFO Sequencer** on a MIDI track.
3. Create a second MIDI track with the instrument you want to play. In its MIDI settings, set **MIDI From** to the LFO Sequencer track, and pick **LFO Sequencer** in the box below it.
4. Arm or monitor the instrument track and the notes will play.

Set each channel up by double-clicking a value box, typing a value and pressing Enter:

- Set **Hz** for cycle speed (try 0.05–0.5 for slow generative patterns)
- Choose a **wave shape**
- Set the **Note** to trigger (a number or a name like `C4` or `F#2`) and the **CC#** to modulate
- Use **Chan** to target different instruments
- Adjust **V Min / V Max** for velocity and CC range
- Set **Len ms** for how long each note sustains

LFOs run freely in Hz. They don't follow the host tempo or transport, and they keep playing while the transport is stopped.

### Tips

- **One synth, many parameters**: put every channel on the same MIDI channel with different CC numbers
- **Several instruments**: put channels on different MIDI channels
- **Patterns that rarely repeat**: use prime-number rate ratios, e.g. 0.1, 0.13, 0.17, 0.23 Hz
- **Ambient drones**: very low rates (0.01–0.02 Hz) with long notes give slowly evolving harmonic fields

## Features

- **4 independent LFO channels**, each with:
  - 6 wave shapes: Sine, Square, Triangle, Saw, Random (smoothly interpolated), Sample & Hold
  - Rate from 0.01 Hz (a 100-second cycle) to 100 Hz
  - Phase offset, to stagger channels against each other
  - MIDI notes (note, channel, velocity range, length) and MIDI CC (number, same V Min–V Max range)
  - Independent on/off for notes and CC
  - Live waveform display with playhead and threshold line
- **Sync** restarts every channel at its phase offset, **Spread** spaces the active channels' offsets evenly, and **Reset All** restarts everything
- All settings are saved with your project

### Parameters per channel

| Parameter | Range | Description |
|-----------|-------|-------------|
| ON        | On/Off | Enable/disable this channel |
| Wave      | Sine/Square/Triangle/Saw/Random/S&H | LFO wave shape |
| Hz        | 0.01–100 | LFO frequency |
| Phase     | 0–360° | Phase offset |
| Note      | 0–127 (C-1 to G9) | MIDI note to trigger |
| Chan      | 1–16 | MIDI channel for notes and CC |
| Thr       | 0.00–1.00 | Level the wave must rise through to fire a note (Sine, Triangle, Random). Square fires on its rising edge; Saw and S&H fire once per cycle |
| CC#       | 0–127 | MIDI CC number for continuous output |
| V Min     | 0–127 | Minimum note velocity and CC value |
| V Max     | 0–127 | Maximum note velocity and CC value |
| Len ms    | 1–5000 | Note length |
| Vel       | display | Velocity a note would get right now |
| Note / CC | On/Off | Enable note output / CC output |

## Building from source

You only need this if you want to change the code.

**Requirements:**
- CMake 3.22 or later, and git
- A C++17 compiler: the Xcode command line tools on macOS (`xcode-select --install`), or Visual Studio 2022 with the C++ workload on Windows

JUCE 8.0.12 is downloaded automatically during configuration.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The plugin ends up in `build/LFOSequencer_artefacts/Release/VST3/`.
- On macOS it's a universal (arm64 + x86_64) binary for macOS 12 and later, ad-hoc signed.
- To build against a local JUCE checkout, add `-DJUCE_PATH=/path/to/JUCE` to the first command.

Every push is built and checked on macOS and Windows by GitHub Actions. The checks run [pluginval](https://github.com/Tracktion/pluginval) at its strictest level, along with Steinberg's VST3 validator. Pushing a `v*` tag also drafts a GitHub release with both zips.

### Source layout

```
Source/LFOChannel.h    — one LFO: wave generation, threshold detection, MIDI output
Source/LFOEngine.h     — manages the channels, Sync/Spread/Reset
Source/PluginProcessor — JUCE AudioProcessor, state save/load
Source/PluginEditor    — UI: waveform displays, editable value boxes, Win95 look
packaging/             — install guides shipped inside the release zips
```

## Licence

Copyright © 2026 Lever Studios. LFO Sequencer is free software, licensed under the **GNU Affero General Public License v3.0**; see [`LICENSE`](LICENSE). It comes with no warranty.

If you distribute a modified version, the AGPL requires you to share its source code under the same licence.

Built with [JUCE](https://juce.com) 8, used under the AGPLv3. VST is a registered trademark of Steinberg Media Technologies GmbH.
