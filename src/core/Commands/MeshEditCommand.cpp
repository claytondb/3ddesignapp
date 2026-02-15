/**
 * @file MeshEditCommand.cpp
 * @brief Implementation of mesh editing commands
 * 
 * Memory Note: MeshEditCommand stores both beforeMesh_ and afterMesh_, doubling
 * memory usage for large meshes. This is intentional for correctness when:
 * - Operations may not be deterministic
 * - User edits the mesh after command execution
 * 
 * For memory-constrained applications, consider storing lightweight deltas instead
 * of full mesh copies, or use streaming undo where only the last N commands store
 * full state and older commands just store operations.
 */

#include "MeshEditCommand.h"

#include <algorithm>
#include <numeric>
#include <chrono>
#include <QDebug>

namespace dc3d {
namespace core {

// ============================================================================
// MeshEditCommand Implementation
// ============================================================================

MeshEditCommand::MeshEditCommand(
    geometry::MeshData& mesh,
    const QString& name,
    EditFunction operation)
    : MeshCommand(name)
    , mesh_(mesh)
    , name_(name)
    , operation_(operation)
{
}

CommandPtr MeshEditCommand::create(
    geometry::MeshData& mesh,
    const QString& name,
    EditFunction operation)
{
    return std::unique_ptr<MeshEditCommand>(
        new MeshEditCommand(mesh, name, operation));
}

void MeshEditCommand::execute() {
    // Store current state
    beforeMesh_ = mesh_;
    
    // Apply operation
    if (!operation_(mesh_)) {
        // Operation failed, restore
        mesh_ = beforeMesh_;
        return;
    }
    
    // Store result for redo
    afterMesh_ = mesh_;
    executed_ = true;
}

void MeshEditCommand::undo() {
    if (!executed_) return;
    mesh_ = beforeMesh_;
}

void MeshEditCommand::redo() {
    if (!executed_) {
        execute();
        return;
    }
    mesh_ = afterMesh_;
}

size_t MeshEditCommand::memoryUsage() const {
    return sizeof(*this) + 
           beforeMesh_.memoryUsage() + 
           afterMesh_.memoryUsage();
}

// ============================================================================
// DecimateCommand Implementation
// ============================================================================

DecimateCommand::DecimateCommand(
    geometry::MeshData& mesh,
    const geometry::DecimationOptions& options)
    : MeshCommand(QStringLiteral("Decimate"))
    , mesh_(mesh)
    , options_(options)
{
}

void DecimateCommand::execute() {
    beforeMesh_ = mesh_;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    auto result = geometry::MeshDecimator::decimate(mesh_, options_, nullptr);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    executionTimeMs_ = duration.count();
    
    if (!result.ok()) {
        mesh_ = beforeMesh_;
        qWarning() << "Polygon reduction failed:" << QString::fromStdString(result.error);
        return;
    }
    
    mesh_ = std::move(result.value->first);
    result_ = result.value->second;
    
    qDebug() << "Polygon reduction complete (" << executionTimeMs_ / 1000.0 << "seconds)"
             << "- reduced from" << result_.originalFaces << "to" << result_.finalFaces << "faces";
}

void DecimateCommand::undo() {
    mesh_ = beforeMesh_;
}

QString DecimateCommand::description() const {
    QString desc = QStringLiteral("Decimate (");
    
    switch (options_.targetMode) {
        case geometry::DecimationTarget::Ratio:
            desc += QString::number(static_cast<int>(options_.targetRatio * 100)) + QStringLiteral("%)");
            break;
        case geometry::DecimationTarget::VertexCount:
            desc += QString::number(options_.targetVertexCount) + QStringLiteral(" vertices)");
            break;
        case geometry::DecimationTarget::FaceCount:
            desc += QString::number(options_.targetFaceCount) + QStringLiteral(" faces)");
            break;
    }
    
    return desc;
}

size_t DecimateCommand::memoryUsage() const {
    return sizeof(*this) + beforeMesh_.memoryUsage();
}

// ============================================================================
// SmoothCommand Implementation
// ============================================================================

SmoothCommand::SmoothCommand(
    geometry::MeshData& mesh,
    const geometry::SmoothingOptions& options)
    : MeshCommand(QStringLiteral("Smooth"))
    , mesh_(mesh)
    , options_(options)
{
}

void SmoothCommand::execute() {
    beforeMesh_ = mesh_;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    result_ = geometry::MeshSmoother::smooth(mesh_, options_, nullptr);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    executionTimeMs_ = duration.count();
    
    qDebug() << "Smoothing complete (" << executionTimeMs_ / 1000.0 << "seconds)"
             << "-" << result_.iterationsPerformed << "iterations," 
             << result_.verticesMoved << "vertices moved";
}

void SmoothCommand::undo() {
    mesh_ = beforeMesh_;
}

QString SmoothCommand::description() const {
    QString desc = QStringLiteral("Smooth (");
    
    switch (options_.algorithm) {
        case geometry::SmoothingAlgorithm::Laplacian:
            desc += QStringLiteral("Laplacian");
            break;
        case geometry::SmoothingAlgorithm::Taubin:
            desc += QStringLiteral("Taubin");
            break;
        case geometry::SmoothingAlgorithm::HCLaplacian:
            desc += QStringLiteral("HC");
            break;
        case geometry::SmoothingAlgorithm::Cotangent:
            desc += QStringLiteral("Cotangent");
            break;
    }
    
    desc += QStringLiteral(", ") + QString::number(options_.iterations) + QStringLiteral(" iterations)");
    
    return desc;
}

size_t SmoothCommand::memoryUsage() const {
    return sizeof(*this) + beforeMesh_.memoryUsage();
}

bool SmoothCommand::canMergeWith(const Command* other) const {
    // Can merge consecutive smoothing commands of the same type
    auto* otherSmooth = dynamic_cast<const SmoothCommand*>(other);
    if (!otherSmooth) return false;
    
    // Same mesh and algorithm
    if (&mesh_ != &otherSmooth->mesh_) return false;
    if (options_.algorithm != otherSmooth->options_.algorithm) return false;
    
    // Within 2 seconds
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        otherSmooth->timestamp() - timestamp());
    return diff.count() < 2;
}

bool SmoothCommand::mergeWith(const Command* other) {
    auto* otherSmooth = dynamic_cast<const SmoothCommand*>(other);
    if (!otherSmooth) return false;
    
    // Keep our before state, use other's after state (current mesh)
    options_.iterations += otherSmooth->options_.iterations;
    result_.iterationsPerformed += otherSmooth->result_.iterationsPerformed;
    result_.verticesMoved = std::max(result_.verticesMoved, otherSmooth->result_.verticesMoved);
    result_.maxDisplacement = std::max(result_.maxDisplacement, otherSmooth->result_.maxDisplacement);
    
    return true;
}

// ============================================================================
// RepairCommand Implementation
// ============================================================================

RepairCommand::RepairCommand(
    geometry::MeshData& mesh,
    Operation operation,
    float parameter)
    : MeshCommand(QStringLiteral("Repair"))
    , mesh_(mesh)
    , operation_(operation)
    , parameter_(parameter)
{
}

void RepairCommand::execute() {
    beforeMesh_ = mesh_;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    switch (operation_) {
        case Operation::RemoveOutliers:
            result_ = geometry::MeshRepair::removeOutliers(mesh_, parameter_);
            break;
            
        case Operation::FillHoles:
            result_ = geometry::MeshRepair::fillHoles(
                mesh_, 
                static_cast<size_t>(parameter_ > 0 ? parameter_ : 100),
                nullptr);
            break;
            
        case Operation::RemoveDuplicates: {
            size_t removed = geometry::MeshRepair::removeDuplicateVertices(
                mesh_, 
                parameter_ > 0 ? parameter_ : 1e-6f,
                nullptr);
            result_.itemsRemoved = removed;
            result_.success = true;
            result_.message = "Removed " + std::to_string(removed) + " duplicate vertices";
            break;
        }
            
        case Operation::RemoveDegenerates: {
            size_t removed = geometry::MeshRepair::removeDegenerateFaces(mesh_);
            result_.itemsRemoved = removed;
            result_.success = true;
            result_.message = "Removed " + std::to_string(removed) + " degenerate faces";
            break;
        }
            
        case Operation::MakeManifold:
            result_ = geometry::MeshRepair::makeManifold(mesh_, nullptr);
            break;
            
        case Operation::RepairAll:
            result_ = geometry::MeshRepair::repairAll(
                mesh_, 
                parameter_ > 0,  // Fill holes if parameter > 0
                nullptr);
            break;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    executionTimeMs_ = duration.count();
    
    qDebug() << description() << "complete (" << executionTimeMs_ / 1000.0 << "seconds)"
             << "-" << QString::fromStdString(result_.message);
}

void RepairCommand::undo() {
    mesh_ = beforeMesh_;
}

QString RepairCommand::description() const {
    switch (operation_) {
        case Operation::RemoveOutliers:
            return QStringLiteral("Remove Outliers");
        case Operation::FillHoles:
            return QStringLiteral("Fill Holes");
        case Operation::RemoveDuplicates:
            return QStringLiteral("Remove Duplicate Vertices");
        case Operation::RemoveDegenerates:
            return QStringLiteral("Remove Degenerate Faces");
        case Operation::MakeManifold:
            return QStringLiteral("Make Manifold");
        case Operation::RepairAll:
            return QStringLiteral("Repair Mesh");
    }
    return QStringLiteral("Mesh Repair");
}

size_t RepairCommand::memoryUsage() const {
    return sizeof(*this) + beforeMesh_.memoryUsage();
}

// ============================================================================
// SubdivideCommand Implementation
// ============================================================================

SubdivideCommand::SubdivideCommand(
    geometry::MeshData& mesh,
    const geometry::SubdivisionOptions& options)
    : MeshCommand(QStringLiteral("Subdivide"))
    , mesh_(mesh)
    , options_(options)
{
}

void SubdivideCommand::execute() {
    beforeMesh_ = mesh_;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    auto result = geometry::MeshSubdivider::subdivide(mesh_, options_, nullptr);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    executionTimeMs_ = duration.count();
    
    if (!result.ok()) {
        mesh_ = beforeMesh_;
        qWarning() << "Subdivision failed:" << QString::fromStdString(result.error);
        return;
    }
    
    mesh_ = std::move(result.value->first);
    result_ = result.value->second;
    
    qDebug() << "Subdivision complete (" << executionTimeMs_ / 1000.0 << "seconds)"
             << "- increased from" << result_.originalFaces << "to" << result_.finalFaces << "faces";
}

void SubdivideCommand::undo() {
    mesh_ = beforeMesh_;
}

QString SubdivideCommand::description() const {
    QString desc = QStringLiteral("Subdivide (");
    
    switch (options_.algorithm) {
        case geometry::SubdivisionAlgorithm::Loop:
            desc += QStringLiteral("Loop");
            break;
        case geometry::SubdivisionAlgorithm::CatmullClark:
            desc += QStringLiteral("Catmull-Clark");
            break;
        case geometry::SubdivisionAlgorithm::Butterfly:
            desc += QStringLiteral("Butterfly");
            break;
        case geometry::SubdivisionAlgorithm::MidPoint:
            desc += QStringLiteral("Midpoint");
            break;
    }
    
    desc += QStringLiteral(", ") + QString::number(options_.iterations) + QStringLiteral("x)");
    
    return desc;
}

size_t SubdivideCommand::memoryUsage() const {
    return sizeof(*this) + beforeMesh_.memoryUsage();
}

// ============================================================================
// CompoundCommand Implementation
// ============================================================================

CompoundCommand::CompoundCommand(const QString& name)
    : MeshCommand(name)
    , name_(name)
{
}

void CompoundCommand::addCommand(CommandPtr cmd) {
    commands_.push_back(std::move(cmd));
}

void CompoundCommand::execute() {
    executedCount_ = 0;
    
    for (auto& cmd : commands_) {
        cmd->execute();
        ++executedCount_;
    }
}

void CompoundCommand::undo() {
    for (size_t i = executedCount_; i > 0; --i) {
        commands_[i - 1]->undo();
    }
}

void CompoundCommand::redo() {
    for (size_t i = 0; i < executedCount_; ++i) {
        commands_[i]->redo();
    }
}

size_t CompoundCommand::memoryUsage() const {
    size_t total = sizeof(*this);
    for (const auto& cmd : commands_) {
        total += cmd->memoryUsage();
    }
    return total;
}

// ============================================================================
// CommandHistory Implementation
// ============================================================================

CommandHistory::CommandHistory(size_t maxMemoryBytes)
    : maxMemoryBytes_(maxMemoryBytes)
{
}

bool CommandHistory::execute(CommandPtr cmd) {
    if (!cmd) return false;
    
    // Try to merge with previous command
    if (!undoStack_.empty() && undoStack_.back()->canMergeWith(cmd.get())) {
        if (undoStack_.back()->mergeWith(cmd.get())) {
            // Merged successfully, don't add new command
            notifyHistoryChanged();
            return true;
        }
    }
    
    // Execute the command
    cmd->execute();
    
    // Clear redo stack
    currentMemoryUsage_ -= std::accumulate(
        redoStack_.begin(), redoStack_.end(), size_t(0),
        [](size_t sum, const CommandPtr& c) { return sum + c->memoryUsage(); });
    redoStack_.clear();
    
    // Add to undo stack
    currentMemoryUsage_ += cmd->memoryUsage();
    undoStack_.push_back(std::move(cmd));
    
    // Trim if over memory limit
    trimToMemoryLimit();
    
    notifyHistoryChanged();
    return true;
}

bool CommandHistory::undo() {
    if (undoStack_.empty()) return false;
    
    CommandPtr cmd = std::move(undoStack_.back());
    undoStack_.pop_back();
    
    cmd->undo();
    
    redoStack_.push_back(std::move(cmd));
    
    notifyHistoryChanged();
    return true;
}

bool CommandHistory::redo() {
    if (redoStack_.empty()) return false;
    
    CommandPtr cmd = std::move(redoStack_.back());
    redoStack_.pop_back();
    
    cmd->redo();
    
    undoStack_.push_back(std::move(cmd));
    
    notifyHistoryChanged();
    return true;
}

QString CommandHistory::undoDescription() const {
    if (undoStack_.empty()) return QString();
    return QStringLiteral("Undo ") + undoStack_.back()->description();
}

QString CommandHistory::redoDescription() const {
    if (redoStack_.empty()) return QString();
    return QStringLiteral("Redo ") + redoStack_.back()->description();
}

void CommandHistory::clear() {
    undoStack_.clear();
    redoStack_.clear();
    currentMemoryUsage_ = 0;
    notifyHistoryChanged();
}

size_t CommandHistory::memoryUsage() const {
    return currentMemoryUsage_;
}

void CommandHistory::setMaxMemory(size_t bytes) {
    maxMemoryBytes_ = bytes;
    trimToMemoryLimit();
}

void CommandHistory::trimToMemoryLimit() {
    // First try removing old undo commands (oldest first)
    while (currentMemoryUsage_ > maxMemoryBytes_ && !undoStack_.empty()) {
        currentMemoryUsage_ -= undoStack_.front()->memoryUsage();
        undoStack_.erase(undoStack_.begin());
    }
    
    // If still over limit, clear the redo stack entirely
    // (redo is less important than recent undo)
    if (currentMemoryUsage_ > maxMemoryBytes_ && !redoStack_.empty()) {
        for (const auto& cmd : redoStack_) {
            currentMemoryUsage_ -= cmd->memoryUsage();
        }
        redoStack_.clear();
    }
}

void CommandHistory::notifyHistoryChanged() {
    if (historyChangedCallback_) {
        historyChangedCallback_();
    }
}

} // namespace core
} // namespace dc3d
