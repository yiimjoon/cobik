#include "VelocityLane.h"
#include "../../core/edit/UndoStack.h"
#include "../../core/edit/EditCommands.h"

namespace pianodaw {

VelocityLane::VelocityLane(Clip& clip_, UndoStack& undoStack_) 
    : clip(clip_), undoStack(undoStack_)
{
}

VelocityLane::~VelocityLane() {}

void VelocityLane::paint(juce::Graphics& g)
{
    juce::ScopedLock sl(clip.getLock());
    auto bounds = getLocalBounds();
    auto area = bounds.withLeft(KEYBOARD_WIDTH_OFFSET);
    
    juce::Graphics::ScopedSaveState ss(g);
    g.reduceClipRegion(area);

    // Background
    g.setColour(juce::Colour(0xff1f1f1f));
    g.fillRect(area);
    
    // Grid lines (velocity reference)
    g.setColour(juce::Colour(0xff2a2a2a));
    for (int vel = 0; vel <= 127; vel += 32)
    {
        int y = velocityToY(vel);
        g.drawHorizontalLine(y, (float)area.getX(), (float)area.getRight());
    }
    
    // Horizontal center line (64 velocity) - brighter
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawHorizontalLine(velocityToY(64), (float)area.getX(), (float)area.getRight());
    
    // Phase 5: Draw velocity bars (Cubase-style)
    auto& notes = clip.getNotes();
    for (const auto& note : notes)
    {
        if (note.endTick < viewStartTick || note.startTick > viewStartTick + 100000) continue;
        
        auto barRect = getNoteBarRect(note);
        if (barRect.getWidth() == 0) continue;
        
        bool isSelected = std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) != selectedNoteIds.end();
        
        // Color based on velocity intensity
        float velocityNorm = note.velocity / 127.0f;
        auto baseColor = isSelected 
            ? juce::Colours::orange.withBrightness(0.6f + velocityNorm * 0.4f)
            : juce::Colour(0xff4a9eff).withBrightness(0.4f + velocityNorm * 0.6f);
        
        // Bar fill
        g.setColour(baseColor.withAlpha(0.8f));
        g.fillRect(barRect);
        
        // Bar outline
        g.setColour(baseColor.brighter(0.3f));
        g.drawRect(barRect, isSelected ? 2.0f : 1.0f);
        
        // Top handle (for easier grabbing)
        if (isSelected)
        {
            g.setColour(juce::Colours::white);
            g.fillRect(barRect.getX(), barRect.getY() - 2, barRect.getWidth(), 4);
        }
    }
}

void VelocityLane::mouseDown(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (p.x < KEYBOARD_WIDTH_OFFSET) return;
    
    // Phase 5: Different behaviors based on edit mode
    if (editMode == EditMode::Draw)
    {
        // Draw mode: Edit individual notes
        activeNoteId = findNoteAtPosition(p);
        
        if (activeNoteId != -1)
        {
            juce::ScopedLock sl(clip.getLock());
            auto* note = clip.findNote(activeNoteId);
            if (note)
            {
                originalVelocity = note->velocity;
                batchEditNotes.clear();
                batchEditNotes.push_back({activeNoteId, originalVelocity});
                mouseDrag(e);
            }
        }
    }
    else if (editMode == EditMode::Line)
    {
        // Line mode: Start drawing a line
        batchEditNotes.clear();
        int newVel = yToVelocity(p.y);
        
        // Collect all selected notes for batch edit
        juce::ScopedLock sl(clip.getLock());
        for (int noteId : selectedNoteIds)
        {
            auto* note = clip.findNote(noteId);
            if (note)
            {
                batchEditNotes.push_back({noteId, note->velocity});
                note->velocity = newVel; // Preview
            }
        }
        repaint();
    }
    else if (editMode == EditMode::Ramp)
    {
        // Ramp mode: Remember start position
        batchEditNotes.clear();
        
        juce::ScopedLock sl(clip.getLock());
        for (int noteId : selectedNoteIds)
        {
            auto* note = clip.findNote(noteId);
            if (note)
            {
                batchEditNotes.push_back({noteId, note->velocity});
            }
        }
    }
}

