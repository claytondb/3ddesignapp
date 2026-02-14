/**
 * @file MeshEditCommand.h
 * @brief Command pattern implementation for mesh editing operations
 * 
 * Provides undoable commands for mesh modifications including decimation,
 * smoothing, repair, subdivision, and generic mesh transformations.
 */

#pragma once

#include "../Command.h"
#include "../../geometry/MeshData.h"
#include "../../geometry/MeshDecimation.h"
#include "../../geometry/MeshSmoothing.h"
#include "../../geometry/MeshRepair.h"
#include "../../geometry/MeshSubdivision.h"

#include <memory>
#include <string>
#include <functional>
#include <variant>
#include <chrono>

namespace dc3d {
namespace core {

// Forward declarations
class Document;
class SceneObject;

// Use CommandPtr from Command.h
using CommandPtr = std::unique_ptr<Command>;

/**
 * @brief Extended interface for mesh edit commands with additional capabilities
 */
class MeshCommand : public Command {
public:
    virtual ~MeshCommand() = default;
    
    /// Redo the command (default: call execute)
    virtual void redo() { execute(); }
    
    /// Get command category (for grouping in history)
    virtual QString category() const { return QStringLiteral("Edit"); }
    
    /// Get estimated memory usage of stored data
    virtual size_t memoryUsage() const { return sizeof(*this); }
    
    /// Get timestamp when command was created
    std::chrono::steady_clock::time_point timestamp() const { return timestamp_; }
    
protected:
    MeshCommand() : timestamp_(std::chrono::steady_clock::now()) {}
    
private:
    std::chrono::steady_clock::time_point timestamp_;
};

/**
 * @brief Generic mesh edit command that stores before/after mesh state
 * 
 * This command stores a complete copy of the mesh before and after
 * the operation, enabling full undo/redo capability.
 * 
 * Usage:
 * @code
 *     // Create command with mesh reference and operation
 *     auto cmd = MeshEditCommand::create(
 *         meshRef,
 *         "Decimate",
 *         [](MeshData& mesh) {
 *             auto result = MeshDecimator::decimate(mesh, 0.5f);
 *             if (result.ok()) {
 *                 mesh = std::move(*result.value);
 *                 return true;
 *             }
 *             return false;
 *         });
 *     
 *     // Execute and add to undo stack
 *     if (cmd->execute()) {
 *         undoStack.push(std::move(cmd));
 *     }
 * @endcode
 */
class MeshEditCommand : public MeshCommand {
public:
    using EditFunction = std::function<bool(geometry::MeshData&)>;
    
    /**
     * @brief Create a mesh edit command
     * @param mesh Reference to mesh to edit
     * @param name Description of the operation
     * @param operation Function that performs the edit
     * @return Command pointer
     */
    static CommandPtr create(
        geometry::MeshData& mesh,
        const QString& name,
        EditFunction operation);
    
    /**
     * @brief Create a mesh edit command with object reference
     * @param object Scene object containing mesh
     * @param name Description of the operation
     * @param operation Function that performs the edit
     * @return Command pointer
     */
    static CommandPtr create(
        SceneObject& object,
        const QString& name,
        EditFunction operation);
    
    void execute() override;
    void undo() override;
    void redo() override;
    
    QString description() const override { return name_; }
    QString category() const override { return QStringLiteral("Mesh Edit"); }
    
    size_t memoryUsage() const override;
    
    /// Get the mesh before the operation
    const geometry::MeshData& beforeMesh() const { return beforeMesh_; }
    
    /// Get the mesh after the operation
    const geometry::MeshData& afterMesh() const { return afterMesh_; }
    
private:
    MeshEditCommand(
        geometry::MeshData& mesh,
        const QString& name,
        EditFunction operation);
    
    geometry::MeshData& mesh_;
    QString name_;
    EditFunction operation_;
    
    geometry::MeshData beforeMesh_;
    geometry::MeshData afterMesh_;
    bool executed_ = false;
};

// ============================================================================
// Specialized Mesh Edit Commands
// ============================================================================

/**
 * @brief Command for mesh decimation
 */
class DecimateCommand : public MeshCommand {
public:
    DecimateCommand(
        geometry::MeshData& mesh,
        const geometry::DecimationOptions& options);
    
