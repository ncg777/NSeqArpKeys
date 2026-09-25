#include "PluginEditor.h"
#include "Engine/GateRunnerEngine.h"
#include "Domain/AssignmentState.h"
#include "Domain/SafeXml.h"
#include "Domain/PatternBankFile.h"
#if __has_include("FourthAtlasData.h")
#include "FourthAtlasData.h"
#else
#include <BinaryData.h>
#endif
#include <iterator>

namespace
{
struct BuiltInBank { const char* filename; const char* label; };
constexpr BuiltInBank builtInBanks[] {
    { "00-START-HERE.nseqbank", "Fourth Atlas: Starter" },
    { "01-filigree-melodic.nseqbank", "Filigree: Melodic" },
    { "02-filigree-rhythmic.nseqbank", "Filigree: Rhythmic" },
    { "03-antiphon-melodic.nseqbank", "Antiphon: Melodic" },
    { "04-antiphon-rhythmic.nseqbank", "Antiphon: Rhythmic" },
    { "05-embers-melodic.nseqbank", "Embers: Melodic" },
    { "06-embers-rhythmic.nseqbank", "Embers: Rhythmic" },
    { "07-kaleidoscope-melodic.nseqbank", "Kaleidoscope: Melodic" },
    { "08-kaleidoscope-rhythmic.nseqbank", "Kaleidoscope: Rhythmic" },
    { "09-lattice-melodic.nseqbank", "Lattice: Melodic" },
    { "10-lattice-rhythmic.nseqbank", "Lattice: Rhythmic" },
    { "11-orbit-melodic.nseqbank", "Orbit: Melodic" },
    { "12-orbit-rhythmic.nseqbank", "Orbit: Rhythmic" },
};

juce::File presetDirectory()
{
    const auto custom = juce::SystemStats::getEnvironmentVariable("NSEQARPKEYS_PRESET_DIR", {});
    if (custom.isNotEmpty() && juce::File::isAbsolutePath(custom))
        return juce::File(custom);
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NSeqArpKeys").getChildFile("Presets");
}

juce::File patternDirectory()
{
    return presetDirectory().getParentDirectory().getChildFile("Patterns");
}

std::unique_ptr<juce::XmlElement> parseSafeXml(const juce::String& text)
{
    return SafeXml::parse(text);
}

std::unique_ptr<juce::XmlElement> parseSafeXmlFile(const juce::File& file, int64_t limit)
{
    return SafeXml::readFile(file, limit);
}

bool writeXmlFile(const juce::File& target, const juce::XmlElement& xml)
{
    if (!target.getParentDirectory().createDirectory().wasOk())
        return false;
    juce::TemporaryFile temporary(target);
    return temporary.getFile().replaceWithText(xml.toString())
        && temporary.overwriteTargetFileWithTemporary();
}

bool validPresetState(const juce::XmlElement* state)
{
    if (state == nullptr || !state->hasTagName("NSeqArpKeys"))
        return false;
    int assignments = 0;
    int definitions = 0;
    std::array<bool, 128> seenKeys {};
    for (auto* child : state->getChildIterator())
    {
        if (child->hasTagName("SharedPattern"))
        {
            if (++definitions > 128 || child->getStringAttribute("id").length() > 80
                || child->getStringAttribute("sequence").length() > 45056)
                return false;
            continue;
        }
        const int key = child->getIntAttribute("key", -1);
        if (!child->hasTagName("Assignment")
            || key < 0 || key > 127
            || seenKeys[static_cast<size_t>(key)]
            || child->getStringAttribute("sequence").length() > 45056
            || child->getStringAttribute("name").length() > 80
            || child->getStringAttribute("tags").length() > 200
            || child->getStringAttribute("linkId").length() > 80
            || ++assignments > 128)
            return false;
        seenKeys[static_cast<size_t>(key)] = true;
    }
    return true;
}
}

