#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include "Clip.h"
#include <vector>
#include <memory>

namespace pianodaw {

/**
 * ClipRegion - A placed instance of a Clip on a Track
 * Represents where and when a clip appears in the arrangement
 */
struct ClipRegion
{
    int id = 0;
    Clip* clip = nullptr;              // Pointer to the actual clip data
    int64_t startTick = 0;             // Position in timeline
    int64_t offsetTick = 0;            // Offset into clip (for loop/trim)
    int64_t lengthTick = 0;            // Duration in timeline (can be different from clip length)
    bool muted = false;
    
    ClipRegion() = default;
    ClipRegion(Clip* clip_, int64_t start, int64_t length)
        : clip(clip_), startTick(start), lengthTick(length) {}
    
    int64_t getEndTick() const { return startTick + lengthTick; }
};

/**
 * Track - A single track in the arrangement
 * Can contain multiple clip regions
 */
class Track
{
public:
    enum class Type { MIDI, Audio, Instrument, Drum, Group, Folder };

    Track(const juce::String& name_, Type type_ = Type::MIDI)
        : name(name_), type(type_) {}

    ~Track() = default;

    // Basic properties
    juce::String getName() const { return name; }
    void setName(const juce::String& n) { name = n; }

    Type getType() const { return type; }

    // Type checking methods
    bool isMIDI() const { return type == Type::MIDI; }
    bool isAudio() const { return type == Type::Audio; }
    bool isInstrument() const { return type == Type::Instrument; }
    bool isDrum() const { return type == Type::Drum; }
    bool isFolder() const { return type == Type::Folder; }
    bool isGroup() const { return type == Type::Group; }

    juce::Colour getColour() const { return colour; }
    void setColour(juce::Colour c) { colour = c; }

    // State
    bool isSolo() const { return solo; }
    void setSolo(bool s) { solo = s; }

    bool isMuted() const { return muted; }
    void setMute(bool m) { muted = m; }

    float getVolume() const { return volume; }
    void setVolume(float v) { volume = juce::jlimit(0.0f, 1.0f, v); }

    float getPan() const { return pan; }
    void setPan(float p) { pan = juce::jlimit(-1.0f, 1.0f, p); }

    // Per-track quantize settings
    struct QuantizeSettings {
        bool enabled = true;
        float strength = 1.0f;
        float swing = 0.5f;
        bool smartChord = true;
    };

    QuantizeSettings getQuantizeSettings() const { return quantizeSettings; }
    void setQuantizeSettings(const QuantizeSettings& settings) { quantizeSettings = settings; }
    
    // Clip regions
    void addClipRegion(const ClipRegion& region)
    {
        clipRegions.push_back(region);
        clipRegions.back().id = nextRegionId++;
    }
    
    void removeClipRegion(int regionId)
    {
        clipRegions.erase(
            std::remove_if(clipRegions.begin(), clipRegions.end(),
                [regionId](const ClipRegion& r) { return r.id == regionId; }),
            clipRegions.end());
    }
    
    ClipRegion* findClipRegion(int regionId)
    {
        for (auto& r : clipRegions)
        {
            if (r.id == regionId) return &r;
        }
        return nullptr;
    }
    
    const std::vector<ClipRegion>& getClipRegions() const { return clipRegions; }
    std::vector<ClipRegion>& getClipRegions() { return clipRegions; }
    
    // Find clip region at specific time
    ClipRegion* findClipAtTime(int64_t tick)
    {
        for (auto& region : clipRegions)
        {
            if (tick >= region.startTick && tick < region.getEndTick())
            {
                return &region;
            }
        }
        return nullptr;
    }
    
    juce::CriticalSection& getLock() { return lock; }
    
private:
    juce::String name;
    Type type;
    juce::Colour colour = juce::Colours::blue;

    bool solo = false;
    bool muted = false;
    float volume = 0.8f;  // 0.0 to 1.0
    float pan = 0.0f;     // -1.0 (left) to 1.0 (right)

    QuantizeSettings quantizeSettings;
    
    std::vector<ClipRegion> clipRegions;
    int nextRegionId = 1;
    
    juce::CriticalSection lock;
    
    JUCE_LEAK_DETECTOR(Track)
};

} // namespace pianodaw
