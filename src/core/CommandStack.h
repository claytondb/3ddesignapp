/**
 * @file CommandStack.h
 * @brief Undo/redo stack for command management
 * 
 * Maintains history of executed commands and provides undo/redo capability.
 */

#ifndef DC3D_CORE_COMMANDSTACK_H
#define DC3D_CORE_COMMANDSTACK_H

#include "Command.h"

#include <QObject>
#include <vector>
#include <memory>

namespace dc3d {
namespace core {

/**
 * @class CommandStack
 * @brief Manages undo/redo history for commands
 * 
 * Features:
 * - Configurable maximum stack size (default 100)
 * - Automatic clearing of redo stack on new command
 * - Signals for UI updates (canUndo/canRedo changes)
 * - Command merging support for incremental edits
 * 
 * Usage:
 *   CommandStack stack;
 *   stack.push(std::make_unique<MyCommand>());  // Executes and adds to history
 *   stack.undo();  // Reverts last command
 *   stack.redo();  // Re-applies undone command
 */
class CommandStack : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)

public:
    /**
     * @brief Construct a command stack
     * @param parent Parent QObject
     * @param maxSize Maximum number of commands to keep (0 = unlimited)
     */
    explicit CommandStack(QObject* parent = nullptr, size_t maxSize = 100);
    ~CommandStack() override;
    
    // Prevent copying
    CommandStack(const CommandStack&) = delete;
    CommandStack& operator=(const CommandStack&) = delete;
    
    // ===================
    // Command Operations
    // ===================
    
    /**
     * @brief Execute a command and push it onto the undo stack
     * @param command The command to execute (ownership transferred)
     * 
     * The command's execute() method is called, then it's added to history.
     * Any pending redo commands are cleared.
     */
    void push(CommandPtr command);
    
    /**
     * @brief Undo the last command
     * 
     * Pops from undo stack, calls undo(), pushes to redo stack.
     * Does nothing if undo stack is empty.
     */
    void undo();
    
    /**
     * @brief Redo the last undone command
     * 
     * Pops from redo stack, calls execute(), pushes to undo stack.
     * Does nothing if redo stack is empty.
     */
    void redo();
    
    // ===================
    // State Queries
    // ===================
    
    /**
     * @brief Check if undo is available
     */
    bool canUndo() const;
    
    /**
     * @brief Check if redo is available
     */
    bool canRedo() const;
    
    /**
     * @brief Get description of command that would be undone
     * @return Command description or empty string if no undo available
     */
    QString undoText() const;
    
    /**
     * @brief Get description of command that would be redone
     * @return Command description or empty string if no redo available
     */
    QString redoText() const;
    
    /**
     * @brief Get number of commands in undo stack
     */
    size_t undoCount() const;
    
    /**
     * @brief Get number of commands in redo stack
     */
    size_t redoCount() const;
    
    // ===================
    // Stack Management
    // ===================
    
    /**
     * @brief Clear all undo/redo history
     */
    void clear();
    
    /**
     * @brief Set the maximum stack size
     * @param maxSize Maximum commands to keep (0 = unlimited)
     * 
     * If current size exceeds new max, oldest commands are removed.
     */
    void setMaxSize(size_t maxSize);
    
    /**
     * @brief Get current maximum stack size
     */
    size_t maxSize() const { return m_maxSize; }
    
    /**
     * @brief Check if the document is in a clean (saved) state
     */
    bool isClean() const;
    
    /**
     * @brief Mark the current state as clean (e.g., after save)
     */
    void setClean();
    
    /**
     * @brief Enable or disable command merging
     * @param enabled true to enable merging consecutive similar commands
     */
    void setMergingEnabled(bool enabled) { m_mergingEnabled = enabled; }
    
    /**
     * @brief Check if command merging is enabled
     */
    bool isMergingEnabled() const { return m_mergingEnabled; }
    
    /**
     * @brief Begin a macro (group of commands as single undo)
     * @param description Description for the grouped command
     */
    void beginMacro(const QString& description);
    
    /**
     * @brief End the current macro
     */
    void endMacro();
    
    /**
     * @brief Check if currently recording a macro
     */
    bool isInMacro() const { return m_macroDepth > 0; }

signals:
    /**
     * @brief Emitted when canUndo() changes
     */
    void canUndoChanged(bool canUndo);
    
    /**
     * @brief Emitted when canRedo() changes
     */
    void canRedoChanged(bool canRedo);
    
    /**
     * @brief Emitted when the clean state changes
     */
    void cleanChanged(bool clean);
    
    /**
     * @brief Emitted whenever the stack changes
     */
    void stackChanged();
    
    /**
     * @brief Emitted when undo text changes
     */
    void undoTextChanged(const QString& text);
    
    /**
     * @brief Emitted when redo text changes
     */
    void redoTextChanged(const QString& text);
    
    /**
     * @brief Emitted when oldest commands are discarded due to stack limit
     * @param discardedCount Number of commands that were discarded
     * 
     * Applications can show a warning to users when this happens.
     */
    void commandsDiscarded(int discardedCount);
    
    /**
     * @brief Emitted when stack is near capacity (90%+)
     * @param usedCount Current number of commands
     * @param maxCount Maximum allowed commands
     */
    void stackNearLimit(size_t usedCount, size_t maxCount);

private:
    void enforceMaxSize();
    void emitStateChanges(bool oldCanUndo, bool oldCanRedo, 
                          const QString& oldUndoText, const QString& oldRedoText);
    
    std::vector<CommandPtr> m_undoStack;
    std::vector<CommandPtr> m_redoStack;
    
    size_t m_maxSize;
    int m_cleanIndex;  // Index in undo stack at last save (-1 if never clean or invalidated)
    bool m_mergingEnabled;
    int m_macroDepth;
    
    // Macro support - stores commands during macro recording
    std::vector<CommandPtr> m_macroCommands;
    QString m_macroDescription;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_COMMANDSTACK_H
