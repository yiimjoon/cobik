#include "Clip.h"
#include "../timeline/PPQ.h"

namespace pianodaw {

bool Clip::exportToMidiFile(const juce::File& file, int ppq) const
{
    juce::ScopedLock sl(lock);
    
    // Create MIDI sequence
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(ppq);
    
    // Create a single track
    juce::MidiMessageSequence track;
    
    // Add notes
    for (const auto& note : notes) {
        // Note On
        double noteOnTime = (double)note.startTick / PPQ::TICKS_PER_QUARTER;
        int velocity = (int)(note.velocity * 127.0f);
        
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, note.pitch, (juce::uint8)velocity);
        noteOn.setTimeStamp(noteOnTime);
        track.addEvent(noteOn);
        
        // Note Off
        double noteOffTime = (double)note.endTick / PPQ::TICKS_PER_QUARTER;
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, note.pitch);
        noteOff.setTimeStamp(noteOffTime);
        track.addEvent(noteOff);
    }
    
    // Add CC events
    for (const auto& cc : ccEvents) {
        double time = (double)cc.tick / PPQ::TICKS_PER_QUARTER;
        juce::MidiMessage ccMsg = juce::MidiMessage::controllerEvent(1, cc.cc, cc.value);
        ccMsg.setTimeStamp(time);
        track.addEvent(ccMsg);
    }
    
    // Sort events by time
    track.updateMatchedPairs();
    track.sort();
    
    // Add track to MIDI file
    midiFile.addTrack(track);
    
    // Write to file
    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
        return false;
    
    midiFile.writeTo(stream);
    return true;
}

} // namespace pianodaw
