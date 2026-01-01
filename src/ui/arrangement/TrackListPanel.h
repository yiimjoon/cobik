#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/model/Project.h"

namespace pianodaw {

/**
 * TrackListPanel - Left panel showing track names and controls (Cubase-style)
 * 
 * Features:
 * - Track names (editable)
 * - Solo/Mute buttons per track
 * - Track color indicator
 * - Add/Remove track buttons at bottom
 * - Drag to reorder tracks (future)
 */
class TrackListPanel : public juce::Component,
                       public juce::Button::Listener
{
public:
    TrackListPanel(Project& project);
    ~TrackListPanel() override = default;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener override
    void buttonClicked(juce::Button* button) override;

    // Selection
    void setSelectedTrack(int trackIndex);
    int getSelectedTrack() const { return selectedTrackIndex; }
    
    // Callbacks
    std::function<void(int trackIndex)> onTrackSelected;
    std::function<void()> onTracksChanged;
    std::function<void(int trackIndex, bool armed)> onRecordArmChanged;

private:
    struct TrackRow {
        int trackIndex = -1;
        juce::Rectangle<int> bounds;
        std::unique_ptr<juce::TextButton> recordArmButton;
        std::unique_ptr<juce::TextButton> soloButton;
        std::unique_ptr<juce::TextButton> muteButton;
    };

    void drawTrackRow(juce::Graphics& g, int trackIndex, const juce::Rectangle<int>& bounds);
    void mouseDown(const juce::MouseEvent& event) override;
    void updateTrackRows();
    int getTrackIndexAt(int y);

    // Reference to project
    Project& project;

    // View state
    int trackHeight = 80;
    int headerHeight = 30;
    int selectedTrackIndex = -1;

    // Track rows with buttons
    juce::OwnedArray<TrackRow> trackRows;
    
    // Add/Remove track buttons
    std::unique_ptr<juce::TextButton> addTrackButton;
    std::unique_ptr<juce::TextButton> removeTrackButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackListPanel)
};

} // namespace pianodaw
