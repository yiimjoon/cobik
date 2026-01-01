#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pianodaw {

/**
 * DebugLogWindow - 디버그 로그를 표시하는 창
 * 
 * 사용법:
 * DebugLogWindow::getInstance()->addLog("메시지");
 */
class DebugLogWindow : public juce::DocumentWindow
{
public:
    static DebugLogWindow* getInstance()
    {
        if (instance == nullptr)
            instance = new DebugLogWindow();
        return instance;
    }
    
    static void addLog(const juce::String& message)
    {
        getInstance()->appendLog(message);
    }
    
    void closeButtonPressed() override
    {
        setVisible(false);
    }
    
private:
    DebugLogWindow()
        : DocumentWindow("Debug Log", 
                         juce::Desktop::getInstance().getDefaultLookAndFeel()
                             .findColour(juce::ResizableWindow::backgroundColourId),
                         DocumentWindow::allButtons)
    {
        logEditor = std::make_unique<juce::TextEditor>();
        logEditor->setMultiLine(true);
        logEditor->setReadOnly(true);
        logEditor->setScrollbarsShown(true);
        logEditor->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
        
        setContentNonOwned(logEditor.get(), true);
        setResizable(true, false);
        centreWithSize(600, 400);
        
        appendLog("=== Debug Log Started ===");
    }
    
    void appendLog(const juce::String& message)
    {
        auto time = juce::Time::getCurrentTime().toString(true, true, true, true);
        juce::String logLine = "[" + time + "] " + message + "\n";
        
        logEditor->moveCaretToEnd();
        logEditor->insertTextAtCaret(logLine);
        logEditor->moveCaretToEnd();
    }
    
    static DebugLogWindow* instance;
    std::unique_ptr<juce::TextEditor> logEditor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugLogWindow)
};

} // namespace pianodaw
