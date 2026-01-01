#include "MainComponent.h"
#include "pianoroll/PianoRollView.h"
#include "pianoroll/VelocityLane.h"
#include "pianoroll/PedalLane.h"
#include "panels/VstBrowserPanel.h"
#include "panels/QuantizePanel.h"
#include "panels/PromptBar.h"
#include "panels/TheoryToolbar.h"
#include "../ui/transport/TransportBar.h"
#include "../core/edit/UndoStack.h"
#include "../core/timeline/PPQ.h"
#include "../core/timeline/Transport.h"
#include "../core/ai/LLMClient.h"
#include "../core/ai/CommandInterpreter.h"

namespace pianodaw {

MainComponent::MainComponent(Clip& clip_, UndoStack& undoStack_, Transport& transport_, AudioEngine& engine_)
    : clip(clip_), undoStack(undoStack_), transport(transport_), audioEngine(engine_),
      resizerBar1(&layoutManager, 1, false),
      resizerBar2(&layoutManager, 3, false),
      resizerBar3(&layoutManager, 5, false),
      resizerBar4(&layoutManager, 7, false),
      inspectorResizer(nullptr, 0, true)
{
    // Phase 1: New UI Components
    infoLine = std::make_unique<InfoLine>(clip);
    statusLine = std::make_unique<StatusLine>();
    toolbar = std::make_unique<PianoRollToolbar>();
    inspector = std::make_unique<InspectorPanel>();
    
    // 1. Initialize all components
    transportBar = std::make_unique<TransportBar>();
    promptBar = std::make_unique<PromptBar>();
    theoryToolbar = std::make_unique<TheoryToolbar>();
    
    pianoRollView = std::make_unique<PianoRollView>(clip, undoStack, transport, audioEngine);
    velocityLane = std::make_unique<VelocityLane>(clip, undoStack); // Phase 5: Added undoStack
    pedalLane = std::make_unique<PedalLane>(clip, undoStack); // Phase 5: Added undoStack
    vstBrowser = std::make_unique<VstBrowserPanel>(audioEngine);
    quantizePanel = std::make_unique<QuantizePanel>(clip, undoStack);
    
    llmClient = std::make_unique<LLMClient>();
    
    // Check for Gemini API Key in environment
    juce::String geminiKey = juce::SystemStats::getEnvironmentVariable("PIANODAW_GEMINI_KEY", "");
    if (geminiKey.isNotEmpty())
    {
        llmClient->setProvider(LLMClient::Provider::Gemini);
        llmClient->setApiKey(geminiKey);
        llmClient->setModel("gemini-1.5-flash");
    }

    interpreter = std::make_unique<CommandInterpreter>(clip);

    horizontalScrollBar = std::make_unique<juce::ScrollBar>(true);
    verticalScrollBar = std::make_unique<juce::ScrollBar>(false);

    // 2. Setup callbacks
    theoryToolbar->onScaleChanged = [this](int root, MusicTheory::ScaleType type) {
        pianoRollView->setScale(root, type);
        const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        juce::String scaleName = juce::String(names[root % 12]);
        if (type == MusicTheory::ScaleType::Major) scaleName += " Major";
        else if (type == MusicTheory::ScaleType::Minor) scaleName += " Minor";
        llmClient->setSystemPrompt("You are a MIDI expert AI. CURRENT SCALE: " + scaleName);
    };

    theoryToolbar->onChordChanged = [this](MusicTheory::ChordType type) {
        pianoRollView->setChordMode(type);
    };

    promptBar->onExecute = [this](const juce::String& text) {
        if (text.isEmpty()) return;
        promptBar->setBusy(true);
        llmClient->sendPrompt(text, [this](const LLMClient::Response& res) {
            if (res.success) {
                auto result = interpreter->interpret(res.text);
                if (!result.commands.empty()) {
                    for (auto& cmd : result.commands) undoStack.execute(std::move(cmd));
                    juce::String status = "Executed!";
                    if (result.reasoning.isNotEmpty()) status << " (" << result.reasoning << ")";
                    promptBar->setStatus(status);
                } else {
                    promptBar->setStatus("AI: " + res.text.substring(0, 50) + "...", false);
                }
            } else {
                promptBar->setStatus("AI Error: " + res.error, true);
            }
            promptBar->setBusy(false);
        });
    };

    pianoRollView->onSelectionChanged = [this](const std::vector<int>& ids) {
        if (quantizePanel) quantizePanel->setSelectedNoteIds(ids);
        if (velocityLane) velocityLane->setSelectedNotes(ids); // Phase 5: Sync velocity lane
    };

    transport.onPositionChanged = [this](int64_t ticks) {
        int64_t ppq = PPQ::TICKS_PER_QUARTER;
        int bar = (int)(ticks / (ppq * 4)) + 1;
        int beat = (int)((ticks % (ppq * 4)) / ppq) + 1;
        int tick = (int)(ticks % ppq);
        
        juce::String posStr;
        posStr << bar << ":" << beat << ":" << tick;
        if (transportBar) transportBar->setPositionText(posStr);
        if (pianoRollView) pianoRollView->repaint(); 
    };

    transportBar->onPlay = [this]() { transport.start(); };
    transportBar->onStop = [this]() { transport.stop(); };
    transportBar->onLoopToggle = [this](bool enabled) { transport.setLooping(enabled); };

    pianoRollView->onViewChanged = [this](int64_t start, double ppt) {
        if (velocityLane) velocityLane->setViewRange(start, ppt);
        if (pedalLane) pedalLane->setViewRange(start, ppt);
        int visibleWidth = pianoRollView->getWidth() - 80;
        if (horizontalScrollBar) 
            horizontalScrollBar->setCurrentRange((double)start, (double)visibleWidth / ppt, juce::dontSendNotification);
    };

    horizontalScrollBar->addListener(this);
    horizontalScrollBar->setRangeLimits(0.0, 960.0 * 100.0);
    verticalScrollBar->addListener(this);
    verticalScrollBar->setRangeLimits(21, 108);
    
    // Phase 2: Toolbar callbacks
    if (toolbar)
    {
        toolbar->onToolChanged = [this](EditorTool tool) {
            if (pianoRollView) pianoRollView->setCurrentTool(tool);
        };
        
        toolbar->onSnapChanged = [this](bool enabled) {
            if (pianoRollView) pianoRollView->setSnapEnabled(enabled);
            if (statusLine) statusLine->updateSnap(enabled, gridSizeToString(pianoRollView->getGridSize()));
        };
        
        toolbar->onGridSizeChanged = [this](GridSize size) {
            if (pianoRollView) pianoRollView->setGridSize(size);
            if (statusLine) statusLine->updateSnap(toolbar->isSnapEnabled(), gridSizeToString(size));
        };
        
        toolbar->onHorizontalZoomChanged = [this](double zoom) {
            if (pianoRollView) pianoRollView->setPixelsPerTick(zoom);
        };
        
        toolbar->onVerticalZoomChanged = [this](double zoom) {
            if (pianoRollView) pianoRollView->setPixelsPerKey(zoom);
        };
    }
    
    // Phase 1: InfoLine callbacks
    if (infoLine)
    {
        pianoRollView->onSelectionChanged = [this](const std::vector<int>& ids) {
            if (infoLine) infoLine->setSelectedNotes(ids);
            if (quantizePanel) quantizePanel->setSelectedNoteIds(ids);
        };
        
        // InfoLine edit callbacks
        infoLine->onVelocityChanged = [this](int noteId, int newVelocity) {
            std::vector<int> ids = {noteId};
            undoStack.execute(std::make_unique<SetVelocityCommand>(clip, ids, newVelocity));
            pianoRollView->repaint();
        };
    }
    
    // Phase 3: InspectorPanel callbacks
    if (inspector)
    {
        // Scale Length
        inspector->onScaleLengthChanged = [this](double percentage) {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (!selectedIds.empty())
            {
                undoStack.execute(std::make_unique<ScaleLengthCommand>(clip, selectedIds, percentage));
                pianoRollView->repaint();
            }
        };
        
        // Legato
        inspector->onLegato = [this]() {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (!selectedIds.empty())
            {
                undoStack.execute(std::make_unique<LegatoCommand>(clip, selectedIds));
                pianoRollView->repaint();
            }
        };
        
        // Overlap
        inspector->onOverlapChanged = [this](int ticks) {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (!selectedIds.empty())
            {
                undoStack.execute(std::make_unique<SetOverlapCommand>(clip, selectedIds, ticks));
                pianoRollView->repaint();
            }
        };
        
        // Transpose
        inspector->onTranspose = [this](int semitones) {
            const auto& selectedIds = pianoRollView->getSelectedNoteIds();
            if (!selectedIds.empty())
            {
                undoStack.execute(std::make_unique<TransposeCommand>(clip, selectedIds, semitones));
                pianoRollView->repaint();
            }
        };
        
        // Scale Changed (forward to theory toolbar logic)
        inspector->onScaleChanged = [this](int root, MusicTheory::ScaleType type) {
            if (pianoRollView) pianoRollView->setScale(root, type);
            const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            juce::String scaleName = juce::String(names[root % 12]);
            if (type == MusicTheory::ScaleType::Major) scaleName += " Major";
            else if (type == MusicTheory::ScaleType::Minor) scaleName += " Minor";
            if (llmClient) llmClient->setSystemPrompt("You are a MIDI expert AI. CURRENT SCALE: " + scaleName);
        };
        
        // Chord Mode Changed
        inspector->onChordModeChanged = [this](MusicTheory::ChordType type) {
            if (pianoRollView) pianoRollView->setChordMode(type);
        };
    }

    // 3. Setup Layout
    layoutManager.setItemLayout(0, -0.1, -0.9, -0.4); 
    layoutManager.setItemLayout(1, 5, 5, 5);          
    layoutManager.setItemLayout(2, -0.05, -0.5, -0.15); 
    layoutManager.setItemLayout(3, 5, 5, 5);          
    layoutManager.setItemLayout(4, -0.05, -0.5, -0.15); 
    layoutManager.setItemLayout(5, 5, 5, 5);           
    layoutManager.setItemLayout(6, -0.05, -0.5, -0.15); 
    layoutManager.setItemLayout(7, 5, 5, 5);           
    layoutManager.setItemLayout(8, -0.05, -0.5, -0.15); 

    // 4. Add all components
    // Phase 1: New UI Components
    addAndMakeVisible(statusLine.get());
    addAndMakeVisible(infoLine.get());
    addAndMakeVisible(toolbar.get());
    addAndMakeVisible(inspector.get());
    
    // Existing components
    addAndMakeVisible(promptBar.get());
    addAndMakeVisible(transportBar.get());
    addAndMakeVisible(theoryToolbar.get());
    addAndMakeVisible(pianoRollView.get());
    addAndMakeVisible(resizerBar1);
    addAndMakeVisible(velocityLane.get());
    addAndMakeVisible(resizerBar2);
    addAndMakeVisible(pedalLane.get());
    addAndMakeVisible(resizerBar3);
    addAndMakeVisible(vstBrowser.get());
    addAndMakeVisible(resizerBar4);
    addAndMakeVisible(quantizePanel.get());
    addAndMakeVisible(horizontalScrollBar.get());
    addAndMakeVisible(verticalScrollBar.get());
    
    setWantsKeyboardFocus(true);
}

MainComponent::~MainComponent() {}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Phase 4.5: Undo/Redo shortcuts
    if (key.getModifiers().isCommandDown())
    {
        if (key.getModifiers().isShiftDown() && key.getKeyCode() == 'Z')
        {
            // Ctrl+Shift+Z: Redo
            if (undoStack.redo())
            {
                pianoRollView->repaint();
                velocityLane->repaint();
                return true;
            }
        }
        else if (key.getKeyCode() == 'Z')
        {
            // Ctrl+Z: Undo
            if (undoStack.undo())
            {
                pianoRollView->repaint();
                velocityLane->repaint();
                return true;
            }
        }
    }
    