//==============================================================================
NSeqArpKeysAudioProcessorEditor::NSeqArpKeysAudioProcessorEditor(NSeqArpKeysAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    theme.setColourScheme(juce::LookAndFeel_V4::getDarkColourScheme());
    theme.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff294354));
    theme.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff397c81));
    theme.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffedf5fa));
    theme.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff172431));
    theme.setColour(juce::TextEditor::textColourId, juce::Colour(0xffeef6f9));
    theme.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff365164));
    theme.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff62d6c6));
    theme.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff203342));
    theme.setColour(juce::ComboBox::textColourId, juce::Colour(0xffedf5fa));
    theme.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff365164));
    theme.setColour(juce::Slider::thumbColourId, juce::Colour(0xff62d6c6));
    theme.setColour(juce::Slider::trackColourId, juce::Colour(0xff4a8791));
    theme.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffedf5fa));
    theme.setColour(juce::Label::textColourId, juce::Colour(0xffc8d9e2));
    theme.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff172431));
    setLookAndFeel(&theme);
    // ----- Keyboard -----------------------------------------------------------
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);
    keyboardComponent.addMouseListener(this, true);

    presetNameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(presetNameLabel);
    previousPresetButton.setButtonText("<");
    nextPresetButton.setButtonText(">");
    browsePresetsButton.setButtonText("Browse Presets");
    browsePatternsButton.setButtonText("Pattern Bank");
    savePresetButton.setButtonText("Save");
    previousPresetButton.onClick = [this] { navigatePreset(-1); };
    nextPresetButton.onClick = [this] { navigatePreset(1); };
    browsePresetsButton.onClick = [this] { setBrowserOpen(!browserOpen); };
    browsePatternsButton.onClick = [this] { setPatternBrowserOpen(!patternBrowserOpen); };
    savePresetButton.onClick = [this]
    {
        const auto id = audioProcessor.getCurrentPresetId();
        for (auto& preset : presets)
            if (preset.id == id && !preset.factory)
            {
                preset.stateXml = captureStateXml();
                if (writePreset(preset))
                {
                    loadedPresetSnapshot = normalisedStateXml(preset.stateXml);
                    updatePresetDisplay();
                    showPresetStatus("Saved " + preset.name);
                }
                return;
            }
        setBrowserOpen(true);
        juce::String suggestedName = "New Preset";
        for (const auto& preset : presets)
            if (preset.id == id) suggestedName = preset.name + " Copy";
        presetNameEditor.setText(suggestedName);
        showPresetStatus("Enter a name, then choose Save New.");
    };
    for (auto* button : { &previousPresetButton, &nextPresetButton,
                          &browsePresetsButton, &browsePatternsButton, &savePresetButton })
        addAndMakeVisible(button);

    latchButton.setButtonText("Latch (keep playing)");
    latchButton.setTooltip("Off: play while a key is held. On: click a key to keep its pattern playing while you edit; click it again to stop.");
    latchButton.setToggleState(audioProcessor.isLatchEnabled(), juce::dontSendNotification);
    latchButton.onClick = [this] { audioProcessor.setLatchEnabled(latchButton.getToggleState()); };
    addAndMakeVisible(latchButton);
    previewSoundButton.setButtonText("Preview sound");
    previewSoundButton.setTooltip("Turn off to silence the built-in preview while generated MIDI continues to your instrument.");
    previewSoundButton.setToggleState(audioProcessor.isPreviewSoundEnabled(), juce::dontSendNotification);
    previewSoundButton.onClick = [this] { audioProcessor.setPreviewSoundEnabled(previewSoundButton.getToggleState()); };
    addAndMakeVisible(previewSoundButton);

    stopKeyButton.setButtonText("Stop Key");
    stopKeyButton.onClick = [this] { audioProcessor.requestStopKey(audioProcessor.getSelectedKey()); };
    addAndMakeVisible(stopKeyButton);

    stopAllButton.setButtonText("Stop All");
    stopAllButton.onClick = [this] { audioProcessor.requestStopAll(); };
    addAndMakeVisible(stopAllButton);

    // ----- Global: Meter Numerator --------------------------------------------
    meterNumeratorLabel.setText("Meter Num", juce::dontSendNotification);
    addAndMakeVisible(meterNumeratorLabel);

    addAndMakeVisible(meterNumeratorSlider);
    meterNumeratorSlider.setRange(1, 16, 1);
    meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get());
    meterNumeratorSlider.onValueChange = [this]
    {
        audioProcessor.getMeterNumerator()->setValueNotifyingHost(
            audioProcessor.getMeterNumerator()->convertTo0to1(
                static_cast<int>(meterNumeratorSlider.getValue())));
    };

    // ----- Global: Meter Denominator ------------------------------------------
    meterDenominatorLabel.setText("Global Steps/QN", juce::dontSendNotification);
    addAndMakeVisible(meterDenominatorLabel);

    addAndMakeVisible(meterDenominatorSlider);
    meterDenominatorSlider.setRange(1, 16, 1);
    meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get());
    meterDenominatorSlider.onValueChange = [this]
    {
        audioProcessor.getMeterDenominator()->setValueNotifyingHost(
            audioProcessor.getMeterDenominator()->convertTo0to1(
                static_cast<int>(meterDenominatorSlider.getValue())));
    };

    // ----- Per-key: Channel ---------------------------------------------------
    channelLabel.setText("Channel", juce::dontSendNotification);
    addAndMakeVisible(channelLabel);

    addAndMakeVisible(channelSlider);
    channelSlider.setRange(1, 16, 1);
    channelSlider.onValueChange = [this]
    {
        recordKeyEdit();
        audioProcessor.setChannelForKey(audioProcessor.getSelectedKey(),
                                        static_cast<int>(channelSlider.getValue()));
    };

    // ----- Per-key: Octave ----------------------------------------------------
    octaveLabel.setText("Octave", juce::dontSendNotification);
    addAndMakeVisible(octaveLabel);

    addAndMakeVisible(octaveSlider);
    octaveSlider.setRange(0, 10, 1);
    octaveSlider.onValueChange = [this]
    {
        recordKeyEdit();
        audioProcessor.setOctaveForKey(audioProcessor.getSelectedKey(),
                                       static_cast<int>(octaveSlider.getValue()));
    };

    // ----- Per-key: Gate ------------------------------------------------------
    gateLabel.setText("Gate", juce::dontSendNotification);
    addAndMakeVisible(gateLabel);

    addAndMakeVisible(gateSlider);
    gateSlider.setRange(0.0, 2.0, 0.01);
    gateSlider.onValueChange = [this]
    {
        recordKeyEdit();
        audioProcessor.setGateForKey(audioProcessor.getSelectedKey(),
                                     static_cast<float>(gateSlider.getValue()));
    };

    fixedLengthStepsLabel.setText("Fixed Steps", juce::dontSendNotification);
    fixedLengthStepsLabel.setTooltip("Adds this many steps at the key's effective Steps/QN to the gated note length.");
    addAndMakeVisible(fixedLengthStepsLabel);
    addAndMakeVisible(fixedLengthStepsSlider);
    fixedLengthStepsSlider.setRange(0.0, 16.0, 0.01);
    fixedLengthStepsSlider.setTooltip("0 to 16 steps added to Gate; each step uses the key's effective Steps/QN and tempo.");
    fixedLengthStepsSlider.onValueChange = [this]
    {
        recordKeyEdit();
        audioProcessor.setFixedLengthStepsForKey(audioProcessor.getSelectedKey(),
            static_cast<float>(fixedLengthStepsSlider.getValue()));
    };

    // ----- Per-key: Pattern ---------------------------------------------------
    patternLabel.setText("Pattern", juce::dontSendNotification);
    addAndMakeVisible(patternLabel);

    addAndMakeVisible(patternTextEditor);
    patternTextEditor.setInputRestrictions(45056);
    patternTextEditor.setTooltip("Melodic: signed 32-bit integers. Rhythmic: decimal masks up to 112 bits. Up to 4096 steps.");
    patternTextEditor.onTextChange = [this]
    {
        auto assignment = audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey());
        if (!assignment.setSequenceFromString(patternTextEditor.getText().toStdString()))
        {
            patternTextEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::orangered);
            patternTextEditor.setTooltip("Not applied: enter up to 4096 valid "
                + juce::String(assignment.mode == KeyAssignment::Mode::rhythmic
                    ? "decimal masks (0 to 2^112-1)." : "signed 32-bit integers."));
            return;
        }
        patternTextEditor.removeColour(juce::TextEditor::outlineColourId);
        recordKeyEdit();
        audioProcessor.setAssignmentForKey(audioProcessor.getSelectedKey(), assignment);
        patternLabel.setText("Pattern (" + juce::String(assignment.sequence.size()) + ")", juce::dontSendNotification);
        updatePatternPreview();
    };

    auto changeAssignment = [this](auto change)
    {
        const int key = audioProcessor.getSelectedKey();
        recordKeyEdit();
        auto assignment = audioProcessor.getAssignmentForKey(key);
        change(assignment);
        audioProcessor.setAssignmentForKey(key, assignment);
        updatePatternPreview();
    };
    auto addLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        addAndMakeVisible(label);
    };
    addLabel(patternNameLabel, "Name");
    addAndMakeVisible(patternNameEditor);
    patternNameEditor.setInputRestrictions(80);
    patternNameEditor.onTextChange = [changeAssignment, this] ( )
    {
        changeAssignment([this](KeyAssignment& a) { a.name = patternNameEditor.getText().toStdString(); });
    };
    addLabel(modeLabel, "Mode");
    addAndMakeVisible(modeSelector);
    modeSelector.addItem("Melodic", 1);
    modeSelector.addItem("Rhythmic (bits to MIDI notes)", 2);
    modeSelector.onChange = [changeAssignment, this]
    {
        changeAssignment([this](KeyAssignment& a) {
            a.mode = modeSelector.getSelectedId() == 2
                ? KeyAssignment::Mode::rhythmic : KeyAssignment::Mode::melodic;
        });
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    };
    addLabel(subdivisionLabel, "Steps/QN");
    addAndMakeVisible(subdivisionSlider);
    subdivisionSlider.setRange(0, 16, 1);
    subdivisionSlider.setTooltip("0 inherits Global Steps/QN. Other values override timing for this key.");
    subdivisionSlider.onValueChange = [changeAssignment, this]
    {
        changeAssignment([this](KeyAssignment& a) { a.subdivision = static_cast<int>(subdivisionSlider.getValue()); });
        updateTimingDisplay();
    };
    addLabel(velocityLabel, "Velocity");
    addAndMakeVisible(velocitySlider);
    velocitySlider.setRange(1, 127, 1);
    velocitySlider.onValueChange = [changeAssignment, this]
    {
        changeAssignment([this](KeyAssignment& a) { a.velocity = static_cast<int>(velocitySlider.getValue()); });
    };
    addLabel(transposeLabel, "Transpose");
    addAndMakeVisible(transposeSlider);
    transposeSlider.setRange(-127, 127, 1);
    transposeSlider.setTooltip("Melodic: move through the selected Forte set by this many notes. Rhythmic: shift drum pitches by semitones.");
    transposeSlider.onValueChange = [changeAssignment, this]
    {
        changeAssignment([this](KeyAssignment& a) { a.transpose = static_cast<int>(transposeSlider.getValue()); });
    };
    addLabel(rotationLabel, "Rotate");
    addAndMakeVisible(rotationSlider);
    rotationSlider.setRange(-4096, 4096, 1);
    rotationSlider.onValueChange = [changeAssignment, this]
    {
        changeAssignment([this](KeyAssignment& a) { a.rotation = static_cast<int>(rotationSlider.getValue()); });
    };
    reverseButton.setButtonText("Reverse sequence");
    addAndMakeVisible(reverseButton);
    reverseButton.onClick = [changeAssignment, this]
    {
        changeAssignment([this](KeyAssignment& a) { a.reverse = reverseButton.getToggleState(); });
    };
    addLabel(drumVelocityBitsLabel, "Velocity bits/lane");
    addAndMakeVisible(drumVelocityBitsSlider);
    drumVelocityBitsSlider.setRange(1, 7, 1);
    drumVelocityBitsSlider.setTooltip("One shared width (1-7 bits) for every drum pitch. Lane 1 uses the least significant bits; zero means rest.");
    drumVelocityBitsSlider.onValueChange = [this, changeAssignment]
    {
        changeAssignment([this](KeyAssignment& a) {
            a.drumVelocityBits = static_cast<int>(drumVelocityBitsSlider.getValue());
        });
        updatePatternPreview();
    };
    modeSelector.setTooltip("Rhythmic uses Drum notes without a Forte set. Set Channel to match your receiving instrument.");
    channelSlider.setTooltip("MIDI output channel: must match the receiving instrument. Changing mode keeps this channel.");
    addLabel(drumNotesLabel, "Drum notes");
    addAndMakeVisible(drumNotesEditor);
    drumNotesEditor.setInputRestrictions(64);
    drumNotesEditor.setTooltip("1 to 16 MIDI pitches. The number of pitches sets the lane count.");
    drumNotesEditor.onTextChange = [changeAssignment, this]
    {
        std::vector<int> notes;
        if (!validateIntegerInput(drumNotesEditor, notes, 0, 127)
            || notes.empty() || notes.size() > 16)
        {
            drumNotesEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::orangered);
            drumNotesEditor.setTooltip("Not applied: enter 1 to 16 MIDI pitches from 0 to 127.");
            return;
        }
        drumNotesEditor.setTooltip("1 to 16 MIDI pitches. The number of pitches sets the lane count.");
        changeAssignment([&notes](KeyAssignment& a) {
            a.drumLaneCount = static_cast<int>(notes.size());
            for (size_t i = 0; i < notes.size(); ++i)
                a.drumNotes[i] = juce::jlimit(0, 127, notes[i]);
        });
        updatePatternPreview();
    };
    addLabel(patternColourLabel, "Colour");
    addAndMakeVisible(patternColourSelector);
    const juce::StringArray colours { "Mint", "Sky", "Violet", "Amber", "Coral" };
    for (int i = 0; i < colours.size(); ++i) patternColourSelector.addItem(colours[i], i + 1);
    patternColourSelector.onChange = [this, changeAssignment]
    {
        static const char* palette[] { "#62D6C6", "#77B9FF", "#B99BFF", "#F4BD68", "#EF8B83" };
        const int index = patternColourSelector.getSelectedId() - 1;
        if (index >= 0 && index < 5)
            changeAssignment([index](KeyAssignment& a) { a.colour = palette[index]; });
        repaint();
    };
    addLabel(patternTagsLabel, "Tags");
    addAndMakeVisible(patternTagsEditor);
    patternTagsEditor.setInputRestrictions(200);
    patternTagsEditor.setTextToShowWhenEmpty("e.g. pulse, intro, drums", juce::Colours::grey);
    patternTagsEditor.onFocusLost = [this, changeAssignment]
    {
        changeAssignment([this](KeyAssignment& a) { a.tags = patternTagsEditor.getText().toStdString(); });
    };
    patternFavouriteButton.setButtonText("Favourite");
    patternFavouriteButton.onClick = [this, changeAssignment]
    {
        changeAssignment([this](KeyAssignment& a) { a.favourite = patternFavouriteButton.getToggleState(); });
    };
    addAndMakeVisible(patternFavouriteButton);
    patternPreviewLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(patternPreviewLabel);
    for (auto* button : { &copyPatternButton, &pastePatternButton, &duplicatePatternButton,
                          &undoButton, &redoButton,
                          &savePatternButton, &loadPatternButton, &applyRangeButton,
                          &cutPatternButton, &clearPatternButton, &independentButton })
        addAndMakeVisible(button);
    copyPatternButton.setButtonText("Copy");
    pastePatternButton.setButtonText("Paste");
    cutPatternButton.setButtonText("Cut");
    clearPatternButton.setButtonText("Clear");
    independentButton.setButtonText("Make independent");
    addAndMakeVisible(pasteScopeSelector);
    pasteScopeSelector.addItem("All settings", 1);
    pasteScopeSelector.addItem("Sequence", 2);
    pasteScopeSelector.addItem("Timing", 3);
    pasteScopeSelector.addItem("Expression", 4);
    pasteScopeSelector.setSelectedId(1);
    duplicatePatternButton.setButtonText("Duplicate to next key");
    undoButton.setButtonText("Undo");
    redoButton.setButtonText("Redo");
    undoButton.setEnabled(false);
    redoButton.setEnabled(false);
    savePatternButton.setButtonText("Save pattern");
    loadPatternButton.setButtonText("Load pattern");
    applyRangeButton.setButtonText("Assign range");
    copyPatternButton.onClick = [this] {
        copiedPattern = std::make_unique<KeyAssignment>(
            audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey()));
        pastePatternButton.setEnabled(true);
    };
    pastePatternButton.onClick = [this] {
        if (copiedPattern == nullptr) return;
        recordKeyEdit();
        auto a = audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey());
        switch (pasteScopeSelector.getSelectedId())
        {
            case 2: a.sequence = copiedPattern->sequence; break;
            case 3: a.subdivision = copiedPattern->subdivision;
                    a.gate = copiedPattern->gate;
                    a.fixedLengthSteps = copiedPattern->fixedLengthSteps; break;
            case 4: a.velocity = copiedPattern->velocity;
                    a.transpose = copiedPattern->transpose;
                    a.rotation = copiedPattern->rotation;
                    a.reverse = copiedPattern->reverse;
                    a.drumVelocityBits = copiedPattern->drumVelocityBits; break;
            default: a = *copiedPattern; a.linkId.clear(); break;
        }
        audioProcessor.setAssignmentForKey(audioProcessor.getSelectedKey(), a);
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    };
    cutPatternButton.onClick = [this]
    {
        copiedPattern = std::make_unique<KeyAssignment>(audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey()));
        pastePatternButton.setEnabled(true);
        recordKeyEdit();
        KeyAssignment blank;
        blank.sequence.clear();
        audioProcessor.setAssignmentForKey(audioProcessor.getSelectedKey(), blank);
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    };
    clearPatternButton.onClick = [this]
    {
        recordKeyEdit();
        KeyAssignment blank;
        blank.sequence.clear();
        audioProcessor.setAssignmentForKey(audioProcessor.getSelectedKey(), blank);
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    };
    independentButton.onClick = [this] { makeSelectedIndependent(); };
    duplicatePatternButton.onClick = [this] {
        const int key = audioProcessor.getSelectedKey();
        if (key == 127) return;
        syncEditHistory();
        editHistory.record({ key + 1, { { key + 1, audioProcessor.getAssignmentForKey(key + 1) } } });
        auto copy = audioProcessor.getAssignmentForKey(key);
        copy.linkId.clear();
        audioProcessor.setAssignmentForKey(key + 1, copy);
        audioProcessor.setSelectedKey(key + 1);
        loadAssignmentForKey(key + 1);
    };
    savePatternButton.onClick = [this] { savePatternToBank(); };
    loadPatternButton.onClick = [this] { loadPatternFromBank(); };
    applyRangeButton.onClick = [this] { applyRange(); };
    undoButton.onClick = [this] { undoKeyEdit(); };
    redoButton.onClick = [this] { redoKeyEdit(); };
    addLabel(rangeLabel, "MIDI range");
    for (auto* slider : { &rangeFirstSlider, &rangeLastSlider }) {
        addAndMakeVisible(*slider);
        slider->setRange(0, 127, 1);
    }
    rangeFirstSlider.setValue(48);
    rangeLastSlider.setValue(72);
    transposeRangeButton.setButtonText("Transpose by key");
    transposeRangeButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(transposeRangeButton);
    // ----- Per-key: searchable Forte set --------------------------------------
    forteSearchLabel.setText("Find Set", juce::dontSendNotification);
    addAndMakeVisible(forteSearchLabel);
    forteSearchEditor.setInputRestrictions(80);
    forteSearchEditor.setTextToShowWhenEmpty("Forte ID, common name, or pitches", juce::Colours::grey);
    forteSearchEditor.onTextChange = [this] { updateForteSearchResults(); };
    forteSearchEditor.onReturnKey = [this]
    {
        if (visibleForteIds.size() > 1)
            forteNumberSelector.setSelectedId(2, juce::sendNotificationSync);
    };
    addAndMakeVisible(forteSearchEditor);

    forteLabel.setText("Forte Set", juce::dontSendNotification);
    addAndMakeVisible(forteLabel);

    addAndMakeVisible(forteNumberSelector);
    {
        auto forteNumbers = Pcs12::getSortedForteNumbers();
        forteSearchEntries.reserve(forteNumbers.size());
        for (const auto& fn : forteNumbers)
        {
            const auto name = Pcs12::getCommonNameForForte(fn);
            juce::String pitches;
            for (int pitch : Pcs12::parseForte(fn).asSequence())
            {
                if (pitches.isNotEmpty()) pitches << ' ';
                pitches << pitch;
            }
            const juce::String id(fn);
            const juce::String display = id
                + (name.empty() ? juce::String() : " - " + juce::String(name))
                + " [" + pitches + "]";
            forteSearchEntries.push_back({ id, display });
        }
    }
    forteNumberSelector.onChange = [this]
    {
        const int index = forteNumberSelector.getSelectedId() - 1;
        if (index >= 0 && index < static_cast<int>(visibleForteIds.size()))
        {
            recordKeyEdit();
            audioProcessor.setForteForKey(audioProcessor.getSelectedKey(),
                                          visibleForteIds[static_cast<size_t>(index)].toStdString());
            updateSelectedForteLabel();
            updatePatternPreview();
        }
    };

    forteSelectionLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(forteSelectionLabel);

    // ----- Selected key label -------------------------------------------------
    selectedKeyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(selectedKeyLabel);
    activeKeysLabel.setJustificationType(juce::Justification::centred);
    activeKeysLabel.setColour(juce::Label::textColourId, juce::Colour(0xff62d6c6));
    activeKeysLabel.setText("No active keys", juce::dontSendNotification);
    addAndMakeVisible(activeKeysLabel);

    // ----- Preset browser -----------------------------------------------------
    auto addBrowserControl = [this](juce::Component& component)
    {
        addAndMakeVisible(component);
    };
    for (auto* label : { &presetSearchLabel, &presetCategoryFilterLabel,
                         &presetNameFieldLabel, &presetCategoryFieldLabel,
                         &presetTagsFieldLabel, &presetDescriptionFieldLabel })
        addBrowserControl(*label);
    presetSearchLabel.setText("Search", juce::dontSendNotification);
    presetCategoryFilterLabel.setText("Category", juce::dontSendNotification);
    presetNameFieldLabel.setText("Name", juce::dontSendNotification);
    presetCategoryFieldLabel.setText("Category", juce::dontSendNotification);
    presetTagsFieldLabel.setText("Tags", juce::dontSendNotification);
    presetDescriptionFieldLabel.setText("Description", juce::dontSendNotification);

    for (auto* editor : { &presetSearchEditor, &presetNameEditor,
                          &presetCategoryEditor, &presetTagsEditor,
                          &presetDescriptionEditor })
        addBrowserControl(*editor);
    presetSearchEditor.setTextToShowWhenEmpty("Name, tag, or description", juce::Colours::grey);
    presetSearchEditor.onTextChange = [this] { refreshPresetFilters(); };
    presetDescriptionEditor.setMultiLine(true);
    presetDescriptionEditor.setReturnKeyStartsNewLine(true);
    presetSearchEditor.setInputRestrictions(80);
    presetNameEditor.setInputRestrictions(80);
    presetCategoryEditor.setInputRestrictions(40);
    presetTagsEditor.setInputRestrictions(200);
    presetDescriptionEditor.setInputRestrictions(500);

    addBrowserControl(presetCategoryFilter);
    presetCategoryFilter.onChange = [this] { refreshPresetFilters(); };
    favouritesOnlyButton.setButtonText("Favourites only");
    favouritesOnlyButton.onClick = [this] { refreshPresetFilters(); };
    addBrowserControl(favouritesOnlyButton);

    presetList.setModel(this);
    presetList.setRowHeight(28);
    addBrowserControl(presetList);
    presetDetailsLabel.setJustificationType(juce::Justification::centredLeft);
    presetStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addBrowserControl(presetDetailsLabel);
    addBrowserControl(presetStatusLabel);

    loadPresetButton.setButtonText("Load");
    saveNewPresetButton.setButtonText("Save New");
    updatePresetButton.setButtonText("Update");
    duplicatePresetButton.setButtonText("Duplicate");
    deletePresetButton.setButtonText("Delete");
    favouritePresetButton.setButtonText("Favourite");
    importPresetButton.setButtonText("Import");
    exportPresetButton.setButtonText("Export");
    loadPresetButton.onClick = [this] { loadPreset(selectedPresetIndex); };
    saveNewPresetButton.onClick = [this] { saveNewPreset(); };
    updatePresetButton.onClick = [this] { updateSelectedPreset(); };
    duplicatePresetButton.onClick = [this] { duplicateSelectedPreset(); };
    deletePresetButton.onClick = [this] { deleteSelectedPreset(); };
    favouritePresetButton.onClick = [this] { toggleSelectedFavourite(); };
    importPresetButton.onClick = [this] { importPreset(); };
    exportPresetButton.onClick = [this] { exportSelectedPreset(); };
    for (auto* button : { &loadPresetButton, &saveNewPresetButton, &updatePresetButton,
                          &duplicatePresetButton, &deletePresetButton, &favouritePresetButton,
                          &importPresetButton, &exportPresetButton })
        addBrowserControl(*button);

    patternSearchLabel.setText("Find pattern", juce::dontSendNotification);
    patternSearchEditor.setInputRestrictions(80);
    patternSearchEditor.setTextToShowWhenEmpty("Search names and tags", juce::Colours::grey);
    patternSearchEditor.onTextChange = [this] { filterPatternLibrary(); };
    bankSourceSelector.addItem("My patterns", 1);
    for (int i = 0; i < static_cast<int>(std::size(builtInBanks)); ++i)
        bankSourceSelector.addItem(builtInBanks[i].label, i + 2);
    bankSourceSelector.setSelectedId(2, juce::dontSendNotification);
    bankSourceSelector.onChange = [this] { loadPatternLibrary(); };
    addAndMakeVisible(bankSourceSelector);
    bankFavouritesButton.setButtonText("Favourites only");
    bankFavouritesButton.onClick = [this] { filterPatternLibrary(); };
    bankFavouriteButton.setButtonText("Favourite this pattern");
    bankNameEditor.setInputRestrictions(80);
    bankTagsEditor.setInputRestrictions(200);
    bankTagsLabel.setText("Tags", juce::dontSendNotification);
    bankEmptyLabel.setText("No patterns match this search.", juce::dontSendNotification);
    bankEmptyLabel.setJustificationType(juce::Justification::centred);
    bankHelpLabel.setText("Assign as a copy or link. Built-in patterns are read-only.", juce::dontSendNotification);
    for (const auto& component : { &patternSearchLabel, &bankDetailsLabel, &bankTagsLabel,
                                   &bankEmptyLabel, &bankHelpLabel, &bankStatusLabel })
        addAndMakeVisible(*component);
    for (auto* component : std::initializer_list<juce::Component*> { &patternSearchEditor,
                             &bankNameEditor, &bankTagsEditor, &bankColourSelector,
                             &bankFavouritesButton, &bankFavouriteButton,
                             &assignCopyButton, &assignLinkButton,
                             &auditionButton, &bankSaveButton,
                             &saveCurrentToBankButton, &importBankButton, &exportBankButton }) addAndMakeVisible(*component);
    for (int i = 0; i < colours.size(); ++i) bankColourSelector.addItem(colours[i], i + 1);
    assignCopyButton.setButtonText("Assign copy");
    assignLinkButton.setButtonText("Assign link");
    auditionButton.setButtonText("Audition");
    bankSaveButton.setButtonText("Save details");
    saveCurrentToBankButton.setButtonText("Save current pattern to bank");
    saveCurrentToBankButton.onClick = [this] { savePatternToBank(); };
    importBankButton.setButtonText("Import bank");
    exportBankButton.setButtonText("Export bank");
    importBankButton.onClick = [this] { importPatternBank(); };
    exportBankButton.onClick = [this] { exportPatternBank(); };
    assignCopyButton.onClick = [this] { assignSelectedPattern(false); };
    assignLinkButton.onClick = [this] { assignSelectedPattern(true); };
    auditionButton.onClick = [this]
    {
        if (auditioningKey >= 0)
        {
            stopPatternAudition();
        }
        else
        {
            if (selectedPatternIndex < 0 || selectedPatternIndex >= static_cast<int>(patternLibrary.size())) return;
            const int key = audioProcessor.getSelectedKey();
            audioProcessor.auditionPattern(key, patternLibrary[static_cast<size_t>(selectedPatternIndex)].assignment);
            auditioningKey = key;
        }
        auditionButton.setButtonText(auditioningKey >= 0 ? "Stop audition" : "Audition");
    };
    bankSaveButton.onClick = [this] { saveSelectedPatternMetadata(); };

    // ----- Initial load -------------------------------------------------------
    setSize(980, 830);
    loadPresetLibrary();
    loadAssignmentForKey(audioProcessor.getSelectedKey());
    lastStateRestoreRevision = audioProcessor.getStateRestoreRevision();
    setBrowserOpen(false);
    startTimerHz(2);
}

