#include "../Source/PluginProcessor.h"
#include "../Source/Domain/AssignmentState.h"
#include "../Source/Domain/AssignmentHistory.h"
#include "../Source/Domain/SafeXml.h"
#include "../Source/VariationEditor.h"
#include "../Source/Domain/PatternBankFile.h"
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
    require(midi.getNumEvents() == 1 && (*midi.begin()).getMessage().isNoteOff()
            && (*midi.begin()).getMessage().getChannel() == 10
            && (*midi.begin()).getMessage().getNoteNumber() == 36,
            "Release must flush the old audition's note-off without starting the queued audition");

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
        const auto controls = [&]
        {
            juce::Array<juce::Component*> result;
            for (auto* child : editor->getChildren())
                if (auto* viewport = dynamic_cast<juce::Viewport*>(child))
                    result.addArray(viewport->getViewedComponent()->getChildren());
                else if (dynamic_cast<juce::Label*>(child) || dynamic_cast<juce::Button*>(child))
                    result.add(child);
            return result;
        };
        bool keyboardFound = false;
        for (auto* child : controls())
            if (auto* keyboard = dynamic_cast<juce::MidiKeyboardComponent*>(child))
            {
                keyboardFound = true;
                require(keyboard->getWhiteNoteText(60) == "C4"
                        && keyboard->getWhiteNoteText(48) == "C3",
                        "Keyboard octave labels do not match MIDI note numbers");
            }
        require(keyboardFound, "Editor keyboard missing");
        bool previewToggleFound = false;
        for (auto* child : controls())
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
            const auto all = controls();
            for (auto* child : all)
                if (child->isVisible())
                {
                    require(child->getHeight() >= 20 && child->getWidth() >= 24
                        && child->getParentComponent()->getLocalBounds().contains(child->getBounds()),
                        "A visible editor control is clipped or collapsed");
                    for (auto* other : all)
                        if (other != child && other->isVisible()
                            && other->getParentComponent() == child->getParentComponent()
                            && !dynamic_cast<juce::ListBox*>(child)
                            && !dynamic_cast<juce::ListBox*>(other))
                            require(!child->getBounds().intersects(other->getBounds()),
                                    "Visible editor controls overlap");
                }
        };
        for (auto* child : controls())
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
            for (auto* child : controls())
                if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    if (button->getButtonText() == caption)
                    {
                        button->onClick();
                        return;
                    }
            throw std::runtime_error("Editor action not found");
        };
        const auto saveSnapshot = [&](const char* filename)
        {
            auto file = juce::File::getCurrentWorkingDirectory().getChildFile(filename);
            file.deleteFile();
            juce::FileOutputStream stream(file);
            require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(
                editor->createComponentSnapshot(editor->getLocalBounds()), stream),
                "Could not save layout snapshot");
        };
        if (argc >= 5)
        {
            editor->setSize(640, 400);
            saveSnapshot(argv[4]);
            editor->setSize(820, 600);
        }
        if (argc >= 6)
        {
            click("Presets");
            saveSnapshot(argv[5]);
            click("Back to Editor");
        }
        // Window size must survive mode/browser changes. At smaller sizes all
        // controls must still fit the scrollable panel and be reachable by scrolling.
        for (const auto size : { juce::Point<int>(820, 600), juce::Point<int>(640, 400),
                                 juce::Point<int>(760, 480), juce::Point<int>(1100, 760) })
        {
            editor->setSize(size.x, size.y);
            for (auto* child : controls())
                if (auto* combo = dynamic_cast<juce::ComboBox*>(child))
                    if (combo->getItemText(0) == "Melodic")
                        for (int mode : { 1, 2 })
                        {
                            combo->setSelectedId(mode, juce::sendNotificationSync);
                            checkLayout();
                            require(editor->getWidth() == size.x && editor->getHeight() == size.y,
                                    "Changing mode resized the editor");
                        }
            for (const auto* caption : { "Presets", "Pattern Bank" })
            {
                click(caption);
                checkLayout();
                require(editor->getWidth() == size.x && editor->getHeight() == size.y,
                        "Opening a browser resized the editor");
                for (auto* child : editor->getChildren())
                    if (auto* viewport = dynamic_cast<juce::Viewport*>(child))
                    {
                        auto* panel = viewport->getViewedComponent();
                        viewport->setViewPosition(panel->getWidth(), panel->getHeight());
                        require(viewport->getViewArea().getRight() >= panel->getWidth()
                            && viewport->getViewArea().getBottom() >= panel->getHeight(),
                            "Cannot scroll to the last controls");
                    }
                click("Back to Editor");
                checkLayout();
            }
        }
        editor->setSize(820, 600);
        processor.setAssignmentForKey(60, pattern);
        editor.reset(processor.createEditor());
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
    if (argc >= 2)
    {
        juce::ScopedJuceInitialiser_GUI gui;
        NSeqArpKeysAudioProcessor p;
        auto source = drum(36);
        source.sequence = { 1, 2, 4, 8, 3, 5, 7, 0 };
        source.drumLaneCount = 4;
        source.name = "UI family"; source.tags = "fixture"; source.linkId = "shared-source";
        source.rotation = 2; source.reverse = true;
        p.restoreAssignments({ {60, source}, {61, source}, {62, source} });
        p.setSelectedKey(60);
        p.prepareToPlay(1000.0, 100);
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        auto mainButton = [&](const juce::String& caption)
        {
            for (auto* child : editor->getChildren())
                if (auto* viewport = dynamic_cast<juce::Viewport*>(child))
                    for (auto* control : viewport->getViewedComponent()->getChildren())
                        if (auto* button = dynamic_cast<juce::TextButton*>(control))
                            if (button->getButtonText() == caption) { button->onClick(); return; }
            throw std::runtime_error("Variation main action missing");
        };
        auto page = [&]() -> VariationEditor*
        {
            for (auto* child : editor->getChildren())
                if (auto* viewport = dynamic_cast<juce::Viewport*>(child))
                    for (auto* control : viewport->getViewedComponent()->getChildren())
                        if (auto* variations = dynamic_cast<VariationEditor*>(control))
                            if (variations->isVisible()) return variations;
            throw std::runtime_error("Variation page missing");
        };
        auto button = [&](const juce::String& caption) -> juce::TextButton*
        {
            for (auto* control : page()->getChildren())
                if (auto* b = dynamic_cast<juce::TextButton*>(control))
                    if (b->getButtonText() == caption) return b;
            throw std::runtime_error("Variation action missing");
        };
        auto slider = [&](const juce::String& id, double value)
        {
            for (auto* control : page()->getChildren())
                if (control->getComponentID() == id)
                    if (auto* s = dynamic_cast<juce::Slider*>(control))
                    { s->setValue(value, juce::sendNotificationSync); return; }
            throw std::runtime_error("Variation slider missing");
        };
        auto inverse = [&]() -> juce::ToggleButton*
        {
            for (auto* control : page()->getChildren())
                if (control->getComponentID() == "variation-inverse")
                    return dynamic_cast<juce::ToggleButton*>(control);
            throw std::runtime_error("Variation inverse missing");
        };
        auto mappingStatus = [&]() -> juce::String
        {
            for (auto* control : page()->getChildren())
                if (control->getComponentID() == "variation-status")
                    if (auto* label = dynamic_cast<juce::Label*>(control)) return label->getText();
            throw std::runtime_error("Variation status missing");
        };
        mainButton("Generate variations");
        for (const auto size : { juce::Point<int>(820, 600), juce::Point<int>(640, 400),
                                 juce::Point<int>(1100, 760) })
        {
            editor->setSize(size.x, size.y);
            for (auto* child : page()->getChildren())
                if (child->isVisible())
                {
                    require(child->getWidth() >= 24 && child->getHeight() >= 20
                            && page()->getLocalBounds().contains(child->getBounds()),
                            "Variation control clipped or collapsed");
                    for (auto* other : page()->getChildren())
                        if (child != other && other->isVisible())
                            require(!child->getBounds().intersects(other->getBounds()), "Variation controls overlap");
                }
            for (auto* child : editor->getChildren())
                if (auto* viewport = dynamic_cast<juce::Viewport*>(child))
                {
                    auto* panel = viewport->getViewedComponent();
                    viewport->setViewPosition(panel->getWidth(), panel->getHeight());
                    require(viewport->getViewArea().getBottom() >= panel->getHeight(),
                            "Cannot reach variation actions by scrolling");
                    viewport->setViewPosition(0, 0);
                }
        }
        const auto snapshot = [&](int argument, int width, int height)
        {
            if (argc <= argument) return;
            editor->setSize(width, height);
            juce::FileOutputStream output(juce::File::getCurrentWorkingDirectory().getChildFile(argv[argument]));
            require(output.openedOk() && output.setPosition(0) && output.truncate().wasOk()
                && juce::PNGImageFormat().writeImageToStream(
                editor->createComponentSnapshot(editor->getLocalBounds()), output), "Variation snapshot failed");
        };
        snapshot(6, 1100, 900); snapshot(7, 640, 400);
        slider("variation-key-count", 2);
        slider("variation-stride", 2);
        require(p.getAssignmentForKey(60) == source && p.getAssignmentForKey(61) == source,
                "Preview edited live assignments");
        require(inverse()->isEnabled(), "A permutation did not enable its inverse");
        inverse()->setToggleState(true, juce::dontSendNotification); inverse()->onClick();
        slider("variation-quadratic", 0); slider("variation-linear", 2);
        require(button("Assign variations")->isEnabled() && button("Save family to bank")->isEnabled()
                && button("Audition variation")->isEnabled() && !inverse()->isEnabled() && !inverse()->getToggleState()
                && mappingStatus().contains("From application 3, cycle length 1"),
                "Colliding polynomial blocked actions, retained inverse, or misreported its transient");
        if (argc > 6)
        {
            editor->setSize(1100, 900);
            const auto file = juce::File::getCurrentWorkingDirectory().getChildFile(argv[6])
                              .getSiblingFile("editor-variations-mapping.png");
            juce::FileOutputStream output(file);
            require(output.openedOk() && output.setPosition(0) && output.truncate().wasOk()
                && juce::PNGImageFormat().writeImageToStream(
                editor->createComponentSnapshot(editor->getLocalBounds()), output), "Mapping snapshot failed");
        }
        slider("variation-first-application", 1);
        button("Assign variations")->onClick();
        require(p.getAssignmentForKey(60).sequence == std::vector<SequenceValue>({1, 4, 3, 7, 1, 4, 3, 7})
                && p.getAssignmentForKey(61).sequence == std::vector<SequenceValue>(8, 1)
                && p.getAssignmentForKey(62) == source, "UI did not assign colliding-map variations");
        mainButton("Undo");
        require(p.getAssignmentForKey(60) == source && p.getAssignmentForKey(61) == source,
                "Colliding-map batch did not undo in one action");
        mainButton("Generate variations");
        slider("variation-key-count", 2); slider("variation-stride", 2);
        slider("variation-quadratic", 2); slider("variation-linear", 1);
        require(inverse()->isEnabled() && mappingStatus().contains("Permutation."),
                "Returning to a permutation did not restore inverse/status");
        button("Audition variation")->onClick(); button("Stop audition")->onClick();
        juce::AudioBuffer<float> audio(2, 100); juce::MidiBuffer midi;
        p.processBlock(audio, midi);
        require(midi.isEmpty() && p.getPlaybackStep(60) == -1, "Queued variation audition did not cancel");
        button("Audition original")->onClick();
        p.processBlock(audio, midi);
        require(p.isKeySounding(60) && p.getPlaybackStep(60) >= 0, "Variation audition/playhead did not start");
        button("Back to Editor")->onClick(); midi.clear(); p.processBlock(audio, midi);
        require(!p.isKeySounding(60) && p.getPlaybackStep(60) == -1, "Closing variations left playback active");
        mainButton("Generate variations");
        slider("variation-key-count", 2); slider("variation-stride", 2);
        button("Assign variations")->onClick();
        PatternVariations::Options options; options.keyCount = 2; options.applicationsBetweenKeys = 2;
        const auto expected = PatternVariations::build(source, options);
        require(p.getAssignmentForKey(60).sequence == source.sequence
                && p.getAssignmentForKey(61).sequence == expected.variations[1].assignment.sequence
                && p.getAssignmentForKey(60).linkId.empty() && p.getAssignmentForKey(61).linkId.empty()
                && p.getAssignmentForKey(62) == source, "UI batch assignment changed the wrong keys or linked copies");
        mainButton("Undo");
        require(p.getAssignmentForKey(60) == source && p.getAssignmentForKey(61) == source,
                "Variation batch did not undo in one action");
        mainButton("Redo");
        require(p.getAssignmentForKey(61).sequence == expected.variations[1].assignment.sequence,
                "Variation batch redo failed");
        mainButton("Undo"); mainButton("Generate variations");
        slider("variation-key-count", 2);
        const auto presetPath = juce::SystemStats::getEnvironmentVariable("NSEQARPKEYS_PRESET_DIR", {});
        if (presetPath.isNotEmpty())
        {
            const auto bank = juce::File(presetPath).getSiblingFile("Patterns");
            const int before = bank.findChildFiles(juce::File::findFiles, true, "*.nseqpattern").size();
            button("Save family to bank")->onClick();
            const auto files = bank.findChildFiles(juce::File::findFiles, true, "*.nseqpattern");
            require(files.size() == before + 2, "Family did not persist all patterns");
            int found = 0;
            for (const auto& file : files)
                if (auto xml = SafeXml::readFile(file, 1024 * 1024))
                {
                    const auto* assignment = xml->getChildByName("Assignment");
                    if (assignment == nullptr) continue;
                    const auto entry = AssignmentState::read(*assignment);
                    if (entry.name.find("UI family #") == 0)
                    {
                        ++found;
                        require(entry.linkId.empty() && entry.tags.find("fixture") != std::string::npos,
                                "Saved family lost tags or retained a shared link");
                    }
                }
            require(found >= 2, "Saved family could not be read back");
        }
        juce::MemoryBlock state; p.getStateInformation(state);
        p.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        slider("variation-first-application", 1);
        button("Assign variations")->onClick(); button("Audition original")->onClick();
        midi.clear(); p.processBlock(audio, midi);
        require(p.getAssignmentForKey(60) == source && !p.isKeySounding(60),
                "Stale variation preview acted across a project restore");
        editor.reset(); p.releaseResources();
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
