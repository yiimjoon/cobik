#pragma once

#include <cstdint>
#include <algorithm>

namespace pianodaw {

/**
 * MIDI CC (Control Change) event
 * 
 * For MVP, we focus on CC64 (sustain pedal)
 * Future: support more CCs (expression, modulation, etc.)
 */
struct CCEvent
{
    int cc;             // CC number (64 = sustain pedal)
    int64_t tick;       // When the event occurs
    int value;          // CC value (0-127)
    
    CCEvent() : cc(64), tick(0), value(0) {}
    
    CCEvent(int cc_, int64_t tick_, int val)
        : cc(cc_), tick(tick_), value(val)
    {
        cc = std::clamp(cc, 0, 127);
        value = std::clamp(value, 0, 127);
        
        if (tick < 0) tick = 0;
    }
    
    /** Check if this is a sustain pedal event */
    bool isSustainPedal() const { return cc == 64; }
    
    /** Check if pedal is "on" (>= 64 is on for sustain) */
    bool isPedalOn() const { return isSustainPedal() && value >= 64; }
    
    /** Comparison for sorting by time */
    bool operator<(const CCEvent& other) const
    {
        if (tick != other.tick)
            return tick < other.tick;
        return cc < other.cc;
    }
};

/**
 * CC64 (Sustain Pedal) specific utilities
 */
namespace CC64
{
    constexpr int CC_NUMBER = 64;
    constexpr int VALUE_ON = 127;
    constexpr int VALUE_OFF = 0;
    
    /** Create pedal down event */
    inline CCEvent pedalDown(int64_t tick)
    {
        return CCEvent(CC_NUMBER, tick, VALUE_ON);
    }
    
    /** Create pedal up event */
    inline CCEvent pedalUp(int64_t tick)
    {
        return CCEvent(CC_NUMBER, tick, VALUE_OFF);
    }
}

} // namespace pianodaw
