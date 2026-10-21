# NSeqArpKeys 1.6.0

Each trigger key can now reshape time with GateRunner's curves or a custom
expression. Generate variations can also combine a polynomial mapping and
automatic transposition across the same MIDI range.

- **Time warp** offers all 94 GateRunner curves, including power, circular,
  bounce, sequence, wobble and FM shapes, plus a curve preview.
- **Custom expression** accepts `T` (position 0-1 in a chunk), `PI`, `E`,
  arithmetic and powers, scalar math functions, and GateRunner's `seq`,
  `wform`, `stowform` and waveform helpers. Examples: `T^2`, `1-T`, or
  `T + sin(2*PI*T)/8`. An optional `Y =` prefix is accepted. Expressions are
  limited to 512 characters, 256 nodes and depth 32. Invalid edits retain the
  last valid assignment; invalid imported syntax and nonfinite evaluations
  fall back to identity timing.
- **Amount** blends original and warped timing from 0-100%. **Repeats**
  applies the curve within 1-64 equal chunks of the pattern. **Quantize**
  snaps warped positions to 1, 2, 4 or 8 divisions per original step, or Off.
  Disabled warp and 0% amount preserve original timing, including note lengths.
- **Warp note lengths** transforms the gated note endpoints inside the
  onset's chunk, clips the end to that chunk, and uses their absolute distance
  with a minimum duration of 0.5 ms when preparing playback. Turn it off to
  retain the original Gate + Fixed Steps lengths. Reversed and folded curves
  retain valid note releases, overlap ownership and fixed loop periods.
- In **Generate variations**, enable **Transpose by key** to add each
  destination's distance from the source key to the source's Transpose value.
  Melodic patterns use Forte-set degrees; rhythmic patterns use semitones.
  Starting application and Applications between keys still control polynomial
  powers independently. The list displays each result's transposition and
  identifies repeats using both its sequence and pitch offset.
- Preview, audition, range assignment, family saving and a single batch Undo
  include the combined result. Time warp travels with linked patterns,
  copies, Timing paste, bank files, presets and DAW projects.

Older files load with time warp disabled. Host parameter IDs, plug-in identity
and pattern/preset/bank format versions remain unchanged. Changes to tempo or
Steps/QN preserve phase and pending warped note-offs; editing warp settings
restarts that key like other pattern edits.
