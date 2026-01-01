#include "MainWindow.h"
#include "AppState.h"
#include "../ui/MainComponent.h"
#include "../ui/panels/DebugLogWindow.h"

namespace pianodaw {

//==============================================================================
MainWindow::MainWindow(juce::String name, AppState& appState_)
    : DocumentWindow("PianoDAW [v1.0.2 Test]",
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons),
      appState(appState_)
{
    setUsingNativeTitleBar(true);
    setMenuBar(this);
    
    // Create project with demo data
    project = std::make_unique<Project>("My Project");
    createDemoProject();

    audioEngine = std::make_unique<AudioEngine>(*project, transport);
    audioPlayer.setProcessor(audioEngine.get());
    
    auto* adm = appState.getAudioDeviceManager();
    adm->addAudioCallback(&audioPlayer);
    
    // Register MIDI input callback to receive MIDI from all enabled devices
    for (auto& device : juce::MidiInput::getAvailableDevices())
    {
        if (adm->isMidiInputDeviceEnabled(device.identifier))
        {
            adm->addMidiInputDeviceCallback(device.identifier, this);
        }
    }
    
    // Create UI (pass project to MainComponent)
    mainComponent = std::make_unique<MainComponent>(*project, undoStack, transport, *audioEngine);
    setContentNonOwned(mainComponent.get(), true);
    
    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, true);
        centreWithSize(1400, 900);
    #endif

    setVisible(true);
}

MainWindow::~MainWindow()
{
    // Unregister MIDI callbacks
    auto* adm = appState.getAudioDeviceManager();
    for (auto& device : juce::MidiInput::getAvailableDevices())
    {
        adm->removeMidiInputDeviceCallback(device.identifier, this);
    }
}

void MainWindow::createDemoProject()
{
    // Create demo track with notes
    auto* track = project->addTrack("Demo Track", Track::Type::MIDI);
    auto* clip = project->addClip("Demo Clip");
    
    // Add clip region to track (starts at bar 0, length 4 bars)
    int64_t bar = PPQ::TICKS_PER_QUARTER * 4;
    ClipRegion region(clip, 0, bar * 4);
    track->addClipRegion(region);
    
    // Bar 1: C major chord
    clip->addNote(60, 0, bar, 100);           // C
    clip->addNote(64, 0, bar, 95);            // E
    clip->addNote(67, 0, bar, 90);            // G
    
    // Bar 2: F major chord
    clip->addNote(65, bar, bar * 2, 100);     // F
    clip->addNote(69, bar, bar * 2, 95);      // A
    clip->addNote(72, bar, bar * 2, 90);      // C
    
    // Bar 3: G major chord
    clip->addNote(67, bar * 2, bar * 3, 100); // G
    clip->addNote(71, bar * 2, bar * 3, 95);  // B
    clip->addNote(74, bar * 2, bar * 3, 90);  // D
    
    // Bar 4: C major chord (octave up)
    clip->addNote(72, bar * 3, bar * 4, 100); // C
    clip->addNote(76, bar * 3, bar * 4, 95);  // E
    clip->addNote(79, bar * 3, bar * 4, 90);  // G
    
    // Add some melody notes
    int64_t eighth = PPQ::TICKS_PER_QUARTER / 2;
    clip->addNote(84, 0, eighth, 110);
    clip->addNote(86, eighth, eighth * 2, 105);
    clip->addNote(88, eighth * 2, eighth * 3, 100);
    clip->addNote(86, eighth * 3, bar, 95);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

juce::StringArray MainWindow::getMenuBarNames()
{
    return { "File", "Edit" };
}

juce::PopupMenu MainWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;
    
    if (topLevelMenuIndex == 0) // File
    {
        menu.addItem(1, "New Project", true);
        menu.addItem(2, "Open Project...", true);
        menu.addSeparator();
        menu.addItem(3, "Save Project", mainComponent != nullptr);
        menu.addItem(4, "Save Project As...", mainComponent != nullptr);
        menu.addSeparator();
        menu.addItem(5, "Export MIDI...", mainComponent != nullptr);
        menu.addSeparator();
        menu.addItem(6, "Show Debug Log", true);  // 디버그 로그 창
        menu.addSeparator();
        menu.addItem(99, "Quit", true);
    }
    else if (topLevelMenuIndex == 1) // Edit
    {
        menu.addItem(10, "Undo", undoStack.canUndo());
        menu.addItem(11, "Redo", undoStack.canRedo());
    }
    
    return menu;
}

void MainWindow::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    switch (menuItemID)
    {
        case 1: // New Project
            // TODO: Implement new project
            break;
            
        case 2: // Open Project
        {
            auto chooser = std::make_shared<juce::FileChooser>("Open Project", juce::File(), "*.pianodaw");
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file.existsAsFile() && project)
                    {
                        if (project->loadFromFile(file))
                        {
                            mainComponent->resized();
                            mainComponent->repaint();
                        }
                        else
                        {
                            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                "Open Failed", "Failed to open project file.");
                        }
                    }
                });
            break;
        }
            
        case 3: // Save Project
        {
            if (project)
            {
                auto file = project->getProjectFile();
                if (file.existsAsFile())
                {
                    project->saveToFile(file);
                }
                else
                {
                    // If no file set, do Save As
                    menuItemSelected(4, topLevelMenuIndex);
                }
            }
            break;
        }
            
        case 4: // Save Project As
        {
            auto chooser = std::make_shared<juce::FileChooser>("Save Project As", juce::File(), "*.pianodaw");
            chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file != juce::File() && project)
                    {
                        project->saveToFile(file);
                    }
                });
            break;
        }
            
        case 5: // Export MIDI
        {
            auto chooser = std::make_shared<juce::FileChooser>("Export MIDI", juce::File(), "*.mid");
            chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file != juce::File() && project)
                    {
                        auto& clips = project->getClips();
                        if (!clips.empty())
                        {
                            clips[0]->exportToMidiFile(file);
                        }
                    }
                });
            break;
        }
        
        case 6: // Show Debug Log
        {
            DebugLogWindow::getInstance()->setVisible(true);
            DebugLogWindow::getInstance()->toFront(true);
            break;
        }
            
        case 10: // Undo
            undoStack.undo();
            break;
            
        case 11: // Redo
            undoStack.redo();
            break;
            
        case 99: // Quit
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;
    }
}

void MainWindow::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    // Forward MIDI input to AudioEngine
    if (audioEngine)
    {
        audioEngine->handleIncomingMidiMessage(source, message);
    }
}

} // namespace pianodaw

