#include "../Source/Engine/GateRunnerEngine.h"

#include "../Source/Domain/AssignmentHistory.h"
#include "../Source/Domain/PatternBankFile.h"
#include "../Source/Domain/SafeXml.h"
#include "FourthAtlasData.h"
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <set>

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
    pattern.transpose = 1;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0] == std::vector<int>({ 60 }));
    pattern.transpose = 7;
    require(GateRunnerEngine::computeAllSteps(pattern)[0].empty());
    pattern.setForteFromString("7-35.11");
    pattern.sequence = { 1, 3, -2 };
    pattern.transpose = 1;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0] == std::vector<int>({ 50 }));
    require(notes[1] == std::vector<int>({ 50, 52 }));
    require(notes[2] == std::vector<int>({ 48 }));
    pattern.transpose = -1;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0] == std::vector<int>({ 47 }));
    require(notes[1] == std::vector<int>({ 47, 48 }));
    pattern.transpose = 7;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0] == std::vector<int>({ 60 }));
    const auto selectedPitches = pattern.forte.asSequence();
    for (const auto& step : notes)
        for (const int note : step)
            require(std::find(selectedPitches.begin(), selectedPitches.end(), note % 12)
                    != selectedPitches.end());
    pattern.octave = 0;
    pattern.transpose = -1;
    notes = GateRunnerEngine::computeAllSteps(pattern);
    require(notes[0].empty() && notes[1] == std::vector<int>({ 0 })
        && notes[2].empty());
    // Apply the degree offset before clipping, so notes can return to MIDI range.
    pattern.sequence = { -3 };
    pattern.transpose = 1;
    require(GateRunnerEngine::computeAllSteps(pattern)[0] == std::vector<int>({ 0, 2 }));
    pattern.setForteFromString("1-1.0");
    pattern.octave = 10;
    pattern.sequence = { 3 };
    pattern.transpose = -1;
    require(GateRunnerEngine::computeAllSteps(pattern)[0] == std::vector<int>({ 108, 120 }));
    pattern.octave = 0;
    pattern.sequence = { std::numeric_limits<int>::min() };
    pattern.transpose = 31;
    require(GateRunnerEngine::computeAllSteps(pattern)[0] == std::vector<int>({ 0 }));
    pattern = KeyAssignment{};
    pattern.mode = KeyAssignment::Mode::rhythmic;
    pattern.drumLaneCount = 1;
    pattern.sequence = { 1 };
    pattern.transpose = 1;
    require(GateRunnerEngine::computeAllSteps(pattern)[0] == std::vector<int>({ 37 }));
    // Every catalogue transposition, in both bit directions, stays in its set.
    KeyAssignment cataloguePattern;
    cataloguePattern.sequence = { std::numeric_limits<int>::max(),
                                  -std::numeric_limits<int>::max() };
    for (const auto& forteId : Pcs12::getSortedForteNumbers())
    {
        cataloguePattern.setForteFromString(forteId);
        const auto pitchClasses = cataloguePattern.forte.asSequence();
        for (const int shift : { -127, -13, -1, 0, 1, 13, 127 })
        {
            cataloguePattern.transpose = shift;
            for (const auto& step : GateRunnerEngine::computeAllSteps(cataloguePattern))
                for (const int note : step)
                    require(std::find(pitchClasses.begin(), pitchClasses.end(), note % 12)
                            != pitchClasses.end());
        }
    }
    pattern = KeyAssignment{};
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

    // External generators must get a useful error instead of silently
    // overwriting an existing pattern with defaulted/clamped settings.
    const auto expectInvalidAttribute = [&](const char* attribute, const juce::String& value)
    {
        auto invalid = PatternBankFile::write({ { "valid-first", bankPattern }, { "invalid-second", bankPattern } });
        invalid->getChildElement(1)->getFirstChildElement()->setAttribute(attribute, value);
        const auto beforeId = imported[0].id;
        const auto beforeAssignment = imported[0].assignment;
        if (PatternBankFile::read(*invalid, imported, bankError))
            throw std::runtime_error("Bank accepted invalid attribute: " + std::string(attribute));
        require(bankError.contains("Pattern 2") && imported.size() == 1
            && imported[0].id == beforeId && imported[0].assignment == beforeAssignment);
    };
    for (const auto& field : std::vector<std::pair<const char*, const char*>> {
            { "channel", "17" }, { "channel", "2oops" }, { "channel", "" },
            { "octave", "11" }, { "subdivision", "1.5" }, { "transpose", "-128" },
            { "velocity", "loud" }, { "velocity", "0" }, { "rotation", "4097" },
            { "rootKey", "-2" }, { "drumVelocityBits", "8" },
            { "gate", "NaN" }, { "gate", "-0.1" }, { "gate", "0.5oops" },
            { "gate", "" }, { "fixedLengthSteps", "17" },
            { "reverse", "maybe" }, { "favourite", "10" }, { "mode", "rhytmic" },
            { "colour", "#XXYYZZ" }, { "drumNotes", "" }, { "drumNotes", "36 128" } })
        expectInvalidAttribute(field.first, field.second);
    expectInvalidAttribute("name", juce::String::repeatedString("x", 81));
    expectInvalidAttribute("tags", juce::String::repeatedString("x", 201));

    auto minimal = SafeXml::parse("<NSeqPatternBank version=\"1\"><NSeqPattern version=\"1\">"
        "<Assignment sequence=\"\" gate=\"5e-1\" favourite=\"true\" reverse=\"no\"/>"
        "</NSeqPattern></NSeqPatternBank>");
    require(minimal != nullptr && PatternBankFile::read(*minimal, imported, bankError));
    require(imported[0].assignment.sequence.empty() && imported[0].assignment.gate == 0.5f
        && imported[0].assignment.favourite && !imported[0].assignment.reverse
        && imported[0].assignment.channel == 1 && imported[0].assignment.drumLaneCount == 16);
    minimal->setAttribute("version", "1oops");
    require(!PatternBankFile::read(*minimal, imported, bankError));
    minimal->setAttribute("version", "1");
    minimal->getFirstChildElement()->setAttribute("version", "1.5");
    require(!PatternBankFile::read(*minimal, imported, bankError));

    // An export must be importable, including IDs, pattern count and byte limits.
    bankPattern.setSequenceFromString("40564819207303340847894502572032 0 1", true);
    juce::String content;
    require(PatternBankFile::serialize({ { " external-1 ", bankPattern } }, content, bankError));
    parsedXml = SafeXml::parse(content);
    require(parsedXml != nullptr && PatternBankFile::read(*parsedXml, imported, bankError));
    require(imported[0].id == " external-1 " && imported[0].assignment == bankPattern);
    const auto previousContent = content;
    require(!PatternBankFile::serialize({ { "duplicate", bankPattern }, { "duplicate", bankPattern } },
                                       content, bankError));
    require(content == previousContent && bankError.isNotEmpty());
    require(!PatternBankFile::serialize({ { "", bankPattern } }, content, bankError));

    std::vector<PatternBankFile::Entry> many;
    for (int i = 0; i < PatternBankFile::maxPatterns; ++i)
        many.push_back({ "pattern-" + juce::String(i), KeyAssignment{} });
    require(PatternBankFile::serialize(many, content, bankError));
    parsedXml = SafeXml::parse(content);
    require(parsedXml != nullptr && PatternBankFile::read(*parsedXml, imported, bankError));
    require(imported.size() == static_cast<size_t>(PatternBankFile::maxPatterns));
    many.push_back({ "one-too-many", KeyAssignment{} });
    require(!PatternBankFile::serialize(many, content, bankError));
    require(bankError.contains("2000"));

    KeyAssignment oversized;
    oversized.name.assign(static_cast<size_t>(PatternBankFile::maxFileBytes), 'x');
    require(!PatternBankFile::serialize({ { "oversized", oversized } }, content, bankError));
    require(bankError.contains("16 MiB"));
    require(PatternBankFile::serialize({}, content, bankError));
    parsedXml = SafeXml::parse(content);
    require(parsedXml != nullptr && PatternBankFile::read(*parsedXml, imported, bankError));
    require(imported.empty());

    // The compiled Fourth Atlas banks must be accepted by the import reader.
    juce::MemoryInputStream atlasBytes(FourthAtlasData::FourthAtlas_zip,
                                       FourthAtlasData::FourthAtlas_zipSize, false);
    juce::ZipFile atlasArchive(atlasBytes);
    require(atlasArchive.getNumEntries() == 13);
    const juce::StringArray bankNames {
        "00-START-HERE.nseqbank", "01-filigree-melodic.nseqbank",
        "02-filigree-rhythmic.nseqbank", "03-antiphon-melodic.nseqbank",
        "04-antiphon-rhythmic.nseqbank", "05-embers-melodic.nseqbank",
        "06-embers-rhythmic.nseqbank", "07-kaleidoscope-melodic.nseqbank",
        "08-kaleidoscope-rhythmic.nseqbank", "09-lattice-melodic.nseqbank",
        "10-lattice-rhythmic.nseqbank", "11-orbit-melodic.nseqbank",
        "12-orbit-rhythmic.nseqbank"
    };
    std::set<std::string> atlasIds;
    std::set<std::string> starterIds;
    for (int bank = 0; bank <= 12; ++bank)
    {
        const auto* zipEntry = atlasArchive.getEntry(bank);
        require(zipEntry != nullptr && zipEntry->filename == bankNames[bank]
                && zipEntry->uncompressedSize <= PatternBankFile::maxFileBytes);
        std::unique_ptr<juce::InputStream> stream(atlasArchive.createStreamForEntry(*zipEntry));
        require(stream != nullptr);
        const auto xml = SafeXml::parse(stream->readEntireStreamAsString());
        require(xml != nullptr);
        juce::String error;
        std::vector<PatternBankFile::Entry> entries;
        require(PatternBankFile::read(*xml, entries, error));
        require(entries.size() == (bank == 0 ? 120u : 1000u));
        for (const auto& entry : entries)
        {
            require(entry.assignment.transpose == 0);
            if (bank == 0)
                starterIds.insert(entry.id.toStdString());
            else
                require(atlasIds.insert(entry.id.toStdString()).second);
        }
    }
    require(atlasIds.size() == 12000 && starterIds.size() == 120);
    for (const auto& id : starterIds)
        require(atlasIds.count(id) == 1);
}
