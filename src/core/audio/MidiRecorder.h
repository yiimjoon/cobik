#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "../model/Clip.h"
#include "../model/Note.h"
#include "../timeline/PPQ.h"
#include <map>

namespace pianodaw {

/**
 * MidiRecorder - Captures MIDI input and records to a Clip
 * 
 * Features:
 * - Records MIDI notes with timing
 * - Handles Note On/Off events
 * - Quantizes input (optional)
 * - Thread-safe recording
 */
class MidiRecorder
{
public:
    MidiRecorder();
    ~MidiRecorder();
    
    /**
     * Start recording to a specific clip
     * @param clip The clip to record into
     * @param startTick The tick position to start recording from
     */
    void startRecording(Clip* clip, int64_t startTick);
    
    /**
     * Stop recording
     */
    void stopRecording();
    
    /**
     * Check if currently recording
     */
    bool isRecording() const { return recording; }
    
    /**
     * Process incoming MIDI messages
     * Call this from audio thread with incoming MIDI
     * @param midiMessages The MIDI buffer to process
     * @param currentTick The current playback position in ticks
     */
    void processMidiInput(const juce::MidiBuffer& midiMessages, int64_t currentTick);
    
    /**
     * Enable/disable input quantization
     * @param enabled True to quantize recorded notes
     * @param gridTicks Quantize grid size in ticks (e.g., PPQ::TICKS_PER_QUARTER / 4 for 16th notes)
     */
    void setQuantizeInput(bool enabled, int64_t gridTicks = PPQ::TICKS_PER_QUARTER / 4);
    
    /**
     * Enable/disable replace mode (vs overdub)
     * Replace mode clears existing notes in the recorded region
     */
    void setReplaceMode(bool replace) { replaceMode = replace; }
    
private:
    // Recording state
    bool recording = false;
    Clip* targetClip = nullptr;
    int64_t recordStartTick = 0;
    
    // Quantization
    bool quantizeInput = false;
    int64_t quantizeGridTicks = PPQ::TICKS_PER_QUARTER / 4;
    
    // Replace vs overdub
    bool replaceMode = false;
    
    // Track active notes (Note On without Note Off yet)
    struct ActiveNote
    {
        int pitch;
        int velocity;  // Changed from float to int (0-127)
        int64_t startTick;
    };
    std::map<int, ActiveNote> activeNotes;  // key = MIDI note number
    
    // Thread safety
    juce::CriticalSection recordLock;
    
    // Helper methods
    int64_t quantizeTick(int64_t tick) const;
    void handleNoteOn(int noteNumber, int velocity, int64_t tick);  // Changed from float to int
    void handleNoteOff(int noteNumber, int64_t tick);
    void clearRegion(int64_t startTick, int64_t endTick);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRecorder)
};

} // namespace pianodaw
