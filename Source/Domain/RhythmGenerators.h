#pragma once

#include <algorithm>
#include <vector>

/** Evenly distribute k hits around n steps, then rotate right by rotation. */
inline std::vector<int> makeEuclideanRhythm(int hits, int steps, int rotation = 0,
                                             int noteMask = 1)
{
    steps = std::clamp(steps, 1, 64);
    hits = std::clamp(hits, 0, steps);
    std::vector<int> result(static_cast<size_t>(steps), 0);
    for (int i = 0; i < steps; ++i)
        if ((i * hits) % steps < hits)
            result[static_cast<size_t>(i)] = noteMask;
    rotation = ((rotation % steps) + steps) % steps;
    std::rotate(result.rbegin(), result.rbegin() + rotation, result.rend());
    return result;
}
