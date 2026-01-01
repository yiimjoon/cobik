#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include "../model/Note.h"
#include "../model/Clip.h"
#include "../quantize/QuantizeEngine.h"
#include "UndoStack.h"

namespace pianodaw {

/**
 * AddNoteCommand - Adds a single note
 */
class AddNoteCommand : public Command
{
public:
    AddNoteCommand(Clip& clip_, int pitch, int64_t start, int64_t end, int vel)
        : clip(clip_), pitch(pitch), startTick(start), endTick(end), velocity(vel) {}

    void execute() override { noteId = clip.addNote(pitch, startTick, endTick, velocity); }
    void undo() override { clip.removeNote(noteId); }
    std::string getDescription() const override { return "Add Note"; }

private:
    Clip& clip;
    int pitch;
    int64_t startTick, endTick;
    int velocity;
    int noteId = -1;
};

/**
 * RemoveNoteCommand - Removes a single note
 */
class RemoveNoteCommand : public Command
{
public:
    RemoveNoteCommand(Clip& clip_, int noteId_) : clip(clip_), noteId(noteId_) {}

    void execute() override {
        if (auto* n = clip.findNote(noteId)) {
            oldNote = *n;
            clip.removeNote(noteId);
        }
    }
    void undo() override { clip.addNote(oldNote); }
    std::string getDescription() const override { return "Remove Note"; }

private:
    Clip& clip;
    int noteId;
    Note oldNote;
};

/**
 * MoveNoteCommand - Moves a set of notes
 */
class MoveNoteCommand : public Command
{
public:
    MoveNoteCommand(Clip& clip_, const std::vector<int>& ids, int64_t deltaTicks, int deltaPitch)
        : clip(clip_), noteIds(ids), dT(deltaTicks), dP(deltaPitch) {}

    void execute() override {
        for (int id : noteIds) {
            if (auto* n = clip.findNote(id)) {
                n->startTick += dT;
                n->endTick += dT;
                n->pitch += dP;
            }
        }
    }
    void undo() override {
        for (int id : noteIds) {
            if (auto* n = clip.findNote(id)) {
                n->startTick -= dT;
                n->endTick -= dT;
                n->pitch -= dP;
            }
        }
    }
    std::string getDescription() const override { return "Move Note(s)"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    int64_t dT;
    int dP;
};

/**
 * BulkAddNotesCommand - Adds multiple notes in one go.
 */
class BulkAddNotesCommand : public Command
{
public:
    BulkAddNotesCommand(Clip& clip_, const std::vector<Note>& notesToAdd) 
        : clip(clip_), notes(notesToAdd) {}

    void execute() override
    {
        addedIds.clear();
        for (auto& n : notes)
        {
            addedIds.push_back(clip.addNote(n));
        }
    }

    void undo() override
    {
        for (int id : addedIds)
        {
            clip.removeNote(id);
        }
        addedIds.clear();
    }
    std::string getDescription() const override { return "AI Generation / Bulk Add"; }

private:
    Clip& clip;
    std::vector<Note> notes;
    std::vector<int> addedIds;
};

/**
 * QuantizeCommand - Quantizes a set of notes
 */
class QuantizeCommand : public Command
{
public:
    QuantizeCommand(Clip& clip_, const std::vector<int>& noteIds_, const QuantizeParams& params_)
        : clip(clip_), noteIds(noteIds_), params(params_) {}

    void execute() override
    {
        originalNotes.clear();
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                originalNotes.push_back(*n);
                QuantizeEngine::quantize(*n, params);
            }
        }
    }

