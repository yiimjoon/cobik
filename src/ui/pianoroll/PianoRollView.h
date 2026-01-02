#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/model/Clip.h"
#include "../../core/timeline/PPQ.h"
#include "../../core/ai/MusicTheory.h"
#include "../editor/PianoRollToolbar.h" // For EditorTool enum

namespace pianodaw {

class UndoStack;
class Transport;
class AudioEngine;

/**
 * Piano Roll View - Main editor component
 */
class PianoRollView : public juce::Component,
                      public juce::ScrollBar::Listener
{
public:
    PianoRollView(Clip& clip, UndoStack& undoStack, Transport& transport, AudioEngine& audioEngine);
    ~PianoRollView() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Mouse/Keyboard
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    // ScrollBar::Listener
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    
    // View state callbacks
    std::function<void(int64_t, double)> onViewChanged;
    std::function<void(const std::vector<int>&)> onSelectionChanged;
    std::function<void()> onCloseButtonClicked;
    std::function<void(int64_t, int)> onMousePositionChanged; // tick, pitch
    
    int64_t getViewStartTick() const { return viewStartTick; }
    double getPixelsPerTick() const { return pixelsPerTick; }
    
    // Settings
    void setGridSize(GridSize size);
    GridSize getGridSize() const { return currentGridSize; }
    
    void setScale(int root, MusicTheory::ScaleType type);
    void setChordMode(MusicTheory::ChordType type);
    
    // Phase 2: Tool System
    void setCurrentTool(EditorTool tool);
    EditorTool getCurrentTool() const { return currentTool; }
    
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    bool isSnapEnabled() const { return snapEnabled; }
    
    // Phase 3: Selection
    const std::vector<int>& getSelectedNoteIds() const { return selectedNoteIds; }
    
    // Phase 6: Advanced Selection
    void selectByPitchRange(int minPitch, int maxPitch, bool additive = false);
    void selectByVelocityRange(int minVel, int maxVel, bool additive = false);
    void selectByDuration(int64_t minTicks, int64_t maxTicks, bool additive = false);
    void selectEveryNth(int n, int offset = 0);
    void invertSelection();

    struct ScaleInfo {
        int rootNote = 0; // 0 = C
        MusicTheory::ScaleType type = MusicTheory::ScaleType::Chromatic;
    };

    struct ChordMode {
        MusicTheory::ChordType type = MusicTheory::ChordType::None;
    };

    ScaleInfo getScaleInfo() const { return scaleInfo; }
    
    // Zoom/scroll
    void setPixelsPerTick(double ppt) { pixelsPerTick = ppt; repaint(); }
    void setPixelsPerKey(double ppk) { pixelsPerKey = ppk; repaint(); }
    void setViewStartTick(int64_t tick) { viewStartTick = tick; repaint(); }
    void setViewStartPitch(int pitch) { viewStartPitch = pitch; repaint(); }
    
private:
    Clip& clip;
    UndoStack& undoStack;
    Transport& transport;
    AudioEngine& audioEngine;
    
    // View settings
    GridSize currentGridSize;
    ScaleInfo scaleInfo;
    ChordMode chordMode;
    
    double pixelsPerTick;      
    double pixelsPerKey;        
    
    // Phase 2: Tool System
    EditorTool currentTool = EditorTool::Select;
    bool snapEnabled = true;
    
    // Keyboard state for chord detection
    std::set<int> currentlyPressedKeys;
    juce::String detectedChordName;
    
    // Selection state
    std::vector<int> selectedNoteIds;
    
    // Clipboard for copy/paste
    std::vector<Note> clipboard;
    
    // Mouse interaction state
    enum class MouseMode { None, Select, Move, ResizeEnd, ResizeStart, Lasso, Pan, DrawPending, DraggingPlayhead };
    MouseMode mouseMode = MouseMode::None;
    
    juce::Point<int> mouseStartPosition; // Restored
    int64_t mouseStartTick = 0; // Restored
    int mouseStartPitch = 0; // Restored
    int64_t panStartTick = 0; // Restored
    int panStartPitch = 0; // Restored
    juce::Point<int> lastMousePos;
    int64_t dragStartTick = 0;
    int dragStartPitch = 0;
    juce::Rectangle<int> lassoRect;
    int activeNoteId = -1;
    
    // Phase 4.5: Resize state tracking
    int64_t resizeOriginalStartTick = 0;
    int64_t resizeOriginalEndTick = 0;
    
    // Helper methods
    int findNoteAt(juce::Point<int> p) const;
    void clearSelection();
    void selectNote(int id, bool additive);
    void deleteSelectedNotes();
    Note* getNoteAt(juce::Point<int> pos);
    
    // Phase 2: Tool-specific mouse handlers
    void handleSelectToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e);
    void handleDrawToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e);
    void handleEraseToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e);
    void handleSplitToolMouseDown(juce::Point<int> p, const juce::MouseEvent& e);
    
    // Constants
    static constexpr int KEYBOARD_WIDTH = 80;
    static constexpr int LOWEST_KEY = 21;   
    static constexpr int HIGHEST_KEY = 108; 
    static constexpr int NUM_KEYS = HIGHEST_KEY - LOWEST_KEY + 1;
    
    int64_t viewStartTick;
    int64_t viewEndTick;
    int viewStartPitch;
    
    void updateViewEndTick();
    
    // Rendering
    void paintTimeline(juce::Graphics& g, juce::Rectangle<int> area);
    void paintPianoKeyboard(juce::Graphics& g, juce::Rectangle<int> area);
    void paintNoteGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void paintNotes(juce::Graphics& g, juce::Rectangle<int> area);
    
    // Coordinate conversion
    int tickToX(int64_t tick) const;
    int64_t xToTick(int x) const;
    int keyToY(int midiKey) const;
    int yToKey(int y) const;
    juce::Rectangle<int> noteToRect(const Note& note, juce::Rectangle<int> gridArea) const;
    
    bool isBlackKey(int midiKey) const;
    juce::String getKeyName(int midiKey) const;
    void updateChordDetection();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollView)
};

} // namespace pianodaw
