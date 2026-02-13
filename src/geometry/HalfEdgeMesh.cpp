/**
 * @file HalfEdgeMesh.cpp
 * @brief Implementation of HalfEdgeMesh half-edge data structure
 */

#include "HalfEdgeMesh.h"

#include <algorithm>
#include <unordered_map>
#include <set>
#include <queue>
#include <sstream>

namespace dc3d {
namespace geometry {

Result<HalfEdgeMesh> HalfEdgeMesh::buildFromMesh(const MeshData& mesh, 
                                                  ProgressCallback progress) {
    return buildFromTriangles(mesh.vertices(), mesh.indices(), progress);
}

Result<HalfEdgeMesh> HalfEdgeMesh::buildFromTriangles(
    const std::vector<glm::vec3>& vertices,
    const std::vector<uint32_t>& indices,
    ProgressCallback progress) {
    
    if (indices.size() % 3 != 0) {
        return Result<HalfEdgeMesh>::failure("Index count must be a multiple of 3");
    }
    
    HalfEdgeMesh mesh;
    const size_t numVertices = vertices.size();
    const size_t numFaces = indices.size() / 3;
    
    const bool reportProgress = progress && numFaces > 1000000;
    
    // Initialize vertices
    mesh.vertices_.resize(numVertices);
    for (size_t i = 0; i < numVertices; ++i) {
        mesh.vertices_[i].position = vertices[i];
        mesh.vertices_[i].halfEdge = INVALID_INDEX;
    }
    
    // Reserve space for half-edges (3 per face)
    mesh.halfEdges_.reserve(numFaces * 3);
    mesh.faces_.resize(numFaces);
    
    // Map from edge (v0, v1) to half-edge index
    // Key is (min(v0,v1), max(v0,v1)), value is pair of half-edge indices
    std::unordered_map<EdgeKey, std::pair<uint32_t, uint32_t>, EdgeKeyHash> edgeMap;
    edgeMap.reserve(numFaces * 3);
    
    // First pass: create half-edges and faces
    for (size_t f = 0; f < numFaces; ++f) {
        uint32_t v0 = indices[f * 3 + 0];
        uint32_t v1 = indices[f * 3 + 1];
        uint32_t v2 = indices[f * 3 + 2];
        
        // Validate indices
        if (v0 >= numVertices || v1 >= numVertices || v2 >= numVertices) {
            return Result<HalfEdgeMesh>::failure(
                "Invalid vertex index in face " + std::to_string(f));
        }
        
        // Check for degenerate faces
        if (v0 == v1 || v1 == v2 || v2 == v0) {
            // Skip degenerate face
            mesh.faces_[f].halfEdge = INVALID_INDEX;
            continue;
        }
        
        // Create three half-edges for this face
        uint32_t he0 = static_cast<uint32_t>(mesh.halfEdges_.size());
        uint32_t he1 = he0 + 1;
        uint32_t he2 = he0 + 2;
        
        mesh.halfEdges_.push_back(HalfEdge{});
        mesh.halfEdges_.push_back(HalfEdge{});
        mesh.halfEdges_.push_back(HalfEdge{});
        
        uint32_t faceIdx = static_cast<uint32_t>(f);
        
        // Setup half-edge 0: v0 -> v1
        mesh.halfEdges_[he0].vertex = v1;
        mesh.halfEdges_[he0].face = faceIdx;
        mesh.halfEdges_[he0].next = he1;
        mesh.halfEdges_[he0].prev = he2;
        
        // Setup half-edge 1: v1 -> v2
        mesh.halfEdges_[he1].vertex = v2;
        mesh.halfEdges_[he1].face = faceIdx;
        mesh.halfEdges_[he1].next = he2;
        mesh.halfEdges_[he1].prev = he0;
        
        // Setup half-edge 2: v2 -> v0
        mesh.halfEdges_[he2].vertex = v0;
        mesh.halfEdges_[he2].face = faceIdx;
        mesh.halfEdges_[he2].next = he0;
        mesh.halfEdges_[he2].prev = he1;
        
        // Set face's half-edge
        mesh.faces_[f].halfEdge = he0;
        
        // Set vertex half-edges (one outgoing half-edge per vertex)
        if (mesh.vertices_[v0].halfEdge == INVALID_INDEX) {
            mesh.vertices_[v0].halfEdge = he0;
        }
        if (mesh.vertices_[v1].halfEdge == INVALID_INDEX) {
            mesh.vertices_[v1].halfEdge = he1;
        }
        if (mesh.vertices_[v2].halfEdge == INVALID_INDEX) {
            mesh.vertices_[v2].halfEdge = he2;
        }
        
        // Register edges in map for twin linking
        // Edge v0-v1
        {
            EdgeKey key(v0, v1);
            auto it = edgeMap.find(key);
            if (it == edgeMap.end()) {
                edgeMap[key] = {he0, INVALID_INDEX};
            } else {
                // FIX Bug 14: Check for non-manifold edge (already has twin)
                if (it->second.second != INVALID_INDEX) {
                    // Edge already has twin - this is a non-manifold edge (>2 faces share it)
                    // Log or track for later handling but continue building mesh
                    // The edge will only link the first two faces found
                } else {
                    // Found twin - link them
                    uint32_t twinHe = it->second.first;
                    mesh.halfEdges_[he0].twin = twinHe;
                    mesh.halfEdges_[twinHe].twin = he0;
                    it->second.second = he0;  // Mark as paired
                }
            }
        }
        
        // Edge v1-v2
        {
            EdgeKey key(v1, v2);
            auto it = edgeMap.find(key);
            if (it == edgeMap.end()) {
                edgeMap[key] = {he1, INVALID_INDEX};
            } else {
                // FIX Bug 14: Check for non-manifold edge
                if (it->second.second != INVALID_INDEX) {
                    // Non-manifold edge - skip linking third+ occurrence
                } else {
                    uint32_t twinHe = it->second.first;
                    mesh.halfEdges_[he1].twin = twinHe;
                    mesh.halfEdges_[twinHe].twin = he1;
                    it->second.second = he1;
                }
            }
        }
        
        // Edge v2-v0
        {
            EdgeKey key(v2, v0);
            auto it = edgeMap.find(key);
            if (it == edgeMap.end()) {
                edgeMap[key] = {he2, INVALID_INDEX};
            } else {
                // FIX Bug 14: Check for non-manifold edge
                if (it->second.second != INVALID_INDEX) {
                    // Non-manifold edge - skip linking third+ occurrence
                } else {
                    uint32_t twinHe = it->second.first;
                    mesh.halfEdges_[he2].twin = twinHe;
                    mesh.halfEdges_[twinHe].twin = he2;
                    it->second.second = he2;
                }
            }
        }
        
        if (reportProgress && (f % 100000 == 0)) {
            if (!progress(static_cast<float>(f) / numFaces)) {
                return Result<HalfEdgeMesh>::failure("Operation cancelled");
            }
        }
    }
    
    // Compute normals
    mesh.computeFaceNormals();
    mesh.computeVertexNormals();
    
    if (progress) {
        progress(1.0f);
    }
    
    return Result<HalfEdgeMesh>::success(std::move(mesh));
}

MeshData HalfEdgeMesh::toMeshData() const {
    MeshData mesh;
    
    if (isEmpty()) return mesh;
    
    // Copy vertices
    mesh.vertices().reserve(vertices_.size());
    for (const auto& v : vertices_) {
        mesh.vertices().push_back(v.position);
    }
    
    // Copy normals
    mesh.normals().reserve(vertices_.size());
    for (const auto& v : vertices_) {
        mesh.normals().push_back(v.normal);
    }
    
    // Build indices from faces
    mesh.indices().reserve(faces_.size() * 3);
    for (const auto& face : faces_) {
        if (!face.isValid()) continue;
        
        auto verts = faceVertices(static_cast<uint32_t>(&face - &faces_[0]));
        if (verts.size() == 3) {
            mesh.indices().push_back(verts[0]);
            mesh.indices().push_back(verts[1]);
            mesh.indices().push_back(verts[2]);
        }
    }
    
    return mesh;
}

std::vector<uint32_t> HalfEdgeMesh::vertexNeighbors(uint32_t vertexIdx) const {
    std::vector<uint32_t> neighbors;
    
    if (vertexIdx >= vertices_.size()) return neighbors;
    
    uint32_t startHe = vertices_[vertexIdx].halfEdge;
    if (startHe == INVALID_INDEX) return neighbors;
    
    // FIX: Add iteration limit to prevent infinite loop on corrupted mesh
    size_t maxIter = halfEdges_.size();
    size_t iter = 0;
    
    uint32_t he = startHe;
    do {
        // FIX: Check for mesh corruption
        if (++iter > maxIter) {
            // Mesh topology is corrupted - break to prevent infinite loop
            break;
        }
        
        // The target of this half-edge is a neighbor
        neighbors.push_back(halfEdges_[he].vertex);
        
        // Move to next outgoing half-edge
        uint32_t twin = halfEdges_[he].twin;
        if (twin == INVALID_INDEX) {
            // Boundary - need to go the other way
            break;
        }
        he = halfEdges_[twin].next;
    } while (he != startHe);
    
    // If we hit a boundary, we need to also traverse backwards
    if (halfEdges_[startHe].twin == INVALID_INDEX || 
        (halfEdges_[halfEdges_[startHe].twin].next != startHe)) {
        // Traverse in reverse direction
        he = halfEdges_[startHe].prev;
        while (he != INVALID_INDEX) {
            uint32_t twin = halfEdges_[he].twin;
            if (twin == INVALID_INDEX) {
                // Add the source of this edge as a neighbor
                neighbors.push_back(halfEdgeSource(he));
                break;
            }
            neighbors.push_back(halfEdges_[twin].vertex);
            he = halfEdges_[twin].prev;
            if (he == halfEdges_[startHe].prev) break;  // Full circle
        }
    }
    
    return neighbors;
}

std::vector<uint32_t> HalfEdgeMesh::vertexFaces(uint32_t vertexIdx) const {
    std::vector<uint32_t> faces;
    
    if (vertexIdx >= vertices_.size()) return faces;
    
    uint32_t startHe = vertices_[vertexIdx].halfEdge;
    if (startHe == INVALID_INDEX) return faces;
    
    uint32_t he = startHe;
    do {
        uint32_t f = halfEdges_[he].face;
        if (f != INVALID_INDEX) {
            faces.push_back(f);
        }
        
        uint32_t twin = halfEdges_[he].twin;
        if (twin == INVALID_INDEX) break;
        he = halfEdges_[twin].next;
    } while (he != startHe);
    
    return faces;
}

std::vector<uint32_t> HalfEdgeMesh::vertexOutgoingEdges(uint32_t vertexIdx) const {
    std::vector<uint32_t> edges;
    
    if (vertexIdx >= vertices_.size()) return edges;
    
    uint32_t startHe = vertices_[vertexIdx].halfEdge;
    if (startHe == INVALID_INDEX) return edges;
    
    uint32_t he = startHe;
    do {
        edges.push_back(he);
        
        uint32_t twin = halfEdges_[he].twin;
        if (twin == INVALID_INDEX) break;
        he = halfEdges_[twin].next;
    } while (he != startHe);
    
    return edges;
}

std::vector<uint32_t> HalfEdgeMesh::faceNeighbors(uint32_t faceIdx) const {
    std::vector<uint32_t> neighbors;
    
    if (faceIdx >= faces_.size()) return neighbors;
    if (!faces_[faceIdx].isValid()) return neighbors;
    
    uint32_t startHe = faces_[faceIdx].halfEdge;
    uint32_t he = startHe;
    
    do {
        uint32_t twin = halfEdges_[he].twin;
        if (twin != INVALID_INDEX) {
            uint32_t neighborFace = halfEdges_[twin].face;
            if (neighborFace != INVALID_INDEX) {
                neighbors.push_back(neighborFace);
            }
        }
        he = halfEdges_[he].next;
    } while (he != startHe);
    
    return neighbors;
}

std::vector<uint32_t> HalfEdgeMesh::faceVertices(uint32_t faceIdx) const {
    std::vector<uint32_t> verts;
    
    if (faceIdx >= faces_.size()) return verts;
    if (!faces_[faceIdx].isValid()) return verts;
    
    uint32_t startHe = faces_[faceIdx].halfEdge;
    uint32_t he = startHe;
    
    // FIX Bug 26: Document safety limit assumption for triangle meshes
    constexpr size_t MAX_FACE_VERTICES = 4;  // For triangle meshes; increase for n-gon support
    do {
        verts.push_back(halfEdges_[he].vertex);
        he = halfEdges_[he].next;
    } while (he != startHe && verts.size() < MAX_FACE_VERTICES);
    
    return verts;
}

std::vector<uint32_t> HalfEdgeMesh::faceHalfEdges(uint32_t faceIdx) const {
    std::vector<uint32_t> edges;
    
    if (faceIdx >= faces_.size()) return edges;
    if (!faces_[faceIdx].isValid()) return edges;
    
    uint32_t startHe = faces_[faceIdx].halfEdge;
    uint32_t he = startHe;
    
    // FIX Bug 26: Document that safety limit of 4 assumes triangles only
    // If quad support is added, increase this limit accordingly
    constexpr size_t MAX_FACE_VERTICES = 4;  // For triangle meshes; increase for n-gon support
    do {
        edges.push_back(he);
        he = halfEdges_[he].next;
    } while (he != startHe && edges.size() < MAX_FACE_VERTICES);
    
    return edges;
}

uint32_t HalfEdgeMesh::halfEdgeSource(uint32_t heIdx) const {
    if (heIdx >= halfEdges_.size()) return INVALID_INDEX;
    
    // Source of he is target of prev
    uint32_t prev = halfEdges_[heIdx].prev;
    if (prev == INVALID_INDEX) return INVALID_INDEX;
    
    return halfEdges_[prev].vertex;
}

uint32_t HalfEdgeMesh::findHalfEdge(uint32_t fromVertex, uint32_t toVertex) const {
    if (fromVertex >= vertices_.size()) return INVALID_INDEX;
    
    uint32_t startHe = vertices_[fromVertex].halfEdge;
    if (startHe == INVALID_INDEX) return INVALID_INDEX;
    
    uint32_t he = startHe;
    do {
        if (halfEdges_[he].vertex == toVertex) {
            return he;
        }
        
        uint32_t twin = halfEdges_[he].twin;
        if (twin == INVALID_INDEX) break;
        he = halfEdges_[twin].next;
    } while (he != startHe);
    
    return INVALID_INDEX;
}

bool HalfEdgeMesh::isVertexOnBoundary(uint32_t vertexIdx) const {
    if (vertexIdx >= vertices_.size()) return false;
    
    uint32_t startHe = vertices_[vertexIdx].halfEdge;
    if (startHe == INVALID_INDEX) return true;  // Isolated vertex
    
    uint32_t he = startHe;
    do {
        if (halfEdges_[he].twin == INVALID_INDEX) {
            return true;
        }
        he = halfEdges_[halfEdges_[he].twin].next;
    } while (he != startHe);
    
    return false;
}

bool HalfEdgeMesh::isBoundaryEdge(uint32_t heIdx) const {
    if (heIdx >= halfEdges_.size()) return false;
    return halfEdges_[heIdx].twin == INVALID_INDEX;
}

std::vector<std::vector<uint32_t>> HalfEdgeMesh::findBoundaryLoops() const {
    std::vector<std::vector<uint32_t>> loops;
    std::vector<bool> visited(halfEdges_.size(), false);
    
    for (size_t i = 0; i < halfEdges_.size(); ++i) {
        if (halfEdges_[i].twin != INVALID_INDEX) continue;  // Not boundary
        if (visited[i]) continue;  // Already processed
        
        // Found unvisited boundary edge - trace the loop
        std::vector<uint32_t> loop;
        uint32_t he = static_cast<uint32_t>(i);
        
        while (!visited[he]) {
            visited[he] = true;
            loop.push_back(halfEdges_[he].vertex);
            
            // Find next boundary edge in the loop
            uint32_t next = halfEdges_[he].next;
            while (halfEdges_[next].twin != INVALID_INDEX) {
                next = halfEdges_[halfEdges_[next].twin].next;
            }
            he = next;
        }
        
        if (!loop.empty()) {
            loops.push_back(std::move(loop));
        }
    }
    
    return loops;
}

std::vector<uint32_t> HalfEdgeMesh::findBoundaryEdges() const {
    std::vector<uint32_t> boundary;
    
    for (size_t i = 0; i < halfEdges_.size(); ++i) {
        if (halfEdges_[i].twin == INVALID_INDEX) {
            boundary.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return boundary;
}

size_t HalfEdgeMesh::boundaryEdgeCount() const {
    size_t count = 0;
    for (const auto& he : halfEdges_) {
        if (he.twin == INVALID_INDEX) {
            ++count;
        }
    }
    return count;
}

bool HalfEdgeMesh::isManifold() const {
    // Check for non-manifold edges and vertices
    return findNonManifoldEdges().empty() && findNonManifoldVertices().empty();
}

std::vector<uint32_t> HalfEdgeMesh::findNonManifoldVertices() const {
    std::vector<uint32_t> nonManifold;
    
    for (size_t v = 0; v < vertices_.size(); ++v) {
        if (vertices_[v].halfEdge == INVALID_INDEX) continue;
        
        // Count the number of boundary edges at this vertex
        // A manifold vertex has at most 2 boundary edges
        int boundaryCount = 0;
        uint32_t he = vertices_[v].halfEdge;
        uint32_t start = he;
        
        // Try to circulate around the vertex
        std::set<uint32_t> visitedEdges;
        bool failed = false;
        
        do {
            if (visitedEdges.count(he)) {
                // Cycle detected that doesn't complete properly
                failed = true;
                break;
            }
            visitedEdges.insert(he);
            
            if (halfEdges_[he].twin == INVALID_INDEX) {
                ++boundaryCount;
                if (boundaryCount > 2) {
                    nonManifold.push_back(static_cast<uint32_t>(v));
                    break;
                }
                break;  // Can't continue past boundary
            }
            
            he = halfEdges_[halfEdges_[he].twin].next;
        } while (he != start);
        
        if (failed) {
            nonManifold.push_back(static_cast<uint32_t>(v));
        }
    }
    
    return nonManifold;
}

std::vector<uint32_t> HalfEdgeMesh::findNonManifoldEdges() const {
    // Non-manifold edges are shared by more than 2 faces
    // With our construction, this shouldn't happen, but we check anyway
    
    std::unordered_map<EdgeKey, int, EdgeKeyHash> edgeFaceCount;
    
    for (size_t f = 0; f < faces_.size(); ++f) {
        if (!faces_[f].isValid()) continue;
        
        auto verts = faceVertices(static_cast<uint32_t>(f));
        if (verts.size() < 3) continue;
        
        for (size_t i = 0; i < verts.size(); ++i) {
            EdgeKey key(verts[i], verts[(i + 1) % verts.size()]);
            edgeFaceCount[key]++;
        }
    }
    
    std::vector<uint32_t> nonManifold;
    for (size_t i = 0; i < halfEdges_.size(); ++i) {
        uint32_t v0 = halfEdgeSource(static_cast<uint32_t>(i));
        uint32_t v1 = halfEdges_[i].vertex;
        
        if (v0 == INVALID_INDEX) continue;
        
        EdgeKey key(v0, v1);
        auto it = edgeFaceCount.find(key);
        if (it != edgeFaceCount.end() && it->second > 2) {
            nonManifold.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return nonManifold;
}

void HalfEdgeMesh::computeFaceNormals() {
    for (size_t f = 0; f < faces_.size(); ++f) {
        if (!faces_[f].isValid()) continue;
        
        auto verts = faceVertices(static_cast<uint32_t>(f));
        if (verts.size() < 3) continue;
        
        const glm::vec3& v0 = vertices_[verts[0]].position;
        const glm::vec3& v1 = vertices_[verts[1]].position;
        const glm::vec3& v2 = vertices_[verts[2]].position;
        
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        float len = glm::length(normal);
        
        faces_[f].normal = len > 1e-10f ? normal / len : glm::vec3(0.0f, 0.0f, 1.0f);
    }
}

void HalfEdgeMesh::computeVertexNormals() {
    // Initialize vertex normals to zero
    for (auto& v : vertices_) {
        v.normal = glm::vec3(0.0f);
    }
    
    // Accumulate face normals at vertices (area-weighted)
    for (size_t f = 0; f < faces_.size(); ++f) {
        if (!faces_[f].isValid()) continue;
        
        auto verts = faceVertices(static_cast<uint32_t>(f));
        if (verts.size() < 3) continue;
        
        const glm::vec3& v0 = vertices_[verts[0]].position;
        const glm::vec3& v1 = vertices_[verts[1]].position;
        const glm::vec3& v2 = vertices_[verts[2]].position;
        
        // Cross product gives area-weighted normal
        glm::vec3 faceNormal = glm::cross(v1 - v0, v2 - v0);
        
        for (uint32_t vi : verts) {
            vertices_[vi].normal += faceNormal;
        }
    }
    
    // Normalize
    for (auto& v : vertices_) {
        float len = glm::length(v.normal);
        if (len > 1e-10f) {
            v.normal /= len;
        } else {
            v.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }
}

BoundingBox HalfEdgeMesh::boundingBox() const {
    BoundingBox bounds;
    for (const auto& v : vertices_) {
        bounds.expand(v.position);
    }
    return bounds;
}

size_t HalfEdgeMesh::vertexValence(uint32_t vertexIdx) const {
    return vertexOutgoingEdges(vertexIdx).size();
}

std::string HalfEdgeMesh::validate() const {
    std::stringstream errors;
    
    // Check all half-edge next/prev pointers form valid loops
    for (size_t i = 0; i < halfEdges_.size(); ++i) {
        const auto& he = halfEdges_[i];
        
        if (he.next != INVALID_INDEX) {
            if (he.next >= halfEdges_.size()) {
                errors << "Half-edge " << i << " has invalid next index\n";
            } else if (halfEdges_[he.next].prev != i) {
                errors << "Half-edge " << i << " next/prev mismatch\n";
            }
        }
        
        if (he.prev != INVALID_INDEX) {
            if (he.prev >= halfEdges_.size()) {
                errors << "Half-edge " << i << " has invalid prev index\n";
            }
        }
        
        if (he.twin != INVALID_INDEX) {
            if (he.twin >= halfEdges_.size()) {
                errors << "Half-edge " << i << " has invalid twin index\n";
            } else if (halfEdges_[he.twin].twin != i) {
                errors << "Half-edge " << i << " twin mismatch\n";
            }
        }
        
        if (he.vertex != INVALID_INDEX && he.vertex >= vertices_.size()) {
            errors << "Half-edge " << i << " has invalid vertex index\n";
        }
        
        if (he.face != INVALID_INDEX && he.face >= faces_.size()) {
            errors << "Half-edge " << i << " has invalid face index\n";
        }
    }
    
    // Check all vertex half-edges are valid
    for (size_t i = 0; i < vertices_.size(); ++i) {
        uint32_t he = vertices_[i].halfEdge;
        if (he != INVALID_INDEX && he >= halfEdges_.size()) {
            errors << "Vertex " << i << " has invalid half-edge index\n";
        }
    }
    
    // Check all face half-edges are valid
    for (size_t i = 0; i < faces_.size(); ++i) {
        uint32_t he = faces_[i].halfEdge;
        if (he != INVALID_INDEX) {
            if (he >= halfEdges_.size()) {
                errors << "Face " << i << " has invalid half-edge index\n";
            } else if (halfEdges_[he].face != i) {
                errors << "Face " << i << " half-edge doesn't point back\n";
            }
        }
    }
    
    return errors.str();
}

bool HalfEdgeMesh::checkConsistency() const {
    return validate().empty();
}

// ===================
// VertexCirculator
// ===================

VertexCirculator::VertexCirculator(const HalfEdgeMesh& mesh, uint32_t vertexIdx)
    : mesh_(mesh), done_(false) {
    
    if (vertexIdx >= mesh.vertexCount()) {
        start_ = INVALID_INDEX;
        current_ = INVALID_INDEX;
        done_ = true;
        return;
    }
    
    start_ = mesh.vertex(vertexIdx).halfEdge;
    current_ = start_;
    
    if (current_ == INVALID_INDEX) {
        done_ = true;
    }
}

uint32_t VertexCirculator::vertexIndex() const {
    if (current_ == INVALID_INDEX) return INVALID_INDEX;
    return mesh_.halfEdge(current_).vertex;
}

uint32_t VertexCirculator::faceIndex() const {
    if (current_ == INVALID_INDEX) return INVALID_INDEX;
    return mesh_.halfEdge(current_).face;
}

VertexCirculator& VertexCirculator::operator++() {
    if (done_ || current_ == INVALID_INDEX) return *this;
    
    uint32_t twin = mesh_.halfEdge(current_).twin;
    if (twin == INVALID_INDEX) {
        done_ = true;
        return *this;
    }
    
    current_ = mesh_.halfEdge(twin).next;
    
    if (current_ == start_) {
        done_ = true;
    }
    
    return *this;
}

// ===================
// FaceCirculator
// ===================

FaceCirculator::FaceCirculator(const HalfEdgeMesh& mesh, uint32_t faceIdx)
    : mesh_(mesh), done_(false) {
    
    if (faceIdx >= mesh.faceCount() || !mesh.face(faceIdx).isValid()) {
        start_ = INVALID_INDEX;
        current_ = INVALID_INDEX;
        done_ = true;
        return;
    }
    
    start_ = mesh.face(faceIdx).halfEdge;
    current_ = start_;
}

uint32_t FaceCirculator::vertexIndex() const {
    if (current_ == INVALID_INDEX) return INVALID_INDEX;
    return mesh_.halfEdge(current_).vertex;
}

FaceCirculator& FaceCirculator::operator++() {
    if (done_ || current_ == INVALID_INDEX) return *this;
    
    current_ = mesh_.halfEdge(current_).next;
    
    if (current_ == start_) {
        done_ = true;
    }
    
    return *this;
}

} // namespace geometry
} // namespace dc3d