    void undo() override
    {
        for (const auto& oldNote : originalNotes)
        {
            if (auto* n = clip.findNote(oldNote.id))
            {
                *n = oldNote;
            }
        }
    }
    std::string getDescription() const override { return "Quantize"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    QuantizeParams params;
    std::vector<Note> originalNotes;
};

// ============= Phase 3: Note Editing Commands =============

/**
 * ScaleLengthCommand - Scale note lengths by a percentage
 * @param percentage: -200% to +200% (0 = no change, 100 = double length)
 */
class ScaleLengthCommand : public Command
{
public:
    ScaleLengthCommand(Clip& clip_, const std::vector<int>& noteIds_, double percentage_)
        : clip(clip_), noteIds(noteIds_), percentage(percentage_) {}

    void execute() override
    {
        originalNotes.clear();
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                originalNotes.push_back(*n);
                
                // Calculate new length
                int64_t originalLength = n->endTick - n->startTick;
                double scaleFactor = 1.0 + (percentage / 100.0);
                int64_t newLength = (int64_t)(originalLength * scaleFactor);
                
                // Minimum length: 60 ticks
                newLength = std::max((int64_t)60, newLength);
                
                // Update end tick (start stays the same)
                n->endTick = n->startTick + newLength;
            }
        }
    }

    void undo() override
    {
        for (const auto& oldNote : originalNotes)
        {
            if (auto* n = clip.findNote(oldNote.id))
            {
                n->endTick = oldNote.endTick;
            }
        }
    }
    
    std::string getDescription() const override { return "Scale Length"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    double percentage;
    std::vector<Note> originalNotes;
};

/**
 * LegatoCommand - Extend notes to next note (remove gaps)
 */
class LegatoCommand : public Command
{
public:
    LegatoCommand(Clip& clip_, const std::vector<int>& noteIds_)
        : clip(clip_), noteIds(noteIds_) {}

    void execute() override
    {
        originalNotes.clear();
        
        // Sort notes by start time and pitch
        std::vector<Note*> sortedNotes;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                originalNotes.push_back(*n);
                sortedNotes.push_back(n);
            }
        }
        
        std::sort(sortedNotes.begin(), sortedNotes.end(), 
            [](const Note* a, const Note* b) {
                if (a->pitch != b->pitch) return a->pitch < b->pitch;
                return a->startTick < b->startTick;
            });
        
        // For each note, extend to next note of same pitch
        for (size_t i = 0; i < sortedNotes.size(); ++i)
        {
            auto* currentNote = sortedNotes[i];
            
            // Find next note with same pitch
            Note* nextNote = nullptr;
            for (size_t j = i + 1; j < sortedNotes.size(); ++j)
            {
                if (sortedNotes[j]->pitch == currentNote->pitch &&
                    sortedNotes[j]->startTick > currentNote->startTick)
                {
                    nextNote = sortedNotes[j];
                    break;
                }
            }
            
            // Extend current note to next note (or keep original if no next)
            if (nextNote)
            {
                currentNote->endTick = nextNote->startTick;
            }
        }
    }

    void undo() override
    {
        for (const auto& oldNote : originalNotes)
        {
            if (auto* n = clip.findNote(oldNote.id))
            {
                n->endTick = oldNote.endTick;
            }
        }
    }
    
    std::string getDescription() const override { return "Legato"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    std::vector<Note> originalNotes;
};

/**
 * SetOverlapCommand - Adjust gap/overlap between consecutive notes
 * @param ticks: Positive = overlap, Negative = gap
 */
class SetOverlapCommand : public Command
{
public:
    SetOverlapCommand(Clip& clip_, const std::vector<int>& noteIds_, int64_t overlapTicks_)
        : clip(clip_), noteIds(noteIds_), overlapTicks(overlapTicks_) {}

