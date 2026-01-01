#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>

namespace pianodaw {

/**
 * LLMClient - Handles communication with LLM APIs (Ollama, OpenAI-compatible)
 * 
 * Provides an asynchronous interface for sending prompts and receiving structured JSON commands.
 */
class LLMClient : private juce::Thread
{
public:
    LLMClient();
    ~LLMClient() override;

    struct Response {
        bool success;
        juce::String text;
        juce::String error;
    };

    enum class Provider {
        Ollama,
        Gemini
    };

    void setProvider(Provider p) { provider = p; }
    void setApiKey(const juce::String& key) { apiKey = key; }

    /** Send a prompt to the LLM asynchronously */
    void sendPrompt(const juce::String& prompt, std::function<void(const Response&)> callback);

    /** Set the API endpoint (default: http://localhost:11434/api/generate for Ollama) */
    void setEndpoint(const juce::String& newEndpoint) { endpoint = newEndpoint; }
    
    /** Set the model name (default: llama3 or similar) */
    void setModel(const juce::String& newModel) { model = newModel; }

    void setSystemPrompt(const juce::String& prompt) { systemPrompt = prompt; }
    juce::String getSystemPrompt() const { return systemPrompt; }

private:
    void run() override;

    Provider provider = Provider::Ollama;
    juce::String apiKey;
    juce::String endpoint = "http://localhost:11434/api/generate";
    juce::String model = "llama3";
    juce::String systemPrompt;

    struct Request {
        juce::String prompt;
        std::function<void(const Response&)> callback;
    };

    juce::CriticalSection lock;
    juce::Array<Request> pendingRequests;

    void processNextRequest();
    
    /** Loads the system prompt from the schema file if possible */
    void loadSystemPrompt();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LLMClient)
};

} // namespace pianodaw
