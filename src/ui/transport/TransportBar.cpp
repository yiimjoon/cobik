#include "TransportBar.h"

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
    // Play button
    playButton = std::make_unique<juce::TextButton>("Play");
    playButton->onClick = [this]() {
        if (onPlay) onPlay();
    };
    addAndMakeVisible(playButton.get());
    
    // Stop button
    stopButton = std::make_unique<juce::TextButton>("Stop");
    stopButton->onClick = [this]() {
        if (onStop) onStop();
    };
    addAndMakeVisible(stopButton.get());
    
    // Loop button
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

} // namespace pianodaw
