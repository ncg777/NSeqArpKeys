#pragma once

#include "KeyAssignment.h"
#include <algorithm>
#include <map>
#include <numeric>
#include <optional>

// Editor-side transformations. Results are ordinary, independently saved
// assignments; no operators, parsing or mapping work runs in playback.
namespace PatternVariations
{
enum class Operation { polynomial, verticalFlip };

struct Polynomial
{
    int cubic = 0, quadratic = 2, linear = 1, constant = 0;
};

struct Options
{
    Operation operation = Operation::polynomial;
    Polynomial polynomial;
    bool keepRests = false;
    bool inverse = false;
    int firstKey = 60, keyCount = 8;
    int firstApplication = 0, applicationsBetweenKeys = 1;
};

struct Variation
{
    int key = 60, application = 0;
    int repeatsKey = -1;
    KeyAssignment assignment;
};

struct Result
{
    std::string error;
    std::vector<Variation> variations;
    std::optional<uint64_t> order; // Eventual period of the operator's powers.
    bool permutation = false;
    int transientApplications = 0;
    int distinctPatterns = 0;
    bool valid() const { return error.empty() && !variations.empty(); }
};

inline bool isRest(const SequenceValue& value)
{
    return std::all_of(value.words.begin(), value.words.end(), [](uint32_t word) { return word == 0; });
}

inline int modulo(int64_t value, int length)
{
    return static_cast<int>((value % length + length) % length);
}

// Horner evaluation reduces after each operation, so even extreme signed
// coefficients cannot overflow for the supported 4096-step sequences.
inline int evaluate(const Polynomial& p, int index, int length)
{
    int64_t value = modulo(p.cubic, length);
    for (const int coefficient : { p.quadratic, p.linear, p.constant })
        value = modulo(value * index + modulo(coefficient, length), length);
    return static_cast<int>(value);
}

inline bool compile(const Polynomial& p, int length, bool inverse,
                    std::vector<int>& map, std::string& error)
{
    if (length < 0 || length > 4096) { error = "A mapping supports at most 4096 steps."; return false; }
    std::vector<int> result(static_cast<size_t>(length));
    std::vector<bool> used(static_cast<size_t>(length));
    bool permutation = true;
    for (int i = 0; i < length; ++i)
    {
        const int target = evaluate(p, i, length);
        if (used[static_cast<size_t>(target)]) permutation = false;
        used[static_cast<size_t>(target)] = true;
        result[static_cast<size_t>(i)] = target;
    }
    if (inverse)
    {
        if (!permutation)
        {
            error = "Inverse is available only for a permutation at length " + std::to_string(length) + ".";
            return false;
        }
        auto forward = result;
        for (int i = 0; i < length; ++i) result[static_cast<size_t>(forward[static_cast<size_t>(i)])] = i;
    }
    map = std::move(result);
    error.clear();
    return true;
}

struct MappingDynamics
{
    std::vector<std::vector<int>> cycles;
    int transientApplications = 0;
};

// Remove all tails before walking the remaining cycles. The longest tail is
// the first application from which powers of the entire mapping repeat.
inline MappingDynamics dynamicsOf(const std::vector<int>& map)
{
    MappingDynamics result;
    std::vector<int> incoming(map.size()), depth(map.size()), pending;
    for (const auto target : map) ++incoming[static_cast<size_t>(target)];
    for (int i = 0; i < static_cast<int>(map.size()); ++i)
        if (incoming[static_cast<size_t>(i)] == 0) pending.push_back(i);
    for (size_t i = 0; i < pending.size(); ++i)
    {
        const auto index = static_cast<size_t>(pending[i]);
        const auto target = static_cast<size_t>(map[index]);
        depth[target] = std::max(depth[target], depth[index] + 1);
        result.transientApplications = std::max(result.transientApplications, depth[target]);
        if (--incoming[target] == 0) pending.push_back(static_cast<int>(target));
    }
    std::vector<bool> visited(map.size());
    for (int first = 0; first < static_cast<int>(map.size()); ++first)
    {
        if (incoming[static_cast<size_t>(first)] == 0 || visited[static_cast<size_t>(first)]) continue;
        auto& cycle = result.cycles.emplace_back();
        int index = first;
        do
        {
            visited[static_cast<size_t>(index)] = true;
            cycle.push_back(index);
            index = map[static_cast<size_t>(index)];
        } while (index != first);
    }
    return result;
}

inline std::optional<uint64_t> orderOf(const std::vector<std::vector<int>>& groups)
{
    uint64_t order = 1;
    for (const auto& cycle : groups)
    {
        const auto length = static_cast<uint64_t>(cycle.size());
        const auto reduced = order / std::gcd(order, length);
        if (reduced > std::numeric_limits<uint64_t>::max() / length) return std::nullopt;
        order = reduced * length;
    }
    return order;
}

inline std::array<uint32_t, 4> melodicMagnitude(const SequenceValue& value)
{
    const int signedValue = value.melodicValue();
    return { signedValue < 0 ? 0u - static_cast<uint32_t>(signedValue)
                            : static_cast<uint32_t>(signedValue), 0, 0, 0 };
}

inline uint32_t magnitudeBit(const SequenceValue& value, int bit)
{
    const auto words = melodicMagnitude(value);
    return (words[static_cast<size_t>(bit / 32)] >> (bit % 32)) & 1u;
}

inline std::string decimalOf(std::array<uint32_t, 4> words)
{
    std::string digits;
    do
    {
        uint64_t remainder = 0;
        for (int i = 3; i >= 0; --i)
        {
            const uint64_t word = (remainder << 32) | words[static_cast<size_t>(i)];
            words[static_cast<size_t>(i)] = static_cast<uint32_t>(word / 10);
            remainder = word % 10;
        }
        digits.push_back(static_cast<char>('0' + remainder));
    } while (std::any_of(words.begin(), words.end(), [](uint32_t word) { return word != 0; }));
    std::reverse(digits.begin(), digits.end());
    return digits;
}

inline int cellCount(const KeyAssignment& assignment)
{
    return assignment.mode == KeyAssignment::Mode::melodic ? 32
         : std::clamp(assignment.drumLaneCount, 1, 16);
}

inline uint32_t cellValue(const KeyAssignment& assignment, const SequenceValue& value, int cell)
{
    return assignment.mode == KeyAssignment::Mode::melodic ? magnitudeBit(value, cell)
         : value.bitsAt(cell * std::clamp(assignment.drumVelocityBits, 1, 7),
                        std::clamp(assignment.drumVelocityBits, 1, 7));
}

inline std::pair<int, int> occupiedBounds(const KeyAssignment& assignment)
{
    int low = cellCount(assignment), high = -1;
    for (const auto& value : assignment.sequence)
        for (int cell = 0; cell < cellCount(assignment); ++cell)
            if (cellValue(assignment, value, cell) != 0)
            {
                low = std::min(low, cell);
                high = std::max(high, cell);
            }
    return { low, high };
}

inline bool flip(const KeyAssignment& source, KeyAssignment& output, std::string& error)
{
    if (source.mode == KeyAssignment::Mode::melodic)
        for (const auto& value : source.sequence)
            if (!value.fitsMelodicInt())
            { error = "Vertical flip needs valid melodic integers. Switch wide masks to Rhythmic."; return false; }
    const auto [low, high] = occupiedBounds(source);
    auto result = source;
    if (high < low) { output = std::move(result); error.clear(); return true; }
    const bool melodic = source.mode == KeyAssignment::Mode::melodic;
    const int width = melodic ? 1 : std::clamp(source.drumVelocityBits, 1, 7);
    for (size_t step = 0; step < source.sequence.size(); ++step)
    {
        // Preserve inactive packed lanes too. Only the occupied, configured
        // lane interval moves; velocity words are never reversed internally.
        auto words = melodic ? std::array<uint32_t, 4>{} : source.sequence[step].words;
        for (int cell = low; cell <= high; ++cell)
            for (int bit = 0; bit < width; ++bit)
            {
                const int position = cell * width + bit;
                words[static_cast<size_t>(position / 32)] &= ~(1u << (position % 32));
            }
        for (int cell = low; cell <= high; ++cell)
        {
            const uint32_t level = cellValue(source, source.sequence[step], cell);
            const int target = (low + high - cell) * width;
            for (int bit = 0; bit < width; ++bit)
                if ((level & (1u << bit)) != 0)
                    words[static_cast<size_t>((target + bit) / 32)] |= 1u << ((target + bit) % 32);
        }
        auto text = decimalOf(words);
        if (melodic && source.sequence[step].negative && !isRest(source.sequence[step])) text.insert(text.begin(), '-');
        SequenceValue value;
        if (!SequenceValue::parse(text, value) || (melodic && !value.fitsMelodicInt()))
        { error = "The reflected mask exceeds the melodic signed 32-bit range."; return false; }
        result.sequence[step] = std::move(value);
    }
    if (result.sequenceToString().size() > 45056)
    { error = "The reflected sequence exceeds the pattern text limit."; return false; }
    output = std::move(result);
    error.clear();
    return true;
}

inline Result build(const KeyAssignment& source, const Options& options)
{
    Result result;
    if (source.sequence.empty() || source.sequence.size() > 4096)
    { result.error = "Choose a nonempty source pattern with at most 4096 steps."; return result; }
    if (source.sequenceToString().size() > 45056)
    { result.error = "The source exceeds the pattern text limit."; return result; }
    if (options.firstKey < 0 || options.firstKey > 127 || options.keyCount < 1
        || options.keyCount > 128 - options.firstKey || options.firstApplication < 0
        || options.firstApplication > 65536 || options.applicationsBetweenKeys < 1
        || options.applicationsBetweenKeys > 4096)
    { result.error = "Choose keys within MIDI 0-127 and valid application counts."; return result; }

    std::vector<size_t> positions;
    std::vector<int> map;
    std::vector<std::vector<int>> powers;
    KeyAssignment flipped;
    if (options.operation == Operation::polynomial)
    {
        for (size_t step = 0; step < source.sequence.size(); ++step)
            if (!options.keepRests || !isRest(source.sequence[step])) positions.push_back(step);
        if (!compile(options.polynomial, static_cast<int>(positions.size()), options.inverse, map, result.error))
            return result;
        const auto dynamics = dynamicsOf(map);
        result.transientApplications = dynamics.transientApplications;
        result.permutation = dynamics.transientApplications == 0;
        result.order = orderOf(dynamics.cycles);
        // Binary lifting makes large starting applications and strides cheap,
        // including maps with tails and collisions.
        const int maximumApplication = options.firstApplication + (options.keyCount - 1) * options.applicationsBetweenKeys;
        powers.push_back(map);
        for (int remaining = maximumApplication; remaining > 1; remaining >>= 1)
        {
            const auto& previous = powers.back();
            std::vector<int> next(map.size());
            for (size_t i = 0; i < map.size(); ++i) next[i] = previous[static_cast<size_t>(previous[i])];
            powers.push_back(std::move(next));
        }
    }
    else
    {
        if (!flip(source, flipped, result.error)) return result;
        result.order = flipped.sequence == source.sequence ? 1 : 2;
    }
    std::map<std::string, int> seen;
    for (int index = 0; index < options.keyCount; ++index)
    {
        Variation variation;
        variation.key = options.firstKey + index;
        variation.application = options.firstApplication + index * options.applicationsBetweenKeys;
        variation.assignment = source;
        variation.assignment.linkId.clear();
        if (options.operation == Operation::verticalFlip)
        {
            if (variation.application % 2 != 0) variation.assignment.sequence = flipped.sequence;
        }
        else
            for (size_t i = 0; i < positions.size(); ++i)
            {
                int from = static_cast<int>(i);
                for (int remaining = variation.application, bit = 0; remaining != 0; remaining >>= 1, ++bit)
                {
                    if ((remaining & 1) != 0) from = powers[static_cast<size_t>(bit)][static_cast<size_t>(from)];
                }
                variation.assignment.sequence[positions[i]] = source.sequence[positions[static_cast<size_t>(from)]];
            }
        const auto sequence = variation.assignment.sequenceToString();
        if (sequence.size() > 45056)
        {
            result.error = "The variation at application " + std::to_string(variation.application)
                         + " exceeds the pattern text limit.";
            result.variations.clear();
            return result;
        }
        const auto found = seen.find(sequence);
        if (found != seen.end()) variation.repeatsKey = found->second;
        else seen.emplace(sequence, variation.key);
        result.variations.push_back(std::move(variation));
    }
    result.distinctPatterns = static_cast<int>(seen.size());
    return result;
}
}
