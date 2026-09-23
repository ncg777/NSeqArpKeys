#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr double previewVoiceTailOffDecay = 0.99;

class PreviewSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override      { return true; }
    bool appliesToChannel(int) override   { return true; }
};

class PreviewVoice final : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<PreviewSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        const auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        const auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample,
                         int numSamples) override
    {
        if (angleDelta == 0.0)
            return;

        if (tailOff > 0.0)
        {
            while (--numSamples >= 0)
            {
                const auto currentSample = static_cast<float>(std::sin(currentAngle) * level * tailOff);

                for (auto channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
                    outputBuffer.addSample(channel, startSample, currentSample);

                currentAngle += angleDelta;
                ++startSample;
                tailOff *= previewVoiceTailOffDecay;

                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    angleDelta = 0.0;
                    break;
                }
            }

            return;
        }

        while (--numSamples >= 0)
        {
            const auto currentSample = static_cast<float>(std::sin(currentAngle) * level);

            for (auto channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
                outputBuffer.addSample(channel, startSample, currentSample);

            currentAngle += angleDelta;
            ++startSample;
        }
    }

private:
    double currentAngle = 0.0;
    double angleDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;
};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NSeqArpKeysAudioProcessor();
}

//==============================================================================
NSeqArpKeysAudioProcessor::NSeqArpKeysAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize Pcs12 static maps (idempotent if already populated).
    Pcs12::GenerateMaps();

    addParameter(meterNumerator   = new juce::AudioParameterInt("meterNumerator",   "Meter Numerator",   1, 16, 4));
    addParameter(meterDenominator = new juce::AudioParameterInt("meterDenominator", "Meter Denominator", 1, 16, 4));

    initialisePreviewSynth();
}

NSeqArpKeysAudioProcessor::~NSeqArpKeysAudioProcessor() {}

//==============================================================================
void NSeqArpKeysAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    m_scheduler.prepare(sampleRate);
    m_previewSynth.setCurrentPlaybackSampleRate(sampleRate);
    m_heldTriggerKeys.fill(false);
    m_latchWasEnabled = m_latchEnabled.load();
}

void NSeqArpKeysAudioProcessor::releaseResources()
{
    juce::MidiBuffer empty;
    m_scheduler.stopAll(empty);
    m_previewSynth.allNotesOff(0, false);

    const juce::ScopedLock lock(m_pendingPreviewMidiLock);
    m_pendingPreviewMidi.clear();
    m_pendingStopKeys.clear();
    m_pendingStopAll = false;
    m_heldTriggerKeys.fill(false);
}

