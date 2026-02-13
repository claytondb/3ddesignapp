/**
 * @file MeshRepair.cpp
 * @brief Implementation of mesh repair tools
 */

#include "MeshRepair.h"
#include "HalfEdgeMesh.h"

#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cmath>

namespace dc3d {
namespace geometry {

// ============================================================================
// Helper structures
// ============================================================================

namespace {

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

// Spatial hash grid for duplicate detection
class SpatialGrid {
public:
    SpatialGrid(float cellSize) : cellSize_(cellSize), invCellSize_(1.0f / cellSize) {}
    
    void insert(uint32_t vertexIdx, const glm::vec3& pos) {
        auto key = getKey(pos);
        cells_[key].push_back(vertexIdx);
    }
    
    std::vector<uint32_t> query(const glm::vec3& pos) const {
        std::vector<uint32_t> result;
        
        // Check 3x3x3 neighborhood
        glm::ivec3 center = getCell(pos);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    auto key = packKey(center.x + dx, center.y + dy, center.z + dz);
                    auto it = cells_.find(key);
                    if (it != cells_.end()) {
                        result.insert(result.end(), it->second.begin(), it->second.end());
                    }
                }
            }
        }
        
        return result;
    }
    
private:
    float cellSize_;
    float invCellSize_;
    std::unordered_map<uint64_t, std::vector<uint32_t>> cells_;
    
    glm::ivec3 getCell(const glm::vec3& pos) const {
        return glm::ivec3(
            static_cast<int>(std::floor(pos.x * invCellSize_)),
            static_cast<int>(std::floor(pos.y * invCellSize_)),
            static_cast<int>(std::floor(pos.z * invCellSize_))
        );
    }
    
    uint64_t getKey(const glm::vec3& pos) const {
        auto c = getCell(pos);
        return packKey(c.x, c.y, c.z);
    }
    
    uint64_t packKey(int x, int y, int z) const {
        // Simple hash combining
        return (static_cast<uint64_t>(x + 0x7FFFFFFF) * 73856093ULL) ^
               (static_cast<uint64_t>(y + 0x7FFFFFFF) * 19349663ULL) ^
               (static_cast<uint64_t>(z + 0x7FFFFFFF) * 83492791ULL);
    }
};

} // anonymous namespace

// ============================================================================
// Outlier Detection and Removal
// ============================================================================

std::vector<std::vector<uint32_t>> MeshRepair::findConnectedComponents(
    const MeshData& mesh)
{
    std::vector<std::vector<uint32_t>> components;
    
    if (mesh.isEmpty()) return components;
    
    const auto& indices = mesh.indices();
    size_t faceCount = indices.size() / 3;
    
    // Build face adjacency
    std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> edgeFaces;
    
    for (size_t fi = 0; fi < faceCount; ++fi) {
        uint32_t v0 = indices[fi * 3];
        uint32_t v1 = indices[fi * 3 + 1];
        uint32_t v2 = indices[fi * 3 + 2];
        
        edgeFaces[EdgeKey(v0, v1)].push_back(static_cast<uint32_t>(fi));
        edgeFaces[EdgeKey(v1, v2)].push_back(static_cast<uint32_t>(fi));
        edgeFaces[EdgeKey(v2, v0)].push_back(static_cast<uint32_t>(fi));
    }
    
    // Build face-face adjacency
    std::vector<std::vector<uint32_t>> faceNeighbors(faceCount);
    for (const auto& [edge, faces] : edgeFaces) {
        if (faces.size() == 2) {
            faceNeighbors[faces[0]].push_back(faces[1]);
            faceNeighbors[faces[1]].push_back(faces[0]);
        }
    }
    
    // BFS to find components
    std::vector<bool> visited(faceCount, false);
    
    for (size_t startFace = 0; startFace < faceCount; ++startFace) {
        if (visited[startFace]) continue;
        
        std::vector<uint32_t> component;
        std::queue<uint32_t> queue;
        queue.push(static_cast<uint32_t>(startFace));
        visited[startFace] = true;
        
        while (!queue.empty()) {
            uint32_t fi = queue.front();
            queue.pop();
            component.push_back(fi);
            
            for (uint32_t neighbor : faceNeighbors[fi]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    queue.push(neighbor);
                }
            }
        }
        
        components.push_back(std::move(component));
    }
    
    return components;
}

