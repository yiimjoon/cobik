#include "PianoRollView.h"
#include "../../core/audio/AudioEngine.h"
#include "../../core/timeline/Transport.h"
#include "../../core/edit/UndoStack.h"
#include "../../core/edit/EditCommands.h"
#include <algorithm>

namespace pianodaw {

PianoRollView::PianoRollView(Clip& clip_, UndoStack& undoStack_, Transport& transport_, AudioEngine& engine_)
    : clip(clip_),
      undoStack(undoStack_),
      transport(transport_),
      audioEngine(engine_),
      currentGridSize(GridSize::Sixteenth),
      pixelsPerTick(0.1),
      pixelsPerKey(16.0),
      viewStartTick(0),
      viewEndTick(PPQ::TICKS_PER_QUARTER * 16),
      viewStartPitch(96) // C5 at top (shows C5 down to ~C3)
{
    setOpaque(true);
    setWantsKeyboardFocus(true);
}

PianoRollView::~PianoRollView() {}

bool PianoRollView::keyPressed(const juce::KeyPress& key)
{
    if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        if (!selectedNoteIds.empty())
        {
            deleteSelectedNotes();
            return true;
        }
    }
    return false;
}

void PianoRollView::paint(juce::Graphics& g)
{
    juce::ScopedLock sl(clip.getLock());
    auto bounds = getLocalBounds();
    g.fillAll(juce::Colour(0xff2b2b2b));
    
    auto keyboardArea = bounds.removeFromLeft(KEYBOARD_WIDTH);
    auto gridArea = bounds;
    
    paintNoteGrid(g, gridArea);
    paintNotes(g, gridArea);
    paintPianoKeyboard(g, keyboardArea);
    
    // Playhead
    int64_t playheadTick = transport.getPosition();
    if (playheadTick >= viewStartTick && playheadTick <= viewEndTick)
    {
        int x = tickToX(playheadTick);
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(x, (float)gridArea.getY(), (float)gridArea.getBottom());
    }
    
    // Lasso
    if (mouseMode == MouseMode::Lasso)
    {
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(lassoRect);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRect(lassoRect);
    }
}

void PianoRollView::paintPianoKeyboard(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(area);
    
    for (int key = HIGHEST_KEY; key >= LOWEST_KEY; --key)
    {
        int y = keyToY(key);
        int h = static_cast<int>(pixelsPerKey);
        
        if (isBlackKey(key)) g.setColour(juce::Colour(0xff0a0a0a));
        else g.setColour(juce::Colour(0xffffffff));
        
        if (key == lastPlayedKey)
            g.setColour(juce::Colours::orange);

        g.fillRect(area.getX(), y, area.getWidth(), h);
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawHorizontalLine(y, (float)area.getX(), (float)area.getRight());
        
        if (key % 12 == 0)
        {
            g.setColour(key == lastPlayedKey ? juce::Colours::black : juce::Colour(0xff888888));
            g.setFont(10.0f);
            g.drawText(getKeyName(key), area.getX() + 2, y, area.getWidth() - 4, h, juce::Justification::centredLeft, false);
        }
    }
}

void PianoRollView::paintNoteGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    for (int key = HIGHEST_KEY; key >= LOWEST_KEY; --key)
    {
        int y = keyToY(key);
        int h = static_cast<int>(pixelsPerKey);
        
        bool inScale = MusicTheory::isPitchInScale(key, scaleInfo.rootNote, scaleInfo.type);
        
        if (inScale)
            g.setColour(isBlackKey(key) ? juce::Colour(0xff222222) : juce::Colour(0xff2b2b2b));
        else
            g.setColour(juce::Colour(0xff1a1a1a)); 
            
        g.fillRect(area.getX(), y, area.getWidth(), h);
    }
    
    g.setColour(juce::Colour(0xff3a3a3a));
    for (int key = HIGHEST_KEY; key >= LOWEST_KEY; --key)
        g.drawHorizontalLine(keyToY(key), (float)area.getX(), (float)area.getRight());
    
    int64_t gridTicks = gridSizeToTicks(currentGridSize);
    for (int64_t tick = (viewStartTick/gridTicks)*gridTicks; tick <= viewEndTick; tick += gridTicks)
    {
        int x = tickToX(tick);
        if (x < area.getX() || x > area.getRight()) continue;
        
        bool isBar = (tick % (PPQ::TICKS_PER_QUARTER * 4)) == 0;
        bool isBeat = (tick % PPQ::TICKS_PER_QUARTER) == 0;
        g.setColour(isBar ? juce::Colour(0xff5a5a5a) : (isBeat ? juce::Colour(0xff4a4a4a) : juce::Colour(0xff333333)));
        g.drawVerticalLine(x, (float)area.getY(), (float)area.getBottom());
    }
}

