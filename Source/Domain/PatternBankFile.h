#pragma once

#include "AssignmentState.h"
#include "SafeXml.h"
#include <locale>
#include <vector>

// Portable bundle of the same NSeqPattern elements used by individual files.
namespace PatternBankFile
{
inline constexpr int maxPatterns = 2000;
inline constexpr int64_t maxFileBytes = 16 * 1024 * 1024;

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

// Project restore tolerates legacy values, but a generated bank must not
// silently replace an existing entry with clamped or defaulted settings.
inline juce::String invalidAssignmentAttribute(const juce::XmlElement& assignment)
{
    struct IntegerAttribute { const char* name; int minimum, maximum; };
    for (const auto& field : { IntegerAttribute { "channel", 1, 16 },
                              { "octave", 0, 10 }, { "subdivision", 0, 16 },
                              { "transpose", -127, 127 }, { "velocity", 1, 127 },
                              { "rotation", -4096, 4096 }, { "rootKey", -1, 127 },
                              { "drumVelocityBits", 1, 7 } })
    {
        if (!assignment.hasAttribute(field.name)) continue;
        std::vector<int> values;
        if (!KeyAssignment::parseIntegerSequence(assignment.getStringAttribute(field.name).toStdString(),
                                                 values, field.minimum, field.maximum)
            || values.size() != 1)
            return field.name;
    }
    for (const auto* name : { "gate", "fixedLengthSteps" })
    {
        if (!assignment.hasAttribute(name)) continue;
        std::istringstream input(assignment.getStringAttribute(name).trim().toStdString());
        input.imbue(std::locale::classic());
        double value = 0;
        input >> value;
        const double maximum = juce::String(name) == "gate" ? 2.0 : 16.0;
        if (input.fail() || !input.eof() || !std::isfinite(value) || value < 0 || value > maximum)
            return name;
    }
    for (const auto* name : { "reverse", "favourite" })
    {
        if (!assignment.hasAttribute(name)) continue;
        const auto value = assignment.getStringAttribute(name).trim().toLowerCase();
        if (value != "0" && value != "1" && value != "true" && value != "false"
            && value != "yes" && value != "no")
            return name;
    }
    if (assignment.hasAttribute("mode"))
    {
        const auto mode = assignment.getStringAttribute("mode");
        if (mode != "melodic" && mode != "rhythmic") return "mode";
    }
    if (assignment.hasAttribute("colour"))
    {
        const auto colour = assignment.getStringAttribute("colour");
        if (colour.length() != 7 || !colour.startsWithChar('#')
            || !colour.substring(1).containsOnly("0123456789abcdefABCDEF"))
            return "colour";
    }
    if (assignment.getStringAttribute("name").length() > 80) return "name";
    if (assignment.getStringAttribute("tags").length() > 200) return "tags";
    if (assignment.getStringAttribute("forte").length() > 64) return "forte";
    return {};
}

// Parse the entire bundle before the editor changes any files. Missing IDs are
// assigned on import, which makes hand-authored bundles easy to create.
inline bool read(const juce::XmlElement& bank, std::vector<Entry>& result, juce::String& error)
{
    if (!bank.hasTagName("NSeqPatternBank") || bank.getStringAttribute("version") != "1")
    {
        error = "Expected an NSeqPatternBank version 1 file.";
        return false;
    }
    if (bank.getNumChildElements() > maxPatterns)
    {
        error = "The bank contains too many patterns.";
        return false;
    }

    std::vector<Entry> parsed;
    juce::StringArray ids;
    for (auto* pattern : bank.getChildIterator())
    {
        const int number = static_cast<int>(parsed.size()) + 1;
        if (!pattern->hasTagName("NSeqPattern") || pattern->getStringAttribute("version") != "1"
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
        // IDs are opaque: trimming an existing ID would import an exported
        // entry as a new pattern instead of updating the original.
        auto id = pattern->getStringAttribute("id");
        if (id.length() > 80 || (id.isNotEmpty() && ids.contains(id)))
        {
            error = "Pattern " + juce::String(number) + " has a duplicate or oversized ID.";
            return false;
        }
        if (id.isEmpty()) id = juce::Uuid().toString();
        ids.add(id);

        const auto invalidAttribute = invalidAssignmentAttribute(*assignment);
        if (invalidAttribute.isNotEmpty())
        {
            error = "Pattern " + juce::String(number) + " has an invalid " + invalidAttribute + " value.";
            return false;
        }

        const auto sequence = assignment->getStringAttribute("sequence");
        KeyAssignment checked;
        if (!checked.setSequenceFromString(sequence.toStdString(), true))
        {
            error = "Pattern " + juce::String(number) + " has an invalid sequence.";
            return false;
        }
        const auto drumText = assignment->getStringAttribute("drumNotes");
        if (assignment->hasAttribute("drumNotes"))
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

// Validate exports through the same reader used on import, including its XML
// limits. Never report a successful backup that cannot subsequently be loaded.
inline bool serialize(const std::vector<Entry>& entries, juce::String& content, juce::String& error)
{
    if (entries.size() > static_cast<size_t>(maxPatterns))
    {
        error = "A bank file can contain at most " + juce::String(maxPatterns) + " patterns.";
        return false;
    }
    for (const auto& entry : entries)
        if (entry.id.isEmpty())
        {
            error = "A bank entry has no ID. Save that pattern again before exporting.";
            return false;
        }
    const auto text = write(entries)->toString();
    if (text.getNumBytesAsUTF8() > static_cast<size_t>(maxFileBytes))
    {
        error = "Pattern bank is too large to export as one file (16 MiB maximum).";
        return false;
    }
    const auto xml = SafeXml::parse(text);
    if (xml == nullptr)
    {
        error = "Pattern bank contains data that cannot be exported as XML.";
        return false;
    }
    std::vector<Entry> checked;
    if (!read(*xml, checked, error)) return false;
    content = text;
    error.clear();
    return true;
}
}
