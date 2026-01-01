#include "TrackListPanel.h"

namespace pianodaw {

TrackListPanel::TrackListPanel(Project& proj)
    : project(proj)
{
    // Create add/remove track buttons
    addTrackButton = std::make_unique<juce::TextButton>("Add Track");
    addTrackButton->addListener(this);
    addChildComponent(addTrackButton.get());
    
    removeTrackButton = std::make_unique<juce::TextButton>("Remove Track");
    removeTrackButton->addListener(this);
    addChildComponent(removeTrackButton.get());
    
    updateTrackRows();
}

void TrackListPanel::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff252525));
    
    // Draw header
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRect(0, 0, getWidth(), headerHeight);
    
    g.setColour(juce::Colour(0xffcccccc));
    g.drawText("TRACKS", 10, 0, getWidth() - 20, headerHeight,
               juce::Justification::centredLeft);
    
    // Draw track rows
    int numTracks = project.getNumTracks();
    for (int i = 0; i < numTracks; ++i) {
        Track* track = project.getTrack(i);
        if (!track) continue;
        
        int y = headerHeight + i * trackHeight;
        juce::Rectangle<int> bounds(0, y, getWidth(), trackHeight);
        drawTrackRow(g, i, bounds);
    }
}

void TrackListPanel::resized()
{
    // Update track row button positions
    updateTrackRows();
    
    // Position add/remove track buttons at bottom
    int buttonHeight = 30;
    int margin = 5;
    int y = getHeight() - buttonHeight - margin;
    int buttonWidth = (getWidth() - 3 * margin) / 2;
    
    addTrackButton->setBounds(margin, y, buttonWidth, buttonHeight);
    removeTrackButton->setBounds(margin * 2 + buttonWidth, y, buttonWidth, buttonHeight);
    
    addTrackButton->setVisible(true);
    removeTrackButton->setVisible(true);
}

void TrackListPanel::buttonClicked(juce::Button* button)
{
    if (button == addTrackButton.get()) {
        // Add new MIDI track
        Track* track = project.addTrack("Track " + juce::String(project.getNumTracks() + 1), Track::Type::MIDI);
        track->setColour(juce::Colour::fromHSV(juce::Random::getSystemRandom().nextFloat(), 0.7f, 0.8f, 1.0f));
        
        updateTrackRows();
        repaint();
        
        if (onTracksChanged)
            onTracksChanged();
    }
    else if (button == removeTrackButton.get()) {
        // Remove selected track
        if (selectedTrackIndex >= 0 && selectedTrackIndex < project.getNumTracks()) {
            project.removeTrack(selectedTrackIndex);
            selectedTrackIndex = -1;
            
            updateTrackRows();
            repaint();
            
            if (onTracksChanged)
                onTracksChanged();
        }
    }
    else {
        // Check solo/mute buttons
        for (int i = 0; i < trackRows.size(); ++i) {
            auto* row = trackRows[i];
            Track* track = project.getTrack(row->trackIndex);
            if (!track) continue;
            
            if (button == row->soloButton.get()) {
                track->setSolo(!track->isSolo());
                repaint();
            }
            else if (button == row->muteButton.get()) {
                track->setMute(!track->isMuted());
                repaint();
            }
        }
    }
}

void TrackListPanel::setSelectedTrack(int trackIndex)
{
    selectedTrackIndex = trackIndex;
    repaint();
}

void TrackListPanel::drawTrackRow(juce::Graphics& g, int trackIndex, const juce::Rectangle<int>& bounds)
{
    Track* track = project.getTrack(trackIndex);
    if (!track) return;
    
    // Background (highlight if selected)
    if (trackIndex == selectedTrackIndex) {
        g.setColour(juce::Colour(0xff3a3a3a));
    } else {
        g.setColour(juce::Colour(0xff2a2a2a));
    }
    g.fillRect(bounds);
    
    // Track color indicator (left edge)
    g.setColour(track->getColour());
    g.fillRect(bounds.getX(), bounds.getY() + 4, 6, bounds.getHeight() - 8);
    
    // Track name
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText(track->getName(), 
               bounds.getX() + 12, bounds.getY() + 4, 
               bounds.getWidth() - 24, 24,
               juce::Justification::centredLeft);
    
    // Track type
    g.setColour(juce::Colour(0xff888888));
    g.setFont(11.0f);
    juce::String typeStr = (track->getType() == Track::Type::MIDI) ? "MIDI" : "AUDIO";
    g.drawText(typeStr,
               bounds.getX() + 12, bounds.getY() + 26,
               bounds.getWidth() - 24, 16,
               juce::Justification::centredLeft);
    
    // Separator
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawLine(bounds.getX(), bounds.getBottom(), 
               bounds.getRight(), bounds.getBottom(), 1.0f);
}

void TrackListPanel::mouseDown(const juce::MouseEvent& event)
{
    int trackIndex = getTrackIndexAt(event.y);
    if (trackIndex >= 0) {
        setSelectedTrack(trackIndex);
        
        if (onTrackSelected)
            onTrackSelected(trackIndex);
    }
}

void TrackListPanel::updateTrackRows()
{
    trackRows.clear();
    
    int numTracks = project.getNumTracks();
    for (int i = 0; i < numTracks; ++i) {
        auto row = new TrackRow();
        row->trackIndex = i;
        
        int y = headerHeight + i * trackHeight;
        row->bounds = juce::Rectangle<int>(0, y, getWidth(), trackHeight);
        
        // Create solo/mute buttons
        row->soloButton = std::make_unique<juce::TextButton>("S");
        row->soloButton->setBounds(getWidth() - 70, y + 48, 30, 24);
        row->soloButton->addListener(this);
        addAndMakeVisible(row->soloButton.get());
        
        row->muteButton = std::make_unique<juce::TextButton>("M");
        row->muteButton->setBounds(getWidth() - 35, y + 48, 30, 24);
        row->muteButton->addListener(this);
        addAndMakeVisible(row->muteButton.get());
        
        // Update button colors based on track state
        Track* track = project.getTrack(i);
        if (track) {
            row->soloButton->setColour(juce::TextButton::buttonColourId, 
                track->isSolo() ? juce::Colours::yellow.darker(0.3f) : juce::Colour(0xff404040));
            row->muteButton->setColour(juce::TextButton::buttonColourId,
                track->isMuted() ? juce::Colours::red.darker(0.3f) : juce::Colour(0xff404040));
        }
        
        trackRows.add(row);
    }
}

int TrackListPanel::getTrackIndexAt(int y)
{
    if (y < headerHeight) return -1;
    
    int trackIndex = (y - headerHeight) / trackHeight;
    if (trackIndex >= 0 && trackIndex < project.getNumTracks())
        return trackIndex;
    
    return -1;
}

} // namespace pianodaw