size_t MeshRepair::keepLargestComponent(MeshData& mesh) {
    auto components = findConnectedComponents(mesh);
    
    if (components.size() <= 1) return 0;
    
    // Find largest
    size_t largestIdx = 0;
    size_t largestSize = 0;
    for (size_t i = 0; i < components.size(); ++i) {
        if (components[i].size() > largestSize) {
            largestSize = components[i].size();
            largestIdx = i;
        }
    }
    
    // Mark faces to keep
    std::unordered_set<uint32_t> keepFaces(
        components[largestIdx].begin(), 
        components[largestIdx].end());
    
    // Build new mesh
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    const auto& normals = mesh.normals();
    
    MeshData newMesh;
    std::vector<uint32_t> vertexMap(vertices.size(), INVALID_INDEX);
    
    for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
        if (!keepFaces.count(static_cast<uint32_t>(fi))) continue;
        
        uint32_t newIndices[3];
        bool validFace = true;
        for (int i = 0; i < 3; ++i) {
            uint32_t vi = indices[fi * 3 + i];
            // FIX: Add bounds checking for vertex index
            if (vi >= vertices.size()) {
                validFace = false;
                break;
            }
            if (vertexMap[vi] == INVALID_INDEX) {
                vertexMap[vi] = static_cast<uint32_t>(newMesh.vertices().size());
                newMesh.vertices().push_back(vertices[vi]);
                if (!normals.empty() && vi < normals.size()) {
                    newMesh.normals().push_back(normals[vi]);
                }
            }
            newIndices[i] = vertexMap[vi];
        }
        
        // FIX: Skip invalid faces
        if (!validFace) continue;
        
        newMesh.addFace(newIndices[0], newIndices[1], newIndices[2]);
    }
    
    size_t removed = mesh.faceCount() - newMesh.faceCount();
    mesh = std::move(newMesh);
    
    return removed;
}

RepairResult MeshRepair::removeOutliers(
    MeshData& mesh, 
    float threshold,
    ProgressCallback progress)
{
    RepairResult result;
    
    if (mesh.isEmpty()) {
        result.success = false;
        result.message = "Empty mesh";
        return result;
    }
    
    auto components = findConnectedComponents(mesh);
    
    if (components.size() <= 1) {
        result.message = "Single component, no outliers";
        return result;
    }
    
    // Calculate mesh diagonal
    auto bounds = mesh.boundingBox();
    float diagonal = bounds.diagonal();
    float distThreshold = diagonal * threshold;
    
    // Find main component (largest)
    size_t mainIdx = 0;
    size_t mainSize = 0;
    for (size_t i = 0; i < components.size(); ++i) {
        if (components[i].size() > mainSize) {
            mainSize = components[i].size();
            mainIdx = i;
        }
    }
    
    // Calculate centroid of main component
    const auto& indices = mesh.indices();
    const auto& vertices = mesh.vertices();
    
    glm::vec3 mainCentroid(0.0f);
    for (uint32_t fi : components[mainIdx]) {
        for (int i = 0; i < 3; ++i) {
            mainCentroid += vertices[indices[fi * 3 + i]];
        }
    }
    mainCentroid /= static_cast<float>(components[mainIdx].size() * 3);
    
    // Mark faces to keep
    std::unordered_set<uint32_t> keepFaces;
    
    for (size_t ci = 0; ci < components.size(); ++ci) {
        // Calculate component centroid
        glm::vec3 compCentroid(0.0f);
        for (uint32_t fi : components[ci]) {
            for (int i = 0; i < 3; ++i) {
                compCentroid += vertices[indices[fi * 3 + i]];
            }
        }
        compCentroid /= static_cast<float>(components[ci].size() * 3);
        
        // Keep if main component or close enough
        float dist = glm::length(compCentroid - mainCentroid);
        if (ci == mainIdx || dist < distThreshold) {
            for (uint32_t fi : components[ci]) {
                keepFaces.insert(fi);
            }
        }
    }
    
    // Rebuild mesh
    MeshData newMesh;
    std::vector<uint32_t> vertexMap(vertices.size(), INVALID_INDEX);
    
    for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
        if (!keepFaces.count(static_cast<uint32_t>(fi))) continue;
        
        uint32_t newIndices[3];
        for (int i = 0; i < 3; ++i) {
            uint32_t vi = indices[fi * 3 + i];
            if (vertexMap[vi] == INVALID_INDEX) {
                vertexMap[vi] = static_cast<uint32_t>(newMesh.vertices().size());
                newMesh.vertices().push_back(vertices[vi]);
            }
            newIndices[i] = vertexMap[vi];
        }
        
        newMesh.addFace(newIndices[0], newIndices[1], newIndices[2]);
    }
    
    result.itemsRemoved = mesh.faceCount() - newMesh.faceCount();
    
    newMesh.computeNormals();
    mesh = std::move(newMesh);
    
    result.success = true;
    result.message = "Removed " + std::to_string(result.itemsRemoved) + " outlier faces";
    
    return result;
}

