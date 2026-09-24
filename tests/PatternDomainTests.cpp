#include "../Source/Engine/GateRunnerEngine.h"
#include "../Source/Domain/RhythmGenerators.h"

#include "../Source/Domain/AssignmentHistory.h"
#include <stdexcept>
#include <limits>

void require(bool condition)
{
    if (!condition) throw std::runtime_error("Pattern domain regression failed");
}

int main()
{
    Pcs12::GenerateMaps();
    KeyAssignment pattern;
    pattern.mode = KeyAssignment::Mode::rhythmic;
    pattern.sequence = { 5, 0, 1 };
    pattern.pitchSteps = { 0, 12 };
    auto notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes.size() == 3 && notes[0].size() == 2);
    require(notes[0][0] == 36 && notes[0][1] == 42);
    require(notes[1].empty() && notes[2].size() == 1 && notes[2][0] == 36);

    pattern.reverse = true;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0].size() == 1 && notes[0][0] == 36);
    pattern.rotation = 1;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0].size() == 2 && notes[0][0] == 36);

    pattern = KeyAssignment{};
    pattern.sequence = { 1, 0 };
    pattern.setForteFromString("1-1.0");
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes.size() == 2 && notes[0].size() == 1 && notes[1].empty());
    pattern.transpose = 12;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0][0] == 60);
    require(pattern.effectiveSubdivision(4) == 4);
    pattern.subdivision = 3;
    require(pattern.effectiveSubdivision(4) == 3);

    const auto euclid = makeEuclideanRhythm(5, 13);
    require(euclid.size() == 13);
    require(std::count(euclid.begin(), euclid.end(), 1) == 5);
    for (int i = 0; i < 13; ++i)
        require(makeEuclideanRhythm(5, 13, 3)[static_cast<size_t>((i + 3) % 13)]
               == euclid[static_cast<size_t>(i)]);

    // Parsing is atomic: malformed or oversized input keeps the last valid pattern.
    pattern.sequence = { 1, 2, 3 };
    require(!pattern.setSequenceFromString("7 8 nope 9"));
    require(pattern.sequence == std::vector<int>({ 1, 2, 3 }));
    require(!pattern.setSequenceFromString("1 2147483648"));
    require(!pattern.setSequenceFromString("1.5 2"));
    require(pattern.setSequenceFromString("+1 -2147483648 0"));
    require(pattern.sequence[1] == std::numeric_limits<int>::min());
    pattern.octave = 10;
    pattern.transpose = 0;
    pattern.setForteFromString("12-1.0");
    const auto minimumNotes = GateRunnerEngine::computeAllSteps(pattern);
    require(minimumNotes[1] == std::vector<int>({ 89 }));
    require(pattern.setSequenceFromString("  "));
    require(pattern.sequence.empty());
    std::string tooLong;
    for (int i = 0; i < 4097; ++i) tooLong += "1 ";
    require(!pattern.setSequenceFromString(tooLong));
    require(pattern.sequence.empty());

    std::array<KeyAssignment, 128> keys;
    keys[60].name = "Original";
    keys[61].name = "Neighbour";
    AssignmentHistory history;
    history.record({ 60, { { 60, keys[60] }, { 61, keys[61] } } });
    keys[60].name = "Copy";
    keys[61] = keys[60];
    auto get = [&](int key) { return keys[static_cast<size_t>(key)]; };
    auto set = [&](int key, const KeyAssignment& a) { keys[static_cast<size_t>(key)] = a; };
    require(history.undo(get, set) == 60);
    require(keys[60].name == "Original" && keys[61].name == "Neighbour");
    require(!history.canUndo() && history.canRedo());
    require(history.redo(get, set) == 60);
    require(keys[60].name == "Copy" && keys[61].name == "Copy");
    history.clear();
    require(!history.canUndo() && !history.canRedo());
    for (int i = 0; i < 110; ++i)
        history.record({ 60, { { 60, keys[60] } } });
    int count = 0;
    while (history.undo(get, set) >= 0) ++count;
    require(count == 100);
}
