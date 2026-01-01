#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cstdint>

namespace pianodaw {

class Clip;
class Project;
class Transport;
class MidiRecorder;

/**
 * AudioEngine - Main audio processing unit
 * Standard JUCE AudioProcessor implementation
 * 
 * Supports:
 * - Multi-track playback from Project
 * - MIDI recording via MidiRecorder
 * - VST3 instrument hosting per track (future)
 */
class AudioEngine : public juce::AudioProcessor
{
public:
    AudioEngine(Project& project, Transport& transport);
    ~AudioEngine() override;

    // AudioProcessor overrides
    // ==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Recording
    MidiRecorder& getMidiRecorder() { return *midiRecorder; }
    void setRecordArmedTrack(int trackIndex) { recordArmedTrackIndex = trackIndex; }
    int getRecordArmedTrack() const { return recordArmedTrackIndex; }
    
    // MIDI input from hardware
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message);
    
private:
    Project& project;
    Transport& transport;
    juce::Synthesiser synth;
    
    int64_t lastProcessedTick = -1;
    
    // Recording
    std::unique_ptr<MidiRecorder> midiRecorder;
    int recordArmedTrackIndex = -1;  // -1 = no track armed
    
    // Hardware MIDI input buffer
    juce::MidiBuffer hardwareMidiBuffer;
    juce::CriticalSection hardwareMidiLock;
    
    void setupVoices();
    void processMidiSequencer(juce::MidiBuffer& midiMessages);
    void processMidiRecording(const juce::MidiBuffer& midiMessages, int64_t currentTick);
    
    // VST Hosting
    juce::AudioPluginFormatManager pluginFormatManager;
    std::unique_ptr<juce::AudioProcessorGraph> mainGraph;
    juce::KnownPluginList knownPluginList;
    
    juce::AudioProcessorGraph::Node::Ptr audioInNode;
    juce::AudioProcessorGraph::Node::Ptr audioOutNode;
    juce::AudioProcessorGraph::Node::Ptr midiInNode;
    juce::AudioProcessorGraph::Node::Ptr instrumentNode;
    juce::MidiMessage currentNoteOn;
    juce::MidiBuffer uiMidiBuffer;
    juce::CriticalSection uiMidiLock;
    juce::PluginDescription currentPluginDescription;

    void updateGraph();

public:
    void scanPlugins();
    void showEditor();
    const juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }
    bool loadPlugin(const juce::PluginDescription& description);
    juce::AudioProcessor* getCurrentPlugin() const;

    void handleNoteOn(int midiNoteNumber, float velocity);
    void handleNoteOff(int midiNoteNumber);

    void savePluginList();
    void loadPluginList();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

} // namespace pianodaw