void VelocityLane::mouseDrag(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (p.x < KEYBOARD_WIDTH_OFFSET) return;
    
    int newVel = yToVelocity(p.y);
    
    if (editMode == EditMode::Draw)
    {
        // Draw mode: Single note edit
        if (activeNoteId == -1) return;
        
        juce::ScopedLock sl(clip.getLock());
        auto* note = clip.findNote(activeNoteId);
        if (note)
        {
            note->velocity = newVel;
            repaint();
        }
    }
    else if (editMode == EditMode::Line)
    {
        // Line mode: Set all selected notes to same value
        juce::ScopedLock sl(clip.getLock());
        for (auto& pair : batchEditNotes)
        {
            auto* note = clip.findNote(pair.first);
            if (note)
            {
                note->velocity = newVel;
            }
        }
        repaint();
    }
    else if (editMode == EditMode::Ramp)
    {
        // Ramp mode: Create gradient between start and current position
        if (batchEditNotes.empty()) return;
        
        int startVel = yToVelocity(e.mouseDownPosition.y);
        int endVel = newVel;
        
        // Sort notes by tick
        juce::ScopedLock sl(clip.getLock());
        std::vector<std::pair<int64_t, int>> sortedNotes; // (tick, noteId)
        
        for (auto& pair : batchEditNotes)
        {
            auto* note = clip.findNote(pair.first);
            if (note)
            {
                sortedNotes.push_back({note->startTick, pair.first});
            }
        }
        
        std::sort(sortedNotes.begin(), sortedNotes.end());
        
        // Apply ramp
        for (size_t i = 0; i < sortedNotes.size(); ++i)
        {
            auto* note = clip.findNote(sortedNotes[i].second);
            if (note)
            {
                float t = sortedNotes.size() > 1 ? (float)i / (sortedNotes.size() - 1) : 0.5f;
                int rampVel = (int)(startVel + t * (endVel - startVel));
                note->velocity = std::max(1, std::min(127, rampVel));
            }
        }
        repaint();
    }
}

void VelocityLane::mouseUp(const juce::MouseEvent& e)
{
    if (batchEditNotes.empty()) return;
    
    // Phase 5: Batch velocity editing with undo support
    bool hasChanges = false;
    std::vector<std::pair<int, int>> finalChanges; // (noteId, finalVelocity)
    
    {
        juce::ScopedLock sl(clip.getLock());
        
        // Check which notes changed and collect final values
        for (auto& pair : batchEditNotes)
        {
            auto* note = clip.findNote(pair.first);
            if (note && note->velocity != pair.second)
            {
                finalChanges.push_back({pair.first, note->velocity});
                // Restore original value
                note->velocity = pair.second;
                hasChanges = true;
            }
        }
    }
    
    // Execute commands for changed notes
    if (hasChanges)
    {
        for (auto& change : finalChanges)
        {
            std::vector<int> noteIds = {change.first};
            undoStack.execute(std::make_unique<SetVelocityCommand>(clip, noteIds, change.second));
        }
    }
    
    batchEditNotes.clear();
    activeNoteId = -1;
    repaint();
}

void VelocityLane::mouseMove(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (p.x < KEYBOARD_WIDTH_OFFSET) return;
    
    int noteId = findNoteAtPosition(p);
    if (noteId != -1)
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void VelocityLane::setViewRange(int64_t startTick, double ppt)
{
    viewStartTick = startTick;
    pixelsPerTick = ppt;
    repaint();
}

int VelocityLane::tickToX(int64_t tick) const
{
    return KEYBOARD_WIDTH_OFFSET + (int)((tick - viewStartTick) * pixelsPerTick);
}

int64_t VelocityLane::xToTick(int x) const
{
    return viewStartTick + (int64_t)((x - KEYBOARD_WIDTH_OFFSET) / pixelsPerTick);
}

int VelocityLane::velocityToY(int velocity) const
{
    auto bounds = getLocalBounds().reduced(5);
    float norm = 1.0f - (velocity / 127.0f);
    return bounds.getY() + (int)(norm * bounds.getHeight());
}

int VelocityLane::yToVelocity(int y) const
{
    auto bounds = getLocalBounds().reduced(5);
    float norm = 1.0f - ((float)(y - bounds.getY()) / bounds.getHeight());
    return std::max(1, std::min(127, (int)(norm * 127.0f)));
}

// Phase 5: Helper methods
int VelocityLane::findNoteAtPosition(juce::Point<int> p)
{
    juce::ScopedLock sl(clip.getLock());
    auto& notes = clip.getNotes();
    
    for (const auto& note : notes)
    {
        auto barRect = getNoteBarRect(note);
        if (barRect.contains(p))
        {
            return note.id;
        }
    }
    
    return -1;
}

juce::Rectangle<int> VelocityLane::getNoteBarRect(const Note& note) const
{
    int x = tickToX(note.startTick);
    int width = std::max(3, (int)((note.endTick - note.startTick) * pixelsPerTick));
    int y = velocityToY(note.velocity);
    int bottom = getLocalBounds().getBottom() - 5;
    int height = bottom - y;
    
    return juce::Rectangle<int>(x, y, width, height);
}

void VelocityLane::setSelectedNotes(const std::vector<int>& noteIds)
{
    selectedNoteIds = noteIds;
    repaint();
}

} // namespace pianodaw
