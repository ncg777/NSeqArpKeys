#include "../Source/Engine/GateRunnerEngine.h"
#include "../Source/Domain/RhythmGenerators.h"

#include <cassert>

int main()
{
    Pcs12::GenerateMaps();
    KeyAssignment pattern;
    pattern.mode = KeyAssignment::Mode::rhythmic;
    pattern.sequence = { 5, 0, 1 };
    pattern.pitchSteps = { 0, 12 };
    auto notes = GateRunnerEngine::computeAllSteps(pattern);
    assert(notes.size() == 3 && notes[0].size() == 2);
    assert(notes[0][0] == 36 && notes[0][1] == 42);
    assert(notes[1].empty() && notes[2].size() == 1 && notes[2][0] == 36);

    pattern.reverse = true;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    assert(notes[0].size() == 1 && notes[0][0] == 36);
    pattern.rotation = 1;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    assert(notes[0].size() == 2 && notes[0][0] == 36);

    pattern = KeyAssignment{};
    pattern.sequence = { 1, 0 };
    pattern.setForteFromString("1-1.0");
    notes = GateRunnerEngine::computeAllSteps(pattern);
    assert(notes.size() == 2 && notes[0].size() == 1 && notes[1].empty());
    pattern.transpose = 12;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    assert(notes[0][0] == 60);
    assert(pattern.effectiveSubdivision(4) == 4);
    pattern.subdivision = 3;
    assert(pattern.effectiveSubdivision(4) == 3);

    const auto euclid = makeEuclideanRhythm(5, 13);
    assert(euclid.size() == 13);
    assert(std::count(euclid.begin(), euclid.end(), 1) == 5);
    for (int i = 0; i < 13; ++i)
        assert(makeEuclideanRhythm(5, 13, 3)[static_cast<size_t>((i + 3) % 13)]
               == euclid[static_cast<size_t>(i)]);
}