NSeqArpKeysAudioProcessorEditor::~NSeqArpKeysAudioProcessorEditor()
{
    stopPatternAudition();
    setLookAndFeel(nullptr);
    presetList.setModel(nullptr);
    keyboardComponent.removeMouseListener(this);
    keyboardState.removeListener(this);
}

void NSeqArpKeysAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isPopupMenu() || patternLibrary.empty()) return;
    auto* target = event.eventComponent;
    if (target != &keyboardComponent && !keyboardComponent.isParentOf(target)) return;
    juce::PopupMenu menu;
    menu.addItem(1, "Assign selected bank pattern as copy", selectedPatternIndex >= 0);
    menu.addItem(2, "Assign selected bank pattern as link", selectedPatternIndex >= 0);
    menu.addItem(3, "Make this key independent",
        !audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey()).linkId.empty());
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&keyboardComponent),
        [safeThis](int result)
        {
            if (safeThis == nullptr) return;
            if (result == 1) safeThis->assignSelectedPattern(false);
            else if (result == 2) safeThis->assignSelectedPattern(true);
            else if (result == 3) safeThis->makeSelectedIndependent();
        });
}

//==============================================================================
void NSeqArpKeysAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff101c29), 0.0f, 0.0f,
        juce::Colour(0xff233748), static_cast<float>(getWidth()), static_cast<float>(getHeight()), false));
    g.fillAll();
    g.setColour(juce::Colour(0xff263c4c));
    g.fillRoundedRectangle(getLocalBounds().reduced(7).toFloat(), 12.0f);
    g.setColour(juce::Colour(0xff62d6c6).withAlpha(0.9f));
    g.fillRoundedRectangle(12.0f, 12.0f, 5.0f, 28.0f, 2.0f);
    if (!browserOpen && !patternBrowserOpen)
    {
        g.setColour(juce::Colour(0xff172a39));
        g.fillRoundedRectangle(14.0f, 183.0f, static_cast<float>(getWidth() - 28),
            static_cast<float>(getHeight() - 198), 10.0f);
    }
}

