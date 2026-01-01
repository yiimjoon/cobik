#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/timeline/PPQ.h"
#include <functional>

namespace pianodaw {

/** Editor Tool Types */
enum class EditorTool
{
    Select,
    Draw,
    Erase,
    Split,
    Glue,
    Mute,
    Line,
    Trim
};

/**
 * PianoRollToolbar - Tool selection, snap, grid size, zoom controls
 */
class PianoRollToolbar : public juce::Component
{
public:
    PianoRollToolbar()
    {
        // Tool Buttons
        addAndMakeVisible(selectButton);
        selectButton.setButtonText("Select");
        selectButton.setRadioGroupId(1);
        selectButton.setToggleState(true, juce::dontSendNotification);
        selectButton.onClick = [this] { if (onToolChanged) onToolChanged(EditorTool::Select); };
        
        addAndMakeVisible(drawButton);
        drawButton.setButtonText("Draw");
        drawButton.setRadioGroupId(1);
        drawButton.onClick = [this] { if (onToolChanged) onToolChanged(EditorTool::Draw); };
        
        addAndMakeVisible(eraseButton);
        eraseButton.setButtonText("Erase");
        eraseButton.setRadioGroupId(1);
        eraseButton.onClick = [this] { if (onToolChanged) onToolChanged(EditorTool::Erase); };
        
        addAndMakeVisible(splitButton);
        splitButton.setButtonText("Split");
        splitButton.setRadioGroupId(1);
        splitButton.onClick = [this] { if (onToolChanged) onToolChanged(EditorTool::Split); };
        
        addAndMakeVisible(lineButton);
        lineButton.setButtonText("Line");
        lineButton.setRadioGroupId(1);
        lineButton.onClick = [this] { if (onToolChanged) onToolChanged(EditorTool::Line); };
        
        // Snap Toggle
        addAndMakeVisible(snapButton);
        snapButton.setButtonText("Snap");
        snapButton.setToggleState(true, juce::dontSendNotification);
        snapButton.setClickingTogglesState(true);
        snapButton.onClick = [this] { 
            if (onSnapChanged) onSnapChanged(snapButton.getToggleState()); 
        };
        
        // Grid Size Combo
        addAndMakeVisible(gridCombo);
        gridCombo.addItem("1/1", 1);
        gridCombo.addItem("1/2", 2);
        gridCombo.addItem("1/4", 3);
        gridCombo.addItem("1/8", 4);
        gridCombo.addItem("1/16", 5);
        gridCombo.addItem("1/32", 6);
        gridCombo.setSelectedId(5, juce::dontSendNotification); // Default 1/16
        gridCombo.onChange = [this] { 
            if (onGridSizeChanged) 
                onGridSizeChanged((GridSize)(gridCombo.getSelectedId() - 1)); 
        };
        
        // Zoom Sliders
        addAndMakeVisible(hZoomLabel);
        hZoomLabel.setText("H-Zoom:", juce::dontSendNotification);
        hZoomLabel.setJustificationType(juce::Justification::centredRight);
        
        addAndMakeVisible(hZoomSlider);
        hZoomSlider.setRange(0.01, 1.0, 0.01);
        hZoomSlider.setValue(0.1, juce::dontSendNotification);
        hZoomSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        hZoomSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        hZoomSlider.onValueChange = [this] { 
            if (onHorizontalZoomChanged) 
                onHorizontalZoomChanged(hZoomSlider.getValue()); 
        };
        
        addAndMakeVisible(vZoomLabel);
        vZoomLabel.setText("V-Zoom:", juce::dontSendNotification);
        vZoomLabel.setJustificationType(juce::Justification::centredRight);
        
        addAndMakeVisible(vZoomSlider);
        vZoomSlider.setRange(8.0, 32.0, 1.0);
        vZoomSlider.setValue(16.0, juce::dontSendNotification);
        vZoomSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        vZoomSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        vZoomSlider.onValueChange = [this] { 
            if (onVerticalZoomChanged) 
                onVerticalZoomChanged(vZoomSlider.getValue()); 
        };
    }
    
    void setTool(EditorTool tool)
    {
        switch (tool)
        {
            case EditorTool::Select: selectButton.setToggleState(true, juce::dontSendNotification); break;
            case EditorTool::Draw:   drawButton.setToggleState(true, juce::dontSendNotification); break;
            case EditorTool::Erase:  eraseButton.setToggleState(true, juce::dontSendNotification); break;
            case EditorTool::Split:  splitButton.setToggleState(true, juce::dontSendNotification); break;
            case EditorTool::Line:   lineButton.setToggleState(true, juce::dontSendNotification); break;
            default: break;
        }
    }
    
    EditorTool getCurrentTool() const
    {
        if (selectButton.getToggleState()) return EditorTool::Select;
        if (drawButton.getToggleState())   return EditorTool::Draw;
        if (eraseButton.getToggleState())  return EditorTool::Erase;
        if (splitButton.getToggleState())  return EditorTool::Split;
        if (lineButton.getToggleState())   return EditorTool::Line;
        return EditorTool::Select;
    }
    
    bool isSnapEnabled() const { return snapButton.getToggleState(); }
    GridSize getGridSize() const { return (GridSize)(gridCombo.getSelectedId() - 1); }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(getLocalBounds());
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        int buttonWidth = 60;
        int spacing = 5;
        
        // Tool Buttons
        selectButton.setBounds(area.removeFromLeft(buttonWidth));
        area.removeFromLeft(spacing);
        drawButton.setBounds(area.removeFromLeft(buttonWidth));
        area.removeFromLeft(spacing);
        eraseButton.setBounds(area.removeFromLeft(buttonWidth));
        area.removeFromLeft(spacing);
        splitButton.setBounds(area.removeFromLeft(buttonWidth));
        area.removeFromLeft(spacing);
        lineButton.setBounds(area.removeFromLeft(buttonWidth));
        area.removeFromLeft(spacing * 3);
        
        // Snap & Grid
        snapButton.setBounds(area.removeFromLeft(buttonWidth));
        area.removeFromLeft(spacing);
        gridCombo.setBounds(area.removeFromLeft(80));
        area.removeFromLeft(spacing * 3);
        
        // Zoom
        hZoomLabel.setBounds(area.removeFromLeft(60));
        hZoomSlider.setBounds(area.removeFromLeft(100));
        area.removeFromLeft(spacing * 2);
        
        vZoomLabel.setBounds(area.removeFromLeft(60));
        vZoomSlider.setBounds(area.removeFromLeft(100));
    }
    
    // Callbacks
    std::function<void(EditorTool)> onToolChanged;
    std::function<void(bool)> onSnapChanged;
    std::function<void(GridSize)> onGridSizeChanged;
    std::function<void(double)> onHorizontalZoomChanged;
    std::function<void(double)> onVerticalZoomChanged;
    
private:
    juce::TextButton selectButton, drawButton, eraseButton, splitButton, lineButton;
    juce::ToggleButton snapButton;
    juce::ComboBox gridCombo;
    juce::Label hZoomLabel, vZoomLabel;
    juce::Slider hZoomSlider, vZoomSlider;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollToolbar)
};

} // namespace pianodaw
