#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <array>
#include <charconv>
#include <limits>
#include <tuple>
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
    enum class Mode { melodic, rhythmic };

    /** Integer sequence (GateRunner-style). Negative values select notes in the
     *  opposite direction from the pitch-class base offset. */
    std::vector<int> sequence;

    /** Zero follows the existing global subdivision, 1–16 overrides it. */
    int subdivision = 0;
    Mode mode = Mode::melodic;
    std::string name;
    /** MIDI notes for bits 0–15 in rhythmic mode. */
    std::array<int, 16> drumNotes { 36, 38, 42, 46, 41, 43, 45, 47,
                                    48, 50, 49, 51, 39, 37, 54, 56 };
    int transpose = 0;
    int velocity = 100;
    std::vector<int> velocitySteps;
    std::vector<int> pitchSteps;
    int rotation = 0;
    bool reverse = false;
    /** Source key of an independent range copy (not a live link). */
    int rootKey = -1;

    /** Parsed Pcs12 pitch-class set (the "Forte set" for this key). */
    Pcs12 forte;

    /** Forte number string kept for serialization / display, e.g. "5-35.05". */
    std::string forteString;

    /** MIDI output channel (1–16). */
    int   channel    = 1;

    /** Octave base (0–10).  Maps the bit-index offset in the GateRunner algorithm:
     *  base position in the expanded scale = octave * forte.getK(). */
    int   octave     = 4;

    /** Gate fraction (0–2) of the span through following rests. */
    float gate       = 0.5f;

    /** Duration added to the gated span, in denominator-based steps (0–16). */
    float fixedLengthSteps = 0.0f;

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
     *  Whitespace-only input clears the sequence. Invalid input is ignored. */
    static bool parseIntegerSequence(const std::string& s, std::vector<int>& values,
                                    int minimum = std::numeric_limits<int>::min(),
                                    int maximum = std::numeric_limits<int>::max())
    {
        std::istringstream iss(s);
        std::vector<int> result;
        std::string token;
        while (iss >> token)
        {
            int n = 0;
            auto* begin = token.data();
            auto* end = begin + token.size();
            if (*begin == '+') ++begin;
            if (begin == end || (*begin == '-' && token.front() == '+')) return false;
            const auto parsed = std::from_chars(begin, end, n);
            if (parsed.ec != std::errc{} || parsed.ptr != end
                || n < minimum || n > maximum || result.size() >= 4096)
                return false;
            result.push_back(n);
        }
        values = std::move(result);
        return true;
    }

    bool setSequenceFromString(const std::string& s)
    {
        return parseIntegerSequence(s, sequence);
    }

    bool operator==(const KeyAssignment& other) const
    {
        const auto fields = [](const KeyAssignment& a) {
            return std::tie(a.sequence, a.subdivision, a.mode, a.name, a.drumNotes,
                            a.transpose, a.velocity, a.velocitySteps, a.pitchSteps,
                            a.rotation, a.reverse, a.rootKey, a.forteString, a.channel,
                            a.octave, a.gate, a.fixedLengthSteps);
        };
        return fields(*this) == fields(other);
    }

    /** Parse a Forte-number string (e.g. "5-35.05") and update forte / forteString.
     *  Silently keeps the existing forte when the string is invalid. */
    void setForteFromString(const std::string& str)
    {
        if (str.empty())
        {
            forte = Pcs12();
            forteString.clear();
            return;
        }
        try
        {
            forte       = Pcs12::parseForte(str);
            forteString = str;
        }
        catch (...)
        {
            // Silently keep the existing forte when the string is invalid.
            // This is intentional: the UI may supply partial input while the
            // user is still typing.  Errors will be visible because the
            // forte display will not update.
        }
    }

    bool hasValidForte() const { return !forte.isEmpty(); }

    int effectiveSubdivision(int global) const { return subdivision > 0 ? subdivision : global; }
};