void NSeqArpKeysAudioProcessorEditor::resized()
{
    auto area  = getLocalBounds().reduced(8);
    int  w     = area.getWidth();
    int  lblW  = 110;
    int  rowH  = 32;

    auto presetRow = area.removeFromTop(32);
    presetNameLabel.setBounds(presetRow.removeFromLeft(300));
    previousPresetButton.setBounds(presetRow.removeFromLeft(34));
    presetRow.removeFromLeft(4);
    nextPresetButton.setBounds(presetRow.removeFromLeft(34));
    presetRow.removeFromLeft(8);
    browsePresetsButton.setBounds(presetRow.removeFromLeft(135));
    presetRow.removeFromLeft(8);
    browsePatternsButton.setBounds(presetRow.removeFromLeft(125));
    presetRow.removeFromLeft(8);
    savePresetButton.setBounds(presetRow.removeFromLeft(90));
    area.removeFromTop(6);

    if (patternBrowserOpen)
    {
        bankStatusLabel.setBounds(area.removeFromBottom(24));
        auto searchRow = area.removeFromTop(32);
        patternSearchLabel.setBounds(searchRow.removeFromLeft(100));
        patternSearchEditor.setBounds(searchRow.removeFromLeft(350));
        searchRow.removeFromLeft(12);
        bankFavouritesButton.setBounds(searchRow.removeFromLeft(180));
        searchRow.removeFromLeft(12);
        bankSourceSelector.setBounds(searchRow.removeFromLeft(240));
        area.removeFromTop(12);
        auto left = area.removeFromLeft(400);
        area.removeFromLeft(18);
        presetList.setBounds(left);
        bankEmptyLabel.setBounds(left.reduced(20));
        bankDetailsLabel.setBounds(area.removeFromTop(36));
        bankHelpLabel.setBounds(area.removeFromTop(28));
        area.removeFromTop(10);
        bankNameEditor.setBounds(area.removeFromTop(32));
        area.removeFromTop(8);
        auto tagsRow = area.removeFromTop(32);
        bankTagsLabel.setBounds(tagsRow.removeFromLeft(70));
        bankTagsEditor.setBounds(tagsRow);
        area.removeFromTop(8);
        bankColourSelector.setBounds(area.removeFromTop(32));
        bankFavouriteButton.setBounds(area.removeFromTop(28));
        area.removeFromTop(16);
        auto assignRow = area.removeFromTop(36);
        assignCopyButton.setBounds(assignRow.removeFromLeft(125));
        assignRow.removeFromLeft(8);
        assignLinkButton.setBounds(assignRow.removeFromLeft(125));
        area.removeFromTop(8);
        auto actionRow = area.removeFromTop(36);
        auditionButton.setBounds(actionRow.removeFromLeft(125));
        actionRow.removeFromLeft(8);
        bankSaveButton.setBounds(actionRow.removeFromLeft(125));
        area.removeFromTop(20);
        auto transferRow = area.removeFromTop(36);
        saveCurrentToBankButton.setBounds(transferRow.removeFromLeft(240));
        transferRow.removeFromLeft(8);
        importBankButton.setBounds(transferRow.removeFromLeft(115));
        transferRow.removeFromLeft(8);
        exportBankButton.setBounds(transferRow.removeFromLeft(115));
        return;
    }
    if (browserOpen)
    {
        auto filterRow = area.removeFromTop(28);
        presetSearchLabel.setBounds(filterRow.removeFromLeft(58));
        presetSearchEditor.setBounds(filterRow.removeFromLeft(235));
        filterRow.removeFromLeft(8);
        presetCategoryFilterLabel.setBounds(filterRow.removeFromLeft(72));
        presetCategoryFilter.setBounds(filterRow.removeFromLeft(175));
        filterRow.removeFromLeft(8);
        favouritesOnlyButton.setBounds(filterRow);
        area.removeFromTop(6);
        presetStatusLabel.setBounds(area.removeFromBottom(24));

        auto left = area.removeFromLeft(315);
        area.removeFromLeft(10);
        presetList.setBounds(left);
        presetDetailsLabel.setBounds(area.removeFromTop(24));
        auto field = [&](juce::Label& label, juce::Component& editor)
        {
            auto row = area.removeFromTop(28);
            label.setBounds(row.removeFromLeft(85));
            editor.setBounds(row);
            area.removeFromTop(3);
        };
        field(presetNameFieldLabel, presetNameEditor);
        field(presetCategoryFieldLabel, presetCategoryEditor);
        field(presetTagsFieldLabel, presetTagsEditor);
        presetDescriptionFieldLabel.setBounds(area.removeFromTop(22));
        presetDescriptionEditor.setBounds(area.removeFromTop(72));
        area.removeFromTop(8);
        auto actionRow = [&](juce::TextButton& a, juce::TextButton& b,
                             juce::TextButton& c, juce::TextButton& d)
        {
            auto row = area.removeFromTop(30);
            const int buttonWidth = (row.getWidth() - 18) / 4;
            a.setBounds(row.removeFromLeft(buttonWidth)); row.removeFromLeft(6);
            b.setBounds(row.removeFromLeft(buttonWidth)); row.removeFromLeft(6);
            c.setBounds(row.removeFromLeft(buttonWidth)); row.removeFromLeft(6);
            d.setBounds(row);
            area.removeFromTop(5);
        };
        actionRow(loadPresetButton, saveNewPresetButton,
                  updatePresetButton, duplicatePresetButton);
        actionRow(deletePresetButton, favouritePresetButton,
                  importPresetButton, exportPresetButton);
        return;
    }

    keyboardComponent.setBounds(area.removeFromTop(88));
    area.removeFromTop(6);

    // Selected key display
    selectedKeyLabel.setBounds(area.removeFromTop(20));
    activeKeysLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(6);

    auto transportRow = area.removeFromTop(28);
    latchButton.setBounds(transportRow.removeFromLeft(210));
    transportRow.removeFromLeft(8);
    stopKeyButton.setBounds(transportRow.removeFromLeft(110));
    transportRow.removeFromLeft(8);
    stopAllButton.setBounds(transportRow.removeFromLeft(110));
    transportRow.removeFromLeft(12);
    previewSoundButton.setBounds(transportRow.removeFromLeft(170));
    area.removeFromTop(6);

    // Two-column layout for global params
    auto globalRow = area.removeFromTop(rowH);
    meterNumeratorLabel .setBounds(globalRow.removeFromLeft(lblW));
    meterNumeratorSlider.setBounds(globalRow.removeFromLeft((w - 2 * lblW) / 2));
    meterDenominatorLabel .setBounds(globalRow.removeFromLeft(lblW));
    meterDenominatorSlider.setBounds(globalRow);

    area.removeFromTop(8);

    // Per-key controls (one per row, label + control)
    auto makeRow = [&](juce::Label& lbl, juce::Component& ctrl)
    {
        auto row = area.removeFromTop(rowH);
        lbl.setBounds(row.removeFromLeft(lblW));
        ctrl.setBounds(row);
        area.removeFromTop(5);
    };

    makeRow(channelLabel, channelSlider);
    const bool rhythmic = modeSelector.getSelectedId() == 2;
    if (!rhythmic) makeRow(octaveLabel, octaveSlider);
    makeRow(gateLabel,    gateSlider);
    makeRow(fixedLengthStepsLabel, fixedLengthStepsSlider);
    makeRow(patternLabel, patternTextEditor);
    patternPreviewLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(3);
    makeRow(patternNameLabel, patternNameEditor);
    auto metadataRow = area.removeFromTop(rowH);
    patternColourLabel.setBounds(metadataRow.removeFromLeft(lblW));
    patternColourSelector.setBounds(metadataRow.removeFromLeft(150));
    metadataRow.removeFromLeft(10);
    patternTagsLabel.setBounds(metadataRow.removeFromLeft(45));
    patternTagsEditor.setBounds(metadataRow.removeFromLeft(360));
    metadataRow.removeFromLeft(10);
    patternFavouriteButton.setBounds(metadataRow);
    area.removeFromTop(2);
    auto modeRow = area.removeFromTop(rowH);
    modeLabel.setBounds(modeRow.removeFromLeft(lblW));
    modeSelector.setBounds(modeRow.removeFromLeft(260));
    subdivisionLabel.setBounds(modeRow.removeFromLeft(lblW));
    subdivisionSlider.setBounds(modeRow);
    area.removeFromTop(2);
    auto expressionRow = area.removeFromTop(rowH);
    velocityLabel.setBounds(expressionRow.removeFromLeft(lblW));
    velocitySlider.setBounds(expressionRow.removeFromLeft(150));
    transposeLabel.setBounds(expressionRow.removeFromLeft(lblW));
    transposeSlider.setBounds(expressionRow.removeFromLeft(160));
    rotationLabel.setBounds(expressionRow.removeFromLeft(75));
    rotationSlider.setBounds(expressionRow);
    area.removeFromTop(2);
    if (rhythmic)
    {
        makeRow(drumNotesLabel, drumNotesEditor);
        makeRow(drumVelocityBitsLabel, drumVelocityBitsSlider);
    }
    else
    {
        makeRow(forteSearchLabel, forteSearchEditor);
        makeRow(forteLabel, forteNumberSelector);
        forteSelectionLabel.setBounds(area.removeFromTop(rowH));
    }
    reverseButton.setBounds(area.removeFromTop(rowH));
    auto copyRow = area.removeFromTop(30);
    copyPatternButton.setBounds(copyRow.removeFromLeft(78));
    cutPatternButton.setBounds(copyRow.removeFromLeft(70));
    clearPatternButton.setBounds(copyRow.removeFromLeft(70));
    pasteScopeSelector.setBounds(copyRow.removeFromLeft(145));
    pastePatternButton.setBounds(copyRow.removeFromLeft(75));
    duplicatePatternButton.setBounds(copyRow.removeFromLeft(170));
    independentButton.setBounds(copyRow.removeFromLeft(160));
    area.removeFromTop(4);
    auto fileRow = area.removeFromTop(30);
    undoButton.setBounds(fileRow.removeFromLeft(75));
    fileRow.removeFromLeft(5);
    redoButton.setBounds(fileRow.removeFromLeft(75));
    fileRow.removeFromLeft(12);
    savePatternButton.setBounds(fileRow.removeFromLeft(130));
    fileRow.removeFromLeft(5);
    loadPatternButton.setBounds(fileRow.removeFromLeft(130));
    area.removeFromTop(3);
    auto rangeRow = area.removeFromTop(rowH);
    rangeLabel.setBounds(rangeRow.removeFromLeft(lblW));
    rangeFirstSlider.setBounds(rangeRow.removeFromLeft(140));
    rangeLastSlider.setBounds(rangeRow.removeFromLeft(140));
    transposeRangeButton.setBounds(rangeRow.removeFromLeft(160));
    applyRangeButton.setBounds(rangeRow.removeFromLeft(130));
}

//==============================================================================
void NSeqArpKeysAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState*,
                                                    int midiChannel,
                                                    int midiNoteNumber,
                                                    float velocity)
{
    audioProcessor.setSelectedKey(midiNoteNumber);
    loadAssignmentForKey(midiNoteNumber);
    audioProcessor.queuePreviewMidiMessage(juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity));

}

void NSeqArpKeysAudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*,
                                                    int midiChannel,
                                                    int midiNoteNumber,
                                                    float /*velocity*/)
{
    audioProcessor.queuePreviewMidiMessage(juce::MidiMessage::noteOff(midiChannel, midiNoteNumber));
}

//==============================================================================
void NSeqArpKeysAudioProcessorEditor::loadAssignmentForKey(int key)
{
    const auto a = audioProcessor.getAssignmentForKey(key);

    // Use dontSendNotification where possible to avoid feedback loops.
    // For TextEditor, setText with false (= don't send a change message) does not
    // call onTextChange, so it is safe to call directly.
    patternTextEditor.setText(a.sequenceToString(), false);
    patternLabel.setText("Pattern (" + juce::String(a.sequence.size()) + ")", juce::dontSendNotification);
    patternNameEditor.setText(a.name, false);
    modeSelector.setSelectedId(a.mode == KeyAssignment::Mode::rhythmic ? 2 : 1, juce::dontSendNotification);
    subdivisionSlider.setValue(a.subdivision, juce::dontSendNotification);
    updateTimingDisplay();
    velocitySlider.setValue(a.velocity, juce::dontSendNotification);
    transposeSlider.setValue(a.transpose, juce::dontSendNotification);
    rotationSlider.setValue(a.rotation, juce::dontSendNotification);
    reverseButton.setToggleState(a.reverse, juce::dontSendNotification);
    auto valuesText = [](const std::vector<int>& values) {
        juce::StringArray parts;
        for (int n : values) parts.add(juce::String(n));
        return parts.joinIntoString(" ");
    };
    drumVelocityBitsSlider.setValue(a.drumVelocityBits, juce::dontSendNotification);
    drumNotesEditor.setText(valuesText(std::vector<int>(a.drumNotes.begin(),
        a.drumNotes.begin() + juce::jlimit(1, 16, a.drumLaneCount))), false);
    for (auto* editor : { &patternTextEditor, &drumNotesEditor })
        editor->removeColour(juce::TextEditor::outlineColourId);
    if (a.mode == KeyAssignment::Mode::melodic
        && std::any_of(a.sequence.begin(), a.sequence.end(),
            [](const SequenceValue& value) { return !value.fitsMelodicInt(); }))
    {
        patternTextEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::orangered);
        patternTextEditor.setTooltip("This rhythmic mask exceeds the melodic 32-bit range. Switch back to Rhythmic or edit the pattern.");
    }
    else
        patternTextEditor.setTooltip(a.mode == KeyAssignment::Mode::rhythmic
            ? "Positive decimal masks up to 112 bits; up to 4096 steps."
            : "Signed 32-bit integers; up to 4096 steps.");
    patternTagsEditor.setText(a.tags, false);
    patternFavouriteButton.setToggleState(a.favourite, juce::dontSendNotification);
    static const char* palette[] { "#62D6C6", "#77B9FF", "#B99BFF", "#F4BD68", "#EF8B83" };
    int colourIndex = 0;
    for (int i = 0; i < 5; ++i) if (a.colour == palette[i]) colourIndex = i;
    patternColourSelector.setSelectedId(colourIndex + 1, juce::dontSendNotification);
    pastePatternButton.setEnabled(copiedPattern != nullptr);
    duplicatePatternButton.setEnabled(key < 127);

    channelSlider.setValue(a.channel, juce::dontSendNotification);
    octaveSlider .setValue(a.octave,  juce::dontSendNotification);
    gateSlider   .setValue(static_cast<double>(a.gate), juce::dontSendNotification);
    fixedLengthStepsSlider.setValue(static_cast<double>(a.fixedLengthSteps), juce::dontSendNotification);

    forteSearchEditor.setText(a.forteString, false);
    updateForteSearchResults();
    updateSelectedForteLabel();
    updatePatternPreview();
    updateModeVisibility();

    juce::String name = juce::MidiMessage::getMidiNoteName(key, true, true, 4);
    selectedKeyLabel.setText("Selected key: " + name
                             + " (MIDI " + juce::String(key) + ")"
                             + (a.linkId.empty() ? " · Independent" : " · Linked"),
                             juce::dontSendNotification);
}

bool NSeqArpKeysAudioProcessorEditor::validateIntegerInput(juce::TextEditor& editor,
        std::vector<int>& values, int minimum, int maximum, int exactCount)
{
    const bool valid = KeyAssignment::parseIntegerSequence(editor.getText().toStdString(),
                                                          values, minimum, maximum)
        && (exactCount < 0 || values.size() == static_cast<size_t>(exactCount));
    if (valid) editor.removeColour(juce::TextEditor::outlineColourId);
    else editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::orangered);
    editor.setTooltip(valid ? "Space-separated integers; up to 4096 steps."
        : "Not applied: enter " + (exactCount < 0 ? juce::String("up to 4096") : juce::String(exactCount))
          + " whole numbers from " + juce::String(minimum) + " to " + juce::String(maximum) + ".");
    return valid;
}

void NSeqArpKeysAudioProcessorEditor::updateTimingDisplay()
{
    const int subdivision = audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey()).subdivision;
    const int effective = subdivision > 0 ? subdivision : audioProcessor.getMeterDenominator()->get();
    subdivisionLabel.setText("Steps/QN: " + juce::String(effective), juce::dontSendNotification);
    subdivisionLabel.setTooltip(subdivision == 0 ? "Effective rate inherited from Global Steps/QN"
                                               : "Effective rate for this key");
}