bool NSeqArpKeysAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // This processor is exported as a stereo-output instrument and deliberately
    // has no audio input bus, so the runtime layout must mirror that contract.
    return layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void NSeqArpKeysAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    std::set<int> stopKeys;
    bool stopAllRequested = false;
    {
        const juce::ScopedLock lock(m_pendingPreviewMidiLock);

        for (const auto& metadata : m_pendingPreviewMidi)
            midiMessages.addEvent(metadata.getMessage(),
                                  juce::jlimit(0, juce::jmax(0, buffer.getNumSamples() - 1), metadata.samplePosition));

        m_pendingPreviewMidi.clear();
        m_nextPendingPreviewSamplePosition = 0;
        stopKeys.swap(m_pendingStopKeys);
        stopAllRequested = m_pendingStopAll;
        m_pendingStopAll = false;
    }

    auto* playHead = getPlayHead();
    double bpm = 120.0;
    if (playHead != nullptr)
        if (auto pos = playHead->getPosition())
            bpm = pos->getBpm().orFallback(120.0);

    int numerator   = meterNumerator->get();
    int denominator = meterDenominator->get();

    struct TriggerEvent { int sample; int note; bool noteOn; };
    std::vector<TriggerEvent> triggers;
    for (const auto& metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            triggers.push_back({ metadata.samplePosition, msg.getNoteNumber(), true });
        else if (msg.isNoteOff())
            triggers.push_back({ metadata.samplePosition, msg.getNoteNumber(), false });
    }

    midiMessages.clear();

    auto appendAt = [&](const juce::MidiBuffer& events, int offset)
    {
        for (const auto& metadata : events)
            midiMessages.addEvent(metadata.getMessage(), offset + metadata.samplePosition);
    };

    auto emitStopKey = [&](int key, int offset)
    {
        juce::MidiBuffer events;
        m_scheduler.stopKey(key, events);
        appendAt(events, offset);
    };

    auto emitTrigger = [&](int key, int offset, const KeyAssignment& assignment)
    {
        if (assignment.sequence.empty())
            return;
        juce::MidiBuffer events;
        m_scheduler.triggerKey(key, assignment, bpm, numerator, denominator, events);
        appendAt(events, offset);
    };

    std::vector<std::pair<int, KeyAssignment>> updates;
    {
        const juce::ScopedLock lock(m_assignmentsLock);
        for (int key : m_pendingAssignmentUpdates)
            updates.emplace_back(key, m_assignments[static_cast<size_t>(key)]);
        m_pendingAssignmentUpdates.clear();
    }
    for (const auto& [key, assignment] : updates)
        if (m_scheduler.isKeyActive(key))
        {
            if (assignment.sequence.empty())
                emitStopKey(key, 0);
            else
                emitTrigger(key, 0, assignment);
        }

    const bool latch = m_latchEnabled.load();
    if (m_latchWasEnabled && !latch)
        for (int key = 0; key < 128; ++key)
            if (!m_heldTriggerKeys[static_cast<size_t>(key)])
                emitStopKey(key, 0);
    m_latchWasEnabled = latch;

    if (stopAllRequested)
    {
        juce::MidiBuffer events;
        m_scheduler.stopAll(events);
        appendAt(events, 0);
    }
    else
        for (int key : stopKeys)
            emitStopKey(key, 0);

    int cursor = 0;
    auto processUntil = [&](int end)
    {
        if (end <= cursor)
            return;
        juce::MidiBuffer events;
        m_scheduler.processBlock(events, end - cursor, bpm, numerator, denominator);
        appendAt(events, cursor);
        cursor = end;
    };

    for (const auto& event : triggers)
    {
        const int position = juce::jlimit(0, juce::jmax(0, buffer.getNumSamples() - 1), event.sample);
        processUntil(position);

        if (event.noteOn)
        {
            const bool wasHeld = m_heldTriggerKeys[static_cast<size_t>(event.note)];
            m_heldTriggerKeys[static_cast<size_t>(event.note)] = true;

            if (latch && !wasHeld && m_scheduler.isKeyActive(event.note))
            {
                emitStopKey(event.note, position);
            }
            else
            {
                KeyAssignment assignment;
                {
                    const juce::ScopedLock lock(m_assignmentsLock);
                    assignment = m_assignments[static_cast<size_t>(event.note)];
                }
                emitTrigger(event.note, position, assignment);
            }
        }
        else
        {
            m_heldTriggerKeys[static_cast<size_t>(event.note)] = false;
            if (!latch)
                emitStopKey(event.note, position);
        }
    }

    processUntil(buffer.getNumSamples());
    m_previewSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool NSeqArpKeysAudioProcessor::hasEditor() const  { return true; }
const juce::String NSeqArpKeysAudioProcessor::getName() const { return JucePlugin_Name; }
bool NSeqArpKeysAudioProcessor::acceptsMidi()  const { return true; }
bool NSeqArpKeysAudioProcessor::producesMidi() const { return true; }
bool NSeqArpKeysAudioProcessor::isMidiEffect() const { return false; }
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
    state->setAttribute("latchEnabled", isLatchEnabled());

    const juce::ScopedLock lock(m_assignmentsLock);
    state->setAttribute("currentPresetId", m_currentPresetId);
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

    m_selectedKey = juce::jlimit(0, 127, state->getIntAttribute("selectedKey", 60));

    if (auto* p = dynamic_cast<juce::AudioParameterInt*>(getParameters()[0]))
        p->setValueNotifyingHost(p->convertTo0to1(state->getIntAttribute("meterNumerator", 4)));
    if (auto* p = dynamic_cast<juce::AudioParameterInt*>(getParameters()[1]))
        p->setValueNotifyingHost(p->convertTo0to1(state->getIntAttribute("meterDenominator", 4)));

    setLatchEnabled(state->getBoolAttribute("latchEnabled", false));
    requestStopAll();
    const juce::ScopedLock lock(m_assignmentsLock);
    m_assignments.fill(KeyAssignment{});
    m_pendingAssignmentUpdates.clear();
    m_currentPresetId = state->getStringAttribute("currentPresetId");
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
        m_pendingAssignmentUpdates.insert(k);
    }
    m_stateRestoreRevision.fetch_add(1);
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

