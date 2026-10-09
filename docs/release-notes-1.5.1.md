# NSeqArpKeys 1.5.1

Polynomial variations now accept every mapping modulo the pattern length,
including mappings that repeat some source steps and omit others.

- **Polynomial mapping** replaces the permutation-only operator. Output step
  `i` reads source step `p(i)`; repeated applications compose the mapping.
  For example, `2i mod 8` changes `1 2 3 4 5 6 7 8` to
  `1 3 5 7 1 3 5 7`, then `1 5 1 5 1 5 1 5`, then all ones.
- **Inverse permutation** remains available for mappings that use every source
  position exactly once. Changing to a mapping with repeats clears and disables
  inverse, keeping preview, audition, assignment and family saving available.
- The status identifies permutations and mappings with repeats. For general
  mappings it reports the application from which powers become periodic and
  their eventual cycle length. Repeated-pattern indicators still compare the
  generated sequences, which can repeat sooner when source values coincide.
- Large starting applications and strides work for both permutations and
  general mappings. **Keep zero steps in place** retains zeros and maps within
  the nonzero positions using their count as the modulus.
- Generated patterns are checked against the existing pattern text limit, so
  a mapping that repeats long integers cannot save an unreadable family.

Existing permutation results, vertical flips, projects, host parameters,
presets and pattern-bank formats remain compatible. Generated variations are
ordinary saved sequences; mapping calculations run outside playback.
