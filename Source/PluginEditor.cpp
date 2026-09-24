#include "PluginEditor.h"
#include "Engine/GateRunnerEngine.h"
#include "Domain/RhythmGenerators.h"

namespace
{
juce::File presetDirectory()
{
    const auto custom = juce::SystemStats::getEnvironmentVariable("NSEQARPKEYS_PRESET_DIR", {});
    if (custom.isNotEmpty() && juce::File::isAbsolutePath(custom))
        return juce::File(custom);
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("NSeqArpKeys").getChildFile("Presets");
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
    for (auto* child : state->getChildIterator())
    {
        if (!child->hasTagName("Assignment")
            || child->getIntAttribute("key", -1) < 0
            || child->getIntAttribute("key", -1) > 127
            || ++assignments > 128)
            return false;
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
    // ----- Keyboard -----------------------------------------------------------
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);

    presetNameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(presetNameLabel);
    previousPresetButton.setButtonText("<");
    nextPresetButton.setButtonText(">");
    browsePresetsButton.setButtonText("Browse Presets");
    savePresetButton.setButtonText("Save");
    previousPresetButton.onClick = [this] { navigatePreset(-1); };
    nextPresetButton.onClick = [this] { navigatePreset(1); };
    browsePresetsButton.onClick = [this] { setBrowserOpen(!browserOpen); };
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
                          &browsePresetsButton, &savePresetButton })
        addAndMakeVisible(button);

    latchButton.setButtonText("Latch (keep playing)");
    latchButton.setTooltip("Off: play while a key is held. On: click a key to keep its pattern playing while you edit; click it again to stop.");
    latchButton.setToggleState(audioProcessor.isLatchEnabled(), juce::dontSendNotification);
    latchButton.onClick = [this] { audioProcessor.setLatchEnabled(latchButton.getToggleState()); };
    addAndMakeVisible(latchButton);

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
    fixedLengthStepsLabel.setTooltip("Adds this many Meter Den steps to the gated note length.");
    addAndMakeVisible(fixedLengthStepsLabel);
    addAndMakeVisible(fixedLengthStepsSlider);
    fixedLengthStepsSlider.setRange(0.0, 16.0, 0.01);
    fixedLengthStepsSlider.setTooltip("0 to 16 steps added to Gate; each step is set by Meter Den and tempo.");
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
    patternTextEditor.onTextChange = [this]
    {
        recordKeyEdit();
        audioProcessor.setPatternForKey(audioProcessor.getSelectedKey(),
                                        patternTextEditor.getText().toStdString());
    };

    auto changeAssignment = [this](auto change)
    {
        const int key = audioProcessor.getSelectedKey();
        recordKeyEdit();
        auto assignment = audioProcessor.getAssignmentForKey(key);
        change(assignment);
        audioProcessor.setAssignmentForKey(key, assignment);
    };
    auto addLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        addAndMakeVisible(label);
    };
    addLabel(patternNameLabel, "Name");
    addAndMakeVisible(patternNameEditor);
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
            if (a.mode == KeyAssignment::Mode::rhythmic && a.channel == 1)
                a.channel = 10;
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
    transposeSlider.setRange(-48, 48, 1);
    transposeSlider.onValueChange = [changeAssignment, this]
    {
        changeAssignment([this](KeyAssignment& a) { a.transpose = static_cast<int>(transposeSlider.getValue()); });
    };
    addLabel(rotationLabel, "Rotate");
    addAndMakeVisible(rotationSlider);
    rotationSlider.setRange(-64, 64, 1);
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
    auto addLane = [this, changeAssignment, addLabel](juce::Label& label, juce::TextEditor& editor,
                                                       const juce::String& title, bool velocity)
    {
        addLabel(label, title);
        addAndMakeVisible(editor);
        editor.onTextChange = [changeAssignment, editorPtr = &editor, velocity]
        {
            auto values = GateRunnerEngine::parseSequence(editorPtr->getText().toStdString());
            for (auto& value : values)
                value = velocity ? juce::jlimit(0, 127, value) : juce::jlimit(-127, 127, value);
            changeAssignment([&](KeyAssignment& a) {
                if (velocity) a.velocitySteps = values;
                else a.pitchSteps = values;
            });
        };
    };
    addLane(velocityStepsLabel, velocityStepsEditor, "Velocity lane", true);
    addLane(pitchStepsLabel, pitchStepsEditor, "Pitch lane", false);
    addLabel(drumNotesLabel, "Drum notes");
    addAndMakeVisible(drumNotesEditor);
    drumNotesEditor.setTooltip("16 MIDI note numbers for bits 0–15; e.g. 36 38 42 ...");
    drumNotesEditor.onTextChange = [changeAssignment, this]
    {
        const auto notes = GateRunnerEngine::parseSequence(drumNotesEditor.getText().toStdString());
        if (notes.size() != 16) return;
        changeAssignment([&notes](KeyAssignment& a) {
            for (size_t i = 0; i < notes.size(); ++i)
                a.drumNotes[i] = juce::jlimit(0, 127, notes[i]);
        });
    };
    for (auto* button : { &copyPatternButton, &pastePatternButton, &duplicatePatternButton,
                          &undoButton, &redoButton,
                          &savePatternButton, &loadPatternButton, &applyRangeButton })
        addAndMakeVisible(button);
    copyPatternButton.setButtonText("Copy");
    pastePatternButton.setButtonText("Paste");
    duplicatePatternButton.setButtonText("Duplicate to next key");
    undoButton.setButtonText("Undo");
    redoButton.setButtonText("Redo");
    savePatternButton.setButtonText("Save pattern");
    loadPatternButton.setButtonText("Load pattern");
    applyRangeButton.setButtonText("Assign range");
    copyPatternButton.onClick = [this] {
        copiedPattern = std::make_unique<KeyAssignment>(
            audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey()));
    };
    pastePatternButton.onClick = [this] {
        if (copiedPattern == nullptr) return;
        recordKeyEdit();
        audioProcessor.setAssignmentForKey(audioProcessor.getSelectedKey(), *copiedPattern);
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    };
    duplicatePatternButton.onClick = [this] {
        const int key = audioProcessor.getSelectedKey();
        if (key == 127) return;
        undoHistory.emplace_back(key + 1, audioProcessor.getAssignmentForKey(key + 1));
        redoHistory.clear();
        audioProcessor.setAssignmentForKey(key + 1, audioProcessor.getAssignmentForKey(key));
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
    addLabel(euclidLabel, "Euclidean");
    addAndMakeVisible(euclidHitsSlider);
    addAndMakeVisible(euclidStepsSlider);
    euclidHitsSlider.setRange(0, 64, 1);
    euclidStepsSlider.setRange(1, 64, 1);
    euclidHitsSlider.setValue(5);
    euclidStepsSlider.setValue(13);
    euclidButton.setButtonText("Generate bit 0");
    addAndMakeVisible(euclidButton);
    euclidButton.onClick = [this]
    {
        const auto values = makeEuclideanRhythm(static_cast<int>(euclidHitsSlider.getValue()),
                                                 static_cast<int>(euclidStepsSlider.getValue()));
        auto assignment = audioProcessor.getAssignmentForKey(audioProcessor.getSelectedKey());
        recordKeyEdit();
        assignment.mode = KeyAssignment::Mode::rhythmic;
        if (assignment.channel == 1) assignment.channel = 10;
        assignment.sequence = values;
        audioProcessor.setAssignmentForKey(audioProcessor.getSelectedKey(), assignment);
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    };

    // ----- Per-key: searchable Forte set --------------------------------------
    forteSearchLabel.setText("Find Set", juce::dontSendNotification);
    addAndMakeVisible(forteSearchLabel);
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
        }
    };

    forteSelectionLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(forteSelectionLabel);

    // ----- Selected key label -------------------------------------------------
    selectedKeyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(selectedKeyLabel);

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

    // ----- Initial load -------------------------------------------------------
    setSize(860, 860);
    loadPresetLibrary();
    loadAssignmentForKey(audioProcessor.getSelectedKey());
    lastStateRestoreRevision = audioProcessor.getStateRestoreRevision();
    setBrowserOpen(false);
    startTimerHz(2);
}

