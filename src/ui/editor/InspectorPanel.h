#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/model/Clip.h"
#include "../../core/timeline/PPQ.h"
#include "../../core/ai/MusicTheory.h"
#include <functional>

namespace pianodaw {

/**
 * InspectorPanel - Left-side panel for quantize, length, transpose, scale assistant
 * 
 * Sections:
 * - Quantize (grid, strength, swing)
 * - Note Length (scale length, legato, overlap)
 * - Transpose (semitones, octaves)
 * - Scale Assistant (root, type, on/off)
 * - Chord Editing (chord stamp mode)
 */
class InspectorPanel : public juce::Component
{
public:
    InspectorPanel()
    {
        // Quantize Section
        addAndMakeVisible(quantizeSectionLabel);
        quantizeSectionLabel.setText("QUANTIZE", juce::dontSendNotification);
        quantizeSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        quantizeSectionLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
        
        addAndMakeVisible(quantizeButton);
        quantizeButton.setButtonText("Quantize");
        quantizeButton.onClick = [this] { if (onQuantize) onQuantize(); };
        
        addAndMakeVisible(strengthLabel);
        strengthLabel.setText("Strength:", juce::dontSendNotification);
        
        addAndMakeVisible(strengthSlider);
        strengthSlider.setRange(0.0, 100.0, 1.0);
        strengthSlider.setValue(100.0);
        strengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        strengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        strengthSlider.setTextValueSuffix("%");
        
        addAndMakeVisible(swingLabel);
        swingLabel.setText("Swing:", juce::dontSendNotification);
        
        addAndMakeVisible(swingSlider);
        swingSlider.setRange(0.0, 100.0, 1.0);
        swingSlider.setValue(0.0);
        swingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        swingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        swingSlider.setTextValueSuffix("%");
        
        // Note Length Section
        addAndMakeVisible(lengthSectionLabel);
        lengthSectionLabel.setText("NOTE LENGTH", juce::dontSendNotification);
        lengthSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        lengthSectionLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
        
        addAndMakeVisible(scaleLengthLabel);
        scaleLengthLabel.setText("Scale Length:", juce::dontSendNotification);
        
        addAndMakeVisible(scaleLengthSlider);
        scaleLengthSlider.setRange(-200.0, 200.0, 1.0);
        scaleLengthSlider.setValue(0.0);
        scaleLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        scaleLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        scaleLengthSlider.setTextValueSuffix("%");
        scaleLengthSlider.onValueChange = [this] { if (onScaleLengthChanged) onScaleLengthChanged(scaleLengthSlider.getValue()); };
        
        addAndMakeVisible(legatoButton);
        legatoButton.setButtonText("Legato");
        legatoButton.onClick = [this] { if (onLegato) onLegato(); };
        
        addAndMakeVisible(overlapLabel);
        overlapLabel.setText("Overlap:", juce::dontSendNotification);
        
        addAndMakeVisible(overlapMinusButton);
        overlapMinusButton.setButtonText("-10 ticks");
        overlapMinusButton.onClick = [this] { if (onOverlapChanged) onOverlapChanged(-10); };
        
        addAndMakeVisible(overlapPlusButton);
        overlapPlusButton.setButtonText("+10 ticks");
        overlapPlusButton.onClick = [this] { if (onOverlapChanged) onOverlapChanged(+10); };
        
        // Transpose Section
        addAndMakeVisible(transposeSectionLabel);
        transposeSectionLabel.setText("TRANSPOSE", juce::dontSendNotification);
        transposeSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        transposeSectionLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
        
        addAndMakeVisible(transposeUpButton);
        transposeUpButton.setButtonText("+1 Semitone");
        transposeUpButton.onClick = [this] { if (onTranspose) onTranspose(1); };
        
        addAndMakeVisible(transposeDownButton);
        transposeDownButton.setButtonText("-1 Semitone");
        transposeDownButton.onClick = [this] { if (onTranspose) onTranspose(-1); };
        
        addAndMakeVisible(octaveUpButton);
        octaveUpButton.setButtonText("+1 Octave");
        octaveUpButton.onClick = [this] { if (onTranspose) onTranspose(12); };
        
        addAndMakeVisible(octaveDownButton);
        octaveDownButton.setButtonText("-1 Octave");
        octaveDownButton.onClick = [this] { if (onTranspose) onTranspose(-12); };
        
        // Scale Assistant Section
        addAndMakeVisible(scaleAssistantLabel);
        scaleAssistantLabel.setText("SCALE ASSISTANT", juce::dontSendNotification);
        scaleAssistantLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        scaleAssistantLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
        
        addAndMakeVisible(scaleRootCombo);
        scaleRootCombo.addItemList({"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 1);
        scaleRootCombo.setSelectedItemIndex(0);
        scaleRootCombo.onChange = [this] { 
            if (onScaleChanged) 
                onScaleChanged(scaleRootCombo.getSelectedItemIndex(), getCurrentScaleType()); 
        };
        
        addAndMakeVisible(scaleTypeCombo);
        scaleTypeCombo.addItem("Chromatic", 1);
        scaleTypeCombo.addItem("Major", 2);
        scaleTypeCombo.addItem("Minor", 3);
        scaleTypeCombo.setSelectedItemIndex(0);
        scaleTypeCombo.onChange = [this] { 
            if (onScaleChanged) 
                onScaleChanged(scaleRootCombo.getSelectedItemIndex(), getCurrentScaleType()); 
        };
        
        // Chord Editing Section
        addAndMakeVisible(chordEditingLabel);
        chordEditingLabel.setText("CHORD EDITING", juce::dontSendNotification);
        chordEditingLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        chordEditingLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
        
        addAndMakeVisible(chordModeCombo);
        chordModeCombo.addItem("Single Note", 1);
        chordModeCombo.addItem("Major", 2);
        chordModeCombo.addItem("Minor", 3);
        chordModeCombo.addItem("Diminished", 4);
        chordModeCombo.addItem("Augmented", 5);
        chordModeCombo.setSelectedItemIndex(0);
        chordModeCombo.onChange = [this] { 
            if (onChordModeChanged) 
                onChordModeChanged((MusicTheory::ChordType)(chordModeCombo.getSelectedId() - 1)); 
        };
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        // Section separators
        g.setColour(juce::Colour(0xff3a3a3a));
        
        int y = quantizeSectionLabel.getBottom() + 90;
        g.drawHorizontalLine(y, 0, (float)getWidth());
        
        y = lengthSectionLabel.getBottom() + 130;
        g.drawHorizontalLine(y, 0, (float)getWidth());
        
        y = transposeSectionLabel.getBottom() + 90;
        g.drawHorizontalLine(y, 0, (float)getWidth());
        
        y = scaleAssistantLabel.getBottom() + 60;
        g.drawHorizontalLine(y, 0, (float)getWidth());
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        int labelHeight = 20;
        int buttonHeight = 25;
        int sliderHeight = 25;
        int spacing = 5;
        
        // Quantize Section
        quantizeSectionLabel.setBounds(area.removeFromTop(labelHeight));
        area.removeFromTop(spacing);
        quantizeButton.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing);
        strengthLabel.setBounds(area.removeFromTop(labelHeight));
        strengthSlider.setBounds(area.removeFromTop(sliderHeight));
        area.removeFromTop(spacing);
        swingLabel.setBounds(area.removeFromTop(labelHeight));
        swingSlider.setBounds(area.removeFromTop(sliderHeight));
        area.removeFromTop(spacing * 3);
        
        // Note Length Section
        lengthSectionLabel.setBounds(area.removeFromTop(labelHeight));
        area.removeFromTop(spacing);
        scaleLengthLabel.setBounds(area.removeFromTop(labelHeight));
        scaleLengthSlider.setBounds(area.removeFromTop(sliderHeight));
        area.removeFromTop(spacing);
        legatoButton.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing);
        overlapLabel.setBounds(area.removeFromTop(labelHeight));
        auto overlapRow = area.removeFromTop(buttonHeight);
        overlapMinusButton.setBounds(overlapRow.removeFromLeft(overlapRow.getWidth() / 2 - 2));
        overlapRow.removeFromLeft(4);
        overlapPlusButton.setBounds(overlapRow);
        area.removeFromTop(spacing * 3);
        
        // Transpose Section
        transposeSectionLabel.setBounds(area.removeFromTop(labelHeight));
        area.removeFromTop(spacing);
        transposeUpButton.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing);
        transposeDownButton.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing);
        octaveUpButton.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing);
        octaveDownButton.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing * 3);
        
        // Scale Assistant Section
        scaleAssistantLabel.setBounds(area.removeFromTop(labelHeight));
        area.removeFromTop(spacing);
        scaleRootCombo.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing);
        scaleTypeCombo.setBounds(area.removeFromTop(buttonHeight));
        area.removeFromTop(spacing * 3);
        
        // Chord Editing Section
        chordEditingLabel.setBounds(area.removeFromTop(labelHeight));
        area.removeFromTop(spacing);
        chordModeCombo.setBounds(area.removeFromTop(buttonHeight));
    }
    
    MusicTheory::ScaleType getCurrentScaleType() const
    {
        return (MusicTheory::ScaleType)(scaleTypeCombo.getSelectedItemIndex());
    }
    
    // Callbacks
    std::function<void()> onQuantize;
    std::function<void(double)> onScaleLengthChanged;
    std::function<void()> onLegato;
    std::function<void(int)> onOverlapChanged;
    std::function<void(int)> onTranspose;
    std::function<void(int, MusicTheory::ScaleType)> onScaleChanged;
    std::function<void(MusicTheory::ChordType)> onChordModeChanged;
    
private:
    // Quantize
    juce::Label quantizeSectionLabel;
    juce::TextButton quantizeButton;
    juce::Label strengthLabel;
    juce::Slider strengthSlider;
    juce::Label swingLabel;
    juce::Slider swingSlider;
    
    // Note Length
    juce::Label lengthSectionLabel;
    juce::Label scaleLengthLabel;
    juce::Slider scaleLengthSlider;
    juce::TextButton legatoButton;
    juce::Label overlapLabel;
    juce::TextButton overlapMinusButton, overlapPlusButton;
    
    // Transpose
    juce::Label transposeSectionLabel;
    juce::TextButton transposeUpButton, transposeDownButton;
    juce::TextButton octaveUpButton, octaveDownButton;
    
    // Scale Assistant
    juce::Label scaleAssistantLabel;
    juce::ComboBox scaleRootCombo, scaleTypeCombo;
    
    // Chord Editing
    juce::Label chordEditingLabel;
    juce::ComboBox chordModeCombo;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorPanel)
};

} // namespace pianodaw