void NSeqArpKeysAudioProcessorEditor::syncEditHistory()
{
    const auto revision = audioProcessor.getStateRestoreRevision();
    if (revision != lastStateRestoreRevision)
    {
        editHistory.clear();
        lastStateRestoreRevision = revision;
    }
}

void NSeqArpKeysAudioProcessorEditor::applyRange()
{
    const int first = static_cast<int>(rangeFirstSlider.getValue());
    const int last = static_cast<int>(rangeLastSlider.getValue());
    if (first > last) return;
    syncEditHistory();
    AssignmentHistory::Edit edit { audioProcessor.getSelectedKey(), {} };
    for (int key = first; key <= last; ++key)
        edit.assignments.emplace_back(key, audioProcessor.getAssignmentForKey(key));
    editHistory.record(std::move(edit));
    audioProcessor.copyAssignmentToRange(audioProcessor.getSelectedKey(), first, last,
                                          transposeRangeButton.getToggleState());
    loadAssignmentForKey(audioProcessor.getSelectedKey());
}

void NSeqArpKeysAudioProcessorEditor::recordKeyEdit()
{
    syncEditHistory();
    const int key = audioProcessor.getSelectedKey();
    AssignmentHistory::Edit edit { key, {} };
    const auto current = audioProcessor.getAssignmentForKey(key);
    if (current.linkId.empty())
        edit.assignments.emplace_back(key, current);
    else
        for (int other = 0; other < 128; ++other)
        {
            auto a = audioProcessor.getAssignmentForKey(other);
            if (a.linkId == current.linkId) edit.assignments.emplace_back(other, std::move(a));
        }
    editHistory.record(std::move(edit));
}

void NSeqArpKeysAudioProcessorEditor::undoKeyEdit()
{
    syncEditHistory();
    const int key = editHistory.undo(
        [this](int k) { return audioProcessor.getAssignmentForKey(k); },
        [this](const auto& assignments) { audioProcessor.restoreAssignments(assignments); });
    if (key < 0) return;
    audioProcessor.setSelectedKey(key);
    loadAssignmentForKey(key);
}

void NSeqArpKeysAudioProcessorEditor::redoKeyEdit()
{
    syncEditHistory();
    const int key = editHistory.redo(
        [this](int k) { return audioProcessor.getAssignmentForKey(k); },
        [this](const auto& assignments) { audioProcessor.restoreAssignments(assignments); });
    if (key < 0) return;
    audioProcessor.setSelectedKey(key);
    loadAssignmentForKey(key);
}

void NSeqArpKeysAudioProcessorEditor::savePatternToBank()
{
    const int key = audioProcessor.getSelectedKey();
    juce::XmlElement bank("NSeqPattern");
    bank.setAttribute("version", 1);
    bank.setAttribute("id", juce::Uuid().toString());
    AssignmentState::write(*bank.createNewChildElement("Assignment"),
                           audioProcessor.getAssignmentForKey(key));
    auto name = juce::String(audioProcessor.getAssignmentForKey(key).name)
        .retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_ ")
        .trim().replaceCharacter(' ', '-');
    if (name.isEmpty()) name = "Pattern-" + juce::String(key);
    auto directory = patternDirectory();
    if (directory.createDirectory().failed()) return;
    presetChooser = std::make_unique<juce::FileChooser>("Save pattern to bank",
        directory.getChildFile(name + ".nseqpattern"), "*.nseqpattern");
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    presetChooser->launchAsync(juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, bank](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr || chooser.getResult() == juce::File()) return;
            const auto target = chooser.getResult().withFileExtension(".nseqpattern");
            auto output = bank;
            if (auto existing = parseSafeXmlFile(target, 1024 * 1024);
                existing != nullptr && existing->hasTagName("NSeqPattern"))
            {
                const auto existingId = existing->getStringAttribute("id").substring(0, 80);
                if (existingId.isNotEmpty()) output.setAttribute("id", existingId);
            }
            const bool saved = writeXmlFile(target, output);
            if (!saved) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                "Pattern bank", "Could not save pattern.");
            else
            {
                safeThis->bankSourceSelector.setSelectedId(1, juce::dontSendNotification);
                safeThis->patternSearchEditor.setText({}, false);
                safeThis->bankFavouritesButton.setToggleState(false, juce::dontSendNotification);
                safeThis->loadPatternLibrary(output.getStringAttribute("id"));
                safeThis->showBankStatus("Saved pattern to " + target.getFullPathName());
            }
        });
}

void NSeqArpKeysAudioProcessorEditor::loadPatternFromBank()
{
    auto directory = patternDirectory();
    directory.createDirectory();
    presetChooser = std::make_unique<juce::FileChooser>("Load pattern from bank",
        directory, "*.nseqpattern");
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    presetChooser->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr || chooser.getResult() == juce::File()) return;
            if (chooser.getResult().getSize() > 1024 * 1024) return;
            auto bank = parseSafeXmlFile(chooser.getResult(), 1024 * 1024);
            if (bank == nullptr || !bank->hasTagName("NSeqPattern")
                || bank->getIntAttribute("version") != 1
                || bank->getNumChildElements() != 1
                || bank->getFirstChildElement() == nullptr
                || !bank->getFirstChildElement()->hasTagName("Assignment"))
                return;
            const int key = safeThis->audioProcessor.getSelectedKey();
            safeThis->recordKeyEdit();
            auto imported = AssignmentState::read(*bank->getFirstChildElement());
            imported.linkId.clear();
            safeThis->audioProcessor.setAssignmentForKey(key, imported);
            safeThis->loadAssignmentForKey(key);
        });
}

void NSeqArpKeysAudioProcessorEditor::loadPatternLibrary(const juce::String& preferredId)
{
    const auto selectedId = preferredId.isNotEmpty() ? preferredId
        : (selectedPatternIndex >= 0 && selectedPatternIndex < static_cast<int>(patternLibrary.size())
            ? patternLibrary[static_cast<size_t>(selectedPatternIndex)].id : juce::String());
    stopPatternAudition();
    patternLibrary.clear();
    selectedPatternIndex = -1;
    const int source = bankSourceSelector.getSelectedId();
    if (source >= 2 && source - 2 < static_cast<int>(std::size(builtInBanks)))
    {
        juce::MemoryInputStream bytes(FourthAtlasData::FourthAtlas_zip,
                                      FourthAtlasData::FourthAtlas_zipSize, false);
        juce::ZipFile archive(bytes);
        const auto* zipEntry = archive.getEntry(builtInBanks[source - 2].filename);
        if (zipEntry != nullptr && zipEntry->uncompressedSize <= PatternBankFile::maxFileBytes)
        {
            std::unique_ptr<juce::InputStream> stream(archive.createStreamForEntry(*zipEntry));
            if (stream != nullptr)
            {
                auto xml = parseSafeXml(stream->readEntireStreamAsString());
                std::vector<PatternBankFile::Entry> entries;
                juce::String error;
                if (xml != nullptr && PatternBankFile::read(*xml, entries, error))
                    for (auto& entry : entries)
                        patternLibrary.push_back({ entry.id, {}, std::move(entry.assignment), true });
                else
                    showBankStatus(error.isNotEmpty() ? error : "Could not load the built-in bank.", true);
            }
        }
    }
    else
    {
        auto directory = patternDirectory();
        if (directory.createDirectory().failed()) { filterPatternLibrary(); return; }
        juce::Array<juce::File> files;
        directory.findChildFiles(files, juce::File::findFiles, false, "*.nseqpattern");
        for (const auto& file : files)
        {
            if (file.getSize() <= 0 || file.getSize() > 1024 * 1024) continue;
            auto xml = parseSafeXmlFile(file, 1024 * 1024);
            if (xml == nullptr || !xml->hasTagName("NSeqPattern")
                || xml->getIntAttribute("version") != 1
                || xml->getNumChildElements() != 1) continue;
            auto* child = xml->getFirstChildElement();
            if (child == nullptr || !child->hasTagName("Assignment")
                || child->getStringAttribute("sequence").length() > 45056) continue;
            PatternEntry entry;
            entry.id = xml->getStringAttribute("id", file.getFileNameWithoutExtension()).substring(0, 80);
            entry.file = file;
            entry.assignment = AssignmentState::read(*child);
            if (entry.assignment.name.empty())
                entry.assignment.name = file.getFileNameWithoutExtension().substring(0, 80).toStdString();
            patternLibrary.push_back(std::move(entry));
        }
    }
    std::sort(patternLibrary.begin(), patternLibrary.end(), [](const PatternEntry& a, const PatternEntry& b)
    {
        if (a.assignment.favourite != b.assignment.favourite) return a.assignment.favourite;
        return juce::String(a.assignment.name).compareIgnoreCase(juce::String(b.assignment.name)) < 0;
    });
    for (int i = 0; i < static_cast<int>(patternLibrary.size()); ++i)
        if (patternLibrary[static_cast<size_t>(i)].id == selectedId) selectedPatternIndex = i;
    filterPatternLibrary();
}

void NSeqArpKeysAudioProcessorEditor::showBankStatus(const juce::String& message, bool error)
{
    bankStatusLabel.setColour(juce::Label::textColourId,
        error ? juce::Colour(0xffff9292) : juce::Colour(0xffa6dfb2));
    bankStatusLabel.setText(message, juce::dontSendNotification);
}

void NSeqArpKeysAudioProcessorEditor::importPatternBank()
{
    presetChooser = std::make_unique<juce::FileChooser>(
        "Import NSeqArpKeys pattern bank", juce::File(), "*.nseqbank");
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    presetChooser->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr || chooser.getResult() == juce::File()) return;
            auto xml = parseSafeXmlFile(chooser.getResult(), PatternBankFile::maxFileBytes);
            std::vector<PatternBankFile::Entry> entries;
            juce::String error;
            if (xml == nullptr || !PatternBankFile::read(*xml, entries, error))
            {
                safeThis->showBankStatus(error.isNotEmpty() ? error : "Could not read pattern bank file.", true);
                return;
            }
            auto directory = patternDirectory();
            if (directory.createDirectory().failed())
            {
                safeThis->showBankStatus("Could not create the pattern bank folder.", true);
                return;
            }
            safeThis->bankSourceSelector.setSelectedId(1, juce::dontSendNotification);
            safeThis->loadPatternLibrary();
            int added = 0, updated = 0;
            for (const auto& entry : entries)
            {
                juce::File target;
                for (const auto& existing : safeThis->patternLibrary)
                    if (existing.id == entry.id) { target = existing.file; break; }
                const bool replacing = target != juce::File();
                if (!replacing)
                {
                    do { target = directory.getChildFile(juce::Uuid().toString() + ".nseqpattern"); }
                    while (target.exists());
                }
                auto pattern = PatternBankFile::writePattern(entry);
                if (!writeXmlFile(target, *pattern))
                {
                    safeThis->loadPatternLibrary();
                    safeThis->showBankStatus("Import stopped: could not write a pattern ("
                        + juce::String(added) + " added, " + juce::String(updated) + " updated).", true);
                    return;
                }
                if (replacing) ++updated; else ++added;
            }
            safeThis->loadPatternLibrary();
            safeThis->showBankStatus("Imported bank: " + juce::String(added) + " added, "
                + juce::String(updated) + " updated.");
        });
}

void NSeqArpKeysAudioProcessorEditor::exportPatternBank()
{
    loadPatternLibrary();
    std::vector<PatternBankFile::Entry> entries;
    for (const auto& pattern : patternLibrary)
        entries.push_back({ pattern.id, pattern.assignment });
    juce::String content, error;
    if (!PatternBankFile::serialize(entries, content, error))
    {
        showBankStatus(error, true);
        return;
    }
    presetChooser = std::make_unique<juce::FileChooser>(
        "Export NSeqArpKeys pattern bank",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("NSeqArpKeys.nseqbank"), "*.nseqbank");
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    presetChooser->launchAsync(juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, content, count = static_cast<int>(entries.size())]
        (const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr || chooser.getResult() == juce::File()) return;
            const auto target = chooser.getResult().withFileExtension(".nseqbank");
            const auto save = [safeThis, target, content, count]
            {
                if (safeThis == nullptr) return;
                juce::TemporaryFile temporary(target);
                const bool saved = temporary.getFile().replaceWithText(content)
                    && temporary.overwriteTargetFileWithTemporary();
                safeThis->showBankStatus(saved
                    ? "Exported " + juce::String(count) + " patterns to " + target.getFileName()
                    : "Could not export pattern bank.", !saved);
            };
            // The native chooser confirmed its selected path, which may differ
            // from the final filename after appending/replacing the extension.
            if (target != chooser.getResult() && target.exists())
                juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle("Replace pattern bank?")
                    .withMessage("Replace \"" + target.getFileName() + "\" with the exported bank?")
                    .withButton("Replace").withButton("Cancel")
                    .withAssociatedComponent(safeThis.getComponent()),
                    [save](int result) { if (result == 1) save(); });
            else
                save();
        });
}

