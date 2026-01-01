#include "LLMClient.h"
#include <juce_core/juce_core.h>

namespace pianodaw {

LLMClient::LLMClient() : Thread("LLMClientThread")
{
    loadSystemPrompt();
    startThread();
}

LLMClient::~LLMClient()
{
    stopThread(5000);
}

void LLMClient::sendPrompt(const juce::String& prompt, std::function<void(const Response&)> callback)
{
    juce::ScopedLock sl(lock);
    pendingRequests.add({prompt, callback});
    notify();
}

void LLMClient::run()
{
    while (!threadShouldExit())
    {
        if (wait(500))
        {
            processNextRequest();
        }
    }
}

void LLMClient::processNextRequest()
{
    Request req;
    {
        juce::ScopedLock sl(lock);
        if (pendingRequests.isEmpty()) return;
        req = pendingRequests.removeAndReturn(0);
    }

    juce::String jsonBody;
    juce::String targetUrl = endpoint;

    if (provider == Provider::Gemini)
    {
        // Gemini API Configuration
        juce::DynamicObject::Ptr content = new juce::DynamicObject();
        juce::DynamicObject::Ptr part = new juce::DynamicObject();
        part->setProperty("text", systemPrompt + "\n\nUser: " + req.prompt);
        juce::Array<juce::var> parts; parts.add(juce::var(part));
        content->setProperty("parts", parts);
        juce::Array<juce::var> contents; contents.add(juce::var(content));
        
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("contents", contents);
        jsonBody = juce::JSON::toString(juce::var(root));
        targetUrl = "https://generativelanguage.googleapis.com/v1beta/models/" + model + ":generateContent?key=" + apiKey;
    }
    else
    {
        // Ollama Configuration
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("model", model);
        obj->setProperty("prompt", req.prompt);
        obj->setProperty("system", systemPrompt);
        obj->setProperty("stream", false);
        jsonBody = juce::JSON::toString(juce::var(obj));
    }

    juce::URL url(targetUrl);
    url = url.withPOSTData(jsonBody);

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                    .withExtraHeaders("Content-Type: application/json")
                    .withConnectionTimeoutMs(30000);

    std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));
    
    Response response;
    response.success = false;

    if (stream != nullptr)
    {
        juce::String responseText = stream->readEntireStreamAsString();
        auto json = juce::JSON::parse(responseText);

        if (provider == Provider::Gemini)
        {
            auto candidates = json.getProperty("candidates", juce::var());
            if (candidates.isArray() && candidates.size() > 0)
            {
                auto firstCandidate = candidates[0];
                auto content = firstCandidate.getProperty("content", juce::var());
                auto parts = content.getProperty("parts", juce::var());
                if (parts.isArray() && parts.size() > 0)
                {
                    response.text = parts[0].getProperty("text", "").toString();
                    response.success = true;
                }
            }
            if (!response.success) response.error = "Failed to parse Gemini response: " + responseText;
        }
        else
        {
            if (json.isObject())
            {
                response.text = json.getProperty("response", "").toString();
                response.success = true;
            }
            else response.error = "Invalid JSON response from Ollama";
        }
    }
    else
    {
        response.error = "Connection failed";
    }

    juce::MessageManager::callAsync([req, response]() {
        req.callback(response);
    });
}

void LLMClient::loadSystemPrompt()
{
    // Simplified: Just use a default or empty. Schema loading can be added if needed.
    systemPrompt = "You are a professional music producer and MIDI expert AI. "
                   "Always respond with a single JSON object containing an 'action' field.";
}

} // namespace pianodaw
