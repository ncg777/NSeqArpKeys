#pragma once

#include <JuceHeader.h>
#include <array>

#include "Domain/KeyAssignment.h"
#include "Engine/PatternScheduler.h"

//==============================================================================
class NSeqArpKeysAudioProcessor : public juce::AudioProcessor
{
public:
    NSeqArpKeysAudioProcessor();
    ~NSeqArpKeysAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // -----------------------------------------------------------------------
    // Global parameters (host-automatable)
    // -----------------------------------------------------------------------
    juce::AudioParameterInt* getMeterNumerator()   const { return meterNumerator; }
    juce::AudioParameterInt* getMeterDenominator() const { return meterDenominator; }

    // -----------------------------------------------------------------------
    // Selected-key management
    // -----------------------------------------------------------------------
    int  getSelectedKey() const;
    void setSelectedKey(int key);

    // -----------------------------------------------------------------------
    // Per-key assignment API used by the editor
    // -----------------------------------------------------------------------
    const KeyAssignment& getAssignmentForKey(int key) const;

    void setPatternForKey (int key, const std::string& text);
    void setForteForKey   (int key, const std::string& forteStr);
    void setChannelForKey (int key, int channel);
    void setOctaveForKey  (int key, int octave);
    void setGateForKey    (int key, float gate);

private:
    // -----------------------------------------------------------------------
    // Global parameters
    // -----------------------------------------------------------------------
    juce::AudioParameterInt* meterNumerator;
    juce::AudioParameterInt* meterDenominator;

    // -----------------------------------------------------------------------
    // Domain state – one KeyAssignment per MIDI note (0–127)
    // -----------------------------------------------------------------------
    std::array<KeyAssignment, 128> m_assignments;

    // -----------------------------------------------------------------------
    // Runtime playback
    // -----------------------------------------------------------------------
    PatternScheduler m_scheduler;

    // -----------------------------------------------------------------------
    // UI state (not host-automatable)
    // -----------------------------------------------------------------------
    int m_selectedKey = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NSeqArpKeysAudioProcessor)
};
