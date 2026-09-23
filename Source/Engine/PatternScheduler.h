#pragma once

#include <map>
#include <set>
#include <vector>
#include <JuceHeader.h>
#include "../Domain/KeyAssignment.h"

// ---------------------------------------------------------------------------
/** Runtime state for one currently-active pattern instance. */
struct ActivePattern
{
    /** Total samples elapsed since this pattern was triggered.
     *  Used together with the current step duration to find which steps
     *  and note-offs fall inside the current processBlock() window. */
    double elapsedSamples = 0.0;

    int   numSteps = 0;
    int   channel  = 1;
    float gate     = 0.5f;

    /** Precomputed note lists for every step (indexed by step % numSteps). */
    std::vector<std::vector<int>> stepNotes;
    std::vector<int> noteLengthSteps;
    int maxNoteLengthSteps = 1;

    /** Notes currently sounding – used to send clean note-offs when the
     *  pattern is stopped / retriggered. */
    std::set<int> currentlyActiveNotes;
};

// ---------------------------------------------------------------------------
/**
 * PatternScheduler – runtime playback engine for per-key triggered patterns.
 *
 * Responsibilities:
 *  • Keep a map of ActivePattern objects (one per sounding trigger key).
 *  • On triggerKey(): pre-compute all step notes via GateRunnerEngine and
 *    reset the elapsed-samples counter; send note-offs for any notes that
 *    were still sounding from a previous pattern on the same key.
 *  • On processBlock(): advance each active pattern using sample-accurate
 *    timing, emitting MIDI note-on / note-off events at the correct offsets
 *    within the buffer.
 *
 * Timing model (mirrors gaterunner/cli/generate.ts):
 *   step duration (seconds) = 60 / (bpm × denominator)
 *   step duration (samples) = step_duration_seconds × sampleRate
 *
 * Patterns loop indefinitely until they are retriggered or cleared.
 */
class PatternScheduler
{
public:
    /** Call once before processing begins (JUCE prepareToPlay). */
    void prepare(double sampleRate);

    /**
     * Start (or restart) the pattern assigned to @p key.
     * Any notes that are still sounding from a previous pattern on this key
     * are turned off at sample offset 0 of @p midiMessages.
     */
    void triggerKey(int key,
                    const KeyAssignment& assignment,
                    double bpm,
                    int    numerator,
                    int    denominator,
                    juce::MidiBuffer& midiMessages);

    /** Stop all active patterns and send note-offs into @p midiMessages. */
    void stopAll(juce::MidiBuffer& midiMessages);
    void stopKey(int key, juce::MidiBuffer& midiMessages);
    bool isKeyActive(int key) const;

    /**
     * Advance all active patterns by @p numSamples.
     * Fills @p midiMessages with note-on / note-off events at the correct
     * sample offsets for this block.
     */
    void processBlock(juce::MidiBuffer& midiMessages,
                      int    numSamples,
                      double bpm,
                      int    numerator,
                      int    denominator);

private:
    double m_sampleRate = 44100.0;
    std::map<int, ActivePattern> m_activePatterns;

    double computeStepDuration(double bpm, int numerator, int denominator) const;
};
