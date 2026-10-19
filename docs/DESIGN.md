<!--
Recovered design plan for the Win95 editor rewrite (originally ~/.claude/plans/adaptive-snacking-toast.md,
written 14 Apr 2026, deleted by Claude Code's automatic cleanup on 19 Oct 2026). Text below is verbatim.
Historical: the Learn button and the "permanent install rule" no longer apply.
-->

# Complete GUI Rewrite: Win95 Aesthetic, 4 Channels, Editable Text Fields

## Context
Full from-scratch rewrite of PluginEditor.h and PluginEditor.cpp. Previous editor had 8 channels with label boxes. New design: Win95 look-and-feel class, 4 displayed channels (engine keeps 8), editable Courier New label boxes for all parameters, live waveform display with red playhead, Win95LookAndFeel class overriding toggle/combo/button/label drawing.

**Permanent install rule** — after every successful build, output these four lines verbatim:
```
sudo rm -rf /Library/Audio/Plug-Ins/VST3/LFO\ Sequencer.vst3
sudo cp -R ~/Downloads/LFOSequencer/build/LFOSequencer_artefacts/Release/VST3/LFO\ Sequencer.vst3 /Library/Audio/Plug-Ins/VST3/
sudo xattr -cr /Library/Audio/Plug-Ins/VST3/LFO\ Sequencer.vst3
sudo codesign --force --deep --sign - /Library/Audio/Plug-Ins/VST3/LFO\ Sequencer.vst3
```

---

## Files Modified
- `Source/PluginEditor.h` — complete rewrite
- `Source/PluginEditor.cpp` — complete rewrite
- No changes to LFOEngine.h, LFOChannel.h, PluginProcessor.h/cpp, CMakeLists.txt

---

## Key Decisions
- `LFOEngine::NUM_CHANNELS` stays 8; editor defines `static constexpr int DISPLAY_CHANNELS = 4` and wires channels 0–3
- `Win95LookAndFeel` inherits `juce::LookAndFeel_V4`, stored as member of `LFOSequencerEditor`, set via `setLookAndFeel(&win95LF)` in constructor; `setLookAndFeel(nullptr)` in destructor
- Waveform display precomputes 2 full cycles from `ch.computeWaveValue()` and draws a red vertical playhead at x = `fmod(ch.phase, 1.0) / 2.0 * width` (phase 0–1 maps across the left half of the 2-cycle view, then wraps)

---

## Window Layout
- Size: `setSize(680, 680)`, `setResizeLimits(580, 600, 1000, 900)`
- Title bar: 22px — navy bg, white bold "LFO Sequencer - Lever Studios", 3 decorative boxes (min/max/close) on right
- Menu bar: 18px — silver bg, static text "File  Edit  View  Help" (non-functional)
- 4 channel panels stacked, each ~138px:
  - Header row: 22px
  - Waveform display: 48px
  - Parameter row 1: 40px (Hz | Phase | Note | Ch | Thr)
  - Parameter row 2: 40px (CC# | V Min | V Max | Len ms | Vel)
  - Padding: remaining px
- Footer buttons: 30px — Sync, Spread, Reset All
- Status bar: 18px — "4-CH LFO MIDI Sequencer" left, "MIDI out active" right

---

## Win95LookAndFeel Class (declared in PluginEditor.h, inherits LookAndFeel_V4)

**`drawButtonBackground`** — fill `#c0c0c0`. Draw 2-pixel bevel:
- not pressed (raised): top+left outer `#ffffff`, inner `#dfdfdf`; bottom+right outer `#404040`, inner `#808080`
- pressed (sunken): swap

**`drawToggleButton`** — draw 13×13 sunken bevel box at left-centre. Fill `#ffffff` inside. If checked, draw black polyline checkmark. Draw button text to the right in black at 11px.

**`drawComboBox`** — fill interior `#ffffff`. Draw sunken bevel around full bounds. Draw 16px-wide raised bevel button on right with "▼" in black 10px centred.

**`drawLabel`** — fill `#ffffff` if editable, else transparent. Let JUCE draw the text.

---

## LFOWaveformDisplay (PluginEditor.h / .cpp)

```cpp
class LFOWaveformDisplay : public juce::Component, public juce::Timer {
public:
    LFOWaveformDisplay(LFOChannel& ch);
    void paint(juce::Graphics& g) override;
    void timerCallback() override;  // repaint() at 30Hz
private:
    LFOChannel& ch;
};
```

**paint():**
1. Draw sunken bevel around `getLocalBounds()`
2. Fill inner area `#ffffff`
3. Draw `#cccccc` horizontal centre line
4. Precompute polyline: for x in [0, width), `phase2 = (float(x)/width) * 2.0`, `val = ch.computeWaveValue(fmod(phase2, 1.0))`, `y = midY - val * halfH`. Stroke navy if active, grey if not.
5. Playhead: `playX = fmod(ch.phase, 1.0) * 0.5 * width` — draw red vertical line
6. If noteEnabled+active: draw red horizontal line at threshold y

---

## LFOChannelPanel (PluginEditor.h / .cpp)

Members:
```
LFOChannel& ch; int index;
LFOWaveformDisplay waveDisplay;
ToggleButton activeBtn{"ON"}, noteEnableBtn{"Note"}, ccEnableBtn{"CC"};
ComboBox waveSelector;
TextButton midiLearnBtn{"Learn"};
// Row 1: title labels + value boxes
Label freqLabel, phaseLabel, noteLabel, chanLabel, threshLabel;
Label freqBox, phaseBox, noteBox, chanBox, thrBox;
// Row 2: title labels + value boxes
Label ccLabel, velMinLabel, velMaxLabel, noteLenLabel, velLabel;
Label ccBox, velMinBox, velMaxBox, lenBox, velBox;
```

**setupTitle(lbl, text):** 10px font, black, centred, non-editable, no background

**setupBox(box, text):** Courier New 12px, white bg (`#ffffff`), black text, transparent outline (bevel drawn in paint()), `setEditable(false, true, false)`

**onTextChange handlers:**
- freqBox: clamp 0.01–100.0, format 3dp → `ch.frequencyHz`
- phaseBox: strip non-numeric, clamp 0–360, format `"Xdeg"` → `ch.phaseOffset = deg/360`
  (display degree symbol U+00B0 via `juce::String::charToString(0x00B0)`)
- noteBox: call `parseNoteInput()` → if valid set `ch.midiNote`, display `midiNoteName()`; else revert
- chanBox: clamp 1–16 → `ch.midiChannel`
- thrBox: clamp 0.00–1.00, 2dp → `ch.peakThreshold`
- ccBox: clamp 0–127 → `ch.ccNumber`
- velMinBox: clamp 0–127 → `ch.velocityMin`
- velMaxBox: clamp 0–127 → `ch.velocityMax`
- lenBox: clamp 1–5000 → `ch.noteLengthMs`
- velBox: display-only (set from timerCallback in editor), shows last computed velocity

**paint():** Fill silver. Raised bevel border. Navy/grey badge (22px wide) on left with channel number in white bold.
Draw sunken bevel manually around each box (since outline colour is transparent to avoid double borders).

**resized():**
```
bounds = getLocalBounds().reduced(2)
bounds.removeFromLeft(22)  // badge
header = bounds.removeFromTop(22)  → activeBtn(40) | waveSelector(80) | noteBtn(50) | ccBtn(40) | learnBtn(50)
wave   = bounds.removeFromTop(48)  → waveDisplay
row1   = bounds.removeFromTop(40)  → 5 cells (w/5 each): [title 10px top] [box remaining]
row2   = bounds                    → 5 cells (w/5 each)
```

**updateMidiLearnState():** `ccBox.setText(ch.ccNumber)`, `chanBox.setText(ch.midiChannel)`, reset midiLearnBtn text+colours.

---

## LFOSequencerEditor (PluginEditor.h / .cpp)

```cpp
class LFOSequencerEditor : public juce::AudioProcessorEditor, public juce::Timer {
    static constexpr int DISPLAY_CHANNELS = 4;
    Win95LookAndFeel win95LF;
    juce::OwnedArray<LFOChannelPanel> channelPanels;
    juce::TextButton syncBtn{"Sync"}, spreadBtn{"Spread"}, resetBtn{"Reset All"};
    ...
};
```

**Constructor order:**
1. `setLookAndFeel(&win95LF)`
2. Build 4 panels (channels 0–3), addAndMakeVisible
3. Setup buttons, addAndMakeVisible
4. `startTimerHz(10)`
5. `setOpaque(true)`
6. `setResizable(true, true)`
7. `setResizeLimits(580, 600, 1000, 900)`
8. `setSize(680, 680)` ← LAST

**Destructor:** `setLookAndFeel(nullptr)` first (JUCE requirement when LF is a member).

**timerCallback():** Check midiLearnCompleted → call updateMidiLearnState on panel. Call repaint().

**paint():** Title bar navy, menu bar silver, sunken border around channel area, status bar sunken.

**resized():**
```
titleBar  = removeFromTop(22)    (drawn in paint, not laid out)
menuBar   = removeFromTop(18)
statusBar = removeFromBottom(18)
btnBar    = removeFromBottom(30) → 3 buttons evenly spaced
bounds.reduce(4, 2)
panelH = bounds.getHeight() / DISPLAY_CHANNELS
each panel → removeFromTop(panelH)
```

---

## Static Helpers (.cpp file scope)
- `draw3DBorder(g, r, raised)` — retained from existing code
- `midiNoteName(int)` — retained
- `parseNoteInput(String)` — retained

---

## Build & Verify
```
~/Applications/CMake.app/Contents/bin/cmake --build build --config Release 2>&1 | grep -E "error:|warning:|Built target"
```
1. Zero errors (pre-existing LFOChannel.h warnings are OK)
2. Install with 4 sudo commands
3. Ableton: loads, shows Win95 UI with 4 channel strips
4. Double-click any box → type value → Enter commits and clamps
5. Waveform animates with red playhead
6. Learn button: turns navy + "..." → receive CC → ccBox/chanBox update
