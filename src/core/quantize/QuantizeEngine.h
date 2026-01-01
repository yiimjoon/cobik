#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../model/Clip.h"
#include "../timeline/PPQ.h"

namespace pianodaw {

/**
 * QuantizeParams - parameters for the quantization process
 */
struct QuantizeParams
{
    int64_t gridTicks = 240; 
    float strength = 1.0f;   
    float swing = 0.5f;     
    int64_t rangeTicks = 0;  
    
    bool smartChord = true;  
    bool smartPedal = true;  
    
    enum class GridMode { Fixed, Auto };
    GridMode gridMode = GridMode::Fixed;
};

/**
 * QuantizeEngine - Logic for rhythmic correction
 */
class QuantizeEngine
{
public:
    /** Quantize a set of notes within a clip */
    static std::vector<int> quantize(Clip& clip, const std::vector<int>& noteIds, const QuantizeParams& params);

    /** Quantize a single note object (in-place) */
    static void quantize(Note& note, const QuantizeParams& params);

private:
    static int64_t getTargetTick(int64_t sourceTick, const QuantizeParams& params);
};

} // namespace pianodaw
