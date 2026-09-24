#include "GateRunnerEngine.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// ---------------------------------------------------------------------------
std::vector<int> GateRunnerEngine::parseSequence(const std::string& text)
{
    std::istringstream iss(text);
    std::vector<int> result;
    int n;
    while (iss >> n)
        result.push_back(n);
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

    int absVal     = std::abs(stepValue);
    int sign       = (stepValue > 0) ? 1 : -1;
    int baseOffset = octave * pitchClassCount;

    // Build bit array: bits[0] = LSB of absVal (matching generate.ts .reverse())
    std::vector<int> bits;
    int tmp = absVal;
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
    const std::vector<int> scale = buildScale(assignment.forte);
    const int k = assignment.forte.getK();

    std::vector<int> values = assignment.sequence;
    if (assignment.reverse)
        std::reverse(values.begin(), values.end());
    if (!values.empty())
    {
        const auto shift = ((assignment.rotation % static_cast<int>(values.size()))
                            + static_cast<int>(values.size())) % static_cast<int>(values.size());
        std::rotate(values.rbegin(), values.rbegin() + shift, values.rend());
    }

    std::vector<std::vector<int>> result;
    result.reserve(values.size());

    for (size_t step = 0; step < values.size(); ++step)
    {
        std::vector<int> notes;
        if (assignment.mode == KeyAssignment::Mode::rhythmic)
        {
            const auto bits = values[step] > 0 ? static_cast<unsigned int>(values[step]) : 0u;
            for (size_t bit = 0; bit < assignment.drumNotes.size(); ++bit)
                if ((bits & (1u << bit)) != 0)
                    notes.push_back(assignment.drumNotes[bit]);
        }
        else
            notes = computeStepNotes(scale, k, values[step], assignment.octave);

        const int pitchOffset = assignment.pitchSteps.empty() ? 0
            : assignment.pitchSteps[step % assignment.pitchSteps.size()];
        for (auto& note : notes)
            note += assignment.transpose + pitchOffset;
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [](int note) { return note < 0 || note > 127; }), notes.end());
        result.push_back(std::move(notes));
    }

    return result;
}
