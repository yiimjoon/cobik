#include "TransportBar.h"
#include "../panels/DebugLogWindow.h"

namespace pianodaw {

TransportBar::TransportBar()
{
    setupButtons();
}

TransportBar::~TransportBar()
{
}

void TransportBar::setupButtons()
{
    // Play button (▶)
    playButton = std::make_unique<juce::TextButton>("Play");
    playButton->onClick = [this]() {
        if (onPlay) onPlay();
    };
    addAndMakeVisible(playButton.get());
    
    // Stop button (■) - 두번 클릭하면 원위치
    stopButton = std::make_unique<juce::TextButton>("Stop");
    stopButton->onClick = [this]() {
        auto currentTime = juce::Time::getMillisecondCounter();
        
        // 더블클릭 감지 (500ms 이내)
        if (currentTime - lastStopClickTime < 500) {
            DebugLogWindow::addLog("Stop button: DOUBLE-CLICK detected - resetting to position 0");
            if (onStopDoubleClick) onStopDoubleClick();
        } else {
            DebugLogWindow::addLog("Stop button: Single click");
            if (onStop) onStop();
        }
        
        lastStopClickTime = currentTime;
    };
    addAndMakeVisible(stopButton.get());
    
    // Record button (●) - 클릭 시 즉시 녹음 시작 (Play와 동시)
    recordButton = std::make_unique<juce::TextButton>("Record");
    recordButton->onClick = [this]() {
        recording = !recording;
        
        // 색상 업데이트
        if (recording) {
            recordButton->setColour(juce::TextButton::buttonColourId, juce::Colours::red);
            recordButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        } else {
            recordButton->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff404040));
            recordButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
        
        if (onRecordToggle) onRecordToggle(recording);
        repaint();
    };
    recordButton->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff404040));
    addAndMakeVisible(recordButton.get());
    
    // Loop button (⟲)
    loopButton = std::make_unique<juce::ToggleButton>("Loop");
    loopButton->onClick = [this]() {
        if (onLoopToggle) onLoopToggle(loopButton->getToggleState());
    };
    addAndMakeVisible(loopButton.get());
    
    // Position label
    positionLabel = std::make_unique<juce::Label>("Position", "0:0:0");
    positionLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(positionLabel.get());
    
    // Tempo label
    tempoLabel = std::make_unique<juce::Label>("Tempo", "120 BPM");
    tempoLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tempoLabel.get());
}

void TransportBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    
    playButton->setBounds(bounds.removeFromLeft(80));
    bounds.removeFromLeft(4);
    
    stopButton->setBounds(bounds.removeFromLeft(80));
    bounds.removeFromLeft(4);
    
    recordButton->setBounds(bounds.removeFromLeft(80));
    bounds.removeFromLeft(4);
    
    loopButton->setBounds(bounds.removeFromLeft(80));
    bounds.removeFromLeft(20);
    
    positionLabel->setBounds(bounds.removeFromLeft(120));
    bounds.removeFromLeft(20);
    
    tempoLabel->setBounds(bounds.removeFromLeft(100));
}

void TransportBar::setPositionText(const juce::String& text)
{
    positionLabel->setText(text, juce::dontSendNotification);
}

void TransportBar::setTempoText(const juce::String& text)
{
    tempoLabel->setText(text, juce::dontSendNotification);
}

void TransportBar::setRecording(bool isRecording)
{
    recording = isRecording;
    
    // 색상 업데이트
    if (recording) {
        recordButton->setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    } else {
        recordButton->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff404040));
    }
    
    repaint();
}

} // namespace pianodaw
