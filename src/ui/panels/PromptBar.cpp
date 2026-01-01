#include "PromptBar.h"

namespace pianodaw {

PromptBar::PromptBar()
{
    promptEditor = std::make_unique<juce::TextEditor>("PromptEditor");
    promptEditor->setMultiLine(false);
    promptEditor->setReturnKeyStartsNewLine(false);
    promptEditor->setReadOnly(false);
    promptEditor->setScrollbarsShown(false);
    promptEditor->setCaretVisible(true);
    promptEditor->setPopupMenuEnabled(true);
    promptEditor->setTextToShowWhenEmpty("Ask AI to edit MIDI (e.g., 'Quantize first 4 bars', 'Add C Major chord')...", juce::Colours::grey);
    promptEditor->addListener(this);
    
    // Modern styling
    promptEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff111111));
    promptEditor->setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff333333));
    promptEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::orange);
    
    addAndMakeVisible(*promptEditor);

    runButton = std::make_unique<juce::TextButton>("Run AI");
    runButton->setTooltip("Execute the natural language command using AI");
    runButton->onClick = [this]() { if (onExecute) onExecute(promptEditor->getText()); };
    addAndMakeVisible(*runButton);

    statusLabel = std::make_unique<juce::Label>("StatusLabel", "AI Ready");
    statusLabel->setFont(juce::Font(12.0f, juce::Font::italic));
    statusLabel->setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(*statusLabel);
}

PromptBar::~PromptBar() {}

void PromptBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    g.setColour(juce::Colour(0xff333333));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());
}

void PromptBar::resized()
{
    auto area = getLocalBounds().reduced(5);
    
    runButton->setBounds(area.removeFromRight(80).reduced(2));
    statusLabel->setBounds(area.removeFromRight(150));
    promptEditor->setBounds(area);
}

void PromptBar::setStatus(const juce::String& status, bool isError)
{
    statusLabel->setText(status, juce::dontSendNotification);
    statusLabel->setColour(juce::Label::textColourId, isError ? juce::Colours::red : juce::Colours::grey);
}

void PromptBar::setBusy(bool busy)
{
    isBusy = busy;
    promptEditor->setEnabled(!busy);
    runButton->setEnabled(!busy);
    if (busy) setStatus("Thinking...");
}

void PromptBar::textEditorReturnKeyPressed(juce::TextEditor& ed)
{
    if (&ed == promptEditor.get() && !isBusy)
    {
        if (onExecute) onExecute(promptEditor->getText());
    }
}

} // namespace pianodaw