void NSeqArpKeysAudioProcessorEditor::filterPatternLibrary()
{
    const auto query = patternSearchEditor.getText().trim();
    const auto previous = selectedPatternIndex >= 0 && selectedPatternIndex < static_cast<int>(patternLibrary.size())
        ? patternLibrary[static_cast<size_t>(selectedPatternIndex)].id : juce::String();
    filteredPatterns.clear();
    for (int i = 0; i < static_cast<int>(patternLibrary.size()); ++i)
    {
        const auto& a = patternLibrary[static_cast<size_t>(i)].assignment;
        if (bankFavouritesButton.getToggleState() && !a.favourite) continue;
        if (query.isNotEmpty() && !(juce::String(a.name) + " " + juce::String(a.tags)).containsIgnoreCase(query)) continue;
        filteredPatterns.push_back(i);
    }
    if (!patternBrowserOpen) return;
    bankEmptyLabel.setVisible(filteredPatterns.empty());
    presetList.updateContent();
    int row = -1;
    for (int i = 0; i < static_cast<int>(filteredPatterns.size()); ++i)
        if (patternLibrary[static_cast<size_t>(filteredPatterns[static_cast<size_t>(i)])].id == previous)
            row = i;
    if (row < 0 && !filteredPatterns.empty()) row = 0;
    if (row >= 0) { presetList.selectRow(row); selectedRowsChanged(row); }
    else { presetList.deselectAllRows(); selectedRowsChanged(-1); }
}

void NSeqArpKeysAudioProcessorEditor::assignSelectedPattern(bool linked)
{
    if (selectedPatternIndex < 0 || selectedPatternIndex >= static_cast<int>(patternLibrary.size())) return;
    const int key = audioProcessor.getSelectedKey();
    if (linked)
    {
        syncEditHistory();
        AssignmentHistory::Edit edit { key, { { key, audioProcessor.getAssignmentForKey(key) } } };
        const auto id = patternLibrary[static_cast<size_t>(selectedPatternIndex)].id.toStdString();
        for (int other = 0; other < 128; ++other)
            if (other != key)
            {
                auto a = audioProcessor.getAssignmentForKey(other);
                if (a.linkId == id) edit.assignments.emplace_back(other, std::move(a));
            }
        editHistory.record(std::move(edit));
    }
    else recordKeyEdit();
    auto assignment = patternLibrary[static_cast<size_t>(selectedPatternIndex)].assignment;
    assignment.linkId = linked ? patternLibrary[static_cast<size_t>(selectedPatternIndex)].id.toStdString() : "";
    // Joining a link uses the definition already edited in this project.
    if (linked)
        for (int other = 0; other < 128; ++other)
        {
            const auto existing = audioProcessor.getAssignmentForKey(other);
            if (existing.linkId == assignment.linkId) { assignment = existing; break; }
        }
    audioProcessor.setAssignmentForKey(key, assignment);
    setPatternBrowserOpen(false);
    loadAssignmentForKey(key);
}

void NSeqArpKeysAudioProcessorEditor::makeSelectedIndependent()
{
    const int key = audioProcessor.getSelectedKey();
    auto assignment = audioProcessor.getAssignmentForKey(key);
    if (assignment.linkId.empty()) return;
    recordKeyEdit();
    assignment.linkId.clear();
    audioProcessor.setAssignmentForKey(key, assignment);
    loadAssignmentForKey(key);
}

void NSeqArpKeysAudioProcessorEditor::saveSelectedPatternMetadata()
{
    if (selectedPatternIndex < 0 || selectedPatternIndex >= static_cast<int>(patternLibrary.size())) return;
    auto& entry = patternLibrary[static_cast<size_t>(selectedPatternIndex)];
    if (entry.builtIn) return;
    const auto name = bankNameEditor.getText().trim();
    if (name.isEmpty()) return;
    entry.assignment.name = name.toStdString();
    entry.assignment.tags = bankTagsEditor.getText().toStdString();
    entry.assignment.favourite = bankFavouriteButton.getToggleState();
    static const char* palette[] { "#62D6C6", "#77B9FF", "#B99BFF", "#F4BD68", "#EF8B83" };
    const int index = bankColourSelector.getSelectedId() - 1;
    if (index >= 0 && index < 5) entry.assignment.colour = palette[index];
    juce::XmlElement xml("NSeqPattern");
    xml.setAttribute("version", 1);
    xml.setAttribute("id", entry.id);
    AssignmentState::write(*xml.createNewChildElement("Assignment"), entry.assignment);
    if (writeXmlFile(entry.file, xml)) loadPatternLibrary();
}

void NSeqArpKeysAudioProcessorEditor::updatePatternPreview()
{
    const auto a = audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey());
    const auto events = GateRunnerEngine::computeAllStepEvents(a, 16);
    juce::String text = "STEP PREVIEW   ";
    for (const auto& step : events)
        text << (step.empty() ? "· " : juce::String(step.size()) + " ");
    if (a.sequence.size() > 16) text << "…";
    patternPreviewLabel.setText(text, juce::dontSendNotification);
    patternPreviewLabel.setColour(juce::Label::textColourId,
        juce::Colour::fromString("ff" + juce::String(a.colour).trimCharactersAtStart("#")));
}

void NSeqArpKeysAudioProcessorEditor::updateForteSearchResults()
{
    const auto query = forteSearchEditor.getText().trim();
    const auto selected = juce::String(audioProcessor.getAssignmentForKey(
        audioProcessor.getSelectedKey()).forteString);

    forteNumberSelector.clear(juce::dontSendNotification);
    visibleForteIds.clear();
    visibleForteIds.emplace_back();
    forteNumberSelector.addItem("(No set)", 1);

    if (query.isNotEmpty())
        for (const auto& entry : forteSearchEntries)
        {
            if (!entry.display.containsIgnoreCase(query))
                continue;
            visibleForteIds.push_back(entry.id);
            forteNumberSelector.addItem(entry.display,
                                        static_cast<int>(visibleForteIds.size()));
            if (visibleForteIds.size() >= 81)
                break;
        }

    int selectedId = selected.isEmpty() ? 1 : 0;
    for (size_t i = 1; i < visibleForteIds.size(); ++i)
        if (visibleForteIds[i] == selected)
        {
            selectedId = static_cast<int>(i + 1);
            break;
        }
    forteNumberSelector.setSelectedId(selectedId, juce::dontSendNotification);
    forteNumberSelector.setTextWhenNothingSelected(
        query.isEmpty() ? "Type above to search" : "Choose a match");
}

void NSeqArpKeysAudioProcessorEditor::updateSelectedForteLabel()
{
    const auto selected = audioProcessor.getAssignmentForKey(
        audioProcessor.getSelectedKey()).forteString;
    if (selected.empty())
    {
        forteSelectionLabel.setText("Selected set: none", juce::dontSendNotification);
        return;
    }

    const auto name = Pcs12::getCommonNameForForte(selected);
    forteSelectionLabel.setText("Selected set: " + juce::String(selected)
                                    + (name.empty() ? juce::String() : " - " + juce::String(name)),
                                juce::dontSendNotification);
}

juce::String NSeqArpKeysAudioProcessorEditor::captureStateXml() const
{
    juce::MemoryBlock data;
    audioProcessor.getStateInformation(data);
    auto state = juce::AudioProcessor::getXmlFromBinary(data.getData(),
                                                       static_cast<int>(data.getSize()));
    return state != nullptr ? state->toString() : juce::String();
}

juce::String NSeqArpKeysAudioProcessorEditor::normalisedStateXml(const juce::String& xml) const
{
    auto state = parseSafeXml(xml);
    if (!validPresetState(state.get()))
        return {};
    state->removeAttribute("selectedKey");
    state->removeAttribute("currentPresetId");
    for (auto* assignment : state->getChildIterator())
    {
        assignment->setAttribute("gate", static_cast<double>(static_cast<float>(
            assignment->getDoubleAttribute("gate", 0.5))));
        assignment->removeAttribute("lengthFactor");
        assignment->setAttribute("fixedLengthSteps", static_cast<double>(static_cast<float>(
            assignment->getDoubleAttribute("fixedLengthSteps", 0.0))));
    }
    return state->toString();
}

void NSeqArpKeysAudioProcessorEditor::loadPresetLibrary()
{
    presets.clear();
    auto addFactory = [this](const juce::String& id, const juce::String& name,
                             const juce::String& category, const juce::String& tags,
                             const juce::String& description, const juce::String& pattern,
                             const juce::String& forte, double gate)
    {
        juce::XmlElement state("NSeqArpKeys");
        state.setAttribute("selectedKey", 60);
        state.setAttribute("meterNumerator", 4);
        state.setAttribute("meterDenominator", 4);
        state.setAttribute("latchEnabled", false);
        if (pattern.isNotEmpty())
        {
            auto* assignment = state.createNewChildElement("Assignment");
            assignment->setAttribute("key", 60);
            assignment->setAttribute("sequence", pattern);
            assignment->setAttribute("forte", forte);
            assignment->setAttribute("channel", 1);
            assignment->setAttribute("octave", 4);
            assignment->setAttribute("gate", gate);
            assignment->setAttribute("fixedLengthSteps", 0.0);
        }
        PresetEntry preset;
        preset.id = "factory:" + id;
        preset.name = name;
        preset.category = category;
        preset.tags = tags;
        preset.description = description;
        preset.stateXml = state.toString();
        preset.factory = true;
        presets.push_back(std::move(preset));
    };
    addFactory("init", "Init", "Basics", "blank reset",
               "A clean setup with no assigned pattern.", "", "", 0.5);
    addFactory("single-pulse", "Single Note Pulse", "Basics", "pulse simple",
               "One note on alternating steps; hold a key to play.",
               "1 0 1 0", "1-1.0", 0.75);
    addFactory("long-pulse", "Long Pulse", "Gates", "sustain zero steps",
               "A note sustains through three zero steps.",
               "1 0 0 0 1 0 0 0", "1-1.0", 0.9);
    addFactory("diatonic-arp", "Diatonic Arp", "Melodic", "scale diatonic arpeggio",
               "Ascending notes from the diatonic scale.",
               "1 2 4 8 16 32 64 0", "7-35.11", 0.7);
    addFactory("diatonic-gate", "Diatonic Gate", "Gates", "scale diatonic rhythmic",
               "Diatonic notes with rests that extend their gates.",
               "1 0 0 2 0 4 0 0", "7-35.11", 0.8);
    addFactory("hexatonic-motion", "Hexatonic Motion", "Melodic", "hexatonic sequence",
               "A six-note rising and falling phrase.",
               "1 2 4 8 16 32 16 8 4 2", "6-32.0", 0.65);

    const auto directory = presetDirectory();
    if (directory.isDirectory())
    {
        juce::Array<juce::File> files;
        directory.findChildFiles(files, juce::File::findFiles, false, "*.nseqpreset");
        for (const auto& file : files)
        {
            if (file.getSize() > 16 * 1024 * 1024)
                continue;
            auto xml = parseSafeXmlFile(file, 16 * 1024 * 1024);
            if (xml == nullptr || !xml->hasTagName("NSeqPreset")
                || xml->getIntAttribute("version") != 1)
                continue;
            auto* state = xml->getChildByName("NSeqArpKeys");
            if (!validPresetState(state))
                continue;
            PresetEntry preset;
            preset.id = xml->getStringAttribute("id", file.getFileNameWithoutExtension());
            preset.name = xml->getStringAttribute("name").trim();
            preset.category = xml->getStringAttribute("category", "User").trim();
            preset.tags = xml->getStringAttribute("tags");
            preset.description = xml->getStringAttribute("description");
            preset.stateXml = state->toString();
            preset.file = file;
            if (preset.id.isNotEmpty() && preset.name.isNotEmpty())
                presets.push_back(std::move(preset));
        }

        auto favourites = parseSafeXmlFile(directory.getChildFile("favourites.xml"), 1024 * 1024);
        if (favourites != nullptr && favourites->hasTagName("Favourites"))
            for (auto* child : favourites->getChildIterator())
                if (child->hasTagName("Preset"))
                    for (auto& preset : presets)
                        if (preset.id == child->getStringAttribute("id"))
                            preset.favourite = true;
    }
    std::sort(presets.begin(), presets.end(), [](const PresetEntry& a, const PresetEntry& b)
    {
        if (a.factory != b.factory) return a.factory;
        if (a.category != b.category) return a.category.compareIgnoreCase(b.category) < 0;
        return a.name.compareIgnoreCase(b.name) < 0;
    });
    refreshPresetCategories();
    refreshPresetFilters();
    const auto id = audioProcessor.getCurrentPresetId();
    for (const auto& preset : presets)
        if (preset.id == id)
        {
            loadedPresetSnapshot = normalisedStateXml(preset.stateXml);
            break;
        }
    updatePresetDisplay();
}

