#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/model/Clip.h"
#include "../core/timeline/Transport.h"
#include "../core/edit/UndoStack.h"
#include "pianoroll/PianoRollView.h"
#include "pianoroll/VelocityLane.h"
#include "pianoroll/PedalLane.h"
#include "panels/VstBrowserPanel.h"
#include "panels/QuantizePanel.h"
#include "panels/PromptBar.h"
#include "panels/TheoryToolbar.h"
#include "transport/TransportBar.h"
#include "editor/InfoLine.h"
#include "editor/StatusLine.h"
#include "editor/PianoRollToolbar.h"
#include "editor/InspectorPanel.h"

namespace pianodaw {

class QuantizePanel;
class LLMClient;
class CommandInterpreter;

/**
 * Main content component that holds everything
 */
class MainComponent : public juce::Component,
                      public juce::ScrollBar::Listener
{
public:
    MainComponent(Clip& clip, UndoStack& undoStack, Transport& transport, AudioEngine& engine);
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    // ScrollBar::Listener
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

private:
    Clip& clip;
    UndoStack& undoStack;
    Transport& transport;
    AudioEngine& audioEngine;
    
    // Phase 1: New UI Components
    std::unique_ptr<InfoLine> infoLine;
    std::unique_ptr<StatusLine> statusLine;
    std::unique_ptr<PianoRollToolbar> toolbar;
    std::unique_ptr<InspectorPanel> inspector;
    
    // Existing components
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<PianoRollView> pianoRollView;
    std::unique_ptr<VelocityLane> velocityLane;
    std::unique_ptr<PedalLane> pedalLane;
    std::unique_ptr<VstBrowserPanel> vstBrowser;
    std::unique_ptr<QuantizePanel> quantizePanel;
    std::unique_ptr<PromptBar> promptBar;
    std::unique_ptr<TheoryToolbar> theoryToolbar;
    
    std::unique_ptr<LLMClient> llmClient;
    std::unique_ptr<CommandInterpreter> interpreter;
    
    std::unique_ptr<juce::ScrollBar> horizontalScrollBar;
    std::unique_ptr<juce::ScrollBar> verticalScrollBar;
    
    juce::StretchableLayoutManager layoutManager;
    juce::StretchableLayoutResizerBar resizerBar1;
    juce::StretchableLayoutResizerBar resizerBar2;
    juce::StretchableLayoutResizerBar resizerBar3;
    juce::StretchableLayoutResizerBar resizerBar4;
    juce::StretchableLayoutResizerBar inspectorResizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace pianodaw
