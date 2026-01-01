#include "MainWindow.h"
#include "AppState.h"
#include "../ui/MainComponent.h"

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
    
    // Create data - no longer need createDemoNotes() as Project will have demo data
    // createDemoNotes();

    audioEngine = std::make_unique<AudioEngine>(clip, transport);
    audioPlayer.setProcessor(audioEngine.get());
    
    auto* adm = appState.getAudioDeviceManager();
    adm->addAudioCallback(&audioPlayer);
    
    // Create UI (MainComponent now creates its own Project)
    mainComponent = std::make_unique<MainComponent>(undoStack, transport, *audioEngine);
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
}

void MainWindow::createDemoNotes()
{
    // Create some demo notes for visualization
    // Simple C major chord progression
    
    int64_t bar = PPQ::TICKS_PER_QUARTER * 4;
    
    // Bar 1: C major chord
    clip.addNote(60, 0, bar, 100);           // C
    clip.addNote(64, 0, bar, 95);            // E
    clip.addNote(67, 0, bar, 90);            // G
    
    // Bar 2: F major chord
    clip.addNote(65, bar, bar * 2, 100);     // F
    clip.addNote(69, bar, bar * 2, 95);      // A
    clip.addNote(72, bar, bar * 2, 90);      // C
    
    // Bar 3: G major chord
    clip.addNote(67, bar * 2, bar * 3, 100); // G
    clip.addNote(71, bar * 2, bar * 3, 95);  // B
    clip.addNote(74, bar * 2, bar * 3, 90);  // D
    
    // Bar 4: C major chord (octave up)
    clip.addNote(72, bar * 3, bar * 4, 100); // C
    clip.addNote(76, bar * 3, bar * 4, 95);  // E
    clip.addNote(79, bar * 3, bar * 4, 90);  // G
    
    // Add some melody notes
    int64_t eighth = PPQ::TICKS_PER_QUARTER / 2;
    clip.addNote(84, 0, eighth, 110);
    clip.addNote(86, eighth, eighth * 2, 105);
    clip.addNote(88, eighth * 2, eighth * 3, 100);
    clip.addNote(86, eighth * 3, bar, 95);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace pianodaw