void NSeqArpKeysAudioProcessorEditor::refreshPresetCategories()
{
    const auto previous = presetCategoryFilter.getText();
    juce::StringArray categories;
    for (const auto& preset : presets)
        categories.addIfNotAlreadyThere(preset.category);
    categories.sort(true);
    presetCategoryFilter.clear(juce::dontSendNotification);
    presetCategoryFilter.addItem("All categories", 1);
    for (int i = 0; i < categories.size(); ++i)
        presetCategoryFilter.addItem(categories[i], i + 2);
    presetCategoryFilter.setSelectedId(1, juce::dontSendNotification);
    for (int i = 0; i < categories.size(); ++i)
        if (categories[i] == previous)
            presetCategoryFilter.setSelectedId(i + 2, juce::dontSendNotification);
}

void NSeqArpKeysAudioProcessorEditor::refreshPresetFilters()
{
    const auto query = presetSearchEditor.getText().trim();
    const auto category = presetCategoryFilter.getText();
    const auto selectedId = selectedPresetIndex >= 0 && selectedPresetIndex < static_cast<int>(presets.size())
        ? presets[static_cast<size_t>(selectedPresetIndex)].id : juce::String();
    filteredPresets.clear();
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
    {
        const auto& preset = presets[static_cast<size_t>(i)];
        if (favouritesOnlyButton.getToggleState() && !preset.favourite)
            continue;
        if (category.isNotEmpty() && category != "All categories" && preset.category != category)
            continue;
        if (query.isNotEmpty()
            && !(preset.name + " " + preset.category + " " + preset.tags + " "
                 + preset.description).containsIgnoreCase(query))
            continue;
        filteredPresets.push_back(i);
    }
    presetList.updateContent();
    int row = -1;
    for (int i = 0; i < static_cast<int>(filteredPresets.size()); ++i)
        if (presets[static_cast<size_t>(filteredPresets[static_cast<size_t>(i)])].id == selectedId)
            row = i;
    if (row < 0 && !filteredPresets.empty()) row = 0;
    if (row >= 0)
    {
        presetList.selectRow(row);
        selectedRowsChanged(row);
    }
    else
    {
        presetList.deselectAllRows();
        selectedRowsChanged(-1);
    }
}

int NSeqArpKeysAudioProcessorEditor::getNumRows()
{
    return static_cast<int>(patternBrowserOpen ? filteredPatterns.size() : filteredPresets.size());
}

void NSeqArpKeysAudioProcessorEditor::paintListBoxItem(int row, juce::Graphics& g,
                                                        int width, int height,
                                                        bool rowIsSelected)
{
    if (patternBrowserOpen)
    {
        if (row < 0 || row >= static_cast<int>(filteredPatterns.size())) return;
        const auto& a = patternLibrary[static_cast<size_t>(filteredPatterns[static_cast<size_t>(row)])].assignment;
        g.fillAll(rowIsSelected ? juce::Colour(0xff294b57) : juce::Colour(0xff1e2836));
        g.setColour(juce::Colour::fromString("ff" + juce::String(a.colour).trimCharactersAtStart("#")));
        g.fillRoundedRectangle(8.0f, 7.0f, 5.0f, static_cast<float>(height - 14), 2.0f);
        g.setColour(juce::Colour(0xffedf6fb));
        g.setFont(14.0f);
        g.drawText((a.favourite ? juce::String::fromUTF8("\xe2\x98\x85 ") : "")
            + (a.name.empty() ? "Untitled pattern" : juce::String(a.name)),
            22, 0, width - 30, height, juce::Justification::centredLeft, true);
        return;
    }
    if (row < 0 || row >= static_cast<int>(filteredPresets.size())) return;
    const auto& preset = presets[static_cast<size_t>(filteredPresets[static_cast<size_t>(row)])];
    g.fillAll(rowIsSelected ? juce::Colour(0xff365e82) : juce::Colour(0xff252d3a));
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    const auto title = (preset.favourite ? juce::String::fromUTF8("\xe2\x98\x85 ") : "")
        + preset.name + (preset.factory ? "  [Factory]" : "");
    g.drawText(title, 8, 0, width - 16, height, juce::Justification::centredLeft, true);
}

void NSeqArpKeysAudioProcessorEditor::selectedRowsChanged(int row)
{
    if (patternBrowserOpen)
    {
        const int nextIndex = row >= 0 && row < static_cast<int>(filteredPatterns.size())
            ? filteredPatterns[static_cast<size_t>(row)] : -1;
        if (nextIndex != selectedPatternIndex) stopPatternAudition();
        selectedPatternIndex = nextIndex;
        const bool selected = selectedPatternIndex >= 0;
        const bool editable = selected
            && !patternLibrary[static_cast<size_t>(selectedPatternIndex)].builtIn;
        for (auto* button : { &assignCopyButton, &assignLinkButton, &auditionButton })
            button->setEnabled(selected);
        bankSaveButton.setEnabled(editable);
        for (juce::Component* field : std::initializer_list<juce::Component*> {
            &bankNameEditor, &bankTagsEditor, &bankColourSelector, &bankFavouriteButton })
            field->setEnabled(editable);
        if (selected)
        {
            const auto& a = patternLibrary[static_cast<size_t>(selectedPatternIndex)].assignment;
            bankDetailsLabel.setText(juce::String(a.sequence.size()) + " steps · "
                + (a.mode == KeyAssignment::Mode::rhythmic ? "Rhythmic" : "Melodic")
                + (editable ? " · My patterns" : " · Built-in"),
                juce::dontSendNotification);
            bankNameEditor.setText(a.name, false);
            bankTagsEditor.setText(a.tags, false);
            bankFavouriteButton.setToggleState(a.favourite, juce::dontSendNotification);
            static const char* palette[] { "#62D6C6", "#77B9FF", "#B99BFF", "#F4BD68", "#EF8B83" };
            int colourIndex = 0;
            for (int i = 0; i < 5; ++i) if (a.colour == palette[i]) colourIndex = i;
            bankColourSelector.setSelectedId(colourIndex + 1, juce::dontSendNotification);
        }
        else
            bankDetailsLabel.setText("No pattern selected", juce::dontSendNotification);
        return;
    }
    selectedPresetIndex = row >= 0 && row < static_cast<int>(filteredPresets.size())
        ? filteredPresets[static_cast<size_t>(row)] : -1;
    if (selectedPresetIndex < 0)
    {
        presetDetailsLabel.setText("No preset selected", juce::dontSendNotification);
        loadPresetButton.setEnabled(false);
        updatePresetButton.setEnabled(false);
        duplicatePresetButton.setEnabled(false);
        deletePresetButton.setEnabled(false);
        favouritePresetButton.setEnabled(false);
        exportPresetButton.setEnabled(false);
        return;
    }
    const auto& preset = presets[static_cast<size_t>(selectedPresetIndex)];
    presetNameEditor.setText(preset.name, false);
    presetCategoryEditor.setText(preset.category, false);
    presetTagsEditor.setText(preset.tags, false);
    presetDescriptionEditor.setText(preset.description, false);
    presetDetailsLabel.setText(preset.factory ? "Factory preset" : "User preset",
                               juce::dontSendNotification);
    loadPresetButton.setEnabled(true);
    updatePresetButton.setEnabled(!preset.factory);
    duplicatePresetButton.setEnabled(true);
    deletePresetButton.setEnabled(!preset.factory);
    favouritePresetButton.setEnabled(true);
    favouritePresetButton.setButtonText(preset.favourite ? "Unfavourite" : "Favourite");
    exportPresetButton.setEnabled(true);
}

void NSeqArpKeysAudioProcessorEditor::showPresetStatus(const juce::String& message, bool error)
{
    presetStatusLabel.setColour(juce::Label::textColourId,
                                error ? juce::Colour(0xffff9292) : juce::Colour(0xffa6dfb2));
    presetStatusLabel.setText(message, juce::dontSendNotification);
}

void NSeqArpKeysAudioProcessorEditor::setBrowserOpen(bool open)
{
    if (open) stopPatternAudition();
    browserOpen = open;
    if (open) patternBrowserOpen = false;
    updateModeVisibility();
}

void NSeqArpKeysAudioProcessorEditor::setPatternBrowserOpen(bool open)
{
    if (!open) stopPatternAudition();
    patternBrowserOpen = open;
    if (open)
    {
        browserOpen = false;
        loadPatternLibrary();
    }
    updateModeVisibility();
}

void NSeqArpKeysAudioProcessorEditor::stopPatternAudition()
{
    if (auditioningKey >= 0) audioProcessor.requestStopKey(auditioningKey);
    auditioningKey = -1;
    auditionButton.setButtonText("Audition");
}

void NSeqArpKeysAudioProcessorEditor::updateModeVisibility()
{
    const bool editor = !browserOpen && !patternBrowserOpen;
    const bool rhythmic = modeSelector.getSelectedId() == 2;
    browsePresetsButton.setButtonText(browserOpen ? "Back to Editor" : "Presets");
    browsePatternsButton.setButtonText(patternBrowserOpen ? "Back to Editor" : "Pattern Bank");
    for (juce::Component* component : std::initializer_list<juce::Component*> { &keyboardComponent, &selectedKeyLabel,
                                        &activeKeysLabel,
                                        &latchButton, &previewSoundButton, &stopKeyButton, &stopAllButton,
                                        &meterNumeratorLabel, &meterNumeratorSlider,
                                        &meterDenominatorLabel, &meterDenominatorSlider,
                                        &channelLabel, &channelSlider, &octaveLabel,
                                        &octaveSlider, &gateLabel, &gateSlider,
                                        &fixedLengthStepsLabel, &fixedLengthStepsSlider,
                                        &patternLabel, &patternTextEditor, &forteSearchLabel,
                                        &forteSearchEditor, &forteLabel, &forteNumberSelector,
                                        &forteSelectionLabel,
                                        &patternNameLabel, &patternNameEditor,
                                        &modeLabel, &modeSelector, &subdivisionLabel, &subdivisionSlider,
                                        &velocityLabel, &velocitySlider, &transposeLabel, &transposeSlider,
                                        &rotationLabel, &rotationSlider, &reverseButton,
                                        &drumVelocityBitsLabel, &drumVelocityBitsSlider,
                                        &drumNotesLabel, &drumNotesEditor, &patternPreviewLabel,
                                        &patternColourLabel, &patternColourSelector,
                                        &patternTagsLabel, &patternTagsEditor, &patternFavouriteButton,
                                        &copyPatternButton, &pastePatternButton, &duplicatePatternButton,
                                        &cutPatternButton, &clearPatternButton, &pasteScopeSelector,
                                        &independentButton,
                                        &undoButton, &redoButton,
                                        &savePatternButton, &loadPatternButton,
                                        &rangeLabel, &rangeFirstSlider, &rangeLastSlider,
                                        &transposeRangeButton, &applyRangeButton })
        component->setVisible(editor);
    for (juce::Component* component : std::initializer_list<juce::Component*> {
        &drumVelocityBitsLabel, &drumVelocityBitsSlider, &drumNotesLabel, &drumNotesEditor })
        component->setVisible(editor && rhythmic);
    for (juce::Component* component : std::initializer_list<juce::Component*> {
        &octaveLabel, &octaveSlider, &forteSearchLabel, &forteSearchEditor,
        &forteLabel, &forteNumberSelector, &forteSelectionLabel })
        component->setVisible(editor && !rhythmic);
    independentButton.setEnabled(audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey()).linkId.size() > 0);
    for (juce::Component* component : std::initializer_list<juce::Component*> { &presetSearchLabel, &presetSearchEditor,
                                        &presetCategoryFilterLabel, &presetCategoryFilter,
                                        &favouritesOnlyButton, &presetList,
                                        &presetDetailsLabel, &presetStatusLabel,
                                        &presetNameFieldLabel, &presetNameEditor,
                                        &presetCategoryFieldLabel, &presetCategoryEditor,
                                        &presetTagsFieldLabel, &presetTagsEditor,
                                        &presetDescriptionFieldLabel, &presetDescriptionEditor,
                                        &loadPresetButton, &saveNewPresetButton,
                                        &updatePresetButton, &duplicatePresetButton,
                                        &deletePresetButton, &favouritePresetButton,
                                        &importPresetButton, &exportPresetButton })
        component->setVisible(browserOpen);
    presetList.setVisible(browserOpen || patternBrowserOpen);
    for (juce::Component* component : std::initializer_list<juce::Component*> {
        &patternSearchLabel, &patternSearchEditor, &bankSourceSelector,
        &bankDetailsLabel, &bankTagsLabel,
        &bankHelpLabel,
        &bankNameEditor, &bankTagsEditor, &bankColourSelector,
        &bankFavouritesButton, &bankFavouriteButton,
        &assignCopyButton, &assignLinkButton, &auditionButton, &bankSaveButton,
        &saveCurrentToBankButton, &importBankButton, &exportBankButton, &bankStatusLabel })
        component->setVisible(patternBrowserOpen);
    bankEmptyLabel.setVisible(patternBrowserOpen && filteredPatterns.empty());
    presetList.updateContent();
    // Melodic mode has an octave row and the selected-set description.
    setSize(getWidth(), editor && !rhythmic ? 900 : 830);
    resized();
}

