# NSeqArpKeys 1.5.0

Generate and compare variations from a snapshot of the selected key, then assign
the results across the keyboard or save them as a reusable Pattern Bank family.

- Polynomial permutations with quadratic, cubic, rotation and custom coefficient
  controls. Mappings are validated for the selected sequence length; inverse
  mappings and permutation of nonzero positions only are supported.
- Vertical reflection over the sequence-wide occupied bit bounds. For example,
  `2 6 8` becomes `8 12 2`. Melodic signs are preserved; drum lanes move with
  their complete encoded velocity levels, including 112-bit masks.
- Repeated applications across consecutive keys, with a starting application,
  stride, cycle length and repeated-pattern indicators. Generated assignments
  are independent copies, and the full batch is one Undo/Redo transaction.
- Original/variation bit grids, long-pattern paging, audition comparison and a
  playhead driven by actual playback. Existing rotation/reversal and Preview
  sound settings are respected. Leaving the page or editor stops its audition.
- Named and tagged variation families saved atomically into the user bank,
  using the existing `.nseqpattern` format and ordinary bank export workflow.

Existing projects, host parameter IDs, presets and pattern-bank formats remain
compatible. Operators are baked into sequence values outside playback; the
audio callback only publishes the current step for the grid playhead.

Legacy negative drum masks become equivalent unsigned masks when reflected.
Unrepresentable melodic flips are rejected rather than truncating notes. Project
restore invalidates the source snapshot and requires reopening the variation page.

Seeded coefficient generation, general operator chains and MIDI capture/export
remain future work.
