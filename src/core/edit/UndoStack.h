#pragma once

#include <memory>
#include <vector>
#include <stack>
#include <string>

namespace pianodaw {

/**
 * Command interface for undo/redo
 * 
 * All edit operations should be implemented as commands
 */
class Command
{
public:
    virtual ~Command() = default;
    
    /** Execute the command */
    virtual void execute() = 0;
    
    /** Undo the command */
    virtual void undo() = 0;
    
    /** Get command description for UI */
    virtual std::string getDescription() const = 0;
};

/**
 * Undo/Redo stack manager
 * 
 * Manages command history and provides undo/redo functionality
 */
class UndoStack
{
public:
    UndoStack(size_t maxSize = 100) : maxStackSize(maxSize) {}
    
    /** Execute and push command onto stack */
    void execute(std::unique_ptr<Command> cmd)
    {
        cmd->execute();
        
        // Clear redo stack when new command is executed
        while (!redoStack.empty())
            redoStack.pop();
        
        // Add to undo stack
        undoStack.push(std::move(cmd));
        
        // Limit stack size
        if (undoStack.size() > maxStackSize)
        {
            // Remove oldest command (at bottom of stack)
            std::stack<std::unique_ptr<Command>> temp;
            
            // Move all but the first to temp
            while (undoStack.size() > 1)
            {
                temp.push(std::move(undoStack.top()));
                undoStack.pop();
            }
            
            // Remove oldest
            undoStack.pop();
            
            // Restore stack
            while (!temp.empty())
            {
                undoStack.push(std::move(temp.top()));
                temp.pop();
            }
        }
    }
    
    /** Undo last command */
    bool undo()
    {
        if (undoStack.empty())
            return false;
        
        auto cmd = std::move(undoStack.top());
        undoStack.pop();
        
        cmd->undo();
        
        redoStack.push(std::move(cmd));
        return true;
    }
    
    /** Redo last undone command */
    bool redo()
    {
        if (redoStack.empty())
            return false;
        
        auto cmd = std::move(redoStack.top());
        redoStack.pop();
        
        cmd->execute();
        
        undoStack.push(std::move(cmd));
        return true;
    }
    
    /** Check if undo is available */
    bool canUndo() const { return !undoStack.empty(); }
    
    /** Check if redo is available */
    bool canRedo() const { return !redoStack.empty(); }
    
    /** Get undo description (for menu text) */
    std::string getUndoDescription() const
    {
        if (canUndo())
            return undoStack.top()->getDescription();
        return "";
    }
    
    /** Get redo description (for menu text) */
    std::string getRedoDescription() const
    {
        if (canRedo())
            return redoStack.top()->getDescription();
        return "";
    }
    
    /** Clear all history */
    void clear()
    {
        while (!undoStack.empty()) undoStack.pop();
        while (!redoStack.empty()) redoStack.pop();
    }
    
private:
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;
    size_t maxStackSize;
};

} // namespace pianodaw
