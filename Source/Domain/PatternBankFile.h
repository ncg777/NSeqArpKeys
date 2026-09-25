#pragma once

#include "AssignmentState.h"
#include <vector>

// Portable bundle of the same NSeqPattern elements used by individual files.
namespace PatternBankFile
{
struct Entry
{
    juce::String id;
    KeyAssignment assignment;
};

inline std::unique_ptr<juce::XmlElement> writePattern(const Entry& entry)
{
    auto pattern = std::make_unique<juce::XmlElement>("NSeqPattern");
    pattern->setAttribute("version", 1);
    pattern->setAttribute("id", entry.id);
    AssignmentState::write(*pattern->createNewChildElement("Assignment"), entry.assignment);
    return pattern;
}

inline std::unique_ptr<juce::XmlElement> write(const std::vector<Entry>& entries)
{
    auto bank = std::make_unique<juce::XmlElement>("NSeqPatternBank");
    bank->setAttribute("version", 1);
    for (const auto& entry : entries)
        bank->addChildElement(writePattern(entry).release());
    return bank;
}

// Parse the entire bundle before the editor changes any files. Missing IDs are
// assigned on import, which makes hand-authored bundles easy to create.
inline bool read(const juce::XmlElement& bank, std::vector<Entry>& result, juce::String& error)
{
    if (!bank.hasTagName("NSeqPatternBank") || bank.getIntAttribute("version") != 1)
    {
        error = "Expected an NSeqPatternBank version 1 file.";
        return false;
    }
    if (bank.getNumChildElements() > 2000)
    {
        error = "The bank contains too many patterns.";
        return false;
    }

    std::vector<Entry> parsed;
    juce::StringArray ids;
    for (auto* pattern : bank.getChildIterator())
    {
        const int number = static_cast<int>(parsed.size()) + 1;
        if (!pattern->hasTagName("NSeqPattern") || pattern->getIntAttribute("version") != 1
            || pattern->getNumChildElements() != 1)
        {
            error = "Pattern " + juce::String(number) + " has an invalid structure or version.";
            return false;
        }
        const auto* assignment = pattern->getFirstChildElement();
        if (assignment == nullptr || !assignment->hasTagName("Assignment")
            || assignment->getNumChildElements() != 0 || !assignment->hasAttribute("sequence"))
        {
            error = "Pattern " + juce::String(number) + " needs one Assignment with a sequence.";
            return false;
        }
        auto id = pattern->getStringAttribute("id").trim();
        if (id.length() > 80 || (id.isNotEmpty() && ids.contains(id)))
        {
            error = "Pattern " + juce::String(number) + " has a duplicate or oversized ID.";
            return false;
        }
        if (id.isEmpty()) id = juce::Uuid().toString();
        ids.add(id);

        const auto sequence = assignment->getStringAttribute("sequence");
        KeyAssignment checked;
        if (!checked.setSequenceFromString(sequence.toStdString(), true))
        {
            error = "Pattern " + juce::String(number) + " has an invalid sequence.";
            return false;
        }
        const auto drumText = assignment->getStringAttribute("drumNotes");
        if (drumText.isNotEmpty())
        {
            std::vector<int> notes;
            if (!KeyAssignment::parseIntegerSequence(drumText.toStdString(), notes)
                || notes.empty() || notes.size() > 16)
            {
                error = "Pattern " + juce::String(number) + " has invalid drum notes.";
                return false;
            }
            for (const int note : notes)
                if (note < 0 || note > 127)
                {
                    error = "Pattern " + juce::String(number) + " has an out-of-range drum note.";
                    return false;
                }
        }

        auto value = AssignmentState::read(*assignment);
        const auto forte = assignment->getStringAttribute("forte");
        if (forte.isNotEmpty() && juce::String(value.forteString) != forte)
        {
            error = "Pattern " + juce::String(number) + " has an invalid Forte set.";
            return false;
        }
        value.linkId.clear(); // A bank file never changes links in an open project.
        if (value.name.empty()) value.name = "Pattern " + std::to_string(number);
        parsed.push_back({ id, std::move(value) });
    }
    result = std::move(parsed);
    error.clear();
    return true;
}
}
