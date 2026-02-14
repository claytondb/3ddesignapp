/**
 * @file CommandStack.cpp
 * @brief Implementation of CommandStack for undo/redo management
 * 
 * Thread Safety Note: CommandStack is designed to be accessed from the main
 * (UI) thread only. All signal/slot connections should use Qt::AutoConnection
 * to ensure thread safety when signals cross thread boundaries.
 */

#include "CommandStack.h"

#include <algorithm>

namespace dc3d {
namespace core {

// ============================================================================
// MacroCommand - Internal class for grouping commands
// ============================================================================

class MacroCommand : public Command
{
public:
    MacroCommand(const QString& description, std::vector<CommandPtr> commands)
        : m_description(description)
        , m_commands(std::move(commands))
    {
    }
    
    void execute() override
    {
        for (auto& cmd : m_commands) {
            cmd->execute();
        }
    }
    
    void undo() override
    {
        // Undo in reverse order
        for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
            (*it)->undo();
        }
    }
    
    QString description() const override { return m_description; }
    
private:
    QString m_description;
    std::vector<CommandPtr> m_commands;
};

// ============================================================================
// CommandStack
// ============================================================================

CommandStack::CommandStack(QObject* parent, size_t maxSize)
    : QObject(parent)
    , m_maxSize(maxSize)
    , m_cleanIndex(-1)
    , m_mergingEnabled(true)
    , m_macroDepth(0)
{
}

CommandStack::~CommandStack() = default;

void CommandStack::push(CommandPtr command)
{
    if (!command) {
        return;
    }
    
    // If recording a macro, add to macro commands instead
    if (m_macroDepth > 0) {
        command->execute();
        m_macroCommands.push_back(std::move(command));
        return;
    }
    
    // Save old state for signal emission
    bool oldCanUndo = canUndo();
    bool oldCanRedo = canRedo();
    QString oldUndoText = undoText();
    QString oldRedoText = redoText();
    
    // Try to merge with previous command if merging is enabled
    if (m_mergingEnabled && !m_undoStack.empty()) {
        if (m_undoStack.back()->canMergeWith(command.get())) {
            if (m_undoStack.back()->mergeWith(command.get())) {
                // Merged successfully - DO NOT re-execute!
                // The new command has already been executed externally before being pushed.
                // The merge only updates the stored state to combine both transformations.
                emitStateChanges(oldCanUndo, oldCanRedo, oldUndoText, oldRedoText);
                return;
            }
        }
    }
    
    // Execute the command
    command->execute();
    
    // Clear redo stack (new action invalidates redo history)
    if (!m_redoStack.empty()) {
        m_redoStack.clear();
        
        // If clean index was in redo stack, invalidate it
        if (m_cleanIndex > static_cast<int>(m_undoStack.size())) {
            int oldCleanIndex = m_cleanIndex;
            m_cleanIndex = -1;
            if (oldCleanIndex >= 0) {
                emit cleanChanged(false);
            }
        }
    }
    
    // Add to undo stack
    m_undoStack.push_back(std::move(command));
    
    // Enforce max size
    enforceMaxSize();
    
    emitStateChanges(oldCanUndo, oldCanRedo, oldUndoText, oldRedoText);
}

void CommandStack::undo()
{
    if (!canUndo()) {
        return;
    }
    
    // Save old state
    bool oldCanUndo = canUndo();
    bool oldCanRedo = canRedo();
    QString oldUndoText = undoText();
    QString oldRedoText = redoText();
    bool oldClean = isClean();
    
    // Pop from undo stack
    CommandPtr command = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    
    // Undo the command
    command->undo();
    
    // Push to redo stack
    m_redoStack.push_back(std::move(command));
    
    emitStateChanges(oldCanUndo, oldCanRedo, oldUndoText, oldRedoText);
    
    if (isClean() != oldClean) {
        emit cleanChanged(isClean());
    }
}

void CommandStack::redo()
{
    if (!canRedo()) {
        return;
    }
    
    // Save old state
    bool oldCanUndo = canUndo();
    bool oldCanRedo = canRedo();
    QString oldUndoText = undoText();
    QString oldRedoText = redoText();
    bool oldClean = isClean();
    
    // Pop from redo stack
    CommandPtr command = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    
    // Re-execute the command
    command->execute();
    
    // Push to undo stack
    m_undoStack.push_back(std::move(command));
    
    emitStateChanges(oldCanUndo, oldCanRedo, oldUndoText, oldRedoText);
    
    if (isClean() != oldClean) {
        emit cleanChanged(isClean());
    }
}

bool CommandStack::canUndo() const
{
    return !m_undoStack.empty() && m_macroDepth == 0;
}

bool CommandStack::canRedo() const
{
    return !m_redoStack.empty() && m_macroDepth == 0;
}

QString CommandStack::undoText() const
{
    if (m_undoStack.empty()) {
        return QString();
    }
    return m_undoStack.back()->description();
}

QString CommandStack::redoText() const
{
    if (m_redoStack.empty()) {
        return QString();
    }
    return m_redoStack.back()->description();
}

size_t CommandStack::undoCount() const
{
    return m_undoStack.size();
}

size_t CommandStack::redoCount() const
{
    return m_redoStack.size();
}

void CommandStack::clear()
{
    if (m_undoStack.empty() && m_redoStack.empty()) {
        return;
    }
    
    bool oldCanUndo = canUndo();
    bool oldCanRedo = canRedo();
    QString oldUndoText = undoText();
    QString oldRedoText = redoText();
    bool oldClean = isClean();
    
    m_undoStack.clear();
    m_redoStack.clear();
    m_cleanIndex = -1;
    
    emitStateChanges(oldCanUndo, oldCanRedo, oldUndoText, oldRedoText);
    
    // After clear, isClean() returns false (since -1 != 0)
    // Emit cleanChanged with the actual current state
    bool nowClean = isClean();
    if (oldClean != nowClean) {
        emit cleanChanged(nowClean);
    }
}

void CommandStack::setMaxSize(size_t maxSize)
{
    m_maxSize = maxSize;
    enforceMaxSize();
}

bool CommandStack::isClean() const
{
    return m_cleanIndex == static_cast<int>(m_undoStack.size());
}

void CommandStack::setClean()
{
    bool wasClean = isClean();
    m_cleanIndex = static_cast<int>(m_undoStack.size());
    if (!wasClean) {
        emit cleanChanged(true);
    }
}

void CommandStack::beginMacro(const QString& description)
{
    if (m_macroDepth == 0) {
        m_macroCommands.clear();
        m_macroDescription = description;
    }
    ++m_macroDepth;
}

void CommandStack::endMacro()
{
    if (m_macroDepth == 0) {
        return;  // Not in a macro
    }
    
    --m_macroDepth;
    
    if (m_macroDepth == 0 && !m_macroCommands.empty()) {
        // Create a macro command from collected commands
        // Note: Commands have already been executed, so we create a macro
        // that just tracks them for undo/redo
        auto macro = std::make_unique<MacroCommand>(
            m_macroDescription, 
            std::move(m_macroCommands)
        );
        
        // Don't execute (already done), just add to stack
        bool oldCanUndo = canUndo();
        bool oldCanRedo = canRedo();
        QString oldUndoText = undoText();
        QString oldRedoText = redoText();
        
        // Clear redo stack
        if (!m_redoStack.empty()) {
            m_redoStack.clear();
            if (m_cleanIndex > static_cast<int>(m_undoStack.size())) {
                m_cleanIndex = -1;
                emit cleanChanged(false);
            }
        }
        
        m_undoStack.push_back(std::move(macro));
        enforceMaxSize();
        
        emitStateChanges(oldCanUndo, oldCanRedo, oldUndoText, oldRedoText);
    }
}

void CommandStack::enforceMaxSize()
{
    if (m_maxSize == 0) {
        return;  // Unlimited
    }
    
    int discardedCount = 0;
    
    while (m_undoStack.size() > m_maxSize) {
        m_undoStack.erase(m_undoStack.begin());
        ++discardedCount;
        
        // Adjust clean index
        if (m_cleanIndex > 0) {
            --m_cleanIndex;
        } else if (m_cleanIndex == 0) {
            m_cleanIndex = -1;  // Clean state removed
            emit cleanChanged(false);
        }
    }
    
    // Emit warning if commands were discarded
    if (discardedCount > 0) {
        emit commandsDiscarded(discardedCount);
    }
    
    // Emit warning if stack is near capacity (90%+)
    if (m_undoStack.size() >= m_maxSize * 0.9) {
        emit stackNearLimit(m_undoStack.size(), m_maxSize);
    }
}

void CommandStack::emitStateChanges(bool oldCanUndo, bool oldCanRedo,
                                     const QString& oldUndoText, const QString& oldRedoText)
{
    bool newCanUndo = canUndo();
    bool newCanRedo = canRedo();
    QString newUndoText = undoText();
    QString newRedoText = redoText();
    
    if (oldCanUndo != newCanUndo) {
        emit canUndoChanged(newCanUndo);
    }
    if (oldCanRedo != newCanRedo) {
        emit canRedoChanged(newCanRedo);
    }
    if (oldUndoText != newUndoText) {
        emit undoTextChanged(newUndoText);
    }
    if (oldRedoText != newRedoText) {
        emit redoTextChanged(newRedoText);
    }
    
    emit stackChanged();
}

} // namespace core
} // namespace dc3d
