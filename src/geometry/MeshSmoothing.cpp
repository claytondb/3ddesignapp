/**
 * @file MeshSmoothing.cpp
 * @brief Implementation of mesh smoothing algorithms
 */

#include "MeshSmoothing.h"
#include "HalfEdgeMesh.h"

#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace dc3d {
namespace geometry {

// ============================================================================
// Helper Functions
// ============================================================================

std::vector<std::vector<uint32_t>> MeshSmoother::buildAdjacencyList(const MeshData& mesh) {
    std::vector<std::vector<uint32_t>> adjacency(mesh.vertexCount());
    
    const auto& indices = mesh.indices();
    
    // For each triangle, add edges
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t v0 = indices[i];
        uint32_t v1 = indices[i + 1];
        uint32_t v2 = indices[i + 2];
        
        // Add neighbors (avoid duplicates)
        auto addNeighbor = [&](uint32_t from, uint32_t to) {
            auto& neighbors = adjacency[from];
            if (std::find(neighbors.begin(), neighbors.end(), to) == neighbors.end()) {
                neighbors.push_back(to);
            }
        };
        
        addNeighbor(v0, v1);
        addNeighbor(v0, v2);
        addNeighbor(v1, v0);
        addNeighbor(v1, v2);
        addNeighbor(v2, v0);
        addNeighbor(v2, v1);
    }
    
    return adjacency;
}

std::unordered_set<uint32_t> MeshSmoother::findBoundaryVertices(const MeshData& mesh) {
    std::unordered_set<uint32_t> boundaryVertices;
    
    // Build edge count map
    std::unordered_map<uint64_t, int> edgeCount;
    
    auto makeEdgeKey = [](uint32_t v0, uint32_t v1) -> uint64_t {
        if (v0 > v1) std::swap(v0, v1);
        return (static_cast<uint64_t>(v0) << 32) | v1;
    };
    
    const auto& indices = mesh.indices();
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t v0 = indices[i];
        uint32_t v1 = indices[i + 1];
        uint32_t v2 = indices[i + 2];
        
        edgeCount[makeEdgeKey(v0, v1)]++;
        edgeCount[makeEdgeKey(v1, v2)]++;
        edgeCount[makeEdgeKey(v2, v0)]++;
    }
    
    // Boundary edges have count == 1
    for (const auto& [key, count] : edgeCount) {
        if (count == 1) {
            uint32_t v0 = static_cast<uint32_t>(key >> 32);
            uint32_t v1 = static_cast<uint32_t>(key & 0xFFFFFFFF);
            boundaryVertices.insert(v0);
            boundaryVertices.insert(v1);
        }
    }
    
    return boundaryVertices;
}

std::unordered_set<uint32_t> MeshSmoother::findFeatureVertices(
    const MeshData& mesh, 
    float angleThreshold)
{
    std::unordered_set<uint32_t> featureVertices;
    
    // Convert threshold to cosine
    float cosThreshold = std::cos(glm::radians(angleThreshold));
    
    // Build face normals and vertex-face adjacency
    const auto& indices = mesh.indices();
    size_t faceCount = indices.size() / 3;
    
    std::vector<glm::vec3> faceNormals(faceCount);
    std::vector<std::vector<uint32_t>> vertexFaces(mesh.vertexCount());
    
    for (size_t fi = 0; fi < faceCount; ++fi) {
        uint32_t v0 = indices[fi * 3];
        uint32_t v1 = indices[fi * 3 + 1];
        uint32_t v2 = indices[fi * 3 + 2];
        
        faceNormals[fi] = mesh.faceNormal(fi);
        
        vertexFaces[v0].push_back(static_cast<uint32_t>(fi));
        vertexFaces[v1].push_back(static_cast<uint32_t>(fi));
        vertexFaces[v2].push_back(static_cast<uint32_t>(fi));
    }
    
    // Build edge-face adjacency
    std::unordered_map<uint64_t, std::vector<uint32_t>> edgeFaces;
    
    auto makeEdgeKey = [](uint32_t v0, uint32_t v1) -> uint64_t {
        if (v0 > v1) std::swap(v0, v1);
        return (static_cast<uint64_t>(v0) << 32) | v1;
    };
    
    for (size_t fi = 0; fi < faceCount; ++fi) {
        uint32_t v0 = indices[fi * 3];
        uint32_t v1 = indices[fi * 3 + 1];
        uint32_t v2 = indices[fi * 3 + 2];
        
        edgeFaces[makeEdgeKey(v0, v1)].push_back(static_cast<uint32_t>(fi));
        edgeFaces[makeEdgeKey(v1, v2)].push_back(static_cast<uint32_t>(fi));
        edgeFaces[makeEdgeKey(v2, v0)].push_back(static_cast<uint32_t>(fi));
    }
    
    // Find feature edges
    for (const auto& [key, faces] : edgeFaces) {
        if (faces.size() == 2) {
            float dot = glm::dot(faceNormals[faces[0]], faceNormals[faces[1]]);
            if (dot < cosThreshold) {
                // This is a feature edge
                uint32_t v0 = static_cast<uint32_t>(key >> 32);
                uint32_t v1 = static_cast<uint32_t>(key & 0xFFFFFFFF);
                featureVertices.insert(v0);
                featureVertices.insert(v1);
            }
        }
    }
    
    return featureVertices;
}

