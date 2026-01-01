#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace pianodaw {

/**
 * A window that hosts a plugin's GUI editor.
 */
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow(juce::AudioProcessor& processor)
        : DocumentWindow(processor.getName(),
                         juce::Colours::darkgrey,
                         DocumentWindow::allButtons)
    {
        if (auto* editor = processor.createEditorIfNeeded())
        {
            setContentNonOwned(editor, true);
        }
        
        setResizable(true, false);
        setUsingNativeTitleBar(true);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        delete this;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

} // namespace pianodaw
