#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <set>

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
    KeyAssignment getAssignmentForKey(int key) const;
    void setAssignmentForKey(int key, const KeyAssignment& assignment);
    void copyAssignmentToRange(int sourceKey, int first, int last, bool transpose);

    void setPatternForKey (int key, const std::string& text);
    void setForteForKey   (int key, const std::string& forteStr);
    void setChannelForKey (int key, int channel);
    void setOctaveForKey  (int key, int octave);
    void setGateForKey    (int key, float gate);
    void setFixedLengthStepsForKey(int key, float steps);
    void queuePreviewMidiMessage(const juce::MidiMessage& message);
    void requestStopKey(int key);
    void requestStopAll();
    void setLatchEnabled(bool enabled);
    bool isLatchEnabled() const;
    void setPreviewSoundEnabled(bool enabled) { m_previewSoundEnabled.store(enabled); }
    bool isPreviewSoundEnabled() const { return m_previewSoundEnabled.load(); }
    juce::String getCurrentPresetId() const;
    void setCurrentPresetId(const juce::String& id);
    uint64_t getStateRestoreRevision() const { return m_stateRestoreRevision.load(); }

private:
    void initialisePreviewSynth();

    // -----------------------------------------------------------------------
    // Global parameters
    // -----------------------------------------------------------------------
    juce::AudioParameterInt* meterNumerator;
    juce::AudioParameterInt* meterDenominator;

    // -----------------------------------------------------------------------
    // Domain state – one KeyAssignment per MIDI note (0–127)
    // -----------------------------------------------------------------------
    std::array<KeyAssignment, 128> m_assignments;
    mutable juce::CriticalSection m_assignmentsLock;
    std::set<int> m_pendingAssignmentUpdates;
    std::set<int> m_pendingTimingUpdates;
    juce::String m_currentPresetId;
    std::atomic<uint64_t> m_stateRestoreRevision { 0 };

    // -----------------------------------------------------------------------
    // Runtime playback
    // -----------------------------------------------------------------------
    PatternScheduler m_scheduler;
    juce::Synthesiser m_previewSynth;
    std::atomic<bool> m_previewSoundEnabled { true };
    juce::MidiBuffer m_pendingPreviewMidi;
    juce::CriticalSection m_pendingPreviewMidiLock;
    int m_nextPendingPreviewSamplePosition = 0;
    std::set<int> m_pendingStopKeys;
    bool m_pendingStopAll = false;
    std::atomic<bool> m_latchEnabled { false };
    bool m_latchWasEnabled = false;
    std::array<bool, 128> m_heldTriggerKeys {};
    std::array<int, 128> m_triggerVelocities {};

    // -----------------------------------------------------------------------
    // UI state (not host-automatable)
    // -----------------------------------------------------------------------
    std::atomic<int> m_selectedKey { 60 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NSeqArpKeysAudioProcessor)
};
