#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/timeline/PPQ.h"

namespace pianodaw {

/**
 * StatusLine - Displays current cursor position and musical context
 * 
 * Shows: Bar:Beat:Tick, Pitch name, Chord detection, Snap/Grid status
 */
class StatusLine : public juce::Component
{
public:
    StatusLine()
    {
        addAndMakeVisible(positionLabel);
        positionLabel.setJustificationType(juce::Justification::centredLeft);
        positionLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        
        addAndMakeVisible(pitchLabel);
        pitchLabel.setJustificationType(juce::Justification::centredLeft);
        pitchLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        
        addAndMakeVisible(chordLabel);
        chordLabel.setJustificationType(juce::Justification::centredLeft);
        chordLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
        
        addAndMakeVisible(snapLabel);
        snapLabel.setJustificationType(juce::Justification::centredRight);
        snapLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        
        updatePosition(0, 0);
        updateSnap(true, "1/16");
    }
    
    void updatePosition(int64_t ticks, int pitch)
    {
        // Format: Bar:Beat:Tick
        int64_t ppq = PPQ::TICKS_PER_QUARTER;
        int bar = (int)(ticks / (ppq * 4)) + 1;
        int beat = (int)((ticks % (ppq * 4)) / ppq) + 1;
        int tick = (int)(ticks % ppq);
        
        juce::String posText = juce::String(bar) + ":" + juce::String(beat) + ":" + juce::String(tick);
        positionLabel.setText(posText, juce::dontSendNotification);
        
        // Format: Pitch (e.g., "C4 (60)")
        if (pitch >= 0 && pitch <= 127)
        {
            const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            int octave = (pitch / 12) - 1;
            int note = pitch % 12;
            juce::String pitchText = juce::String(names[note]) + juce::String(octave) + " (" + juce::String(pitch) + ")";
            pitchLabel.setText(pitchText, juce::dontSendNotification);
        }
        else
        {
            pitchLabel.setText("---", juce::dontSendNotification);
        }
    }
    
    void updateChord(const juce::String& chordName)
    {
        if (chordName.isEmpty())
            chordLabel.setText("", juce::dontSendNotification);
        else
            chordLabel.setText("Chord: " + chordName, juce::dontSendNotification);
    }
    
    void updateSnap(bool enabled, const juce::String& gridSize)
    {
        juce::String snapText = "Snap: ";
        if (enabled)
            snapText += "ON (" + gridSize + ")";
        else
            snapText += "OFF";
        
        snapLabel.setText(snapText, juce::dontSendNotification);
        snapLabel.setColour(juce::Label::textColourId, enabled ? juce::Colours::lightgreen : juce::Colours::grey);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1a1a1a));
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(getLocalBounds());
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        
        positionLabel.setBounds(area.removeFromLeft(150));
        area.removeFromLeft(10);
        
        pitchLabel.setBounds(area.removeFromLeft(150));
        area.removeFromLeft(10);
        
        chordLabel.setBounds(area.removeFromLeft(200));
        
        snapLabel.setBounds(area.removeFromRight(150));
    }
    
private:
    juce::Label positionLabel;
    juce::Label pitchLabel;
    juce::Label chordLabel;
    juce::Label snapLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusLine)
};

} // namespace pianodaw
