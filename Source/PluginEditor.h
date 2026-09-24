#pragma once

#include <JuceHeader.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Domain/AssignmentHistory.h"

//==============================================================================
class NSeqArpKeysAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::MidiKeyboardStateListener,
                                         private juce::ListBoxModel,
                                         private juce::Timer
{
public:
    explicit NSeqArpKeysAudioProcessorEditor(NSeqArpKeysAudioProcessor&);
    ~NSeqArpKeysAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // MidiKeyboardStateListener – key selection
    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;

    // Refresh the UI controls to show the assignment for the given key.
    void loadAssignmentForKey(int key);
    void savePatternToBank();
    void loadPatternFromBank();
    void applyRange();
    void recordKeyEdit();
    void undoKeyEdit();
    void redoKeyEdit();
    void syncEditHistory();
    void updateTimingDisplay();
    bool validateIntegerInput(juce::TextEditor&, std::vector<int>&, int minimum, int maximum,
                              int exactCount = -1);
    void updateForteSearchResults();
    void updateSelectedForteLabel();
    void timerCallback() override;

    struct PresetEntry
    {
        juce::String id, name, category, tags, description, stateXml;
        juce::File file;
        bool factory = false;
        bool favourite = false;
    };

    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height,
                          bool rowIsSelected) override;
    void selectedRowsChanged(int row) override;
    void loadPresetLibrary();
    void refreshPresetFilters();
    void refreshPresetCategories();
    void updatePresetDisplay();
    void setBrowserOpen(bool open);
    void loadPreset(int index);
    void saveNewPreset();
    void updateSelectedPreset();
    void duplicateSelectedPreset();
    void deleteSelectedPreset();
    void toggleSelectedFavourite();
    void importPreset();
    void exportSelectedPreset();
    void selectPresetById(const juce::String& id);
    void navigatePreset(int direction);
    juce::String captureStateXml() const;
    juce::String normalisedStateXml(const juce::String& xml) const;
    bool writePreset(PresetEntry& preset);
    void saveFavourites() const;
    void showPresetStatus(const juce::String& message, bool error = false);

    struct ForteSearchEntry
    {
        juce::String id;
        juce::String display;
    };

    // -------------------------------------------------------------------------
    NSeqArpKeysAudioProcessor& audioProcessor;
    juce::TooltipWindow tooltipWindow { this, 500 };

    juce::MidiKeyboardState     keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;

    // Global parameters
    juce::Slider meterNumeratorSlider;
    juce::Slider meterDenominatorSlider;
    juce::ToggleButton latchButton;
    juce::TextButton stopKeyButton;
    juce::TextButton stopAllButton;
    juce::Label presetNameLabel;
    juce::TextButton previousPresetButton, nextPresetButton;
    juce::TextButton browsePresetsButton, savePresetButton;

    // Per-key assignment controls
    juce::Slider       channelSlider;
    juce::Slider       octaveSlider;
    juce::Slider       gateSlider;
    juce::Slider       fixedLengthStepsSlider;
    juce::TextEditor   patternTextEditor;
    juce::TextEditor   patternNameEditor, velocityStepsEditor, pitchStepsEditor, drumNotesEditor;
    juce::ComboBox     modeSelector;
    juce::Slider       subdivisionSlider, velocitySlider, transposeSlider, rotationSlider;
    juce::ToggleButton reverseButton;
    juce::TextButton copyPatternButton, pastePatternButton, duplicatePatternButton;
    juce::TextButton savePatternButton, loadPatternButton, applyRangeButton;
    juce::TextButton undoButton, redoButton;
    juce::Slider rangeFirstSlider, rangeLastSlider;
    juce::ToggleButton transposeRangeButton;
    std::unique_ptr<KeyAssignment> copiedPattern;
    AssignmentHistory editHistory;
    juce::TextEditor   forteSearchEditor;
    juce::ComboBox     forteNumberSelector;
    std::vector<ForteSearchEntry> forteSearchEntries;
    std::vector<juce::String> visibleForteIds;

    // Labels
    juce::Label meterNumeratorLabel;
    juce::Label meterDenominatorLabel;
    juce::Label channelLabel;
    juce::Label octaveLabel;
    juce::Label gateLabel;
    juce::Label fixedLengthStepsLabel;
    juce::Label patternLabel;
    juce::Label patternNameLabel, modeLabel, subdivisionLabel, velocityLabel;
    juce::Label transposeLabel, rotationLabel, velocityStepsLabel, pitchStepsLabel;
    juce::Label drumNotesLabel, rangeLabel;
    juce::Label forteLabel;
    juce::Label forteSearchLabel;
    juce::Label forteSelectionLabel;
    juce::Label selectedKeyLabel;

    std::vector<PresetEntry> presets;
    std::vector<int> filteredPresets;
    int selectedPresetIndex = -1;
    bool browserOpen = false;
    juce::String loadedPresetSnapshot;
    juce::String lastObservedState;
    juce::String lastDisplayedPresetId;
    uint64_t lastStateRestoreRevision = 0;
    std::unique_ptr<juce::FileChooser> presetChooser;

    juce::TextEditor presetSearchEditor, presetNameEditor, presetCategoryEditor;
    juce::TextEditor presetTagsEditor, presetDescriptionEditor;
    juce::ComboBox presetCategoryFilter;
    juce::ToggleButton favouritesOnlyButton;
    juce::ListBox presetList;
    juce::Label presetDetailsLabel, presetStatusLabel;
    juce::TextButton loadPresetButton, saveNewPresetButton, updatePresetButton;
    juce::TextButton duplicatePresetButton, deletePresetButton, favouritePresetButton;
    juce::TextButton importPresetButton, exportPresetButton;
    juce::Label presetSearchLabel, presetCategoryFilterLabel;
    juce::Label presetNameFieldLabel, presetCategoryFieldLabel, presetTagsFieldLabel;
    juce::Label presetDescriptionFieldLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NSeqArpKeysAudioProcessorEditor)
};
