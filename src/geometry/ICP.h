/**
 * @file ICP.h
 * @brief Iterative Closest Point algorithm for fine mesh alignment
 * 
 * Provides ICP variants:
 * - Point-to-point ICP (classic)
 * - Point-to-plane ICP (faster convergence)
 * - Trimmed ICP (outlier rejection)
 */

#pragma once

#include "MeshData.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <memory>

namespace dc3d {
namespace geometry {

/**
 * @brief ICP algorithm variant
 */
enum class ICPAlgorithm {
    PointToPoint,   ///< Classic point-to-point ICP
    PointToPlane    ///< Point-to-plane ICP (faster convergence)
};

/**
 * @brief Options for ICP algorithm
 */
struct ICPOptions {
    ICPAlgorithm algorithm = ICPAlgorithm::PointToPlane;
    
    int maxIterations = 50;             ///< Maximum number of iterations
    float convergenceThreshold = 1e-5f; ///< Stop when transform change < threshold
    
    bool outlierRejection = true;       ///< Enable outlier rejection
    float outlierThreshold = 3.0f;      ///< Reject points beyond threshold * stddev
    float trimPercentage = 0.0f;        ///< Trim highest N% of correspondences (0-1)
    
    float maxCorrespondenceDistance = std::numeric_limits<float>::max();  ///< Max distance for correspondence
    
    int correspondenceSampling = 1;     ///< Sample every Nth point (1 = all points)
    bool useNormals = true;             ///< Use normals for point-to-plane
};

/**
 * @brief Result of ICP alignment
 */
struct ICPResult {
    bool converged = false;
    glm::mat4 transform{1.0f};      ///< Final transformation matrix
    
    float initialRMSError = 0.0f;    ///< RMS error before alignment
    float finalRMSError = 0.0f;      ///< RMS error after alignment
    
    int iterationsUsed = 0;          ///< Actual iterations performed
    int correspondenceCount = 0;     ///< Number of point correspondences used
    
    std::vector<float> errorHistory; ///< RMS error per iteration
    
    /// Check if ICP was successful
    explicit operator bool() const { return converged; }
};

/**
 * @brief Statistics for a single ICP iteration
 */
struct ICPIterationStats {
    int iteration = 0;
    float rmsError = 0.0f;
    int correspondenceCount = 0;
    int outlierCount = 0;
    float transformChange = 0.0f;
};

/**
 * @brief Callback for ICP iteration progress
 * @param stats Statistics for current iteration
 * @return false to cancel, true to continue
 */
using ICPIterationCallback = std::function<bool(const ICPIterationStats& stats)>;

/**
 * @brief KD-Tree node for nearest neighbor queries
 */
struct KDNode {
    glm::vec3 point;
    glm::vec3 normal;
    int index = -1;
    std::unique_ptr<KDNode> left;
    std::unique_ptr<KDNode> right;
    int splitAxis = 0;
};

/**
 * @brief Simple KD-Tree for nearest neighbor search
 */
class KDTree {
public:
    KDTree() = default;
    ~KDTree() = default;
    
    /**
     * @brief Build KD-Tree from points
     * @param points Point positions
     * @param normals Optional point normals
     */
    void build(const std::vector<glm::vec3>& points,
               const std::vector<glm::vec3>& normals = {});
    
    /**
     * @brief Find nearest neighbor
     * @param query Query point
     * @param maxDistance Maximum search distance
     * @return Index of nearest point, or -1 if none found
     */
    int findNearest(const glm::vec3& query, float maxDistance = std::numeric_limits<float>::max()) const;
    
    /**
     * @brief Find nearest neighbor with distance and normal
     * @param query Query point
     * @param outDistance Output: distance to nearest
     * @param outNormal Output: normal at nearest point
     * @param maxDistance Maximum search distance
     * @return Index of nearest point, or -1 if none found
     */
    int findNearest(const glm::vec3& query, float& outDistance, glm::vec3& outNormal,
                    float maxDistance = std::numeric_limits<float>::max()) const;
    
    /**
     * @brief Check if tree is built
     */
    bool isBuilt() const { return root_ != nullptr; }
    
    /**
     * @brief Get point at index
     * @param index Index of the point
     * @return Point position at index, or zero vector if invalid
     */
    glm::vec3 getPoint(int index) const;
    
private:
    std::unique_ptr<KDNode> buildRecursive(std::vector<int>& indices, int depth,
                                            const std::vector<glm::vec3>& points,
                                            const std::vector<glm::vec3>& normals);
    
    void findNearestRecursive(const KDNode* node, const glm::vec3& query,
                              int& bestIndex, float& bestDist, glm::vec3& bestNormal,
                              float maxDistance) const;
    
    std::unique_ptr<KDNode> root_;
    std::vector<glm::vec3> points_;
    std::vector<glm::vec3> normals_;
};

/**
 * @brief Point correspondence for ICP
 */
struct Correspondence {
    int sourceIndex;
    int targetIndex;
    glm::vec3 sourcePoint;
    glm::vec3 targetPoint;
    glm::vec3 targetNormal;
    float distance;
    float weight = 1.0f;
};

/**
 * @brief Iterative Closest Point algorithm implementation
 */
class ICP {
public:
    ICP() = default;
    ~ICP() = default;
    
    /**
     * @brief Align source mesh to target mesh using ICP
     * 
     * @param source Source mesh (will be transformed)
     * @param target Target mesh (reference, unchanged)
     * @param options ICP configuration options
     * @param progress Progress callback
     * @return ICPResult with transformation and statistics
     */
    ICPResult align(MeshData& source, const MeshData& target,
                    const ICPOptions& options = {},
                    ProgressCallback progress = nullptr);
    
    /**
     * @brief Align source points to target mesh using ICP
     * 
     * @param sourcePoints Points to align (modified in place)
     * @param sourceNormals Source normals (optional)
     * @param target Target mesh
     * @param options ICP configuration options
     * @param iterationCallback Per-iteration callback
     * @return ICPResult with transformation
     */
    ICPResult alignPoints(std::vector<glm::vec3>& sourcePoints,
                          std::vector<glm::vec3>& sourceNormals,
                          const MeshData& target,
                          const ICPOptions& options = {},
                          ICPIterationCallback iterationCallback = nullptr);
    
    /**
     * @brief Compute transformation for one ICP iteration
     */
    glm::mat4 computeIterationTransform(
        const std::vector<Correspondence>& correspondences,
        ICPAlgorithm algorithm);
    
private:
    /**
     * @brief Find correspondences between source and target
     */
    std::vector<Correspondence> findCorrespondences(
        const std::vector<glm::vec3>& sourcePoints,
        const KDTree& targetTree,
        const ICPOptions& options);
    
    /**
     * @brief Reject outliers from correspondences
     */
    void rejectOutliers(std::vector<Correspondence>& correspondences,
                        const ICPOptions& options);
    
    /**
     * @brief Compute point-to-point transformation
     */
    glm::mat4 computePointToPointTransform(
        const std::vector<Correspondence>& correspondences);
    
    /**
     * @brief Compute point-to-plane transformation
     */
    glm::mat4 computePointToPlaneTransform(
        const std::vector<Correspondence>& correspondences);
    
    /**
     * @brief Compute RMS error for current correspondences
     */
    float computeRMSError(const std::vector<Correspondence>& correspondences);
    
    /**
     * @brief Compute transformation change between iterations
     */
    float computeTransformChange(const glm::mat4& prev, const glm::mat4& curr);
};

} // namespace geometry
} // namespace dc3d