void PianoRollView::paintNotes(juce::Graphics& g, juce::Rectangle<int> area)
{
    for (const auto& note : clip.getNotes())
    {
        if (note.endTick < viewStartTick || note.startTick > viewEndTick) continue;
        auto rect = noteToRect(note, area);
        bool isSelected = std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) != selectedNoteIds.end();
        float velocityNorm = note.velocity / 127.0f;
        auto baseColor = isSelected ? juce::Colours::white : juce::Colour(0xff4a9eff).withBrightness(0.5f + velocityNorm * 0.5f);
        g.setColour(baseColor);
        g.fillRoundedRectangle(rect.toFloat(), 3.0f);
        g.setColour(baseColor.brighter(0.3f));
        g.drawRoundedRectangle(rect.toFloat(), 3.0f, isSelected ? 2.0f : 1.0f);
    }
    
    // Phase 4.5: Ghost rendering during move
    if (mouseMode == MouseMode::Move && !selectedNoteIds.empty())
    {
        int64_t currentTick = xToTick(lastMousePos.x);
        int currentPitch = yToKey(lastMousePos.y);
        int64_t deltaTicks = currentTick - mouseStartTick;
        int deltaPitch = currentPitch - mouseStartPitch;
        
        // Apply snap to deltaTicks if snap is enabled
        if (snapEnabled && deltaTicks != 0)
        {
            int64_t gridTicks = gridSizeToTicks(currentGridSize);
            // Round delta to nearest grid
            deltaTicks = ((deltaTicks + gridTicks / 2) / gridTicks) * gridTicks;
        }
        
        if (deltaTicks != 0 || deltaPitch != 0)
        {
            // Draw ghost images of selected notes at new position
            for (const auto& note : clip.getNotes())
            {
                if (std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) != selectedNoteIds.end())
                {
                    Note ghostNote = note;
                    ghostNote.startTick += deltaTicks;
                    ghostNote.endTick += deltaTicks;
                    ghostNote.pitch += deltaPitch;
                    
                    // Clamp pitch to valid range
                    if (ghostNote.pitch < 0 || ghostNote.pitch > 127) continue;
                    
                    auto ghostRect = noteToRect(ghostNote, area);
                    g.setColour(juce::Colours::white.withAlpha(0.3f));
                    g.fillRoundedRectangle(ghostRect.toFloat(), 3.0f);
                    g.setColour(juce::Colours::white.withAlpha(0.5f));
                    g.drawRoundedRectangle(ghostRect.toFloat(), 3.0f, 1.0f);
                }
            }
        }
    }
}

void PianoRollView::resized()
{
    updateViewEndTick();
    if (onViewChanged) onViewChanged(viewStartTick, pixelsPerTick);
}

void PianoRollView::mouseDown(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    mouseStartPosition = p;

    // Pan is always available with middle mouse button
    if (e.mods.isMiddleButtonDown())
    {
        mouseMode = MouseMode::Pan;
        panStartTick = viewStartTick;
        panStartPitch = viewStartPitch;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        return;
    }

    // Keyboard preview is always available
    if (p.x < KEYBOARD_WIDTH)
    {
        int key = yToKey(p.y);
        if (key >= LOWEST_KEY && key <= HIGHEST_KEY)
        {
            lastPlayedKey = key;
            audioEngine.handleNoteOn(key, 0.8f);
            repaint();
        }
        return;
    }

    mouseStartTick = xToTick(p.x);
    mouseStartPitch = yToKey(p.y);
    activeNoteId = findNoteAt(p);
    
    // Phase 2: Tool-specific behavior
    switch (currentTool)
    {
        case EditorTool::Select:
            handleSelectToolMouseDown(p, e);
            break;
            
        case EditorTool::Draw:
            handleDrawToolMouseDown(p, e);
            break;
            
        case EditorTool::Erase:
            handleEraseToolMouseDown(p, e);
            break;
            
        case EditorTool::Split:
            handleSplitToolMouseDown(p, e);
            break;
            
        case EditorTool::Line:
            // Line tool is for Controller lanes, not implemented yet
            break;
            
        default:
            handleSelectToolMouseDown(p, e);
            break;
    }
    
    repaint();
}

