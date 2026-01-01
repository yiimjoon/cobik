#include "ArrangementView.h"

namespace pianodaw {

ArrangementView::ArrangementView(Project& proj)
    : project(proj)
{
    setOpaque(true);
}

void ArrangementView::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2b2b2b));

    // Draw grid
    drawGrid(g);

    // Draw tracks
    drawTracks(g);
}

void ArrangementView::resized()
{
    // Viewport updates handled in paint
}

void ArrangementView::mouseDown(const juce::MouseEvent& event)
{
    auto hit = findClipRegionAt(event.x, event.y);
    
    selectedTrack = hit.track;
    selectedClipRegion = hit.clipRegion;
    
    if (selectedClipRegion) {
        isDraggingClip = true;
        dragStartX = event.x;
        dragStartTick = selectedClipRegion->startTick;
    }
    
    repaint();
}

void ArrangementView::mouseDoubleClick(const juce::MouseEvent& event)
{
    auto hit = findClipRegionAt(event.x, event.y);
    
    if (hit.clipRegion && onClipRegionDoubleClick) {
        onClipRegionDoubleClick(hit.track, hit.clipRegion);
    }
}

void ArrangementView::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingClip && selectedClipRegion) {
        // Calculate new position with snap to grid (snap to quarter notes for now)
        int deltaX = event.x - dragStartX;
        int deltaTicks = xToTicks(deltaX);
        int newStartTick = dragStartTick + deltaTicks;
        
        // Snap to quarter note (PPQ ticks)
        int snapTicks = 480; // Assuming 480 PPQ
        newStartTick = (newStartTick + snapTicks / 2) / snapTicks * snapTicks;
        
        // Clamp to valid range
        if (newStartTick >= 0) {
            selectedClipRegion->startTick = newStartTick;
            repaint();
        }
    }
}

void ArrangementView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isCtrlDown()) {
        // Ctrl+Wheel = Horizontal zoom
        double zoomFactor = 1.0 + wheel.deltaY * 0.5;
        setPixelsPerTick(pixelsPerTick * zoomFactor);
    }
    else if (event.mods.isAltDown()) {
        // Alt+Wheel = Horizontal scroll
        int scrollDelta = static_cast<int>(wheel.deltaY * 100);
        setViewportX(viewportX - scrollDelta);
    }
    else {
        // Wheel = Vertical scroll (handled by parent scroll container)
    }
}

void ArrangementView::setPixelsPerTick(double newPixelsPerTick)
{
    pixelsPerTick = juce::jlimit(0.01, 1.0, newPixelsPerTick);
    repaint();
}

void ArrangementView::setViewportX(int newX)
{
    viewportX = juce::jmax(0, newX);
    repaint();
}

int ArrangementView::ticksToX(int ticks) const
{
    return static_cast<int>(ticks * pixelsPerTick) - viewportX;
}

int ArrangementView::xToTicks(int x) const
{
    return static_cast<int>((x + viewportX) / pixelsPerTick);
}

int ArrangementView::trackIndexToY(int trackIndex) const
{
    return headerHeight + trackIndex * trackHeight;
}

int ArrangementView::yToTrackIndex(int y) const
{
    if (y < headerHeight) return -1;
    return (y - headerHeight) / trackHeight;
}

ArrangementView::ClipRegionHit ArrangementView::findClipRegionAt(int x, int y)
{
    ClipRegionHit hit;
    
    int trackIndex = yToTrackIndex(y);
    if (trackIndex < 0 || trackIndex >= project.getNumTracks())
        return hit;
    
    Track* track = project.getTrack(trackIndex);
    if (!track) return hit;
    
    int ticks = xToTicks(x);
    
    // Find clip region at this time
    juce::ScopedLock lock(track->getLock());
    for (auto& region : track->getClipRegions()) {
        int regionStart = region.startTick;
        int regionEnd = region.startTick + region.lengthTick;
        
        if (ticks >= regionStart && ticks < regionEnd) {
            hit.track = track;
            hit.clipRegion = const_cast<ClipRegion*>(&region);
            hit.trackIndex = trackIndex;
            break;
        }
    }
    
    return hit;
}

