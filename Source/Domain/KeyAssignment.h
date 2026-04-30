#pragma once

#include <vector>
#include <string>
#include <sstream>
#include "../Pcs12.h"

/**
 * KeyAssignment – everything assigned to one trigger MIDI key.
 *
 * Replaces the three parallel maps that previously lived in PluginProcessor
 * (keyPatterns / keyChannels / keyForteNumbers). One object per MIDI key (0–127)
 * is stored in the processor's std::array<KeyAssignment, 128>.
 */
struct KeyAssignment
{
    /** Integer sequence (GateRunner-style). Negative values select notes in the
     *  opposite direction from the pitch-class base offset. */
    std::vector<int> sequence;

    /** Parsed Pcs12 pitch-class set (the "Forte set" for this key). */
    Pcs12 forte;

    /** Forte number string kept for serialization / display, e.g. "5-35.05". */
    std::string forteString;

    /** MIDI output channel (1–16). */
    int   channel    = 1;

    /** Octave base (0–10).  Maps the bit-index offset in the GateRunner algorithm:
     *  base position in the expanded scale = octave * forte.getK(). */
    int   octave     = 4;

    /** Gate fraction (0–1): how much of each step duration the note is held on. */
    float gate       = 0.5f;

    /** Length factor (1–400, expressed as a fraction here, i.e. 1.0 = 100 %). */
    float lengthFactor = 1.0f;

    KeyAssignment()
        : sequence(16, 0)
    {}

    // -------------------------------------------------------------------------
    // Convenience helpers
    // -------------------------------------------------------------------------

    /** Convert the current sequence to a space-separated string. */
    std::string sequenceToString() const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < sequence.size(); ++i)
        {
            if (i > 0) oss << ' ';
            oss << sequence[i];
        }
        return oss.str();
    }

    /** Parse a space-separated integer string and store as the sequence.
     *  Allows negative numbers (they invert the GateRunner bit-mapping direction).
     *  If parsing yields an empty result the existing sequence is preserved. */
    void setSequenceFromString(const std::string& s)
    {
        std::istringstream iss(s);
        std::vector<int> result;
        int n;
        while (iss >> n)
            result.push_back(n);
        if (!result.empty())
            sequence = result;
    }

    /** Parse a Forte-number string (e.g. "5-35.05") and update forte / forteString.
     *  Silently keeps the existing forte when the string is invalid. */
    void setForteFromString(const std::string& str)
    {
        if (str.empty()) return;
        try
        {
            forte       = Pcs12::parseForte(str);
            forteString = str;
        }
        catch (...) {}
    }

    bool hasValidForte() const { return !forte.isEmpty(); }
};