    void execute() override
    {
        originalNotes.clear();
        
        // Sort notes by start time and pitch
        std::vector<Note*> sortedNotes;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                originalNotes.push_back(*n);
                sortedNotes.push_back(n);
            }
        }
        
        std::sort(sortedNotes.begin(), sortedNotes.end(), 
            [](const Note* a, const Note* b) {
                if (a->pitch != b->pitch) return a->pitch < b->pitch;
                return a->startTick < b->startTick;
            });
        
        // For each note, adjust end to create overlap/gap with next note
        for (size_t i = 0; i < sortedNotes.size(); ++i)
        {
            auto* currentNote = sortedNotes[i];
            
            // Find next note with same pitch
            Note* nextNote = nullptr;
            for (size_t j = i + 1; j < sortedNotes.size(); ++j)
            {
                if (sortedNotes[j]->pitch == currentNote->pitch &&
                    sortedNotes[j]->startTick > currentNote->startTick)
                {
                    nextNote = sortedNotes[j];
                    break;
                }
            }
            
            // Adjust current note end
            if (nextNote)
            {
                int64_t newEnd = nextNote->startTick + overlapTicks;
                
                // Ensure minimum length (60 ticks)
                if (newEnd > currentNote->startTick + 60)
                {
                    currentNote->endTick = newEnd;
                }
            }
        }
    }

    void undo() override
    {
        for (const auto& oldNote : originalNotes)
        {
            if (auto* n = clip.findNote(oldNote.id))
            {
                n->endTick = oldNote.endTick;
            }
        }
    }
    
    std::string getDescription() const override { return "Set Overlap"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    int64_t overlapTicks;
    std::vector<Note> originalNotes;
};

/**
 * TransposeCommand - Transpose notes by semitones
 */
class TransposeCommand : public Command
{
public:
    TransposeCommand(Clip& clip_, const std::vector<int>& noteIds_, int semitones_)
        : clip(clip_), noteIds(noteIds_), semitones(semitones_) {}

    void execute() override
    {
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                n->pitch += semitones;
                n->pitch = std::max(0, std::min(127, n->pitch)); // Clamp to MIDI range
            }
        }
    }

    void undo() override
    {
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                n->pitch -= semitones;
                n->pitch = std::max(0, std::min(127, n->pitch));
            }
        }
    }
    
    std::string getDescription() const override { return "Transpose"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    int semitones;
};

/**
 * SetVelocityCommand - Set velocity for selected notes
 */
class SetVelocityCommand : public Command
{
public:
    SetVelocityCommand(Clip& clip_, const std::vector<int>& noteIds_, int velocity_)
        : clip(clip_), noteIds(noteIds_), newVelocity(velocity_) {}

    void execute() override
    {
        originalVelocities.clear();
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                originalVelocities.push_back(n->velocity);
                n->velocity = std::max(1, std::min(127, newVelocity));
            }
        }
    }

    void undo() override
    {
        size_t idx = 0;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                if (idx < originalVelocities.size())
                {
                    n->velocity = originalVelocities[idx++];
                }
            }
        }
    }
    
    std::string getDescription() const override { return "Set Velocity"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    int newVelocity;
    std::vector<int> originalVelocities;
};

/**
 * ResizeNoteCommand - Resize a note's start or end time
 */
class ResizeNoteCommand : public Command
{
public:
    ResizeNoteCommand(Clip& clip_, int noteId_, int64_t newStartTick_, int64_t newEndTick_)
        : clip(clip_), noteId(noteId_), newStartTick(newStartTick_), newEndTick(newEndTick_) {}

    void execute() override
    {
        if (auto* n = clip.findNote(noteId))
        {
            oldStartTick = n->startTick;
            oldEndTick = n->endTick;
            n->startTick = newStartTick;
            n->endTick = newEndTick;
        }
    }

    void undo() override
    {
        if (auto* n = clip.findNote(noteId))
        {
            n->startTick = oldStartTick;
            n->endTick = oldEndTick;
        }
    }
    
    std::string getDescription() const override { return "Resize Note"; }

private:
    Clip& clip;
    int noteId;
    int64_t newStartTick, newEndTick;
    int64_t oldStartTick, oldEndTick;
};

// ============= Phase 5: CC Event Commands =============

/**
 * AddCCEventCommand - Add a CC event
 */