    void execute() override;
    void undo() override;
    
    QString description() const override;
    QString category() const override { return QStringLiteral("Mesh Edit"); }
    size_t memoryUsage() const override;
    
    /// Get decimation result statistics
    const geometry::DecimationResult& result() const { return result_; }
    
    /// Get execution time in milliseconds
    int64_t executionTimeMs() const { return executionTimeMs_; }
    
private:
    geometry::MeshData& mesh_;
    geometry::DecimationOptions options_;
    geometry::MeshData beforeMesh_;
    geometry::DecimationResult result_;
    int64_t executionTimeMs_ = 0;
};

/**
 * @brief Command for mesh smoothing
 */
class SmoothCommand : public MeshCommand {
public:
    SmoothCommand(
        geometry::MeshData& mesh,
        const geometry::SmoothingOptions& options);
    
    void execute() override;
    void undo() override;
    
    QString description() const override;
    QString category() const override { return QStringLiteral("Mesh Edit"); }
    size_t memoryUsage() const override;
    
    bool canMergeWith(const Command* other) const override;
    bool mergeWith(const Command* other) override;
    
    /// Get smoothing result statistics
    const geometry::SmoothingResult& result() const { return result_; }
    
    /// Get execution time in milliseconds
    int64_t executionTimeMs() const { return executionTimeMs_; }
    
private:
    geometry::MeshData& mesh_;
    geometry::SmoothingOptions options_;
    geometry::MeshData beforeMesh_;
    geometry::SmoothingResult result_;
    int64_t executionTimeMs_ = 0;
};

/**
 * @brief Command for mesh repair operations
 */
class RepairCommand : public MeshCommand {
public:
    enum class Operation {
        RemoveOutliers,
        FillHoles,
        RemoveDuplicates,
        RemoveDegenerates,
        MakeManifold,
        RepairAll
    };
    
    RepairCommand(
        geometry::MeshData& mesh,
        Operation operation,
        float parameter = 0.0f);
    
    void execute() override;
    void undo() override;
    
    QString description() const override;
    QString category() const override { return QStringLiteral("Mesh Repair"); }
    size_t memoryUsage() const override;
    
    /// Get repair result
    const geometry::RepairResult& result() const { return result_; }
    
    /// Get execution time in milliseconds
    int64_t executionTimeMs() const { return executionTimeMs_; }
    
private:
    geometry::MeshData& mesh_;
    Operation operation_;
    float parameter_;
    geometry::MeshData beforeMesh_;
    geometry::RepairResult result_;
    int64_t executionTimeMs_ = 0;
};

/**
 * @brief Command for mesh subdivision
 */
class SubdivideCommand : public MeshCommand {
public:
    SubdivideCommand(
        geometry::MeshData& mesh,
        const geometry::SubdivisionOptions& options);
    
    void execute() override;
    void undo() override;
    
    QString description() const override;
    QString category() const override { return QStringLiteral("Mesh Edit"); }
    size_t memoryUsage() const override;
    
    /// Get subdivision result
    const geometry::SubdivisionResult& result() const { return result_; }
    
    /// Get execution time in milliseconds
    int64_t executionTimeMs() const { return executionTimeMs_; }
    
private:
    geometry::MeshData& mesh_;
    geometry::SubdivisionOptions options_;
    geometry::MeshData beforeMesh_;
    geometry::SubdivisionResult result_;
    int64_t executionTimeMs_ = 0;
};

// ============================================================================
// Compound Commands
// ============================================================================

/**
 * @brief Command that combines multiple commands into one undoable action
 */
class CompoundCommand : public MeshCommand {
public:
    explicit CompoundCommand(const QString& name);
    
    /// Add a command to the compound
    void addCommand(CommandPtr cmd);
    
    /// Get number of sub-commands
    size_t commandCount() const { return commands_.size(); }
    