    // Phase 5: Velocity Lane edit mode shortcuts
    if (key.getKeyCode() == '1')
    {
        velocityLane->setEditMode(VelocityLane::EditMode::Draw);
        // TODO: Show mode in status line
        return true;
    }
    else if (key.getKeyCode() == '2')
    {
        velocityLane->setEditMode(VelocityLane::EditMode::Line);
        // TODO: Show mode in status line
        return true;
    }
    else if (key.getKeyCode() == '3')
    {
        velocityLane->setEditMode(VelocityLane::EditMode::Ramp);
        // TODO: Show mode in status line
        return true;
    }
    
    if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        if (transport.isPlaying()) transport.stop();
        else transport.start();
        return true;
    }
    return false;
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Phase 1: New UI Layout (Top to Bottom)
    
    // StatusLine (top)
    if (statusLine)
    {
        auto statusArea = bounds.removeFromTop(30);
        statusLine->setBounds(statusArea);
    }
    
    // InfoLine
    if (infoLine)
    {
        auto infoArea = bounds.removeFromTop(40);
        infoLine->setBounds(infoArea);
    }
    
    // PromptBar
    auto promptArea = bounds.removeFromTop(40);
    promptBar->setBounds(promptArea);

    // TransportBar
    auto transportArea = bounds.removeFromTop(40);
    transportBar->setBounds(transportArea);

    // PianoRollToolbar (Phase 1)
    if (toolbar)
    {
        auto toolbarArea = bounds.removeFromTop(50);
        toolbar->setBounds(toolbarArea);
    }

    // TheoryToolbar (legacy, can be removed later or kept)
    auto theoryArea = bounds.removeFromTop(40);
    theoryToolbar->setBounds(theoryArea.reduced(5, 0));

    // Inspector Panel (Left side, fixed width 250px)
    if (inspector)
    {
        auto inspectorWidth = 250;
        auto inspectorArea = bounds.removeFromLeft(inspectorWidth);
        inspector->setBounds(inspectorArea);
        // TODO: Add resizer for inspector
    }

    // Scroll bars
    auto scrollBarSize = 16;
    auto hScrollArea = bounds.removeFromBottom(scrollBarSize);
    auto vScrollArea = bounds.removeFromRight(scrollBarSize);
    
    horizontalScrollBar->setBounds(hScrollArea.withTrimmedLeft(80));
    verticalScrollBar->setBounds(vScrollArea);

    // Main content area (Piano Roll + Velocity + Pedal + VST + Quantize)
    juce::Component* comps[] = { 
        pianoRollView.get(), &resizerBar1, 
        velocityLane.get(), &resizerBar2, 
        pedalLane.get(), &resizerBar3,
        vstBrowser.get(), &resizerBar4,
        quantizePanel.get()
    };
    
    const int numComps = 9;
    for (int i = 0; i < numComps; ++i) if (!comps[i]) return;

    layoutManager.layOutComponents(comps, numComps, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true, true);
}

void MainComponent::scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart)
{
    if (scrollBar == horizontalScrollBar.get())
        pianoRollView->setViewStartTick((int64_t)newRangeStart);
    else if (scrollBar == verticalScrollBar.get())
        pianoRollView->setViewStartPitch((int)newRangeStart);
}

} // namespace pianodaw
