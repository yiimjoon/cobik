#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../core/ai/MusicTheory.h"

namespace pianodaw {

/**
 * TheoryToolbar - UI for selecting Scale and Chord Stamping mode
 */
class TheoryToolbar : public juce::Component
{
public:
    TheoryToolbar()
    {
        addAndMakeVisible(scaleRootCombo);
        scaleRootCombo.addItemList({"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 1);
        scaleRootCombo.setSelectedItemIndex(0);
        scaleRootCombo.onChange = [this] { if (onScaleChanged) onScaleChanged(getScaleRoot(), getScaleType()); };

        addAndMakeVisible(scaleTypeCombo);
        scaleTypeCombo.addItem("Chromatic", 1);
        scaleTypeCombo.addItem("Major", 2);
        scaleTypeCombo.addItem("Minor", 3);
        scaleTypeCombo.addItem("Dorian", 4);
        scaleTypeCombo.addItem("Phrygian", 5);
        scaleTypeCombo.addItem("Lydian", 6);
        scaleTypeCombo.addItem("Mixolydian", 7);
        scaleTypeCombo.addItem("Locrian", 8);
        scaleTypeCombo.addItem("Pentatonic Major", 9);
        scaleTypeCombo.addItem("Pentatonic Minor", 10);
        scaleTypeCombo.setSelectedItemIndex(0);
        scaleTypeCombo.onChange = [this] { if (onScaleChanged) onScaleChanged(getScaleRoot(), getScaleType()); };

        addAndMakeVisible(chordStampCombo);
        chordStampCombo.addItem("No Chord (Single)", 1);
        chordStampCombo.addItem("Major", 2);
        chordStampCombo.addItem("Minor", 3);
        chordStampCombo.addItem("Diminished", 4);
        chordStampCombo.addItem("Augmented", 5);
        chordStampCombo.addItem("Sus2", 6);
        chordStampCombo.addItem("Sus4", 7);
        chordStampCombo.addItem("Maj7", 8);
        chordStampCombo.addItem("Min7", 9);
        chordStampCombo.addItem("Dom7", 10);
        chordStampCombo.setSelectedItemIndex(0);
        chordStampCombo.onChange = [this] { if (onChordChanged) onChordChanged(getChordType()); };

        for (auto* c : {&scaleRootCombo, &scaleTypeCombo, &chordStampCombo})
            c->setJustificationType(juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        auto w = area.getWidth() / 3;
        scaleRootCombo.setBounds(area.removeFromLeft(w / 3));
        scaleTypeCombo.setBounds(area.removeFromLeft(w * 2 / 3));
        area.removeFromLeft(10); // Spacer
        chordStampCombo.setBounds(area.removeFromLeft(w));
    }

    std::function<void(int, MusicTheory::ScaleType)> onScaleChanged;
    std::function<void(MusicTheory::ChordType)> onChordChanged;

    int getScaleRoot() const { return scaleRootCombo.getSelectedItemIndex(); }
    MusicTheory::ScaleType getScaleType() const { return (MusicTheory::ScaleType)(scaleTypeCombo.getSelectedItemIndex()); }
    MusicTheory::ChordType getChordType() const { return (MusicTheory::ChordType)(chordStampCombo.getSelectedItemIndex()); }

private:
    juce::ComboBox scaleRootCombo;
    juce::ComboBox scaleTypeCombo;
    juce::ComboBox chordStampCombo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TheoryToolbar)
};

} // namespace pianodaw