    void execute() override;
    void undo() override;
    void redo() override;
    
    QString description() const override { return name_; }
    QString category() const override { return QStringLiteral("Compound"); }
    size_t memoryUsage() const override;
    
private:
    QString name_;
    std::vector<CommandPtr> commands_;
    size_t executedCount_ = 0;
};

// ============================================================================
// Command History Manager
// ============================================================================

/**
 * @brief Manages command history for undo/redo
 */
class CommandHistory {
public:
    CommandHistory(size_t maxMemoryBytes = 100 * 1024 * 1024);  // 100MB default
    
    /// Execute and record a command
    bool execute(CommandPtr cmd);
    
    /// Undo the last command
    bool undo();
    
    /// Redo the last undone command
    bool redo();
    
    /// Check if undo is available
    bool canUndo() const { return !undoStack_.empty(); }
    
    /// Check if redo is available
    bool canRedo() const { return !redoStack_.empty(); }
    
    /// Get description of command to undo
    QString undoDescription() const;
    
    /// Get description of command to redo
    QString redoDescription() const;
    
    /// Clear all history
    void clear();
    
    /// Get current memory usage
    size_t memoryUsage() const;
    
    /// Get maximum memory limit
    size_t maxMemory() const { return maxMemoryBytes_; }
    
    /// Set maximum memory limit
    void setMaxMemory(size_t bytes);
    
    /// Get number of undoable commands
    size_t undoCount() const { return undoStack_.size(); }
    
    /// Get number of redoable commands
    size_t redoCount() const { return redoStack_.size(); }
    
    /// Set callback for history changes
    using HistoryChangedCallback = std::function<void()>;
    void setHistoryChangedCallback(HistoryChangedCallback callback) {
        historyChangedCallback_ = callback;
    }
    
private:
    std::vector<CommandPtr> undoStack_;
    std::vector<CommandPtr> redoStack_;
    size_t maxMemoryBytes_;
    size_t currentMemoryUsage_ = 0;
    HistoryChangedCallback historyChangedCallback_;
    
    void trimToMemoryLimit();
    void notifyHistoryChanged();
};

// ============================================================================
// Helper Functions for Creating Commands
// ============================================================================

namespace commands {

/**
 * @brief Create a decimate command
 */
inline CommandPtr decimate(
    geometry::MeshData& mesh,
    float targetRatio,
    bool preserveBoundary = true)
{
    geometry::DecimationOptions opts;
    opts.targetRatio = targetRatio;
    opts.preserveBoundary = preserveBoundary;
    return std::make_unique<DecimateCommand>(mesh, opts);
}

/**
 * @brief Create a smooth command
 */
inline CommandPtr smooth(
    geometry::MeshData& mesh,
    geometry::SmoothingAlgorithm algorithm,
    int iterations,
    float factor = 0.5f)
{
    geometry::SmoothingOptions opts;
    opts.algorithm = algorithm;
    opts.iterations = iterations;
    opts.lambda = factor;
    return std::make_unique<SmoothCommand>(mesh, opts);
}

/**
 * @brief Create a subdivide command
 */
inline CommandPtr subdivide(
    geometry::MeshData& mesh,
    geometry::SubdivisionAlgorithm algorithm,
    int iterations = 1)
{
    geometry::SubdivisionOptions opts;
    opts.algorithm = algorithm;
    opts.iterations = iterations;
    return std::make_unique<SubdivideCommand>(mesh, opts);
}

/**
 * @brief Create a repair command
 */
inline CommandPtr repair(
    geometry::MeshData& mesh,
    RepairCommand::Operation operation,
    float parameter = 0.0f)
{
    return std::make_unique<RepairCommand>(mesh, operation, parameter);
}

/**
 * @brief Create a full repair command
 */
inline CommandPtr repairAll(geometry::MeshData& mesh, bool fillHoles = true)
{
    return std::make_unique<RepairCommand>(
        mesh, 
        RepairCommand::Operation::RepairAll,
        fillHoles ? 1.0f : 0.0f);
}

} // namespace commands

} // namespace core
} // namespace dc3d
