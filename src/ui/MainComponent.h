#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/model/Clip.h"
#include "../core/model/Project.h"
#include "../core/timeline/Transport.h"
#include "../core/edit/UndoStack.h"
#include "arrangement/ArrangementView.h"
#include "arrangement/TrackListPanel.h"
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
 * Main content component - Cubase-style multi-track DAW
 * 
 * Layout:
 * - Top: Transport bar
 * - Middle: Arrangement view (TrackListPanel + ArrangementView)
 * - Bottom: Piano roll editor (when clip is opened)
 */
class MainComponent : public juce::Component,
                      public juce::ScrollBar::Listener
{
public:
    MainComponent(UndoStack& undoStack, Transport& transport, AudioEngine& engine);
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    // ScrollBar::Listener
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

private:
    void onClipRegionDoubleClicked(Track* track, ClipRegion* clipRegion);
    void onTrackSelected(int trackIndex);
    void closeEditor();

    UndoStack& undoStack;
    Transport& transport;
    AudioEngine& audioEngine;
    
    // Project (owns all tracks and clips)
    std::unique_ptr<Project> project;
    
    // Current editing state
    std::unique_ptr<Clip> currentClip;  // The clip being edited in piano roll
    Track* currentTrack = nullptr;
    ClipRegion* currentClipRegion = nullptr;
    
    // Arrangement view components
    std::unique_ptr<TrackListPanel> trackListPanel;
    std::unique_ptr<ArrangementView> arrangementView;
    
    // Piano roll editor components (shown when editing a clip)
    std::unique_ptr<PianoRollView> pianoRollView;
    std::unique_ptr<VelocityLane> velocityLane;
    std::unique_ptr<PedalLane> pedalLane;
    std::unique_ptr<InfoLine> infoLine;
    std::unique_ptr<StatusLine> statusLine;
    std::unique_ptr<PianoRollToolbar> toolbar;
    std::unique_ptr<InspectorPanel> inspector;
    
    // Other panels
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<VstBrowserPanel> vstBrowser;
    std::unique_ptr<QuantizePanel> quantizePanel;
    std::unique_ptr<PromptBar> promptBar;
    std::unique_ptr<TheoryToolbar> theoryToolbar;
    
    std::unique_ptr<LLMClient> llmClient;
    std::unique_ptr<CommandInterpreter> interpreter;
    
    // Layout
    bool pianoRollVisible = false;
    int arrangementHeight = 300;  // Height of arrangement view
    int pianoRollHeight = 400;     // Height of piano roll when visible

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace pianodaw