class AddCCEventCommand : public Command
{
public:
    AddCCEventCommand(Clip& clip_, int cc_, int64_t tick_, int value_)
        : clip(clip_), cc(cc_), tick(tick_), value(value_) {}

    void execute() override
    {
        clip.addCCEvent(cc, tick, value);
    }

    void undo() override
    {
        clip.removeCCEventsAtTick(tick, cc);
    }
    
    std::string getDescription() const override { return "Add CC Event"; }

private:
    Clip& clip;
    int cc;
    int64_t tick;
    int value;
};

/**
 * RemoveCCEventCommand - Remove CC events at a specific tick
 */
class RemoveCCEventCommand : public Command
{
public:
    RemoveCCEventCommand(Clip& clip_, int64_t tick_, int cc_)
        : clip(clip_), tick(tick_), cc(cc_) {}

    void execute() override
    {
        // Store events before removing
        removedEvents.clear();
        auto& events = clip.getCCEvents();
        for (const auto& e : events)
        {
            if (e.tick == tick && e.cc == cc)
            {
                removedEvents.push_back(e);
            }
        }
        
        clip.removeCCEventsAtTick(tick, cc);
    }

    void undo() override
    {
        // Restore removed events
        for (const auto& e : removedEvents)
        {
            clip.addCCEvent(e.cc, e.tick, e.value);
        }
    }
    
    std::string getDescription() const override { return "Remove CC Event"; }

private:
    Clip& clip;
    int64_t tick;
    int cc;
    std::vector<CCEvent> removedEvents;
};

// ============= Phase 6: Advanced Manipulation Commands =============

/**
 * MirrorVerticalCommand - Mirror notes vertically (flip pitch around center)
 */
class MirrorVerticalCommand : public Command
{
public:
    MirrorVerticalCommand(Clip& clip_, const std::vector<int>& noteIds_)
        : clip(clip_), noteIds(noteIds_) {}

    void execute() override
    {
        juce::ScopedLock sl(clip.getLock());
        
        // Find center pitch
        int minPitch = 127, maxPitch = 0;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                minPitch = std::min(minPitch, n->pitch);
                maxPitch = std::max(maxPitch, n->pitch);
            }
        }
        
        int centerPitch = (minPitch + maxPitch) / 2;
        
        // Mirror each note
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                int offset = n->pitch - centerPitch;
                n->pitch = centerPitch - offset;
                n->pitch = std::max(0, std::min(127, n->pitch));
            }
        }
    }

    void undo() override
    {
        // Mirror again to restore (mirror is self-inverse)
        execute();
    }
    
    std::string getDescription() const override { return "Mirror Vertical"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
};

/**
 * MirrorHorizontalCommand - Mirror notes horizontally (reverse time)
 */
class MirrorHorizontalCommand : public Command
{
public:
    MirrorHorizontalCommand(Clip& clip_, const std::vector<int>& noteIds_)
        : clip(clip_), noteIds(noteIds_) {}

    void execute() override
    {
        juce::ScopedLock sl(clip.getLock());
        
        // Find time bounds
        int64_t minTick = INT64_MAX, maxTick = 0;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                minTick = std::min(minTick, n->startTick);
                maxTick = std::max(maxTick, n->endTick);
            }
        }
        
        // Mirror each note
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                int64_t duration = n->endTick - n->startTick;
                int64_t newEnd = minTick + (maxTick - n->startTick);
                int64_t newStart = newEnd - duration;
                
                n->startTick = newStart;
                n->endTick = newEnd;
            }
        }
    }

    void undo() override
    {
        // Mirror again to restore
        execute();
    }
    
    std::string getDescription() const override { return "Mirror Horizontal"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
};

/**
 * ReverseCommand - Reverse notes in time
 */
class ReverseCommand : public Command
{
public:
    ReverseCommand(Clip& clip_, const std::vector<int>& noteIds_)
        : clip(clip_), noteIds(noteIds_) {}

