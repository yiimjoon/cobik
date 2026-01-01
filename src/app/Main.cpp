#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "MainWindow.h"
#include "AppState.h"

//==============================================================================
class PianoDAWApplication : public juce::JUCEApplication
{
public:
    PianoDAWApplication() {}

    const juce::String getApplicationName() override { return "Piano DAW"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        
        appState = std::make_unique<pianodaw::AppState>();
        
        // Create main window
        mainWindow = std::make_unique<pianodaw::MainWindow>(getApplicationName(), *appState);
        
        // Log audio device info (Day 1 requirement)
        logAudioDevices();
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appState = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }

private:
    void logAudioDevices()
    {
        auto* deviceManager = appState->getAudioDeviceManager();
        
        DBG("=== Audio Devices ===");
        
        auto& deviceTypes = deviceManager->getAvailableDeviceTypes();
        for (auto* type : deviceTypes)
        {
            DBG("Device Type: " << type->getTypeName());
            
            auto deviceNames = type->getDeviceNames();
            for (int i = 0; i < deviceNames.size(); ++i)
            {
                DBG("  - " << deviceNames[i]);
            }
        }
        
        if (auto* currentDevice = deviceManager->getCurrentAudioDevice())
        {
            DBG("\nCurrent Device: " << currentDevice->getName());
            DBG("Sample Rate: " << currentDevice->getCurrentSampleRate());
            DBG("Buffer Size: " << currentDevice->getCurrentBufferSizeSamples());
        }
        
        DBG("====================");
    }

    std::unique_ptr<pianodaw::AppState> appState;
    std::unique_ptr<pianodaw::MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(PianoDAWApplication)
