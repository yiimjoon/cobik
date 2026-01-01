#include "MidiRecorder.h"
#include <algorithm>

namespace pianodaw {

MidiRecorder::MidiRecorder()
{
}

MidiRecorder::~MidiRecorder()
{
    stopRecording();
}

void MidiRecorder::startRecording(Clip* clip, int64_t startTick)
{
    juce::ScopedLock lock(recordLock);
    
    if (!clip)
        return;
    
    targetClip = clip;
    recordStartTick = startTick;
    recording = true;
    activeNotes.clear();
    
    // If replace mode, clear existing notes in the recording region
    if (replaceMode && targetClip)
    {
        // We'll clear notes as we record (in real-time)
        // For now, just set the flag
    }
}

void MidiRecorder::stopRecording()
{
    juce::ScopedLock lock(recordLock);
    
    if (!recording)
        return;
    
    // End any active notes at current position
    // (user released keys after stopping transport)
    for (auto& pair : activeNotes)
    {
        // Don't add incomplete notes - just discard them
    }
    activeNotes.clear();
    
    recording = false;
    targetClip = nullptr;
}

void MidiRecorder::processMidiInput(const juce::MidiBuffer& midiMessages, int64_t currentTick)
{
    juce::ScopedLock lock(recordLock);
    
    if (!recording || !targetClip)
        return;
    
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        
        if (msg.isNoteOn())
        {
            int noteNumber = msg.getNoteNumber();
            float velocity = msg.getFloatVelocity();
            handleNoteOn(noteNumber, velocity, currentTick);
        }
        else if (msg.isNoteOff())
        {
            int noteNumber = msg.getNoteNumber();
            handleNoteOff(noteNumber, currentTick);
        }
        // TODO: Handle CC events (pedal, etc.)
    }
}

void MidiRecorder::setQuantizeInput(bool enabled, int64_t gridTicks)
{
    juce::ScopedLock lock(recordLock);
    quantizeInput = enabled;
    quantizeGridTicks = gridTicks;
}

int64_t MidiRecorder::quantizeTick(int64_t tick) const
{
    if (!quantizeInput || quantizeGridTicks <= 0)
        return tick;
    
    // Round to nearest grid line
    int64_t remainder = tick % quantizeGridTicks;
    if (remainder < quantizeGridTicks / 2)
    {
        return tick - remainder;
    }
    else
    {
        return tick + (quantizeGridTicks - remainder);
    }
}

void MidiRecorder::handleNoteOn(int noteNumber, float velocity, int64_t tick)
{
    if (!targetClip)
        return;
    
    // Quantize if enabled
    int64_t recordTick = quantizeTick(tick);
    
    // Store active note
    ActiveNote active;
    active.pitch = noteNumber;
    active.velocity = velocity;
    active.startTick = recordTick;
    activeNotes[noteNumber] = active;
}

void MidiRecorder::handleNoteOff(int noteNumber, int64_t tick)
{
    if (!targetClip)
        return;
    
    // Find matching Note On
    auto it = activeNotes.find(noteNumber);
    if (it == activeNotes.end())
        return;  // Note Off without Note On - ignore
    
    ActiveNote& active = it->second;
    
    // Quantize end tick if enabled
    int64_t endTick = quantizeTick(tick);
    
    // Ensure note has positive length
    if (endTick <= active.startTick)
    {
        endTick = active.startTick + (PPQ::TICKS_PER_QUARTER / 16);  // Minimum 32nd note length
    }
    
    // Create note in clip
    Note note;
    note.pitch = active.pitch;
    note.startTick = active.startTick;
    note.endTick = endTick;
    note.velocity = active.velocity;
    
    // If replace mode, remove overlapping notes first
    if (replaceMode)
    {
        targetClip->removeNotesInRange(note.pitch, note.pitch, note.startTick, note.endTick);
    }
    
    // Add note to clip
    targetClip->addNote(note);
    
    // Remove from active notes
    activeNotes.erase(it);
}

void MidiRecorder::clearRegion(int64_t startTick, int64_t endTick)
{
    if (!targetClip)
        return;
    
    // Clear all notes in the region
    targetClip->removeNotesInRange(0, 127, startTick, endTick);
}

} // namespace pianodaw