NSeqArpKeysAudioProcessorEditor::~NSeqArpKeysAudioProcessorEditor()
{
    presetList.setModel(nullptr);
    keyboardState.removeListener(this);
}

//==============================================================================
void NSeqArpKeysAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
}

void NSeqArpKeysAudioProcessorEditor::resized()
{
    auto area  = getLocalBounds().reduced(8);
    int  w     = area.getWidth();
    int  lblW  = 110;
    int  rowH  = 26;

    auto presetRow = area.removeFromTop(32);
    presetNameLabel.setBounds(presetRow.removeFromLeft(300));
    previousPresetButton.setBounds(presetRow.removeFromLeft(34));
    presetRow.removeFromLeft(4);
    nextPresetButton.setBounds(presetRow.removeFromLeft(34));
    presetRow.removeFromLeft(8);
    browsePresetsButton.setBounds(presetRow.removeFromLeft(135));
    presetRow.removeFromLeft(8);
    savePresetButton.setBounds(presetRow.removeFromLeft(90));
    area.removeFromTop(6);

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

    keyboardComponent.setBounds(area.removeFromTop(80));
    area.removeFromTop(6);

    // Selected key display
    selectedKeyLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    auto transportRow = area.removeFromTop(28);
    latchButton.setBounds(transportRow.removeFromLeft(210));
    transportRow.removeFromLeft(8);
    stopKeyButton.setBounds(transportRow.removeFromLeft(110));
    transportRow.removeFromLeft(8);
    stopAllButton.setBounds(transportRow.removeFromLeft(110));
    area.removeFromTop(6);

    // Two-column layout for global params
    auto globalRow = area.removeFromTop(rowH);
    meterNumeratorLabel .setBounds(globalRow.removeFromLeft(lblW));
    meterNumeratorSlider.setBounds(globalRow.removeFromLeft((w - 2 * lblW) / 2));
    meterDenominatorLabel .setBounds(globalRow.removeFromLeft(lblW));
    meterDenominatorSlider.setBounds(globalRow);

    area.removeFromTop(4);

    // Per-key controls (one per row, label + control)
    auto makeRow = [&](juce::Label& lbl, juce::Component& ctrl)
    {
        auto row = area.removeFromTop(rowH);
        lbl.setBounds(row.removeFromLeft(lblW));
        ctrl.setBounds(row);
        area.removeFromTop(2);
    };

    makeRow(channelLabel, channelSlider);
    makeRow(octaveLabel,  octaveSlider);
    makeRow(gateLabel,    gateSlider);
    makeRow(fixedLengthStepsLabel, fixedLengthStepsSlider);
    makeRow(patternLabel, patternTextEditor);
    makeRow(patternNameLabel, patternNameEditor);
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
    makeRow(velocityStepsLabel, velocityStepsEditor);
    makeRow(pitchStepsLabel, pitchStepsEditor);
    makeRow(drumNotesLabel, drumNotesEditor);
    reverseButton.setBounds(area.removeFromTop(rowH));
    auto copyRow = area.removeFromTop(30);
    copyPatternButton.setBounds(copyRow.removeFromLeft(90));
    pastePatternButton.setBounds(copyRow.removeFromLeft(90));
    duplicatePatternButton.setBounds(copyRow.removeFromLeft(170));
    undoButton.setBounds(copyRow.removeFromLeft(65));
    redoButton.setBounds(copyRow.removeFromLeft(65));
    savePatternButton.setBounds(copyRow.removeFromLeft(115));
    loadPatternButton.setBounds(copyRow.removeFromLeft(115));
    area.removeFromTop(3);
    auto rangeRow = area.removeFromTop(rowH);
    rangeLabel.setBounds(rangeRow.removeFromLeft(lblW));
    rangeFirstSlider.setBounds(rangeRow.removeFromLeft(140));
    rangeLastSlider.setBounds(rangeRow.removeFromLeft(140));
    transposeRangeButton.setBounds(rangeRow.removeFromLeft(160));
    applyRangeButton.setBounds(rangeRow.removeFromLeft(130));
    auto euclidRow = area.removeFromTop(rowH);
    euclidLabel.setBounds(euclidRow.removeFromLeft(lblW));
    euclidHitsSlider.setBounds(euclidRow.removeFromLeft(140));
    euclidStepsSlider.setBounds(euclidRow.removeFromLeft(140));
    euclidButton.setBounds(euclidRow.removeFromLeft(145));
    makeRow(forteSearchLabel, forteSearchEditor);
    makeRow(forteLabel,   forteNumberSelector);
    forteSelectionLabel.setBounds(area.removeFromTop(rowH));
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

    juce::String name = juce::MidiMessage::getMidiNoteName(midiNoteNumber, true, true, 4);
    selectedKeyLabel.setText("Selected key: " + name
                             + " (MIDI " + juce::String(midiNoteNumber) + ")",
                             juce::dontSendNotification);
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
    // For TextEditor, setText with false (= don't move cursor) does not
    // call onTextChange, so it is safe to call directly.
    patternTextEditor.setText(a.sequenceToString(), false);
    patternNameEditor.setText(a.name, false);
    modeSelector.setSelectedId(a.mode == KeyAssignment::Mode::rhythmic ? 2 : 1, juce::dontSendNotification);
    subdivisionSlider.setValue(a.subdivision, juce::dontSendNotification);
    subdivisionLabel.setTooltip(a.subdivision == 0
        ? "Inherits global steps per quarter note: " + juce::String(audioProcessor.getMeterDenominator()->get())
        : "Per-key steps per quarter note");
    velocitySlider.setValue(a.velocity, juce::dontSendNotification);
    transposeSlider.setValue(a.transpose, juce::dontSendNotification);
    rotationSlider.setValue(a.rotation, juce::dontSendNotification);
    reverseButton.setToggleState(a.reverse, juce::dontSendNotification);
    auto valuesText = [](const std::vector<int>& values) {
        juce::StringArray parts;
        for (int n : values) parts.add(juce::String(n));
        return parts.joinIntoString(" ");
    };
    velocityStepsEditor.setText(valuesText(a.velocitySteps), false);
    pitchStepsEditor.setText(valuesText(a.pitchSteps), false);
    drumNotesEditor.setText(valuesText(std::vector<int>(a.drumNotes.begin(), a.drumNotes.end())), false);

    channelSlider.setValue(a.channel, juce::dontSendNotification);
    octaveSlider .setValue(a.octave,  juce::dontSendNotification);
    gateSlider   .setValue(static_cast<double>(a.gate), juce::dontSendNotification);
    fixedLengthStepsSlider.setValue(static_cast<double>(a.fixedLengthSteps), juce::dontSendNotification);

    forteSearchEditor.setText(a.forteString, false);
    updateForteSearchResults();
    updateSelectedForteLabel();

    juce::String name = juce::MidiMessage::getMidiNoteName(key, true, true, 4);
    selectedKeyLabel.setText("Selected key: " + name
                             + " (MIDI " + juce::String(key) + ")",
                             juce::dontSendNotification);
}

