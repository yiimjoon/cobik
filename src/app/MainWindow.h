#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../core/model/Project.h"
#include "../core/edit/UndoStack.h"
#include "../core/timeline/Transport.h"
#include "../core/audio/AudioEngine.h"

namespace pianodaw {

class MainComponent;
class AppState;

//==============================================================================
class MainWindow : public juce::DocumentWindow,
                   public juce::MenuBarModel,
                   private juce::MidiInputCallback
{
public:
    MainWindow(juce::String name, AppState& appState);
    ~MainWindow() override;

    void closeButtonPressed() override;
    
    // MenuBarModel overrides
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
    // MidiInputCallback override
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    // Data
    AppState& appState;
    std::unique_ptr<Project> project;  // Owns the project
    UndoStack undoStack;
    Transport transport;
    std::unique_ptr<AudioEngine> audioEngine;
    juce::AudioProcessorPlayer audioPlayer;
    
    // UI Components
    std::unique_ptr<MainComponent> mainComponent;
    
    void createDemoProject();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace pianodaw
