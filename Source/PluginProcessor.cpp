#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NSeqArpKeysAudioProcessor();
}

//==============================================================================
NSeqArpKeysAudioProcessor::NSeqArpKeysAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize Pcs12 static maps (idempotent if already populated).
    Pcs12::GenerateMaps();

    addParameter(meterNumerator   = new juce::AudioParameterInt("meterNumerator",   "Meter Numerator",   1, 16, 4));
    addParameter(meterDenominator = new juce::AudioParameterInt("meterDenominator", "Meter Denominator", 1, 16, 4));
}

NSeqArpKeysAudioProcessor::~NSeqArpKeysAudioProcessor() {}

//==============================================================================
void NSeqArpKeysAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    m_scheduler.prepare(sampleRate);
}

void NSeqArpKeysAudioProcessor::releaseResources()
{
    juce::MidiBuffer empty;
    m_scheduler.stopAll(empty);
}

bool NSeqArpKeysAudioProcessor::isBusesLayoutSupported(const BusesLayout& /*layouts*/) const
{
    return true;
}

//==============================================================================
void NSeqArpKeysAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    auto* playHead = getPlayHead();
    double bpm = 120.0;
    if (playHead != nullptr)
        if (auto pos = playHead->getPosition())
            bpm = pos->getBpm().orFallback(120.0);

    int numerator   = meterNumerator->get();
    int denominator = meterDenominator->get();

    // Collect incoming trigger notes before we clear the buffer.
    std::vector<std::pair<int, bool>> triggers; // (noteNumber, isNoteOn)
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            triggers.push_back({ msg.getNoteNumber(), true });
        else if (msg.isNoteOff())
            triggers.push_back({ msg.getNoteNumber(), false });
    }

    // Output only scheduled pattern notes (consume the trigger MIDI).
    midiMessages.clear();

    // Apply note-on triggers to the scheduler.
    // Note-offs are ignored in latch mode (patterns run until retriggered).
    for (auto [note, isOn] : triggers)
    {
        if (isOn)
        {
            const auto& assignment = m_assignments[note];
            if (!assignment.sequence.empty())
                m_scheduler.triggerKey(note, assignment, bpm, numerator, denominator, midiMessages);
        }
    }

    // Let the scheduler generate pattern MIDI for this block.
    m_scheduler.processBlock(midiMessages, buffer.getNumSamples(), bpm, numerator, denominator);
}

//==============================================================================
bool NSeqArpKeysAudioProcessor::hasEditor() const  { return true; }
const juce::String NSeqArpKeysAudioProcessor::getName() const { return JucePlugin_Name; }
bool NSeqArpKeysAudioProcessor::acceptsMidi()  const { return true; }
bool NSeqArpKeysAudioProcessor::producesMidi() const { return true; }
bool NSeqArpKeysAudioProcessor::isMidiEffect() const { return true; }
double NSeqArpKeysAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int NSeqArpKeysAudioProcessor::getNumPrograms()  { return 1; }
int NSeqArpKeysAudioProcessor::getCurrentProgram() { return 0; }
void NSeqArpKeysAudioProcessor::setCurrentProgram(int) {}
const juce::String NSeqArpKeysAudioProcessor::getProgramName(int) { return {}; }
void NSeqArpKeysAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
// State persistence – save / restore all 128 key assignments using JUCE XML.
//==============================================================================
void NSeqArpKeysAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = std::make_unique<juce::XmlElement>("NSeqArpKeys");
    state->setAttribute("selectedKey",      m_selectedKey);
    state->setAttribute("meterNumerator",   meterNumerator->get());
    state->setAttribute("meterDenominator", meterDenominator->get());

    for (int k = 0; k < 128; ++k)
    {
        const auto& a = m_assignments[k];

        // Only persist keys that differ from the default state.
        bool isDefault = (a.forteString.empty()
                         && a.channel == 1
                         && a.octave  == 4
                         && a.gate    == 0.5f
                         && a.lengthFactor == 1.0f);
        // Check if all sequence values are zero.
        bool seqAllZero = true;
        for (int v : a.sequence) if (v != 0) { seqAllZero = false; break; }
        if (isDefault && seqAllZero)
            continue;

        auto* child = state->createNewChildElement("Assignment");
        child->setAttribute("key",          k);
        child->setAttribute("sequence",     a.sequenceToString());
        child->setAttribute("forte",        a.forteString);
        child->setAttribute("channel",      a.channel);
        child->setAttribute("octave",       a.octave);
        child->setAttribute("gate",         static_cast<double>(a.gate));
        child->setAttribute("lengthFactor", static_cast<double>(a.lengthFactor));
    }

    juce::AudioProcessor::copyXmlToBinary(*state, destData);
}

void NSeqArpKeysAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto state = juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes);
    if (state == nullptr || !state->hasTagName("NSeqArpKeys"))
        return;

    m_selectedKey = state->getIntAttribute("selectedKey", 60);

    if (auto* p = dynamic_cast<juce::AudioParameterInt*>(getParameters()[0]))
        p->setValueNotifyingHost(p->convertTo0to1(state->getIntAttribute("meterNumerator", 4)));
    if (auto* p = dynamic_cast<juce::AudioParameterInt*>(getParameters()[1]))
        p->setValueNotifyingHost(p->convertTo0to1(state->getIntAttribute("meterDenominator", 4)));

    for (auto* child : state->getChildIterator())
    {
        if (!child->hasTagName("Assignment"))
            continue;

        int k = child->getIntAttribute("key", -1);
        if (k < 0 || k >= 128)
            continue;

        auto& a = m_assignments[k];
        a.setSequenceFromString(child->getStringAttribute("sequence").toStdString());
        a.setForteFromString   (child->getStringAttribute("forte").toStdString());
        a.channel      = child->getIntAttribute   ("channel",      1);
        a.octave       = child->getIntAttribute   ("octave",       4);
        a.gate         = static_cast<float>(child->getDoubleAttribute("gate",         0.5));
        a.lengthFactor = static_cast<float>(child->getDoubleAttribute("lengthFactor", 1.0));
    }
}

//==============================================================================
// Per-key assignment API
//==============================================================================
int NSeqArpKeysAudioProcessor::getSelectedKey() const
{
    return m_selectedKey;
}

void NSeqArpKeysAudioProcessor::setSelectedKey(int key)
{
    if (key >= 0 && key < 128)
        m_selectedKey = key;
}

const KeyAssignment& NSeqArpKeysAudioProcessor::getAssignmentForKey(int key) const
{
    jassert(key >= 0 && key < 128);
    return m_assignments[static_cast<size_t>(key)];
}

void NSeqArpKeysAudioProcessor::setPatternForKey(int key, const std::string& text)
{
    if (key >= 0 && key < 128)
        m_assignments[static_cast<size_t>(key)].setSequenceFromString(text);
}

void NSeqArpKeysAudioProcessor::setForteForKey(int key, const std::string& forteStr)
{
    if (key >= 0 && key < 128)
        m_assignments[static_cast<size_t>(key)].setForteFromString(forteStr);
}

void NSeqArpKeysAudioProcessor::setChannelForKey(int key, int channel)
{
    if (key >= 0 && key < 128)
        m_assignments[static_cast<size_t>(key)].channel = juce::jlimit(1, 16, channel);
}

void NSeqArpKeysAudioProcessor::setOctaveForKey(int key, int octave)
{
    if (key >= 0 && key < 128)
        m_assignments[static_cast<size_t>(key)].octave = juce::jlimit(0, 10, octave);
}

void NSeqArpKeysAudioProcessor::setGateForKey(int key, float gate)
{
    if (key >= 0 && key < 128)
        m_assignments[static_cast<size_t>(key)].gate = juce::jlimit(0.0f, 1.0f, gate);
}

juce::AudioProcessorEditor* NSeqArpKeysAudioProcessor::createEditor()
{
    return new NSeqArpKeysAudioProcessorEditor(*this);
}
