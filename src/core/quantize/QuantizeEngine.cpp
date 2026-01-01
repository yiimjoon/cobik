#include "QuantizeEngine.h"
#include <algorithm>

namespace pianodaw {

std::vector<int> QuantizeEngine::quantize(Clip& clip, const std::vector<int>& noteIds, const QuantizeParams& params)
{
    juce::ScopedLock sl(clip.getLock());
    std::vector<int> modifiedIds;
    
    for (int id : noteIds)
    {
        if (auto* note = clip.findNote(id))
        {
            int64_t oldStart = note->startTick;
            quantize(*note, params);
            if (note->startTick != oldStart)
                modifiedIds.push_back(id);
        }
    }
    
    return modifiedIds;
}

void QuantizeEngine::quantize(Note& note, const QuantizeParams& params)
{
    int64_t target = getTargetTick(note.startTick, params);
    int64_t diff = target - note.startTick;
    int64_t move = (int64_t)(diff * params.strength);
    
    int64_t duration = note.endTick - note.startTick;
    note.startTick += move;
    note.endTick = note.startTick + duration;
}

int64_t QuantizeEngine::getTargetTick(int64_t sourceTick, const QuantizeParams& params)
{
    int64_t grid = params.gridTicks;
    if (grid <= 0) return sourceTick;
    
    // Basic snap to grid
    int64_t lower = (sourceTick / grid) * grid;
    int64_t upper = lower + grid;
    
    int64_t target = (sourceTick - lower < upper - sourceTick) ? lower : upper;
    
    // Apply swing (simple 50-75% style)
    if (params.swing != 0.5f && (target % (grid * 2)) != 0)
    {
        int64_t swingOffset = (int64_t)(grid * (params.swing - 0.5f) * 2.0f);
        target += swingOffset;
    }
    
    return target;
}

} // namespace pianodaw
