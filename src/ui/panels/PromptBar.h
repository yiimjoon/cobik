#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace pianodaw {

/**
 * PromptBar - UI component for entering natural language AI commands
 */
class PromptBar : public juce::Component,
                  public juce::TextEditor::Listener
{
public:
    PromptBar();
    ~PromptBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    /** Triggered when the user presses Enter or clicks 'Run' */
    std::function<void(const juce::String&)> onExecute;

    void setStatus(const juce::String& status, bool isError = false);
    void setBusy(bool busy);

    void textEditorReturnKeyPressed(juce::TextEditor& ed) override;

private:
    std::unique_ptr<juce::TextEditor> promptEditor;
    std::unique_ptr<juce::TextButton> runButton;
    std::unique_ptr<juce::Label> statusLabel;
    
    bool isBusy = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptBar)
};

} // namespace pianodaw
