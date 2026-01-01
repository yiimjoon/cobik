#pragma once

#include <cstdint>
#include <algorithm>
#include <string>
#include <algorithm>

namespace pianodaw {

/**
 * Single MIDI note
 * 
 * Represents a note with timing, pitch, and velocity.
 * All timing is in ticks (PPQ-based).
 */
struct Note
{
    int id;                 // Unique identifier
    int pitch;              // MIDI pitch (0-127, middle C = 60)
    int64_t startTick;      // Note start position
    int64_t endTick;        // Note end position
    int velocity;           // MIDI velocity (0-127, 0 = silent, 127 = max)
    
    Note()
        : id(0), pitch(60), startTick(0), endTick(960), velocity(100)
    {}
    
    Note(int id_, int pitch_, int64_t start, int64_t end, int vel)
        : id(id_), pitch(pitch_), startTick(start), endTick(end), velocity(vel)
    {
        // Ensure valid ranges
        pitch = std::max(0, std::min(127, pitch));
        velocity = std::max(1, std::min(127, velocity));  // 0 velocity not allowed
        
        // Ensure end >= start
        if (endTick < startTick)
            endTick = startTick + 240;  // Minimum note length (1/16th at 960 PPQ)
    }
    
    /** Get note duration in ticks */
    int64_t getDuration() const
    {
        return endTick - startTick;
    }
    
    /** Check if note contains given tick */
    bool containsTick(int64_t tick) const
    {
        return tick >= startTick && tick < endTick;
    }
    
    /** Check if note overlaps with time range */
    bool overlaps(int64_t rangeStart, int64_t rangeEnd) const
    {
        return !(endTick <= rangeStart || startTick >= rangeEnd);
    }
    
    /** Move note by delta ticks */
    void move(int64_t deltaTicks)
    {
        startTick += deltaTicks;
        endTick += deltaTicks;
        
        // Don't allow negative positions
        if (startTick < 0)
        {
            int64_t offset = -startTick;
            startTick += offset;
            endTick += offset;
        }
    }
    
    /** Resize note (change duration) */
    void resize(int64_t newEndTick)
    {
        endTick = std::max(newEndTick, startTick + 60);  // Minimum 60 ticks
    }
    
    /** Comparison for sorting by start time */
    bool operator<(const Note& other) const
    {
        if (startTick != other.startTick)
            return startTick < other.startTick;
        return pitch < other.pitch;
    }
};

} // namespace pianodaw
