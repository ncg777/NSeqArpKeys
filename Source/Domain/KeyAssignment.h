#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <array>
#include <charconv>
#include <limits>
#include <tuple>
#include <cstdint>
#include "../Pcs12.h"

// A rhythmic step can address 16 lanes with seven bits each. Store its
// unsigned 112-bit mask without narrowing it to a C++ int. Negative values
// retain the legacy signed 32-bit interpretation.
struct SequenceValue
{
    std::array<uint32_t, 4> words {};
    std::string decimal = "0";
    bool negative = false;
    int signedValue = 0;

    SequenceValue() = default;
    SequenceValue(int value)
        : words { static_cast<uint32_t>(value), 0, 0, 0 },
          decimal(std::to_string(value)), negative(value < 0), signedValue(value) {}

    static bool parse(const std::string& token, SequenceValue& output)
    {
        if (token.empty()) return false;
        if (token[0] == '-')
        {
            int value = 0;
            const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
            if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) return false;
            output = SequenceValue(value);
            return true;
        }
        size_t start = token[0] == '+' ? 1 : 0;
        if (start == token.size()) return false;
        SequenceValue parsed;
        for (size_t i = start; i < token.size(); ++i)
        {
            const char digit = token[i];
            if (digit < '0' || digit > '9') return false;
            uint64_t carry = static_cast<uint64_t>(digit - '0');
            for (auto& word : parsed.words)
            {
                const uint64_t next = static_cast<uint64_t>(word) * 10 + carry;
                word = static_cast<uint32_t>(next);
                carry = next >> 32;
            }
            if (carry != 0 || (parsed.words[3] & 0xffff0000u) != 0) return false;
        }
        while (start + 1 < token.size() && token[start] == '0') ++start;
        parsed.decimal = token.substr(start);
        output = std::move(parsed);
        return true;
    }

    bool fitsMelodicInt() const
    {
        return negative || (words[1] == 0 && words[2] == 0 && words[3] == 0
                            && words[0] <= static_cast<uint32_t>(std::numeric_limits<int>::max()));
    }

    int melodicValue() const
    {
        if (negative) return signedValue;
        return fitsMelodicInt() ? static_cast<int>(words[0]) : 0;
    }

    uint32_t bitsAt(int offset, int width) const
    {
        if (offset < 0 || offset >= 112 || width < 1 || width > 7) return 0;
        const int word = offset / 32;
        const int shift = offset % 32;
        uint64_t bits = static_cast<uint64_t>(words[static_cast<size_t>(word)]) >> shift;
        if (shift + width > 32 && word < 3)
            bits |= static_cast<uint64_t>(words[static_cast<size_t>(word + 1)]) << (32 - shift);
        return static_cast<uint32_t>(bits) & ((1u << width) - 1u);
    }

    bool operator==(const SequenceValue& other) const
    {
        return negative == other.negative && words == other.words;
    }
};

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
    std::vector<SequenceValue> sequence;

    /** Zero follows the existing global subdivision, 1–16 overrides it. */
    int subdivision = 0;
    Mode mode = Mode::melodic;
    std::string name;
    /** MIDI notes for 1–16 rhythmic lanes. */
    std::array<int, 16> drumNotes { 36, 38, 42, 46, 41, 43, 45, 47,
                                    48, 50, 49, 51, 39, 37, 54, 56 };
    int drumLaneCount = 16;
    /** Shared bit width for every rhythmic lane, as in GateRunner. */
    int drumVelocityBits = 1;
    int transpose = 0;
    int velocity = 100;
    int rotation = 0;
    bool reverse = false;
    /** Same nonempty ID on multiple assignments means edits follow the link. */
    std::string linkId;
    /** Visual metadata travels with patterns and presets. */
    std::string colour = "#62D6C6";
    std::string tags;
    bool favourite = false;
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
            oss << sequence[i].decimal;
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
        if (s.size() > 45056) return false;
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

    bool setSequenceFromString(const std::string& s, bool allowWideValues = false)
    {
        if (s.size() > 45056) return false;
        std::istringstream iss(s);
        std::vector<SequenceValue> result;
        std::string token;
        while (iss >> token)
        {
            SequenceValue value;
            if (result.size() >= 4096 || !SequenceValue::parse(token, value)
                || (!allowWideValues && mode == Mode::melodic && !value.fitsMelodicInt())) return false;
            result.push_back(std::move(value));
        }
        sequence = std::move(result);
        return true;
    }

    bool operator==(const KeyAssignment& other) const
    {
        const auto fields = [](const KeyAssignment& a) {
            return std::tie(a.sequence, a.subdivision, a.mode, a.name, a.drumNotes,
                            a.drumLaneCount,
                            a.drumVelocityBits, a.transpose, a.velocity,
                            a.rotation, a.reverse, a.linkId, a.colour, a.tags,
                            a.favourite, a.rootKey, a.forteString, a.channel,
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

    bool hasValidDrumVelocityBits() const
    {
        return drumLaneCount >= 1 && drumLaneCount <= 16
            && drumVelocityBits >= 1 && drumVelocityBits <= 7;
    }

    int effectiveSubdivision(int global) const { return subdivision > 0 ? subdivision : global; }
};
