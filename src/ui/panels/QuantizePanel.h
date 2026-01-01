#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/edit/UndoStack.h"
#include "../../core/model/Clip.h"
#include "../../core/quantize/QuantizeEngine.h"

namespace pianodaw {

/**
 * QuantizePanel - UI for configuring and applying quantization
 */
class QuantizePanel : public juce::Component,
                      public juce::Button::Listener,
                      public juce::Slider::Listener
{
public:
    QuantizePanel(Clip& clip, UndoStack& undoStack);
    ~QuantizePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;

    // Selection management (should be updated by PianoRollView)
    void setSelectedNoteIds(const std::vector<int>& ids) { selectedNoteIds = ids; }

private:
    Clip& clip;
    UndoStack& undoStack;
    std::vector<int> selectedNoteIds;

    // UI Elements
    std::unique_ptr<juce::ComboBox> gridCombo;
    std::unique_ptr<juce::Slider> strengthSlider;
    std::unique_ptr<juce::Slider> swingSlider;
    std::unique_ptr<juce::ToggleButton> smartChordButton;
    std::unique_ptr<juce::TextButton> applyButton;

    QuantizeParams params;
    
    void updateParamsFromUI();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuantizePanel)
};

} // namespace pianodaw
