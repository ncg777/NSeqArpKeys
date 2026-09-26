#include "../Source/PluginProcessor.h"
#include "../Source/Domain/AssignmentState.h"
#include "../Source/Domain/AssignmentHistory.h"
#include "../Source/Domain/SafeXml.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

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

int runTests(int argc, char** argv)
{
    NSeqArpKeysAudioProcessor processor;
    auto pattern = drum(36);
    pattern.name = "Saved pattern";
    pattern.subdivision = 3;
    pattern.velocity = 89;
    pattern.drumLaneCount = 4;
    pattern.drumVelocityBits = 4;
    pattern.tags = "drums, test";
    pattern.colour = "#F4BD68";
    pattern.favourite = true;
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
    processor.setPreviewSoundEnabled(false);
    processor.getMeterDenominator()->setValueNotifyingHost(
        processor.getMeterDenominator()->convertTo0to1(7));

    juce::MemoryBlock saved;
    processor.getStateInformation(saved);
    NSeqArpKeysAudioProcessor restored;
    restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    require(restored.getAssignmentForKey(60) == pattern, "DAW state lost 1.3.0 fields");
    require(!restored.isPreviewSoundEnabled(), "Preview sound setting was not restored");
    require(restored.getAssignmentForKey(61).sequence.empty(), "Empty pattern was replaced on restore");
    require(restored.getAssignmentForKey(62).sequence.size() == 3, "All-rest loop length was lost");
    require(restored.getSelectedKey() == 62 && restored.getMeterDenominator()->get() == 7,
            "Selection or global timing was lost");

    juce::XmlElement file("Assignment");
    AssignmentState::write(file, pattern);
    require(file.getStringAttribute("drumVelocityBits") == "4",
            "Velocity bits must serialize as one numeric parameter");
    const auto decoded = juce::XmlDocument::parse(file.toString());
    require(decoded != nullptr && AssignmentState::read(*decoded) == pattern,
            "Individual pattern export/import must preserve every setting");
    auto shortDrums = drum(36);
    shortDrums.drumLaneCount = 3;
    shortDrums.drumVelocityBits = 7;
    require(shortDrums.setSequenceFromString("40564819207303340847894502572032"),
            "Wide rhythmic mask fixture was rejected");
    juce::XmlElement shortFile("Assignment");
    AssignmentState::write(shortFile, shortDrums);
    require(AssignmentState::read(shortFile) == shortDrums,
            "Variable drum lane count, shared velocity bits or wide mask were lost");
    shortDrums.mode = KeyAssignment::Mode::melodic;
    AssignmentState::write(shortFile, shortDrums);
    require(AssignmentState::read(shortFile) == shortDrums,
            "Switching modes must not lose wide masks when saving a pattern");
    processor.setAssignmentForKey(63, shortDrums);
    juce::MemoryBlock switchedState;
    processor.getStateInformation(switchedState);
    restored.setStateInformation(switchedState.getData(), static_cast<int>(switchedState.getSize()));
    require(restored.getAssignmentForKey(63) == shortDrums,
            "Host restore lost a wide mask after a mode switch");

    juce::XmlElement legacy("NSeqArpKeys");
    legacy.setAttribute("meterDenominator", 3);
    auto* old = legacy.createNewChildElement("Assignment");
    old->setAttribute("key", 60);
    old->setAttribute("sequence", "1 0");
    old->setAttribute("forte", "1-1.0");
    restoreXml(restored, legacy);
    require(restored.isPreviewSoundEnabled(), "Legacy state should retain audible preview by default");
    const auto migrated = restored.getAssignmentForKey(60);
    require(migrated.subdivision == 0 && migrated.effectiveSubdivision(3) == 3
            && migrated.fixedLengthSteps == 0 && migrated.mode == KeyAssignment::Mode::melodic,
            "Legacy preset must inherit global timing with compatible defaults");
    const juce::String hostileXml = "<?xml version=\"1.0\"?><!DOCTYPE NSeqArpKeys "
        "[<!ENTITY x \"unexpected\">]><NSeqArpKeys><Assignment key=\"60\" "
        "sequence=\"&x;\"/></NSeqArpKeys>";
    juce::MemoryBlock hostile;
    hostile.append(saved.getData(), 8); // Keep JUCE's state magic.
    const auto xmlLength = static_cast<juce::uint32>(hostileXml.getNumBytesAsUTF8());
    hostile.append(hostileXml.toRawUTF8(), xmlLength + 1);
    const auto littleLength = juce::ByteOrder::swapIfBigEndian(xmlLength);
    std::memcpy(static_cast<char*>(hostile.getData()) + 4, &littleLength, 4);
    restored.setStateInformation(hostile.getData(), static_cast<int>(hostile.getSize()));
    require(restored.getAssignmentForKey(60) == migrated,
            "A DTD-bearing host state was accepted before safe validation");
    require(SafeXml::parse(hostileXml) == nullptr, "Pattern/preset parser accepted a DTD");
    juce::String nested;
    for (int i = 0; i < 1024; ++i) nested += "<NSeqArpKeys>";
    for (int i = 0; i < 1024; ++i) nested += "</NSeqArpKeys>";
    require(SafeXml::parse(nested) == nullptr, "Deeply nested XML reached the recursive parser");
    hostile.setSize(8);
    const auto nestedLength = static_cast<juce::uint32>(nested.getNumBytesAsUTF8());
    hostile.append(nested.toRawUTF8(), nestedLength + 1);
    const auto nestedLittleLength = juce::ByteOrder::swapIfBigEndian(nestedLength);
    std::memcpy(static_cast<char*>(hostile.getData()) + 4, &nestedLittleLength, 4);
    restored.setStateInformation(hostile.getData(), static_cast<int>(hostile.getSize()));
    require(restored.getAssignmentForKey(60) == migrated, "Deeply nested host XML changed state");
    hostile.setSize(9);
    const juce::uint32 oneByte = juce::ByteOrder::swapIfBigEndian(static_cast<juce::uint32>(1));
    std::memcpy(static_cast<char*>(hostile.getData()) + 4, &oneByte, 4);
    static_cast<char*>(hostile.getData())[8] = static_cast<char>(0xe2);
    restored.setStateInformation(hostile.getData(), static_cast<int>(hostile.getSize()));
    require(restored.getAssignmentForKey(60) == migrated, "Truncated UTF-8 host state was accepted");
    require(SafeXml::parse("<?xml version=\"1.0\"?><NSeqArpKeys><!-- <ignored> -->"
                          "<Assignment name=\"A &amp; B > C\"/><![CDATA[<text>]]></NSeqArpKeys>") != nullptr,
            "Safe XML comments, escaped attributes or CDATA were rejected");
    juce::String manyAttributes = "<NSeqArpKeys";
    for (int i = 0; i < 65; ++i) manyAttributes += " a" + juce::String(i) + "=\"x\"";
    require(SafeXml::parse(manyAttributes + "/>") == nullptr, "XML attribute limit was not enforced");
    std::vector<char> oversizedState(16 * 1024 * 1024 + 1);
    restored.setStateInformation(oversizedState.data(), static_cast<int>(oversizedState.size()));
    require(restored.getAssignmentForKey(60) == migrated,
            "Oversized host state changed assignments");

    processor.copyAssignmentToRange(60, 63, 65, true);
    require(processor.getAssignmentForKey(64).transpose == pattern.transpose + 4,
            "Range transpose did not use source key distance");
    auto independent = processor.getAssignmentForKey(64);
    independent.name = "Independent";
    processor.setAssignmentForKey(64, independent);
    require(processor.getAssignmentForKey(60).name == pattern.name
            && processor.getAssignmentForKey(63).name == pattern.name,
            "Range copies must be independent");

    auto linked = drum(40);
    linked.linkId = "library-example";
    processor.setAssignmentForKey(70, linked);
    processor.setAssignmentForKey(71, linked);
    linked.name = "Shared edit";
    processor.setAssignmentForKey(70, linked);
    require(processor.getAssignmentForKey(71).name == "Shared edit",
            "Linked assignments did not follow edits");
    auto separate = processor.getAssignmentForKey(71);
    separate.linkId.clear();
    processor.setAssignmentForKey(71, separate);
    linked.name = "Another shared edit";
    processor.setAssignmentForKey(70, linked);
    require(processor.getAssignmentForKey(71).name == "Shared edit",
            "Make independent did not break the link");
    juce::MemoryBlock sharedState;
    processor.getStateInformation(sharedState);
    NSeqArpKeysAudioProcessor onCleanInstall;
    onCleanInstall.setStateInformation(sharedState.getData(), static_cast<int>(sharedState.getSize()));
    require(onCleanInstall.getAssignmentForKey(70) == linked,
            "Whole preset lost embedded shared pattern definition");

    // Linked edits capture the complete inverse before restoring any key.
    processor.setAssignmentForKey(71, linked);
    AssignmentHistory history;
    history.record({ 70, { { 70, linked }, { 71, linked } } });
    auto editedLink = linked;
    editedLink.name = "Redo both members";
    processor.setAssignmentForKey(70, editedLink);
    const auto get = [&](int key) { return processor.getAssignmentForKey(key); };
    const auto set = [&](const auto& assignments) { processor.restoreAssignments(assignments); };
    history.undo(get, set);
    require(get(70) == linked && get(71) == linked, "Linked undo failed");
    history.redo(get, set);
    require(get(70) == editedLink && get(71) == editedLink, "Linked redo captured a partially undone edit");
    auto detached = editedLink;
    detached.linkId.clear();
    detached.name = "Independent replacement";
    processor.restoreAssignments({ { 70, detached }, { 71, linked } });
    require(get(70) == detached && get(71) == linked, "Snapshot restore propagated outside its keys");

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

    NSeqArpKeysAudioProcessor linkedPlayback;
    linkedPlayback.prepareToPlay(1000.0, 100);
    linkedPlayback.getMeterDenominator()->setValueNotifyingHost(0.0f);
    auto linkedBeat = drum(36);
    linkedBeat.linkId = "timing";
    linkedPlayback.setAssignmentForKey(60, linkedBeat);
    linkedPlayback.setAssignmentForKey(61, linkedBeat);
    audio.setSize(2, 100);
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(127)), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 61, static_cast<juce::uint8>(127)), 0);
    linkedPlayback.processBlock(audio, midi);
    linkedBeat.subdivision = 2;
    linkedPlayback.setAssignmentForKey(60, linkedBeat);
    midi.clear();
    audio.setSize(2, 201);
    linkedPlayback.processBlock(audio, midi);
    int linkedOnsets = 0;
    for (const auto event : midi)
        if (event.getMessage().isNoteOn())
        {
            ++linkedOnsets;
            require(event.samplePosition == 200, "A linked timing edit restarted a follower");
        }
    require(linkedOnsets == 2, "Linked timing did not preserve both phases");
    linkedBeat.name = "Edited before Stop";
    linkedPlayback.setAssignmentForKey(60, linkedBeat);
    linkedPlayback.requestStopAll();
    midi.clear();
    linkedPlayback.processBlock(audio, midi);
    for (const auto event : midi)
        require(!event.getMessage().isNoteOn(), "Stop retriggered a pending pattern edit");

    NSeqArpKeysAudioProcessor audition;
    require(!audition.isKeySounding(60), "Sounding indicators must start inactive");
    audition.prepareToPlay(1000.0, 100);
    audio.setSize(2, 100);
    for (bool stopAll : { false, true })
    {
        audition.auditionPattern(60, drum(36));
        if (stopAll) audition.requestStopAll(); else audition.requestStopKey(60);
        midi.clear();
        audition.processBlock(audio, midi);
        require(midi.isEmpty() && !audition.isKeySounding(60), "Stop did not cancel a queued audition");
    }
    audition.auditionPattern(60, drum(36));
    midi.clear();
    audition.processBlock(audio, midi);
    require(audition.isKeySounding(60), "Audition did not start");
    audition.auditionPattern(61, drum(38));
    audition.releaseResources();
    require(!audition.isKeySounding(60), "Release left a stale sounding indicator");
    audition.prepareToPlay(1000.0, 100);
    midi.clear();
    audition.processBlock(audio, midi);
    require(midi.isEmpty(), "Release left a queued audition");

    // Reported rhythmic pattern, without a Forte set.
    // Preview-key input follows the same queue used by the editor keyboard.
    NSeqArpKeysAudioProcessor rhythm;
    rhythm.prepareToPlay(48000.0, 512);
    KeyAssignment beat;
    beat.mode = KeyAssignment::Mode::rhythmic;
    beat.channel = 10;
    beat.sequence = { 1, 0, 0, 0, 2, 0, 0, 0 };
    beat.subdivision = 4;
    rhythm.setAssignmentForKey(60, beat);
    std::vector<int> drumOnsets;
    juce::AudioBuffer<float> drumAudio(2, 512);
    juce::MidiBuffer drumMidi;
    rhythm.queuePreviewMidiMessage(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(127)));
    for (int block = 0; block < 94; ++block)
    {
        if (block == 2) rhythm.setPreviewSoundEnabled(false);
        if (block == 60) rhythm.setPreviewSoundEnabled(true);
        rhythm.processBlock(drumAudio, drumMidi);
        if (block >= 2 && block < 60)
            require(drumAudio.getMagnitude(0, drumAudio.getNumSamples()) == 0.0f,
                    "MIDI-only playback leaked preview audio");
        if (block == 0 || block == 60)
            require(drumAudio.getMagnitude(0, drumAudio.getNumSamples()) > 0.0f,
                    "Enabling preview did not restore sound during playback");
        for (const auto event : drumMidi)
            if (event.getMessage().isNoteOn())
            {
                require(event.getMessage().getChannel() == 10, "Rhythm changed output routing");
                require(event.getMessage().getVelocity() == 100, "Empty lane muted rhythm");
                drumOnsets.push_back(event.getMessage().getNoteNumber());
            }
        drumMidi.clear();
    }
    require(drumOnsets == std::vector<int>({ 36, 38, 36 }),
            "Rhythmic pattern must play kick/snare and loop without a Forte set");

    if (argc >= 2)
    {
        juce::ScopedJuceInitialiser_GUI gui;
        processor.setSelectedKey(60);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Editor failed to open");
        bool keyboardFound = false;
        for (auto* child : editor->getChildren())
            if (auto* keyboard = dynamic_cast<juce::MidiKeyboardComponent*>(child))
            {
                keyboardFound = true;
                require(keyboard->getWhiteNoteText(60) == "C4"
                        && keyboard->getWhiteNoteText(48) == "C3",
                        "Keyboard octave labels do not match MIDI note numbers");
            }
        require(keyboardFound, "Editor keyboard missing");
        bool previewToggleFound = false;
        for (auto* child : editor->getChildren())
            if (auto* toggle = dynamic_cast<juce::ToggleButton*>(child))
                if (toggle->getButtonText() == "Preview sound")
                {
                    previewToggleFound = true;
                    require(!toggle->getToggleState(), "Editor lost the saved preview setting");
                    toggle->setToggleState(true, juce::dontSendNotification);
                    toggle->onClick();
                    require(processor.isPreviewSoundEnabled(), "Preview toggle did not enable sound");
                    toggle->setToggleState(false, juce::dontSendNotification);
                    toggle->onClick();
                    require(!processor.isPreviewSoundEnabled(), "Preview toggle did not mute sound");
                }
        require(previewToggleFound, "Preview sound control missing");
        const auto checkLayout = [&]
        {
            for (auto* child : editor->getChildren())
                if (child->isVisible())
                    require(child->getHeight() >= 16 && editor->getLocalBounds().contains(child->getBounds()),
                            "A visible editor control is clipped or collapsed");
        };
        for (auto* child : editor->getChildren())
            if (auto* combo = dynamic_cast<juce::ComboBox*>(child))
                if (combo->getItemText(0) == "Melodic")
                {
                    processor.setChannelForKey(60, 1);
                    combo->setSelectedId(1, juce::sendNotificationSync);
                    checkLayout();
                    if (argc >= 4)
                    {
                        const auto file = juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]);
                        file.deleteFile();
                        juce::FileOutputStream stream(file);
                        require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(
                            editor->createComponentSnapshot(editor->getLocalBounds()), stream),
                            "Could not save melodic editor snapshot");
                    }
                    combo->setSelectedId(2, juce::sendNotificationSync);
                    checkLayout();
                    require(processor.getAssignmentForKey(60).channel == 1,
                            "Switching to rhythmic mode changed the selected MIDI channel");
                    processor.setAssignmentForKey(60, pattern);
                }
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
        const auto snapshotFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        snapshotFile.deleteFile();
        juce::FileOutputStream output(snapshotFile);
        require(output.openedOk() && juce::PNGImageFormat().writeImageToStream(snapshot, output),
                "Could not save editor snapshot");
        editor.reset();
        editor.reset(processor.createEditor());
        require(processor.getAssignmentForKey(60) == pattern, "Reopening editor changed pattern state");
        if (argc >= 3)
        {
            click("Pattern Bank");
            checkLayout();
            auto bankSnapshot = editor->createComponentSnapshot(editor->getLocalBounds());
            const auto bankFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
            bankFile.deleteFile();
            juce::FileOutputStream bankOutput(bankFile);
            require(bankOutput.openedOk()
                && juce::PNGImageFormat().writeImageToStream(bankSnapshot, bankOutput),
                "Could not save Pattern Bank snapshot");

            // Run with an isolated, nonempty bank to exercise the actual UI callbacks.
            if (juce::SystemStats::getEnvironmentVariable("NSEQARPKEYS_PRESET_DIR", {}).isNotEmpty())
            {
                click("Assign link");
                auto bankLink = processor.getAssignmentForKey(60);
                require(!bankLink.linkId.empty(), "UI bank fixture must contain a pattern");
                bankLink.name = "Edited shared definition";
                processor.setAssignmentForKey(60, bankLink);
                const auto previous62 = processor.getAssignmentForKey(62);
                processor.setSelectedKey(62);
                click("Pattern Bank");
                click("Assign link");
                require(processor.getAssignmentForKey(60) == bankLink
                    && processor.getAssignmentForKey(62) == bankLink,
                    "Joining a bank link overwrote the project's edited definition");
                click("Undo");
                require(processor.getAssignmentForKey(60) == bankLink
                    && processor.getAssignmentForKey(62) == previous62, "Bank link undo failed");
                click("Redo");
                require(processor.getAssignmentForKey(60) == bankLink
                    && processor.getAssignmentForKey(62) == bankLink, "Bank link redo failed");

                processor.prepareToPlay(1000.0, 100);
                click("Pattern Bank");
                click("Audition");
                click("Stop audition");
                midi.clear();
                processor.processBlock(audio, midi);
                require(midi.isEmpty(), "UI Stop audition failed before the first audio callback");
                click("Audition");
                click("Back to Editor");
                midi.clear();
                processor.processBlock(audio, midi);
                require(midi.isEmpty(), "Closing the bank left a queued audition");
                click("Pattern Bank");
                click("Audition");
                midi.clear();
                processor.processBlock(audio, midi);
                require(processor.isKeySounding(62), "UI audition did not start");
                editor.reset();
                midi.clear();
                processor.processBlock(audio, midi);
                require(!processor.isKeySounding(62), "Closing the editor left its audition running");
                processor.releaseResources();
            }
        }
    }
    std::cout << "Processor state and playback tests passed\n";
    return 0;
}

int main(int argc, char** argv)
{
    try { return runTests(argc, argv); }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