void NSeqArpKeysAudioProcessorEditor::applyRange()
{
    const int first = static_cast<int>(rangeFirstSlider.getValue());
    const int last = static_cast<int>(rangeLastSlider.getValue());
    if (first > last) return;
    audioProcessor.copyAssignmentToRange(audioProcessor.getSelectedKey(), first, last,
                                          transposeRangeButton.getToggleState());
}

void NSeqArpKeysAudioProcessorEditor::recordKeyEdit()
{
    const int key = audioProcessor.getSelectedKey();
    if (undoHistory.size() == 100) undoHistory.erase(undoHistory.begin());
    undoHistory.emplace_back(key, audioProcessor.getAssignmentForKey(key));
    redoHistory.clear();
}

void NSeqArpKeysAudioProcessorEditor::undoKeyEdit()
{
    if (undoHistory.empty()) return;
    const auto previous = std::move(undoHistory.back());
    undoHistory.pop_back();
    redoHistory.emplace_back(previous.first, audioProcessor.getAssignmentForKey(previous.first));
    audioProcessor.setAssignmentForKey(previous.first, previous.second);
    audioProcessor.setSelectedKey(previous.first);
    loadAssignmentForKey(previous.first);
}

void NSeqArpKeysAudioProcessorEditor::redoKeyEdit()
{
    if (redoHistory.empty()) return;
    const auto next = std::move(redoHistory.back());
    redoHistory.pop_back();
    undoHistory.emplace_back(next.first, audioProcessor.getAssignmentForKey(next.first));
    audioProcessor.setAssignmentForKey(next.first, next.second);
    audioProcessor.setSelectedKey(next.first);
    loadAssignmentForKey(next.first);
}