glm::vec3 MeshSmoother::computeLaplacian(
    const MeshData& mesh,
    uint32_t vertexIdx,
    const std::vector<std::vector<uint32_t>>& adjacency)
{
    const auto& neighbors = adjacency[vertexIdx];
    if (neighbors.empty()) {
        return glm::vec3(0.0f);
    }
    
    const auto& vertices = mesh.vertices();
    glm::vec3 centroid(0.0f);
    
    for (uint32_t ni : neighbors) {
        centroid += vertices[ni];
    }
    centroid /= static_cast<float>(neighbors.size());
    
    // Laplacian = centroid - current position
    return centroid - vertices[vertexIdx];
}

glm::vec3 MeshSmoother::computeCotangentLaplacian(
    const MeshData& mesh,
    uint32_t vertexIdx,
    const std::vector<std::vector<uint32_t>>& adjacency,
    const std::vector<std::vector<uint32_t>>& vertexFaces)
{
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    const auto& neighbors = adjacency[vertexIdx];
    
    if (neighbors.empty()) {
        return glm::vec3(0.0f);
    }
    
    const glm::vec3& vi = vertices[vertexIdx];
    glm::vec3 laplacian(0.0f);
    float weightSum = 0.0f;
    
    for (uint32_t nj : neighbors) {
        const glm::vec3& vj = vertices[nj];
        
        // Find triangles containing edge (vi, vj)
        float cotWeight = 0.0f;
        
        for (uint32_t fi : vertexFaces[vertexIdx]) {
            uint32_t fv0 = indices[fi * 3];
            uint32_t fv1 = indices[fi * 3 + 1];
            uint32_t fv2 = indices[fi * 3 + 2];
            
            // Check if this face contains edge (vertexIdx, nj)
            bool hasVI = (fv0 == vertexIdx || fv1 == vertexIdx || fv2 == vertexIdx);
            bool hasVJ = (fv0 == nj || fv1 == nj || fv2 == nj);
            
            if (hasVI && hasVJ) {
                // Find the opposite vertex
                uint32_t opposite = INVALID_INDEX;
                if (fv0 != vertexIdx && fv0 != nj) opposite = fv0;
                else if (fv1 != vertexIdx && fv1 != nj) opposite = fv1;
                else opposite = fv2;
                
                const glm::vec3& vo = vertices[opposite];
                
                // Compute cotangent of angle at opposite vertex
                glm::vec3 e1 = vi - vo;
                glm::vec3 e2 = vj - vo;
                
                float dot = glm::dot(e1, e2);
                float cross = glm::length(glm::cross(e1, e2));
                
                if (cross > 1e-10f) {
                    cotWeight += dot / cross;
                }
            }
        }
        
        // Clamp weight to avoid numerical issues
        cotWeight = std::max(cotWeight, 0.01f);
        
        laplacian += cotWeight * (vj - vi);
        weightSum += cotWeight;
    }
    
    if (weightSum > 1e-10f) {
        laplacian /= weightSum;
    }
    
    return laplacian;
}

// ============================================================================
// MeshSmoother Implementation
// ============================================================================

