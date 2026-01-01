#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/model/Project.h"
#include "../../core/model/Track.h"

namespace pianodaw {

class Transport;

/**
 * ArrangementView - Timeline view showing tracks and clip regions (Cubase-style)
 * 
 * Features:
 * - Horizontal timeline with measures/beats grid
 * - Displays ClipRegion blocks on each track
 * - Click to select clip region
 * - Double-click to open in PianoRollEditor
 * - Horizontal/vertical scrolling and zooming
 * - Playhead visualization
 */
class ArrangementView : public juce::Component,
                        private juce::Timer
{
public:
    ArrangementView(Project& project, Transport* transport = nullptr);
    ~ArrangementView() override = default;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // Zoom and scroll
    void setPixelsPerTick(double newPixelsPerTick);
    double getPixelsPerTick() const { return pixelsPerTick; }
    
    // Drawing helpers
    void drawLoopMarkers(juce::Graphics& g);
    void setViewportX(int newX);
    int getViewportX() const { return viewportX; }

    // Selection
    ClipRegion* getSelectedClipRegion() { return selectedClipRegion; }
    Track* getSelectedTrack() { return selectedTrack; }
    
    // Callback for double-click on clip region
    std::function<void(Track*, ClipRegion*)> onClipRegionDoubleClick;

private:
    // Convert between time (ticks) and screen coordinates
    int ticksToX(int ticks) const;
    int xToTicks(int x) const;
    int trackIndexToY(int trackIndex) const;
    int yToTrackIndex(int y) const;

    // Find clip region at position
    struct ClipRegionHit {
        Track* track = nullptr;
        ClipRegion* clipRegion = nullptr;
        int trackIndex = -1;
    };
    ClipRegionHit findClipRegionAt(int x, int y);

    // Draw grid and clips
    void drawGrid(juce::Graphics& g);
    void drawTracks(juce::Graphics& g);
    void drawClipRegion(juce::Graphics& g, Track* track, ClipRegion* region, int trackY);
    void drawPlayhead(juce::Graphics& g);
    
    // Timer callback
    void timerCallback() override;

    // Reference to project
    Project& project;
    Transport* transport = nullptr;  // For playhead position

    // View state
    double pixelsPerTick = 0.05;  // Zoom level (pixels per tick)
    int viewportX = 0;             // Horizontal scroll offset (pixels)
    int trackHeight = 80;          // Height of each track lane
    int headerHeight = 30;         // Height of timeline ruler

    // Selection
    Track* selectedTrack = nullptr;
    ClipRegion* selectedClipRegion = nullptr;

    // Drag state
    bool isDraggingClip = false;
    bool isDraggingPlayhead = false;
    int dragStartX = 0;
    int dragStartTick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementView)
};

} // namespace pianodaw
