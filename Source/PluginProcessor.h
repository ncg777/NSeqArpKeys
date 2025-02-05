#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <JuceHeader.h>
#include <unordered_map>
#include <vector>
#include "Pcs12.h"

//==============================================================================
/**
*/
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

    // Public getter methods for parameters
    juce::AudioParameterInt* getMeterNumerator() const { return meterNumerator; }
    juce::AudioParameterInt* getMeterDenominator() const { return meterDenominator; }
    juce::AudioParameterInt* getOctaveNumber() const { return octaveNumber; }
    juce::AudioParameterFloat* getGateParameter() const { return gateParameter; }
    juce::AudioParameterInt* getChannelParameter() const { return channelParameter; }
    juce::AudioParameterInt* getSelectedKeyParameter() const { return selectedKeyParameter; }

    std::unordered_map<int, std::vector<int>>& getKeyPatterns() { return keyPatterns; }
    std::unordered_map<int, int>& getKeyChannels() { return keyChannels; }
    std::unordered_map<int, Pcs12>& getKeyForteNumbers() { return keyForteNumbers; }
    std::vector<int> parsePattern(const juce::String& pattern);
private:
    // Parameters
    juce::AudioParameterInt* meterNumerator;
    juce::AudioParameterInt* meterDenominator;
    juce::AudioParameterInt* octaveNumber;
    juce::AudioParameterFloat* gateParameter;
    juce::AudioParameterInt* channelParameter;
    juce::AudioParameterInt* selectedKeyParameter;

    // Pattern logic
    std::unordered_map<int, std::vector<int>> keyPatterns;
    std::unordered_map<int, int> keyChannels;
    std::unordered_map<int, Pcs12> keyForteNumbers;
    std::vector<int> getBinaryIndices(int number);
    void processPattern(juce::MidiBuffer& midiMessages, int key, double bpm);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NSeqArpKeysAudioProcessor)
};
