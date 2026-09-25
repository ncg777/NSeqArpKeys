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
    void mouseDown(const juce::MouseEvent&) override;

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
    void updateModeVisibility();
    void updatePatternPreview();
    void setPatternBrowserOpen(bool open);
    void loadPatternLibrary();
    void filterPatternLibrary();
    void assignSelectedPattern(bool linked);
    void makeSelectedIndependent();
    void saveSelectedPatternMetadata();
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
    juce::LookAndFeel_V4 theme;
    juce::TooltipWindow tooltipWindow { this, 500 };

    juce::MidiKeyboardState     keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;

    // Global parameters
    juce::Slider meterNumeratorSlider;
    juce::Slider meterDenominatorSlider;
    juce::ToggleButton latchButton;
    juce::ToggleButton previewSoundButton;
    juce::TextButton stopKeyButton;
    juce::TextButton stopAllButton;
    juce::Label presetNameLabel;
    juce::TextButton previousPresetButton, nextPresetButton;
    juce::TextButton browsePresetsButton, savePresetButton;
    juce::TextButton browsePatternsButton;

    // Per-key assignment controls
    juce::Slider       channelSlider;
    juce::Slider       octaveSlider;
    juce::Slider       gateSlider;
    juce::Slider       fixedLengthStepsSlider;
    juce::TextEditor   patternTextEditor;
    juce::TextEditor   patternNameEditor, drumVelocityBitsEditor, drumNotesEditor;
    juce::ComboBox     modeSelector;
    juce::Slider       subdivisionSlider, velocitySlider, transposeSlider, rotationSlider;
    juce::ToggleButton reverseButton;
    juce::TextButton copyPatternButton, pastePatternButton, duplicatePatternButton;
    juce::TextButton cutPatternButton, clearPatternButton, independentButton;
    juce::ComboBox pasteScopeSelector, patternColourSelector;
    juce::TextEditor patternTagsEditor;
    juce::ToggleButton patternFavouriteButton;
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
    juce::Label transposeLabel, rotationLabel, drumVelocityBitsLabel;
    juce::Label drumNotesLabel, rangeLabel;
    juce::Label patternTagsLabel, patternColourLabel, patternPreviewLabel;
    juce::Label forteLabel;
    juce::Label forteSearchLabel;
    juce::Label forteSelectionLabel;
    juce::Label selectedKeyLabel;
    juce::Label activeKeysLabel;

    std::vector<PresetEntry> presets;
    std::vector<int> filteredPresets;
    int selectedPresetIndex = -1;
    bool browserOpen = false;
    bool patternBrowserOpen = false;
    struct PatternEntry { juce::String id; juce::File file; KeyAssignment assignment; };
    std::vector<PatternEntry> patternLibrary;
    std::vector<int> filteredPatterns;
    int selectedPatternIndex = -1;
    int auditioningKey = -1;
    juce::TextEditor patternSearchEditor, bankNameEditor, bankTagsEditor;
    juce::ComboBox bankColourSelector;
    juce::Label patternSearchLabel, bankDetailsLabel, bankTagsLabel;
    juce::Label bankEmptyLabel, bankHelpLabel;
    juce::ToggleButton bankFavouritesButton;
    juce::ToggleButton bankFavouriteButton;
    juce::TextButton assignCopyButton, assignLinkButton, auditionButton, bankSaveButton;
    juce::TextButton saveCurrentToBankButton;
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
