/**
 * @file DeviationAnalysis.h
 * @brief Mesh deviation analysis for comparing two meshes
 * 
 * Provides tools for computing per-vertex distances between meshes,
 * useful for comparing scanned data to CAD models, or checking
 * before/after modifications.
 */

#pragma once

#include "MeshData.h"
#include <vector>
#include <memory>
#include <array>

namespace dc3d {
namespace geometry {

/**
 * @brief Statistics about deviation between two meshes
 */
struct DeviationStats {
    float minDeviation = 0.0f;      ///< Minimum distance
    float maxDeviation = 0.0f;      ///< Maximum distance
    float avgDeviation = 0.0f;      ///< Average distance
    float stddevDeviation = 0.0f;   ///< Standard deviation
    float rmsDeviation = 0.0f;      ///< Root mean square deviation
    
    // Signed statistics (if signed distance was computed)
    float minSigned = 0.0f;         ///< Minimum signed distance (most inside)
    float maxSigned = 0.0f;         ///< Maximum signed distance (most outside)
    float avgSigned = 0.0f;         ///< Average signed distance
    
    // Percentiles
    float percentile50 = 0.0f;      ///< Median deviation
    float percentile90 = 0.0f;      ///< 90th percentile
    float percentile95 = 0.0f;      ///< 95th percentile
    float percentile99 = 0.0f;      ///< 99th percentile
    
    // Counts
    size_t totalPoints = 0;
    size_t pointsWithinTolerance = 0;  ///< Points within threshold
    float toleranceThreshold = 0.0f;   ///< Threshold used
};

/**
 * @brief 3D axis-aligned bounding box for KD-tree
 */
struct KDBox {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};
    
    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    
    float distanceSquared(const glm::vec3& point) const;
};

/**
 * @brief KD-tree node for spatial acceleration
 */
struct KDNode {
    uint32_t triangleIndex;    ///< Index of triangle in mesh
    int splitAxis;             ///< 0=X, 1=Y, 2=Z, -1=leaf
    float splitPos;            ///< Split position
    KDBox bounds;
    std::unique_ptr<KDNode> left;
    std::unique_ptr<KDNode> right;
};

/**
 * @brief KD-tree for accelerated point-to-mesh distance queries
 * 
 * Builds a spatial acceleration structure over triangles
 * for efficient nearest-triangle queries.
 */
class KDTree {
public:
    KDTree() = default;
    ~KDTree() = default;
    
    /**
     * @brief Build KD-tree from mesh triangles
     * @param mesh Source mesh
     * @param progress Optional progress callback
     */
    void build(const MeshData& mesh, ProgressCallback progress = nullptr);
    
    /**
     * @brief Find closest point on mesh to query point
     * @param point Query point
     * @param closestPoint Output: closest point on mesh surface
     * @param closestTriangle Output: index of closest triangle
     * @return Distance to closest point
     */
    float findClosestPoint(
        const glm::vec3& point,
        glm::vec3& closestPoint,
        uint32_t& closestTriangle
    ) const;
    
    /**
     * @brief Find closest distance to mesh (unsigned)
     * @param point Query point
     * @return Distance to closest point
     */
    float findClosestDistance(const glm::vec3& point) const;
    
    /**
     * @brief Check if tree has been built
     */
    bool isBuilt() const { return root_ != nullptr; }
    
private:
    std::unique_ptr<KDNode> buildRecursive(
        std::vector<uint32_t>& triangleIndices,
        int depth
    );
    
    void findClosestRecursive(
        const KDNode* node,
        const glm::vec3& point,
        float& bestDistSq,
        glm::vec3& bestPoint,
        uint32_t& bestTriangle
    ) const;
    
    glm::vec3 closestPointOnTriangle(
        const glm::vec3& point,
        uint32_t triangleIndex
    ) const;
    
    KDBox computeTriangleBounds(uint32_t triangleIndex) const;
    glm::vec3 computeTriangleCentroid(uint32_t triangleIndex) const;
    
