/**
 * @file HalfEdgeMesh.h
 * @brief Half-edge mesh data structure for topological operations
 * 
 * Provides efficient adjacency queries and boundary detection
 * for mesh algorithms that require connectivity information.
 */

#pragma once

#include "MeshData.h"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <limits>

#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

// Forward declarations
class HalfEdgeMesh;

/// Invalid index constant
constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

/**
 * @brief Vertex in half-edge mesh
 */
struct HEVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 0.0f, 1.0f};
    uint32_t halfEdge = INVALID_INDEX;  ///< One outgoing half-edge (or INVALID if isolated)
    
    bool isIsolated() const { return halfEdge == INVALID_INDEX; }
};

/**
 * @brief Half-edge connecting two vertices
 * 
 * Each directed edge v0->v1 has a half-edge. If the mesh is closed,
 * there's also a twin edge v1->v0.
 */
struct HalfEdge {
    uint32_t vertex = INVALID_INDEX;    ///< Target vertex (edge points TO this vertex)
    uint32_t face = INVALID_INDEX;      ///< Adjacent face (or INVALID if boundary)
    uint32_t next = INVALID_INDEX;      ///< Next half-edge in face loop (CCW)
    uint32_t prev = INVALID_INDEX;      ///< Previous half-edge in face loop
    uint32_t twin = INVALID_INDEX;      ///< Opposite half-edge (or INVALID if boundary)
    
    bool isBoundary() const { return twin == INVALID_INDEX; }
};

/**
 * @brief Face in half-edge mesh (always triangular)
 */
struct HEFace {
    uint32_t halfEdge = INVALID_INDEX;  ///< One half-edge on the boundary
    glm::vec3 normal{0.0f, 0.0f, 1.0f}; ///< Face normal
    
    bool isValid() const { return halfEdge != INVALID_INDEX; }
};

/**
 * @brief Half-edge mesh data structure for topological operations
 * 
 * This structure maintains full connectivity information:
 * - Vertex to outgoing half-edge
 * - Half-edge to target vertex, face, next/prev/twin edges
 * - Face to one half-edge on its boundary
 * 
 * Enables O(1) access to:
 * - Vertices around a vertex (1-ring)
 * - Faces around a vertex
 * - Vertices/edges of a face
 * - Neighboring faces of a face
 * 
 * Note: This mesh is always triangular. Non-triangular faces
 * should be triangulated before conversion.
 */
class HalfEdgeMesh {
public:
    HalfEdgeMesh() = default;
    ~HalfEdgeMesh() = default;
    
    HalfEdgeMesh(HalfEdgeMesh&&) noexcept = default;
    HalfEdgeMesh& operator=(HalfEdgeMesh&&) noexcept = default;
    
    HalfEdgeMesh(const HalfEdgeMesh&) = default;
    HalfEdgeMesh& operator=(const HalfEdgeMesh&) = default;
    
    // ===================
    // Construction
    // ===================
    
    /**
     * @brief Build half-edge mesh from triangle soup
     * @param mesh Source mesh data
     * @param progress Optional progress callback
     * @return Result with HalfEdgeMesh or error message
     */
    static Result<HalfEdgeMesh> buildFromMesh(const MeshData& mesh, 
                                              ProgressCallback progress = nullptr);
    
    /**
     * @brief Build half-edge mesh from raw vertex/index data
     * @param vertices Vertex positions
     * @param indices Triangle indices (3 per face)
     * @param progress Optional progress callback
     * @return Result with HalfEdgeMesh or error message
     */
    static Result<HalfEdgeMesh> buildFromTriangles(
        const std::vector<glm::vec3>& vertices,
        const std::vector<uint32_t>& indices,
        ProgressCallback progress = nullptr);
    
    /**
     * @brief Convert back to simple triangle mesh
     * @return MeshData with vertices, indices, and computed normals
     */
    MeshData toMeshData() const;
    
    // ===================
    // Basic Queries
    // ===================
    
    size_t vertexCount() const { return vertices_.size(); }
    size_t halfEdgeCount() const { return halfEdges_.size(); }
    size_t faceCount() const { return faces_.size(); }
    size_t edgeCount() const { return halfEdges_.size() / 2; }  // Approximate
    
    bool isEmpty() const { return vertices_.empty(); }
    
    const HEVertex& vertex(uint32_t idx) const { return vertices_[idx]; }
    const HalfEdge& halfEdge(uint32_t idx) const { return halfEdges_[idx]; }
    const HEFace& face(uint32_t idx) const { return faces_[idx]; }
    
    HEVertex& vertex(uint32_t idx) { return vertices_[idx]; }
    HalfEdge& halfEdge(uint32_t idx) { return halfEdges_[idx]; }
    HEFace& face(uint32_t idx) { return faces_[idx]; }
    
    // ===================
    // Adjacency Queries
    // ===================
    
    /**
     * @brief Get vertices adjacent to a vertex (1-ring neighbors)
     * @param vertexIdx Index of center vertex
     * @return Vector of adjacent vertex indices
     */
    std::vector<uint32_t> vertexNeighbors(uint32_t vertexIdx) const;
    
    /**
     * @brief Get faces adjacent to a vertex
     * @param vertexIdx Index of vertex
     * @return Vector of adjacent face indices
     */
    std::vector<uint32_t> vertexFaces(uint32_t vertexIdx) const;
    
    /**
     * @brief Get half-edges emanating from a vertex
     * @param vertexIdx Index of vertex
     * @return Vector of outgoing half-edge indices
     */
    std::vector<uint32_t> vertexOutgoingEdges(uint32_t vertexIdx) const;
    