// Phase 2: Tool-specific mouse handlers
void PianoRollView::handleSelectToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e)
{
    if (activeNoteId != -1)
    {
        selectNote(activeNoteId, e.mods.isShiftDown());
        auto* note = clip.findNote(activeNoteId);
        int noteX = tickToX(note->startTick);
        int noteW = static_cast<int>((note->endTick - note->startTick) * pixelsPerTick);
        if (p.x > noteX + noteW - 10)
        {
            mouseMode = MouseMode::ResizeEnd;
            resizeOriginalStartTick = note->startTick;
            resizeOriginalEndTick = note->endTick;
        }
        else if (p.x < noteX + 10)
        {
            mouseMode = MouseMode::ResizeStart;
            resizeOriginalStartTick = note->startTick;
            resizeOriginalEndTick = note->endTick;
        }
        else mouseMode = MouseMode::Move;
    }
    else
    {
        if (!e.mods.isShiftDown()) clearSelection();
        mouseMode = MouseMode::Lasso;
        lassoRect = juce::Rectangle<int>(p.x, p.y, 0, 0);
    }
}

void PianoRollView::handleDrawToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e)
{
    // Draw Tool: Prepare to create note on drag (not on click)
    if (activeNoteId == -1)
    {
        mouseMode = MouseMode::DrawPending;
        // Note will be created in mouseDrag if user drags
    }
}

void PianoRollView::handleEraseToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e)
{
    // Erase Tool: Delete note on click
    if (activeNoteId != -1)
    {
        undoStack.execute(std::make_unique<RemoveNoteCommand>(clip, activeNoteId));
        activeNoteId = -1;
    }
}

void PianoRollView::handleSplitToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e)
{
    // Split Tool: Split note at click position
    if (activeNoteId != -1)
    {
        int64_t splitTick = xToTick(p.x);
        auto* note = clip.findNote(activeNoteId);
        
        // Only split if click is inside the note
        if (splitTick > note->startTick && splitTick < note->endTick)
        {
            // Snap split point if enabled
            if (snapEnabled)
            {
                int64_t gridTicks = gridSizeToTicks(currentGridSize);
                splitTick = PPQ::snapToGrid(splitTick, gridTicks);
            }
            
            // Create split command (TODO: implement SplitNoteCommand)
            // For now, manually split:
            int pitch = note->pitch;
            int velocity = note->velocity;
            int64_t originalStart = note->startTick;
            int64_t originalEnd = note->endTick;
            
            // Remove original note
            undoStack.execute(std::make_unique<RemoveNoteCommand>(clip, activeNoteId));
            
            // Add two new notes
            undoStack.execute(std::make_unique<AddNoteCommand>(clip, pitch, originalStart, splitTick, velocity));
            undoStack.execute(std::make_unique<AddNoteCommand>(clip, pitch, splitTick, originalEnd, velocity));
        }
        
        activeNoteId = -1;
    }
}

