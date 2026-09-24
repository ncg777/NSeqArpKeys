#include "PatternScheduler.h"
#include "GateRunnerEngine.h"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
void PatternScheduler::prepare(double sampleRate)
{
    m_sampleRate = sampleRate;
    m_activePatterns.clear();
    m_outputNoteCounts = {};
}

// ---------------------------------------------------------------------------
double PatternScheduler::computeStepDuration(double bpm, int numerator, int denominator) const
{
    // GateRunner quant = 60 / (bpm * denominator)  [seconds per step]
    double quant = 60.0 / (bpm * static_cast<double>(denominator));
    return quant * m_sampleRate;
}

// ---------------------------------------------------------------------------
void PatternScheduler::triggerKey(int key,
                                  const KeyAssignment& assignment,
                                  double bpm,
                                  int    numerator,
                                  int    denominator,
                                  juce::MidiBuffer& midiMessages,
                                  int triggerVelocity)
{
    if (assignment.sequence.empty())
        return;

    // Send note-offs for any notes still sounding from a previous pattern on
    // this key so we do not leave hanging notes.
    auto existing = m_activePatterns.find(key);
    if (existing != m_activePatterns.end())
    {
        auto& old = existing->second;
        for (int note = 0; note < 128; ++note)
            if (old.activeNoteCounts[static_cast<size_t>(note)] > 0)
            {
                auto& count = m_outputNoteCounts[static_cast<size_t>(old.channel - 1)][static_cast<size_t>(note)];
                count = std::max(0, count - old.activeNoteCounts[static_cast<size_t>(note)]);
                if (count == 0)
                    midiMessages.addEvent(juce::MidiMessage::noteOff(old.channel, note), 0);
            }
    }

    // Precompute step notes via the GateRunner engine.
    ActivePattern pat;
    pat.elapsedSamples = 0.0;
    pat.channel        = assignment.channel;
    pat.subdivision    = assignment.subdivision;
    pat.velocity       = juce::jlimit(1, 127, assignment.velocity)
                       * juce::jlimit(1, 127, triggerVelocity) / 127;
    pat.velocitySteps  = assignment.velocitySteps;
    pat.lastStepDuration = computeStepDuration(bpm, numerator,
                                  assignment.effectiveSubdivision(denominator));
    pat.gate           = assignment.gate;
    pat.fixedLengthSteps = assignment.fixedLengthSteps;
    pat.stepNotes      = GateRunnerEngine::computeAllSteps(assignment);
    pat.numSteps       = static_cast<int>(pat.stepNotes.size());
    pat.noteLengthSteps.assign(pat.numSteps, 1);

    // Sounding steps own the following zero steps, including loop wrap.
    int firstSoundingStep = -1;
    for (int i = 0; i < pat.numSteps; ++i)
        if (!pat.stepNotes[i].empty())
        {
            firstSoundingStep = i;
            break;
        }

    if (firstSoundingStep >= 0)
    {
        int nextSoundingStep = firstSoundingStep + pat.numSteps;
        for (int i = pat.numSteps - 1; i >= 0; --i)
            if (!pat.stepNotes[i].empty())
            {
                pat.noteLengthSteps[i] = nextSoundingStep - i;
                pat.maxNoteLengthSteps = std::max(pat.maxNoteLengthSteps, pat.noteLengthSteps[i]);
                nextSoundingStep = i;
            }
    }

    m_activePatterns[key] = std::move(pat);
}

// ---------------------------------------------------------------------------
void PatternScheduler::stopKey(int key, juce::MidiBuffer& midiMessages)
{
    auto it = m_activePatterns.find(key);
    if (it == m_activePatterns.end())
        return;

    for (int note = 0; note < 128; ++note)
        if (it->second.activeNoteCounts[static_cast<size_t>(note)] > 0)
        {
            auto& count = m_outputNoteCounts[static_cast<size_t>(it->second.channel - 1)][static_cast<size_t>(note)];
            count = std::max(0, count - it->second.activeNoteCounts[static_cast<size_t>(note)]);
            if (count == 0)
                midiMessages.addEvent(juce::MidiMessage::noteOff(it->second.channel, note), 0);
        }

    m_activePatterns.erase(it);
}

// ---------------------------------------------------------------------------
bool PatternScheduler::isKeyActive(int key) const
{
    return m_activePatterns.find(key) != m_activePatterns.end();
}

