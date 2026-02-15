/**
 * @file Command.h
 * @brief Command pattern interface for undo/redo operations
 * 
 * All undoable operations should inherit from Command and implement
 * execute() and undo() methods.
 */

#ifndef DC3D_CORE_COMMAND_H
#define DC3D_CORE_COMMAND_H

#include <QString>
#include <QUndoCommand>
#include <memory>

namespace dc3d {
namespace core {

/**
 * @class Command
 * @brief Abstract base class for undoable commands
 * 
 * Commands encapsulate operations that can be executed and undone.
 * Each command stores enough state to reverse its operation.
 * 
 * Inherits from QUndoCommand for compatibility with Qt's undo framework.
 * 
 * Usage:
 *   auto cmd = new TransformCommand(object, oldTransform, newTransform);
 *   undoStack->push(cmd);  // Executes and takes ownership
 */
class Command : public QUndoCommand
{
public:
    Command(const QString& text = QString(), QUndoCommand* parent = nullptr)
        : QUndoCommand(text, parent) {}
    
    virtual ~Command() = default;
    
    /**
     * @brief Execute the command (called by redo())
     * 
     * Applies the operation to the scene. Should be idempotent
     * (calling multiple times has same effect as calling once).
     */
    virtual void execute() = 0;
    
    /**
     * @brief Undo the command (overrides QUndoCommand::undo)
     * 
     * Reverts the scene to state before execute() was called.
     * Must restore exact previous state.
     */
    void undo() override = 0;
    
    /**
     * @brief Redo the command (overrides QUndoCommand::redo)
     * 
     * Re-applies the command after an undo. Calls execute().
     */
    void redo() override { execute(); }
    
    /**
     * @brief Get estimated memory usage of this command
     * @return Memory in bytes used by this command's stored state
     */
    virtual size_t memoryUsage() const { return sizeof(*this); }
    
    /**
     * @brief Get a human-readable description of the command
     * @return Description for display in Edit menu (e.g., "Undo Import Mesh")
     */
    virtual QString description() const = 0;
    
    /**
     * @brief Check if this command can be merged with another
     * @param other The command to potentially merge with
     * @return true if commands can be merged
     * 
     * Override to enable command compression (e.g., multiple small
     * transform changes merged into one).
     */
    virtual bool canMergeWith(const Command* other) const { 
        Q_UNUSED(other);
        return false; 
    }
    
    /**
     * @brief Merge another command into this one
     * @param other The command to merge
     * @return true if merge was successful
     * 
     * Only called if canMergeWith() returns true.
     */
    virtual bool mergeWith(const Command* other) { 
        Q_UNUSED(other);
        return false; 
    }
    
    // Hide the QUndoCommand mergeWith to avoid confusion
    bool mergeWith(const QUndoCommand* other) override {
        auto cmd = dynamic_cast<const Command*>(other);
        if (cmd && canMergeWith(cmd)) {
            return mergeWith(cmd);
        }
        return false;
    }
};

/// Convenience alias for command pointers
using CommandPtr = std::unique_ptr<Command>;

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_COMMAND_H
