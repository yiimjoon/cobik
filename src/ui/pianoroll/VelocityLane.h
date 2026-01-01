#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/model/Clip.h"
#include "../../core/timeline/PPQ.h"

namespace pianodaw {

class UndoStack;

/**
 * VelocityLane - Professional velocity editor (Cubase-style bar display)
 * Phase 5: Bar display, selection sync, undo support
 */
class VelocityLane : public juce::Component
{
public:
    VelocityLane(Clip& clip, UndoStack& undoStack);
    ~VelocityLane() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    
    // Sync with PianoRollView
    void setViewRange(int64_t startTick, double pixelsPerTick);
    void setSelectedNotes(const std::vector<int>& noteIds);
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    void setGridSize(GridSize size) { currentGridSize = size; }
    
    // Phase 5: Editing modes
    enum class EditMode { Draw, Line, Ramp, Scale };
    void setEditMode(EditMode mode) { editMode = mode; }
    
private:
    Clip& clip;
    UndoStack& undoStack;
    
    int64_t viewStartTick = 0;
    double pixelsPerTick = 0.1;
    static constexpr int KEYBOARD_WIDTH_OFFSET = 80;
    
    // Phase 5: Selection & editing state
    std::vector<int> selectedNoteIds;
    bool snapEnabled = true;
    GridSize currentGridSize = GridSize::Sixteenth;
    EditMode editMode = EditMode::Draw;
    
    // Mouse interaction
    int activeNoteId = -1;
    int originalVelocity = 0;
    std::vector<std::pair<int, int>> batchEditNotes; // (noteId, originalVelocity)
    
    int tickToX(int64_t tick) const;
    int64_t xToTick(int x) const;
    int velocityToY(int velocity) const;
    int yToVelocity(int y) const;
    
    // Phase 5: Helper methods
    int findNoteAtPosition(juce::Point<int> p);
    juce::Rectangle<int> getNoteBarRect(const Note& note) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityLane)
};

} // namespace pianodaw