KeyAssignment NSeqArpKeysAudioProcessor::getAssignmentForKey(int key) const
{
    jassert(key >= 0 && key < 128);
    const juce::ScopedLock lock(m_assignmentsLock);
    return m_assignments[static_cast<size_t>(key)];
}

void NSeqArpKeysAudioProcessor::setPatternForKey(int key, const std::string& text)
{
    if (key >= 0 && key < 128)
    {
        const juce::ScopedLock lock(m_assignmentsLock);
        m_assignments[static_cast<size_t>(key)].setSequenceFromString(text);
        m_pendingAssignmentUpdates.insert(key);
    }
}

void NSeqArpKeysAudioProcessor::setForteForKey(int key, const std::string& forteStr)
{
    if (key >= 0 && key < 128)
    {
        const juce::ScopedLock lock(m_assignmentsLock);
        m_assignments[static_cast<size_t>(key)].setForteFromString(forteStr);
        m_pendingAssignmentUpdates.insert(key);
    }
}

void NSeqArpKeysAudioProcessor::setChannelForKey(int key, int channel)
{
    if (key >= 0 && key < 128)
    {
        const juce::ScopedLock lock(m_assignmentsLock);
        m_assignments[static_cast<size_t>(key)].channel = juce::jlimit(1, 16, channel);
        m_pendingAssignmentUpdates.insert(key);
    }
}

void NSeqArpKeysAudioProcessor::setOctaveForKey(int key, int octave)
{
    if (key >= 0 && key < 128)
    {
        const juce::ScopedLock lock(m_assignmentsLock);
        m_assignments[static_cast<size_t>(key)].octave = juce::jlimit(0, 10, octave);
        m_pendingAssignmentUpdates.insert(key);
    }
}

void NSeqArpKeysAudioProcessor::setGateForKey(int key, float gate)
{
    if (key >= 0 && key < 128)
    {
        const juce::ScopedLock lock(m_assignmentsLock);
        m_assignments[static_cast<size_t>(key)].gate = juce::jlimit(0.0f, 1.0f, gate);
        m_pendingAssignmentUpdates.insert(key);
    }
}

void NSeqArpKeysAudioProcessor::queuePreviewMidiMessage(const juce::MidiMessage& message)
{
    const juce::ScopedLock lock(m_pendingPreviewMidiLock);
    m_pendingPreviewMidi.addEvent(message, m_nextPendingPreviewSamplePosition++);
}

void NSeqArpKeysAudioProcessor::requestStopKey(int key)
{
    if (key < 0 || key >= 128)
        return;
    const juce::ScopedLock lock(m_pendingPreviewMidiLock);
    juce::MidiBuffer remaining;
    for (const auto& metadata : m_pendingPreviewMidi)
        if (metadata.getMessage().getNoteNumber() != key)
            remaining.addEvent(metadata.getMessage(), metadata.samplePosition);
    m_pendingPreviewMidi.swapWith(remaining);
    m_pendingStopKeys.insert(key);
}

void NSeqArpKeysAudioProcessor::requestStopAll()
{
    const juce::ScopedLock lock(m_pendingPreviewMidiLock);
    m_pendingPreviewMidi.clear();
    m_nextPendingPreviewSamplePosition = 0;
    m_pendingStopAll = true;
    m_pendingStopKeys.clear();
}

void NSeqArpKeysAudioProcessor::setLatchEnabled(bool enabled)
{
    m_latchEnabled.store(enabled);
}

bool NSeqArpKeysAudioProcessor::isLatchEnabled() const
{
    return m_latchEnabled.load();
}

juce::String NSeqArpKeysAudioProcessor::getCurrentPresetId() const
{
    const juce::ScopedLock lock(m_assignmentsLock);
    return m_currentPresetId;
}

void NSeqArpKeysAudioProcessor::setCurrentPresetId(const juce::String& id)
{
    const juce::ScopedLock lock(m_assignmentsLock);
    m_currentPresetId = id;
}

void NSeqArpKeysAudioProcessor::initialisePreviewSynth()
{
    m_previewSynth.clearVoices();

    for (int i = 0; i < 16; ++i)
        m_previewSynth.addVoice(new PreviewVoice());

    m_previewSynth.clearSounds();
    m_previewSynth.addSound(new PreviewSound());
}

juce::AudioProcessorEditor* NSeqArpKeysAudioProcessor::createEditor()
{
    return new NSeqArpKeysAudioProcessorEditor(*this);
}
