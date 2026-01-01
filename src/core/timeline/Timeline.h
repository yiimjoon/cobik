#pragma once

#include <vector>

namespace pianodaw {

/**
 * Tempo event for tempo changes
 * MVP: We start with fixed tempo, but this allows future expansion
 */
struct TempoEvent
{
    int64_t tick;     // When does the tempo change occur
    double bpm;       // Tempo in beats per minute
    
    TempoEvent(int64_t t, double b) : tick(t), bpm(b) {}
};

/**
 * Time signature event
 * MVP: We start with fixed 4/4, but this allows future expansion
 */
struct TimeSigEvent
{
    int64_t tick;       // When does the time signature change occur
    int numerator;      // Top number (4 in 4/4)
    int denominator;    // Bottom number (4 in 4/4 means quarter note gets the beat)
    
    TimeSigEvent(int64_t t, int n, int d) 
        : tick(t), numerator(n), denominator(d) {}
};

/**
 * Timeline manages tempo and time signature events
 * 
 * MVP: Single tempo (120 BPM) and time signature (4/4)
 * Future: Multiple tempo/time signature changes
 */
class Timeline
{
public:
    Timeline()
    {
        // Default: 120 BPM at start
        tempoEvents.push_back(TempoEvent(0, 120.0));
        
        // Default: 4/4 time at start
        timeSigEvents.push_back(TimeSigEvent(0, 4, 4));
    }
    
    /** Get tempo at given tick */
    double getTempoAtTick(int64_t tick) const
    {
        // Find the last tempo event before or at this tick
        for (int i = static_cast<int>(tempoEvents.size()) - 1; i >= 0; --i)
        {
            if (tempoEvents[i].tick <= tick)
                return tempoEvents[i].bpm;
        }
        
        return 120.0; // Fallback
    }
    
    /** Get time signature at given tick */
    TimeSigEvent getTimeSigAtTick(int64_t tick) const
    {
        // Find the last time sig event before or at this tick
        for (int i = static_cast<int>(timeSigEvents.size()) - 1; i >= 0; --i)
        {
            if (timeSigEvents[i].tick <= tick)
                return timeSigEvents[i];
        }
        
        return TimeSigEvent(0, 4, 4); // Fallback to 4/4
    }
    
    /** Add tempo change (future expansion) */
    void addTempoEvent(int64_t tick, double bpm)
    {
        // Keep sorted by tick
        auto it = tempoEvents.begin();
        while (it != tempoEvents.end() && it->tick < tick)
            ++it;
        
        tempoEvents.insert(it, TempoEvent(tick, bpm));
    }
    
    /** Add time signature change (future expansion) */
    void addTimeSigEvent(int64_t tick, int numerator, int denominator)
    {
        auto it = timeSigEvents.begin();
        while (it != timeSigEvents.end() && it->tick < tick)
            ++it;
        
        timeSigEvents.insert(it, TimeSigEvent(tick, numerator, denominator));
    }
    
private:
    std::vector<TempoEvent> tempoEvents;
    std::vector<TimeSigEvent> timeSigEvents;
};

} // namespace pianodaw
