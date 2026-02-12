/**
 * @file MeshData.h
 * @brief Core mesh data structure for storing triangle meshes
 * 
 * Provides efficient storage for vertex positions, normals, and face indices.
 * Designed to handle meshes up to 50M triangles.
 */

#pragma once

#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <memory>
#include <cstdint>

#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

/**
 * @brief Result type for operations that may fail
 */
template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;
    
    bool ok() const { return value.has_value(); }
    explicit operator bool() const { return ok(); }
    
    static Result<T> success(T&& val) {
        return Result<T>{ std::move(val), {} };
    }
    
    static Result<T> success(const T& val) {
        return Result<T>{ val, {} };
    }
    
    static Result<T> failure(const std::string& err) {
        return Result<T>{ std::nullopt, err };
    }
};

/**
 * @brief Progress callback for long-running operations
 * @param progress Progress value between 0.0 and 1.0
 * @return false to cancel the operation, true to continue
 */
using ProgressCallback = std::function<bool(float progress)>;

/**
 * @brief Axis-aligned bounding box
 */
struct BoundingBox {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};
    
    /// Expand to include a point
    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    
    /// Expand to include another bounding box
    void expand(const BoundingBox& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }
    
    /// Get center of the bounding box
    glm::vec3 center() const {
        return (min + max) * 0.5f;
    }
    
    /// Get dimensions of the bounding box
    glm::vec3 dimensions() const {
        return max - min;
    }
    
    /// Get diagonal length of the bounding box
    float diagonal() const {
        return glm::length(dimensions());
    }
    
    /// Check if the bounding box is valid (min <= max)
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
    
    /// Reset to invalid state
    void reset() {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::lowest());
    }
};

/**
 * @brief Statistics about a mesh
 */
struct MeshStats {
    size_t vertexCount = 0;
    size_t faceCount = 0;
    size_t edgeCount = 0;
    BoundingBox bounds;
    bool hasNormals = false;
    bool hasUVs = false;
    bool isWatertight = false;
    size_t boundaryEdgeCount = 0;
    size_t nonManifoldEdgeCount = 0;
    float surfaceArea = 0.0f;
    float volume = 0.0f;  // Only valid if watertight
};

/**
 * @brief Core mesh data structure using indexed triangle storage
 * 
 * Stores mesh data optimized for rendering and processing:
 * - Vertices as std::vector<glm::vec3> for positions
 * - Faces as std::vector<uint32_t> where every 3 indices form a triangle
 * - Optional per-vertex normals
 * - Optional per-vertex texture coordinates
 * 
 * This is a simple indexed triangle mesh (triangle soup), suitable for
 * rendering and as input for algorithms. For topological operations,
 * convert to HalfEdgeMesh.
 */
class MeshData {
public:
    MeshData() = default;
    ~MeshData() = default;
    
    // Move semantics for efficiency with large meshes
    MeshData(MeshData&&) noexcept = default;
    MeshData& operator=(MeshData&&) noexcept = default;
    
    // Copy (can be expensive for large meshes)
    MeshData(const MeshData&) = default;
    MeshData& operator=(const MeshData&) = default;
    
    // ===================
    // Data Access
    // ===================
    
    /// Get vertex positions (read-only)
    const std::vector<glm::vec3>& vertices() const { return vertices_; }
    
    /// Get vertex positions (mutable)
    std::vector<glm::vec3>& vertices() { return vertices_; }
    
    /// Get face indices (read-only) - every 3 indices form a triangle
    const std::vector<uint32_t>& indices() const { return indices_; }
    
    /// Get face indices (mutable)
    std::vector<uint32_t>& indices() { return indices_; }
    
    /// Get vertex normals (read-only)
    const std::vector<glm::vec3>& normals() const { return normals_; }
    
    /// Get vertex normals (mutable)
    std::vector<glm::vec3>& normals() { return normals_; }
    
    /// Get texture coordinates (read-only)
    const std::vector<glm::vec2>& uvs() const { return uvs_; }
    
    /// Get texture coordinates (mutable)
    std::vector<glm::vec2>& uvs() { return uvs_; }
    