SmoothingResult MeshSmoother::smooth(
    MeshData& mesh,
    const SmoothingOptions& options,
    ProgressCallback progress)
{
    SmoothingResult result;
    
    if (mesh.isEmpty()) {
        return result;
    }
    
    // Build adjacency
    auto adjacency = buildAdjacencyList(mesh);
    
    // Find fixed vertices
    std::unordered_set<uint32_t> fixedVertices = options.lockedVertices;
    
    if (options.preserveBoundary) {
        auto boundary = findBoundaryVertices(mesh);
        fixedVertices.insert(boundary.begin(), boundary.end());
        result.boundaryVerticesSkipped = boundary.size();
    }
    
    if (options.preserveFeatures) {
        auto features = findFeatureVertices(mesh, options.featureAngle);
        fixedVertices.insert(features.begin(), features.end());
    }
    
    // Store original positions for HC smoothing
    std::vector<glm::vec3> originalPositions;
    if (options.algorithm == SmoothingAlgorithm::HCLaplacian) {
        originalPositions = mesh.vertices();
    }
    
    // Build vertex-face adjacency for cotangent weights
    std::vector<std::vector<uint32_t>> vertexFaces;
    if (options.algorithm == SmoothingAlgorithm::Cotangent) {
        vertexFaces.resize(mesh.vertexCount());
        const auto& indices = mesh.indices();
        for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
            vertexFaces[indices[fi * 3]].push_back(static_cast<uint32_t>(fi));
            vertexFaces[indices[fi * 3 + 1]].push_back(static_cast<uint32_t>(fi));
            vertexFaces[indices[fi * 3 + 2]].push_back(static_cast<uint32_t>(fi));
        }
    }
    
    auto& vertices = mesh.vertices();
    std::vector<glm::vec3> newPositions(vertices.size());
    std::vector<glm::vec3> bValues;  // For HC smoothing
    
    // FIX: Track total displacement locally to avoid uninitialized struct member issue
    float localTotalDisplacement = 0.0f;
    
    for (int iter = 0; iter < options.iterations; ++iter) {
        // Progress callback
        if (progress) {
            float p = static_cast<float>(iter) / options.iterations;
            if (!progress(p)) {
                result.cancelled = true;
                break;
            }
        }
        
        // Compute new positions
        switch (options.algorithm) {
            case SmoothingAlgorithm::Laplacian:
            case SmoothingAlgorithm::Cotangent: {
                for (size_t i = 0; i < vertices.size(); ++i) {
                    if (fixedVertices.count(static_cast<uint32_t>(i))) {
                        newPositions[i] = vertices[i];
                        continue;
                    }
                    
                    glm::vec3 laplacian;
                    if (options.algorithm == SmoothingAlgorithm::Cotangent) {
                        laplacian = computeCotangentLaplacian(
                            mesh, static_cast<uint32_t>(i), adjacency, vertexFaces);
                    } else {
                        laplacian = computeLaplacian(
                            mesh, static_cast<uint32_t>(i), adjacency);
                    }
                    
                    newPositions[i] = vertices[i] + options.lambda * laplacian;
                }
                
                // Apply
                for (size_t i = 0; i < vertices.size(); ++i) {
                    if (!fixedVertices.count(static_cast<uint32_t>(i))) {
                        float disp = glm::length(newPositions[i] - vertices[i]);
                        localTotalDisplacement += disp;  // FIX: Use local variable
                        result.maxDisplacement = std::max(result.maxDisplacement, disp);
                        if (disp > 1e-10f) ++result.verticesMoved;
                    }
                    vertices[i] = newPositions[i];
                }
                break;
            }
            
            case SmoothingAlgorithm::Taubin: {
                // Forward pass (shrink)
                for (size_t i = 0; i < vertices.size(); ++i) {
                    if (fixedVertices.count(static_cast<uint32_t>(i))) {
                        newPositions[i] = vertices[i];
                        continue;
                    }
                    
                    glm::vec3 laplacian = computeLaplacian(
                        mesh, static_cast<uint32_t>(i), adjacency);
                    newPositions[i] = vertices[i] + options.lambda * laplacian;
                }
                
                vertices = newPositions;
                
                // Backward pass (inflate)
                for (size_t i = 0; i < vertices.size(); ++i) {
                    if (fixedVertices.count(static_cast<uint32_t>(i))) {
                        newPositions[i] = vertices[i];
                        continue;
                    }
                    
                    glm::vec3 laplacian = computeLaplacian(
                        mesh, static_cast<uint32_t>(i), adjacency);
                    newPositions[i] = vertices[i] + options.mu * laplacian;
                }
                
                // Apply and measure
                for (size_t i = 0; i < vertices.size(); ++i) {
                    float disp = glm::length(newPositions[i] - vertices[i]);
                    localTotalDisplacement += disp;  // FIX: Use local variable
                    result.maxDisplacement = std::max(result.maxDisplacement, disp);
                    if (disp > 1e-10f) ++result.verticesMoved;
                    vertices[i] = newPositions[i];
                }
                break;
            }
            
            case SmoothingAlgorithm::HCLaplacian: {
                // Step 1: Regular Laplacian smoothing
                for (size_t i = 0; i < vertices.size(); ++i) {
                    if (fixedVertices.count(static_cast<uint32_t>(i))) {
                        newPositions[i] = vertices[i];
                        continue;
                    }
                    
                    glm::vec3 laplacian = computeLaplacian(
                        mesh, static_cast<uint32_t>(i), adjacency);
                    newPositions[i] = vertices[i] + options.lambda * laplacian;
                }
                
                // Step 2: Compute b values (difference from original)
                bValues.resize(vertices.size());
                for (size_t i = 0; i < vertices.size(); ++i) {
                    bValues[i] = newPositions[i] - 
                        (options.alpha * originalPositions[i] + 
                         (1.0f - options.alpha) * vertices[i]);
                }
                
                // Step 3: Pushback based on neighbor b values
                for (size_t i = 0; i < vertices.size(); ++i) {
                    if (fixedVertices.count(static_cast<uint32_t>(i))) {
                        continue;
                    }
                    
                    // Average neighbor b values
                    const auto& neighbors = adjacency[i];
                    glm::vec3 avgB(0.0f);
                    for (uint32_t ni : neighbors) {
                        avgB += bValues[ni];
                    }
                    if (!neighbors.empty()) {
                        avgB /= static_cast<float>(neighbors.size());
                    }
                    
                    // Pushback
                    newPositions[i] -= options.beta * bValues[i] + 
                                       (1.0f - options.beta) * avgB;
                }
                
                // Apply
                for (size_t i = 0; i < vertices.size(); ++i) {
                    float disp = glm::length(newPositions[i] - vertices[i]);
                    localTotalDisplacement += disp;  // FIX: Use local variable
                    result.maxDisplacement = std::max(result.maxDisplacement, disp);
                    if (disp > 1e-10f) ++result.verticesMoved;
                    vertices[i] = newPositions[i];
                }
                break;
            }
        }
        
        ++result.iterationsPerformed;
    }
    
    // Recompute normals
    mesh.computeNormals();
    
    // Compute average
    // FIX: Use local total displacement variable
    if (result.verticesMoved > 0) {
        result.averageDisplacement = localTotalDisplacement / result.verticesMoved;
    }
    
    return result;
}