void PianoRollView::mouseDrag(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    lastMousePos = p; // Phase 4.5: Track mouse position for ghost rendering
    
    if (mouseMode == MouseMode::Pan)
    {
        int dx = p.x - mouseStartPosition.x;
        int dy = p.y - mouseStartPosition.y;
        int64_t tickOffset = (int64_t)(dx / pixelsPerTick);
        int pitchOffset = (int)(dy / pixelsPerKey);
        viewStartTick = std::max((int64_t)0, panStartTick - tickOffset);
        viewStartPitch = std::max(LOWEST_KEY, std::min(HIGHEST_KEY, panStartPitch + pitchOffset));
        updateViewEndTick();
        if (onViewChanged) onViewChanged(viewStartTick, pixelsPerTick);
        repaint();
        return;
    }

    if (e.mouseDownPosition.getX() < KEYBOARD_WIDTH)
    {
        int key = yToKey(p.y);
        if (key != lastPlayedKey && key >= LOWEST_KEY && key <= HIGHEST_KEY)
        {
            if (lastPlayedKey != -1) audioEngine.handleNoteOff(lastPlayedKey);
            lastPlayedKey = key;
            audioEngine.handleNoteOn(key, 0.8f);
            repaint();
        }
        return;
    }

    if (mouseMode == MouseMode::DrawPending)
    {
        // Draw Tool: Create note on first drag movement
        int64_t tick = xToTick(mouseStartPosition.x);
        int pitch = yToKey(mouseStartPosition.y);
        
        // Snap to grid if enabled
        if (snapEnabled)
        {
            int64_t gridTicks = gridSizeToTicks(currentGridSize);
            tick = PPQ::snapToGrid(tick, gridTicks);
        }
        
        // Start with minimum length
        int64_t gridTicks = gridSizeToTicks(currentGridSize);
        int64_t endTick = tick + gridTicks;
        
        undoStack.execute(std::make_unique<AddNoteCommand>(clip, pitch, tick, endTick, 100));
        
        // Find the newly created note and switch to resize mode
        {
            juce::ScopedLock sl(clip.getLock());
            auto& notes = clip.getNotes();
            if (!notes.empty())
            {
                const auto& newNote = notes.back();
                activeNoteId = newNote.id;
                mouseMode = MouseMode::ResizeEnd;
                resizeOriginalStartTick = newNote.startTick;
                resizeOriginalEndTick = newNote.endTick;
            }
        }
    }
    else if (mouseMode == MouseMode::Move)
    {
        // Visual feedback during move (actual move happens in mouseUp)
        repaint();
    }
    else if (mouseMode == MouseMode::ResizeEnd)
    {
        // Phase 4.5: Resize note end
        if (activeNoteId != -1)
        {
            auto* note = clip.findNote(activeNoteId);
            if (note)
            {
                int64_t newEndTick = xToTick(p.x);
                
                // Apply snap if enabled
                if (snapEnabled)
                {
                    int64_t gridTicks = gridSizeToTicks(currentGridSize);
                    newEndTick = PPQ::snapToGrid(newEndTick, gridTicks);
                }
                
                // Ensure minimum length
                if (newEndTick > note->startTick + 60)
                {
                    note->endTick = newEndTick;
                }
                repaint();
            }
        }
    }
    else if (mouseMode == MouseMode::ResizeStart)
    {
        // Phase 4.5: Resize note start
        if (activeNoteId != -1)
        {
            auto* note = clip.findNote(activeNoteId);
            if (note)
            {
                int64_t newStartTick = xToTick(p.x);
                
                // Apply snap if enabled
                if (snapEnabled)
                {
                    int64_t gridTicks = gridSizeToTicks(currentGridSize);
                    newStartTick = PPQ::snapToGrid(newStartTick, gridTicks);
                }
                
                // Ensure minimum length
                if (newStartTick < note->endTick - 60)
                {
                    note->startTick = newStartTick;
                }
                repaint();
            }
        }
    }
    else if (mouseMode == MouseMode::Lasso)
    {
        lassoRect = juce::Rectangle<int>::leftTopRightBottom(
            std::min(mouseStartPosition.x, p.x), std::min(mouseStartPosition.y, p.y),
            std::max(mouseStartPosition.x, p.x), std::max(mouseStartPosition.y, p.y));
        repaint();
    }
}