// ============================================================================
// Hole Detection and Filling
// ============================================================================

std::vector<HoleInfo> MeshRepair::detectHoles(const MeshData& mesh) {
    std::vector<HoleInfo> holes;
    
    if (mesh.isEmpty()) return holes;
    
    // Build half-edge mesh to find boundary loops
    auto heMeshResult = HalfEdgeMesh::buildFromMesh(mesh, nullptr);
    if (!heMeshResult.ok()) {
        return holes;  // Can't detect holes
    }
    
    auto boundaryLoops = heMeshResult.value->findBoundaryLoops();
    const auto& vertices = mesh.vertices();
    
    for (const auto& loop : boundaryLoops) {
        if (loop.size() < 3) continue;
        
        HoleInfo hole;
        hole.boundaryVertices = loop;
        
        // Calculate perimeter and centroid
        glm::vec3 centroid(0.0f);
        for (size_t i = 0; i < loop.size(); ++i) {
            uint32_t v0 = loop[i];
            uint32_t v1 = loop[(i + 1) % loop.size()];
            
            hole.perimeter += glm::length(vertices[v1] - vertices[v0]);
            centroid += vertices[v0];
        }
        centroid /= static_cast<float>(loop.size());
        hole.centroid = centroid;
        
        // Estimate normal (average of edge cross products)
        glm::vec3 normal(0.0f);
        for (size_t i = 0; i < loop.size(); ++i) {
            uint32_t v0 = loop[i];
            uint32_t v1 = loop[(i + 1) % loop.size()];
            
            glm::vec3 e1 = vertices[v0] - centroid;
            glm::vec3 e2 = vertices[v1] - centroid;
            normal += glm::cross(e1, e2);
        }
        if (glm::length(normal) > 1e-10f) {
            hole.normal = glm::normalize(normal);
        }
        
        // Estimate area (sum of triangles to centroid)
        for (size_t i = 0; i < loop.size(); ++i) {
            uint32_t v0 = loop[i];
            uint32_t v1 = loop[(i + 1) % loop.size()];
            
            glm::vec3 e1 = vertices[v0] - centroid;
            glm::vec3 e2 = vertices[v1] - centroid;
            hole.estimatedArea += 0.5f * glm::length(glm::cross(e1, e2));
        }
        
        holes.push_back(hole);
    }
    
    return holes;
}

void MeshRepair::triangulateHoleSimple(
    MeshData& mesh, 
    const std::vector<uint32_t>& boundary)
{
    // Simple fan triangulation from centroid
    if (boundary.size() < 3) return;
    
    const auto& vertices = mesh.vertices();
    
    // Calculate centroid
    glm::vec3 centroid(0.0f);
    for (uint32_t vi : boundary) {
        centroid += vertices[vi];
    }
    centroid /= static_cast<float>(boundary.size());
    
    // Add center vertex
    uint32_t centerIdx = mesh.addVertex(centroid);
    
    // Create fan triangles
    for (size_t i = 0; i < boundary.size(); ++i) {
        uint32_t v0 = boundary[i];
        uint32_t v1 = boundary[(i + 1) % boundary.size()];
        mesh.addFace(v0, v1, centerIdx);
    }
}