void NSeqArpKeysAudioProcessorEditor::savePatternToBank()
{
    const int key = audioProcessor.getSelectedKey();
    auto state = juce::XmlDocument::parse(captureStateXml());
    if (state == nullptr) return;
    juce::XmlElement bank("NSeqPattern");
    bank.setAttribute("version", 1);
    bool found = false;
    for (auto* child : state->getChildIterator())
        if (child->hasTagName("Assignment") && child->getIntAttribute("key", -1) == key)
        {
            bank.addChildElement(new juce::XmlElement(*child));
            found = true;
            break;
        }
    if (!found)
    {
        auto* assignment = bank.createNewChildElement("Assignment");
        assignment->setAttribute("sequence", juce::String(
            audioProcessor.getAssignmentForKey(key).sequenceToString()));
    }
    auto name = juce::String(audioProcessor.getAssignmentForKey(key).name)
        .retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_ ")
        .trim().replaceCharacter(' ', '-');
    if (name.isEmpty()) name = "Pattern-" + juce::String(key);
    auto directory = presetDirectory().getParentDirectory().getChildFile("Patterns");
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
            const bool saved = writeXmlFile(chooser.getResult().withFileExtension(".nseqpattern"), bank);
            if (!saved) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                "Pattern bank", "Could not save pattern.");
        });
}