void ArrangementView::drawGrid(juce::Graphics& g)
{
    // Draw timeline header background
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRect(0, 0, getWidth(), headerHeight);
    
    // Draw measure lines and numbers
    g.setColour(juce::Colour(0xff404040));
    int ticksPerMeasure = 480 * 4; // 4 quarter notes per measure (4/4 time)
    int ticksPerBeat = 480;
    
    int startTick = xToTicks(0);
    int endTick = xToTicks(getWidth());
    
    // Round to nearest measure
    int startMeasure = startTick / ticksPerMeasure;
    int endMeasure = (endTick / ticksPerMeasure) + 1;
    
    for (int measure = startMeasure; measure <= endMeasure; ++measure) {
        int measureTick = measure * ticksPerMeasure;
        int x = ticksToX(measureTick);
        
        // Draw measure line
        g.setColour(juce::Colour(0xff505050));
        g.drawLine(x, 0, x, getHeight(), 1.0f);
        
        // Draw measure number
        g.setColour(juce::Colour(0xffcccccc));
        g.drawText(juce::String(measure + 1), x + 4, 4, 40, 20, 
                   juce::Justification::centredLeft);
        
        // Draw beat lines
        g.setColour(juce::Colour(0xff303030));
        for (int beat = 1; beat < 4; ++beat) {
            int beatTick = measureTick + beat * ticksPerBeat;
            int beatX = ticksToX(beatTick);
            g.drawLine(beatX, headerHeight, beatX, getHeight(), 1.0f);
        }
    }
}

void ArrangementView::drawTracks(juce::Graphics& g)
{
    int numTracks = project.getNumTracks();
    
    for (int i = 0; i < numTracks; ++i) {
        Track* track = project.getTrack(i);
        if (!track) continue;
        
        int trackY = trackIndexToY(i);
        
        // Draw track background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(0, trackY, getWidth(), trackHeight);
        
        // Draw track separator
        g.setColour(juce::Colour(0xff000000));
        g.drawLine(0, trackY + trackHeight, getWidth(), trackY + trackHeight, 1.0f);
        
        // Draw clip regions
        juce::ScopedLock lock(track->getLock());
        for (auto& region : track->getClipRegions()) {
            drawClipRegion(g, track, const_cast<ClipRegion*>(&region), trackY);
        }
    }
}

void ArrangementView::drawClipRegion(juce::Graphics& g, Track* track, ClipRegion* region, int trackY)
{
    int x1 = ticksToX(region->startTick);
    int x2 = ticksToX(region->startTick + region->lengthTick);
    int width = x2 - x1;
    
    // Clip region background
    juce::Colour clipColour = track->getColour();
    
    // Highlight if selected
    if (region == selectedClipRegion) {
        clipColour = clipColour.brighter(0.3f);
    }
    
    g.setColour(clipColour.withAlpha(0.7f));
    g.fillRoundedRectangle(x1 + 2, trackY + 4, width - 4, trackHeight - 8, 4.0f);
    
    // Clip region border
    g.setColour(clipColour);
    g.drawRoundedRectangle(x1 + 2, trackY + 4, width - 4, trackHeight - 8, 4.0f, 2.0f);
    
    // Clip name
    if (region->clip) {
        g.setColour(juce::Colours::white);
        g.drawText(region->clip->getName(), 
                   x1 + 6, trackY + 8, width - 12, 20,
                   juce::Justification::centredLeft);
    }
    
    // Muted indicator
    if (region->muted) {
        g.setColour(juce::Colours::red.withAlpha(0.5f));
        g.fillRoundedRectangle(x1 + 2, trackY + 4, width - 4, trackHeight - 8, 4.0f);
    }
}

} // namespace pianodaw
