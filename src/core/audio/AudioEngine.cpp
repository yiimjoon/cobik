#include "AudioEngine.h"
#include "MidiRecorder.h"
#include "../model/Clip.h"
#include "../model/Project.h"
#include "../model/Track.h"
#include "../timeline/Transport.h"
#include "../timeline/PPQ.h"
#include <cmath>

namespace pianodaw {

// Basic Piano Voice using Sine with simple envelope
struct SimplePianoVoice : public juce::SynthesiserVoice
{
    SimplePianoVoice() {}

    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return sound != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        level = velocity * 0.15f;
        tailOff = 0.0;

        auto cyclesPerSample = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber) / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        currentAngle = 0.0;
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0)
            {
                while (--numSamples >= 0)
                {
                    auto sample = (float)(std::sin(currentAngle) * level * tailOff);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, sample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99;

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote();
                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0)
                {
                    auto sample = (float)(std::sin(currentAngle) * level);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, sample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
};

struct SimplePianoSound : public juce::SynthesiserSound
{
    SimplePianoSound() {}
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

//==============================================================================

AudioEngine::AudioEngine(Project& project_, Transport& transport_)
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)  // For audio recording (future)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      project(project_), transport(transport_)
{
    pluginFormatManager.addDefaultFormats();
    mainGraph = std::make_unique<juce::AudioProcessorGraph>();
    loadPluginList();
    setupVoices();
    
    // Create MIDI recorder
    midiRecorder = std::make_unique<MidiRecorder>();
}

AudioEngine::~AudioEngine() {}

const juce::String AudioEngine::getName() const { return "AudioEngine"; }
bool AudioEngine::acceptsMidi() const { return true; }
bool AudioEngine::producesMidi() const { return false; }
bool AudioEngine::isMidiEffect() const { return false; }
double AudioEngine::getTailLengthSeconds() const { return 0.0; }

void AudioEngine::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    
    if (mainGraph != nullptr)
    {
        mainGraph->setPlayConfigDetails(getMainBusNumInputChannels(),
                                      getMainBusNumOutputChannels(),
                                      sampleRate, samplesPerBlock);
        mainGraph->prepareToPlay(sampleRate, samplesPerBlock);
        updateGraph();
    }
}

void AudioEngine::releaseResources() {}

void AudioEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedLock sl(project.getLock());
    
    // Copy incoming MIDI for recording
    juce::MidiBuffer incomingMidi = midiMessages;
    
    // Merge UI MIDI events (piano roll preview notes)
    {
        juce::ScopedLock uiLock(uiMidiLock);
        midiMessages.addEvents(uiMidiBuffer, 0, buffer.getNumSamples(), 0);
        uiMidiBuffer.clear();
    }

    if (transport.isPlaying())
    {
        // Generate MIDI from all tracks
        processMidiSequencer(midiMessages);
        
        // Record incoming MIDI if armed
        int64_t currentTick = transport.getPosition();
        processMidiRecording(incomingMidi, currentTick);
    }
    else if (lastProcessedTick != -1)
    {
        synth.allNotesOff(0, false);
        
        // Send MIDI All Notes Off and All Sound Off to all channels for hosted plugins
        for (int ch = 1; ch <= 16; ++ch)
        {
            midiMessages.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
            midiMessages.addEvent(juce::MidiMessage::allSoundOff(ch), 0);
        }
        
        lastProcessedTick = -1;
    }

    if (mainGraph != nullptr && instrumentNode != nullptr)
    {
        mainGraph->processBlock(buffer, midiMessages);
    }
    else
    {
        buffer.clear();
        synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }
}

void AudioEngine::processBlock(juce::AudioBuffer<double>& /*buffer*/, juce::MidiBuffer& /*midiMessages*/)
{
    // Double precision not used in this MVP
}

