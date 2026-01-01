#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

namespace pianodaw {

//==============================================================================
/**
 * Central application state
 * Manages audio device, project data, and global settings
 */
class AppState
{
public:
    AppState();
    ~AppState();

    // Audio device management
    juce::AudioDeviceManager* getAudioDeviceManager() { return &audioDeviceManager; }
    
    // Settings
    void saveSettings();
    void loadSettings();

private:
    juce::AudioDeviceManager audioDeviceManager;
    juce::ApplicationProperties appProperties;
    
    void initializeAudioDevice();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};

} // namespace pianodaw