    // ===================
    // Statistics
    // ===================
    
    /// Number of vertices
    size_t vertexCount() const { return vertices_.size(); }
    
    /// Number of triangular faces
    size_t faceCount() const { return indices_.size() / 3; }
    
    /// Number of indices
    size_t indexCount() const { return indices_.size(); }
    
    /// Check if mesh has normals
    bool hasNormals() const { return !normals_.empty() && normals_.size() == vertices_.size(); }
    
    /// Check if mesh has texture coordinates
    bool hasUVs() const { return !uvs_.empty() && uvs_.size() == vertices_.size(); }
    
    /// Check if mesh is empty
    bool isEmpty() const { return vertices_.empty() || indices_.empty(); }
    
    /// Check if mesh has valid data
    bool isValid() const;
    
    /// Get bounding box (cached, computed lazily)
    const BoundingBox& boundingBox() const;
    
    /// Get comprehensive mesh statistics
    MeshStats computeStats() const;
    
    // ===================
    // Modification
    // ===================
    
    /// Reserve memory for expected vertex count
    void reserveVertices(size_t count);
    
    /// Reserve memory for expected face count
    void reserveFaces(size_t count);
    
    /// Add a vertex, returns index
    uint32_t addVertex(const glm::vec3& position);
    
    /// Add a vertex with normal, returns index
    uint32_t addVertex(const glm::vec3& position, const glm::vec3& normal);
    
    /// Add a triangular face (3 vertex indices)
    void addFace(uint32_t v0, uint32_t v1, uint32_t v2);
    
    /// Clear all data
    void clear();
    
    // ===================
    // Normals
    // ===================
    
    /// Compute per-vertex normals by averaging adjacent face normals
    void computeNormals();
    
    /// Compute per-vertex normals with angle weighting
    void computeNormalsWeighted();
    
    /// Flip all normals (reverse winding)
    void flipNormals();
    
    /// Clear normals
    void clearNormals();
    
    // ===================
    // Geometry Queries
    // ===================
    
    /// Get face normal for a specific face
    glm::vec3 faceNormal(size_t faceIndex) const;
    
    /// Get face area for a specific face
    float faceArea(size_t faceIndex) const;
    
    /// Compute total surface area
    float surfaceArea() const;
    
    /// Compute volume (assumes watertight mesh with consistent winding)
    float volume() const;
    
    /// Get centroid of the mesh
    glm::vec3 centroid() const;
    
    // ===================
    // Transformations
    // ===================
    
    /// Apply a transformation matrix to all vertices
    void transform(const glm::mat4& matrix);
    
    /// Translate all vertices
    void translate(const glm::vec3& offset);
    
    /// Scale uniformly
    void scale(float factor);
    
    /// Scale non-uniformly
    void scale(const glm::vec3& factors);
    
    /// Center the mesh at origin
    void centerAtOrigin();
    
    /// Normalize to fit in unit cube centered at origin
    void normalizeToUnitCube();
    
    // ===================
    // Validation
    // ===================
    
    /// Check for degenerate triangles
    size_t countDegenerateFaces(float areaThreshold = 1e-10f) const;
    
    /// Remove degenerate triangles
    size_t removeDegenerateFaces(float areaThreshold = 1e-10f);
    
    /// Check for duplicate vertices
    size_t countDuplicateVertices(float tolerance = 1e-6f) const;
    
    /// Merge duplicate vertices within tolerance
    size_t mergeDuplicateVertices(float tolerance = 1e-6f, ProgressCallback progress = nullptr);
    
    // ===================
    // Memory
    // ===================
    
    /// Estimate memory usage in bytes
    size_t memoryUsage() const;
    
    /// Shrink vectors to fit their contents
    void shrinkToFit();
    
private:
    std::vector<glm::vec3> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<glm::vec3> normals_;
    std::vector<glm::vec2> uvs_;
    
    // Cached bounding box
    mutable BoundingBox bounds_;
    mutable bool boundsDirty_ = true;
    
    void invalidateBounds() { boundsDirty_ = true; }
    void updateBounds() const;
};

} // namespace geometry
} // namespace dc3d
