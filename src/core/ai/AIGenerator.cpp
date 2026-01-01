#include "AIGenerator.h"
#include <map>

namespace pianodaw {

std::vector<Note> AIGenerator::generateNotesFromProgression(const juce::Array<juce::var>& progression, int64_t startTick, int64_t ticksPerChord)
{
    std::vector<Note> allNotes;
    int64_t currentStart = startTick;

    for (const auto& chordVar : progression)
    {
        juce::String chordName = chordVar.toString();
        auto pitches = getPitchesForChord(chordName);
        
        for (int pitch : pitches)
        {
            Note n;
            n.pitch = pitch;
            n.startTick = currentStart;
            n.endTick = currentStart + (int64_t)(ticksPerChord * 0.9); // Slight gap between chords
            n.velocity = 0.7f;
            allNotes.push_back(n);
        }
        
        currentStart += ticksPerChord;
    }

    return allNotes;
}

std::vector<int> AIGenerator::getPitchesForChord(const juce::String& chordName)
{
    std::vector<int> pitches;
    if (chordName.isEmpty()) return pitches;

    // Simple parser: Root + Quality
    juce::String root = chordName.substring(0, 1);
    if (chordName.length() > 1 && (chordName[1] == '#' || chordName[1] == 'b'))
        root = chordName.substring(0, 2);

    int rootPitch = noteNameToPitch(root);
    if (rootPitch == -1) return pitches;

    // Center chords around Octave 3/4 (MIDI 48-60)
    while (rootPitch < 48) rootPitch += 12;

    pitches.push_back(rootPitch);

    juce::String quality = chordName.substring(root.length());
    
    // Basic chord qualities
    if (quality == "" || quality == "maj" || quality == "Major") {
        pitches.push_back(rootPitch + 4); 
        pitches.push_back(rootPitch + 7);
    }
    else if (quality == "m" || quality == "min" || quality == "Minor") {
        pitches.push_back(rootPitch + 3);
        pitches.push_back(rootPitch + 7);
    }
    else if (quality == "7") {
        pitches.push_back(rootPitch + 4);
        pitches.push_back(rootPitch + 7);
        pitches.push_back(rootPitch + 10);
    }
    else if (quality == "maj7") {
        pitches.push_back(rootPitch + 4);
        pitches.push_back(rootPitch + 7);
        pitches.push_back(rootPitch + 11);
    }
    else if (quality == "m7") {
        pitches.push_back(rootPitch + 3);
        pitches.push_back(rootPitch + 7);
        pitches.push_back(rootPitch + 10);
    }
    else if (quality == "dim") {
        pitches.push_back(rootPitch + 3);
        pitches.push_back(rootPitch + 6);
    }

    return pitches;
}

int AIGenerator::noteNameToPitch(juce::String name)
{
    static std::map<juce::String, int> noteMap = {
        {"C", 0}, {"C#", 1}, {"Db", 1}, {"D", 2}, {"D#", 3}, {"Eb", 3},
        {"E", 4}, {"F", 5}, {"F#", 6}, {"Gb", 6}, {"G", 7}, {"G#", 8},
        {"Ab", 8}, {"A", 9}, {"A#", 10}, {"Bb", 10}, {"B", 11}
    };

    auto it = noteMap.find(name);
    return (it != noteMap.end()) ? it->second : -1;
}

} // namespace pianodaw