void PianoRollView::mouseUp(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (mouseMode == MouseMode::Pan) setMouseCursor(juce::MouseCursor::NormalCursor);
    
    if (lastPlayedKey != -1)
    {
        audioEngine.handleNoteOff(lastPlayedKey);
        lastPlayedKey = -1;
        repaint();
    }

    if (mouseMode == MouseMode::Lasso)
    {
        juce::ScopedLock sl(clip.getLock());
        auto& notes = clip.getNotes();
        for (const auto& note : notes)
        {
            auto rect = noteToRect(note, getLocalBounds());
            if (lassoRect.intersects(rect))
            {
                if (std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) == selectedNoteIds.end())
                    selectedNoteIds.push_back(note.id);
            }
        }
    }
    else if (mouseMode == MouseMode::Move)
    {
        int64_t currentTick = xToTick(p.x);
        int currentPitch = yToKey(p.y);
        int64_t deltaTicks = currentTick - mouseStartTick;
        int deltaPitch = currentPitch - mouseStartPitch;
        
        // Apply snap to deltaTicks if snap is enabled
        if (snapEnabled && deltaTicks != 0)
        {
            int64_t gridTicks = gridSizeToTicks(currentGridSize);
            // Round delta to nearest grid
            deltaTicks = ((deltaTicks + gridTicks / 2) / gridTicks) * gridTicks;
        }
        
        if (deltaTicks != 0 || deltaPitch != 0)
            undoStack.execute(std::make_unique<MoveNoteCommand>(clip, selectedNoteIds, deltaTicks, deltaPitch));
    }
    else if (mouseMode == MouseMode::ResizeEnd || mouseMode == MouseMode::ResizeStart)
    {
        // Phase 4.5: Apply resize command if note was resized
        if (activeNoteId != -1)
        {
            auto* note = clip.findNote(activeNoteId);
            if (note && (note->startTick != resizeOriginalStartTick || note->endTick != resizeOriginalEndTick))
            {
                // Restore original state first
                int64_t newStartTick = note->startTick;
                int64_t newEndTick = note->endTick;
                note->startTick = resizeOriginalStartTick;
                note->endTick = resizeOriginalEndTick;
                
                // Execute resize command through undo system
                undoStack.execute(std::make_unique<ResizeNoteCommand>(clip, activeNoteId, newStartTick, newEndTick));
            }
        }
    }
    
    mouseMode = MouseMode::None;
    activeNoteId = -1;
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