    void execute() override
    {
        juce::ScopedLock sl(clip.getLock());
        
        // Sort notes by start time
        std::vector<std::pair<int64_t, Note*>> sortedNotes;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                sortedNotes.push_back({n->startTick, n});
            }
        }
        std::sort(sortedNotes.begin(), sortedNotes.end());
        
        // Find bounds
        if (sortedNotes.empty()) return;
        int64_t minTick = sortedNotes.front().first;
        int64_t maxTick = sortedNotes.back().second->endTick;
        
        // Reverse timing
        for (auto& pair : sortedNotes)
        {
            auto* n = pair.second;
            int64_t duration = n->endTick - n->startTick;
            int64_t distFromStart = n->startTick - minTick;
            int64_t newStart = maxTick - distFromStart - duration;
            
            n->startTick = newStart;
            n->endTick = newStart + duration;
        }
    }

    void undo() override
    {
        execute(); // Reverse is self-inverse
    }
    
    std::string getDescription() const override { return "Reverse"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
};

/**
 * FixedLengthCommand - Set all notes to fixed length
 */
class FixedLengthCommand : public Command
{
public:
    FixedLengthCommand(Clip& clip_, const std::vector<int>& noteIds_, int64_t newLength_)
        : clip(clip_), noteIds(noteIds_), newLength(newLength_) {}

    void execute() override
    {
        originalLengths.clear();
        
        juce::ScopedLock sl(clip.getLock());
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                originalLengths.push_back(n->endTick - n->startTick);
                n->endTick = n->startTick + newLength;
            }
        }
    }

    void undo() override
    {
        juce::ScopedLock sl(clip.getLock());
        size_t idx = 0;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                if (idx < originalLengths.size())
                {
                    n->endTick = n->startTick + originalLengths[idx++];
                }
            }
        }
    }
    
    std::string getDescription() const override { return "Fixed Length"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    int64_t newLength;
    std::vector<int64_t> originalLengths;
};

/**
 * HumanizeCommand - Add random variation to timing and velocity
 */
class HumanizeCommand : public Command
{
public:
    HumanizeCommand(Clip& clip_, const std::vector<int>& noteIds_, int timingAmount_, int velocityAmount_)
        : clip(clip_), noteIds(noteIds_), timingAmount(timingAmount_), velocityAmount(velocityAmount_) {}

    void execute() override
    {
        originalStates.clear();
        
        juce::ScopedLock sl(clip.getLock());
        std::srand((unsigned)std::time(nullptr));
        
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                originalStates.push_back({n->startTick, n->endTick, n->velocity});
                
                // Random timing offset (-timingAmount to +timingAmount)
                int64_t timeOffset = (std::rand() % (timingAmount * 2 + 1)) - timingAmount;
                n->startTick = std::max((int64_t)0, n->startTick + timeOffset);
                n->endTick = std::max(n->startTick + 60, n->endTick + timeOffset);
                
                // Random velocity offset
                int velOffset = (std::rand() % (velocityAmount * 2 + 1)) - velocityAmount;
                n->velocity = std::max(1, std::min(127, n->velocity + velOffset));
            }
        }
    }

    void undo() override
    {
        juce::ScopedLock sl(clip.getLock());
        size_t idx = 0;
        for (int id : noteIds)
        {
            if (auto* n = clip.findNote(id))
            {
                if (idx < originalStates.size())
                {
                    auto& state = originalStates[idx++];
                    n->startTick = std::get<0>(state);
                    n->endTick = std::get<1>(state);
                    n->velocity = std::get<2>(state);
                }
            }
        }
    }
    
    std::string getDescription() const override { return "Humanize"; }

private:
    Clip& clip;
    std::vector<int> noteIds;
    int timingAmount;
    int velocityAmount;
    std::vector<std::tuple<int64_t, int64_t, int>> originalStates; // (startTick, endTick, velocity)
};

} // namespace pianodaw
