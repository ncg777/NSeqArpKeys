#include "../Source/PluginProcessor.h"
#include "../Source/Domain/AssignmentState.h"
#include <stdexcept>
#include <iostream>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

KeyAssignment drum(int note)
{
    KeyAssignment a;
    a.mode = KeyAssignment::Mode::rhythmic;
    a.sequence = { 1 };
    a.channel = 10;
    a.drumNotes[0] = note;
    a.gate = 1.0f;
    return a;
}

void restoreXml(NSeqArpKeysAudioProcessor& p, const juce::XmlElement& state)
{
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary(state, data);
    p.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
}
}

int main(int argc, char** argv)
{
    NSeqArpKeysAudioProcessor processor;
    auto pattern = drum(36);
    pattern.name = "Saved pattern";
    pattern.subdivision = 3;
    pattern.velocity = 89;
    pattern.velocitySteps = { 127, 0, 64 };
    pattern.pitchSteps = { 0, 7 };
    pattern.transpose = -5;
    pattern.rotation = 2;
    pattern.reverse = true;
    pattern.rootKey = 48;
    pattern.fixedLengthSteps = 0.25f;
    pattern.setForteFromString("1-1.0");
    require(pattern.hasValidForte(), "State fixture must include a valid Forte set");
    processor.setAssignmentForKey(60, pattern);
    auto empty = KeyAssignment{};
    empty.sequence.clear();
    processor.setAssignmentForKey(61, empty);
    auto rests = KeyAssignment{};
    rests.sequence = { 0, 0, 0 };
    processor.setAssignmentForKey(62, rests);
    processor.setSelectedKey(62);
    processor.getMeterDenominator()->setValueNotifyingHost(
        processor.getMeterDenominator()->convertTo0to1(7));

    juce::MemoryBlock saved;
    processor.getStateInformation(saved);
    NSeqArpKeysAudioProcessor restored;
    restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    require(restored.getAssignmentForKey(60) == pattern, "DAW state lost 1.2.0 fields");
    require(restored.getAssignmentForKey(61).sequence.empty(), "Empty pattern was replaced on restore");
    require(restored.getAssignmentForKey(62).sequence.size() == 3, "All-rest loop length was lost");
    require(restored.getSelectedKey() == 62 && restored.getMeterDenominator()->get() == 7,
            "Selection or global timing was lost");

    juce::XmlElement file("Assignment");
    AssignmentState::write(file, pattern);
    const auto decoded = juce::XmlDocument::parse(file.toString());
    require(decoded != nullptr && AssignmentState::read(*decoded) == pattern,
            "Individual pattern export/import must preserve every setting");

    juce::XmlElement legacy("NSeqArpKeys");
    legacy.setAttribute("meterDenominator", 3);
    auto* old = legacy.createNewChildElement("Assignment");
    old->setAttribute("key", 60);
    old->setAttribute("sequence", "1 0");
    old->setAttribute("forte", "1-1.0");
    restoreXml(restored, legacy);
    const auto migrated = restored.getAssignmentForKey(60);
    require(migrated.subdivision == 0 && migrated.effectiveSubdivision(3) == 3
            && migrated.fixedLengthSteps == 0 && migrated.mode == KeyAssignment::Mode::melodic,
            "Legacy preset must inherit global timing with compatible defaults");

    processor.copyAssignmentToRange(60, 63, 65, true);
    require(processor.getAssignmentForKey(64).transpose == pattern.transpose + 4,
            "Range transpose did not use source key distance");
    auto independent = processor.getAssignmentForKey(64);
    independent.name = "Independent";
    processor.setAssignmentForKey(64, independent);
    require(processor.getAssignmentForKey(60).name == pattern.name
            && processor.getAssignmentForKey(63).name == pattern.name,
            "Range copies must be independent");

    // Real processor callback: sample-accurate triggering, release, editing,
    // isolated pattern replacement and per-key timing changes.
    NSeqArpKeysAudioProcessor playback;
    playback.prepareToPlay(1000.0, 1000);
    playback.getMeterDenominator()->setValueNotifyingHost(0.0f); // 1 step/QN, 120 BPM
    auto a = drum(36);
    a.velocity = 100;
    playback.setAssignmentForKey(60, a);
    playback.setAssignmentForKey(61, drum(38));
    juce::AudioBuffer<float> audio(2, 100);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(64)), 25);
    midi.addEvent(juce::MidiMessage::noteOn(1, 61, static_cast<juce::uint8>(127)), 25);
    playback.processBlock(audio, midi);
    require(midi.getNumEvents() == 2 && (*midi.begin()).samplePosition == 25,
            "Input onset offset must be preserved");
    a.name = "Edited while held";
    playback.setAssignmentForKey(60, a);
    midi.clear();
    playback.processBlock(audio, midi);
    bool restartedQuietly = false;
    for (const auto event : midi)
    {
        const auto message = event.getMessage();
        require(message.getNoteNumber() != 38, "Editing/importing one key interrupted another");
        if (message.isNoteOn()) restartedQuietly = message.getVelocity() == 100 * 64 / 127;
    }
    require(restartedQuietly, "Editing a held pattern lost trigger velocity");

    // The held key is 100/500 of a step into its restarted pattern. Changing
    // its subdivision to 2 leaves 200 samples until the next onset.
    a.subdivision = 2;
    playback.setAssignmentForKey(60, a);
    audio.setSize(2, 201);
    midi.clear();
    playback.processBlock(audio, midi);
    bool nextAt200 = false;
    for (const auto event : midi)
        if (event.getMessage().isNoteOn() && event.getMessage().getNoteNumber() == 36)
            nextAt200 = event.samplePosition == 200;
    require(nextAt200, "Editing per-key timing restarted the loop instead of preserving phase");

    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 17);
    playback.processBlock(audio, midi);
    bool releaseAt17 = false;
    for (const auto event : midi)
        if (event.getMessage().isNoteOff() && event.getMessage().getNoteNumber() == 36)
            releaseAt17 = event.samplePosition == 17;
    require(releaseAt17, "Input release offset must be preserved");
    playback.requestStopAll();
    midi.clear();
    playback.processBlock(audio, midi);
    require(midi.getNumEvents() == 1 && (*midi.begin()).getMessage().isNoteOff(),
            "Stop All must release the other active key");

    if (argc == 2)
    {
        juce::ScopedJuceInitialiser_GUI gui;
        processor.setSelectedKey(60);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Editor failed to open");
        auto click = [&](const juce::String& caption)
        {
            for (auto* child : editor->getChildren())
                if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    if (button->getButtonText() == caption)
                    {
                        button->onClick();
                        return;
                    }
            throw std::runtime_error("Editor action not found");
        };
        click("Duplicate to next key");
        require(processor.getAssignmentForKey(61) == pattern, "Editor duplicate lost settings");
        click("Undo");
        require(processor.getAssignmentForKey(61).sequence.empty(), "Editor duplicate undo failed");
        click("Redo");
        require(processor.getAssignmentForKey(61) == pattern, "Editor redo failed");
        // A project restore must invalidate prior edit history before the next timer tick.
        processor.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        click("Undo");
        require(processor.getAssignmentForKey(61).sequence.empty(), "Undo crossed a project restore");
        click("Assign range");
        require(processor.getAssignmentForKey(48).sequence == rests.sequence, "Range action did not apply");
        click("Undo");
        require(processor.getAssignmentForKey(60) == pattern
                && processor.getAssignmentForKey(61).sequence.empty(), "Range undo lost assignments");
        processor.setSelectedKey(60);
        editor.reset();
        editor.reset(processor.createEditor());
        auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());
        juce::FileOutputStream output(juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]));
        require(output.openedOk() && juce::PNGImageFormat().writeImageToStream(snapshot, output),
                "Could not save editor snapshot");
        editor.reset();
        editor.reset(processor.createEditor());
        require(processor.getAssignmentForKey(60) == pattern, "Reopening editor changed pattern state");
    }
    std::cout << "Processor state and playback tests passed\n";
}
