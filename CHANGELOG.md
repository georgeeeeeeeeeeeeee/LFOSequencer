# Changelog

All notable changes to LFO Sequencer are recorded here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versioning: [SemVer](https://semver.org/).

## [1.0.0] - 2026-10-20

First public release, for macOS 12+ (universal: Apple silicon and Intel) and 64-bit Windows 10/11.

- 4 LFO channels, each playing MIDI notes when its wave crosses a threshold and sending MIDI CC as it moves
- 6 wave shapes, rates from 0.01 Hz to 100 Hz, phase offsets with Sync and Spread
- Settings are saved with your project
- Ready-to-install zips for macOS and Windows, each with an install guide
- Every build is checked with pluginval at its strictest level and Steinberg's VST3 validator

The macOS plugin is ad-hoc signed and not notarised, so macOS asks you to allow it once; see the install guide.

The sections below list what changed since the private build of 14 Apr 2026.

### Changed (you may hear these)
- **Phase offset now applies to everything.** With a non-zero offset:
  - Saw and S&H notes fire at the saw's visible reset and the S&H's latch point, instead of at the un-offset cycle start.
  - The Random shape no longer jumps once per cycle.
  - The display's playhead sits on the part of the wave being played.
- **Threshold near 1.0 now fires once per cycle.** Before, a threshold at or near 1.0 on Sine or Triangle fired only when a sample happened to land exactly on the peak. Depending on the rate this was every cycle, some cycles or none at all (for example, Triangle at 7.3 Hz never fired). The threshold is now capped just below the peak, by a margin that depends on the rate.
- **Changing CC# or MIDI channel, or switching CC back on, sends the current CC value immediately.** Before, nothing was sent until the wave value changed, which could take minutes on slow Square or S&H channels.

With phase offset 0 and a threshold below about 0.99, notes are identical to the previous version event for event. This was tested on all 6 shapes at several rates.

### Fixed
- **Stuck notes.** Every way a sounding note can be dropped now sends its note-off:
  - turning a channel off, or unticking Note;
  - changing the note or MIDI channel while a note is on (the note-off goes to the note that was actually started);
  - Sync and Reset All;
  - the host re-preparing the plugin;
  - host bypass.
- **Spurious notes** when the phase offset or wave shape is changed while running.
- **Invalid event positions.** Empty processing blocks no longer produce MIDI events past the end of the block.
- **Typing `nan` or `inf`** into Hz, Phase or Thr no longer breaks the channel. Non-finite input keeps the previous value.
- **Session loading:**
  - every value is range-checked, so a damaged or hand-edited session can't produce invalid MIDI;
  - missing values fall back to that channel's normal defaults.
- **UI and audio thread safety:**
  - settings are shared through atomics;
  - Sync and Reset All are carried out on the audio thread;
  - the audio thread no longer copies incoming MIDI every block.
- **UI refresh.** The value boxes update after Spread and after loading a session while the window is open.
- **Minimum window height** is 680 px, so the second row of boxes can't disappear.
- **Steinberg VST3 validator:** the default program now has a name ("Default"), which the validator requires.

### Removed
- MIDI Learn. Its button was removed in April; the leftover engine code is now gone too.

### Project
- **Location and history:**
  - moved from `~/Downloads/LFOSequencer` to `~/Projects/LFOSequencer`;
  - put under git;
  - added `.gitignore`.
- **Docs:** added the original brief (`docs/BRIEF-original.md`) and the recovered Win95 UI design plan (`docs/DESIGN.md`).
- **Licence:** now AGPLv3. The copyright notice in the plugin metadata and README says so.
- **Plugin description** now says 4-channel.
- **Build:**
  - the macOS-only post-build step is wrapped in `if(APPLE)` so Windows builds don't fail;
  - JUCE 8.0.12 is downloaded automatically at a pinned version (`-DJUCE_PATH` still selects a local copy), and `build.sh` no longer needs JUCE installed;
  - macOS builds are ad-hoc signed as the last build step.
- **Windows:** the Visual C++ runtime is linked statically, so the plugin loads on PCs without the Visual C++ Redistributable installed. CI fails if the plugin ever depends on those runtime DLLs again.
- **CI:** a GitHub Actions workflow builds macOS (universal) and Windows (x64), runs pluginval at strictness 10 with the Steinberg VST3 validator, and also checks the Intel half of the Mac build under Rosetta.
- **Release packages:** CI builds `LFOSequencer-<version>-mac.zip` and `-win.zip`, each containing the plugin, an install guide (`README.txt`, from `packaging/`) and the licence. Pushing a `v*` tag that matches the version in `CMakeLists.txt` drafts a GitHub release with both zips and SHA-256 checksums.
- **README** rewritten for people installing the plugin, including the macOS Gatekeeper step.
- **LICENSE:** the full AGPLv3 text, verbatim from gnu.org.

### Validation
- pluginval 1.0.4 passes at strictness 5 and 10, with the Steinberg VST3 validator (SDK 3.8.1) enabled.

## Planned for 1.1
- Channels 5–8. The engine already runs 8 channels, but only 4 are shown in 1.0.
- Tempo sync and follow-transport. In 1.0 the LFOs run freely and keep playing while the host is stopped.
- Host-automatable parameters.
