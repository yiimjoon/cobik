#include "VstBrowserPanel.h"
#include "PluginEditorWindow.h"

namespace pianodaw {

VstBrowserPanel::VstBrowserPanel(AudioEngine& engine) : audioEngine(engine)
{
    scanButton = std::make_unique<juce::TextButton>("Scan VST3");
    scanButton->addListener(this);
    addAndMakeVisible(*scanButton);

    pluginListBox = std::make_unique<juce::ListBox>("PluginList", this);
    pluginListBox->setRowHeight(30);
    addAndMakeVisible(*pluginListBox);

    updatePluginList();
}

VstBrowserPanel::~VstBrowserPanel()
{
}

void VstBrowserPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
    
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("VSTi Browser", getLocalBounds().removeFromTop(40).reduced(10, 0), juce::Justification::centredLeft);
}

void VstBrowserPanel::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(40); // Title area

    auto buttonArea = area.removeFromTop(40).reduced(10, 5);
    scanButton->setBounds(buttonArea);

    pluginListBox->setBounds(area.reduced(10));
}

void VstBrowserPanel::buttonClicked(juce::Button* button)
{
    if (button == scanButton.get())
    {
        audioEngine.scanPlugins();
        updatePluginList();
    }
}

int VstBrowserPanel::getNumRows()
{
    return plugins.size();
}

void VstBrowserPanel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::lightblue.withAlpha(0.3f));

    if (rowNumber < plugins.size())
    {
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(plugins[rowNumber].name + " (" + plugins[rowNumber].descriptiveName + ")",
                   5, 0, width - 10, height, juce::Justification::centredLeft);
    }
}

void VstBrowserPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row < plugins.size())
    {
        if (audioEngine.loadPlugin(plugins[row]))
        {
            if (auto* proc = audioEngine.getCurrentPlugin())
            {
                if (proc->hasEditor())
                {
                    // Open the editor window
                    // Re-use current implementation pattern for simplicity
                    new PluginEditorWindow(*proc);
                }
            }
        }
        else
        {
             DBG("VstBrowserPanel: Failed to load plugin " + plugins[row].name);
        }
    }
}

void VstBrowserPanel::updatePluginList()
{
    plugins.clear();
    auto& knownList = audioEngine.getKnownPluginList();
    for (auto& desc : knownList.getTypes())
    {
        plugins.add(desc);
    }
    pluginListBox->updateContent();
}

} // namespace pianodaw