void AudioEngine::processMidiSequencer(juce::MidiBuffer& midiMessages)
{
    // No lock here as it's called from processBlock which already has the lock
    int64_t currentTick = transport.getPosition();

    if (lastProcessedTick == -1)
    {
        lastProcessedTick = currentTick;
        return;
    }

    if (currentTick < lastProcessedTick) // Looped or jumped back
    {
        synth.allNotesOff(0, false);
        lastProcessedTick = currentTick;
    }

    // Play all tracks
    for (const auto& track : project.getTracks())
    {
        // Skip muted tracks
        if (track->isMuted())
            continue;
        
        // Play each clip region in the track
        for (const auto& clipRegion : track->getClipRegions())
        {
            Clip* clip = clipRegion.clip;
            if (!clip)
                continue;
            
            int64_t clipStartTick = clipRegion.startTick;
            int64_t clipOffset = clipRegion.offsetTick;
            int64_t clipLengthTick = clipRegion.lengthTick;
            int64_t clipEndTick = clipStartTick + clipLengthTick;
            
            // Check if we're currently playing this clip region
            if (currentTick < clipStartTick || currentTick >= clipEndTick)
                continue;
            
            // Play notes from this clip
            auto& notes = clip->getNotes();
            for (const auto& note : notes)
            {
                // Convert note time to timeline time (add clip start + offset)
                int64_t noteAbsoluteStart = clipStartTick + (note.startTick - clipOffset);
                int64_t noteAbsoluteEnd = clipStartTick + (note.endTick - clipOffset);
                
                // Check if note is within clip region bounds
                if (noteAbsoluteStart < clipStartTick || noteAbsoluteEnd > clipEndTick)
                    continue;
                
                // Note On
                if (noteAbsoluteStart >= lastProcessedTick && noteAbsoluteStart < currentTick)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, note.pitch, (juce::uint8)note.velocity), 0);
                }
                
                // Note Off
                if (noteAbsoluteEnd >= lastProcessedTick && noteAbsoluteEnd < currentTick)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.pitch), 0);
                }
            }
        }
    }

    lastProcessedTick = currentTick;
}

void AudioEngine::processMidiRecording(const juce::MidiBuffer& midiMessages, int64_t currentTick)
{
    // Only record if a track is armed and we have a recorder
    if (recordArmedTrackIndex < 0 || !midiRecorder || !transport.isPlaying())
        return;
    
    // Pass MIDI to recorder
    midiRecorder->processMidiInput(midiMessages, currentTick);
}

juce::AudioProcessorEditor* AudioEngine::createEditor() { return nullptr; }
bool AudioEngine::hasEditor() const { return false; }

int AudioEngine::getNumPrograms() { return 1; }
int AudioEngine::getCurrentProgram() { return 0; }
void AudioEngine::setCurrentProgram(int) {}
const juce::String AudioEngine::getProgramName(int /*index*/) { return "Default"; }
void AudioEngine::changeProgramName(int /*index*/, const juce::String& /*newName*/) {}

void AudioEngine::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement>("PianoDAWAudioSettings");
    
    if (instrumentNode != nullptr && instrumentNode->getProcessor() != nullptr)
    {
        auto* plugin = instrumentNode->getProcessor();
        xml->setAttribute("pluginDescription", currentPluginDescription.createIdentifierString());
        
        juce::MemoryBlock pluginState;
        plugin->getStateInformation(pluginState);
        xml->setAttribute("pluginState", pluginState.toBase64Encoding());
    }
    
    copyXmlToBinary(*xml, destData);
}

void AudioEngine::setStateInformation(const void* data, int sizeInBytes)
{
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    
    if (xmlState != nullptr && xmlState->hasTagName("PianoDAWAudioSettings"))
    {
        juce::String pluginID = xmlState->getStringAttribute("pluginDescription");
        if (pluginID.isNotEmpty())
        {
            auto desc = knownPluginList.getTypeForIdentifierString(pluginID);
            if (desc != nullptr)
            {
                if (loadPlugin(*desc))
                {
                    auto stateStr = xmlState->getStringAttribute("pluginState");
                    if (stateStr.isNotEmpty())
                    {
                        juce::MemoryBlock pluginState;
                        pluginState.fromBase64Encoding(stateStr);
                        if (instrumentNode != nullptr)
                            instrumentNode->getProcessor()->setStateInformation(pluginState.getData(), (int)pluginState.getSize());
                    }
                }
            }
        }
    }
}

