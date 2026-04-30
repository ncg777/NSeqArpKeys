#include "PatternScheduler.h"
#include "GateRunnerEngine.h"

#include <algorithm>

// ---------------------------------------------------------------------------
void PatternScheduler::prepare(double sampleRate)
{
    m_sampleRate = sampleRate;
    m_activePatterns.clear();
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
                                  juce::MidiBuffer& midiMessages)
{
    if (assignment.sequence.empty())
        return;

    // Send note-offs for any notes still sounding from a previous pattern on
    // this key so we do not leave hanging notes.
    auto existing = m_activePatterns.find(key);
    if (existing != m_activePatterns.end())
    {
        auto& old = existing->second;
        for (int note : old.currentlyActiveNotes)
            midiMessages.addEvent(juce::MidiMessage::noteOff(old.channel, note), 0);
    }

    // Precompute step notes via the GateRunner engine.
    ActivePattern pat;
    pat.elapsedSamples = 0.0;
    pat.channel        = assignment.channel;
    pat.gate           = assignment.gate;
    pat.stepNotes      = GateRunnerEngine::computeAllSteps(assignment);
    pat.numSteps       = static_cast<int>(pat.stepNotes.size());

    m_activePatterns[key] = std::move(pat);
}

// ---------------------------------------------------------------------------
void PatternScheduler::stopAll(juce::MidiBuffer& midiMessages)
{
    for (auto& [key, pat] : m_activePatterns)
        for (int note : pat.currentlyActiveNotes)
            midiMessages.addEvent(juce::MidiMessage::noteOff(pat.channel, note), 0);

    m_activePatterns.clear();
}

// ---------------------------------------------------------------------------
void PatternScheduler::processBlock(juce::MidiBuffer& midiMessages,
                                    int    numSamples,
                                    double bpm,
                                    int    numerator,
                                    int    denominator)
{
    double stepDur = computeStepDuration(bpm, numerator, denominator);
    if (stepDur <= 0.0)
        return;

    for (auto& [key, pat] : m_activePatterns)
    {
        if (pat.numSteps == 0)
            continue;

        double blockStart = pat.elapsedSamples;
        double blockEnd   = blockStart + static_cast<double>(numSamples);

        // We need to find every step i whose note-on or note-off falls in
        // [blockStart, blockEnd).
        //
        // Note-on  for step i: noteOnTime  = i * stepDur
        // Note-off for step i: noteOffTime = i * stepDur + gate * stepDur
        //
        // Because gate ≤ 1.0 the note-off is always ≤ the next note-on, so we
        // only need to search a small window of step indices.

        // Lowest step index whose note-off could fall in this block:
        //   i * stepDur + gate * stepDur >= blockStart
        //   i >= (blockStart / stepDur) - gate
        int firstStep = juce::jmax(0, static_cast<int>(blockStart / stepDur) - 1);

        // Highest step index whose note-on could fall in this block:
        int lastStep  = static_cast<int>(blockEnd / stepDur) + 1;

        for (int i = firstStep; i <= lastStep; ++i)
        {
            int stepIdx = i % pat.numSteps;
            const auto& notes = pat.stepNotes[stepIdx];

            double noteOnTime  = static_cast<double>(i) * stepDur;
            double noteOffTime = noteOnTime + pat.gate * stepDur;

            // -- Note-on -------------------------------------------------------
            if (noteOnTime >= blockStart && noteOnTime < blockEnd)
            {
                int sampleAt = static_cast<int>(noteOnTime - blockStart);
                sampleAt = juce::jmax(0, juce::jmin(numSamples - 1, sampleAt));

                for (int note : notes)
                {
                    if (note >= 0 && note < 128)
                    {
                        midiMessages.addEvent(
                            juce::MidiMessage::noteOn(pat.channel, note, static_cast<juce::uint8>(100)),
                            sampleAt);
                        pat.currentlyActiveNotes.insert(note);
                    }
                }
            }

            // -- Note-off ------------------------------------------------------
            if (noteOffTime >= blockStart && noteOffTime < blockEnd)
            {
                int sampleAt = static_cast<int>(noteOffTime - blockStart);
                sampleAt = juce::jmax(0, juce::jmin(numSamples - 1, sampleAt));

                for (int note : notes)
                {
                    if (note >= 0 && note < 128)
                    {
                        midiMessages.addEvent(
                            juce::MidiMessage::noteOff(pat.channel, note),
                            sampleAt);
                        pat.currentlyActiveNotes.erase(note);
                    }
                }
            }
        }

        pat.elapsedSamples += static_cast<double>(numSamples);
    }
}