void MeshRepair::triangulateHoleEarClipping(
    MeshData& mesh, 
    const std::vector<uint32_t>& boundary,
    const glm::vec3& normal)
{
    if (boundary.size() < 3) return;
    if (boundary.size() == 3) {
        mesh.addFace(boundary[0], boundary[1], boundary[2]);
        return;
    }
    
    const auto& vertices = mesh.vertices();
    
    // Create working copy of boundary
    std::vector<uint32_t> remaining = boundary;
    
    while (remaining.size() > 3) {
        bool earFound = false;
        
        for (size_t i = 0; i < remaining.size() && !earFound; ++i) {
            size_t prev = (i + remaining.size() - 1) % remaining.size();
            size_t next = (i + 1) % remaining.size();
            
            uint32_t vPrev = remaining[prev];
            uint32_t vCurr = remaining[i];
            uint32_t vNext = remaining[next];
            
            const glm::vec3& p0 = vertices[vPrev];
            const glm::vec3& p1 = vertices[vCurr];
            const glm::vec3& p2 = vertices[vNext];
            
            // Check if this is a convex vertex (ear candidate)
            glm::vec3 e1 = p1 - p0;
            glm::vec3 e2 = p2 - p1;
            glm::vec3 triNormal = glm::cross(e1, e2);
            
            if (glm::dot(triNormal, normal) <= 0) {
                continue;  // Concave vertex, skip
            }
            
            // Check if any other vertex is inside this triangle
            bool hasInside = false;
            for (size_t j = 0; j < remaining.size() && !hasInside; ++j) {
                if (j == prev || j == i || j == next) continue;
                
                const glm::vec3& p = vertices[remaining[j]];
                
                // Point-in-triangle test
                glm::vec3 v0 = p2 - p0;
                glm::vec3 v1 = p1 - p0;
                glm::vec3 v2 = p - p0;
                
                float dot00 = glm::dot(v0, v0);
                float dot01 = glm::dot(v0, v1);
                float dot02 = glm::dot(v0, v2);
                float dot11 = glm::dot(v1, v1);
                float dot12 = glm::dot(v1, v2);
                
                float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
                float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
                float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
                
                if (u >= 0 && v >= 0 && (u + v) <= 1) {
                    hasInside = true;
                }
            }
            
            if (!hasInside) {
                // This is an ear, clip it
                mesh.addFace(vPrev, vCurr, vNext);
                remaining.erase(remaining.begin() + i);
                earFound = true;
            }
        }
        
        if (!earFound) {
            // Fallback to simple triangulation
            triangulateHoleSimple(mesh, remaining);
            return;
        }
    }
    
    // Add final triangle
    if (remaining.size() == 3) {
        mesh.addFace(remaining[0], remaining[1], remaining[2]);
    }
}

void MeshRepair::triangulateHoleMinArea(
    MeshData& mesh,
    const std::vector<uint32_t>& boundary,
    const glm::vec3& centroid)
{
    // Minimum area triangulation using dynamic programming
    // For simplicity, we use the centroid-based approach for larger holes
    
    if (boundary.size() <= 10) {
        // Small hole: try ear clipping with estimated normal
        const auto& vertices = mesh.vertices();
        glm::vec3 normal(0.0f);
        for (size_t i = 0; i < boundary.size(); ++i) {
            uint32_t v0 = boundary[i];
            uint32_t v1 = boundary[(i + 1) % boundary.size()];
            glm::vec3 e1 = vertices[v0] - centroid;
            glm::vec3 e2 = vertices[v1] - centroid;
            normal += glm::cross(e1, e2);
        }
        if (glm::length(normal) > 1e-10f) {
            normal = glm::normalize(normal);
        }
        triangulateHoleEarClipping(mesh, boundary, normal);
    } else {
        // Large hole: use centroid fan
        triangulateHoleSimple(mesh, boundary);
    }
}

RepairResult MeshRepair::fillHole(
    MeshData& mesh, 
    const HoleInfo& hole,
    const HoleFillOptions& options)
{
    RepairResult result;
    
    if (hole.boundaryVertices.size() < 3) {
        result.success = false;
        result.message = "Hole has less than 3 vertices";
        return result;
    }
    
    if (hole.boundaryVertices.size() > options.maxEdges) {
        result.success = false;
        result.message = "Hole exceeds maximum edge count";
        return result;
    }
    
    size_t startFaces = mesh.faceCount();
    size_t startVerts = mesh.vertexCount();
    
    if (options.triangulate) {
        triangulateHoleMinArea(mesh, hole.boundaryVertices, hole.centroid);
    } else {
        triangulateHoleSimple(mesh, hole.boundaryVertices);
    }
    
    result.facesAdded = mesh.faceCount() - startFaces;
    result.verticesAdded = mesh.vertexCount() - startVerts;
    result.itemsFixed = 1;
    result.success = true;
    result.message = "Filled hole with " + std::to_string(result.facesAdded) + " faces";
    
    return result;
}

