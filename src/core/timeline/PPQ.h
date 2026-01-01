#pragma once

#include <cstdint>
#include <cmath>

namespace pianodaw {

/**
 * PPQ (Pulses Per Quarter note) timing system
 * 
 * MIDI standard: 960 ticks per quarter note
 * This provides fine resolution for musical timing.
 */
class PPQ
{
public:
    static constexpr int32_t TICKS_PER_QUARTER = 960;
    
    /**
     * Convert tick to beat (quarter note)
     * @param tick Absolute tick position
     * @return Beat position (0.0 = start, 1.0 = one quarter note, etc.)
     */
    static double tickToBeat(int64_t tick)
    {
        return static_cast<double>(tick) / static_cast<double>(TICKS_PER_QUARTER);
    }
    
    /**
     * Convert beat to tick
     * @param beat Beat position
     * @return Tick position
     */
    static int64_t beatToTick(double beat)
    {
        return static_cast<int64_t>(std::round(beat * TICKS_PER_QUARTER));
    }
    
    /**
     * Convert tick to seconds (requires tempo in BPM)
     * @param tick Absolute tick position
     * @param tempoBPM Tempo in beats per minute
     * @return Time in seconds
     */
    static double tickToSeconds(int64_t tick, double tempoBPM)
    {
        double beatsPerSecond = tempoBPM / 60.0;
        double ticksPerSecond = beatsPerSecond * TICKS_PER_QUARTER;
        return static_cast<double>(tick) / ticksPerSecond;
    }
    
    /**
     * Convert seconds to tick (requires tempo in BPM)
     * @param seconds Time in seconds
     * @param tempoBPM Tempo in beats per minute
     * @return Tick position
     */
    static int64_t secondsToTick(double seconds, double tempoBPM)
    {
        double beatsPerSecond = tempoBPM / 60.0;
        double ticksPerSecond = beatsPerSecond * TICKS_PER_QUARTER;
        return static_cast<int64_t>(std::round(seconds * ticksPerSecond));
    }
    
    /**
     * Convert tick to bar:beat representation
     * @param tick Absolute tick position
     * @param beatsPerBar Time signature numerator (e.g., 4 for 4/4)
     * @param barOut Output: bar number (1-based)
     * @param beatOut Output: beat within bar (0-based)
     * @param tickOut Output: tick within beat
     */
    static void tickToBarBeat(int64_t tick, int beatsPerBar, 
                               int& barOut, int& beatOut, int& tickOut)
    {
        int64_t ticksPerBar = TICKS_PER_QUARTER * beatsPerBar;
        
        barOut = static_cast<int>(tick / ticksPerBar) + 1;  // 1-based bars
        int64_t tickInBar = tick % ticksPerBar;
        
        beatOut = static_cast<int>(tickInBar / TICKS_PER_QUARTER);
        tickOut = static_cast<int>(tickInBar % TICKS_PER_QUARTER);
    }
    
    /**
     * Convert bar:beat:tick to absolute tick
     * @param bar Bar number (1-based)
     * @param beat Beat within bar (0-based)
     * @param tick Tick within beat
     * @param beatsPerBar Time signature numerator
     * @return Absolute tick position
     */
    static int64_t barBeatToTick(int bar, int beat, int tick, int beatsPerBar)
    {
        int64_t ticksPerBar = TICKS_PER_QUARTER * beatsPerBar;
        int64_t result = (bar - 1) * ticksPerBar;  // bar is 1-based
        result += beat * TICKS_PER_QUARTER;
        result += tick;
        return result;
    }
    
    /** 
     * Snap tick to the nearest grid point
     * @param tick Input tick
     * @param gridTicks Grid size in ticks (e.g., TICKS_PER_QUARTER/4 for 16th notes)
     * @return Snapped tick
     */
    static int64_t snapToGrid(int64_t tick, int64_t gridTicks)
    {
        if (gridTicks <= 0) return tick;
        
        int64_t remainder = tick % gridTicks;
        int64_t half = gridTicks / 2;
        
        if (remainder >= half)
            return tick + (gridTicks - remainder);
        else
            return tick - remainder;
    }
};

/**
 * Grid size definitions
 */
enum class GridSize
{
    Whole = 0,       // 1/1 note (whole note)
    Half,            // 1/2 note
    Quarter,         // 1/4 note
    Eighth,          // 1/8 note
    Sixteenth,       // 1/16 note
    ThirtySecond,    // 1/32 note
    EighthTriplet,   // 1/8 triplet
    SixteenthTriplet // 1/16 triplet
};

/**
 * Convert GridSize enum to tick value
 */
inline int64_t gridSizeToTicks(GridSize size)
{
    switch (size)
    {
        case GridSize::Whole:
            return PPQ::TICKS_PER_QUARTER * 4;
        case GridSize::Half:
            return PPQ::TICKS_PER_QUARTER * 2;
        case GridSize::Quarter:
            return PPQ::TICKS_PER_QUARTER;
        case GridSize::Eighth:
            return PPQ::TICKS_PER_QUARTER / 2;
        case GridSize::Sixteenth:
            return PPQ::TICKS_PER_QUARTER / 4;
        case GridSize::ThirtySecond:
            return PPQ::TICKS_PER_QUARTER / 8;
        case GridSize::EighthTriplet:
            return PPQ::TICKS_PER_QUARTER * 2 / 3;  // 2 beats in space of 3
        case GridSize::SixteenthTriplet:
            return PPQ::TICKS_PER_QUARTER / 3;      // 3 notes in space of 1 beat
        default:
            return PPQ::TICKS_PER_QUARTER;
    }
}

/**
 * Convert GridSize enum to string representation
 */
inline const char* gridSizeToString(GridSize size)
{
    switch (size)
    {
        case GridSize::Whole:        return "1/1";
        case GridSize::Half:         return "1/2";
        case GridSize::Quarter:      return "1/4";
        case GridSize::Eighth:       return "1/8";
        case GridSize::Sixteenth:    return "1/16";
        case GridSize::ThirtySecond: return "1/32";
        default:                     return "1/16";
    }
}

} // namespace pianodaw
