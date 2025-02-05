#include "PluginProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NSeqArpKeysAudioProcessor();
}
NSeqArpKeysAudioProcessor::NSeqArpKeysAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter(meterNumerator = new juce::AudioParameterInt("meterNumerator", "Meter Numerator", 1, 16, 4));
    addParameter(meterDenominator = new juce::AudioParameterInt("meterDenominator", "Meter Denominator", 1, 16, 4));
    addParameter(octaveNumber = new juce::AudioParameterInt("octaveNumber", "Octave Number", 0, 10, 4));
    addParameter(gateParameter = new juce::AudioParameterFloat("gateParameter", "Gate Parameter", 0.0f, 1.0f, 0.5f));
    addParameter(channelParameter = new juce::AudioParameterInt("channelParameter", "Channel Parameter", 1, 16, 1));
    addParameter(selectedKeyParameter = new juce::AudioParameterInt("selectedKey", "Selected Key", 0, 127, 60)); // Default to middle C

    // Initialize key patterns and channels
    for (int i = 0; i < 128; ++i)
    {
        keyPatterns[i] = std::vector<int>(16, 0); // Default pattern
        keyChannels[i] = 1; // Default channel
        keyForteNumbers[i] = Pcs12::empty(); // Default Forte number
    }
}

NSeqArpKeysAudioProcessor::~NSeqArpKeysAudioProcessor() {}

void NSeqArpKeysAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {}
void NSeqArpKeysAudioProcessor::releaseResources() {}
bool NSeqArpKeysAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { return true; }

void NSeqArpKeysAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    double bpm = getPlayHead()->getPosition()->getBpm().orFallback(120.0);
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            int key = message.getNoteNumber();
            processPattern(midiMessages, key, bpm);
        }
    }
}

bool NSeqArpKeysAudioProcessor::hasEditor() const { return true; }
const juce::String NSeqArpKeysAudioProcessor::getName() const { return JucePlugin_Name; }
bool NSeqArpKeysAudioProcessor::acceptsMidi() const { return true; }
bool NSeqArpKeysAudioProcessor::producesMidi() const { return true; }
bool NSeqArpKeysAudioProcessor::isMidiEffect() const { return true; }
double NSeqArpKeysAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int NSeqArpKeysAudioProcessor::getNumPrograms() { return 1; }
int NSeqArpKeysAudioProcessor::getCurrentProgram() { return 0; }
void NSeqArpKeysAudioProcessor::setCurrentProgram(int index) {}
const juce::String NSeqArpKeysAudioProcessor::getProgramName(int index) { return {}; }
void NSeqArpKeysAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

void NSeqArpKeysAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    stream.writeInt(*selectedKeyParameter);
    // Save other parameters as needed
}

void NSeqArpKeysAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream(data, static_cast<size_t> (sizeInBytes), false);
    selectedKeyParameter->setValueNotifyingHost(stream.readInt());
    // Restore other parameters as needed
}

std::vector<int> NSeqArpKeysAudioProcessor::parsePattern(const juce::String& pattern)
{
    std::vector<int> parsedPattern;
    juce::StringArray tokens = juce::StringArray::fromTokens(pattern, " ", "");
    for (auto& token : tokens)
    {
        int number = token.getIntValue();
        if (number < 0)
        {
            throw std::invalid_argument("Pattern contains a negative number");
        }
        parsedPattern.push_back(number);
    }
    return parsedPattern;
}

std::vector<int> NSeqArpKeysAudioProcessor::getBinaryIndices(int number)
{
    std::vector<int> indices;
    int index = 0;
    while (number > 0)
    {
        if (number % 2 == 1)
        {
            indices.push_back(index);
        }
        number /= 2;
        index++;
    }
    return indices;
}

void NSeqArpKeysAudioProcessor::processPattern(juce::MidiBuffer& midiMessages, int key, double bpm)
{
    auto& pattern = keyPatterns[key];
    auto channel = keyChannels[key];
    auto forteNumber = keyForteNumbers[key];

    if (pattern.empty())
    {
        return;
    }

    double period = (240.0 / bpm) / (meterNumerator->get() * meterDenominator->get());
    int patternLength = static_cast<int>(pattern.size());
    int position = static_cast<int>((getPlayHead()->getPosition()->getPpqPosition().orFallback(0.0) / period)) % patternLength;

    for (int i = 0; i < patternLength; ++i)
    {
        auto indices = getBinaryIndices(pattern[i]);
        for (auto index : indices)
        {
            int noteNumber = key + (index % 12) + (octaveNumber->get() * 12);
            midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)127), i * period);
            midiMessages.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), (i * period) + (period * gateParameter->get()));
        }
    }
}
