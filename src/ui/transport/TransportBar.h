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
    std::function<void(bool)> onLoopToggle;
    
private:
    std::unique_ptr<juce::TextButton> playButton;
    std::unique_ptr<juce::TextButton> stopButton;
    std::unique_ptr<juce::ToggleButton> loopButton;
    std::unique_ptr<juce::Label> positionLabel;
    std::unique_ptr<juce::Label> tempoLabel;
    
    void setupButtons();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace pianodaw
