#include "CommandInterpreter.h"
#include "AIGenerator.h"
#include "../../core/timeline/PPQ.h"
#include <algorithm>

namespace pianodaw {

CommandInterpreter::CommandInterpreter(Clip& clip_) : clip(clip_) {}
CommandInterpreter::~CommandInterpreter() {}

CommandInterpreter::InterpretationResult CommandInterpreter::interpret(const juce::String& jsonText)
{
    InterpretationResult result;
    
    juce::String cleanJson = jsonText.trim();
    if (cleanJson.startsWith("```json")) {
        cleanJson = cleanJson.fromFirstOccurrenceOf("```json", false, false);
        cleanJson = cleanJson.upToLastOccurrenceOf("```", false, false).trim();
    }

    auto json = juce::JSON::parse(cleanJson);
    if (!json.isObject()) return result;

    result.reasoning = json.getProperty("reasoning", "").toString();
    juce::String action = json.getProperty("action", "").toString();

    if (action == "smart_quantize")
    {
        auto cmd = createQuantizeCommand(json);
        if (cmd) result.commands.push_back(std::move(cmd));
    }
    else if (action == "generate_progression" || action == "generate_melody")
    {
        auto comp = json.getProperty("composition", juce::var());
        if (comp.isObject())
        {
            std::vector<Note> notesToAdd;

            // 1. Direct MIDI notes if provided
            auto notesArr = comp.getProperty("notes", juce::var());
            if (notesArr.isArray())
            {
                for (int i = 0; i < notesArr.size(); ++i)
                {
                    auto nObj = notesArr[i];
                    Note n;
                    n.pitch = (int)nObj.getProperty("pitch", 60);
                    n.startTick = (int64_t)nObj.getProperty("startTick", 0);
                    n.endTick = n.startTick + (int64_t)nObj.getProperty("duration", 480);
                    n.velocity = (float)nObj.getProperty("velocity", 0.8f);
                    notesToAdd.push_back(n);
                }
            }
            // 2. High-level progression as fallback
            else
            {
                auto prog = comp.getProperty("progression", juce::var());
                if (prog.isArray())
                {
                    int64_t start = (int64_t)json.getProperty("selection", juce::var()).getProperty("startTick", 0);
                    notesToAdd = AIGenerator::generateNotesFromProgression(*prog.getArray(), start, 960 * 2); // 2 beats per chord
                }
            }

            if (!notesToAdd.empty())
                result.commands.push_back(std::make_unique<BulkAddNotesCommand>(clip, notesToAdd));
        }
    }

    return result;
}

std::unique_ptr<Command> CommandInterpreter::createQuantizeCommand(const juce::var& obj)
{
    QuantizeParams params;
    
    juce::String gridStr = obj.getProperty("grid", "1/16").toString();
    if (gridStr == "1/4") params.gridTicks = 960;
    else if (gridStr == "1/8") params.gridTicks = 480;
    else if (gridStr == "1/16") params.gridTicks = 240;
    else if (gridStr == "1/32") params.gridTicks = 120;
    else params.gridTicks = 240;

    params.strength = (float)obj.getProperty("qStrength", 100) / 100.0f;
    params.swing = (float)obj.getProperty("swing", 50) / 100.0f;

    juce::String mode = obj.getProperty("mode", "auto").toString();
    params.smartChord = (mode == "preserve_roll" || mode == "auto" || mode == "tight_chords");
    
    std::vector<int> noteIds;
    auto sel = obj.getProperty("selection", juce::var());
    if (sel.isObject())
    {
        juce::String scope = sel.getProperty("scope", "all").toString();
        if (scope == "time_range")
        {
            int64_t start = (int64_t)sel.getProperty("startTick", 0);
            int64_t end = (int64_t)sel.getProperty("endTick", 960 * 4);
            
            juce::ScopedLock sl(clip.getLock());
            for (const auto& n : clip.getNotes())
            {
                if (n.startTick >= start && n.startTick < end)
                    noteIds.push_back(n.id);
            }
        }
        else
        {
            juce::ScopedLock sl(clip.getLock());
            for (const auto& n : clip.getNotes()) noteIds.push_back(n.id);
        }
    }

    if (noteIds.empty()) return nullptr;

    return std::make_unique<QuantizeCommand>(clip, noteIds, params);
}

} // namespace pianodaw