void MeshSmoother::smooth(
    MeshData& mesh,
    SmoothingAlgorithm algorithm,
    int iterations,
    float factor)
{
    SmoothingOptions options;
    options.algorithm = algorithm;
    options.iterations = iterations;
    options.lambda = factor;
    
    smooth(mesh, options, nullptr);
}

size_t MeshSmoother::laplacianSmooth(
    MeshData& mesh,
    float lambda,
    bool preserveBoundary)
{
    SmoothingOptions options;
    options.algorithm = SmoothingAlgorithm::Laplacian;
    options.iterations = 1;
    options.lambda = lambda;
    options.preserveBoundary = preserveBoundary;
    
    auto result = smooth(mesh, options, nullptr);
    return result.verticesMoved;
}

size_t MeshSmoother::taubinSmooth(
    MeshData& mesh,
    float lambda,
    float mu,
    bool preserveBoundary)
{
    SmoothingOptions options;
    options.algorithm = SmoothingAlgorithm::Taubin;
    options.iterations = 1;
    options.lambda = lambda;
    options.mu = mu;
    options.preserveBoundary = preserveBoundary;
    
    auto result = smooth(mesh, options, nullptr);
    return result.verticesMoved;
}

size_t MeshSmoother::hcSmooth(
    MeshData& mesh,
    const std::vector<glm::vec3>& originalPositions,
    float alpha,
    float beta,
    bool preserveBoundary)
{
    // For HC smoothing with custom original positions, we need manual implementation
    auto adjacency = buildAdjacencyList(mesh);
    
    std::unordered_set<uint32_t> fixedVertices;
    if (preserveBoundary) {
        fixedVertices = findBoundaryVertices(mesh);
    }
    
    auto& vertices = mesh.vertices();
    std::vector<glm::vec3> newPositions(vertices.size());
    std::vector<glm::vec3> bValues(vertices.size());
    
    size_t moved = 0;
    
    // Step 1: Laplacian
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (fixedVertices.count(static_cast<uint32_t>(i))) {
            newPositions[i] = vertices[i];
            continue;
        }
        
        glm::vec3 laplacian = computeLaplacian(mesh, static_cast<uint32_t>(i), adjacency);
        newPositions[i] = vertices[i] + 0.5f * laplacian;
    }
    
    // Step 2: b values
    for (size_t i = 0; i < vertices.size(); ++i) {
        const glm::vec3& orig = (i < originalPositions.size()) ? 
            originalPositions[i] : vertices[i];
        bValues[i] = newPositions[i] - (alpha * orig + (1.0f - alpha) * vertices[i]);
    }
    
    // Step 3: Pushback
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (fixedVertices.count(static_cast<uint32_t>(i))) {
            continue;
        }
        
        const auto& neighbors = adjacency[i];
        glm::vec3 avgB(0.0f);
        for (uint32_t ni : neighbors) {
            avgB += bValues[ni];
        }
        if (!neighbors.empty()) {
            avgB /= static_cast<float>(neighbors.size());
        }
        
        newPositions[i] -= beta * bValues[i] + (1.0f - beta) * avgB;
    }
    
    // Apply
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (glm::length(newPositions[i] - vertices[i]) > 1e-10f) {
            ++moved;
        }
        vertices[i] = newPositions[i];
    }
    
    mesh.computeNormals();
    return moved;
}

