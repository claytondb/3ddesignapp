/**
 * @file MeshSubdivision.cpp
 * @brief Implementation of mesh subdivision algorithms
 */

#include "MeshSubdivision.h"

#include <cmath>
#include <unordered_map>
#include <algorithm>

namespace dc3d {
namespace geometry {

namespace {

// Edge key for lookup
struct EdgeKey {
    uint32_t v0, v1;
    
    EdgeKey(uint32_t a, uint32_t b) : v0(std::min(a, b)), v1(std::max(a, b)) {}
    
    bool operator==(const EdgeKey& other) const {
        return v0 == other.v0 && v1 == other.v1;
    }
};

struct EdgeKeyHash {
    size_t operator()(const EdgeKey& k) const {
        return std::hash<uint64_t>{}(
            (static_cast<uint64_t>(k.v0) << 32) | k.v1);
    }
};

// Find boundary vertices in half-edge mesh
std::unordered_set<uint32_t> findBoundaryVertices(const HalfEdgeMesh& mesh) {
    std::unordered_set<uint32_t> boundary;
    
    for (size_t i = 0; i < mesh.halfEdgeCount(); ++i) {
        const auto& he = mesh.halfEdge(static_cast<uint32_t>(i));
        if (he.isBoundary()) {
            boundary.insert(he.vertex);
            boundary.insert(mesh.halfEdgeSource(static_cast<uint32_t>(i)));
        }
    }
    
    return boundary;
}

} // anonymous namespace

// ============================================================================
// MeshSubdivider Implementation
// ============================================================================

Result<std::pair<MeshData, SubdivisionResult>> MeshSubdivider::subdivide(
    const MeshData& mesh,
    const SubdivisionOptions& options,
    ProgressCallback progress)
{
    SubdivisionResult result;
    result.originalVertices = mesh.vertexCount();
    result.originalFaces = mesh.faceCount();
    
    if (mesh.isEmpty()) {
        return Result<std::pair<MeshData, SubdivisionResult>>::failure("Empty mesh");
    }
    
    MeshData current = mesh;  // Copy
    
    for (int iter = 0; iter < options.iterations; ++iter) {
        if (progress) {
            float p = static_cast<float>(iter) / options.iterations;
            if (!progress(p)) {
                result.cancelled = true;
                break;
            }
        }
        
        Result<MeshData> subdivided;
        
        switch (options.algorithm) {
            case SubdivisionAlgorithm::Loop:
                subdivided = loopSubdivide(current, options.preserveBoundary);
                break;
            case SubdivisionAlgorithm::CatmullClark:
                subdivided = catmullClarkSubdivide(current, options.preserveBoundary);
                break;
            case SubdivisionAlgorithm::Butterfly:
                subdivided = butterflySubdivide(current, options.preserveBoundary);
                break;
            case SubdivisionAlgorithm::MidPoint:
                subdivided = midpointSubdivide(current);
                break;
        }
        
        if (!subdivided.ok()) {
            return Result<std::pair<MeshData, SubdivisionResult>>::failure(
                "Subdivision iteration " + std::to_string(iter) + " failed: " + subdivided.error);
        }
        
        current = std::move(*subdivided.value);
        ++result.iterationsPerformed;
    }
    
    result.finalVertices = current.vertexCount();
    result.finalFaces = current.faceCount();
    
    return Result<std::pair<MeshData, SubdivisionResult>>::success(
        std::make_pair(std::move(current), result));
}

Result<MeshData> MeshSubdivider::subdivide(
    const MeshData& mesh,
    SubdivisionAlgorithm algorithm,
    int iterations)
{
    SubdivisionOptions options;
    options.algorithm = algorithm;
    options.iterations = iterations;
    
    auto result = subdivide(mesh, options, nullptr);
    if (!result.ok()) {
        return Result<MeshData>::failure(result.error);
    }
    
    return Result<MeshData>::success(std::move(result.value->first));
}

// ============================================================================
// Loop Subdivision
// ============================================================================

float LoopSubdivisionState::betaCoefficient(size_t valence) {
    if (valence == 3) {
        return 3.0f / 16.0f;
    }
    
    // Warren's formula
    float n = static_cast<float>(valence);
    float center = 3.0f / 8.0f + 0.25f * std::cos(2.0f * glm::pi<float>() / n);
    return (5.0f / 8.0f - center * center) / n;
}

LoopSubdivisionState::LoopSubdivisionState(const HalfEdgeMesh& mesh, bool preserveBoundary)
    : mesh_(mesh)
    , preserveBoundary_(preserveBoundary)
{
    if (preserveBoundary) {
        boundaryVertices_ = findBoundaryVertices(mesh);
    }
}

glm::vec3 LoopSubdivisionState::computeVertexPoint(uint32_t vertexIdx) const {
    const auto& v = mesh_.vertex(vertexIdx);
    
    // Handle boundary vertices
    if (preserveBoundary_ && boundaryVertices_.count(vertexIdx)) {
        // Find boundary neighbors
        auto neighbors = mesh_.vertexNeighbors(vertexIdx);
        std::vector<uint32_t> boundaryNeighbors;
        
        for (uint32_t ni : neighbors) {
            if (boundaryVertices_.count(ni)) {
                boundaryNeighbors.push_back(ni);
            }
        }
        
        if (boundaryNeighbors.size() == 2) {
            // Boundary vertex rule: 1/8 * (n0 + n1) + 3/4 * v
            const auto& n0 = mesh_.vertex(boundaryNeighbors[0]).position;
            const auto& n1 = mesh_.vertex(boundaryNeighbors[1]).position;
            return 0.125f * (n0 + n1) + 0.75f * v.position;
        } else {
            // Corner or irregular boundary
            return v.position;
        }
    }
    
    // Interior vertex
    auto neighbors = mesh_.vertexNeighbors(vertexIdx);
    size_t n = neighbors.size();
    
    if (n == 0) {
        return v.position;
    }
    
    float beta = betaCoefficient(n);
    
    glm::vec3 neighborSum(0.0f);
    for (uint32_t ni : neighbors) {
        neighborSum += mesh_.vertex(ni).position;
    }
    
    // New position: (1 - n*beta) * v + beta * sum(neighbors)
    return (1.0f - n * beta) * v.position + beta * neighborSum;
}

glm::vec3 LoopSubdivisionState::computeEdgePoint(uint32_t heIdx) const {
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    const auto& p0 = mesh_.vertex(v0).position;
    const auto& p1 = mesh_.vertex(v1).position;
    
    // Handle boundary edge
    if (he.isBoundary() || (he.twin != INVALID_INDEX && mesh_.halfEdge(he.twin).face == INVALID_INDEX)) {
        // Boundary edge: simple midpoint
        return 0.5f * (p0 + p1);
    }
    
    // Interior edge: use 1/8 * (a + b) + 3/8 * (c + d)
    // where a, b are edge vertices and c, d are opposite vertices
    
    // Find opposite vertices in the two adjacent faces
    glm::vec3 oppositeSum(0.0f);
    int oppositeCount = 0;
    
    // Face of this half-edge
    if (he.face != INVALID_INDEX) {
        auto faceVerts = mesh_.faceVertices(he.face);
        for (uint32_t fv : faceVerts) {
            if (fv != v0 && fv != v1) {
                oppositeSum += mesh_.vertex(fv).position;
                ++oppositeCount;
                break;
            }
        }
    }
    
    // Face of twin half-edge
    if (he.twin != INVALID_INDEX) {
        const auto& twin = mesh_.halfEdge(he.twin);
        if (twin.face != INVALID_INDEX) {
            auto faceVerts = mesh_.faceVertices(twin.face);
            for (uint32_t fv : faceVerts) {
                if (fv != v0 && fv != v1) {
                    oppositeSum += mesh_.vertex(fv).position;
                    ++oppositeCount;
                    break;
                }
            }
        }
    }
    
    if (oppositeCount == 2) {
        // Standard Loop rule: 3/8 * (v0 + v1) + 1/8 * (opp0 + opp1)
        return 0.375f * (p0 + p1) + 0.125f * oppositeSum;
    } else {
        // Boundary or irregular case
        return 0.5f * (p0 + p1);
    }
}

MeshData LoopSubdivisionState::execute() {
    MeshData output;
    
    // Step 1: Create edge point lookup
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertices;
    
    // Step 2: Add new vertex positions for existing vertices
    for (size_t i = 0; i < mesh_.vertexCount(); ++i) {
        glm::vec3 newPos = computeVertexPoint(static_cast<uint32_t>(i));
        output.addVertex(newPos);
    }
    
    // Step 3: Add edge midpoints
    std::unordered_set<uint64_t> processedEdges;
    
    for (size_t heIdx = 0; heIdx < mesh_.halfEdgeCount(); ++heIdx) {
        const auto& he = mesh_.halfEdge(static_cast<uint32_t>(heIdx));
        if (he.vertex == INVALID_INDEX) continue;
        
        uint32_t v0 = mesh_.halfEdgeSource(static_cast<uint32_t>(heIdx));
        uint32_t v1 = he.vertex;
        
        EdgeKey key(v0, v1);
        if (edgeVertices.count(key)) continue;
        
        glm::vec3 edgePoint = computeEdgePoint(static_cast<uint32_t>(heIdx));
        uint32_t edgeVertexIdx = output.addVertex(edgePoint);
        edgeVertices[key] = edgeVertexIdx;
    }
    
    // Step 4: Create new faces
    // Each original triangle becomes 4 triangles
    for (size_t fi = 0; fi < mesh_.faceCount(); ++fi) {
        auto faceVerts = mesh_.faceVertices(static_cast<uint32_t>(fi));
        if (faceVerts.size() != 3) continue;
        
        uint32_t v0 = faceVerts[0];
        uint32_t v1 = faceVerts[1];
        uint32_t v2 = faceVerts[2];
        
        // Get edge midpoint vertices
        uint32_t e01 = edgeVertices[EdgeKey(v0, v1)];
        uint32_t e12 = edgeVertices[EdgeKey(v1, v2)];
        uint32_t e20 = edgeVertices[EdgeKey(v2, v0)];
        
        // Create 4 new triangles
        output.addFace(v0, e01, e20);   // Corner 0
        output.addFace(v1, e12, e01);   // Corner 1
        output.addFace(v2, e20, e12);   // Corner 2
        output.addFace(e01, e12, e20);  // Center
    }
    
    output.computeNormals();
    return output;
}

Result<MeshData> MeshSubdivider::loopSubdivide(
    const MeshData& mesh,
    bool preserveBoundary)
{
    auto heMeshResult = HalfEdgeMesh::buildFromMesh(mesh, nullptr);
    if (!heMeshResult.ok()) {
        return Result<MeshData>::failure("Failed to build half-edge mesh: " + heMeshResult.error);
    }
    
    LoopSubdivisionState state(*heMeshResult.value, preserveBoundary);
    return Result<MeshData>::success(state.execute());
}

// ============================================================================
// Catmull-Clark Subdivision
// ============================================================================

CatmullClarkState::CatmullClarkState(const HalfEdgeMesh& mesh, bool preserveBoundary)
    : mesh_(mesh)
    , preserveBoundary_(preserveBoundary)
{
    if (preserveBoundary) {
        boundaryVertices_ = findBoundaryVertices(mesh);
    }
}

glm::vec3 CatmullClarkState::computeFacePoint(uint32_t faceIdx) const {
    auto verts = mesh_.faceVertices(faceIdx);
    
    glm::vec3 centroid(0.0f);
    for (uint32_t vi : verts) {
        centroid += mesh_.vertex(vi).position;
    }
    
    return centroid / static_cast<float>(verts.size());
}

glm::vec3 CatmullClarkState::computeEdgePoint(
    uint32_t heIdx, 
    const std::vector<glm::vec3>& facePoints) const
{
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    const auto& p0 = mesh_.vertex(v0).position;
    const auto& p1 = mesh_.vertex(v1).position;
    
    // Boundary edge
    if (he.isBoundary() || (he.twin != INVALID_INDEX && mesh_.halfEdge(he.twin).face == INVALID_INDEX)) {
        return 0.5f * (p0 + p1);
    }
    
    // Interior edge: average of edge midpoint and face points
    glm::vec3 faceSum(0.0f);
    int faceCount = 0;
    
    if (he.face != INVALID_INDEX) {
        faceSum += facePoints[he.face];
        ++faceCount;
    }
    
    if (he.twin != INVALID_INDEX) {
        const auto& twin = mesh_.halfEdge(he.twin);
        if (twin.face != INVALID_INDEX) {
            faceSum += facePoints[twin.face];
            ++faceCount;
        }
    }
    
    if (faceCount == 2) {
        return 0.25f * (p0 + p1 + faceSum);
    } else {
        return 0.5f * (p0 + p1);
    }
}

glm::vec3 CatmullClarkState::computeVertexPoint(
    uint32_t vertexIdx,
    const std::vector<glm::vec3>& facePoints,
    const std::vector<glm::vec3>& edgePoints) const
{
    const auto& v = mesh_.vertex(vertexIdx);
    
    // Boundary vertex
    if (preserveBoundary_ && boundaryVertices_.count(vertexIdx)) {
        auto neighbors = mesh_.vertexNeighbors(vertexIdx);
        std::vector<uint32_t> boundaryNeighbors;
        
        for (uint32_t ni : neighbors) {
            if (boundaryVertices_.count(ni)) {
                boundaryNeighbors.push_back(ni);
            }
        }
        
        if (boundaryNeighbors.size() == 2) {
            const auto& n0 = mesh_.vertex(boundaryNeighbors[0]).position;
            const auto& n1 = mesh_.vertex(boundaryNeighbors[1]).position;
            return 0.125f * (n0 + n1) + 0.75f * v.position;
        } else {
            return v.position;
        }
    }
    
    // Interior vertex
    auto adjacentFaces = mesh_.vertexFaces(vertexIdx);
    auto adjacentEdges = mesh_.vertexOutgoingEdges(vertexIdx);
    
    size_t n = adjacentFaces.size();
    if (n == 0) return v.position;
    
    // Average of adjacent face points
    glm::vec3 F(0.0f);
    for (uint32_t fi : adjacentFaces) {
        F += facePoints[fi];
    }
    F /= static_cast<float>(n);
    
    // Average of edge midpoints
    glm::vec3 R(0.0f);
    int edgeCount = 0;
    for (uint32_t heIdx : adjacentEdges) {
        const auto& he = mesh_.halfEdge(heIdx);
        glm::vec3 midpoint = 0.5f * (v.position + mesh_.vertex(he.vertex).position);
        R += midpoint;
        ++edgeCount;
    }
    if (edgeCount > 0) {
        R /= static_cast<float>(edgeCount);
    }
    
    // Catmull-Clark formula: (F + 2R + (n-3)P) / n
    float nf = static_cast<float>(n);
    return (F + 2.0f * R + (nf - 3.0f) * v.position) / nf;
}

MeshData CatmullClarkState::execute() {
    MeshData output;
    
    // Step 1: Compute face points
    std::vector<glm::vec3> facePoints(mesh_.faceCount());
    for (size_t fi = 0; fi < mesh_.faceCount(); ++fi) {
        facePoints[fi] = computeFacePoint(static_cast<uint32_t>(fi));
    }
    
    // Step 2: Compute edge points
    std::unordered_map<EdgeKey, glm::vec3, EdgeKeyHash> edgePointMap;
    
    for (size_t heIdx = 0; heIdx < mesh_.halfEdgeCount(); ++heIdx) {
        const auto& he = mesh_.halfEdge(static_cast<uint32_t>(heIdx));
        if (he.vertex == INVALID_INDEX) continue;
        
        uint32_t v0 = mesh_.halfEdgeSource(static_cast<uint32_t>(heIdx));
        uint32_t v1 = he.vertex;
        
        EdgeKey key(v0, v1);
        if (edgePointMap.count(key)) continue;
        
        edgePointMap[key] = computeEdgePoint(static_cast<uint32_t>(heIdx), facePoints);
    }
    
    // Convert to vector for vertex lookup
    std::vector<glm::vec3> edgePointVec(edgePointMap.size());
    
    // Step 3: Compute new vertex positions
    for (size_t i = 0; i < mesh_.vertexCount(); ++i) {
        glm::vec3 newPos = computeVertexPoint(
            static_cast<uint32_t>(i), facePoints, edgePointVec);
        output.addVertex(newPos);
    }
    
    // Step 4: Add face points as vertices
    std::vector<uint32_t> faceVertexIndices(mesh_.faceCount());
    for (size_t fi = 0; fi < mesh_.faceCount(); ++fi) {
        faceVertexIndices[fi] = output.addVertex(facePoints[fi]);
    }
    
    // Step 5: Add edge points as vertices
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertexIndices;
    for (const auto& [key, point] : edgePointMap) {
        edgeVertexIndices[key] = output.addVertex(point);
    }
    
    // Step 6: Create new faces
    // For triangle input, each triangle becomes 3 quads
    // But since we only support triangles, we split quads into triangles
    
    for (size_t fi = 0; fi < mesh_.faceCount(); ++fi) {
        auto faceVerts = mesh_.faceVertices(static_cast<uint32_t>(fi));
        uint32_t faceVertex = faceVertexIndices[fi];
        
        // For each vertex in the original face, create a quad
        // (original vertex, edge point, face point, edge point)
        // We split this into 2 triangles
        
        for (size_t i = 0; i < faceVerts.size(); ++i) {
            uint32_t v0 = faceVerts[i];
            uint32_t v1 = faceVerts[(i + 1) % faceVerts.size()];
            uint32_t vPrev = faceVerts[(i + faceVerts.size() - 1) % faceVerts.size()];
            
            uint32_t e01 = edgeVertexIndices[EdgeKey(v0, v1)];
            uint32_t ePrev = edgeVertexIndices[EdgeKey(vPrev, v0)];
            
            // Create quad as 2 triangles: v0-e01-f and v0-f-ePrev
            output.addFace(v0, e01, faceVertex);
            output.addFace(v0, faceVertex, ePrev);
        }
    }
    
    output.computeNormals();
    return output;
}

Result<MeshData> MeshSubdivider::catmullClarkSubdivide(
    const MeshData& mesh,
    bool preserveBoundary)
{
    auto heMeshResult = HalfEdgeMesh::buildFromMesh(mesh, nullptr);
    if (!heMeshResult.ok()) {
        return Result<MeshData>::failure("Failed to build half-edge mesh: " + heMeshResult.error);
    }
    
    CatmullClarkState state(*heMeshResult.value, preserveBoundary);
    return Result<MeshData>::success(state.execute());
}

// ============================================================================
// Butterfly Subdivision
// ============================================================================

ButterflySubdivisionState::ButterflySubdivisionState(const HalfEdgeMesh& mesh, bool preserveBoundary)
    : mesh_(mesh)
    , preserveBoundary_(preserveBoundary)
{
    if (preserveBoundary) {
        boundaryVertices_ = findBoundaryVertices(mesh);
    }
}

glm::vec3 ButterflySubdivisionState::computeBoundaryEdgePoint(uint32_t heIdx) const {
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    // Find boundary neighbors
    auto n0 = mesh_.vertexNeighbors(v0);
    auto n1 = mesh_.vertexNeighbors(v1);
    
    uint32_t b0 = INVALID_INDEX, b1 = INVALID_INDEX;
    
    for (uint32_t ni : n0) {
        if (ni != v1 && boundaryVertices_.count(ni)) {
            b0 = ni;
            break;
        }
    }
    
    for (uint32_t ni : n1) {
        if (ni != v0 && boundaryVertices_.count(ni)) {
            b1 = ni;
            break;
        }
    }
    
    const auto& p0 = mesh_.vertex(v0).position;
    const auto& p1 = mesh_.vertex(v1).position;
    
    if (b0 != INVALID_INDEX && b1 != INVALID_INDEX) {
        // 4-point rule: 9/16 * (v0 + v1) - 1/16 * (b0 + b1)
        const auto& pb0 = mesh_.vertex(b0).position;
        const auto& pb1 = mesh_.vertex(b1).position;
        return 0.5625f * (p0 + p1) - 0.0625f * (pb0 + pb1);
    } else {
        return 0.5f * (p0 + p1);
    }
}

glm::vec3 ButterflySubdivisionState::computeEdgePoint(uint32_t heIdx) const {
    const auto& he = mesh_.halfEdge(heIdx);
    uint32_t v0 = mesh_.halfEdgeSource(heIdx);
    uint32_t v1 = he.vertex;
    
    // Boundary edge
    if (preserveBoundary_ && (boundaryVertices_.count(v0) || boundaryVertices_.count(v1))) {
        if (he.isBoundary() || (he.twin != INVALID_INDEX && mesh_.halfEdge(he.twin).isBoundary())) {
            return computeBoundaryEdgePoint(heIdx);
        }
    }
    
    const auto& p0 = mesh_.vertex(v0).position;
    const auto& p1 = mesh_.vertex(v1).position;
    
    // Find the two opposite vertices
    glm::vec3 opp0(0.0f), opp1(0.0f);
    bool hasOpp0 = false, hasOpp1 = false;
    
    if (he.face != INVALID_INDEX) {
        auto faceVerts = mesh_.faceVertices(he.face);
        for (uint32_t fv : faceVerts) {
            if (fv != v0 && fv != v1) {
                opp0 = mesh_.vertex(fv).position;
                hasOpp0 = true;
                break;
            }
        }
    }
    
    if (he.twin != INVALID_INDEX) {
        const auto& twin = mesh_.halfEdge(he.twin);
        if (twin.face != INVALID_INDEX) {
            auto faceVerts = mesh_.faceVertices(twin.face);
            for (uint32_t fv : faceVerts) {
                if (fv != v0 && fv != v1) {
                    opp1 = mesh_.vertex(fv).position;
                    hasOpp1 = true;
                    break;
                }
            }
        }
    }
    
    if (!hasOpp0 || !hasOpp1) {
        return 0.5f * (p0 + p1);
    }
    
    // Standard butterfly 8-point stencil
    // For regular case: 1/2 * (v0 + v1) + 1/8 * (opp0 + opp1) - 1/16 * (secondary neighbors)
    
    // Find secondary neighbors (opposite vertices of adjacent triangles)
    glm::vec3 secondarySum(0.0f);
    int secondaryCount = 0;
    
    // Adjacent triangles to v0 (not including the two main faces)
    auto v0Faces = mesh_.vertexFaces(v0);
    for (uint32_t fi : v0Faces) {
        if (fi == he.face) continue;
        if (he.twin != INVALID_INDEX && fi == mesh_.halfEdge(he.twin).face) continue;
        
        auto verts = mesh_.faceVertices(fi);
        for (uint32_t fv : verts) {
            if (fv != v0 && fv != v1) {
                // Check if this triangle is adjacent
                bool isAdjacent = false;
                for (uint32_t fv2 : verts) {
                    if (fv2 == v1) {
                        isAdjacent = true;
                        break;
                    }
                }
                if (isAdjacent) {
                    secondarySum += mesh_.vertex(fv).position;
                    ++secondaryCount;
                }
                break;
            }
        }
    }
    
    // Adjacent triangles to v1
    auto v1Faces = mesh_.vertexFaces(v1);
    for (uint32_t fi : v1Faces) {
        if (fi == he.face) continue;
        if (he.twin != INVALID_INDEX && fi == mesh_.halfEdge(he.twin).face) continue;
        
        auto verts = mesh_.faceVertices(fi);
        for (uint32_t fv : verts) {
            if (fv != v0 && fv != v1) {
                bool isAdjacent = false;
                for (uint32_t fv2 : verts) {
                    if (fv2 == v0) {
                        isAdjacent = true;
                        break;
                    }
                }
                if (isAdjacent) {
                    secondarySum += mesh_.vertex(fv).position;
                    ++secondaryCount;
                }
                break;
            }
        }
    }
    
    // Butterfly weights: 1/2, 1/8, -1/16
    glm::vec3 result = 0.5f * (p0 + p1) + 0.125f * (opp0 + opp1);
    
    if (secondaryCount > 0) {
        result -= 0.0625f * secondarySum;
    }
    
    return result;
}

MeshData ButterflySubdivisionState::execute() {
    MeshData output;
    
    // Step 1: Copy original vertices (butterfly is interpolating)
    for (size_t i = 0; i < mesh_.vertexCount(); ++i) {
        output.addVertex(mesh_.vertex(static_cast<uint32_t>(i)).position);
    }
    
    // Step 2: Compute edge midpoints
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertices;
    
    for (size_t heIdx = 0; heIdx < mesh_.halfEdgeCount(); ++heIdx) {
        const auto& he = mesh_.halfEdge(static_cast<uint32_t>(heIdx));
        if (he.vertex == INVALID_INDEX) continue;
        
        uint32_t v0 = mesh_.halfEdgeSource(static_cast<uint32_t>(heIdx));
        uint32_t v1 = he.vertex;
        
        EdgeKey key(v0, v1);
        if (edgeVertices.count(key)) continue;
        
        glm::vec3 edgePoint = computeEdgePoint(static_cast<uint32_t>(heIdx));
        edgeVertices[key] = output.addVertex(edgePoint);
    }
    
    // Step 3: Create new faces (same as Loop)
    for (size_t fi = 0; fi < mesh_.faceCount(); ++fi) {
        auto faceVerts = mesh_.faceVertices(static_cast<uint32_t>(fi));
        if (faceVerts.size() != 3) continue;
        
        uint32_t v0 = faceVerts[0];
        uint32_t v1 = faceVerts[1];
        uint32_t v2 = faceVerts[2];
        
        uint32_t e01 = edgeVertices[EdgeKey(v0, v1)];
        uint32_t e12 = edgeVertices[EdgeKey(v1, v2)];
        uint32_t e20 = edgeVertices[EdgeKey(v2, v0)];
        
        output.addFace(v0, e01, e20);
        output.addFace(v1, e12, e01);
        output.addFace(v2, e20, e12);
        output.addFace(e01, e12, e20);
    }
    
    output.computeNormals();
    return output;
}

Result<MeshData> MeshSubdivider::butterflySubdivide(
    const MeshData& mesh,
    bool preserveBoundary)
{
    auto heMeshResult = HalfEdgeMesh::buildFromMesh(mesh, nullptr);
    if (!heMeshResult.ok()) {
        return Result<MeshData>::failure("Failed to build half-edge mesh: " + heMeshResult.error);
    }
    
    ButterflySubdivisionState state(*heMeshResult.value, preserveBoundary);
    return Result<MeshData>::success(state.execute());
}

// ============================================================================
// Midpoint Subdivision
// ============================================================================

Result<MeshData> MeshSubdivider::midpointSubdivide(const MeshData& mesh) {
    MeshData output;
    
    // Copy original vertices
    output.vertices() = mesh.vertices();
    
    const auto& indices = mesh.indices();
    const auto& vertices = mesh.vertices();
    
    // Edge midpoint cache
    std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeVertices;
    
    for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
        uint32_t v0 = indices[fi * 3];
        uint32_t v1 = indices[fi * 3 + 1];
        uint32_t v2 = indices[fi * 3 + 2];
        
        // Get or create edge midpoints
        auto getEdgeMidpoint = [&](uint32_t a, uint32_t b) -> uint32_t {
            EdgeKey key(a, b);
            auto it = edgeVertices.find(key);
            if (it != edgeVertices.end()) {
                return it->second;
            }
            
            glm::vec3 midpoint = 0.5f * (vertices[a] + vertices[b]);
            uint32_t idx = output.addVertex(midpoint);
            edgeVertices[key] = idx;
            return idx;
        };
        
        uint32_t e01 = getEdgeMidpoint(v0, v1);
        uint32_t e12 = getEdgeMidpoint(v1, v2);
        uint32_t e20 = getEdgeMidpoint(v2, v0);
        
        // Create 4 triangles
        output.addFace(v0, e01, e20);
        output.addFace(v1, e12, e01);
        output.addFace(v2, e20, e12);
        output.addFace(e01, e12, e20);
    }
    
    output.computeNormals();
    return Result<MeshData>::success(std::move(output));
}

} // namespace geometry
} // namespace dc3d
