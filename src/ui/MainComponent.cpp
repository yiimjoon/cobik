#include "MainComponent.h"
#include "../core/edit/UndoStack.h"
#include "../core/timeline/PPQ.h"
#include "../core/timeline/Transport.h"
#include "../core/ai/LLMClient.h"
#include "../core/ai/CommandInterpreter.h"

namespace pianodaw {

MainComponent::MainComponent(UndoStack& undoStack_, Transport& transport_, AudioEngine& engine_)
    : undoStack(undoStack_), transport(transport_), audioEngine(engine_)
{
    // Create project
    project = std::make_unique<Project>("New Project");
    
    // Add a default MIDI track with a default clip
    Track* defaultTrack = project->addTrack("Track 1", Track::Type::MIDI);
    defaultTrack->setColour(juce::Colours::blue);
    
    // Create a default clip and add it to the project
    Clip* clipPtr = project->addClip("Clip 1");
    
    // Add clip region to track (4 bars starting at 0)
    ClipRegion region(clipPtr, 0, PPQ::TICKS_PER_QUARTER * 16);
    defaultTrack->addClipRegion(region);
    
    // Create transport bar
    transportBar = std::make_unique<TransportBar>();
    addAndMakeVisible(transportBar.get());
    
    // Create track list panel
    trackListPanel = std::make_unique<TrackListPanel>(*project);
    addAndMakeVisible(trackListPanel.get());
    
    // Create arrangement view
    arrangementView = std::make_unique<ArrangementView>(*project);
    addAndMakeVisible(arrangementView.get());
    
    // Setup callbacks
    trackListPanel->onTrackSelected = [this](int trackIndex) {
        onTrackSelected(trackIndex);
    };
    
    trackListPanel->onTracksChanged = [this]() {
        arrangementView->repaint();
    };
    
    arrangementView->onClipRegionDoubleClick = [this](Track* track, ClipRegion* clipRegion) {
        onClipRegionDoubleClicked(track, clipRegion);
    };
    
    setSize(1280, 720);
    setWantsKeyboardFocus(true);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Transport bar at top (50px)
    transportBar->setBounds(bounds.removeFromTop(50));
    
    if (pianoRollVisible && currentClip) {
        // Split view: Arrangement + Piano Roll
        auto arrangementArea = bounds.removeFromTop(arrangementHeight);
        
        // Arrangement: Track list (150px) + Arrangement view
        trackListPanel->setBounds(arrangementArea.removeFromLeft(150));
        arrangementView->setBounds(arrangementArea);
        
        // Piano roll editor area
        auto editorBounds = bounds;
        
        // Piano roll toolbar
        toolbar->setBounds(editorBounds.removeFromTop(40));
        
        // Info line
        infoLine->setBounds(editorBounds.removeFromTop(30));
        
        // Inspector panel on right (200px)
        inspector->setBounds(editorBounds.removeFromRight(200));
        
        // Main piano roll area
        auto pianoArea = editorBounds;
        
        // Velocity lane at bottom (100px)
        velocityLane->setBounds(pianoArea.removeFromBottom(100));
        
        // Pedal lane at bottom (60px)
        pedalLane->setBounds(pianoArea.removeFromBottom(60));
        
        // Piano roll view takes remaining space
        pianoRollView->setBounds(pianoArea);
        
        // Status line at very bottom
        statusLine->setBounds(editorBounds.removeFromBottom(24));
    }
    else {
        // Full arrangement view
        trackListPanel->setBounds(bounds.removeFromLeft(150));
        arrangementView->setBounds(bounds);
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Close piano roll editor on Escape
    if (key == juce::KeyPress::escapeKey && pianoRollVisible) {
        closeEditor();
        return true;
    }
    
    return false;
}

void MainComponent::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    // Not used in new architecture (scrolling handled by ArrangementView and PianoRollView)
}

void MainComponent::onClipRegionDoubleClicked(Track* track, ClipRegion* clipRegion)
{
    if (!track || !clipRegion || !clipRegion->clip) return;
    
    currentTrack = track;
    currentClipRegion = clipRegion;
    
    // Use the actual clip from the clip region (not a copy)
    currentClip.reset();  // We'll edit the clip directly
    
    // Create piano roll editor components if not exist
    // They all need a Clip reference, so use the clip from the region
    Clip& editClip = *clipRegion->clip;
    
    if (!pianoRollView) {
        pianoRollView = std::make_unique<PianoRollView>(editClip, undoStack, transport, audioEngine);
        addAndMakeVisible(pianoRollView.get());
    }
    
    if (!velocityLane) {
        velocityLane = std::make_unique<VelocityLane>(editClip, undoStack);
        addAndMakeVisible(velocityLane.get());
    }
    
    if (!pedalLane) {
        pedalLane = std::make_unique<PedalLane>(editClip, undoStack);
        addAndMakeVisible(pedalLane.get());
    }
    
    if (!infoLine) {
        infoLine = std::make_unique<InfoLine>(editClip);
        addAndMakeVisible(infoLine.get());
    }
    
    if (!statusLine) {
        statusLine = std::make_unique<StatusLine>();
        addAndMakeVisible(statusLine.get());
    }
    
    if (!toolbar) {
        toolbar = std::make_unique<PianoRollToolbar>();
        addAndMakeVisible(toolbar.get());
    }
    
    if (!inspector) {
        inspector = std::make_unique<InspectorPanel>();
        addAndMakeVisible(inspector.get());
    }
    
    pianoRollVisible = true;
    resized();
}

void MainComponent::onTrackSelected(int trackIndex)
{
    // Update arrangement view selection if needed
}

void MainComponent::closeEditor()
{
    pianoRollVisible = false;
    
    // Hide piano roll components
    if (pianoRollView) pianoRollView->setVisible(false);
    if (velocityLane) velocityLane->setVisible(false);
    if (pedalLane) pedalLane->setVisible(false);
    if (infoLine) infoLine->setVisible(false);
    if (statusLine) statusLine->setVisible(false);
    if (toolbar) toolbar->setVisible(false);
    if (inspector) inspector->setVisible(false);
    
    currentClip.reset();
    currentTrack = nullptr;
    currentClipRegion = nullptr;
    
    resized();
}

} // namespace pianodaw