// ============================================================================
// SmoothingState Implementation
// ============================================================================

SmoothingState::SmoothingState(MeshData& mesh, const SmoothingOptions& options)
    : mesh_(mesh)
    , options_(options)
{
    originalPositions_ = mesh_.vertices();
    previousPositions_ = mesh_.vertices();
    adjacency_ = MeshSmoother::buildAdjacencyList(mesh_);
    
    if (options_.preserveBoundary) {
        auto boundary = MeshSmoother::findBoundaryVertices(mesh_);
        fixedVertices_.insert(boundary.begin(), boundary.end());
    }
    
    if (options_.preserveFeatures) {
        auto features = MeshSmoother::findFeatureVertices(mesh_, options_.featureAngle);
        fixedVertices_.insert(features.begin(), features.end());
    }
    
    fixedVertices_.insert(options_.lockedVertices.begin(), options_.lockedVertices.end());
}

void SmoothingState::iterate() {
    if (isComplete()) return;
    
    previousPositions_ = mesh_.vertices();
    
    SmoothingOptions iterOpts = options_;
    iterOpts.iterations = 1;
    iterOpts.lockedVertices = fixedVertices_;
    iterOpts.preserveBoundary = false;  // Already handled
    iterOpts.preserveFeatures = false;
    
    auto result = MeshSmoother::smooth(mesh_, iterOpts, nullptr);
    
    totalDisplacement_ += result.averageDisplacement * result.verticesMoved;
    maxDisplacement_ = std::max(maxDisplacement_, result.maxDisplacement);
    verticesMoved_ = std::max(verticesMoved_, result.verticesMoved);
    
    ++currentIteration_;
}

SmoothingResult SmoothingState::getResult() const {
    SmoothingResult result;
    result.iterationsPerformed = currentIteration_;
    result.verticesMoved = verticesMoved_;
    result.maxDisplacement = maxDisplacement_;
    if (verticesMoved_ > 0) {
        result.averageDisplacement = totalDisplacement_ / verticesMoved_;
    }
    result.boundaryVerticesSkipped = fixedVertices_.size();
    return result;
}

} // namespace geometry
} // namespace dc3d