RepairResult MeshRepair::fillHoles(
    MeshData& mesh, 
    size_t maxEdges,
    ProgressCallback progress)
{
    RepairResult result;
    
    auto holes = detectHoles(mesh);
    
    if (holes.empty()) {
        result.message = "No holes detected";
        return result;
    }
    
    HoleFillOptions options;
    options.maxEdges = maxEdges;
    
    for (size_t i = 0; i < holes.size(); ++i) {
        if (progress) {
            float p = static_cast<float>(i) / holes.size();
            if (!progress(p)) {
                result.message = "Cancelled";
                return result;
            }
        }
        
        if (holes[i].boundaryVertices.size() <= maxEdges) {
            auto holeResult = fillHole(mesh, holes[i], options);
            result.facesAdded += holeResult.facesAdded;
            result.verticesAdded += holeResult.verticesAdded;
            result.itemsFixed += holeResult.itemsFixed;
        }
    }
    
    result.success = true;
    result.message = "Filled " + std::to_string(result.itemsFixed) + " holes";
    
    return result;
}

// ============================================================================
// Duplicate Vertex Handling
// ============================================================================

size_t MeshRepair::removeDuplicateVertices(
    MeshData& mesh, 
    float tolerance,
    ProgressCallback progress)
{
    if (mesh.isEmpty()) return 0;
    
    const auto& vertices = mesh.vertices();
    auto& indices = mesh.indices();
    
    // Use spatial hashing for efficiency
    SpatialGrid grid(tolerance * 10.0f);
    
    std::vector<uint32_t> vertexMap(vertices.size());
    std::vector<glm::vec3> newVertices;
    std::vector<glm::vec3> newNormals;
    
    float toleranceSq = tolerance * tolerance;
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (progress && i % 10000 == 0) {
            float p = static_cast<float>(i) / vertices.size() * 0.5f;
            if (!progress(p)) break;
        }
        
        const glm::vec3& v = vertices[i];
        
        // Query nearby vertices
        auto nearby = grid.query(v);
        
        uint32_t mergeTarget = INVALID_INDEX;
        for (uint32_t ni : nearby) {
            if (glm::length2(newVertices[ni] - v) < toleranceSq) {
                mergeTarget = ni;
                break;
            }
        }
        
        if (mergeTarget != INVALID_INDEX) {
            vertexMap[i] = mergeTarget;
        } else {
            vertexMap[i] = static_cast<uint32_t>(newVertices.size());
            grid.insert(static_cast<uint32_t>(newVertices.size()), v);
            newVertices.push_back(v);
            if (mesh.hasNormals()) {
                newNormals.push_back(mesh.normals()[i]);
            }
        }
    }
    
    // Update indices
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = vertexMap[indices[i]];
    }
    
    size_t removed = vertices.size() - newVertices.size();
    
    mesh.vertices() = std::move(newVertices);
    if (!newNormals.empty()) {
        mesh.normals() = std::move(newNormals);
    }
    
    // Remove degenerate faces created by merging
    removeDegenerateFaces(mesh);
    
    return removed;
}

// ============================================================================
// Degenerate Face Handling
// ============================================================================

std::vector<uint32_t> MeshRepair::detectDegenerateFaces(
    const MeshData& mesh, 
    float areaThreshold)
{
    std::vector<uint32_t> degenerate;
    
    const auto& indices = mesh.indices();
    
    for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
        uint32_t v0 = indices[fi * 3];
        uint32_t v1 = indices[fi * 3 + 1];
        uint32_t v2 = indices[fi * 3 + 2];
        
        // Check for duplicate vertices
        if (v0 == v1 || v1 == v2 || v2 == v0) {
            degenerate.push_back(static_cast<uint32_t>(fi));
            continue;
        }
        
        // Check area
        float area = mesh.faceArea(fi);
        if (area < areaThreshold) {
            degenerate.push_back(static_cast<uint32_t>(fi));
        }
    }
    
    return degenerate;
}

size_t MeshRepair::removeDegenerateFaces(MeshData& mesh) {
    auto degenerate = detectDegenerateFaces(mesh);
    
    if (degenerate.empty()) return 0;
    
    std::unordered_set<uint32_t> removeSet(degenerate.begin(), degenerate.end());
    
    const auto& indices = mesh.indices();
    std::vector<uint32_t> newIndices;
    newIndices.reserve(indices.size());
    
    for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
        if (!removeSet.count(static_cast<uint32_t>(fi))) {
            newIndices.push_back(indices[fi * 3]);
            newIndices.push_back(indices[fi * 3 + 1]);
            newIndices.push_back(indices[fi * 3 + 2]);
        }
    }
    
    mesh.indices() = std::move(newIndices);
    
    return degenerate.size();
}

