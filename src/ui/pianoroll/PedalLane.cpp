#include "PedalLane.h"
#include "../../core/edit/UndoStack.h"
#include "../../core/edit/EditCommands.h"

namespace pianodaw {

PedalLane::PedalLane(Clip& clip_, UndoStack& undoStack_) 
    : clip(clip_), undoStack(undoStack_)
{
}

PedalLane::~PedalLane() {}

void PedalLane::paint(juce::Graphics& g)
{
    juce::ScopedLock sl(clip.getLock());
    auto bounds = getLocalBounds();
    auto area = bounds.withLeft(KEYBOARD_WIDTH_OFFSET);
    
    juce::Graphics::ScopedSaveState ss(g);
    g.reduceClipRegion(area);

    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(area);
    
    // Horizontal center line
    g.setColour(juce::Colour(0xff2a2a2a));
    int centerY = bounds.getCentreY();
    g.drawHorizontalLine(centerY, (float)area.getX(), (float)area.getRight());
    
    // Phase 5: Draw CC64 events as bars (like velocity lane style)
    auto& events = clip.getCCEvents();
    
    // Find sustain pedal events (CC64)
    std::vector<CCEvent*> pedalEvents;
    for (auto& e : events) {
        if (e.cc == 64) pedalEvents.push_back(&e);
    }
    
    // Draw pedal ON regions as bars
    for (size_t i = 0; i < pedalEvents.size(); ++i)
    {
        auto* current = pedalEvents[i];
        bool isOn = current->value >= 64;
        
        if (!isOn) continue; // Only draw ON states
        
        int x1 = tickToX(current->tick);
        int x2 = bounds.getRight();
        
        // Find next event to determine bar width
        for (size_t j = i + 1; j < pedalEvents.size(); ++j)
        {
            if (pedalEvents[j]->value < 64) // Found OFF event
            {
                x2 = tickToX(pedalEvents[j]->tick);
                break;
            }
        }
        
        // Culling
        if (x2 < area.getX() || x1 > area.getRight()) continue;
        
        int width = x2 - x1;
        int barTop = centerY + 5;
        int barHeight = bounds.getBottom() - barTop - 5;
        
        // Bar fill
        g.setColour(juce::Colours::green.withAlpha(0.7f));
        g.fillRect(x1, barTop, width, barHeight);
        
        // Bar outline
        g.setColour(juce::Colours::green);
        g.drawRect((float)x1, (float)barTop, (float)width, (float)barHeight, 2.0f);
    }
    
    // Phase 5: Draw drag preview
    if (isDragging)
    {
        int x1 = tickToX(std::min(dragStartTick, dragEndTick));
        int x2 = tickToX(std::max(dragStartTick, dragEndTick));
        int barTop = centerY + 5;
        int barHeight = bounds.getBottom() - barTop - 5;
        
        if (currentEditMode == EditMode::Add)
        {
            g.setColour(juce::Colours::green.withAlpha(0.4f));
            g.fillRect(x1, barTop, x2 - x1, barHeight);
            g.setColour(juce::Colours::green.withAlpha(0.8f));
            g.drawRect((float)x1, (float)barTop, (float)(x2 - x1), (float)barHeight, 1.0f);
        }
        else
        {
            g.setColour(juce::Colours::red.withAlpha(0.4f));
            g.fillRect(x1, barTop, x2 - x1, barHeight);
        }
    }
}

void PedalLane::mouseDown(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (p.x < KEYBOARD_WIDTH_OFFSET) return;
    
    int64_t tick = xToTick(p.x);
    
    // Phase 5: Snap to grid
    if (snapEnabled)
    {
        int64_t gridTicks = gridSizeToTicks(currentGridSize);
        tick = PPQ::snapToGrid(tick, gridTicks);
    }
    
    // Determine edit mode based on vertical position
    currentEditMode = (p.y < getLocalBounds().getCentreY()) ? EditMode::Remove : EditMode::Add;
    
    dragStartTick = tick;
    dragEndTick = tick;
    isDragging = true;
    
    repaint();
}

void PedalLane::mouseDrag(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (p.x < KEYBOARD_WIDTH_OFFSET || !isDragging) return;
    
    int64_t tick = xToTick(p.x);
    
    // Snap to grid
    if (snapEnabled)
    {
        int64_t gridTicks = gridSizeToTicks(currentGridSize);
        tick = PPQ::snapToGrid(tick, gridTicks);
    }
    
    dragEndTick = tick;
    repaint();
}

void PedalLane::mouseUp(const juce::MouseEvent& e)
{
    if (!isDragging) return;
    
    int64_t startTick = std::min(dragStartTick, dragEndTick);
    int64_t endTick = std::max(dragStartTick, dragEndTick);
    
    if (currentEditMode == EditMode::Add)
    {
        // Add pedal ON at start, pedal OFF at end
        undoStack.execute(std::make_unique<AddCCEventCommand>(clip, 64, startTick, 127));
        if (endTick != startTick)
        {
            undoStack.execute(std::make_unique<AddCCEventCommand>(clip, 64, endTick, 0));
        }
    }
    else // Remove
    {
        // Remove all pedal events in range
        undoStack.execute(std::make_unique<RemoveCCEventCommand>(clip, startTick, 64));
        if (endTick != startTick)
        {
            undoStack.execute(std::make_unique<RemoveCCEventCommand>(clip, endTick, 64));
        }
    }
    
    isDragging = false;
    repaint();
}

void PedalLane::mouseMove(const juce::MouseEvent& e)
{
    auto p = e.getPosition();
    if (p.x < KEYBOARD_WIDTH_OFFSET) return;
    
    if (p.y < getLocalBounds().getCentreY())
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
}

void PedalLane::setViewRange(int64_t startTick, double ppt)
{
    viewStartTick = startTick;
    pixelsPerTick = ppt;
    repaint();
}

int PedalLane::tickToX(int64_t tick) const
{
    return KEYBOARD_WIDTH_OFFSET + (int)((tick - viewStartTick) * pixelsPerTick);
}

int64_t PedalLane::xToTick(int x) const
{
    return viewStartTick + (int64_t)((x - KEYBOARD_WIDTH_OFFSET) / pixelsPerTick);
}

} // namespace pianodaw
