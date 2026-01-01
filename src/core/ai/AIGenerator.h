#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include "../model/Note.h"

namespace pianodaw {

/**
 * AIGenerator - Utility for creating MIDI data from musical descriptions
 */
class AIGenerator
{
public:
    struct ChordInfo {
        juce::String name;
        int64_t startTick;
        int64_t duration;
    };

    /** Generates a list of notes from a chord progression string array */
    static std::vector<Note> generateNotesFromProgression(const juce::Array<juce::var>& progression, int64_t startTick, int64_t ticksPerChord);

private:
    /** Helper to get MIDI pitches for a chord name (e.g., "Cmaj7") */
    static std::vector<int> getPitchesForChord(const juce::String& chordName);
    
    /** Maps note names to MIDI numbers */
    static int noteNameToPitch(juce::String name);
};

} // namespace pianodaw
