#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/model/Clip.h"
#include "../../core/timeline/PPQ.h"

namespace pianodaw {

class UndoStack;

/**
 * PedalLane - Professional CC64 (Sustain Pedal) editor
 * Phase 5: Bar display, snap support, undo integration, drag editing
 */
class PedalLane : public juce::Component
{
public:
    PedalLane(Clip& clip, UndoStack& undoStack);
    ~PedalLane() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    
    void setViewRange(int64_t startTick, double pixelsPerTick);
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    void setGridSize(GridSize size) { currentGridSize = size; }
    
private:
    Clip& clip;
    UndoStack& undoStack;
    
    int64_t viewStartTick = 0;
    double pixelsPerTick = 0.1;
    static constexpr int KEYBOARD_WIDTH_OFFSET = 80;
    
    // Phase 5: Snap support
    bool snapEnabled = true;
    GridSize currentGridSize = GridSize::Sixteenth;
    
    // Mouse interaction state
    enum class EditMode { Add, Remove };
    EditMode currentEditMode = EditMode::Add;
    int64_t dragStartTick = 0;
    int64_t dragEndTick = 0;
    bool isDragging = false;
    
    int tickToX(int64_t tick) const;
    int64_t xToTick(int x) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PedalLane)
};

} // namespace pianodaw