// ============================================================================
// Manifold Repair
// ============================================================================

std::vector<std::pair<uint32_t, uint32_t>> MeshRepair::detectNonManifoldEdges(
    const MeshData& mesh)
{
    std::vector<std::pair<uint32_t, uint32_t>> nonManifold;
    
    std::unordered_map<EdgeKey, int, EdgeKeyHash> edgeCount;
    
    const auto& indices = mesh.indices();
    
    for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
        uint32_t v0 = indices[fi * 3];
        uint32_t v1 = indices[fi * 3 + 1];
        uint32_t v2 = indices[fi * 3 + 2];
        
        edgeCount[EdgeKey(v0, v1)]++;
        edgeCount[EdgeKey(v1, v2)]++;
        edgeCount[EdgeKey(v2, v0)]++;
    }
    
    for (const auto& [edge, count] : edgeCount) {
        if (count > 2) {
            nonManifold.emplace_back(edge.v0, edge.v1);
        }
    }
    
    return nonManifold;
}

std::vector<uint32_t> MeshRepair::detectNonManifoldVertices(
    const MeshData& mesh)
{
    std::vector<uint32_t> nonManifold;
    
    // Build half-edge mesh to check vertex neighborhoods
    auto heMeshResult = HalfEdgeMesh::buildFromMesh(mesh, nullptr);
    if (!heMeshResult.ok()) {
        return nonManifold;
    }
    
    return heMeshResult.value->findNonManifoldVertices();
}

bool MeshRepair::isManifold(const MeshData& mesh) {
    auto nonManifoldEdges = detectNonManifoldEdges(mesh);
    if (!nonManifoldEdges.empty()) return false;
    
    auto nonManifoldVertices = detectNonManifoldVertices(mesh);
    return nonManifoldVertices.empty();
}

RepairResult MeshRepair::makeManifold(
    MeshData& mesh,
    ProgressCallback progress)
{
    RepairResult result;
    
    // Step 1: Handle non-manifold edges (edges shared by > 2 faces)
    auto nmEdges = detectNonManifoldEdges(mesh);
    
    if (!nmEdges.empty()) {
        // For each non-manifold edge, duplicate one of its faces
        // This is a simplified approach - full implementation would be more sophisticated
        
        const auto& indices = mesh.indices();
        std::vector<uint32_t> newIndices;
        newIndices.reserve(indices.size());
        
        std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> edgeFaces;
        
        for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
            uint32_t v0 = indices[fi * 3];
            uint32_t v1 = indices[fi * 3 + 1];
            uint32_t v2 = indices[fi * 3 + 2];
            
            edgeFaces[EdgeKey(v0, v1)].push_back(static_cast<uint32_t>(fi));
            edgeFaces[EdgeKey(v1, v2)].push_back(static_cast<uint32_t>(fi));
            edgeFaces[EdgeKey(v2, v0)].push_back(static_cast<uint32_t>(fi));
        }
        
        std::unordered_set<uint32_t> removeFaces;
        
        for (const auto& [edge, faces] : edgeFaces) {
            if (faces.size() > 2) {
                // Keep first 2 faces, remove the rest
                for (size_t i = 2; i < faces.size(); ++i) {
                    removeFaces.insert(faces[i]);
                }
            }
        }
        
        for (size_t fi = 0; fi < indices.size() / 3; ++fi) {
            if (!removeFaces.count(static_cast<uint32_t>(fi))) {
                newIndices.push_back(indices[fi * 3]);
                newIndices.push_back(indices[fi * 3 + 1]);
                newIndices.push_back(indices[fi * 3 + 2]);
            }
        }
        
        result.itemsRemoved = removeFaces.size();
        mesh.indices() = std::move(newIndices);
    }
    
    // Step 2: Handle non-manifold vertices
    // This requires duplicating vertices, which is more complex
    // For now, we just report them
    
    auto nmVertices = detectNonManifoldVertices(mesh);
    result.itemsFixed = nmEdges.size();
    
    result.success = true;
    result.message = "Fixed " + std::to_string(nmEdges.size()) + " non-manifold edges, " +
                     std::to_string(nmVertices.size()) + " non-manifold vertices remaining";
    
    return result;
}

