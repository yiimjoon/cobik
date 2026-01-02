#include "MainComponent.h"
#include "../core/edit/UndoStack.h"
#include "../core/timeline/PPQ.h"
#include "../core/timeline/Transport.h"
#include "../core/audio/MidiRecorder.h"
#include "../core/ai/LLMClient.h"
#include "../core/ai/CommandInterpreter.h"
#include "panels/DebugLogWindow.h"

namespace pianodaw {

MainComponent::MainComponent(Project& project_, UndoStack& undoStack_, Transport& transport_, AudioEngine& engine_)
    : project(project_), undoStack(undoStack_), transport(transport_), audioEngine(engine_)
{
    // Create transport bar
    transportBar = std::make_unique<TransportBar>();
    addAndMakeVisible(transportBar.get());
    
    // Create track list panel
    trackListPanel = std::make_unique<TrackListPanel>(project);
    addAndMakeVisible(trackListPanel.get());
    
    // Create arrangement view (with Transport for playhead)
    arrangementView = std::make_unique<ArrangementView>(project, &transport);
    addAndMakeVisible(arrangementView.get());
    
    // Setup transport bar callbacks
    transportBar->onPlay = [this]() {
        transport.start();
    };
    
    transportBar->onPlayDoubleClick = [this]() {
        transport.togglePlay();
    };
    
    transportBar->onStop = [this]() {
        transport.stop();
        stopRecording(); // Stop recording if active
    };
    
    transportBar->onStopDoubleClick = [this]() {
        DebugLogWindow::addLog("MainComponent: onStopDoubleClick callback called!");
        DebugLogWindow::addLog("MainComponent: Current position = " + juce::String(transport.getPosition()));
        transport.stop();
        transport.setPosition(0);  // 원위치
        DebugLogWindow::addLog("MainComponent: Position set to " + juce::String(transport.getPosition()));
        arrangementView->repaint();  // 화면 업데이트
        if (pianoRollView) pianoRollView->repaint();  // 피아노롤도 업데이트
        DebugLogWindow::addLog("MainComponent: Repaint called");
        stopRecording();
    };
    
    transportBar->onLoopToggle = [this](bool looping) {
        transport.setLooping(looping);
    };
    
    transportBar->onRecordToggle = [this](bool recording) {
        if (recording) {
            startRecording();  // 즉시 녹음 시작 (자동 재생 포함)
        } else {
            stopRecording();
        }
    };
    
    // Setup track list panel callbacks
    trackListPanel->onTrackSelected = [this](int trackIndex) {
        onTrackSelected(trackIndex);
    };
    
    trackListPanel->onTracksChanged = [this]() {
        arrangementView->repaint();
    };
    
    trackListPanel->onRecordArmChanged = [this](int trackIndex, bool armed) {
        recordArmedTrackIndex = armed ? trackIndex : -1;
        audioEngine.setRecordArmedTrack(recordArmedTrackIndex);
    };
    
    // Setup arrangement view callbacks
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
    
    if (pianoRollVisible && currentClipRegion) {
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
    // Ctrl+Z: Undo
    if (key.getModifiers().isCommandDown() && !key.getModifiers().isShiftDown() && key.getTextCharacter() == 'z') {
        if (undoStack.canUndo()) {
            undoStack.undo();
            DebugLogWindow::addLog("Undo performed");
            // Repaint all views to reflect undo changes
            arrangementView->repaint();
            if (pianoRollView) pianoRollView->repaint();
            if (velocityLane) velocityLane->repaint();
            if (pedalLane) pedalLane->repaint();
        }
        return true;
    }
    
    // Ctrl+Shift+Z: Redo
    if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.getTextCharacter() == 'z') {
        if (undoStack.canRedo()) {
            undoStack.redo();
            DebugLogWindow::addLog("Redo performed");
            // Repaint all views to reflect redo changes
            arrangementView->repaint();
            if (pianoRollView) pianoRollView->repaint();
            if (velocityLane) velocityLane->repaint();
            if (pedalLane) pedalLane->repaint();
        }
        return true;
    }
    
    // Spacebar: Play/Stop toggle (Cubase style)
    if (key == juce::KeyPress::spaceKey) {
        if (transport.isPlaying()) {
            DebugLogWindow::addLog("Spacebar: Stopping playback");
            transport.stop();
            stopRecording();
        } else {
            DebugLogWindow::addLog("Spacebar: Starting playback");
            transport.start();
        }
        return true;
    }
    
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
    DebugLogWindow::addLog("MainComponent: onClipRegionDoubleClicked called");
    
    if (!track) {
        DebugLogWindow::addLog("MainComponent: ERROR - track is NULL");
        return;
    }
    if (!clipRegion) {
        DebugLogWindow::addLog("MainComponent: ERROR - clipRegion is NULL");
        return;
    }
    if (!clipRegion->clip) {
        DebugLogWindow::addLog("MainComponent: ERROR - clipRegion->clip is NULL");
        return;
    }
    
    DebugLogWindow::addLog("MainComponent: Opening piano roll for: " + clipRegion->clip->getName());
    
    currentTrack = track;
    currentClipRegion = clipRegion;
    
    // Use the actual clip from the clip region (not a copy)
    currentClip.reset();  // We'll edit the clip directly
    
    // ALWAYS recreate piano roll components with new clip reference
    // (Otherwise it keeps editing the old clip)
    Clip& editClip = *clipRegion->clip;
    
    DebugLogWindow::addLog("MainComponent: Creating piano roll view...");
    
    // Recreate piano roll view
    pianoRollView.reset();
    DebugLogWindow::addLog("MainComponent: Creating PianoRollView...");
    pianoRollView = std::make_unique<PianoRollView>(editClip, undoStack, transport, audioEngine);
    
    // Set close button callback
    pianoRollView->onCloseButtonClicked = [this]() {
        closeEditor();
    };
    
    addAndMakeVisible(pianoRollView.get());
    DebugLogWindow::addLog("MainComponent: PianoRollView created");
    
    // Recreate velocity lane
    velocityLane.reset();
    velocityLane = std::make_unique<VelocityLane>(editClip, undoStack);
    addAndMakeVisible(velocityLane.get());
    DebugLogWindow::addLog("MainComponent: VelocityLane created");
    
    // Recreate pedal lane
    pedalLane.reset();
    pedalLane = std::make_unique<PedalLane>(editClip, undoStack);
    addAndMakeVisible(pedalLane.get());
    DebugLogWindow::addLog("MainComponent: PedalLane created");
    
    // Recreate info line
    infoLine.reset();
    infoLine = std::make_unique<InfoLine>(editClip);
    addAndMakeVisible(infoLine.get());
    DebugLogWindow::addLog("MainComponent: InfoLine created");
    
    // Status line, toolbar, inspector don't depend on clip - create once
    if (!statusLine) {
        statusLine = std::make_unique<StatusLine>();
        addAndMakeVisible(statusLine.get());
    } else {
        statusLine->setVisible(true);
    }
    
    if (!toolbar) {
        toolbar = std::make_unique<PianoRollToolbar>();
        addAndMakeVisible(toolbar.get());
    } else {
        toolbar->setVisible(true);
    }
    
    if (!inspector) {
        inspector = std::make_unique<InspectorPanel>();
        addAndMakeVisible(inspector.get());
    } else {
        inspector->setVisible(true);
    }
    
    pianoRollVisible = true;
    DebugLogWindow::addLog("MainComponent: Calling resized()...");
    resized();
    DebugLogWindow::addLog("MainComponent: Piano roll opened successfully!");
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

void MainComponent::startRecording()
{
    // Check if a track is armed
    if (recordArmedTrackIndex < 0 || recordArmedTrackIndex >= project.getNumTracks())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Cannot Record",
            "Please arm a track first by clicking the 'R' button."
        );
        transportBar->setRecording(false);
        return;
    }
    
