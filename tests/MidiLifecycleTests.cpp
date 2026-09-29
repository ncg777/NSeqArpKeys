#include "../Source/PluginProcessor.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

// Model a receiver that stacks note-ons instead of implicitly replacing them.
struct Receiver
{
    std::array<std::array<int, 128>, 16> voices {};
    int onsets = 0;

    void consume(const juce::MidiBuffer& midi)
    {
        for (const auto event : midi)
        {
            const auto message = event.getMessage();
            if (!message.isNoteOnOrOff()) continue;
            auto& count = voices[message.getChannel() - 1][message.getNoteNumber()];
            count += message.isNoteOn() ? 1 : -1;
            if (message.isNoteOn()) ++onsets;
            require(count >= 0, "An output note-off has no matching note-on");
            require(count <= 1, "Repeated note-ons stacked an unreleased voice");
        }
    }

    int sounding() const
    {
        int total = 0;
        for (const auto& channel : voices)
            for (int count : channel) total += count;
        return total;
    }
};

class PlayHead final : public juce::AudioPlayHead
{
public:
    bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo position;
        position.setBpm(120.0);
        position.setIsPlaying(playing);
        return position;
    }
};

KeyAssignment overlappingPattern(bool rhythmic)
{
    KeyAssignment assignment;
    assignment.setForteFromString("7-35.11");
    assignment.sequence = { 3, 1, 0, 3 };
    assignment.gate = 2.0f;
    assignment.fixedLengthSteps = 16.0f;
    assignment.channel = rhythmic ? 10 : 3;
    if (rhythmic)
    {
        assignment.mode = KeyAssignment::Mode::rhythmic;
        assignment.drumLaneCount = 2;
        // Duplicate drum pitches are legal and must also be balanced.
        assignment.drumNotes[0] = assignment.drumNotes[1] = 36;
    }
    return assignment;
}

enum class Stop
{
    keyUp, zeroVelocity, stopKey, stopAll, unlatch, latchToggle, clear,
    restore, allNotesOff, allSoundOff, transport, reset, release, prepare
};

void testStop(Stop stop, bool rhythmic)
{
    PlayHead playHead;
    NSeqArpKeysAudioProcessor processor;
    processor.setPlayHead(&playHead);
    processor.prepareToPlay(1000.0, 100);
    processor.setAssignmentForKey(60, overlappingPattern(rhythmic));
    const bool latched = stop != Stop::keyUp && stop != Stop::zeroVelocity;
    processor.setLatchEnabled(latched);
    juce::AudioBuffer<float> audio(2, 100);
    juce::MidiBuffer midi;
    Receiver receiver;
    auto process = [&]
    {
        processor.processBlock(audio, midi);
        receiver.consume(midi);
        midi.clear();
    };
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
    process();
    if (latched) midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    for (int block = 0; block < 35; ++block) process();
    require(receiver.onsets > 10 && receiver.sounding() > 0,
            "Stop fixture did not exercise repeated overlapping notes");

    const int onsetsBeforeStop = receiver.onsets;
    int expectedOffset = 0;
    switch (stop)
    {
        case Stop::keyUp:
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), expectedOffset = 17); break;
        case Stop::zeroVelocity:
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(0)), expectedOffset = 17); break;
        case Stop::stopKey: processor.requestStopKey(60); break;
        case Stop::stopAll: processor.requestStopAll(); break;
        case Stop::unlatch: processor.setLatchEnabled(false); break;
        case Stop::latchToggle:
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), expectedOffset = 17); break;
        case Stop::clear: processor.setPatternForKey(60, ""); break;
        case Stop::restore:
        {
            juce::MemoryBlock state;
            processor.getStateInformation(state);
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            break;
        }
        case Stop::allNotesOff:
            midi.addEvent(juce::MidiMessage::allNotesOff(1), expectedOffset = 17); break;
        case Stop::allSoundOff:
            midi.addEvent(juce::MidiMessage::allSoundOff(1), expectedOffset = 17); break;
        case Stop::transport:
            playHead.playing = false;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
            break;
        case Stop::reset: processor.reset(); break;
        case Stop::release:
            processor.releaseResources();
            processor.prepareToPlay(1000.0, 100);
            break;
        case Stop::prepare: processor.prepareToPlay(2000.0, 100); break;
    }
    // A zero-length host callback must not discard deferred releases.
    if (stop == Stop::release || stop == Stop::reset || stop == Stop::prepare)
    {
        juce::AudioBuffer<float> empty(2, 0);
        processor.processBlock(empty, midi);
    }
    processor.processBlock(audio, midi);
    require(!midi.isEmpty(), "Stopping an active pattern did not emit any releases");
    for (const auto event : midi)
        require(event.getMessage().isNoteOff() && event.samplePosition == expectedOffset,
                "Stop released at the wrong offset or emitted another onset");
    receiver.consume(midi);
    midi.clear();
    require(receiver.sounding() == 0 && !processor.isKeySounding(60),
            "Stopped pattern left a receiver voice or active pattern behind");
    for (int block = 0; block < 12; ++block) process();
    require(receiver.onsets == onsetsBeforeStop && receiver.sounding() == 0,
            "Stopped pattern resumed without a new trigger");
    require(audio.getMagnitude(0, audio.getNumSamples()) == 0.0f,
            "Preview voice remained audible after its release tail");

    if (stop == Stop::transport)
    {
        // Auditioning while the DAW remains stopped must still work.
        processor.auditionPattern(60, overlappingPattern(rhythmic));
        process();
        require(receiver.sounding() > 0, "Stopped transport disabled manual audition");
        processor.requestStopAll();
        process();
        require(receiver.sounding() == 0, "Audition stop left output voices behind");
    }
}

void testSharedPitchAndReplacement()
{
    NSeqArpKeysAudioProcessor processor;
    processor.prepareToPlay(1000.0, 100);
    const auto pattern = overlappingPattern(true);
    processor.setAssignmentForKey(60, pattern);
    processor.setAssignmentForKey(61, pattern);
    processor.setLatchEnabled(true);
    juce::AudioBuffer<float> audio(2, 100);
    juce::MidiBuffer midi;
    Receiver receiver;
    auto process = [&]
    {
        processor.processBlock(audio, midi);
        receiver.consume(midi);
        midi.clear();
    };
    for (int key : { 60, 61 })
        midi.addEvent(juce::MidiMessage::noteOn(1, key, static_cast<juce::uint8>(100)), 0);
    for (int block = 0; block < 20; ++block) process();
    processor.requestStopKey(60);
    process();
    require(receiver.sounding() == 1 && processor.isKeySounding(61),
            "Stopping one key interrupted another owner of the same pitch");
    auto replacement = pattern;
    replacement.channel = 16;
    replacement.drumNotes[0] = replacement.drumNotes[1] = 127;
    processor.setAssignmentForKey(61, replacement);
    process();
    require(receiver.voices[9][36] == 0 && receiver.voices[15][127] == 1,
            "Live replacement lost the old pitch/channel release");
    processor.requestStopAll();
    process();
    require(receiver.sounding() == 0, "Replacement left a stuck note");
}
}

int main()
{
    try
    {
        for (bool rhythmic : { false, true })
            for (auto stop : { Stop::keyUp, Stop::zeroVelocity, Stop::stopKey, Stop::stopAll,
                               Stop::unlatch, Stop::latchToggle, Stop::clear, Stop::restore,
                               Stop::allNotesOff, Stop::allSoundOff, Stop::transport,
                               Stop::reset, Stop::release, Stop::prepare })
                testStop(stop, rhythmic);
        testSharedPitchAndReplacement();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
