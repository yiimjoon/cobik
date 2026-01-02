#include "ArrangementView.h"
#include "../../core/timeline/Transport.h"
#include "../panels/DebugLogWindow.h"

namespace pianodaw {

ArrangementView::ArrangementView(Project& proj, Transport* trans)
    : project(proj), transport(trans)
{
    setOpaque(false);  // 반투명하게 - 그리드가 보이도록
    setWantsKeyboardFocus(true);  // Enable keyboard input
    
    // 30fps로 재생바 업데이트
    startTimer(33);
}

void ArrangementView::paint(juce::Graphics& g)
{
    // Background - 약간 투명
    g.fillAll(juce::Colour(0xff2b2b2b));

    // Draw grid FIRST (배경)
    drawGrid(g);

    // Draw tracks and clips (반투명)
    drawTracks(g);
    
    // Draw playhead LAST (맨 위)
    drawPlayhead(g);
    
    // Draw loop markers (L/R)
    drawLoopMarkers(g);
}

void ArrangementView::resized()
{
    // Viewport updates handled in paint
}

void ArrangementView::mouseDown(const juce::MouseEvent& event)
{
    // 헤더 영역 클릭 시 재생바 이동
    if (event.y < headerHeight && transport) {
        int64_t clickTick = xToTicks(event.x);
        transport->setPosition(clickTick);
        isDraggingPlayhead = true;
        repaint();
        return;
    }
    
    auto hit = findClipRegionAt(event.x, event.y);
    
    selectedTrack = hit.track;
    selectedClipRegion = hit.clipRegion;
    
    if (selectedClipRegion && selectedTrack) {
        int clipStartX = ticksToX(selectedClipRegion->startTick);
        int clipEndX = ticksToX(selectedClipRegion->getEndTick());
        
        const int resizeEdgeThreshold = 8;
        
        // Check if clicking near edges for resize
        if (std::abs(event.x - clipStartX) < resizeEdgeThreshold)
        {
            isResizingClipStart = true;
            dragStartTick = selectedClipRegion->startTick;
            originalClipLength = selectedClipRegion->lengthTick;
        }
        else if (std::abs(event.x - clipEndX) < resizeEdgeThreshold)
        {
            isResizingClipEnd = true;
            dragStartTick = selectedClipRegion->startTick;
            originalClipLength = selectedClipRegion->lengthTick;
        }
        else
        {
            // Regular clip drag
            isDraggingClip = true;
            dragStartX = event.x;
            dragStartTick = selectedClipRegion->startTick;
        }
    }
    
    repaint();
}

void ArrangementView::mouseDoubleClick(const juce::MouseEvent& event)
{
    DebugLogWindow::addLog("ArrangementView: mouseDoubleClick at x=" + juce::String(event.x) + " y=" + juce::String(event.y));
    
    auto hit = findClipRegionAt(event.x, event.y);
    
    if (hit.clipRegion) {
        DebugLogWindow::addLog("ArrangementView: Clip found: " + (hit.clipRegion->clip ? hit.clipRegion->clip->getName() : "NULL CLIP"));
        
        if (onClipRegionDoubleClick) {
            DebugLogWindow::addLog("ArrangementView: Calling callback...");
            onClipRegionDoubleClick(hit.track, hit.clipRegion);
            DebugLogWindow::addLog("ArrangementView: Callback returned");
        } else {
            DebugLogWindow::addLog("ArrangementView: ERROR - Callback is NULL!");
        }
    } else {
        DebugLogWindow::addLog("ArrangementView: No clip found at position");
    }
}

void ArrangementView::mouseDrag(const juce::MouseEvent& event)
{
    // Resize clip start
    if (isResizingClipStart && selectedClipRegion)
    {
        int64_t newStartTick = xToTicks(event.x);
        int64_t clipEnd = dragStartTick + originalClipLength;
        
        // Ensure clip doesn't become negative length
        if (newStartTick >= 0 && newStartTick < clipEnd - 480)
        {
            selectedClipRegion->startTick = newStartTick;
            selectedClipRegion->lengthTick = clipEnd - newStartTick;
            repaint();
        }
        return;
    }
    
    // Resize clip end
    if (isResizingClipEnd && selectedClipRegion)
    {
        int64_t newEndTick = xToTicks(event.x);
        
        // Ensure clip has minimum length
        if (newEndTick > selectedClipRegion->startTick + 480)
        {
            selectedClipRegion->lengthTick = newEndTick - selectedClipRegion->startTick;
            repaint();
        }
        return;
    }
    
    // Playhead dragging (헤더 영역에서 드래그)
    if (isDraggingPlayhead && transport) {
        int64_t dragTick = xToTicks(event.x);
        if (dragTick >= 0) {
            transport->setPosition(dragTick);
            repaint();
        }
        return;
    }
    
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

void ArrangementView::mouseUp(const juce::MouseEvent& event)
{
    isDraggingClip = false;
    isDraggingPlayhead = false;
    isResizingClipStart = false;
    isResizingClipEnd = false;
}

void ArrangementView::mouseMove(const juce::MouseEvent& event)
{
    // Check if mouse is near clip edge for resize cursor
    auto hit = findClipRegionAt(event.x, event.y);
    
    if (hit.clipRegion && hit.track)
    {
        int clipStartX = ticksToX(hit.clipRegion->startTick);
        int clipEndX = ticksToX(hit.clipRegion->getEndTick());
        
        const int resizeEdgeThreshold = 8; // pixels from edge
        
        if (std::abs(event.x - clipStartX) < resizeEdgeThreshold)
        {
            // Near start edge
            setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
        }
        else if (std::abs(event.x - clipEndX) < resizeEdgeThreshold)
        {
            // Near end edge
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
        }
        else
        {
            // Inside clip but not near edge
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void ArrangementView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isCtrlDown()) {
        // Ctrl+Wheel = Horizontal zoom
        double zoomFactor = 1.0 + wheel.deltaY * 0.5;
        setPixelsPerTick(pixelsPerTick * zoomFactor);
    }
    else {
        // Wheel = Horizontal scroll (default behavior for timeline)
        int scrollDelta = static_cast<int>(wheel.deltaY * 200);  // Increased sensitivity
        setViewportX(viewportX - scrollDelta);
        DebugLogWindow::addLog("ArrangementView: Scroll viewportX=" + juce::String(viewportX));
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

void ArrangementView::drawPlayhead(juce::Graphics& g)
{
    if (!transport)
        return;
    
    int64_t currentTick = transport->getPosition();
    int x = ticksToX(currentTick);
    
    // Playhead line (빨간색 재생바)
    g.setColour(juce::Colours::red);
    g.drawLine(x, 0, x, getHeight(), 2.0f);
    
    // Playhead triangle (위쪽 삼각형)
    juce::Path triangle;
    triangle.addTriangle(x - 6, 0, x + 6, 0, x, 12);
    g.fillPath(triangle);
}

void ArrangementView::drawLoopMarkers(juce::Graphics& g)
{
    if (!transport)
        return;
    
    int64_t loopStart = project.getLoopStart();
    int64_t loopEnd = project.getLoopEnd();
    
    // Don't draw if loop is disabled
    if (loopEnd <= 0)
        return;
    
    // Draw loop region highlight (light blue background)
    int xStart = ticksToX(loopStart);
    int xEnd = ticksToX(loopEnd);
    
    if (xEnd > xStart)
    {
        int y = headerHeight;  // Below timeline header
        int height = getHeight() - headerHeight;
        
        // Highlight loop region
        g.setColour(juce::Colour::fromRGBA(51, 102, 153, 21));  // Light blue with alpha
        g.fillRect(xStart, y, xEnd - xStart, height);
        
        // Draw loop borders
        g.setColour(juce::Colours::lightblue);
        g.drawRect((float)xStart, (float)y, (float)(xEnd - xStart), (float)height, 2.0f);
    }
    
    // Draw L/R markers (green triangles)
    g.setColour(juce::Colours::green);
    g.setFont(juce::Font(10.0f));
    
    // L marker
    juce::Path lMarker;
    lMarker.addTriangle(xStart - 4, headerHeight, xStart + 4, headerHeight, xStart, headerHeight + 10);
    g.fillPath(lMarker);
    g.drawText("L", xStart - 6, headerHeight - 15, 12, 12, juce::Justification::centred);
    
    // R marker
    juce::Path rMarker;
    rMarker.addTriangle(xEnd - 4, headerHeight, xEnd + 4, headerHeight, xEnd, headerHeight + 10);
    g.fillPath(rMarker);
    g.drawText("R", xEnd - 6, headerHeight - 15, 12, 12, juce::Justification::centred);
}

void ArrangementView::timerCallback()
{
    if (!transport)
        return;
    
    // Check if position changed (for stop double-click, timeline click, etc.)
    static int64_t lastTick = -1;
    int64_t currentTick = transport->getPosition();
    
    // Repaint if playing OR if position changed
    if (transport->isPlaying() || currentTick != lastTick)
    {
        repaint();
        lastTick = currentTick;
    }
}

bool ArrangementView::keyPressed(const juce::KeyPress& key)
{
    // Shift+A: Auto-loop from selection (Cubase style)
    if (key.getModifiers().isShiftDown() && (key.getTextCharacter() == 'a' || key.getTextCharacter() == 'A'))
    {
        if (selectedClipRegion && selectedTrack)
        {
            // Set loop to match clip region bounds
            int64_t loopStart = selectedClipRegion->startTick;
            int64_t loopEnd = selectedClipRegion->getEndTick();
            
            project.setLoopStart(loopStart);
            project.setLoopEnd(loopEnd);
            
            // Also update Transport if available
            if (transport)
            {
                transport->setLoopRange(loopStart, loopEnd);
                transport->setLooping(true);  // Enable looping
            }
            
            repaint();
            DebugLogWindow::addLog("ArrangementView: Auto-loop from clip (" + 
                                 juce::String(loopStart) + " - " + juce::String(loopEnd) + ")");
            return true;
        }
        else
        {
            // No clip selected - clear loop region
            project.setLoopStart(0);
            project.setLoopEnd(0);
            
            if (transport)
            {
                transport->setLoopRange(0, 0);
                transport->setLooping(false);  // Disable looping
            }
            
            repaint();
            DebugLogWindow::addLog("ArrangementView: Loop region cleared (Shift+A with no selection)");
            return true;
        }
    }
    
    // H: Zoom in (Cubase style)
    if (key.getTextCharacter() == 'h' || key.getTextCharacter() == 'H')
    {
        setPixelsPerTick(pixelsPerTick * 1.2);  // Zoom in 20%
        repaint();
        DebugLogWindow::addLog("ArrangementView: Zoom in (H)");
        return true;
    }
    
    // G: Zoom out (Cubase style)
    if (key.getTextCharacter() == 'g' || key.getTextCharacter() == 'G')
    {
        setPixelsPerTick(pixelsPerTick / 1.2);  // Zoom out 20%
        repaint();
        DebugLogWindow::addLog("ArrangementView: Zoom out (G)");
        return true;
    }
    
    // Shift+F: Zoom to fit (show all content)
    if (key.getModifiers().isShiftDown() && (key.getTextCharacter() == 'f' || key.getTextCharacter() == 'F'))
    {
        // Find the rightmost clip end position
        int64_t maxTick = 0;
        for (int i = 0; i < project.getNumTracks(); ++i)
        {
            Track* track = project.getTrack(i);
            if (track)
            {
                for (const auto& region : track->getClipRegions())
                {
                    int64_t endTick = region.getEndTick();
                    if (endTick > maxTick)
                        maxTick = endTick;
                }
            }
        }
        
        // Calculate zoom to fit all content in viewport
        if (maxTick > 0)
        {
            int viewportWidth = getWidth();
            double newPixelsPerTick = (viewportWidth * 0.9) / maxTick;  // 90% of viewport width
            setPixelsPerTick(newPixelsPerTick);
            setViewportX(0);  // Reset scroll to start
            repaint();
            DebugLogWindow::addLog("ArrangementView: Zoom to fit (Shift+F)");
        }
        return true;
    }
    
    // Delete/Backspace: Delete selected clip
    if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        if (selectedClipRegion && selectedTrack)
        {
            int regionId = selectedClipRegion->id;
            juce::String clipName = selectedClipRegion->clip ? selectedClipRegion->clip->getName() : "Unknown";
            
            // Remove the clip region from the track
            selectedTrack->removeClipRegion(regionId);
            
            // Clear selection
            selectedClipRegion = nullptr;
            
            // Repaint to show changes
            repaint();
            
            DebugLogWindow::addLog("ArrangementView: Deleted clip region '" + clipName + "' (ID: " + juce::String(regionId) + ")");
            return true;
        }
    }
    
    return false;
}

} // namespace pianodaw
