#include "GateRunnerEngine.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// ---------------------------------------------------------------------------
std::vector<int> GateRunnerEngine::parseSequence(const std::string& text)
{
    std::vector<int> result;
    KeyAssignment::parseIntegerSequence(text, result);
    return result;
}

// ---------------------------------------------------------------------------
std::vector<int> GateRunnerEngine::buildScale(const Pcs12& forte)
{
    std::vector<int> scale;
    if (forte.isEmpty())
        return scale;

    Sequence pitches = forte.asSequence();

    for (int p : pitches)
    {
        for (int oct = 0; oct <= 10; ++oct)
        {
            int t = p + 12 * oct;
            if (t < 128)
                scale.push_back(t);
        }
    }

    std::sort(scale.begin(), scale.end());
    return scale;
}

// ---------------------------------------------------------------------------
std::vector<int> GateRunnerEngine::computeStepNotes(const std::vector<int>& scale,
                                                     int pitchClassCount,
                                                     int stepValue,
                                                     int octave)
{
    std::vector<int> notes;

    if (scale.empty() || pitchClassCount <= 0 || stepValue == 0)
        return notes;

    // Unsigned arithmetic also represents the magnitude of INT_MIN safely.
    const auto absVal = stepValue < 0 ? 0u - static_cast<unsigned int>(stepValue)
                                     : static_cast<unsigned int>(stepValue);
    int sign       = (stepValue > 0) ? 1 : -1;
    int baseOffset = octave * pitchClassCount;

    // Build bit array: bits[0] = LSB of absVal (matching generate.ts .reverse())
    std::vector<int> bits;
    auto tmp = absVal;
    while (tmp > 0)
    {
        bits.push_back(tmp & 1);
        tmp >>= 1;
    }

    for (int idx = 0; idx < static_cast<int>(scale.size()); ++idx)
    {
        int bitIndex = sign * (idx - baseOffset);
        if (bitIndex >= 0
            && bitIndex < static_cast<int>(bits.size())
            && bits[bitIndex] == 1)
        {
            notes.push_back(scale[idx]);
        }
    }

    return notes;
}

// ---------------------------------------------------------------------------
std::vector<std::vector<int>> GateRunnerEngine::computeAllSteps(const KeyAssignment& assignment)
{
    std::vector<std::vector<int>> result;
    for (const auto& step : computeAllStepEvents(assignment))
    {
        auto& notes = result.emplace_back();
        for (const auto& event : step) notes.push_back(event.note);
    }
    return result;
}

std::vector<std::vector<GateRunnerEngine::StepNote>>
GateRunnerEngine::computeAllStepEvents(const KeyAssignment& assignment, size_t maxSteps)
{
    const std::vector<int> scale = buildScale(assignment.forte);
    const int k = assignment.forte.getK();

    auto values = assignment.sequence;
    if (assignment.reverse)
        std::reverse(values.begin(), values.end());
    if (!values.empty())
    {
        const auto shift = ((assignment.rotation % static_cast<int>(values.size()))
                            + static_cast<int>(values.size())) % static_cast<int>(values.size());
        std::rotate(values.rbegin(), values.rbegin() + shift, values.rend());
    }
    if (values.size() > maxSteps) values.resize(maxSteps);

    std::vector<std::vector<StepNote>> result;
    result.reserve(values.size());

    for (size_t step = 0; step < values.size(); ++step)
    {
        std::vector<StepNote> notes;
        if (assignment.mode == KeyAssignment::Mode::rhythmic)
        {
            const int width = std::clamp(assignment.drumVelocityBits, 1, 7);
            const auto mask = (1u << width) - 1u;
            for (int lane = 0; lane < std::clamp(assignment.drumLaneCount, 1, 16); ++lane)
            {
                const auto level = values[step].bitsAt(lane * width, width);
                if (level != 0)
                    notes.push_back({ assignment.drumNotes[lane],
                                      static_cast<int>((level * 127u + mask / 2u) / mask) });
            }
        }
        else
            for (int note : computeStepNotes(scale, k, values[step].melodicValue(), assignment.octave))
                notes.push_back({ note, 127 });

        for (auto& note : notes)
            note.note += assignment.transpose;
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [](const StepNote& note) { return note.note < 0 || note.note > 127; }), notes.end());
        result.push_back(std::move(notes));
    }

    return result;
}