void PianoRollView::mouseMove(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    
    // Skip keyboard area
    if (p.x < KEYBOARD_WIDTH)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }
    
    // Update cursor based on tool and hover state
    if (currentTool == EditorTool::Select)
    {
        // Check if hovering over note edge for resize cursor
        int noteId = findNoteAt(p);
        if (noteId != -1)
        {
            auto* note = clip.findNote(noteId);
            if (note)
            {
                int noteX = tickToX(note->startTick);
                int noteW = static_cast<int>((note->endTick - note->startTick) * pixelsPerTick);
                
                // Show resize cursor (I-beam) near edges
                if (p.x > noteX + noteW - 10 || p.x < noteX + 10)
                {
                    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                }
                else
                {
                    setMouseCursor(juce::MouseCursor::NormalCursor);
                }
            }
        }
        else
        {
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }
    else if (currentTool == EditorTool::Draw)
    {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
    else if (currentTool == EditorTool::Erase)
    {
        setMouseCursor(juce::MouseCursor::CrosshairCursor); // Use crosshair instead of NoCursor
    }
    else if (currentTool == EditorTool::Split)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void PianoRollView::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (p.x < KEYBOARD_WIDTH) return;
    
    int64_t tick = xToTick(p.x);
    int pitch = yToKey(p.y);
    int64_t gridTicks = gridSizeToTicks(currentGridSize);
    int64_t snappedTick = PPQ::snapToGrid(tick, gridTicks);

    if (chordMode.type == MusicTheory::ChordType::None)
    {
        undoStack.execute(std::make_unique<AddNoteCommand>(clip, pitch, snappedTick, snappedTick + gridTicks, 100));
    }
    else
    {
        auto intervals = MusicTheory::getChordIntervals(chordMode.type);
        std::vector<Note> chordNotes;
        for (int interval : intervals)
        {
            Note n;
            n.pitch = pitch + interval;
            n.startTick = snappedTick;
            n.endTick = snappedTick + gridTicks;
            n.velocity = 0.8f;
            chordNotes.push_back(n);
        }
        undoStack.execute(std::make_unique<BulkAddNotesCommand>(clip, chordNotes));
    }
    repaint();
}

void PianoRollView::setScale(int root, MusicTheory::ScaleType type)
{
    scaleInfo.rootNote = root;
    scaleInfo.type = type;
    repaint();
}

void PianoRollView::setChordMode(MusicTheory::ChordType type)
{
    chordMode.type = type;
}

// Phase 2: Tool System
void PianoRollView::setCurrentTool(EditorTool tool)
{
    currentTool = tool;
    
    // Update mouse cursor based on tool
    switch (tool)
    {
        case EditorTool::Select:
            setMouseCursor(juce::MouseCursor::NormalCursor);
            break;
        case EditorTool::Draw:
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            break;
        case EditorTool::Erase:
            setMouseCursor(juce::MouseCursor::CrosshairCursor); // Use crosshair for eraser
            break;
        case EditorTool::Split:
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
            break;
        case EditorTool::Line:
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            break;
        default:
            setMouseCursor(juce::MouseCursor::NormalCursor);
            break;
    }
    
    repaint();
}

Note* PianoRollView::getNoteAt(juce::Point<int> pos)
{
    int id = findNoteAt(pos);
    if (id == -1) return nullptr;
    return clip.findNote(id);
}

void PianoRollView::deleteSelectedNotes()
{
    if (selectedNoteIds.empty()) return;
    for (int id : selectedNoteIds)
        undoStack.execute(std::make_unique<RemoveNoteCommand>(clip, id));
    clearSelection();
}

void PianoRollView::setGridSize(GridSize size) { currentGridSize = size; repaint(); }

void PianoRollView::selectNote(int id, bool additive)
{
    bool alreadySelected = std::find(selectedNoteIds.begin(), selectedNoteIds.end(), id) != selectedNoteIds.end();
    if (additive)
    {
        if (alreadySelected) selectedNoteIds.erase(std::remove(selectedNoteIds.begin(), selectedNoteIds.end(), id), selectedNoteIds.end());
        else selectedNoteIds.push_back(id);
    }
    else
    {
        if (alreadySelected && selectedNoteIds.size() == 1) return;
        selectedNoteIds.clear();
        selectedNoteIds.push_back(id);
    }
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

void PianoRollView::clearSelection()
{
    if (selectedNoteIds.empty()) return;
    selectedNoteIds.clear();
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

int PianoRollView::findNoteAt(juce::Point<int> p) const
{
    juce::ScopedLock sl(clip.getLock());
    auto& notes = clip.getNotes();
    for (int i = (int)notes.size() - 1; i >= 0; --i)
        if (noteToRect(notes[i], getLocalBounds().withLeft(KEYBOARD_WIDTH)).contains(p)) return notes[i].id;
    return -1;
}

int PianoRollView::tickToX(int64_t tick) const { return KEYBOARD_WIDTH + (int)((tick - viewStartTick) * pixelsPerTick); }
int64_t PianoRollView::xToTick(int x) const { return viewStartTick + (int64_t)((x - KEYBOARD_WIDTH) / pixelsPerTick); }

void PianoRollView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCtrlDown())
    {
        // Ctrl+Wheel: Horizontal zoom
        double zoomFactor = 1.0 + (wheel.deltaY * 0.1);
        pixelsPerTick *= zoomFactor;
        pixelsPerTick = std::max(0.01, std::min(2.0, pixelsPerTick)); // Clamp zoom range
        updateViewEndTick();
        if (onViewChanged) onViewChanged(viewStartTick, pixelsPerTick);
    }
    else if (e.mods.isAltDown())
    {
        // Alt+Wheel: Horizontal scroll
        int64_t scrollAmount = (int64_t)(wheel.deltaY * 1000);
        viewStartTick -= scrollAmount;
        viewStartTick = std::max(0LL, viewStartTick);
        updateViewEndTick();
        if (onViewChanged) onViewChanged(viewStartTick, pixelsPerTick);
    }
    else
    {
        // Wheel: Vertical scroll (viewStartPitch = top visible key)
        int scrollAmount = (int)(wheel.deltaY * 3);
        viewStartPitch -= scrollAmount; // Invert: wheel up = scroll up (higher notes)
        
        // Calculate visible range
        int visibleKeys = (int)(getHeight() / pixelsPerKey);
        int minViewStart = LOWEST_KEY; // Can show from C0
        int maxViewStart = HIGHEST_KEY; // Can show up to C8
        
        viewStartPitch = std::max(minViewStart, std::min(maxViewStart, viewStartPitch));
    }
    
    repaint();
}

void PianoRollView::scrollBarMoved(juce::ScrollBar*, double) {}

int PianoRollView::keyToY(int midiKey) const { return (int)((viewStartPitch - midiKey) * pixelsPerKey); }
int PianoRollView::yToKey(int y) const { return viewStartPitch - (int)(y / pixelsPerKey); }

void PianoRollView::updateViewEndTick()
{
    int gridWidth = getWidth() - KEYBOARD_WIDTH - 20;
    viewEndTick = viewStartTick + (int64_t)(gridWidth / pixelsPerTick);
}

juce::Rectangle<int> PianoRollView::noteToRect(const Note& note, juce::Rectangle<int> gridArea) const
{
    return { tickToX(note.startTick), keyToY(note.pitch), std::max(2, (int)((note.endTick - note.startTick) * pixelsPerTick)), std::max(2, (int)pixelsPerKey - 2) };
}

bool PianoRollView::isBlackKey(int midiKey) const
{
    int n = midiKey % 12; return n==1||n==3||n==6||n==8||n==10;
}

juce::String PianoRollView::getKeyName(int midiKey) const
{
    const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    return juce::String(names[midiKey % 12]) + juce::String((midiKey / 12) - 1);
}

// ============= Phase 6: Advanced Selection =============

void PianoRollView::selectByPitchRange(int minPitch, int maxPitch, bool additive)
{
    if (!additive) clearSelection();
    
    juce::ScopedLock sl(clip.getLock());
    auto& notes = clip.getNotes();
    
    for (const auto& note : notes)
    {
        if (note.pitch >= minPitch && note.pitch <= maxPitch)
        {
            if (std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) == selectedNoteIds.end())
            {
                selectedNoteIds.push_back(note.id);
            }
        }
    }
    
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

void PianoRollView::selectByVelocityRange(int minVel, int maxVel, bool additive)
{
    if (!additive) clearSelection();
    
    juce::ScopedLock sl(clip.getLock());
    auto& notes = clip.getNotes();
    
    for (const auto& note : notes)
    {
        if (note.velocity >= minVel && note.velocity <= maxVel)
        {
            if (std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) == selectedNoteIds.end())
            {
                selectedNoteIds.push_back(note.id);
            }
        }
    }
    
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

void PianoRollView::selectByDuration(int64_t minTicks, int64_t maxTicks, bool additive)
{
    if (!additive) clearSelection();
    
    juce::ScopedLock sl(clip.getLock());
    auto& notes = clip.getNotes();
    
    for (const auto& note : notes)
    {
        int64_t duration = note.endTick - note.startTick;
        if (duration >= minTicks && duration <= maxTicks)
        {
            if (std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) == selectedNoteIds.end())
            {
                selectedNoteIds.push_back(note.id);
            }
        }
    }
    
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

void PianoRollView::selectEveryNth(int n, int offset)
{
    clearSelection();
    
    juce::ScopedLock sl(clip.getLock());
    auto& notes = clip.getNotes();
    
    // Sort notes by start time
    std::vector<const Note*> sortedNotes;
    for (const auto& note : notes)
    {
        sortedNotes.push_back(&note);
    }
    std::sort(sortedNotes.begin(), sortedNotes.end(), 
        [](const Note* a, const Note* b) { return a->startTick < b->startTick; });
    
    // Select every Nth note
    for (size_t i = offset; i < sortedNotes.size(); i += n)
    {
        selectedNoteIds.push_back(sortedNotes[i]->id);
    }
    
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

void PianoRollView::invertSelection()
{
    juce::ScopedLock sl(clip.getLock());
    auto& notes = clip.getNotes();
    
    std::vector<int> newSelection;
    for (const auto& note : notes)
    {
        bool isSelected = std::find(selectedNoteIds.begin(), selectedNoteIds.end(), note.id) != selectedNoteIds.end();
        if (!isSelected)
        {
            newSelection.push_back(note.id);
        }
    }
    
    selectedNoteIds = newSelection;
    if (onSelectionChanged) onSelectionChanged(selectedNoteIds);
    repaint();
}

} // namespace pianodaw
