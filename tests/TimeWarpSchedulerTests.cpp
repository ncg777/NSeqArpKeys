#include "../Source/Engine/PatternScheduler.h"
#include <stdexcept>
#include <tuple>

namespace
{
using Event = std::tuple<int, int, bool>;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
KeyAssignment pattern()
{
    KeyAssignment a;
    a.sequence = { 1, 2, 4, 8 }; a.setForteFromString("1-1.0"); a.gate = 1;
    a.timeWarp.enabled = true; a.timeWarp.curve = "custom"; a.timeWarp.expression = "T^2";
    return a;
}
std::vector<Event> render(const KeyAssignment& assignment, int blockSize, int total = 9500)
{
    PatternScheduler scheduler; scheduler.prepare(1000);
    juce::MidiBuffer trigger;
    scheduler.triggerKey(60, assignment, 60, 4, 1, trigger);
    std::vector<Event> events;
    for (int start = 0; start < total; start += blockSize)
    {
        juce::MidiBuffer midi;
        scheduler.processBlock(midi, std::min(blockSize, total - start), 60, 4, 1);
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            events.emplace_back(start + metadata.samplePosition, message.getNoteNumber(), message.isNoteOn());
        }
    }
    juce::MidiBuffer stopped; scheduler.stopAll(stopped);
    for (const auto metadata : stopped)
        events.emplace_back(total, metadata.getMessage().getNoteNumber(), false);
    return events;
}
}

void testTimeWarpScheduler()
{
    const auto a = pattern();
    const auto events = render(a, 125, 4000);
    const std::vector<Event> expected {
        {0,48,true}, {250,48,false}, {250,60,true}, {1000,60,false},
        {1000,72,true}, {2250,72,false}, {2250,84,true}, {4000,84,false}
    };
    check(events == expected, "Warped MIDI endpoints or loop timing are incorrect");
    auto onsetOnly = a; onsetOnly.timeWarp.noteLengths = false;
    const auto fixed = render(onsetOnly, 125, 4000);
    check(std::find(fixed.begin(), fixed.end(), Event {1000,48,false}) != fixed.end()
          && std::find(fixed.begin(), fixed.end(), Event {1250,60,false}) != fixed.end(), "Onset-only MIDI warp changed note duration");
    auto reverse = a; reverse.timeWarp.curve = "invlin";
    const auto reversed = render(reverse, 125, 4100);
    check(std::find(reversed.begin(), reversed.end(), Event {1000,84,true}) != reversed.end()
          && std::find(reversed.begin(), reversed.end(), Event {4000,48,true}) != reversed.end(), "Reversed curve lost or reordered an endpoint");
    for (const auto& curve : TimeWarp::curves())
    {
        auto warped = a; warped.timeWarp.curve = curve.name;
        warped.timeWarp.amount = 83; warped.timeWarp.repeats = 3; warped.fixedLengthSteps = 0.2f;
        // A partial final block and fractional chunk positions must not cause
        // dropped or duplicated events when the host changes buffer size.
        check(render(warped, 37) == render(warped, 9500), "Warped MIDI depends on host buffer boundaries");
    }
    auto collapsed = a; collapsed.timeWarp.expression = "0";
    const auto unison = render(collapsed, 37);
    check(unison.size() == 24, "Coincident warped onsets did not release cleanly across loops");
    auto overlap = a; overlap.sequence = { 1, 1, 1, 1 }; overlap.timeWarp.noteLengths = false; overlap.gate = 2;
    check(render(overlap, 37) == render(overlap, 9500), "Warped same-pitch ownership depends on buffer size");
    auto bypass = a; bypass.timeWarp.enabled = false;
    auto zero = a; zero.timeWarp.amount = 0; zero.timeWarp.quantize = 8;
    check(render(bypass, 37) == render(zero, 37), "Disabled and zero warp must match legacy MIDI");

    PatternScheduler scheduler; scheduler.prepare(1000);
    juce::MidiBuffer trigger, before, after;
    scheduler.triggerKey(60, a, 60, 4, 1, trigger);
    scheduler.processBlock(before, 100, 60, 4, 1);
    scheduler.processBlock(after, 200, 120, 4, 1);
    bool off = false, on = false;
    for (const auto metadata : after)
    {
        const auto message = metadata.getMessage();
        if (metadata.samplePosition == 75 && message.isNoteOff() && message.getNoteNumber() == 48) off = true;
        if (metadata.samplePosition == 75 && message.isNoteOn() && message.getNoteNumber() == 60) on = true;
    }
    check(off && on, "Warped pending note-offs did not preserve phase on tempo change");
    juce::MidiBuffer stopped; scheduler.stopKey(60, stopped);
    check(stopped.getNumEvents() == 1 && !scheduler.isKeyActive(60), "Stopping warped playback left notes active");
}
