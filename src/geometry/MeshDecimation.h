/**
 * @file MeshDecimation.h
 * @brief Polygon reduction using Quadric Error Metrics (QEM)
 * 
 * Implements efficient mesh simplification based on Garland & Heckbert's
 * QEM algorithm. Supports various targeting modes and boundary preservation.
 */

#pragma once

#include "MeshData.h"
#include "HalfEdgeMesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <functional>
#include <queue>
#include <unordered_set>

namespace dc3d {
namespace geometry {

/**
 * @brief Target mode for decimation
 */
enum class DecimationTarget {
    Ratio,          ///< Reduce to percentage of original (0.0 - 1.0)
    VertexCount,    ///< Target specific vertex count
    FaceCount       ///< Target specific face count
};

/**
 * @brief Options for mesh decimation
 */
struct DecimationOptions {
    DecimationTarget targetMode = DecimationTarget::Ratio;
    float targetRatio = 0.5f;           ///< For Ratio mode: target as fraction of original
    size_t targetVertexCount = 0;       ///< For VertexCount mode
    size_t targetFaceCount = 0;         ///< For FaceCount mode
    
    bool preserveBoundary = true;       ///< Prevent collapsing boundary edges
    float boundaryWeight = 100.0f;      ///< Weight multiplier for boundary edges
    
    bool preserveTopology = true;       ///< Prevent creating non-manifold geometry
    
    float maxError = std::numeric_limits<float>::max();  ///< Stop if error exceeds this
    
    bool lockVertices = false;          ///< If true, use lockedVertices set
    std::unordered_set<uint32_t> lockedVertices;  ///< Vertices that cannot be collapsed
};

/**
 * @brief Result information from decimation
 */
struct DecimationResult {
    size_t originalVertices = 0;
    size_t originalFaces = 0;
    size_t finalVertices = 0;
    size_t finalFaces = 0;
    size_t edgesCollapsed = 0;
    float maxError = 0.0f;
    float avgError = 0.0f;
    bool reachedTarget = false;
    bool cancelled = false;
};

/**
 * @brief 4x4 Symmetric Quadric Error Matrix (stored as 10 unique values)
 * 
 * Represents the quadric Q = n*n^T + d^2 for a plane with normal n and distance d.
 * The error of a point p is: e = p^T * Q * p
 */
class Quadric {
public:
    Quadric() : data_{0} {}
    
    /// Create quadric from plane equation ax + by + cz + d = 0
    Quadric(float a, float b, float c, float d);
    
    /// Create quadric from plane normal and point on plane
    static Quadric fromPlane(const glm::vec3& normal, const glm::vec3& point);
    
    /// Evaluate quadric error at a point
    float evaluate(const glm::vec3& point) const;
    
    /// Add two quadrics
    Quadric operator+(const Quadric& other) const;
    Quadric& operator+=(const Quadric& other);
    
    /// Scale quadric
    Quadric operator*(float scale) const;
    
    /// Find optimal point minimizing error (solves linear system)
    /// Returns false if matrix is singular
    bool findOptimal(glm::vec3& outPoint) const;
    
    /// Get the quadric as a 4x4 matrix
    glm::mat4 toMatrix() const;
    
private:
    // Store upper triangle: [a00, a01, a02, a03, a11, a12, a13, a22, a23, a33]
    float data_[10];
};

/**
 * @brief Edge collapse candidate with error cost
 */
struct EdgeCollapse {
    uint32_t heIdx;         ///< Half-edge to collapse
    uint32_t v0, v1;        ///< Vertices (v0 -> v1)
    glm::vec3 target;       ///< Optimal collapse position
    float cost;             ///< Quadric error cost
    uint32_t version;       ///< For lazy deletion in priority queue
    
    bool operator>(const EdgeCollapse& other) const {
        return cost > other.cost;
    }
};

/**
 * @brief Mesh decimation using Quadric Error Metrics
 * 
 * Usage:
 * @code
 *     DecimationOptions opts;
 *     opts.targetRatio = 0.5f;  // Reduce to 50%
 *     opts.preserveBoundary = true;
 *     
 *     auto result = MeshDecimator::decimate(mesh, opts, [](float p) {
 *         std::cout << "Progress: " << p * 100 << "%" << std::endl;
 *         return true;  // Continue
 *     });
 *     
 *     if (result.ok()) {
 *         MeshData simplified = std::move(*result.value);
 *     }
 * @endcode
 */
class MeshDecimator {
public:
    /**
     * @brief Decimate a mesh
     * @param mesh Input mesh
     * @param options Decimation options
     * @param progress Optional progress callback
     * @return Result with decimated mesh and statistics
     */
    static Result<std::pair<MeshData, DecimationResult>> decimate(
        const MeshData& mesh,
        const DecimationOptions& options,
        ProgressCallback progress = nullptr);
    
    /**
     * @brief Decimate a mesh (simple interface)
     * @param mesh Input mesh
     * @param targetRatio Target ratio (0.0 - 1.0)
     * @param preserveBoundary Whether to preserve boundaries
     * @param progress Optional progress callback
     * @return Decimated mesh or error
     */
    static Result<MeshData> decimate(
        const MeshData& mesh,
        float targetRatio,
        bool preserveBoundary = true,
        ProgressCallback progress = nullptr);
    
private:
    MeshDecimator() = default;
};

/**
 * @brief Internal decimation state (for advanced use or testing)
 */
class DecimationState {
public:
    DecimationState(HalfEdgeMesh&& mesh, const DecimationOptions& options);
    
    /// Run decimation to target
    DecimationResult run(ProgressCallback progress = nullptr);
    
    /// Perform single edge collapse
    bool collapseEdge(uint32_t heIdx, const glm::vec3& newPosition);
    
    /// Check if edge can be collapsed
    bool canCollapse(uint32_t heIdx) const;
    
    /// Get current mesh
    const HalfEdgeMesh& mesh() const { return mesh_; }
    
    /// Convert to MeshData (compacts and returns)
    MeshData toMeshData() const;
    
private:
    HalfEdgeMesh mesh_;
    DecimationOptions options_;
    
    std::vector<Quadric> vertexQuadrics_;
    std::vector<uint32_t> vertexVersions_;
    std::vector<bool> vertexDeleted_;
    std::vector<bool> faceDeleted_;
    
    std::priority_queue<EdgeCollapse, std::vector<EdgeCollapse>, std::greater<EdgeCollapse>> queue_;
    
    size_t activeVertices_ = 0;
    size_t activeFaces_ = 0;
    
    void initializeQuadrics();
    void initializeQueue();
    void updateVertexAfterCollapse(uint32_t vIdx);
    void recomputeEdgeCosts(uint32_t vIdx);
    
    EdgeCollapse computeEdgeCost(uint32_t heIdx) const;
    bool isEdgeValid(uint32_t heIdx) const;
    bool checkTopology(uint32_t heIdx) const;
    
    size_t computeTargetFaces() const;
};

} // namespace geometry
} // namespace dc3d
