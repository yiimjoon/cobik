#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/model/Clip.h"
#include "../../core/timeline/PPQ.h"
#include <functional>

namespace pianodaw {

/**
 * InfoLine - Displays and allows editing of selected note properties
 * 
 * Shows: Start, End, Length, Pitch, Velocity for selected note(s)
 * Supports direct editing via text fields
 */
class InfoLine : public juce::Component
{
public:
    InfoLine(Clip& clip_)
        : clip(clip_)
    {
        // Labels
        addAndMakeVisible(startLabel);
        startLabel.setText("Start:", juce::dontSendNotification);
        startLabel.setJustificationType(juce::Justification::centredRight);
        
        addAndMakeVisible(endLabel);
        endLabel.setText("End:", juce::dontSendNotification);
        endLabel.setJustificationType(juce::Justification::centredRight);
        
        addAndMakeVisible(lengthLabel);
        lengthLabel.setText("Length:", juce::dontSendNotification);
        lengthLabel.setJustificationType(juce::Justification::centredRight);
        
        addAndMakeVisible(pitchLabel);
        pitchLabel.setText("Pitch:", juce::dontSendNotification);
        pitchLabel.setJustificationType(juce::Justification::centredRight);
        
        addAndMakeVisible(velocityLabel);
        velocityLabel.setText("Velocity:", juce::dontSendNotification);
        velocityLabel.setJustificationType(juce::Justification::centredRight);
        
        // Editors
        addAndMakeVisible(startEditor);
        startEditor.setJustification(juce::Justification::centred);
        startEditor.onReturnKey = [this] { applyStartEdit(); };
        startEditor.onFocusLost = [this] { applyStartEdit(); };
        
        addAndMakeVisible(endEditor);
        endEditor.setJustification(juce::Justification::centred);
        endEditor.onReturnKey = [this] { applyEndEdit(); };
        endEditor.onFocusLost = [this] { applyEndEdit(); };
        
        addAndMakeVisible(lengthEditor);
        lengthEditor.setJustification(juce::Justification::centred);
        lengthEditor.onReturnKey = [this] { applyLengthEdit(); };
        lengthEditor.onFocusLost = [this] { applyLengthEdit(); };
        
        addAndMakeVisible(pitchEditor);
        pitchEditor.setJustification(juce::Justification::centred);
        pitchEditor.onReturnKey = [this] { applyPitchEdit(); };
        pitchEditor.onFocusLost = [this] { applyPitchEdit(); };
        
        addAndMakeVisible(velocityEditor);
        velocityEditor.setJustification(juce::Justification::centred);
        velocityEditor.onReturnKey = [this] { applyVelocityEdit(); };
        velocityEditor.onFocusLost = [this] { applyVelocityEdit(); };
        
        updateDisplay();
    }
    
    void setSelectedNotes(const std::vector<int>& noteIds)
    {
        selectedNoteIds = noteIds;
        updateDisplay();
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(getLocalBounds());
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        int labelWidth = 60;
        int editorWidth = 80;
        int spacing = 10;
        
        auto startArea = area.removeFromLeft(labelWidth + editorWidth + spacing);
        startLabel.setBounds(startArea.removeFromLeft(labelWidth));
        startArea.removeFromLeft(5);
        startEditor.setBounds(startArea.removeFromLeft(editorWidth));
        area.removeFromLeft(spacing);
        
        auto endArea = area.removeFromLeft(labelWidth + editorWidth + spacing);
        endLabel.setBounds(endArea.removeFromLeft(labelWidth));
        endArea.removeFromLeft(5);
        endEditor.setBounds(endArea.removeFromLeft(editorWidth));
        area.removeFromLeft(spacing);
        
        auto lengthArea = area.removeFromLeft(labelWidth + editorWidth + spacing);
        lengthLabel.setBounds(lengthArea.removeFromLeft(labelWidth));
        lengthArea.removeFromLeft(5);
        lengthEditor.setBounds(lengthArea.removeFromLeft(editorWidth));
        area.removeFromLeft(spacing);
        
        auto pitchArea = area.removeFromLeft(labelWidth + editorWidth + spacing);
        pitchLabel.setBounds(pitchArea.removeFromLeft(labelWidth));
        pitchArea.removeFromLeft(5);
        pitchEditor.setBounds(pitchArea.removeFromLeft(editorWidth));
        area.removeFromLeft(spacing);
        
        auto velocityArea = area.removeFromLeft(labelWidth + editorWidth + spacing);
        velocityLabel.setBounds(velocityArea.removeFromLeft(labelWidth));
        velocityArea.removeFromLeft(5);
        velocityEditor.setBounds(velocityArea.removeFromLeft(editorWidth));
    }
    
    // Callbacks
    std::function<void(int noteId, int64_t newStart)> onStartChanged;
    std::function<void(int noteId, int64_t newEnd)> onEndChanged;
    std::function<void(int noteId, int64_t newLength)> onLengthChanged;
    std::function<void(int noteId, int newPitch)> onPitchChanged;
    std::function<void(int noteId, int newVelocity)> onVelocityChanged;
    
private:
    Clip& clip;
    std::vector<int> selectedNoteIds;
    
