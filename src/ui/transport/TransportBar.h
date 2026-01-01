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
    std::function<void()> onStop;
    std::function<void()> onStopDoubleClick;  // Stop 두번 클릭 → 원위치
    std::function<void(bool)> onLoopToggle;
    std::function<void(bool)> onRecordToggle;
    
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
    juce::uint32 lastStopClickTime = 0;  // Stop 더블클릭 감지용
    
    void setupButtons();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace pianodaw