    /**
     * @brief Get faces adjacent to a face (sharing an edge)
     * @param faceIdx Index of face
     * @return Vector of adjacent face indices
     */
    std::vector<uint32_t> faceNeighbors(uint32_t faceIdx) const;
    
    /**
     * @brief Get vertices of a face (in CCW order)
     * @param faceIdx Index of face
     * @return Vector of 3 vertex indices
     */
    std::vector<uint32_t> faceVertices(uint32_t faceIdx) const;
    
    /**
     * @brief Get half-edges of a face (in CCW order)
     * @param faceIdx Index of face
     * @return Vector of 3 half-edge indices
     */
    std::vector<uint32_t> faceHalfEdges(uint32_t faceIdx) const;
    
    /**
     * @brief Get the source vertex of a half-edge
     * @param heIdx Half-edge index
     * @return Source vertex index
     */
    uint32_t halfEdgeSource(uint32_t heIdx) const;
    
    /**
     * @brief Find half-edge from vertex A to vertex B
     * @param fromVertex Source vertex
     * @param toVertex Target vertex
     * @return Half-edge index or INVALID_INDEX if not found
     */
    uint32_t findHalfEdge(uint32_t fromVertex, uint32_t toVertex) const;
    
    // ===================
    // Boundary Detection
    // ===================
    
    /**
     * @brief Check if vertex is on a boundary
     * @param vertexIdx Vertex index
     * @return true if vertex is on a boundary edge
     */
    bool isVertexOnBoundary(uint32_t vertexIdx) const;
    
    /**
     * @brief Check if half-edge is a boundary edge
     * @param heIdx Half-edge index
     * @return true if this is a boundary half-edge (no twin)
     */
    bool isBoundaryEdge(uint32_t heIdx) const;
    
    /**
     * @brief Get all boundary loops
     * @return Vector of loops, each loop is a vector of vertex indices in order
     */
    std::vector<std::vector<uint32_t>> findBoundaryLoops() const;
    
    /**
     * @brief Get all boundary half-edges
     * @return Vector of boundary half-edge indices
     */
    std::vector<uint32_t> findBoundaryEdges() const;
    
    /**
     * @brief Count boundary edges (holes)
     * @return Number of boundary (unpaired) half-edges
     */
    size_t boundaryEdgeCount() const;
    
    /**
     * @brief Check if mesh is closed (watertight)
     * @return true if all edges have twins
     */
    bool isClosed() const { return boundaryEdgeCount() == 0; }
    
    // ===================
    // Manifold Checks
    // ===================
    
    /**
     * @brief Check if mesh is manifold
     * 
     * A manifold mesh has:
     * - Each edge shared by exactly 1 or 2 faces
     * - Each vertex has a disk-like neighborhood
     * 
     * @return true if mesh is manifold
     */
    bool isManifold() const;
    
    /**
     * @brief Find non-manifold vertices
     * @return Vector of vertex indices with non-manifold neighborhoods
     */
    std::vector<uint32_t> findNonManifoldVertices() const;
    
    /**
     * @brief Find non-manifold edges (shared by >2 faces)
     * @return Vector of half-edge indices
     */
    std::vector<uint32_t> findNonManifoldEdges() const;
    
    // ===================
    // Geometry
    // ===================
    
    /**
     * @brief Compute face normals for all faces
     */
    void computeFaceNormals();
    
    /**
     * @brief Compute vertex normals from face normals
     */
    void computeVertexNormals();
    
    /**
     * @brief Get bounding box
     * @return Axis-aligned bounding box
     */
    BoundingBox boundingBox() const;
    
    /**
     * @brief Get vertex valence (number of edges)
     * @param vertexIdx Vertex index
     * @return Number of edges connected to vertex
     */
    size_t vertexValence(uint32_t vertexIdx) const;
    
    // ===================
    // Validation
    // ===================
    
    /**
     * @brief Validate mesh connectivity
     * @return Empty string if valid, error message otherwise
     */
    std::string validate() const;
    
    /**
     * @brief Check consistency of half-edge pointers
     * @return true if all pointers are consistent
     */
    bool checkConsistency() const;
    
private:
    std::vector<HEVertex> vertices_;
    std::vector<HalfEdge> halfEdges_;
    std::vector<HEFace> faces_;
    
    // Helper for construction
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
                (static_cast<uint64_t>(k.v0) << 32) | k.v1
            );
        }
    };
};

// ===================
// Iterator Helpers
// ===================

/**
 * @brief Iterator for circulating around a vertex (visiting neighbors)
 */
class VertexCirculator {
public:
    VertexCirculator(const HalfEdgeMesh& mesh, uint32_t vertexIdx);
    
    bool isValid() const { return current_ != INVALID_INDEX; }
    bool atEnd() const { return done_; }
    
    uint32_t vertexIndex() const;
    uint32_t halfEdgeIndex() const { return current_; }
    uint32_t faceIndex() const;
    
    VertexCirculator& operator++();
    
private:
    const HalfEdgeMesh& mesh_;
    uint32_t start_;
    uint32_t current_;
    bool done_ = false;
};

/**
 * @brief Iterator for circulating around a face (visiting vertices)
 */
class FaceCirculator {
public:
    FaceCirculator(const HalfEdgeMesh& mesh, uint32_t faceIdx);
    
    bool isValid() const { return current_ != INVALID_INDEX; }
    bool atEnd() const { return done_; }
    
    uint32_t vertexIndex() const;
    uint32_t halfEdgeIndex() const { return current_; }
    
    FaceCirculator& operator++();
    
private:
    const HalfEdgeMesh& mesh_;
    uint32_t start_;
    uint32_t current_;
    bool done_ = false;
};

} // namespace geometry
} // namespace dc3d
