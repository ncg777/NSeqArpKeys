#pragma once

#include <JuceHeader.h>
#include "KeyAssignment.h"
#include "../Engine/GateRunnerEngine.h"
#include <cmath>

// Shared by DAW state, whole presets and individual pattern files.
namespace AssignmentState
{
inline void write(juce::XmlElement& element, const KeyAssignment& a)
{
    auto* child = &element;
    child->setAttribute("sequence",     a.sequenceToString());
    child->setAttribute("forte",        a.forteString);
    child->setAttribute("channel",      a.channel);
    child->setAttribute("octave",       a.octave);
    child->setAttribute("gate",         static_cast<double>(a.gate));
    child->setAttribute("fixedLengthSteps", static_cast<double>(a.fixedLengthSteps));
    child->setAttribute("subdivision", a.subdivision);
    child->setAttribute("mode", a.mode == KeyAssignment::Mode::rhythmic ? "rhythmic" : "melodic");
    child->setAttribute("name", juce::String(a.name));
    child->setAttribute("transpose", a.transpose);
    child->setAttribute("velocity", a.velocity);
    child->setAttribute("rotation", a.rotation);
    child->setAttribute("reverse", a.reverse);
    child->setAttribute("rootKey", a.rootKey);
    child->setAttribute("linkId", juce::String(a.linkId));
    child->setAttribute("colour", juce::String(a.colour));
    child->setAttribute("tags", juce::String(a.tags));
    child->setAttribute("favourite", a.favourite);
    auto sequenceText = [](const std::vector<int>& values)
    {
        juce::StringArray tokens;
        for (int value : values) tokens.add(juce::String(value));
        return tokens.joinIntoString(" ");
    };
    child->setAttribute("drumNotes", sequenceText(
        std::vector<int>(a.drumNotes.begin(), a.drumNotes.begin() + juce::jlimit(1, 16, a.drumLaneCount))));
    child->setAttribute("drumVelocityBits", sequenceText(
        std::vector<int>(a.drumVelocityBits.begin(), a.drumVelocityBits.begin() + juce::jlimit(1, 16, a.drumLaneCount))));
}

inline KeyAssignment read(const juce::XmlElement& element)
{
    const auto* child = &element;
    KeyAssignment a;
    const auto sequence = child->getStringAttribute("sequence");
    if (sequence.length() <= 45056)
        a.setSequenceFromString(sequence.toStdString());
    const auto forte = child->getStringAttribute("forte");
    if (forte.length() <= 64)
        a.setForteFromString(forte.toStdString());
    a.channel      = juce::jlimit(1, 16, child->getIntAttribute("channel", 1));
    a.octave       = juce::jlimit(0, 10, child->getIntAttribute("octave", 4));
    const auto gate = child->getDoubleAttribute("gate", 0.5);
    const auto fixed = child->getDoubleAttribute("fixedLengthSteps", 0.0);
    a.gate = std::isfinite(gate) ? juce::jlimit(0.0f, 2.0f, static_cast<float>(gate)) : 0.5f;
    a.fixedLengthSteps = std::isfinite(fixed)
        ? juce::jlimit(0.0f, 16.0f, static_cast<float>(fixed)) : 0.0f;
    a.subdivision = juce::jlimit(0, 16, child->getIntAttribute("subdivision", 0));
    a.mode = child->getStringAttribute("mode") == "rhythmic"
        ? KeyAssignment::Mode::rhythmic : KeyAssignment::Mode::melodic;
    a.name = child->getStringAttribute("name").substring(0, 80).toStdString();
    a.transpose = juce::jlimit(-127, 127, child->getIntAttribute("transpose", 0));
    a.velocity = juce::jlimit(1, 127, child->getIntAttribute("velocity", 100));
    a.rotation = juce::jlimit(-4096, 4096, child->getIntAttribute("rotation", 0));
    a.reverse = child->getBoolAttribute("reverse", false);
    a.rootKey = juce::jlimit(-1, 127, child->getIntAttribute("rootKey", -1));
    a.linkId = child->getStringAttribute("linkId").substring(0, 80).toStdString();
    const auto colour = child->getStringAttribute("colour", "#62D6C6");
    if (colour.length() == 7 && colour.startsWithChar('#')
        && colour.substring(1).containsOnly("0123456789abcdefABCDEF"))
        a.colour = colour.toStdString();
    a.tags = child->getStringAttribute("tags").substring(0, 200).toStdString();
    a.favourite = child->getBoolAttribute("favourite", false);
    auto parseValues = [](const juce::String& text) {
        return GateRunnerEngine::parseSequence(text.toStdString());
    };
    const auto drumValues = parseValues(child->getStringAttribute("drumNotes"));
    if (!drumValues.empty() && drumValues.size() <= a.drumNotes.size())
    {
        a.drumLaneCount = static_cast<int>(drumValues.size());
        for (size_t i = 0; i < drumValues.size(); ++i)
            a.drumNotes[i] = juce::jlimit(0, 127, drumValues[i]);
    }
    const auto velocityBits = parseValues(child->getStringAttribute("drumVelocityBits"));
    if (velocityBits.size() == static_cast<size_t>(a.drumLaneCount))
        for (size_t i = 0; i < velocityBits.size(); ++i)
            a.drumVelocityBits[i] = juce::jlimit(1, 7, velocityBits[i]);
    if (!a.hasValidDrumVelocityBits()) a.drumVelocityBits.fill(1);
    return a;
}
}
