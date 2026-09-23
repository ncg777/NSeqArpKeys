#include "../Source/Engine/PatternScheduler.h"

#include <stdexcept>
#include <vector>

namespace
{
struct Event
{
    int sample;
    bool on;
};

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

KeyAssignment assignmentWith(const std::vector<int>& sequence, float gate, float fixedSteps)
{
    KeyAssignment assignment;
    assignment.sequence = sequence;
    assignment.setForteFromString("1-1.0");
    assignment.gate = gate;
    assignment.fixedLengthSteps = fixedSteps;
    require(assignment.hasValidForte(), "Test Forte set did not load");
    return assignment;
}

std::vector<Event> process(PatternScheduler& scheduler, int blocks)
{
    std::vector<Event> result;
    for (int block = 0; block < blocks; ++block)
    {
        juce::MidiBuffer midi;
        scheduler.processBlock(midi, 1000, 60.0, 4, 1);
        for (const auto& metadata : midi)
        {
            const auto& message = metadata.getMessage();
            if (message.isNoteOnOrOff())
                result.push_back({ block * 1000 + metadata.samplePosition,
                                   message.isNoteOn() });
        }
    }
    return result;
}
}

int main()
{
    Pcs12::GenerateMaps();

    PatternScheduler scheduler;
    scheduler.prepare(1000.0);
    juce::MidiBuffer trigger;
    scheduler.triggerKey(60, assignmentWith({ 1, 0 }, 0.5f, 0.5f),
                         60.0, 4, 1, trigger);
    const auto additive = process(scheduler, 3);
    require(additive.size() == 3, "Expected two note-ons and one note-off");
    require(additive[0].sample == 0 && additive[0].on, "First onset is wrong");
    require(additive[1].sample == 1500 && !additive[1].on,
            "Fixed Steps and Gate were not added");
    require(additive[2].sample == 2000 && additive[2].on, "Loop onset is wrong");

    scheduler.prepare(1000.0);
    trigger.clear();
    scheduler.triggerKey(60, assignmentWith({ 1 }, 2.0f, 0.5f),
                         60.0, 4, 1, trigger);
    const auto overlapping = process(scheduler, 4);
    require(overlapping.size() == 4, "Overlapping pitch was released early");
    for (int i = 0; i < 4; ++i)
        require(overlapping[static_cast<size_t>(i)].on
                && overlapping[static_cast<size_t>(i)].sample == i * 1000,
                "Overlapping onset is wrong");
    juce::MidiBuffer stopped;
    scheduler.stopKey(60, stopped);
    require(stopped.getNumEvents() == 1, "Stop must release the overlapping pitch once");

    scheduler.prepare(1000.0);
    trigger.clear();
    scheduler.triggerKey(60, assignmentWith({ 1 }, 0.0f, 0.0f),
                         60.0, 4, 1, trigger);
    const auto zero = process(scheduler, 1);
    require(zero.size() == 2 && zero[0].on && !zero[1].on
            && zero[0].sample == 0 && zero[1].sample == 0,
            "Zero duration must emit a matching note-off");
}