void NSeqArpKeysAudioProcessorEditor::updatePresetDisplay()
{
    const auto id = audioProcessor.getCurrentPresetId();
    if (id != lastDisplayedPresetId)
    {
        lastDisplayedPresetId = id;
        loadedPresetSnapshot.clear();
        for (const auto& preset : presets)
            if (preset.id == id)
            {
                loadedPresetSnapshot = normalisedStateXml(preset.stateXml);
                break;
            }
    }
    juce::String label = "Preset: Unsaved setup";
    for (const auto& preset : presets)
        if (preset.id == id)
        {
            const bool dirty = normalisedStateXml(captureStateXml()) != loadedPresetSnapshot;
            label = "Preset: " + preset.name + (dirty ? " *" : "");
            break;
        }
    presetNameLabel.setText(label, juce::dontSendNotification);
}

void NSeqArpKeysAudioProcessorEditor::timerCallback()
{
    juce::StringArray sounding;
    for (int key = 0; key < 128; ++key)
        if (audioProcessor.isKeySounding(key))
            sounding.add(juce::MidiMessage::getMidiNoteName(key, true, true, 4));
    juce::String active = sounding.isEmpty() ? "No active keys" : "Playing: " + sounding.joinIntoString("  ·  ");
    activeKeysLabel.setText(active, juce::dontSendNotification);
    activeKeysLabel.setTooltip(active);
    previewSoundButton.setToggleState(audioProcessor.isPreviewSoundEnabled(), juce::dontSendNotification);
    // Host automation may update the global parameter without restoring state.
    if (!meterNumeratorSlider.isMouseButtonDown())
        meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get(), juce::dontSendNotification);
    if (!meterDenominatorSlider.isMouseButtonDown())
        meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get(), juce::dontSendNotification);
    const auto revision = audioProcessor.getStateRestoreRevision();
    if (revision != lastStateRestoreRevision)
    {
        stopPatternAudition();
        syncEditHistory();
        meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get(), juce::dontSendNotification);
        meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get(), juce::dontSendNotification);
        latchButton.setToggleState(audioProcessor.isLatchEnabled(), juce::dontSendNotification);
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    }
    updateTimingDisplay();
    undoButton.setEnabled(editHistory.canUndo());
    redoButton.setEnabled(editHistory.canRedo());
    applyRangeButton.setEnabled(rangeFirstSlider.getValue() <= rangeLastSlider.getValue());
    const auto state = normalisedStateXml(captureStateXml());
    if (state != lastObservedState || audioProcessor.getCurrentPresetId() != lastDisplayedPresetId)
    {
        lastObservedState = state;
        updatePresetDisplay();
    }
}

void NSeqArpKeysAudioProcessorEditor::selectPresetById(const juce::String& id)
{
    presetSearchEditor.clear();
    favouritesOnlyButton.setToggleState(false, juce::dontSendNotification);
    presetCategoryFilter.setSelectedId(1, juce::dontSendNotification);
    refreshPresetFilters();
    for (int row = 0; row < static_cast<int>(filteredPresets.size()); ++row)
        if (presets[static_cast<size_t>(filteredPresets[static_cast<size_t>(row)])].id == id)
        {
            presetList.selectRow(row);
            selectedRowsChanged(row);
            presetList.scrollToEnsureRowIsOnscreen(row);
            break;
        }
}

void NSeqArpKeysAudioProcessorEditor::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets.size())) return;
    const auto preset = presets[static_cast<size_t>(index)];
    auto state = parseSafeXml(preset.stateXml);
    if (!validPresetState(state.get()))
    {
        showPresetStatus("Preset data is invalid.", true);
        return;
    }
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary(*state, data);
    audioProcessor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    audioProcessor.setCurrentPresetId(preset.id);
    syncEditHistory();
    lastDisplayedPresetId = preset.id;
    loadedPresetSnapshot = normalisedStateXml(preset.stateXml);
    lastObservedState = normalisedStateXml(captureStateXml());
    meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get(), juce::dontSendNotification);
    meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get(), juce::dontSendNotification);
    latchButton.setToggleState(audioProcessor.isLatchEnabled(), juce::dontSendNotification);
    loadAssignmentForKey(audioProcessor.getSelectedKey());
    updatePresetDisplay();
    setBrowserOpen(false);
}

void NSeqArpKeysAudioProcessorEditor::navigatePreset(int direction)
{
    if (presets.empty()) return;
    const auto id = audioProcessor.getCurrentPresetId();
    int index = -1;
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
        if (presets[static_cast<size_t>(i)].id == id) index = i;
    if (index < 0) index = direction > 0 ? -1 : 0;
    index = (index + direction + static_cast<int>(presets.size())) % static_cast<int>(presets.size());
    loadPreset(index);
}

bool NSeqArpKeysAudioProcessorEditor::writePreset(PresetEntry& preset)
{
    preset.name = preset.name.trim();
    preset.category = preset.category.trim();
    if (preset.name.isEmpty() || preset.name.length() > 80)
    {
        showPresetStatus("Enter a preset name of 1 to 80 characters.", true);
        return false;
    }
    if (preset.category.isEmpty()) preset.category = "User";
    if (preset.category.length() > 40 || preset.tags.length() > 200
        || preset.description.length() > 500)
    {
        showPresetStatus("Preset details are too long.", true);
        return false;
    }
    auto state = parseSafeXml(preset.stateXml);
    if (!validPresetState(state.get()))
    {
        showPresetStatus("Cannot save invalid preset state.", true);
        return false;
    }
    if (preset.id.isEmpty()) preset.id = juce::Uuid().toString();
    if (preset.file == juce::File())
        preset.file = presetDirectory().getChildFile(preset.id + ".nseqpreset");
    juce::XmlElement xml("NSeqPreset");
    xml.setAttribute("version", 1);
    xml.setAttribute("id", preset.id);
    xml.setAttribute("name", preset.name);
    xml.setAttribute("category", preset.category);
    xml.setAttribute("tags", preset.tags);
    xml.setAttribute("description", preset.description);
    xml.addChildElement(state.release());
    if (!writeXmlFile(preset.file, xml))
    {
        showPresetStatus("Could not write the preset file.", true);
        return false;
    }
    return true;
}

void NSeqArpKeysAudioProcessorEditor::saveNewPreset()
{
    PresetEntry preset;
    preset.name = presetNameEditor.getText();
    preset.category = presetCategoryEditor.getText();
    preset.tags = presetTagsEditor.getText();
    preset.description = presetDescriptionEditor.getText();
    preset.stateXml = captureStateXml();
    if (!writePreset(preset)) return;
    audioProcessor.setCurrentPresetId(preset.id);
    const auto id = preset.id;
    loadPresetLibrary();
    selectPresetById(id);
    showPresetStatus("Saved new preset: " + preset.name);
}

void NSeqArpKeysAudioProcessorEditor::updateSelectedPreset()
{
    if (selectedPresetIndex < 0 || selectedPresetIndex >= static_cast<int>(presets.size())) return;
    auto preset = presets[static_cast<size_t>(selectedPresetIndex)];
    if (preset.factory) return;
    preset.name = presetNameEditor.getText();
    preset.category = presetCategoryEditor.getText();
    preset.tags = presetTagsEditor.getText();
    preset.description = presetDescriptionEditor.getText();
    preset.stateXml = captureStateXml();
    if (!writePreset(preset)) return;
    audioProcessor.setCurrentPresetId(preset.id);
    const auto id = preset.id;
    loadPresetLibrary();
    selectPresetById(id);
    showPresetStatus("Updated preset: " + preset.name);
}

void NSeqArpKeysAudioProcessorEditor::duplicateSelectedPreset()
{
    if (selectedPresetIndex < 0 || selectedPresetIndex >= static_cast<int>(presets.size())) return;
    auto preset = presets[static_cast<size_t>(selectedPresetIndex)];
    preset.id.clear();
    preset.file = juce::File();
    preset.factory = false;
    preset.favourite = false;
    preset.name += " Copy";
    if (!writePreset(preset)) return;
    const auto id = preset.id;
    loadPresetLibrary();
    selectPresetById(id);
    loadPreset(selectedPresetIndex);
    setBrowserOpen(true);
    showPresetStatus("Duplicated preset: " + preset.name);
}

void NSeqArpKeysAudioProcessorEditor::saveFavourites() const
{
    juce::XmlElement xml("Favourites");
    for (const auto& preset : presets)
        if (preset.favourite)
            xml.createNewChildElement("Preset")->setAttribute("id", preset.id);
    writeXmlFile(presetDirectory().getChildFile("favourites.xml"), xml);
}

void NSeqArpKeysAudioProcessorEditor::toggleSelectedFavourite()
{
    if (selectedPresetIndex < 0 || selectedPresetIndex >= static_cast<int>(presets.size())) return;
    auto& preset = presets[static_cast<size_t>(selectedPresetIndex)];
    preset.favourite = !preset.favourite;
    saveFavourites();
    refreshPresetFilters();
}

void NSeqArpKeysAudioProcessorEditor::deleteSelectedPreset()
{
    if (selectedPresetIndex < 0 || selectedPresetIndex >= static_cast<int>(presets.size())
        || presets[static_cast<size_t>(selectedPresetIndex)].factory)
        return;
    const auto id = presets[static_cast<size_t>(selectedPresetIndex)].id;
    const auto name = presets[static_cast<size_t>(selectedPresetIndex)].name;
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::WarningIcon)
            .withTitle("Delete preset")
            .withMessage("Delete \"" + name + "\" from your preset library?")
            .withButton("Delete")
            .withButton("Cancel")
            .withAssociatedComponent(this),
        [safeThis, id](int result)
        {
            if (safeThis == nullptr || result != 1) return;
            for (const auto& preset : safeThis->presets)
                if (preset.id == id && !preset.factory)
                {
                    if (!preset.file.deleteFile())
                    {
                        safeThis->showPresetStatus("Could not delete the preset file.", true);
                        return;
                    }
                    if (safeThis->audioProcessor.getCurrentPresetId() == id)
                        safeThis->audioProcessor.setCurrentPresetId({});
                    safeThis->loadPresetLibrary();
                    safeThis->saveFavourites();
                    safeThis->showPresetStatus("Deleted preset.");
                    return;
                }
        });
}

void NSeqArpKeysAudioProcessorEditor::importPreset()
{
    presetChooser = std::make_unique<juce::FileChooser>(
        "Import NSeqArpKeys preset", juce::File(), "*.nseqpreset");
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    presetChooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr) return;
            const auto file = chooser.getResult();
            if (file == juce::File()) return;
            if (file.getSize() > 16 * 1024 * 1024)
            {
                safeThis->showPresetStatus("Preset file is too large.", true);
                return;
            }
            auto xml = parseSafeXmlFile(file, 16 * 1024 * 1024);
            if (xml == nullptr || !xml->hasTagName("NSeqPreset")
                || xml->getIntAttribute("version") != 1
                || !validPresetState(xml->getChildByName("NSeqArpKeys")))
            {
                safeThis->showPresetStatus("This is not a valid NSeqArpKeys preset.", true);
                return;
            }
            PresetEntry preset;
            preset.name = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
            preset.category = xml->getStringAttribute("category", "Imported");
            preset.tags = xml->getStringAttribute("tags");
            preset.description = xml->getStringAttribute("description");
            preset.stateXml = xml->getChildByName("NSeqArpKeys")->toString();
            if (!safeThis->writePreset(preset)) return;
            const auto id = preset.id;
            safeThis->loadPresetLibrary();
            safeThis->selectPresetById(id);
            safeThis->showPresetStatus("Imported preset: " + preset.name);
        });
}

void NSeqArpKeysAudioProcessorEditor::exportSelectedPreset()
{
    if (selectedPresetIndex < 0 || selectedPresetIndex >= static_cast<int>(presets.size())) return;
    const auto preset = presets[static_cast<size_t>(selectedPresetIndex)];
    const auto safeName = preset.name.retainCharacters("abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_").trim().replaceCharacter(' ', '-');
    presetChooser = std::make_unique<juce::FileChooser>(
        "Export NSeqArpKeys preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile((safeName.isEmpty() ? "Preset" : safeName) + ".nseqpreset"),
        "*.nseqpreset");
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    presetChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, preset](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr) return;
            auto file = chooser.getResult();
            if (file == juce::File()) return;
            file = file.withFileExtension(".nseqpreset");
            auto state = parseSafeXml(preset.stateXml);
            if (!validPresetState(state.get()))
            {
                safeThis->showPresetStatus("Preset data is invalid.", true);
                return;
            }
            juce::XmlElement xml("NSeqPreset");
            xml.setAttribute("version", 1);
            xml.setAttribute("id", preset.id);
            xml.setAttribute("name", preset.name);
            xml.setAttribute("category", preset.category);
            xml.setAttribute("tags", preset.tags);
            xml.setAttribute("description", preset.description);
            xml.addChildElement(state.release());
            const bool saved = writeXmlFile(file, xml);
            safeThis->showPresetStatus(saved
                ? "Exported preset to " + file.getFileName()
                : "Could not export preset.", !saved);
        });
}
