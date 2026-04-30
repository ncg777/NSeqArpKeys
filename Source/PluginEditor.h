#pragma once

#include <JuceHeader.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

//==============================================================================
class NSeqArpKeysAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::MidiKeyboardStateListener
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

    // -------------------------------------------------------------------------
    NSeqArpKeysAudioProcessor& audioProcessor;

    juce::MidiKeyboardState     keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;

    // Global parameters
    juce::Slider meterNumeratorSlider;
    juce::Slider meterDenominatorSlider;

    // Per-key assignment controls
    juce::Slider       channelSlider;
    juce::Slider       octaveSlider;
    juce::Slider       gateSlider;
    juce::TextEditor   patternTextEditor;
    juce::ComboBox     forteNumberSelector;

    // Labels
    juce::Label meterNumeratorLabel;
    juce::Label meterDenominatorLabel;
    juce::Label channelLabel;
    juce::Label octaveLabel;
    juce::Label gateLabel;
    juce::Label patternLabel;
    juce::Label forteLabel;
    juce::Label selectedKeyLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NSeqArpKeysAudioProcessorEditor)
};
