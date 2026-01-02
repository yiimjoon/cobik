#include "MainComponent.h"
#include "../core/edit/UndoStack.h"
#include "../core/edit/EditCommands.h"
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
    
    // Set selection changed callback (will be connected to InfoLine later)
    pianoRollView->onSelectionChanged = [this](const std::vector<int>& selectedIds) {
        if (infoLine) {
            infoLine->setSelectedNotes(selectedIds);
        }
        if (statusLine) {
            // TODO: Update chord detection based on selection
        }
    };
    
    // Set mouse position changed callback for status line
    pianoRollView->onMousePositionChanged = [this](int64_t tick, int pitch) {
        if (statusLine) {
            statusLine->updatePosition(tick, pitch);
        }
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
    
    // Connect InfoLine callbacks
    infoLine->onStartChanged = [this, &editClip](int noteId, int64_t newStart) {
        auto* note = editClip.findNote(noteId);
        if (note && newStart >= 0 && newStart < note->endTick) {
            undoStack.execute(std::make_unique<ResizeNoteCommand>(editClip, noteId, newStart, note->endTick));
            pianoRollView->repaint();
            infoLine->setSelectedNotes({noteId});
        }
    };
    
    infoLine->onEndChanged = [this, &editClip](int noteId, int64_t newEnd) {
        auto* note = editClip.findNote(noteId);
        if (note && newEnd > note->startTick) {
            undoStack.execute(std::make_unique<ResizeNoteCommand>(editClip, noteId, note->startTick, newEnd));
            pianoRollView->repaint();
            infoLine->setSelectedNotes({noteId});
        }
    };
    
    infoLine->onLengthChanged = [this, &editClip](int noteId, int64_t newLength) {
        auto* note = editClip.findNote(noteId);
        if (note && newLength > 0) {
            int64_t newEnd = note->startTick + newLength;
            undoStack.execute(std::make_unique<ResizeNoteCommand>(editClip, noteId, note->startTick, newEnd));
            pianoRollView->repaint();
            infoLine->setSelectedNotes({noteId});
        }
    };
    
    infoLine->onPitchChanged = [this, &editClip](int noteId, int newPitch) {
        auto* note = editClip.findNote(noteId);
        if (note && newPitch >= 0 && newPitch <= 127) {
            int semitones = newPitch - note->pitch;
            undoStack.execute(std::make_unique<TransposeCommand>(editClip, std::vector<int>{noteId}, semitones));
            pianoRollView->repaint();
            infoLine->setSelectedNotes({noteId});
        }
    };
    
    infoLine->onVelocityChanged = [this, &editClip](int noteId, int newVelocity) {
        // For multi-selection (InfoLine allows batch velocity edit)
        const auto& selectedIds = pianoRollView->getSelectedNoteIds();
        std::vector<int> targetIds = selectedIds.empty() ? std::vector<int>{noteId} : selectedIds;
        
        if (newVelocity >= 1 && newVelocity <= 127) {
            undoStack.execute(std::make_unique<SetVelocityCommand>(editClip, targetIds, newVelocity));
            pianoRollView->repaint();
            velocityLane->repaint();
            infoLine->setSelectedNotes(targetIds);
        }
    };
    
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
        
        // Connect toolbar callbacks
        toolbar->onToolChanged = [this](EditorTool tool) {
            if (pianoRollView) {
                pianoRollView->setCurrentTool(tool);
                DebugLogWindow::addLog("Tool changed to: " + juce::String((int)tool));
            }
        };
        
        toolbar->onSnapChanged = [this](bool enabled) {
            if (pianoRollView) {
                pianoRollView->setSnapEnabled(enabled);
                if (statusLine) {
                    statusLine->updateSnap(enabled, toolbar->getGridSize() == GridSize::Whole ? "1/1" :
                                                     toolbar->getGridSize() == GridSize::Half ? "1/2" :
                                                     toolbar->getGridSize() == GridSize::Quarter ? "1/4" :
                                                     toolbar->getGridSize() == GridSize::Eighth ? "1/8" :
                                                     toolbar->getGridSize() == GridSize::Sixteenth ? "1/16" : "1/32");
                }
                DebugLogWindow::addLog("Snap: " + juce::String(enabled ? "ON" : "OFF"));
            }
        };
        
        toolbar->onGridSizeChanged = [this](GridSize size) {
            if (pianoRollView) {
                pianoRollView->setGridSize(size);
                if (statusLine) {
                    statusLine->updateSnap(toolbar->isSnapEnabled(), 
                                          size == GridSize::Whole ? "1/1" :
                                          size == GridSize::Half ? "1/2" :
                                          size == GridSize::Quarter ? "1/4" :
                                          size == GridSize::Eighth ? "1/8" :
                                          size == GridSize::Sixteenth ? "1/16" : "1/32");
                }
                DebugLogWindow::addLog("Grid size changed");
            }
        };
        
        toolbar->onHorizontalZoomChanged = [this](double zoom) {
            if (pianoRollView) {
                pianoRollView->setPixelsPerTick(zoom);
                DebugLogWindow::addLog("H-Zoom: " + juce::String(zoom));
            }
        };
        
        toolbar->onVerticalZoomChanged = [this](double zoom) {
            if (pianoRollView) {
                pianoRollView->setPixelsPerKey(zoom);
                DebugLogWindow::addLog("V-Zoom: " + juce::String(zoom));
            }
        };
    } else {
        toolbar->setVisible(true);
    }
    
    if (!inspector) {
        inspector = std::make_unique<InspectorPanel>();
        addAndMakeVisible(inspector.get());
        
        // Connect inspector callbacks
        inspector->onQuantize = [this, &editClip]() {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (selectedIds.empty()) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Quantize", "No notes selected.");
                return;
            }
            
            QuantizeParams params;
            params.gridTicks = gridSizeToTicks(toolbar->getGridSize());
            params.strength = 1.0f; // TODO: Get from inspector slider
            params.swing = 0.5f;    // TODO: Get from inspector slider
            
            undoStack.execute(std::make_unique<QuantizeCommand>(editClip, selectedIds, params));
            pianoRollView->repaint();
            DebugLogWindow::addLog("Quantized " + juce::String(selectedIds.size()) + " notes");
        };
        
        inspector->onScaleLengthChanged = [this, &editClip](double percentage) {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (!selectedIds.empty()) {
                undoStack.execute(std::make_unique<ScaleLengthCommand>(editClip, selectedIds, percentage));
                pianoRollView->repaint();
                DebugLogWindow::addLog("Scaled length: " + juce::String(percentage) + "%");
            }
        };
        
        inspector->onLegato = [this, &editClip]() {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (selectedIds.empty()) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Legato", "No notes selected.");
                return;
            }
            
            undoStack.execute(std::make_unique<LegatoCommand>(editClip, selectedIds));
            pianoRollView->repaint();
            DebugLogWindow::addLog("Applied legato to " + juce::String(selectedIds.size()) + " notes");
        };
        
        inspector->onOverlapChanged = [this, &editClip](int ticks) {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (!selectedIds.empty()) {
                undoStack.execute(std::make_unique<SetOverlapCommand>(editClip, selectedIds, ticks));
                pianoRollView->repaint();
                DebugLogWindow::addLog("Overlap adjusted: " + juce::String(ticks) + " ticks");
            }
        };
        
        inspector->onTranspose = [this, &editClip](int semitones) {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (!selectedIds.empty()) {
                undoStack.execute(std::make_unique<TransposeCommand>(editClip, selectedIds, semitones));
                pianoRollView->repaint();
                if (infoLine) infoLine->setSelectedNotes(selectedIds);
                DebugLogWindow::addLog("Transposed: " + juce::String(semitones) + " semitones");
            }
        };
        
        inspector->onScaleChanged = [this](int root, MusicTheory::ScaleType type) {
            if (pianoRollView) {
                pianoRollView->setScale(root, type);
                DebugLogWindow::addLog("Scale changed");
            }
        };
        
        inspector->onChordModeChanged = [this](MusicTheory::ChordType type) {
            if (pianoRollView) {
                pianoRollView->setChordMode(type);
                DebugLogWindow::addLog("Chord mode changed");
            }
        };
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
