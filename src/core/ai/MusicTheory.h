#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <set>
#include <algorithm>

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

    struct ChordMatch {
        juce::String rootNoteName;
        ChordType type;
        juce::String displayName;
        bool isValid = false;
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
    
    /** Get note name from MIDI pitch (e.g., 60 -> "C4", 62 -> "D4") */
    static juce::String getNoteName(int pitch) {
        const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int semitone = pitch % 12;
        int octave = (pitch / 12) - 1;
        return juce::String(notes[semitone]) + juce::String(octave);
    }

    /** Get root note name from semitone (0-11) -> "C", "C#", "D", etc. */
    static juce::String getRootNoteName(int semitone) {
        const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        return juce::String(notes[semitone % 12]);
    }

    /** Detect chord from a set of MIDI pitches (e.g., {60, 64, 67} -> "C") */
    static ChordMatch detectChord(const std::set<int>& pitches) {
        ChordMatch result;

        if (pitches.size() < 3) {
            return result;
        }

        std::set<int> normalizedPitches;
        for (int pitch : pitches) {
            normalizedPitches.insert(pitch % 12);
        }

        for (int rootCandidate : normalizedPitches) {
            std::vector<int> intervals;
            for (int pitch : normalizedPitches) {
                int interval = (pitch - rootCandidate + 12) % 12;
                intervals.push_back(interval);
            }
            std::sort(intervals.begin(), intervals.end());

            ChordType matchedType = matchIntervals(intervals);

            if (matchedType != ChordType::None) {
                result.rootNoteName = getRootNoteName(rootCandidate);
                result.type = matchedType;
                result.displayName = formatChordName(result.rootNoteName, matchedType);
                result.isValid = true;
                return result;
            }
        }

        result.displayName = juce::String(normalizedPitches.size()) + " notes";
        result.isValid = true;
        return result;
    }

private:
    static ChordType matchIntervals(const std::vector<int>& intervals) {
        if (intervals.empty() || intervals[0] != 0) return ChordType::None;

        if (intervals.size() == 3) {
            if (intervals == std::vector<int>{0, 4, 7}) return ChordType::Major;
            if (intervals == std::vector<int>{0, 3, 7}) return ChordType::Minor;
            if (intervals == std::vector<int>{0, 3, 6}) return ChordType::Diminished;
            if (intervals == std::vector<int>{0, 4, 8}) return ChordType::Augmented;
            if (intervals == std::vector<int>{0, 2, 7}) return ChordType::Sus2;
            if (intervals == std::vector<int>{0, 5, 7}) return ChordType::Sus4;
        }

        if (intervals.size() == 4) {
            if (intervals == std::vector<int>{0, 4, 7, 11}) return ChordType::Maj7;
            if (intervals == std::vector<int>{0, 3, 7, 10}) return ChordType::Min7;
            if (intervals == std::vector<int>{0, 4, 7, 10}) return ChordType::Dom7;
        }

        return ChordType::None;
    }

    static juce::String formatChordName(const juce::String& root, ChordType type) {
        switch (type) {
            case ChordType::Major:      return root;
            case ChordType::Minor:      return root + "m";
            case ChordType::Diminished: return root + "dim";
            case ChordType::Augmented:  return root + "aug";
            case ChordType::Sus2:       return root + "sus2";
            case ChordType::Sus4:       return root + "sus4";
            case ChordType::Maj7:       return root + "maj7";
            case ChordType::Min7:       return root + "m7";
            case ChordType::Dom7:       return root + "7";
            default:                    return root;
        }
    }
};

} // namespace pianodaw
