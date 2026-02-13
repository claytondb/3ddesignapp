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
 * Usage:
 *   auto cmd = std::make_unique<TransformCommand>(object, oldTransform, newTransform);
 *   cmd->execute();  // Apply the transformation
 *   cmd->undo();     // Revert to old transformation
 */
class Command
{
public:
    virtual ~Command() = default;
    
    /**
     * @brief Execute the command
     * 
     * Applies the operation to the scene. Should be idempotent
     * (calling multiple times has same effect as calling once).
     */
    virtual void execute() = 0;
    
    /**
     * @brief Undo the command
     * 
     * Reverts the scene to state before execute() was called.
     * Must restore exact previous state.
     */
    virtual void undo() = 0;
    
    /**
     * @brief Redo the command
     * 
     * Re-applies the command after an undo. Default calls execute().
     */
    virtual void redo() { execute(); }
    
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
};

/// Convenience alias for command pointers
using CommandPtr = std::unique_ptr<Command>;

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_COMMAND_H
