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
    // Send note-offs for any notes still sounding from a previous pattern on
    // this key so we do not leave hanging notes.
    stopKey(key, midiMessages);
    if (assignment.sequence.empty())
        return;

    // Precompute step notes via the GateRunner engine.
    ActivePattern pat;
    pat.elapsedSamples = 0.0;
    pat.channel        = juce::jlimit(1, 16, assignment.channel);
    pat.subdivision    = assignment.subdivision;
    pat.velocity       = std::max(1, juce::jlimit(1, 127, assignment.velocity)
                       * juce::jlimit(1, 127, triggerVelocity) / 127);
    pat.lastStepDuration = computeStepDuration(bpm, numerator,
                                  assignment.effectiveSubdivision(denominator));
    pat.gate           = assignment.gate;
    pat.fixedLengthSteps = assignment.fixedLengthSteps;
    pat.stepNotes      = GateRunnerEngine::computeAllStepEvents(assignment);
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

    const auto warp = TimeWarp::resolve(assignment.timeWarp);
    const auto minimumDuration = pat.lastStepDuration > 0 ? 0.0005 * m_sampleRate / pat.lastStepDuration : 0.0;
    for (int step = 0; step < pat.numSteps; ++step)
    {
        const auto duration = pat.fixedLengthSteps + pat.gate * pat.noteLengthSteps[step];
        const auto timing = TimeWarp::timing(step, duration, pat.numSteps, assignment.timeWarp, warp, minimumDuration);
        pat.onsets.push_back({ timing.onset, timing.onset, step });
        pat.releases.push_back({ timing.onset + timing.duration, timing.onset, step });
    }
    auto byTime = [](const ActivePattern::StepEvent& a, const ActivePattern::StepEvent& b) { return a.time < b.time; };
    std::stable_sort(pat.onsets.begin(), pat.onsets.end(), byTime);
    std::stable_sort(pat.releases.begin(), pat.releases.end(), byTime);
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

int PatternScheduler::getPlaybackStep(int key) const
{
    const auto found = m_activePatterns.find(key);
    if (found == m_activePatterns.end() || found->second.numSteps <= 0 || found->second.lastStepDuration <= 0)
        return -1;
    const auto& pattern = found->second;
    const auto phase = std::fmod(pattern.elapsedSamples / pattern.lastStepDuration, pattern.numSteps);
    auto next = std::upper_bound(pattern.onsets.begin(), pattern.onsets.end(), phase,
        [](double time, const ActivePattern::StepEvent& event) { return time < event.time; });
    if (next == pattern.onsets.begin())
        return pattern.elapsedSamples < pattern.numSteps * pattern.lastStepDuration ? -1 : pattern.onsets.back().step;
    return std::prev(next)->step;
}

void PatternScheduler::setSubdivision(int key, int subdivision)
{
    if (auto it = m_activePatterns.find(key); it != m_activePatterns.end())
        it->second.subdivision = juce::jlimit(0, 16, subdivision);
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
    if (numSamples <= 0)
        return;
    auto& events = m_pendingEvents;
    events.clear();
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

        auto appendEvents = [&](const std::vector<ActivePattern::StepEvent>& schedule, bool on)
        {
            const double length = pat.numSteps;
            const auto firstCycle = std::max<int64_t>(0, static_cast<int64_t>(std::floor((blockStart / stepDur - schedule.back().time) / length)) - 1);
            const auto lastCycle = static_cast<int64_t>(std::floor((blockEnd / stepDur - schedule.front().time) / length)) + 1;
            for (auto cycle = firstCycle; cycle <= lastCycle; ++cycle)
            {
                const auto cycleStart = static_cast<double>(cycle) * length;
                // Search in sample units too: identical comparisons at block
                // boundaries prevent a fractional onset being lost or doubled.
                auto it = std::lower_bound(schedule.begin(), schedule.end(), blockStart,
                    [&](const ActivePattern::StepEvent& event, double time) { return (cycleStart + event.time) * stepDur < time; });
                for (; it != schedule.end(); ++it)
                {
                    const auto time = (cycleStart + it->time) * stepDur;
                    if (time >= blockEnd) break;
                    const auto onset = (cycleStart + it->onset) * stepDur;
                    for (const auto& hit : pat.stepNotes[it->step])
                    {
                        if (hit.note < 0 || hit.note >= 128) continue;
                        const auto velocity = std::max(1, pat.velocity * hit.velocityLevel / 127);
                        events.push_back({ time - blockStart, onset - blockStart, key, hit.note, velocity, on });
                    }
                }
            }
        };
        appendEvents(pat.onsets, true);
        appendEvents(pat.releases, false);

        pat.elapsedSamples += static_cast<double>(numSamples);
    }

    // Ownership must be evaluated chronologically across ALL keys, not
    // one complete pattern at a time. Older notes end before new onsets;
    // a zero-duration note still begins before its own off.
    std::stable_sort(events.begin(), events.end(), [](const NoteEvent& a, const NoteEvent& b)
    {
        if (a.time != b.time) return a.time < b.time;
        if (a.onset != b.onset) return a.onset < b.onset;
        return a.on && !b.on;
    });

    for (const auto& event : events)
    {
        auto& pat = m_activePatterns.at(event.key);
        const int sampleAt = juce::jlimit(0, numSamples - 1,
            static_cast<int>(event.time));
        if (event.on)
        {
            // MIDI receivers may stack repeated note-ons. Keep exactly one
            // wire-level note per channel/pitch, while retaining all logical
            // owners so an older gate or another key cannot end it early.
            if (m_outputNoteCounts[static_cast<size_t>(pat.channel - 1)][static_cast<size_t>(event.note)] > 0)
                midiMessages.addEvent(juce::MidiMessage::noteOff(pat.channel, event.note), sampleAt);
            midiMessages.addEvent(juce::MidiMessage::noteOn(pat.channel, event.note,
                static_cast<juce::uint8>(event.velocity)), sampleAt);
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
}
