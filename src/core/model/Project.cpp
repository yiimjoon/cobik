#include "Project.h"
#include "Clip.h"
#include "Track.h"

namespace pianodaw {

bool Project::saveToFile(const juce::File& file)
{
    auto xml = toXml();
    if (!xml)
        return false;
    
    bool success = xml->writeToFile(file, juce::String());
    
    if (success) {
        projectFile = file;
        modified = false;
    }
    
    return success;
}

bool Project::loadFromFile(const juce::File& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if (!xml)
        return false;
    
    bool success = fromXml(*xml);
    
    if (success) {
        projectFile = file;
        modified = false;
    }
    
    return success;
}

juce::XmlElement* Project::toXml() const
{
    auto* root = new juce::XmlElement("PianoDAWProject");
    root->setAttribute("version", "1.0");
    
    // Project info
    auto* info = root->createNewChildElement("ProjectInfo");
    info->createNewChildElement("Name")->addTextElement(name);
    info->createNewChildElement("Tempo")->addTextElement(juce::String(tempo));
    
    auto* timeSig = info->createNewChildElement("TimeSignature");
    timeSig->setAttribute("numerator", timeSignatureNumerator);
    timeSig->setAttribute("denominator", timeSignatureDenominator);
    
    info->createNewChildElement("ProjectLength")->addTextElement(juce::String(projectLengthTicks));
    
    // Clips
    auto* clipsXml = root->createNewChildElement("Clips");
    int clipId = 1;
    
    juce::HashMap<Clip*, int> clipToId;  // Map clip pointer to ID
    
    for (const auto& clip : clips) {
        clipToId.set(clip.get(), clipId);
        
        auto* clipXml = clipsXml->createNewChildElement("Clip");
        clipXml->setAttribute("id", clipId++);
        clipXml->setAttribute("name", clip->getName());
        
        // Notes
        juce::ScopedLock sl(clip->getLock());
        for (const auto& note : clip->getNotes()) {
            auto* noteXml = clipXml->createNewChildElement("Note");
            noteXml->setAttribute("id", note.id);
            noteXml->setAttribute("pitch", note.pitch);
            noteXml->setAttribute("startTick", (int)note.startTick);
            noteXml->setAttribute("endTick", (int)note.endTick);
            noteXml->setAttribute("velocity", (int)(note.velocity * 127.0f));
        }
        
        // CC Events
        for (const auto& cc : clip->getCCEvents()) {
            auto* ccXml = clipXml->createNewChildElement("CCEvent");
            ccXml->setAttribute("cc", cc.cc);
            ccXml->setAttribute("tick", (int)cc.tick);
            ccXml->setAttribute("value", cc.value);
        }
    }
    
    // Tracks
    auto* tracksXml = root->createNewChildElement("Tracks");
    int trackId = 1;
    
    for (const auto& track : tracks) {
        auto* trackXml = tracksXml->createNewChildElement("Track");
        trackXml->setAttribute("id", trackId++);
        trackXml->setAttribute("name", track->getName());
        trackXml->setAttribute("type", track->getType() == Track::Type::MIDI ? "MIDI" : "Audio");
        
        // Color
        auto color = track->getColour();
        auto* colorXml = trackXml->createNewChildElement("Color");
        colorXml->setAttribute("r", color.getRed());
        colorXml->setAttribute("g", color.getGreen());
        colorXml->setAttribute("b", color.getBlue());
        
        // State
        trackXml->createNewChildElement("Solo")->addTextElement(track->isSolo() ? "true" : "false");
        trackXml->createNewChildElement("Mute")->addTextElement(track->isMuted() ? "true" : "false");
        trackXml->createNewChildElement("Volume")->addTextElement(juce::String(track->getVolume()));
        trackXml->createNewChildElement("Pan")->addTextElement(juce::String(track->getPan()));
        
        // Clip regions
        juce::ScopedLock sl(track->getLock());
        for (const auto& region : track->getClipRegions()) {
            auto* regionXml = trackXml->createNewChildElement("ClipRegion");
            
            int clipId = clipToId[region.clip];
            regionXml->setAttribute("clipId", clipId);
            regionXml->setAttribute("startTick", (int)region.startTick);
            regionXml->setAttribute("offsetTick", (int)region.offsetTick);
            regionXml->setAttribute("lengthTick", (int)region.lengthTick);
            regionXml->setAttribute("muted", region.muted ? "true" : "false");
        }
    }
    
    return root;
}

bool Project::fromXml(const juce::XmlElement& xml)
{
    if (xml.getTagName() != "PianoDAWProject")
        return false;
    
    // Clear existing data
    tracks.clear();
    clips.clear();
    
    // Load project info
    auto* info = xml.getChildByName("ProjectInfo");
    if (info) {
        name = info->getChildByName("Name")->getAllSubText();
        tempo = info->getChildByName("Tempo")->getAllSubText().getDoubleValue();
        
        auto* timeSig = info->getChildByName("TimeSignature");
        if (timeSig) {
            timeSignatureNumerator = timeSig->getIntAttribute("numerator", 4);
            timeSignatureDenominator = timeSig->getIntAttribute("denominator", 4);
        }
        
        projectLengthTicks = info->getChildByName("ProjectLength")->getAllSubText().getLargeIntValue();
    }
    
    // Load clips
    juce::HashMap<int, Clip*> idToClip;  // Map ID to clip pointer
    
    auto* clipsXml = xml.getChildByName("Clips");
    if (clipsXml) {
        for (auto* clipXml : clipsXml->getChildIterator()) {
            if (clipXml->getTagName() != "Clip") continue;
            
            int clipId = clipXml->getIntAttribute("id");
            juce::String clipName = clipXml->getStringAttribute("name");
            
            Clip* clip = addClip(clipName);
            idToClip.set(clipId, clip);
            
            // Load notes
            for (auto* noteXml : clipXml->getChildIterator()) {
                if (noteXml->getTagName() == "Note") {
                    int pitch = noteXml->getIntAttribute("pitch");
                    int64_t startTick = noteXml->getIntAttribute("startTick");
                    int64_t endTick = noteXml->getIntAttribute("endTick");
                    int velocity = noteXml->getIntAttribute("velocity");
                    
                    clip->addNote(pitch, startTick, endTick, velocity);
                }
                else if (noteXml->getTagName() == "CCEvent") {
                    int cc = noteXml->getIntAttribute("cc");
                    int64_t tick = noteXml->getIntAttribute("tick");
                    int value = noteXml->getIntAttribute("value");
                    
                    clip->addCCEvent(cc, tick, value);
                }
            }
        }
    }
    
    // Load tracks
    auto* tracksXml = xml.getChildByName("Tracks");
    if (tracksXml) {
        for (auto* trackXml : tracksXml->getChildIterator()) {
            if (trackXml->getTagName() != "Track") continue;
            
            juce::String trackName = trackXml->getStringAttribute("name");
            juce::String typeStr = trackXml->getStringAttribute("type");
            Track::Type type = (typeStr == "MIDI") ? Track::Type::MIDI : Track::Type::Audio;
            
            Track* track = addTrack(trackName, type);
            
            // Load color
            auto* colorXml = trackXml->getChildByName("Color");
            if (colorXml) {
                int r = colorXml->getIntAttribute("r");
                int g = colorXml->getIntAttribute("g");
                int b = colorXml->getIntAttribute("b");
                track->setColour(juce::Colour((juce::uint8)r, (juce::uint8)g, (juce::uint8)b));
            }
            
            // Load state
            if (auto* solo = trackXml->getChildByName("Solo"))
                track->setSolo(solo->getAllSubText() == "true");
            if (auto* mute = trackXml->getChildByName("Mute"))
                track->setMute(mute->getAllSubText() == "true");
            if (auto* volume = trackXml->getChildByName("Volume"))
                track->setVolume(volume->getAllSubText().getFloatValue());
            if (auto* pan = trackXml->getChildByName("Pan"))
                track->setPan(pan->getAllSubText().getFloatValue());
            
            // Load clip regions
            for (auto* regionXml : trackXml->getChildIterator()) {
                if (regionXml->getTagName() != "ClipRegion") continue;
                
                int clipId = regionXml->getIntAttribute("clipId");
                Clip* clip = idToClip[clipId];
                if (!clip) continue;
                
                int64_t startTick = regionXml->getIntAttribute("startTick");
                int64_t offsetTick = regionXml->getIntAttribute("offsetTick");
                int64_t lengthTick = regionXml->getIntAttribute("lengTick");
                bool muted = regionXml->getStringAttribute("muted") == "true";
                
                ClipRegion region(clip, startTick, lengthTick);
                region.offsetTick = offsetTick;
                region.muted = muted;
                
                track->addClipRegion(region);
            }
        }
    }
    
    return true;
}

} // namespace pianodaw