// ============================================================================
// Orientation
// ============================================================================

size_t MeshRepair::makeOrientationConsistent(MeshData& mesh) {
    if (mesh.isEmpty()) return 0;
    
    const auto& indices = mesh.indices();
    size_t faceCount = indices.size() / 3;
    
    // Build face adjacency
    std::unordered_map<EdgeKey, std::vector<std::pair<uint32_t, bool>>, EdgeKeyHash> edgeFaces;
    
    for (size_t fi = 0; fi < faceCount; ++fi) {
        uint32_t v0 = indices[fi * 3];
        uint32_t v1 = indices[fi * 3 + 1];
        uint32_t v2 = indices[fi * 3 + 2];
        
        // Store face and edge direction (v0 < v1 means forward)
        edgeFaces[EdgeKey(v0, v1)].emplace_back(static_cast<uint32_t>(fi), v0 < v1);
        edgeFaces[EdgeKey(v1, v2)].emplace_back(static_cast<uint32_t>(fi), v1 < v2);
        edgeFaces[EdgeKey(v2, v0)].emplace_back(static_cast<uint32_t>(fi), v2 < v0);
    }
    
    // BFS to propagate orientation
    std::vector<bool> visited(faceCount, false);
    std::vector<bool> flipped(faceCount, false);
    size_t flipCount = 0;
    
    for (size_t startFace = 0; startFace < faceCount; ++startFace) {
        if (visited[startFace]) continue;
        
        std::queue<uint32_t> queue;
        queue.push(static_cast<uint32_t>(startFace));
        visited[startFace] = true;
        
        while (!queue.empty()) {
            uint32_t fi = queue.front();
            queue.pop();
            
            uint32_t fv0 = indices[fi * 3];
            uint32_t fv1 = indices[fi * 3 + 1];
            uint32_t fv2 = indices[fi * 3 + 2];
            
            EdgeKey edges[3] = {
                EdgeKey(fv0, fv1),
                EdgeKey(fv1, fv2),
                EdgeKey(fv2, fv0)
            };
            
            bool edgeDirs[3] = {
                fv0 < fv1,
                fv1 < fv2,
                fv2 < fv0
            };
            
            for (int ei = 0; ei < 3; ++ei) {
                auto it = edgeFaces.find(edges[ei]);
                if (it == edgeFaces.end()) continue;
                
                for (const auto& [neighborFi, neighborDir] : it->second) {
                    if (neighborFi == fi) continue;
                    if (visited[neighborFi]) continue;
                    
                    visited[neighborFi] = true;
                    
                    // Check if orientations are consistent
                    // Adjacent faces should have opposite edge directions
                    bool currentDir = edgeDirs[ei] != flipped[fi];
                    bool neighborNeedsFlip = (currentDir == neighborDir);
                    
                    if (neighborNeedsFlip) {
                        flipped[neighborFi] = true;
                        ++flipCount;
                    }
                    
                    queue.push(neighborFi);
                }
            }
        }
    }
    
    // Apply flips
    if (flipCount > 0) {
        auto& mutableIndices = mesh.indices();
        for (size_t fi = 0; fi < faceCount; ++fi) {
            if (flipped[fi]) {
                std::swap(mutableIndices[fi * 3 + 1], mutableIndices[fi * 3 + 2]);
            }
        }
        mesh.computeNormals();
    }
    
    return flipCount;
}

bool MeshRepair::orientOutward(MeshData& mesh) {
    // First make consistent
    makeOrientationConsistent(mesh);
    
    // Check if volume is positive (outward) or negative (inward)
    float vol = mesh.volume();
    
    if (vol < 0) {
        // Flip all faces
        mesh.flipNormals();
        return true;
    }
    
    return true;
}

// ============================================================================
// Comprehensive Repair
// ============================================================================

