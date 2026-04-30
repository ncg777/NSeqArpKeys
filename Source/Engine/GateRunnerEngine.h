#pragma once

#include <vector>
#include <string>
#include "../Domain/KeyAssignment.h"

/**
 * GateRunnerEngine – pure musical transform layer.
 *
 * This is the C++ equivalent of the core algorithm in gaterunner/cli/generate.ts.
 * It is stateless (all methods are static) and has no JUCE dependency, making it
 * independently testable.
 *
 * The algorithm (mirroring generate.ts):
 *  1. Build a sorted MIDI-note scale from the Forte pitch-class set by expanding
 *     every pitch class across all valid octaves (0–10, keeping note < 128).
 *  2. For each integer value in the sequence, extract the bits of |value| (LSB
 *     first).  The sign of value determines the index direction relative to the
 *     base offset (octave × pitchClassCount).
 *  3. Scale notes whose index satisfies the bit-test become active for that step.
 */
class GateRunnerEngine
{
public:
    /** Parse a whitespace-separated integer string.  Negative values are allowed
     *  and represent reverse-direction bit-mapping in computeStepNotes(). */
    static std::vector<int> parseSequence(const std::string& text);

    /** Build a sorted MIDI-note scale from a Forte pitch-class set.
     *  Each pitch class p is expanded to p, p+12, p+24, … while the result < 128. */
    static std::vector<int> buildScale(const Pcs12& forte);

    /**
     * Compute the set of MIDI notes active for one step.
     *
     * @param scale           Sorted MIDI-note scale produced by buildScale().
     * @param pitchClassCount Number of pitch classes (forte.getK()).
     * @param stepValue       The integer at this step position.
     * @param octave          Base octave for the bit-index calculation.
     *
     * The GateRunner bit-mapping (from generate.ts):
     *   bits        = abs(stepValue) in binary, LSB first
     *   sign        = sign(stepValue)   (0 produces no notes)
     *   baseOffset  = octave × pitchClassCount
     *   active(idx) = (sign × (idx − baseOffset)) ∈ [0, bits.size())
     *                 AND bits[sign × (idx − baseOffset)] == 1
     */
    static std::vector<int> computeStepNotes(const std::vector<int>& scale,
                                             int pitchClassCount,
                                             int stepValue,
                                             int octave);

    /** Convenience: precompute all step note lists for a full KeyAssignment. */
    static std::vector<std::vector<int>> computeAllSteps(const KeyAssignment& assignment);
};
