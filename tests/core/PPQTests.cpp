#include "PPQ.h"
#include <cassert>
#include <cmath>

namespace pianodaw {

// Test basic conversion
void testPPQConversions()
{
    // Test tick to beat
    assert(std::abs(PPQ::tickToBeat(960) - 1.0) < 0.001);
    assert(std::abs(PPQ::tickToBeat(480) - 0.5) < 0.001);
    
    // Test beat to tick
    assert(PPQ::beatToTick(1.0) == 960);
    assert(PPQ::beatToTick(0.5) == 480);
    
    // Test tick to seconds (120 BPM = 2 beats per second)
    double seconds = PPQ::tickToSeconds(960, 120.0);
    assert(std::abs(seconds - 0.5) < 0.001); // 1 beat at 120 BPM is 0.5 seconds
    
    // Test seconds to tick
    int64_t tick = PPQ::secondsToTick(0.5, 120.0);
    assert(tick == 960);
    
    // Test bar:beat conversion (4/4 time)
    int bar, beat, tickInBeat;
    PPQ::tickToBarBeat(960 * 4, 4, bar, beat, tickInBeat);  // 4 beats = 1 bar
    assert(bar == 2 && beat == 0 && tickInBeat == 0);  // Start of bar 2
    
    int64_t reconstructed = PPQ::barBeatToTick(2, 0, 0, 4);
    assert(reconstructed == 960 * 4);
}

} // namespace pianodaw
