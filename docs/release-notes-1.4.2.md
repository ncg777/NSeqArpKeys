# NSeqArpKeys 1.4.2

## Stuck-note fixes

- Balances every generated MIDI note-on with a note-off. Repeated notes on
  the same channel and pitch now release the previous note immediately before
  retriggering it, preventing voice buildup in instruments that stack note-ons.
  This covers overlapping gates, long Fixed Steps, shared pitches across keys,
  and duplicate rhythmic drum pitches.
- Keeps shared pitches active until their final pattern owner releases them.
  Key release, Stop Key, Stop All, latch changes, clearing or replacing a
  pattern, and restoring a preset/project release the appropriate notes.
- Stops all patterns on a playing-to-stopped DAW transport transition and on
  incoming MIDI All Notes Off (CC123) or All Sound Off (CC120), including
  latched patterns and bank auditions. MIDI panic follows the message's sample
  offset and applies to all trigger assignments, regardless of input channel.
  Manual playback and audition still work while the transport is stopped.
- Preserves outstanding output note-offs during host reset, resource release,
  and playback reinitialization, delivering them on the next nonempty audio
  callback. The internal preview is silenced during lifecycle resets.

Adds regression coverage using a receiver that counts every note-on and
note-off, plus checks for preview silence, shared-pitch ownership, channel
changes, and sample-accurate stop events.

Existing projects, presets, pattern banks, parameter IDs, and timing settings
remain compatible. Same-pitch overlaps now explicitly retrigger one MIDI
voice instead of accumulating multiple unreleased voices.
