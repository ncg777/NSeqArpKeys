# NSeqArpKeys 1.2.0

## Formats and installation

This release ships **VST3 for Windows, Linux and macOS**, plus **Audio Units on macOS**.
The macOS build supports Intel and Apple Silicon. The standalone application has
been removed: its MIDI timing depended on audio-device access, which could conflict
with a DAW. Load NSeqArpKeys inside your DAW instead.

Copy the complete plug-in bundle to your platform's plug-in folder and rescan in
your DAW. Each archive includes the manual and SHA-256 checksums. macOS binaries
are not signed or notarized.

## What's new

- Per-key timing with independent Steps/QN and phase-preserving rate changes.
- Rhythmic bit masks mapped to 16 editable MIDI notes, without requiring a Forte set.
- Velocity and pitch lanes, transpose, rotation and reversal.
- Pattern names, copy/paste, duplication, range assignment and undo/redo.
- Individual pattern save/load alongside whole-preset files.
- Preview sound toggle: silence the internal tones while MIDI continues to another instrument.
- Switching modes preserves the selected MIDI output channel.

## Reliability and compatibility

- Improved shared-note ownership, muted-step handling and quiet-note velocities.
- Live edits preserve trigger velocity; timing changes preserve musical phase.
- State restore retains all pattern fields, including empty and all-rest loops.
- Legacy presets keep their timing defaults. Invalid integer input is rejected atomically.

Euclidean rhythm generation is not included. The roadmap retains the remaining
pattern-bank, modulation and performance milestones for future versions.

Generated MIDI routing depends on the host. The manual explains routing, rhythmic
mapping, expression lanes, and the preview sound control.
