#pragma once

#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <functional>

namespace pianodaw {

/**
 * Transport - Manages project playback state and timing
 */
class Transport : private juce::Timer
{
public:
    Transport();
    ~Transport() override;

    // Playback control
    void start();
    void stop();
    void togglePlay();
    void setPlaying(bool play);
    bool isPlaying() const { return playing; }

    // Timing
    void setPosition(int64_t ticks);
    int64_t getPosition() const { return currentTick; }
    
    void setTempo(double bpm);
    double getTempo() const { return currentBPM; }
    
    void setLooping(bool loop) { looping = loop; }
    bool isLooping() const { return looping; }
    void setLoopRange(int64_t start, int64_t end) { loopStart = start; loopEnd = end; }

    // Callbacks
    std::function<void()> onStatusChanged;
    std::function<void(int64_t)> onPositionChanged;

private:
    void timerCallback() override;

    bool playing = false;
    bool looping = false;
    int64_t currentTick = 0;
    double currentBPM = 120.0;
    
    int64_t loopStart = 0;
    int64_t loopEnd = 960 * 16; // 4 bars default
    
    juce::uint32 lastTimeMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Transport)
};

} // namespace pianodaw
