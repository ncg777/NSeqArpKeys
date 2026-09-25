#pragma once

#include <map>
#include <array>
#include <vector>
#include <cstdint>
#include <JuceHeader.h>
#include "../Domain/KeyAssignment.h"
#include "GateRunnerEngine.h"

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
    int   subdivision = 0;
    int   velocity = 100;
    double lastStepDuration = 0.0;
    float gate     = 0.5f;
    float fixedLengthSteps = 0.0f;

    /** Precomputed note lists for every step (indexed by step % numSteps). */
    std::vector<std::vector<GateRunnerEngine::StepNote>> stepNotes;
    std::vector<int> noteLengthSteps;
    int maxNoteLengthSteps = 1;

    /** Active instances per pitch, used to avoid cutting off overlaps early. */
    std::array<int, 128> activeNoteCounts {};
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
                    juce::MidiBuffer& midiMessages,
                    int triggerVelocity = 127);

    /** Stop all active patterns and send note-offs into @p midiMessages. */
    void stopAll(juce::MidiBuffer& midiMessages);
    void stopKey(int key, juce::MidiBuffer& midiMessages);
    bool isKeyActive(int key) const;
    void setSubdivision(int key, int subdivision);

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
    std::array<std::array<int, 128>, 16> m_outputNoteCounts {};
    struct NoteEvent
    {
        double time;
        double onset;
        int key;
        int note;
        int velocity;
        bool on;
    };
    std::vector<NoteEvent> m_pendingEvents;

    double computeStepDuration(double bpm, int numerator, int denominator) const;
};