    // Get the armed track
    Track* track = project.getTrack(recordArmedTrackIndex);
    if (!track)
    {
        transportBar->setRecording(false);
        return;
    }
    
    // Create a new clip for recording or use existing one
    Clip* recordClip = nullptr;
    ClipRegion* recordRegion = nullptr;
    
    // Check if there's already a clip at the current position
    int64_t currentTick = transport.getPosition();
    for (auto& region : track->getClipRegions())
    {
        if (currentTick >= region.startTick && currentTick < region.getEndTick())
        {
            recordClip = region.clip;
            recordRegion = const_cast<ClipRegion*>(&region);
            break;
        }
    }
    
    // If no existing clip, create a new one
    if (!recordClip)
    {
        recordClip = project.addClip("Recording");
        
        // Create a clip region (4 bars long by default)
        int64_t clipLength = PPQ::TICKS_PER_QUARTER * 4 * 4; // 4 bars
        ClipRegion newRegion(recordClip, currentTick, clipLength);
        track->addClipRegion(newRegion);
        
        arrangementView->repaint();
    }
    
    // Start recording on the MidiRecorder
    audioEngine.getMidiRecorder().startRecording(recordClip, currentTick);
    
    isRecording = true;
    
    // Start playback if not already playing
    if (!transport.isPlaying())
    {
        transport.start();
    }
}

void MainComponent::stopRecording()
{
    if (!isRecording)
        return;
    
    // Stop the MidiRecorder
    audioEngine.getMidiRecorder().stopRecording();
    
    isRecording = false;
    transportBar->setRecording(false);
    
    // Refresh arrangement view to show recorded notes
    arrangementView->repaint();
}

} // namespace pianodaw
