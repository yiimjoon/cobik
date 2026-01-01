#include "AppState.h"

namespace pianodaw {

//==============================================================================
AppState::AppState()
{
    // Initialize audio device with default settings
    initializeAudioDevice();
    
    // Load saved settings
    loadSettings();
}

AppState::~AppState()
{
    saveSettings();
}

void AppState::initializeAudioDevice()
{
    // Set default number of input/output channels
    juce::String audioError = audioDeviceManager.initialise(
        0,      // numInputChannelsNeeded (0 for piano DAW)
        2,      // numOutputChannelsNeeded (stereo out)
        nullptr,
        true,   // selectDefaultDeviceOnFailure
        {},     // preferredDefaultDeviceName
        nullptr // preferredSetupOptions
    );

    if (audioError.isNotEmpty())
    {
        DBG("Audio initialization error: " << audioError);
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Audio Device Error",
            "Could not initialize audio device:\n" + audioError
        );
    }
}

void AppState::saveSettings()
{
    // TODO: Save app settings
}

void AppState::loadSettings()
{
    // TODO: Load app settings
}

} // namespace pianodaw
