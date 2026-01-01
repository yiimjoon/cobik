#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Note.h"
#include "CC.h"
#include <vector>
#include <algorithm>
#include <memory>

namespace pianodaw {

/**
 * Clip contains notes and CC events
 */
class Clip
{
public:
    Clip() : nextNoteId(1), name("Clip") {}
    Clip(const juce::String& clipName) : nextNoteId(1), name(clipName) {}
    
    // === Name ===
    const juce::String& getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }
    
    // === Note access (for select all) ===
    const std::vector<Note>& getNotes() const { return notes; }
    
    // === Note management ===
    
    /** Add note and return its ID */
    int addNote(int pitch, int64_t startTick, int64_t endTick, int velocity)
    {
        juce::ScopedLock sl(lock);
        int id = nextNoteId++;
        notes.emplace_back(id, pitch, startTick, endTick, velocity);
        sortNotes();
        return id;
    }

    int addNote(const Note& n)
    {
        return addNote(n.pitch, n.startTick, n.endTick, (int)(n.velocity * 127.0f));
    }
    
    /** Remove note by ID */
    bool removeNote(int noteId)
    {
        juce::ScopedLock sl(lock);
        auto it = std::find_if(notes.begin(), notes.end(),
            [noteId](const Note& n) { return n.id == noteId; });
        
        if (it != notes.end())
        {
            notes.erase(it);
            return true;
        }
        return false;
    }
    
    /** Find note by ID */
    Note* findNote(int noteId)
    {
        juce::ScopedLock sl(lock);
        auto it = std::find_if(notes.begin(), notes.end(),
            [noteId](const Note& n) { return n.id == noteId; });
        
        return (it != notes.end()) ? &(*it) : nullptr;
    }
    
    /** Get all notes in time range */
    std::vector<Note*> getNotesInRange(int64_t startTick, int64_t endTick)
    {
        juce::ScopedLock sl(lock);
        std::vector<Note*> result;
        for (auto& note : notes)
        {
            if (note.overlaps(startTick, endTick))
                result.push_back(&note);
        }
        return result;
    }
    
    /** Remove notes in pitch and time range (used for recording replace mode) */
    void removeNotesInRange(int minPitch, int maxPitch, int64_t startTick, int64_t endTick)
    {
        juce::ScopedLock sl(lock);
        notes.erase(
            std::remove_if(notes.begin(), notes.end(),
                [minPitch, maxPitch, startTick, endTick](const Note& n) {
                    return n.pitch >= minPitch && n.pitch <= maxPitch &&
                           n.overlaps(startTick, endTick);
                }),
            notes.end());
    }
    
    /** Get all notes */
    std::vector<Note>& getNotes() { return notes; }
    const std::vector<Note>& getNotes() const { return notes; }
    
    // === CC management ===
    
    /** Add CC event */
    void addCCEvent(int cc, int64_t tick, int value)
    {
        juce::ScopedLock sl(lock);
        ccEvents.emplace_back(cc, tick, value);
        sortCCEvents();
    }
    
    /** Remove CC events at tick */
    void removeCCEventsAtTick(int64_t tick, int cc = -1)
    {
        juce::ScopedLock sl(lock);
        ccEvents.erase(
            std::remove_if(ccEvents.begin(), ccEvents.end(),
                [tick, cc](const CCEvent& e) {
                    return e.tick == tick && (cc < 0 || e.cc == cc);
                }),
            ccEvents.end());
    }
    
    /** Get CC events in range */
    std::vector<CCEvent*> getCCEventsInRange(int64_t startTick, int64_t endTick)
    {
        juce::ScopedLock sl(lock);
        std::vector<CCEvent*> result;
        for (auto& event : ccEvents)
        {
            if (event.tick >= startTick && event.tick < endTick)
                result.push_back(&event);
        }
        return result;
    }
    
    /** Get all CC events */
    std::vector<CCEvent>& getCCEvents() { return ccEvents; }
    const std::vector<CCEvent>& getCCEvents() const { return ccEvents; }
    
    juce::CriticalSection& getLock() { return lock; }
    
    // === Utility ===
    
    /** Clear all data */
    void clear()
    {
        juce::ScopedLock sl(lock);
        notes.clear();
        ccEvents.clear();
        nextNoteId = 1;
    }
    
    /** Get total duration (last note end or last CC event) */
    int64_t getTotalDuration() const
    {
        juce::ScopedLock sl(lock); 
        int64_t maxTick = 0;
        
        for (const auto& note : notes)
            maxTick = std::max(maxTick, note.endTick);
        
        for (const auto& cc : ccEvents)
            maxTick = std::max(maxTick, cc.tick);
        
        return maxTick;
    }
    
    /** Export to MIDI file */
    bool exportToMidiFile(const juce::File& file, int ppq = 480) const;
    
private:
    juce::String name;
    std::vector<Note> notes;
    std::vector<CCEvent> ccEvents;
    int nextNoteId;
    juce::CriticalSection lock;
    
    void sortNotes()
    {
        std::sort(notes.begin(), notes.end());
    }
    
    void sortCCEvents()
    {
        std::sort(ccEvents.begin(), ccEvents.end());
    }
};

} // namespace pianodaw