    std::unique_ptr<KDNode> root_;
    const MeshData* mesh_ = nullptr;
};

/**
 * @brief Configuration for deviation computation
 */
struct DeviationConfig {
    bool computeSigned = true;           ///< Compute signed distance
    bool useKDTree = true;               ///< Use KD-tree acceleration
    float toleranceThreshold = 0.1f;     ///< Threshold for "within tolerance"
    int maxIterations = 100;             ///< Max iterations for distance refining
};

/**
 * @brief Mesh deviation analysis
 * 
 * Computes per-vertex distances from one mesh to another,
 * useful for quality inspection and comparison.
 */
class DeviationAnalysis {
public:
    /**
     * @brief Compute per-vertex deviation from meshA to meshB
     * 
     * For each vertex in meshA, finds the closest point on meshB
     * and returns the distance.
     * 
     * @param meshA Source mesh (distances computed for each vertex)
     * @param meshB Target mesh (surface to measure distance to)
     * @param config Configuration options
     * @param progress Optional progress callback
     * @return Per-vertex distances (same size as meshA vertices)
     */
    static std::vector<float> computeDeviation(
        const MeshData& meshA,
        const MeshData& meshB,
        const DeviationConfig& config = {},
        ProgressCallback progress = nullptr
    );
    
    /**
     * @brief Compute signed per-vertex deviation
     * 
     * Positive = outside meshB, Negative = inside meshB
     * 
     * @param meshA Source mesh
     * @param meshB Target mesh (should be watertight for signed distance)
     * @param progress Optional progress callback
     * @return Per-vertex signed distances
     */
    static std::vector<float> computeSignedDeviation(
        const MeshData& meshA,
        const MeshData& meshB,
        ProgressCallback progress = nullptr
    );
    
    /**
     * @brief Compute deviation statistics
     * @param deviations Per-vertex deviation values
     * @param toleranceThreshold Threshold for "within tolerance" count
     * @return Comprehensive statistics
     */
    static DeviationStats computeStats(
        const std::vector<float>& deviations,
        float toleranceThreshold = 0.1f
    );
    
    /**
     * @brief Compute signed deviation statistics
     * @param deviations Per-vertex signed deviation values
     * @param toleranceThreshold Threshold for "within tolerance" count
     * @return Comprehensive statistics including signed values
     */
    static DeviationStats computeSignedStats(
        const std::vector<float>& deviations,
        float toleranceThreshold = 0.1f
    );
    
    /**
     * @brief Compute distance from a point to closest point on mesh
     * @param point Query point
     * @param mesh Target mesh
     * @param closestPoint Output: closest point on mesh
     * @return Distance to closest point
     */
    static float pointToMeshDistance(
        const glm::vec3& point,
        const MeshData& mesh,
        glm::vec3& closestPoint
    );
    
    /**
     * @brief Compute signed distance from point to mesh
     * 
     * Uses ray casting to determine inside/outside.
     * 
     * @param point Query point
     * @param mesh Target mesh (should be watertight)
     * @param closestPoint Output: closest point on mesh
     * @return Signed distance (negative = inside)
     */
    static float pointToMeshSignedDistance(
        const glm::vec3& point,
        const MeshData& mesh,
        glm::vec3& closestPoint
    );
    
    /**
     * @brief Create histogram of deviation values
     * @param deviations Per-vertex deviations
     * @param numBins Number of histogram bins
     * @param minVal Minimum value for histogram range (auto if > maxVal)
     * @param maxVal Maximum value for histogram range (auto if < minVal)
     * @return Bin counts
     */
    static std::vector<size_t> createHistogram(
        const std::vector<float>& deviations,
        size_t numBins = 50,
        float minVal = 0.0f,
        float maxVal = -1.0f
    );

    /**
     * @brief Compute closest point on triangle to query point
     */
    static glm::vec3 closestPointOnTriangle(
        const glm::vec3& point,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2
    );

private:
    /**
     * @brief Ray-triangle intersection test
     */
    static bool rayTriangleIntersect(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& t
    );
    
    /**
     * @brief Count ray-mesh intersections for inside/outside test
     */
    static int countRayIntersections(
        const glm::vec3& point,
        const glm::vec3& direction,
        const MeshData& mesh
    );
};

} // namespace geometry
} // namespace dc3d