    juce::Label startLabel, endLabel, lengthLabel, pitchLabel, velocityLabel;
    juce::TextEditor startEditor, endEditor, lengthEditor, pitchEditor, velocityEditor;
    
    void updateDisplay()
    {
        if (selectedNoteIds.empty())
        {
            startEditor.setText("---", false);
            endEditor.setText("---", false);
            lengthEditor.setText("---", false);
            pitchEditor.setText("---", false);
            velocityEditor.setText("---", false);
            
            startEditor.setEnabled(false);
            endEditor.setEnabled(false);
            lengthEditor.setEnabled(false);
            pitchEditor.setEnabled(false);
            velocityEditor.setEnabled(false);
        }
        else if (selectedNoteIds.size() == 1)
        {
            auto* note = clip.findNote(selectedNoteIds[0]);
            if (note)
            {
                startEditor.setText(ticksToString(note->startTick), false);
                endEditor.setText(ticksToString(note->endTick), false);
                lengthEditor.setText(ticksToString(note->getDuration()), false);
                pitchEditor.setText(pitchToString(note->pitch), false);
                velocityEditor.setText(juce::String(note->velocity), false);
                
                startEditor.setEnabled(true);
                endEditor.setEnabled(true);
                lengthEditor.setEnabled(true);
                pitchEditor.setEnabled(true);
                velocityEditor.setEnabled(true);
            }
        }
        else
        {
            // Multiple selection
            startEditor.setText("Multiple", false);
            endEditor.setText("Multiple", false);
            lengthEditor.setText("Multiple", false);
            pitchEditor.setText("Multiple", false);
            velocityEditor.setText("Multiple", false);
            
            startEditor.setEnabled(false);
            endEditor.setEnabled(false);
            lengthEditor.setEnabled(false);
            pitchEditor.setEnabled(false);
            velocityEditor.setEnabled(true); // Can batch-edit velocity
        }
    }
    
    juce::String ticksToString(int64_t ticks) const
    {
        int64_t ppq = PPQ::TICKS_PER_QUARTER;
        int bar = (int)(ticks / (ppq * 4)) + 1;
        int beat = (int)((ticks % (ppq * 4)) / ppq) + 1;
        int tick = (int)(ticks % ppq);
        return juce::String(bar) + "." + juce::String(beat) + "." + juce::String(tick);
    }
    
    int64_t stringToTicks(const juce::String& str) const
    {
        auto parts = juce::StringArray::fromTokens(str, ".", "");
        if (parts.size() != 3) return -1;
        
        int bar = parts[0].getIntValue() - 1;
        int beat = parts[1].getIntValue() - 1;
        int tick = parts[2].getIntValue();
        
        if (bar < 0 || beat < 0 || beat >= 4 || tick < 0) return -1;
        
        int64_t ppq = PPQ::TICKS_PER_QUARTER;
        return (int64_t)bar * ppq * 4 + (int64_t)beat * ppq + (int64_t)tick;
    }
    
    juce::String pitchToString(int pitch) const
    {
        const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        int octave = (pitch / 12) - 1;
        int note = pitch % 12;
        return juce::String(names[note]) + juce::String(octave) + " (" + juce::String(pitch) + ")";
    }
    
    int stringToPitch(const juce::String& str) const
    {
        // Accept MIDI number directly
        int pitch = str.getIntValue();
        if (pitch >= 0 && pitch <= 127) return pitch;
        return -1;
    }
    
    void applyStartEdit()
    {
        if (selectedNoteIds.size() != 1) return;
        int64_t newStart = stringToTicks(startEditor.getText());
        if (newStart >= 0 && onStartChanged)
            onStartChanged(selectedNoteIds[0], newStart);
    }
    
    void applyEndEdit()
    {
        if (selectedNoteIds.size() != 1) return;
        int64_t newEnd = stringToTicks(endEditor.getText());
        if (newEnd >= 0 && onEndChanged)
            onEndChanged(selectedNoteIds[0], newEnd);
    }
    
    void applyLengthEdit()
    {
        if (selectedNoteIds.size() != 1) return;
        int64_t newLength = stringToTicks(lengthEditor.getText());
        if (newLength > 0 && onLengthChanged)
            onLengthChanged(selectedNoteIds[0], newLength);
    }
    
    void applyPitchEdit()
    {
        if (selectedNoteIds.size() != 1) return;
        int newPitch = stringToPitch(pitchEditor.getText());
        if (newPitch >= 0 && onPitchChanged)
            onPitchChanged(selectedNoteIds[0], newPitch);
    }
    
    void applyVelocityEdit()
    {
        int newVelocity = velocityEditor.getText().getIntValue();
        if (newVelocity < 1 || newVelocity > 127) return;
        
        if (onVelocityChanged)
        {
            for (int id : selectedNoteIds)
                onVelocityChanged(id, newVelocity);
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InfoLine)
};

} // namespace pianodaw
