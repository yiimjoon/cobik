#pragma once

#include <juce_core/juce_core.h>
#include "Track.h"
#include "Clip.h"
#include "../timeline/PPQ.h"
#include <vector>
#include <memory>

namespace pianodaw {

/**
 * Project - Top-level container for the entire DAW project
 * Contains tracks, clips, tempo, time signature, etc.
 */
class Project
{
public:
    Project(const juce::String& name_ = "Untitled")
        : name(name_), tempo(120.0), timeSignatureNumerator(4), timeSignatureDenominator(4) {}
    
    ~Project() = default;
    
    // Basic properties
    juce::String getName() const { return name; }
    void setName(const juce::String& n) { name = n; }
    
    double getTempo() const { return tempo; }
    void setTempo(double t) { tempo = juce::jlimit(20.0, 999.0, t); }
    
    int getTimeSignatureNumerator() const { return timeSignatureNumerator; }
    int getTimeSignatureDenominator() const { return timeSignatureDenominator; }
    void setTimeSignature(int numerator, int denominator)
    {
        timeSignatureNumerator = juce::jlimit(1, 32, numerator);
        timeSignatureDenominator = juce::jlimit(1, 32, denominator);
    }
    
    int64_t getProjectLengthTicks() const { return projectLengthTicks; }
    void setProjectLengthTicks(int64_t length) { projectLengthTicks = length; }
    
    // Track management
    Track* addTrack(const juce::String& name, Track::Type type = Track::Type::MIDI)
    {
        auto track = std::make_unique<Track>(name, type);
        Track* ptr = track.get();
        tracks.push_back(std::move(track));
        return ptr;
    }
    
    void removeTrack(int index)
    {
        if (index >= 0 && index < (int)tracks.size())
        {
            tracks.erase(tracks.begin() + index);
        }
    }
    
    Track* getTrack(int index)
    {
        if (index >= 0 && index < (int)tracks.size())
        {
            return tracks[index].get();
        }
        return nullptr;
    }
    
    int getNumTracks() const { return (int)tracks.size(); }
    
    const std::vector<std::unique_ptr<Track>>& getTracks() const { return tracks; }
    std::vector<std::unique_ptr<Track>>& getTracks() { return tracks; }
    
    // Clip pool management (all clips stored here)
    Clip* addClip(const juce::String& name = "New Clip")
    {
        auto clip = std::make_unique<Clip>();
        Clip* ptr = clip.get();
        clips.push_back(std::move(clip));
        return ptr;
    }
    
    void removeClip(Clip* clip)
    {
        clips.erase(
            std::remove_if(clips.begin(), clips.end(),
                [clip](const std::unique_ptr<Clip>& c) { return c.get() == clip; }),
            clips.end());
    }
    
    const std::vector<std::unique_ptr<Clip>>& getClips() const { return clips; }
    std::vector<std::unique_ptr<Clip>>& getClips() { return clips; }
    
    // File path
    juce::File getProjectFile() const { return projectFile; }
    void setProjectFile(const juce::File& file) { projectFile = file; }
    
    bool isModified() const { return modified; }
    void setModified(bool m) { modified = m; }
    
    // Loop markers
    int64_t getLoopStart() const { return loopStart; }
    int64_t getLoopEnd() const { return loopEnd; }
    void setLoopStart(int64_t tick) { loopStart = std::max((int64_t)0, tick); modified = true; }
    void setLoopEnd(int64_t tick) { loopEnd = std::max(loopStart, tick); modified = true; }
    void setLoopRange(int64_t start, int64_t end) { 
        loopStart = std::max((int64_t)0, start);
        loopEnd = std::max(loopStart, end);
        modified = true;
    }
    
    // File I/O
    bool saveToFile(const juce::File& file);
    bool loadFromFile(const juce::File& file);
    juce::XmlElement* toXml() const;
    bool fromXml(const juce::XmlElement& xml);
    
    juce::CriticalSection& getLock() { return lock; }
    
private:
    juce::String name;
    double tempo;
    int timeSignatureNumerator;
    int timeSignatureDenominator;
    int64_t projectLengthTicks = PPQ::TICKS_PER_QUARTER * 4 * 32; // 32 bars default
    
    // Loop markers
    int64_t loopStart = 0;
    int64_t loopEnd = PPQ::TICKS_PER_QUARTER * 4 * 4;  // 4 bars default
    
    std::vector<std::unique_ptr<Track>> tracks;
    std::vector<std::unique_ptr<Clip>> clips;  // Clip pool
    
    juce::File projectFile;
    bool modified = false;
    
    juce::CriticalSection lock;
    
    JUCE_LEAK_DETECTOR(Project)
};

} // namespace pianodaw
