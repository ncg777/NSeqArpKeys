#include "../Source/Engine/GateRunnerEngine.h"

#include "../Source/Domain/AssignmentHistory.h"
#include "../Source/Domain/PatternBankFile.h"
#include "../Source/Domain/SafeXml.h"
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
    auto notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes.size() == 3 && notes[0].size() == 2);
    require(notes[0][0] == 36 && notes[0][1] == 42);
    require(notes[1].empty() && notes[2].size() == 1 && notes[2][0] == 36);
    pattern.drumLaneCount = 3;
    pattern.drumVelocityBits = 2;
    pattern.sequence = { 9 }; // lane 1 level 1, lane 2 level 2
    const auto hits = GateRunnerEngine::computeAllStepEvents(pattern);
    require(hits.size() == 1 && hits[0].size() == 2
        && hits[0][0].note == 36 && hits[0][0].velocityLevel == 42
        && hits[0][1].note == 38 && hits[0][1].velocityLevel == 85);
    require(pattern.hasValidDrumVelocityBits());
    pattern.drumVelocityBits = 7;
    require(pattern.hasValidDrumVelocityBits());
    pattern.drumVelocityBits = 8;
    require(!pattern.hasValidDrumVelocityBits());
    pattern.drumVelocityBits = 1;
    pattern.drumLaneCount = 16;
    pattern.sequence = { 5, 0, 1 };

    // GateRunner uses positive decimal BigInt masks, even when 16 lanes each
    // reserve seven bits. The last lane must still decode beyond bit 31.
    pattern.drumVelocityBits = 7;
    require(pattern.setSequenceFromString("40564819207303340847894502572032"));
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes.size() == 1 && notes[0] == std::vector<int>({ 56 }));
    require(pattern.sequenceToString() == "40564819207303340847894502572032");
    require(pattern.setSequenceFromString("34091302912"));
    const auto crossing = GateRunnerEngine::computeAllStepEvents(pattern);
    require(crossing.size() == 1 && crossing[0].size() == 1
        && crossing[0][0].note == 41 && crossing[0][0].velocityLevel == 127);
    require(pattern.setSequenceFromString("5192296858534827628530496329220095"));
    require(GateRunnerEngine::computeAllStepEvents(pattern)[0].size() == 16);
    require(!pattern.setSequenceFromString("5192296858534827628530496329220096"));
    require(pattern.sequence.size() == 1);
    pattern.drumVelocityBits = 1;
    pattern.sequence = { 5, 0, 1 };

    pattern.reverse = true;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0].size() == 1 && notes[0][0] == 36);
    pattern.rotation = 1;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0].size() == 2 && notes[0][0] == 36);

    // A short display window must follow the complete transformed pattern.
    pattern.sequence.assign(20, 0);
    pattern.sequence.back() = 1;
    pattern.rotation = 0;
    auto preview = GateRunnerEngine::computeAllStepEvents(pattern, 16);
    require(preview.size() == 16 && preview[0].size() == 1 && preview[0][0].note == 36);
    pattern.reverse = false;
    pattern.rotation = 1;
    preview = GateRunnerEngine::computeAllStepEvents(pattern, 16);
    require(preview.size() == 16 && preview[0].size() == 1 && preview[0][0].note == 36);

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

    // Parsing is atomic: malformed or oversized input keeps the last valid pattern.
    pattern.sequence = { 1, 2, 3 };
    require(!pattern.setSequenceFromString("7 8 nope 9"));
    require(pattern.sequenceToString() == "1 2 3");
    require(!pattern.setSequenceFromString("1 2147483648"));
    require(!pattern.setSequenceFromString("1.5 2"));
    require(pattern.setSequenceFromString("+1 -2147483648 0"));
    require(pattern.sequence[1].melodicValue() == std::numeric_limits<int>::min());
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
    require(!pattern.setSequenceFromString(std::string(45057, ' ')));

    std::array<KeyAssignment, 128> keys;
    keys[60].name = "Original";
    keys[61].name = "Neighbour";
    AssignmentHistory history;
    history.record({ 60, { { 60, keys[60] }, { 61, keys[61] } } });
    keys[60].name = "Copy";
    keys[61] = keys[60];
    auto get = [&](int key) { return keys[static_cast<size_t>(key)]; };
    auto set = [&](const auto& assignments) {
        for (const auto& [key, a] : assignments) keys[static_cast<size_t>(key)] = a;
    };
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

    // Whole-bank files preserve the complete assignment and stable IDs.
    KeyAssignment bankPattern;
    bankPattern.name = "Generated rhythm";
    bankPattern.tags = "test generated";
    bankPattern.favourite = true;
    bankPattern.mode = KeyAssignment::Mode::rhythmic;
    bankPattern.drumVelocityBits = 2;
    bankPattern.sequence = { 9, 0, 3 };
    bankPattern.linkId = "project-only-link";
    const auto exported = PatternBankFile::write({ { "external-1", bankPattern } });
    auto parsedXml = SafeXml::parse(exported->toString());
    require(parsedXml != nullptr);
    std::vector<PatternBankFile::Entry> imported;
    juce::String bankError;
    require(PatternBankFile::read(*parsedXml, imported, bankError));
    require(imported.size() == 1 && imported[0].id == "external-1");
    bankPattern.linkId.clear();
    require(imported[0].assignment == bankPattern);

    auto* first = parsedXml->getFirstChildElement();
    auto* assignment = first->getFirstChildElement();
    assignment->setAttribute("sequence", "1 bad 3");
    require(!PatternBankFile::read(*parsedXml, imported, bankError));
    require(imported.size() == 1 && imported[0].id == "external-1");
    assignment->setAttribute("sequence", "9 0 3");
    auto* duplicate = new juce::XmlElement(*first);
    parsedXml->addChildElement(duplicate);
    require(!PatternBankFile::read(*parsedXml, imported, bankError));
    parsedXml->removeChildElement(duplicate, true);
    first->removeAttribute("id");
    require(PatternBankFile::read(*parsedXml, imported, bankError));
    require(imported.size() == 1 && imported[0].id.isNotEmpty());
    parsedXml->setAttribute("version", 2);
    require(!PatternBankFile::read(*parsedXml, imported, bankError));
}
