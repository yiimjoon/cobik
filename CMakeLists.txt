cmake_minimum_required(VERSION 3.15)

project(PianoDAW VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE framework
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

# Main application target
juce_add_gui_app(PianoDAW
    PRODUCT_NAME "PianoDAW"
    COMPANY_NAME "Cobik"
)

# Source files
target_sources(PianoDAW PRIVATE
    src/app/Main.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/app/AppState.h
    src/app/AppState.cpp
    src/core/model/Note.h
    src/core/model/Clip.h
    src/core/model/Clip.cpp
    src/core/model/CC.h
    src/core/model/Track.h
    src/core/model/Project.h
    src/core/model/Project.cpp
    src/core/edit/UndoStack.h
    src/core/edit/EditCommands.h
    src/core/timeline/PPQ.h
    src/core/timeline/Timeline.h
    src/core/timeline/Transport.h
    src/core/timeline/Transport.cpp
    src/core/quantize/QuantizeEngine.h
    src/core/quantize/QuantizeEngine.cpp
    src/core/ai/AIGenerator.h
    src/core/ai/AIGenerator.cpp
    src/core/ai/LLMClient.h
    src/core/ai/LLMClient.cpp
    src/core/ai/CommandInterpreter.h
    src/core/ai/CommandInterpreter.cpp
    src/core/ai/MusicTheory.h
    src/core/audio/AudioEngine.h
    src/core/audio/AudioEngine.cpp
    src/ui/pianoroll/PianoRollView.h
    src/ui/pianoroll/PianoRollView.cpp
    src/ui/pianoroll/VelocityLane.h
    src/ui/pianoroll/VelocityLane.cpp
    src/ui/pianoroll/PedalLane.h
    src/ui/pianoroll/PedalLane.cpp
    src/ui/arrangement/ArrangementView.h
    src/ui/arrangement/ArrangementView.cpp
    src/ui/arrangement/TrackListPanel.h
    src/ui/arrangement/TrackListPanel.cpp
    src/ui/transport/TransportBar.h
    src/ui/transport/TransportBar.cpp
    src/ui/MainComponent.h
    src/ui/MainComponent.cpp
    src/ui/panels/VstBrowserPanel.h
    src/ui/panels/VstBrowserPanel.cpp
    src/ui/panels/QuantizePanel.h
    src/ui/panels/QuantizePanel.cpp
    src/ui/panels/PromptBar.h
    src/ui/panels/PromptBar.cpp
    src/ui/panels/PluginEditorWindow.h
    src/ui/panels/TheoryToolbar.h
    src/ui/editor/InfoLine.h
    src/ui/editor/StatusLine.h
    src/ui/editor/PianoRollToolbar.h
    src/ui/editor/InspectorPanel.h
)

# JUCE modules
target_compile_definitions(PianoDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:PianoDAW,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:PianoDAW,JUCE_VERSION>"
)

target_link_libraries(PianoDAW PRIVATE
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_audio_formats
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Include directories
target_include_directories(PianoDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enable testing
enable_testing()
add_subdirectory(tests)
