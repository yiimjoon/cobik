#include "QuantizePanel.h"
#include "../../core/edit/EditCommands.h"

namespace pianodaw {

QuantizePanel::QuantizePanel(Clip& clip_, UndoStack& undoStack_)
    : clip(clip_), undoStack(undoStack_)
{
    gridCombo = std::make_unique<juce::ComboBox>("GridCombo");
    gridCombo->addItem("1/4", 1);
    gridCombo->addItem("1/8", 2);
    gridCombo->addItem("1/16", 3);
    gridCombo->addItem("1/32", 4);
    gridCombo->setSelectedId(3); // 1/16 default
    addAndMakeVisible(*gridCombo);

    strengthSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    strengthSlider->setRange(0.0, 1.0, 0.01);
    strengthSlider->setValue(1.0);
    strengthSlider->addListener(this);
    addAndMakeVisible(*strengthSlider);

    swingSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    swingSlider->setRange(0.5, 0.75, 0.01); // Standard swing ranges
    swingSlider->setValue(0.5);
    swingSlider->addListener(this);
    addAndMakeVisible(*swingSlider);

    smartChordButton = std::make_unique<juce::ToggleButton>("Smart Chord");
    smartChordButton->setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(*smartChordButton);

    applyButton = std::make_unique<juce::TextButton>("Apply Quantize");
    applyButton->addListener(this);
    addAndMakeVisible(*applyButton);
}

QuantizePanel::~QuantizePanel() {}

void QuantizePanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
    
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    auto area = getLocalBounds();
    g.drawText("Smart Quantize", area.removeFromTop(40).reduced(10, 0), juce::Justification::centredLeft);
    
    g.setFont(12.0f);
    g.setColour(juce::Colours::grey);
    
    // Labels
    auto labelArea = area.removeFromTop(20).reduced(10, 0);
    g.drawText("Grid", labelArea, juce::Justification::bottomLeft);
    
    area.removeFromTop(35); // Grid combo space
    
    labelArea = area.removeFromTop(20).reduced(10, 0);
    g.drawText("Strength", labelArea, juce::Justification::bottomLeft);
    
    area.removeFromTop(35); // Strength space
    
    labelArea = area.removeFromTop(20).reduced(10, 0);
    g.drawText("Swing", labelArea, juce::Justification::bottomLeft);
}

void QuantizePanel::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(40); // Title

    area.removeFromTop(20); // Grid label
    gridCombo->setBounds(area.removeFromTop(30).reduced(10, 0));
    area.removeFromTop(5);

    area.removeFromTop(20); // Strength label
    strengthSlider->setBounds(area.removeFromTop(30).reduced(10, 0));
    area.removeFromTop(5);

    area.removeFromTop(20); // Swing label
    swingSlider->setBounds(area.removeFromTop(30).reduced(10, 0));
    area.removeFromTop(10);

    smartChordButton->setBounds(area.removeFromTop(30).reduced(10, 0));
    area.removeFromTop(20);

    applyButton->setBounds(area.removeFromTop(40).reduced(20, 0));
}

void QuantizePanel::buttonClicked(juce::Button* button)
{
    if (button == applyButton.get())
    {
        updateParamsFromUI();
        
        // Use all notes if no selection is set (for MVP simplicity, or we can get selection from view)
        // In a real DAW, you'd only quantize the selected ones.
        std::vector<int> ids;
        if (selectedNoteIds.empty())
        {
            juce::ScopedLock sl(clip.getLock());
            for (const auto& n : clip.getNotes()) ids.push_back(n.id);
        }
        else
        {
            ids = selectedNoteIds;
        }

        if (!ids.empty())
        {
            undoStack.execute(std::make_unique<QuantizeCommand>(clip, ids, params));
        }
    }
}

void QuantizePanel::sliderValueChanged(juce::Slider*) {}

void QuantizePanel::updateParamsFromUI()
{
    int id = gridCombo->getSelectedId();
    if (id == 1) params.gridTicks = 960;      // 1/4
    else if (id == 2) params.gridTicks = 480; // 1/8
    else if (id == 3) params.gridTicks = 240; // 1/16
    else if (id == 4) params.gridTicks = 120; // 1/32

    params.strength = (float)strengthSlider->getValue();
    params.swing = (float)swingSlider->getValue();
    params.smartChord = smartChordButton->getToggleState();
}

} // namespace pianodaw
