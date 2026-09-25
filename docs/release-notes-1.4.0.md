# NSeqArpKeys 1.4.0

## Forte-set transposition

- In Melodic mode, Transpose now moves each note by positions in the selected
  Forte set. Every generated note remains in the chosen pitch-class collection.
  For the diatonic set, +1 moves C to D, and +7 moves C up an octave.
- Negative values move down through the set. Notes that move outside the MIDI
  range 0-127 are omitted.
- Transpose by key uses these same set-degree offsets on melodic assignments.
  Rhythmic mode continues to transpose drum pitches by semitones.
- Existing projects, presets, and pattern-bank files remain readable. Their
  saved melodic transpose values now represent Forte-set degrees, so a project
  using a nonzero melodic transpose may sound different from version 1.3.1.

The plug-in keeps the same host parameters and project state format as 1.3.1.

## Fourth Atlas banks

- The plug-in includes Fourth Atlas's twelve 1,000-pattern banks and a
  120-pattern starter bank. Select one in the Pattern Bank browser; the banks
  are available without importing files.
- Every included pattern has a zero transpose value, so no bank migration is
  needed for the new melodic transpose behavior. The bank file format stays
  at version 1.
