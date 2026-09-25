# NSeqArpKeys 1.3.0

## What changed

- Global Steps/QN and each key's 0-to-inherit override remain compatible with
  existing host automation and projects.
- Rhythmic patterns now accept 1-16 drum pitches. One numeric Velocity
  bits/lane parameter sets the same 1-7-bit width for every pitch, matching
  GateRunner. Positive decimal masks span all 16 seven-bit lanes. The default
  of one bit preserves existing binary masks.
- The pitch and velocity step lanes have been removed. Existing presets still
  load their other settings; old lane data is ignored on restore.
- The editor shows controls relevant to the current mode, the pattern's item
  count, a short step preview and running-key indicators. It uses a new dark
  palette with pattern colours.
- Cut, clear and selective paste join copy and independent duplicate. The
  searchable Pattern Bank supports colour, tags, favourites, auditioning,
  independent copy and shared link assignment. A keyboard context menu can
  assign the selected bank entry. Make independent breaks a link.
- Whole presets embed shared pattern definitions and assignment mappings.
  A setup reopens without access to the local Pattern Bank.
- Text input, file import and state restore are bounded. Invalid integer
  edits remain unapplied; oversized pattern and preset files are rejected.
  XML declarations that define entities, excessive nesting and excessive
  element or attribute counts are rejected before parsing.
- Linked-pattern undo/redo preserves each assignment, and timing edits keep
  linked keys in phase. Joining a bank link retains the project's edited
  definition. Audition stops when leaving the bank or closing the editor.
- Step previews follow rotation and reversal across the full sequence.
  Switching modes preserves large rhythmic values through save and restore.

## Compatibility

The plug-in keeps the same global host parameter IDs. Old rhythmic patterns
with one-bit lane masks retain their pitch mapping. Legacy presets default to
16 pitches and one shared velocity bit per pitch. Because the removed pitch and
velocity step lanes have no 1.3.0 equivalent, their values do not affect
playback after migration. Save a copy of a 1.2.0 preset before editing it in
1.3.0 if those lane values matter to an older project.

Rhythmic sequence values are positive decimal masks up to 112 bits. Legacy
negative signed 32-bit masks retain their two's-complement interpretation. In
rhythmic mode, 0 is silent and each nonzero lane value is scaled to MIDI
velocity 1-127 before base and trigger velocity scaling.

## Formats

Windows, Linux and macOS receive VST3. macOS also receives Audio Units.
Generated MIDI routing still depends on the host.
