/**
 * @file MeshDecimation.cpp
 * @brief Implementation of QEM mesh decimation
 */

#include "MeshDecimation.h"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <array>

namespace dc3d {
namespace geometry {

// ============================================================================
// Quadric Implementation
// ============================================================================

Quadric::Quadric(float a, float b, float c, float d) {
    // Q = [a]   [a b c d]
    //     [b] *
    //     [c]
    //     [d]
    // Upper triangle storage
    data_[0] = a * a;      // a00
    data_[1] = a * b;      // a01
    data_[2] = a * c;      // a02
    data_[3] = a * d;      // a03
    data_[4] = b * b;      // a11
    data_[5] = b * c;      // a12
    data_[6] = b * d;      // a13
    data_[7] = c * c;      // a22
    data_[8] = c * d;      // a23
    data_[9] = d * d;      // a33
}

Quadric Quadric::fromPlane(const glm::vec3& normal, const glm::vec3& point) {
    // Plane equation: n·x + d = 0, where d = -n·point
    float d = -glm::dot(normal, point);
    return Quadric(normal.x, normal.y, normal.z, d);
}

float Quadric::evaluate(const glm::vec3& point) const {
    // Compute v^T * Q * v where v = [x, y, z, 1]
    float x = point.x, y = point.y, z = point.z;
    
    return data_[0] * x * x + 2.0f * data_[1] * x * y + 2.0f * data_[2] * x * z + 2.0f * data_[3] * x
         + data_[4] * y * y + 2.0f * data_[5] * y * z + 2.0f * data_[6] * y
         + data_[7] * z * z + 2.0f * data_[8] * z
         + data_[9];
}

Quadric Quadric::operator+(const Quadric& other) const {
    Quadric result;
    for (int i = 0; i < 10; ++i) {
        result.data_[i] = data_[i] + other.data_[i];
    }
    return result;
}

Quadric& Quadric::operator+=(const Quadric& other) {
    for (int i = 0; i < 10; ++i) {
        data_[i] += other.data_[i];
    }
    return *this;
}

Quadric Quadric::operator*(float scale) const {
    Quadric result;
    for (int i = 0; i < 10; ++i) {
        result.data_[i] = data_[i] * scale;
    }
    return result;
}

bool Quadric::findOptimal(glm::vec3& outPoint) const {
    // Solve the 3x3 linear system derived from dQ/dx = 0
    // [ 2*a00  2*a01  2*a02 ] [x]   [-2*a03]
    // [ 2*a01  2*a11  2*a12 ] [y] = [-2*a13]
    // [ 2*a02  2*a12  2*a22 ] [z]   [-2*a23]
    
    // Simplified to:
    // [ a00  a01  a02 ] [x]   [-a03]
    // [ a01  a11  a12 ] [y] = [-a13]
    // [ a02  a12  a22 ] [z]   [-a23]
    
    // Using Cramer's rule with 3x3 determinants
    float a00 = data_[0], a01 = data_[1], a02 = data_[2], a03 = data_[3];
    float a11 = data_[4], a12 = data_[5], a13 = data_[6];
    float a22 = data_[7], a23 = data_[8];
    
    // Determinant of coefficient matrix
    float det = a00 * (a11 * a22 - a12 * a12)
              - a01 * (a01 * a22 - a12 * a02)
              + a02 * (a01 * a12 - a11 * a02);
    
    const float eps = 1e-10f;
    if (std::abs(det) < eps) {
        return false;  // Singular matrix
    }
    
    float invDet = 1.0f / det;
    
    // b = [-a03, -a13, -a23]
    float b0 = -a03, b1 = -a13, b2 = -a23;
    
    // Cramer's rule
    outPoint.x = invDet * (b0 * (a11 * a22 - a12 * a12)
                         - a01 * (b1 * a22 - a12 * b2)
                         + a02 * (b1 * a12 - a11 * b2));
    
    outPoint.y = invDet * (a00 * (b1 * a22 - a12 * b2)
                         - b0 * (a01 * a22 - a12 * a02)
                         + a02 * (a01 * b2 - b1 * a02));
    
    outPoint.z = invDet * (a00 * (a11 * b2 - b1 * a12)
                         - a01 * (a01 * b2 - b1 * a02)
                         + b0 * (a01 * a12 - a11 * a02));
    
    return true;
}

glm::mat4 Quadric::toMatrix() const {
    return glm::mat4(
        data_[0], data_[1], data_[2], data_[3],  // Column 0
        data_[1], data_[4], data_[5], data_[6],  // Column 1
        data_[2], data_[5], data_[7], data_[8],  // Column 2
        data_[3], data_[6], data_[8], data_[9]   // Column 3
    );
}

// ============================================================================
// DecimationState Implementation
// ============================================================================

DecimationState::DecimationState(HalfEdgeMesh&& mesh, const DecimationOptions& options)
    : mesh_(std::move(mesh))
    , options_(options)
    , activeVertices_(mesh_.vertexCount())
    , activeFaces_(mesh_.faceCount())
{
    vertexQuadrics_.resize(mesh_.vertexCount());
    vertexVersions_.resize(mesh_.vertexCount(), 0);
    vertexDeleted_.resize(mesh_.vertexCount(), false);
    faceDeleted_.resize(mesh_.faceCount(), false);
    
    initializeQuadrics();
    initializeQueue();
}

void DecimationState::initializeQuadrics() {
    // Compute quadric for each vertex as sum of quadrics from adjacent faces
    for (size_t fi = 0; fi < mesh_.faceCount(); ++fi) {
        auto verts = mesh_.faceVertices(static_cast<uint32_t>(fi));
        if (verts.size() != 3) continue;
        
        const auto& p0 = mesh_.vertex(verts[0]).position;
        const auto& p1 = mesh_.vertex(verts[1]).position;
        const auto& p2 = mesh_.vertex(verts[2]).position;
        
        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        
        // Skip degenerate triangles
        if (!std::isfinite(normal.x)) continue;
        
        Quadric faceQuadric = Quadric::fromPlane(normal, p0);
        
        // Add to each vertex
        for (uint32_t vi : verts) {
            vertexQuadrics_[vi] += faceQuadric;
        }
    }
    
    // Add boundary penalty if preserving boundaries
    if (options_.preserveBoundary) {
        auto boundaryEdges = mesh_.findBoundaryEdges();
        
        for (uint32_t heIdx : boundaryEdges) {
            const auto& he = mesh_.halfEdge(heIdx);
            uint32_t v0 = mesh_.halfEdgeSource(heIdx);
            uint32_t v1 = he.vertex;
            
            const auto& p0 = mesh_.vertex(v0).position;
            const auto& p1 = mesh_.vertex(v1).position;
            
            // Create quadric from boundary edge plane
            glm::vec3 edge = p1 - p0;
            float edgeLen = glm::length(edge);
            if (edgeLen < 1e-10f) continue;
            
            // We need a perpendicular plane to the boundary edge
            // Use the average of adjacent face normals
            glm::vec3 faceNormal(0.0f);
            if (he.face != INVALID_INDEX) {
                faceNormal = mesh_.face(he.face).normal;
            }
            
            // Boundary constraint plane: perpendicular to edge, containing edge
            glm::vec3 boundaryNormal = glm::normalize(glm::cross(edge, faceNormal));
            if (!std::isfinite(boundaryNormal.x)) continue;
            
            Quadric boundaryQuadric = Quadric::fromPlane(boundaryNormal, p0) * options_.boundaryWeight;
            
            vertexQuadrics_[v0] += boundaryQuadric;
            vertexQuadrics_[v1] += boundaryQuadric;
        }
    }
}

void DecimationState::initializeQueue() {
    // Add all edges to priority queue
    std::unordered_set<uint64_t> addedEdges;
    
    for (size_t heIdx = 0; heIdx < mesh_.halfEdgeCount(); ++heIdx) {
        const auto& he = mesh_.halfEdge(static_cast<uint32_t>(heIdx));
        if (he.vertex == INVALID_INDEX) continue;
        
        uint32_t v0 = mesh_.halfEdgeSource(static_cast<uint32_t>(heIdx));
        uint32_t v1 = he.vertex;
        
        // Only add each edge once
        uint64_t edgeKey = (std::min(v0, v1) * uint64_t(0x100000000)) + std::max(v0, v1);
        if (addedEdges.count(edgeKey)) continue;
        addedEdges.insert(edgeKey);
        
        if (!isEdgeValid(static_cast<uint32_t>(heIdx))) continue;
        
        EdgeCollapse collapse = computeEdgeCost(static_cast<uint32_t>(heIdx));
        queue_.push(collapse);
    }
}

EdgeCollapse DecimationState::computeEdgeCost(uint32_t heIdx) const {
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    const auto& p0 = mesh_.vertex(v0).position;
    const auto& p1 = mesh_.vertex(v1).position;
    
    // Combined quadric
    Quadric Q = vertexQuadrics_[v0] + vertexQuadrics_[v1];
    
    EdgeCollapse collapse;
    collapse.heIdx = heIdx;
    collapse.v0 = v0;
    collapse.v1 = v1;
    collapse.version = vertexVersions_[v0];
    
    // Try to find optimal point
    if (Q.findOptimal(collapse.target)) {
        // Check if optimal point is reasonable (within edge bounds)
        glm::vec3 edgeCenter = (p0 + p1) * 0.5f;
        float edgeLen = glm::length(p1 - p0);
        float optDist = glm::length(collapse.target - edgeCenter);
        
        if (optDist > edgeLen * 2.0f) {
            // Optimal point is too far, use midpoint
            collapse.target = edgeCenter;
        }
    } else {
        // Singular matrix, use midpoint
        collapse.target = (p0 + p1) * 0.5f;
    }
    
    // Compute error at target position
    collapse.cost = Q.evaluate(collapse.target);
    
    // Apply boundary penalty if either vertex is on boundary
    if (options_.preserveBoundary) {
        if (mesh_.isVertexOnBoundary(v0) || mesh_.isVertexOnBoundary(v1)) {
            // If both are on boundary but not connected by boundary edge,
            // this collapse might destroy boundary topology
            if (mesh_.isVertexOnBoundary(v0) && mesh_.isVertexOnBoundary(v1)) {
                if (!he.isBoundary() && (he.twin == INVALID_INDEX || !mesh_.halfEdge(he.twin).isBoundary())) {
                    collapse.cost += options_.boundaryWeight * 1000.0f;
                }
            }
        }
    }
    
    // Apply locked vertex penalty
    if (options_.lockVertices) {
        if (options_.lockedVertices.count(v0) || options_.lockedVertices.count(v1)) {
            collapse.cost = std::numeric_limits<float>::max();
        }
    }
    
    return collapse;
}

bool DecimationState::isEdgeValid(uint32_t heIdx) const {
    const auto& he = mesh_.halfEdge(heIdx);
    if (he.vertex == INVALID_INDEX) return false;
    
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    if (vertexDeleted_[v0] || vertexDeleted_[v1]) return false;
    
    return true;
}

bool DecimationState::checkTopology(uint32_t heIdx) const {
    if (!options_.preserveTopology) return true;
    
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    // Get 1-ring neighbors of both vertices
    auto neighbors0 = mesh_.vertexNeighbors(v0);
    auto neighbors1 = mesh_.vertexNeighbors(v1);
    
    std::unordered_set<uint32_t> set0(neighbors0.begin(), neighbors0.end());
    
    // Count common neighbors (should be exactly 2 for a valid edge collapse)
    int commonCount = 0;
    for (uint32_t n : neighbors1) {
        if (set0.count(n)) {
            ++commonCount;
        }
    }
    
    // For manifold meshes, an internal edge has exactly 2 common neighbors
    // A boundary edge has exactly 1 common neighbor
    bool isBoundary = he.isBoundary() || (he.twin != INVALID_INDEX && mesh_.halfEdge(he.twin).isBoundary());
    
    if (isBoundary) {
        return commonCount <= 1;
    } else {
        return commonCount <= 2;
    }
}

bool DecimationState::canCollapse(uint32_t heIdx) const {
    if (!isEdgeValid(heIdx)) return false;
    if (!checkTopology(heIdx)) return false;
    
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    // Check valence
    size_t val0 = mesh_.vertexValence(v0);
    size_t val1 = mesh_.vertexValence(v1);
    
    // Prevent creating vertices with very low valence
    if (val0 <= 3 || val1 <= 3) {
        // This is a critical vertex, be careful
        if (val0 + val1 <= 6) return false;
    }
    
    return true;
}

bool DecimationState::collapseEdge(uint32_t heIdx, const glm::vec3& newPosition) {
    if (!canCollapse(heIdx)) return false;
    
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    // Move v0 to new position
    mesh_.vertex(v0).position = newPosition;
    
    // Update quadric at v0
    vertexQuadrics_[v0] += vertexQuadrics_[v1];
    
    // Mark v1 as deleted
    vertexDeleted_[v1] = true;
    --activeVertices_;
    
    // Get faces to delete
    std::vector<uint32_t> facesToDelete;
    if (he.face != INVALID_INDEX && !faceDeleted_[he.face]) {
        facesToDelete.push_back(he.face);
    }
    if (he.twin != INVALID_INDEX) {
        const auto& twin = mesh_.halfEdge(he.twin);
        if (twin.face != INVALID_INDEX && !faceDeleted_[twin.face]) {
            facesToDelete.push_back(twin.face);
        }
    }
    
    // Mark faces as deleted
    for (uint32_t fi : facesToDelete) {
        faceDeleted_[fi] = true;
        --activeFaces_;
    }
    
    // Redirect edges pointing to v1 to point to v0
    // FIX: Properly iterate over half-edges and update references to v1
    for (size_t i = 0; i < mesh_.halfEdgeCount(); ++i) {
        auto& he = mesh_.halfEdge(static_cast<uint32_t>(i));
        if (he.vertex == v1) {
            he.vertex = v0;
        }
    }
    
    // Update version to invalidate old queue entries
    ++vertexVersions_[v0];
    ++vertexVersions_[v1];
    
    return true;
}

size_t DecimationState::computeTargetFaces() const {
    switch (options_.targetMode) {
        case DecimationTarget::Ratio:
            return static_cast<size_t>(mesh_.faceCount() * options_.targetRatio);
        case DecimationTarget::VertexCount:
            // Approximate: F ≈ 2V for closed manifolds
            return options_.targetVertexCount * 2;
        case DecimationTarget::FaceCount:
            return options_.targetFaceCount;
    }
    return mesh_.faceCount() / 2;
}

DecimationResult DecimationState::run(ProgressCallback progress) {
    DecimationResult result;
    result.originalVertices = mesh_.vertexCount();
    result.originalFaces = mesh_.faceCount();
    
    size_t targetFaces = computeTargetFaces();
    size_t startFaces = activeFaces_;
    
    float totalError = 0.0f;
    
    while (activeFaces_ > targetFaces && !queue_.empty()) {
        // Get lowest cost collapse
        EdgeCollapse collapse = queue_.top();
        queue_.pop();
        
        // Check if this entry is stale
        if (vertexDeleted_[collapse.v0] || vertexDeleted_[collapse.v1]) {
            continue;
        }
        if (vertexVersions_[collapse.v0] != collapse.version) {
            continue;
        }
        
        // Check max error threshold
        if (collapse.cost > options_.maxError) {
            break;
        }
        
        // Attempt collapse
        if (!canCollapse(collapse.heIdx)) {
            continue;
        }
        
        // Perform collapse (simplified - actual implementation would be more complex)
        // For now, we mark vertices/faces as deleted and track positions
        
        // Move surviving vertex to optimal position
        mesh_.vertex(collapse.v0).position = collapse.target;
        
        // Update quadric
        vertexQuadrics_[collapse.v0] += vertexQuadrics_[collapse.v1];
        
        // Mark deleted
        vertexDeleted_[collapse.v1] = true;
        --activeVertices_;
        
        // Mark adjacent faces as deleted
        const auto& he = mesh_.halfEdge(collapse.heIdx);
        if (he.face != INVALID_INDEX && !faceDeleted_[he.face]) {
            faceDeleted_[he.face] = true;
            --activeFaces_;
        }
        if (he.twin != INVALID_INDEX) {
            const auto& twin = mesh_.halfEdge(he.twin);
            if (twin.face != INVALID_INDEX && !faceDeleted_[twin.face]) {
                faceDeleted_[twin.face] = true;
                --activeFaces_;
            }
        }
        
        // Track stats
        ++result.edgesCollapsed;
        totalError += collapse.cost;
        result.maxError = std::max(result.maxError, collapse.cost);
        
        // Update version
        ++vertexVersions_[collapse.v0];
        ++vertexVersions_[collapse.v1];
        
        // Recompute edges around surviving vertex
        auto outEdges = mesh_.vertexOutgoingEdges(collapse.v0);
        for (uint32_t outHeIdx : outEdges) {
            if (!isEdgeValid(outHeIdx)) continue;
            
            EdgeCollapse newCollapse = computeEdgeCost(outHeIdx);
            queue_.push(newCollapse);
        }
        
        // Progress callback
        if (progress) {
            float progressVal = 1.0f - static_cast<float>(activeFaces_ - targetFaces) / 
                                       static_cast<float>(startFaces - targetFaces);
            if (!progress(std::clamp(progressVal, 0.0f, 1.0f))) {
                result.cancelled = true;
                break;
            }
        }
    }
    
    result.finalVertices = activeVertices_;
    result.finalFaces = activeFaces_;
    result.avgError = result.edgesCollapsed > 0 ? totalError / result.edgesCollapsed : 0.0f;
    result.reachedTarget = activeFaces_ <= targetFaces;
    
    return result;
}

MeshData DecimationState::toMeshData() const {
    MeshData output;
    
    // Build vertex mapping (old index -> new index)
    std::vector<uint32_t> vertexMap(mesh_.vertexCount(), INVALID_INDEX);
    uint32_t newIdx = 0;
    
    for (size_t i = 0; i < mesh_.vertexCount(); ++i) {
        if (!vertexDeleted_[i]) {
            vertexMap[i] = newIdx++;
            output.vertices().push_back(mesh_.vertex(static_cast<uint32_t>(i)).position);
        }
    }
    
    // Add faces
    for (size_t fi = 0; fi < mesh_.faceCount(); ++fi) {
        if (faceDeleted_[fi]) continue;
        
        auto verts = mesh_.faceVertices(static_cast<uint32_t>(fi));
        if (verts.size() != 3) continue;
        
        // Check all vertices are valid
        bool valid = true;
        for (uint32_t vi : verts) {
            if (vertexMap[vi] == INVALID_INDEX) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;
        
        output.addFace(vertexMap[verts[0]], vertexMap[verts[1]], vertexMap[verts[2]]);
    }
    
    output.computeNormals();
    return output;
}

// ============================================================================
// MeshDecimator Implementation
// ============================================================================

Result<std::pair<MeshData, DecimationResult>> MeshDecimator::decimate(
    const MeshData& mesh,
    const DecimationOptions& options,
    ProgressCallback progress)
{
    if (mesh.isEmpty()) {
        return Result<std::pair<MeshData, DecimationResult>>::failure("Input mesh is empty");
    }
    
    // Build half-edge mesh
    auto heMeshResult = HalfEdgeMesh::buildFromMesh(mesh, nullptr);
    if (!heMeshResult.ok()) {
        return Result<std::pair<MeshData, DecimationResult>>::failure(
            "Failed to build half-edge mesh: " + heMeshResult.error);
    }
    
    // Run decimation
    DecimationState state(std::move(*heMeshResult.value), options);
    DecimationResult result = state.run(progress);
    
    // Convert back to MeshData
    MeshData output = state.toMeshData();
    
    return Result<std::pair<MeshData, DecimationResult>>::success(
        std::make_pair(std::move(output), result));
}

Result<MeshData> MeshDecimator::decimate(
    const MeshData& mesh,
    float targetRatio,
    bool preserveBoundary,
    ProgressCallback progress)
{
    DecimationOptions options;
    options.targetMode = DecimationTarget::Ratio;
    options.targetRatio = targetRatio;
    options.preserveBoundary = preserveBoundary;
    
    auto result = decimate(mesh, options, progress);
    if (!result.ok()) {
        return Result<MeshData>::failure(result.error);
    }
    
    return Result<MeshData>::success(std::move(result.value->first));
}

} // namespace geometry
} // namespace dc3d
