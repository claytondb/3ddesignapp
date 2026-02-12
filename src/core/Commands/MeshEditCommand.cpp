/**
 * @file MeshEditCommand.cpp
 * @brief Implementation of mesh editing commands
 */

#include "MeshEditCommand.h"

#include <sstream>
#include <algorithm>

namespace dc3d {
namespace core {

// ============================================================================
// MeshEditCommand Implementation
// ============================================================================

MeshEditCommand::MeshEditCommand(
    geometry::MeshData& mesh,
    const std::string& name,
    EditFunction operation)
    : mesh_(mesh)
    , name_(name)
    , operation_(operation)
{
}

CommandPtr MeshEditCommand::create(
    geometry::MeshData& mesh,
    const std::string& name,
    EditFunction operation)
{
    return std::unique_ptr<MeshEditCommand>(
        new MeshEditCommand(mesh, name, operation));
}

bool MeshEditCommand::execute() {
    // Store current state
    beforeMesh_ = mesh_;
    
    // Apply operation
    if (!operation_(mesh_)) {
        // Operation failed, restore
        mesh_ = beforeMesh_;
        return false;
    }
    
    // Store result
    afterMesh_ = mesh_;
    executed_ = true;
    
    return true;
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
    : mesh_(mesh)
    , options_(options)
{
}

bool DecimateCommand::execute() {
    beforeMesh_ = mesh_;
    
    auto result = geometry::MeshDecimator::decimate(mesh_, options_, nullptr);
    
    if (!result.ok()) {
        mesh_ = beforeMesh_;
        return false;
    }
    
    mesh_ = std::move(result.value->first);
    result_ = result.value->second;
    
    return true;
}

void DecimateCommand::undo() {
    mesh_ = beforeMesh_;
}

std::string DecimateCommand::description() const {
    std::ostringstream ss;
    ss << "Decimate (";
    
    switch (options_.targetMode) {
        case geometry::DecimationTarget::Ratio:
            ss << static_cast<int>(options_.targetRatio * 100) << "%)";
            break;
        case geometry::DecimationTarget::VertexCount:
            ss << options_.targetVertexCount << " vertices)";
            break;
        case geometry::DecimationTarget::FaceCount:
            ss << options_.targetFaceCount << " faces)";
            break;
    }
    
    return ss.str();
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
    : mesh_(mesh)
    , options_(options)
{
}

bool SmoothCommand::execute() {
    beforeMesh_ = mesh_;
    
    result_ = geometry::MeshSmoother::smooth(mesh_, options_, nullptr);
    
    return true;  // Smoothing doesn't fail
}

void SmoothCommand::undo() {
    mesh_ = beforeMesh_;
}

std::string SmoothCommand::description() const {
    std::ostringstream ss;
    ss << "Smooth (";
    
    switch (options_.algorithm) {
        case geometry::SmoothingAlgorithm::Laplacian:
            ss << "Laplacian";
            break;
        case geometry::SmoothingAlgorithm::Taubin:
            ss << "Taubin";
            break;
        case geometry::SmoothingAlgorithm::HCLaplacian:
            ss << "HC";
            break;
        case geometry::SmoothingAlgorithm::Cotangent:
            ss << "Cotangent";
            break;
    }
    
    ss << ", " << options_.iterations << " iterations)";
    
    return ss.str();
}

size_t SmoothCommand::memoryUsage() const {
    return sizeof(*this) + beforeMesh_.memoryUsage();
}

bool SmoothCommand::canMergeWith(const Command& other) const {
    // Can merge consecutive smoothing commands of the same type
    auto* otherSmooth = dynamic_cast<const SmoothCommand*>(&other);
    if (!otherSmooth) return false;
    
    // Same mesh and algorithm
    if (&mesh_ != &otherSmooth->mesh_) return false;
    if (options_.algorithm != otherSmooth->options_.algorithm) return false;
    
    // Within 2 seconds
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        otherSmooth->timestamp() - timestamp());
    return diff.count() < 2;
}

bool SmoothCommand::mergeWith(const Command& other) {
    auto* otherSmooth = dynamic_cast<const SmoothCommand*>(&other);
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
    : mesh_(mesh)
    , operation_(operation)
    , parameter_(parameter)
{
}

bool RepairCommand::execute() {
    beforeMesh_ = mesh_;
    
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
    
    return result_.success;
}

void RepairCommand::undo() {
    mesh_ = beforeMesh_;
}

std::string RepairCommand::description() const {
    switch (operation_) {
        case Operation::RemoveOutliers:
            return "Remove Outliers";
        case Operation::FillHoles:
            return "Fill Holes";
        case Operation::RemoveDuplicates:
            return "Remove Duplicate Vertices";
        case Operation::RemoveDegenerates:
            return "Remove Degenerate Faces";
        case Operation::MakeManifold:
            return "Make Manifold";
        case Operation::RepairAll:
            return "Repair Mesh";
    }
    return "Mesh Repair";
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
    : mesh_(mesh)
    , options_(options)
{
}

bool SubdivideCommand::execute() {
    beforeMesh_ = mesh_;
    
    auto result = geometry::MeshSubdivider::subdivide(mesh_, options_, nullptr);
    
    if (!result.ok()) {
        mesh_ = beforeMesh_;
        return false;
    }
    
    mesh_ = std::move(result.value->first);
    result_ = result.value->second;
    
    return true;
}

void SubdivideCommand::undo() {
    mesh_ = beforeMesh_;
}

std::string SubdivideCommand::description() const {
    std::ostringstream ss;
    ss << "Subdivide (";
    
    switch (options_.algorithm) {
        case geometry::SubdivisionAlgorithm::Loop:
            ss << "Loop";
            break;
        case geometry::SubdivisionAlgorithm::CatmullClark:
            ss << "Catmull-Clark";
            break;
        case geometry::SubdivisionAlgorithm::Butterfly:
            ss << "Butterfly";
            break;
        case geometry::SubdivisionAlgorithm::MidPoint:
            ss << "Midpoint";
            break;
    }
    
    ss << ", " << options_.iterations << "x)";
    
    return ss.str();
}

size_t SubdivideCommand::memoryUsage() const {
    return sizeof(*this) + beforeMesh_.memoryUsage();
}

// ============================================================================
// CompoundCommand Implementation
// ============================================================================

CompoundCommand::CompoundCommand(const std::string& name)
    : name_(name)
{
}

void CompoundCommand::addCommand(CommandPtr cmd) {
    commands_.push_back(std::move(cmd));
}

bool CompoundCommand::execute() {
    executedCount_ = 0;
    
    for (auto& cmd : commands_) {
        if (!cmd->execute()) {
            // Rollback
            for (size_t i = executedCount_; i > 0; --i) {
                commands_[i - 1]->undo();
            }
            return false;
        }
        ++executedCount_;
    }
    
    return true;
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
    if (!undoStack_.empty() && undoStack_.back()->canMergeWith(*cmd)) {
        if (undoStack_.back()->mergeWith(*cmd)) {
            // Merged successfully, don't add new command
            notifyHistoryChanged();
            return true;
        }
    }
    
    // Execute the command
    if (!cmd->execute()) {
        return false;
    }
    
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

std::string CommandHistory::undoDescription() const {
    if (undoStack_.empty()) return "";
    return "Undo " + undoStack_.back()->description();
}

std::string CommandHistory::redoDescription() const {
    if (redoStack_.empty()) return "";
    return "Redo " + redoStack_.back()->description();
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
    while (currentMemoryUsage_ > maxMemoryBytes_ && !undoStack_.empty()) {
        currentMemoryUsage_ -= undoStack_.front()->memoryUsage();
        undoStack_.erase(undoStack_.begin());
    }
}

void CommandHistory::notifyHistoryChanged() {
    if (historyChangedCallback_) {
        historyChangedCallback_();
    }
}

} // namespace core
} // namespace dc3d
