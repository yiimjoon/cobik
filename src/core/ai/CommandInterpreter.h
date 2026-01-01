#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include "../edit/EditCommands.h"
#include "../model/Clip.h"

namespace pianodaw {

/**
 * CommandInterpreter - Parses LLM JSON output and creates executable Commands
 */
class CommandInterpreter
{
public:
    CommandInterpreter(Clip& clip);
    ~CommandInterpreter();

    struct InterpretationResult {
        std::vector<std::unique_ptr<Command>> commands;
        juce::String reasoning;
    };

    /** 
     * Parses the LLM response text and returns a list of commands and reasoning.
     */
    InterpretationResult interpret(const juce::String& jsonText);

private:
    Clip& clip;

    std::unique_ptr<Command> createQuantizeCommand(const juce::var& obj);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandInterpreter)
};

} // namespace pianodaw
