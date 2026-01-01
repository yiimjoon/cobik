#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/audio/AudioEngine.h"

namespace pianodaw {

class VstBrowserPanel : public juce::Component,
                        public juce::Button::Listener,
                        public juce::ListBoxModel
{
public:
    VstBrowserPanel(AudioEngine& engine);
    ~VstBrowserPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Button::Listener
    void buttonClicked(juce::Button* button) override;

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

private:
    AudioEngine& audioEngine;
    
    std::unique_ptr<juce::TextButton> scanButton;
    std::unique_ptr<juce::ListBox> pluginListBox;

    juce::Array<juce::PluginDescription> plugins;
    void updatePluginList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstBrowserPanel)
};

} // namespace pianodaw
