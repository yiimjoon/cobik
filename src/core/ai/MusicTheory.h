#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace pianodaw {

/**
 * MusicTheory - Constants and types for scales and chords
 */
struct MusicTheory {
    enum class ScaleType {
        Chromatic,
        Major,
        Minor,
        Dorian,
        Phrygian,
        Lydian,
        Mixolydian,
        Locrian,
        PentatonicMajor,
        PentatonicMinor
    };

    enum class ChordType {
        None,
        Major,
        Minor,
        Diminished,
        Augmented,
        Sus2,
        Sus4,
        Maj7,
        Min7,
        Dom7
    };

    /** Returns intervals from root for a given scale type */
    static std::vector<int> getScaleIntervals(ScaleType type) {
        switch (type) {
            case ScaleType::Major:           return {0, 2, 4, 5, 7, 9, 11};
            case ScaleType::Minor:           return {0, 2, 3, 5, 7, 8, 10};
            case ScaleType::Dorian:          return {0, 2, 3, 5, 7, 9, 10};
            case ScaleType::Phrygian:        return {0, 1, 3, 5, 7, 8, 10};
            case ScaleType::Lydian:          return {0, 2, 4, 6, 7, 9, 11};
            case ScaleType::Mixolydian:      return {0, 2, 4, 5, 7, 9, 10};
            case ScaleType::Locrian:         return {0, 1, 3, 5, 6, 8, 10};
            case ScaleType::PentatonicMajor: return {0, 2, 4, 7, 9};
            case ScaleType::PentatonicMinor: return {0, 3, 5, 7, 10};
            default:                         return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        }
    }

    /** Returns intervals from root for a given chord type */
    static std::vector<int> getChordIntervals(ChordType type) {
        switch (type) {
            case ChordType::Major:      return {0, 4, 7};
            case ChordType::Minor:      return {0, 3, 7};
            case ChordType::Diminished: return {0, 3, 6};
            case ChordType::Augmented:  return {0, 4, 8};
            case ChordType::Sus2:       return {0, 2, 7};
            case ChordType::Sus4:       return {0, 5, 7};
            case ChordType::Maj7:       return {0, 4, 7, 11};
            case ChordType::Min7:       return {0, 3, 7, 10};
            case ChordType::Dom7:       return {0, 4, 7, 10};
            default:                    return {0};
        }
    }

    /** Checks if a MIDI pitch (0-127) is within a scale */
    static bool isPitchInScale(int pitch, int rootNote, ScaleType type) {
        if (type == ScaleType::Chromatic) return true;
        int semitone = (pitch - rootNote) % 12;
        if (semitone < 0) semitone += 12;
        auto intervals = getScaleIntervals(type);
        for (int i : intervals) if (i == semitone) return true;
        return false;
    }
};

} // namespace pianodaw
