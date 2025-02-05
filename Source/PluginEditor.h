#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Add the missing include for MidiKeyboardComponent
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
/**
*/
class NSeqArpKeysAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::MidiKeyboardStateListener
{
public:
    NSeqArpKeysAudioProcessorEditor(NSeqArpKeysAudioProcessor&);
    ~NSeqArpKeysAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;

    NSeqArpKeysAudioProcessor& audioProcessor;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;

    juce::Slider meterNumeratorSlider;
    juce::Slider meterDenominatorSlider;
    juce::Slider octaveNumberSlider;
    juce::Slider gateParameterSlider;
    juce::Slider channelParameterSlider;
    juce::TextEditor patternTextEditor;
    juce::ComboBox forteNumberSelector;

    void updateParametersForKey(int key);
    void updateSelectedKey(int key);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NSeqArpKeysAudioProcessorEditor)
};