void NSeqArpKeysAudioProcessorEditor::loadPatternFromBank()
{
    auto directory = presetDirectory().getParentDirectory().getChildFile("Patterns");
    if (!directory.isDirectory()) return;
    presetChooser = std::make_unique<juce::FileChooser>("Load pattern from bank",
        directory, "*.nseqpattern");
    auto safeThis = juce::Component::SafePointer<NSeqArpKeysAudioProcessorEditor>(this);
    presetChooser->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr || chooser.getResult() == juce::File()) return;
            if (chooser.getResult().getSize() > 1024 * 1024) return;
            auto bank = juce::XmlDocument::parse(chooser.getResult());
            if (bank == nullptr || !bank->hasTagName("NSeqPattern")
                || bank->getIntAttribute("version") != 1
                || bank->getNumChildElements() != 1
                || bank->getFirstChildElement() == nullptr
                || !bank->getFirstChildElement()->hasTagName("Assignment"))
                return;
            auto state = juce::XmlDocument::parse(safeThis->captureStateXml());
            if (state == nullptr) return;
            const int key = safeThis->audioProcessor.getSelectedKey();
            for (int i = state->getNumChildElements() - 1; i >= 0; --i)
            {
                auto* child = state->getChildElement(i);
                if (child->hasTagName("Assignment") && child->getIntAttribute("key", -1) == key)
                    state->removeChildElement(child, true);
            }
            auto* assignment = new juce::XmlElement(*bank->getFirstChildElement());
            assignment->setAttribute("key", key);
            state->addChildElement(assignment);
            juce::MemoryBlock data;
            juce::AudioProcessor::copyXmlToBinary(*state, data);
            safeThis->audioProcessor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
            safeThis->loadAssignmentForKey(key);
        });
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
    auto state = juce::XmlDocument::parse(xml);
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
            if (file.getSize() > 1024 * 1024)
                continue;
            auto xml = juce::XmlDocument::parse(file);
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

        auto favourites = juce::XmlDocument::parse(directory.getChildFile("favourites.xml"));
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
    return static_cast<int>(filteredPresets.size());
}

void NSeqArpKeysAudioProcessorEditor::paintListBoxItem(int row, juce::Graphics& g,
                                                        int width, int height,
                                                        bool rowIsSelected)
{
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
    browserOpen = open;
    browsePresetsButton.setButtonText(open ? "Back to Editor" : "Browse Presets");
    for (juce::Component* component : std::initializer_list<juce::Component*> { &keyboardComponent, &selectedKeyLabel,
                                        &latchButton, &stopKeyButton, &stopAllButton,
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
                                        &velocityStepsLabel, &velocityStepsEditor, &pitchStepsLabel, &pitchStepsEditor,
                                        &drumNotesLabel, &drumNotesEditor,
                                        &copyPatternButton, &pastePatternButton, &duplicatePatternButton,
                                        &undoButton, &redoButton,
                                        &savePatternButton, &loadPatternButton,
                                        &rangeLabel, &rangeFirstSlider, &rangeLastSlider,
                                        &transposeRangeButton, &applyRangeButton })
        component->setVisible(!open);
    for (juce::Component* component : std::initializer_list<juce::Component*> {
                                        &euclidLabel, &euclidHitsSlider,
                                        &euclidStepsSlider, &euclidButton })
        component->setVisible(!open);
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
        component->setVisible(open);
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
    // Host automation may update the global parameter without restoring state.
    if (!meterNumeratorSlider.isMouseButtonDown())
        meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get(), juce::dontSendNotification);
    if (!meterDenominatorSlider.isMouseButtonDown())
        meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get(), juce::dontSendNotification);
    const auto revision = audioProcessor.getStateRestoreRevision();
    if (revision != lastStateRestoreRevision)
    {
        lastStateRestoreRevision = revision;
        meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get(), juce::dontSendNotification);
        meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get(), juce::dontSendNotification);
        latchButton.setToggleState(audioProcessor.isLatchEnabled(), juce::dontSendNotification);
        loadAssignmentForKey(audioProcessor.getSelectedKey());
    }
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
    auto state = juce::XmlDocument::parse(preset.stateXml);
    if (!validPresetState(state.get()))
    {
        showPresetStatus("Preset data is invalid.", true);
        return;
    }
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary(*state, data);
    audioProcessor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    audioProcessor.setCurrentPresetId(preset.id);
    lastStateRestoreRevision = audioProcessor.getStateRestoreRevision();
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
    auto state = juce::XmlDocument::parse(preset.stateXml);
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
            if (file.getSize() > 1024 * 1024)
            {
                safeThis->showPresetStatus("Preset file is too large.", true);
                return;
            }
            auto xml = juce::XmlDocument::parse(file);
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
            auto state = juce::XmlDocument::parse(preset.stateXml);
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