void AudioEngine::setupVoices()
{
    for (int i = 0; i < 32; ++i)
        synth.addVoice(new SimplePianoVoice());

    synth.addSound(new SimplePianoSound());
}

void AudioEngine::updateGraph()
{
    if (mainGraph == nullptr) return;

    mainGraph->clear();

    audioOutNode = mainGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    midiInNode = mainGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));

    // Connections will be made when a plugin is loaded
}

void AudioEngine::scanPlugins()
{
    // Basic standard paths
    juce::FileSearchPath searchPath;
#if JUCE_WINDOWS
    searchPath.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    searchPath.add(juce::File("C:\\Program Files\\VSTPlugins"));
#elif JUCE_MAC
    searchPath.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
#endif

    juce::AudioPluginFormatManager manager;
    manager.addDefaultFormats();

    knownPluginList.clear();
    
    for (auto* format : manager.getFormats())
    {
        juce::PluginDirectoryScanner scanner(knownPluginList, *format, searchPath, true, juce::File());
        juce::String name;
        while (scanner.scanNextFile(true, name)) {}
    }

    savePluginList();
}

void AudioEngine::handleNoteOn(int midiNoteNumber, float velocity)
{
    juce::ScopedLock sl(uiMidiLock);
    uiMidiBuffer.addEvent(juce::MidiMessage::noteOn(1, midiNoteNumber, velocity), 0);
}

void AudioEngine::handleNoteOff(int midiNoteNumber)
{
    juce::ScopedLock sl(uiMidiLock);
    uiMidiBuffer.addEvent(juce::MidiMessage::noteOff(1, midiNoteNumber), 0);
}

bool AudioEngine::loadPlugin(const juce::PluginDescription& description)
{
    juce::String errorMessage;
    auto instance = pluginFormatManager.createPluginInstance(description, getSampleRate(), getBlockSize(), errorMessage);

    if (instance == nullptr)
    {
        DBG("AudioEngine: Failed to load plugin: " + errorMessage);
        return false;
    }

    currentPluginDescription = description;

    if (audioOutNode == nullptr || midiInNode == nullptr)
        updateGraph();

    if (mainGraph == nullptr) return false;

    // Remove old instrument if exists
    if (instrumentNode != nullptr)
        mainGraph->removeNode(instrumentNode);

    instrumentNode = mainGraph->addNode(std::move(instance));

    // Connect MIDI
    mainGraph->addConnection({ { midiInNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex },
                             { instrumentNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });

    // Connect Audio
    for (int i = 0; i < getMainBusNumOutputChannels(); ++i)
    {
        mainGraph->addConnection({ { instrumentNode->nodeID, i },
                                 { audioOutNode->nodeID, i } });
    }

    instrumentNode->getProcessor()->prepareToPlay(getSampleRate(), getBlockSize());

    return true;
}

juce::AudioProcessor* AudioEngine::getCurrentPlugin() const
{
    if (instrumentNode != nullptr)
        return instrumentNode->getProcessor();
    return nullptr;
}

void AudioEngine::showEditor()
{
    // This will be implemented using a separate window class
    // For now, we'll trigger it from VstBrowserPanel or a dedicated button
}

void AudioEngine::savePluginList()
{
    auto file = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                    .getSiblingFile("plugins.xml");
    
    auto xml = knownPluginList.createXml();
    if (xml != nullptr)
        xml->writeTo(file);
}

void AudioEngine::loadPluginList()
{
    auto file = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                    .getSiblingFile("plugins.xml");
    
    if (file.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr)
        {
            knownPluginList.recreateFromXml(*xml);
        }
    }
}

} // namespace pianodaw