RepairResult MeshRepair::repairAll(
    MeshData& mesh,
    bool fillSmallHoles,
    ProgressCallback progress)
{
    RepairResult result;
    
    // Step 1: Remove duplicate vertices
    if (progress) progress(0.1f);
    size_t dupsRemoved = removeDuplicateVertices(mesh, 1e-6f, nullptr);
    
    // Step 2: Remove degenerate faces
    if (progress) progress(0.2f);
    size_t degensRemoved = removeDegenerateFaces(mesh);
    
    // Step 3: Remove outliers
    if (progress) progress(0.4f);
    auto outlierResult = removeOutliers(mesh, 0.01f, nullptr);
    
    // Step 4: Make manifold
    if (progress) progress(0.6f);
    auto manifoldResult = makeManifold(mesh, nullptr);
    
    // Step 5: Make orientation consistent
    if (progress) progress(0.8f);
    size_t flipped = makeOrientationConsistent(mesh);
    
    // Step 6: Fill small holes
    RepairResult holeResult;
    if (fillSmallHoles) {
        if (progress) progress(0.9f);
        holeResult = fillHoles(mesh, 20, nullptr);
    }
    
    // Recompute normals
    mesh.computeNormals();
    
    if (progress) progress(1.0f);
    
    result.success = true;
    result.itemsRemoved = dupsRemoved + degensRemoved + outlierResult.itemsRemoved + 
                          manifoldResult.itemsRemoved;
    result.itemsFixed = manifoldResult.itemsFixed + flipped + holeResult.itemsFixed;
    result.facesAdded = holeResult.facesAdded;
    result.verticesAdded = holeResult.verticesAdded;
    
    std::ostringstream ss;
    ss << "Repair complete: "
       << dupsRemoved << " duplicate vertices, "
       << degensRemoved << " degenerate faces, "
       << outlierResult.itemsRemoved << " outlier faces removed, "
       << flipped << " faces flipped, "
       << holeResult.itemsFixed << " holes filled";
    result.message = ss.str();
    
    return result;
}

// ============================================================================
// Mesh Health Analysis
// ============================================================================

std::string MeshHealthReport::summary() const {
    std::ostringstream ss;
    
    ss << "Mesh Health Report:\n"
       << "  Valid: " << (isValid ? "Yes" : "No") << "\n"
       << "  Manifold: " << (isManifold ? "Yes" : "No") << "\n"
       << "  Closed: " << (isClosed ? "Yes" : "No") << "\n"
       << "  Orientable: " << (isOrientable ? "Yes" : "No") << "\n"
       << "\nIssues found:\n"
       << "  Duplicate vertices: " << duplicateVertices << "\n"
       << "  Degenerate faces: " << degenerateFaces << "\n"
       << "  Non-manifold edges: " << nonManifoldEdges << "\n"
       << "  Non-manifold vertices: " << nonManifoldVertices << "\n"
       << "  Boundary edges: " << boundaryEdges << "\n"
       << "  Holes: " << holeCount << "\n"
       << "  Connected components: " << componentCount;
    
    return ss.str();
}

MeshHealthReport analyzeMeshHealth(
    const MeshData& mesh,
    ProgressCallback progress)
{
    MeshHealthReport report;
    
    if (mesh.isEmpty()) {
        return report;
    }
    
    report.isValid = mesh.isValid();
    
    if (progress) progress(0.1f);
    
    // Count duplicates
    report.duplicateVertices = mesh.countDuplicateVertices();
    
    if (progress) progress(0.2f);
    
    // Count degenerates
    report.degenerateFaces = mesh.countDegenerateFaces();
    
    if (progress) progress(0.3f);
    
    // Non-manifold edges
    auto nmEdges = MeshRepair::detectNonManifoldEdges(mesh);
    report.nonManifoldEdges = nmEdges.size();
    
    if (progress) progress(0.5f);
    
    // Non-manifold vertices
    auto nmVertices = MeshRepair::detectNonManifoldVertices(mesh);
    report.nonManifoldVertices = nmVertices.size();
    
    report.isManifold = (report.nonManifoldEdges == 0 && report.nonManifoldVertices == 0);
    
    if (progress) progress(0.7f);
    
    // Holes and boundaries
    auto holes = MeshRepair::detectHoles(mesh);
    report.holeCount = holes.size();
    
    for (const auto& hole : holes) {
        report.boundaryEdges += hole.boundaryVertices.size();
    }
    
    report.isClosed = (report.boundaryEdges == 0);
    
    if (progress) progress(0.9f);
    
    // Components
    auto components = MeshRepair::findConnectedComponents(mesh);
    report.componentCount = components.size();
    
    // Orientability (check if consistent orientation is possible)
    report.isOrientable = report.isManifold;  // Manifold implies orientable for our purposes
    
    if (progress) progress(1.0f);
    
    return report;
}

} // namespace geometry
} // namespace dc3d
