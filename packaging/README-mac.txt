LFO Sequencer @VERSION@ for macOS
=================================

Up to 4 slow LFOs that play MIDI notes and send MIDI CC to your synths.
Free and open source (GNU AGPLv3).

Needs: macOS 12 or later (Apple silicon or Intel) and a VST3 host such as
Ableton Live.


1. INSTALL
----------
Copy "LFO Sequencer.vst3" into your VST3 folder:

    ~/Library/Audio/Plug-Ins/VST3

In Finder: choose Go > Go to Folder..., paste the line above, press Return,
then drag "LFO Sequencer.vst3" into that window. (If the VST3 folder doesn't
exist, create it.)

Do step 2 before you open your music software.


2. LET MACOS OPEN IT (one time only)
------------------------------------
This plugin is NOT notarised by Apple and is NOT signed with an Apple
Developer ID. It is a free release with an "ad hoc" signature, so the first
time it loads macOS blocks it with a message like "LFO Sequencer.vst3 Not
Opened" or "Apple could not verify...". This is expected. Unblock it with
ONE of these:

  A) Terminal - quickest
     Open Terminal (Applications > Utilities > Terminal), paste this line and
     press Return:

       xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"LFO Sequencer.vst3"

     Nothing is printed when it works.

  B) No Terminal
     Open your music software and let it scan. When macOS says the plugin was
     not opened, click Done. Then open System Settings > Privacy & Security,
     scroll down to Security, and click "Open Anyway" next to the message
     about LFO Sequencer. Confirm with your password, then quit and reopen
     your music software and rescan.

Already saw the warning and it still won't load? Delete the plugin from the
VST3 folder, copy it again from this zip, run the line in A, then open your
music software.

Only do this for software you trust. The full source code is public; see
the link at the end.


3. USE IT IN ABLETON LIVE
-------------------------
1. Live > Settings (Preferences in older versions) > Plug-Ins: turn on
   "Use VST3 Plug-In System Folders", then click Rescan.
2. Load LFO Sequencer on a MIDI track.
3. On a second MIDI track with your instrument, set "MIDI From" to the
   LFO Sequencer track, and choose "LFO Sequencer" in the box below it.
4. Arm or monitor the instrument track. Double-click any value box to type
   a new value.


LICENCE AND SOURCE
------------------
Copyright (c) 2026 Lever Studios. Licensed under the GNU Affero General
Public License v3.0; see LICENSE.txt. No warranty.
Source code: https://github.com/georgeeeeeeeeeeeeee/LFOSequencer/tree/v@VERSION@
