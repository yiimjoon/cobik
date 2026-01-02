#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pianodaw {

/**
 * Transport Bar - Play/Stop/Loop controls
 */
class TransportBar : public juce::Component
{
public:
    TransportBar();
    ~TransportBar() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setPositionText(const juce::String& text);
    void setTempoText(const juce::String& text);
    
    // Callbacks
    std::function<void()> onPlay;
    std::function<void()> onPlayDoubleClick;  // Play double-click -> toggle
    std::function<void()> onStop;
    std::function<void()> onStopDoubleClick;  // Stop double-click -> rewind
    std::function<void()> onRecordToggle;
    std::function<void(bool)> onLoopToggle;
    
    // State
    void setRecording(bool isRecording);
    bool isRecording() const { return recording; }
    
private:
    std::unique_ptr<juce::TextButton> playButton;
    std::unique_ptr<juce::TextButton> stopButton;
    std::unique_ptr<juce::TextButton> recordButton;
    std::unique_ptr<juce::ToggleButton> loopButton;
    std::unique_ptr<juce::Label> positionLabel;
    std::unique_ptr<juce::Label> tempoLabel;
    
    bool recording = false;
    juce::uint32 lastStopClickTime = 0;  // For Stop double-click detection
    juce::uint32 lastPlayClickTime = 0;  // For Play double-click detection
    
    void setupButtons();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace pianodaw
