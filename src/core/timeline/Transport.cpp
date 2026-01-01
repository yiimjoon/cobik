#include "Transport.h"
#include "PPQ.h"
#include "../../ui/panels/DebugLogWindow.h"

namespace pianodaw {

Transport::Transport()
{
}

Transport::~Transport()
{
    stopTimer();
}

void Transport::start()
{
    DBG("Transport::start() called");
    if (!playing)
    {
        playing = true;
        lastTimeMs = juce::Time::getMillisecondCounter();
        startTimerHz(60); // 60 FPS for smooth playhead
        if (onStatusChanged) onStatusChanged();
    }
}

void Transport::stop()
{
    DBG("Transport::stop() called");
    if (playing)
    {
        playing = false;
        stopTimer();
        if (onStatusChanged) onStatusChanged();
    }
}

void Transport::setPlaying(bool play)
{
    if (play) start();
    else stop();
}

void Transport::setPosition(int64_t ticks)
{
    currentTick = std::max((int64_t)0, ticks);
    if (onPositionChanged) onPositionChanged(currentTick);
}

void Transport::setTempo(double bpm)
{
    currentBPM = std::max(1.0, bpm);
}

void Transport::timerCallback()
{
    if (!playing) return;

    auto now = juce::Time::getMillisecondCounter();
    auto deltaMs = now - lastTimeMs;
    lastTimeMs = now;

    // Convert time delta to ticks
    // ms -> seconds -> beats -> ticks
    double deltaSeconds = deltaMs / 1000.0;
    double beatsPerSecond = currentBPM / 60.0;
    double deltaTicks = deltaSeconds * beatsPerSecond * PPQ::TICKS_PER_QUARTER;

    currentTick += (int64_t)std::round(deltaTicks);

    // Looping
    if (looping && currentTick >= loopEnd)
    {
        currentTick = loopStart + (currentTick % (loopEnd - loopStart));
        DebugLogWindow::addLog("Transport: Loop! Position reset to " + juce::String(loopStart) + ", tick " + juce::String(currentTick % (loopEnd - loopStart)));
    }
    
    if (onPositionChanged) onPositionChanged(currentTick);
}
}

} // namespace pianodaw
