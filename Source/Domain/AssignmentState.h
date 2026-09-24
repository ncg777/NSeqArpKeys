#pragma once

#include <JuceHeader.h>
#include "KeyAssignment.h"
#include "../Engine/GateRunnerEngine.h"

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
    auto sequenceText = [](const std::vector<int>& values)
    {
        juce::StringArray tokens;
        for (int value : values) tokens.add(juce::String(value));
        return tokens.joinIntoString(" ");
    };
    child->setAttribute("velocitySteps", sequenceText(a.velocitySteps));
    child->setAttribute("pitchSteps", sequenceText(a.pitchSteps));
    child->setAttribute("drumNotes", sequenceText(
        std::vector<int>(a.drumNotes.begin(), a.drumNotes.end())));
}

inline KeyAssignment read(const juce::XmlElement& element)
{
    const auto* child = &element;
    KeyAssignment a;
    a.setSequenceFromString(child->getStringAttribute("sequence").toStdString());
    a.setForteFromString   (child->getStringAttribute("forte").toStdString());
    a.channel      = juce::jlimit(1, 16, child->getIntAttribute("channel", 1));
    a.octave       = juce::jlimit(0, 10, child->getIntAttribute("octave", 4));
    a.gate = juce::jlimit(0.0f, 2.0f, static_cast<float>(child->getDoubleAttribute("gate", 0.5)));
    a.fixedLengthSteps = juce::jlimit(0.0f, 16.0f,
        static_cast<float>(child->getDoubleAttribute("fixedLengthSteps", 0.0)));
    a.subdivision = juce::jlimit(0, 16, child->getIntAttribute("subdivision", 0));
    a.mode = child->getStringAttribute("mode") == "rhythmic"
        ? KeyAssignment::Mode::rhythmic : KeyAssignment::Mode::melodic;
    a.name = child->getStringAttribute("name").toStdString();
    a.transpose = juce::jlimit(-127, 127, child->getIntAttribute("transpose", 0));
    a.velocity = juce::jlimit(1, 127, child->getIntAttribute("velocity", 100));
    a.rotation = juce::jlimit(-4096, 4096, child->getIntAttribute("rotation", 0));
    a.reverse = child->getBoolAttribute("reverse", false);
    a.rootKey = juce::jlimit(-1, 127, child->getIntAttribute("rootKey", -1));
    auto parseValues = [](const juce::String& text) {
        return GateRunnerEngine::parseSequence(text.toStdString());
    };
    a.velocitySteps = parseValues(child->getStringAttribute("velocitySteps"));
    for (auto& value : a.velocitySteps) value = juce::jlimit(0, 127, value);
    a.pitchSteps = parseValues(child->getStringAttribute("pitchSteps"));
    for (auto& value : a.pitchSteps) value = juce::jlimit(-127, 127, value);
    const auto drumValues = parseValues(child->getStringAttribute("drumNotes"));
    if (drumValues.size() == a.drumNotes.size())
        for (size_t i = 0; i < drumValues.size(); ++i)
            a.drumNotes[i] = juce::jlimit(0, 127, drumValues[i]);
    return a;
}
}
