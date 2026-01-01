#include "AppState.h"
#include "../ui/panels/DebugLogWindow.h"

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
        2,      // numInputChannelsNeeded (2 for MIDI recording + future audio recording)
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
    
    // Enable ALL MIDI input devices (for keyboard support)
    audioDeviceManager.setMidiInputDeviceEnabled(juce::String(), true);  // Enable default
    
    // Enable all available MIDI devices
    auto devices = juce::MidiInput::getAvailableDevices();
    DebugLogWindow::addLog("=== MIDI Device Detection ===");
    DebugLogWindow::addLog("Found " + juce::String(devices.size()) + " MIDI device(s)");
    
    for (auto& device : devices)
    {
        audioDeviceManager.setMidiInputDeviceEnabled(device.identifier, true);
        DBG("Enabled MIDI device: " << device.name);
        DebugLogWindow::addLog("Enabled: " + device.name);
    }
    
    if (devices.isEmpty()) {
        DebugLogWindow::addLog("WARNING: No MIDI devices found!");
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