// ---------------------------------------------------------------------------
void PatternScheduler::stopAll(juce::MidiBuffer& midiMessages)
{
    for (int channel = 0; channel < 16; ++channel)
        for (int note = 0; note < 128; ++note)
            if (m_outputNoteCounts[static_cast<size_t>(channel)][static_cast<size_t>(note)] > 0)
                midiMessages.addEvent(juce::MidiMessage::noteOff(channel + 1, note), 0);

    m_activePatterns.clear();
    m_outputNoteCounts = {};
}

// ---------------------------------------------------------------------------
void PatternScheduler::processBlock(juce::MidiBuffer& midiMessages,
                                    int    numSamples,
                                    double bpm,
                                    int    numerator,
                                    int    denominator)
{
    for (auto& [key, pat] : m_activePatterns)
    {
        if (pat.numSteps == 0)
            continue;

        const double stepDur = computeStepDuration(bpm, numerator,
                                       pat.subdivision > 0 ? pat.subdivision : denominator);
        if (stepDur <= 0.0 || !std::isfinite(stepDur))
            continue;
        if (pat.lastStepDuration > 0.0 && stepDur != pat.lastStepDuration)
            pat.elapsedSamples *= stepDur / pat.lastStepDuration;
        pat.lastStepDuration = stepDur;

        double blockStart = pat.elapsedSamples;
        double blockEnd   = blockStart + static_cast<double>(numSamples);

        // Look back by the longest possible duration, including both terms.
        const double maxLength = pat.fixedLengthSteps + pat.gate * pat.maxNoteLengthSteps;
        const int firstStep = juce::jmax(0, static_cast<int>(blockStart / stepDur)
                                            - static_cast<int>(std::ceil(maxLength)) - 1);
        const int lastStep = static_cast<int>(blockEnd / stepDur) + 1;

        auto& events = pat.pendingEvents;
        events.clear();
        for (int i = firstStep; i <= lastStep; ++i)
        {
            const int stepIdx = i % pat.numSteps;
            const double noteOnTime = static_cast<double>(i) * stepDur;
            const double noteOffTime = noteOnTime
                + (pat.fixedLengthSteps + pat.gate * pat.noteLengthSteps[stepIdx]) * stepDur;

            for (int note : pat.stepNotes[stepIdx])
            {
                if (note < 0 || note >= 128)
                    continue;
                if (noteOnTime >= blockStart && noteOnTime < blockEnd)
                    events.push_back({ noteOnTime, i, note, true });
                if (noteOffTime >= blockStart && noteOffTime < blockEnd)
                    events.push_back({ noteOffTime, i, note, false });
            }
        }

        // Sort by time, then origin step. At a shared boundary an older note
        // ends before the next begins; a zero-duration note begins before it ends.
        std::sort(events.begin(), events.end(), [](const ActivePattern::NoteEvent& a,
                                                   const ActivePattern::NoteEvent& b)
        {
            if (a.time != b.time) return a.time < b.time;
            if (a.step != b.step) return a.step < b.step;
            return a.on && !b.on;
        });

        for (const auto& event : events)
        {
            const int sampleAt = juce::jlimit(0, numSamples - 1,
                static_cast<int>(event.time - blockStart));
            if (event.on)
            {
                const int stepVelocity = pat.velocitySteps.empty() ? 127
                    : juce::jlimit(0, 127, pat.velocitySteps[static_cast<size_t>(event.step % pat.velocitySteps.size())]);
                const int velocity = pat.velocity * stepVelocity / 127;
                if (velocity == 0)
                    continue;
                midiMessages.addEvent(juce::MidiMessage::noteOn(pat.channel, event.note,
                    static_cast<juce::uint8>(velocity)), sampleAt);
                ++pat.activeNoteCounts[static_cast<size_t>(event.note)];
                ++m_outputNoteCounts[static_cast<size_t>(pat.channel - 1)][static_cast<size_t>(event.note)];
            }
            else
            {
                auto& count = pat.activeNoteCounts[static_cast<size_t>(event.note)];
                if (count > 0)
                {
                    --count;
                    auto& global = m_outputNoteCounts[static_cast<size_t>(pat.channel - 1)][static_cast<size_t>(event.note)];
                    global = std::max(0, global - 1);
                    if (global == 0)
                        midiMessages.addEvent(juce::MidiMessage::noteOff(pat.channel, event.note), sampleAt);
                }
            }
        }

        pat.elapsedSamples += static_cast<double>(numSamples);
    }
}
