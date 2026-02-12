/**
 * @file Alignment.h
 * @brief Mesh alignment algorithms for positioning objects in world coordinate system
 * 
 * Provides alignment tools including:
 * - WCS alignment using primary/secondary/tertiary features
 * - Interactive transform application
 * - N-point correspondence alignment
 * - Fine alignment using ICP
 */

#pragma once

#include "MeshData.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <optional>
#include <functional>

namespace dc3d {
namespace geometry {

// Forward declarations
class MeshData;

/**
 * @brief Alignment feature type for WCS alignment
 */
enum class AlignmentFeature {
    Point,          ///< Single point (origin)
    Line,           ///< Line/axis direction
    Plane,          ///< Plane normal
    CylinderAxis,   ///< Cylinder axis
    SphereCenter    ///< Sphere center point
};

/**
 * @brief Represents a geometric feature for alignment
 */
struct AlignmentFeatureData {
    AlignmentFeature type = AlignmentFeature::Point;
    glm::vec3 point{0.0f};      ///< Point position or plane point
    glm::vec3 direction{0.0f, 1.0f, 0.0f}; ///< Direction for line/plane normal/axis
    float radius = 0.0f;        ///< Radius for cylinders/spheres
    
    /// Create a point feature
    static AlignmentFeatureData createPoint(const glm::vec3& p) {
        return {AlignmentFeature::Point, p, {}, 0.0f};
    }
    
    /// Create a line feature
    static AlignmentFeatureData createLine(const glm::vec3& point, const glm::vec3& dir) {
        return {AlignmentFeature::Line, point, glm::normalize(dir), 0.0f};
    }
    
    /// Create a plane feature
    static AlignmentFeatureData createPlane(const glm::vec3& point, const glm::vec3& normal) {
        return {AlignmentFeature::Plane, point, glm::normalize(normal), 0.0f};
    }
    
    /// Create a cylinder feature
    static AlignmentFeatureData createCylinder(const glm::vec3& point, const glm::vec3& axis, float r) {
        return {AlignmentFeature::CylinderAxis, point, glm::normalize(axis), r};
    }
    
    /// Create a sphere feature
    static AlignmentFeatureData createSphere(const glm::vec3& center, float r) {
        return {AlignmentFeature::SphereCenter, center, {}, r};
    }
};

/**
 * @brief Point pair for N-point alignment
 */
struct PointPair {
    glm::vec3 source;   ///< Point on source mesh
    glm::vec3 target;   ///< Corresponding point on target mesh
    float weight = 1.0f; ///< Optional weight for weighted alignment
};

/**
 * @brief WCS axis specification
 */
enum class WCSAxis {
    PositiveX,
    NegativeX,
    PositiveY,
    NegativeY,
    PositiveZ,
    NegativeZ
};

/**
 * @brief Result of an alignment operation
 */
struct AlignmentResult {
    bool success = false;
    glm::mat4 transform{1.0f};  ///< 4x4 transformation matrix
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    
    float rmsError = 0.0f;      ///< Root mean square error
    float maxError = 0.0f;      ///< Maximum point-to-point error
    int iterationsUsed = 0;     ///< Iterations used (for iterative methods)
    std::string errorMessage;   ///< Error message if failed
    
    /// Check if alignment was successful
    explicit operator bool() const { return success; }
    
    /// Create a successful result
    static AlignmentResult createSuccess(const glm::mat4& mat);
    
    /// Create a failed result
    static AlignmentResult createFailure(const std::string& error);
    
    /// Decompose transform matrix into translation, rotation, scale
    void decomposeTransform();
    
    /// Compose transform matrix from translation, rotation, scale
    void composeTransform();
};

/**
 * @brief Options for alignment operations
 */
struct AlignmentOptions {
    bool preview = false;       ///< Preview mode (don't modify mesh)
    bool computeError = true;   ///< Compute alignment error statistics
    float tolerance = 1e-6f;    ///< Numerical tolerance
};

/**
 * @brief Core alignment algorithms for mesh positioning
 */
class Alignment {
public:
    /**
     * @brief Align mesh to World Coordinate System using feature constraints
     * 
     * @param mesh Mesh to align (modified in place unless preview)
     * @param primary Primary axis feature (defines one axis direction)
     * @param primaryAxis Which WCS axis the primary feature aligns to
     * @param secondary Secondary axis feature (defines second axis)
     * @param secondaryAxis Which WCS axis the secondary feature aligns to
     * @param origin Optional origin point (defaults to primary feature point)
     * @param options Alignment options
     * @return AlignmentResult with transformation and error statistics
     * 
     * The tertiary axis is computed as cross product of primary and secondary.
     * If secondary is not orthogonal to primary, it will be orthogonalized.
     */
    static AlignmentResult alignToWCS(
        MeshData& mesh,
        const AlignmentFeatureData& primary,
        WCSAxis primaryAxis,
        const AlignmentFeatureData& secondary,
        WCSAxis secondaryAxis,
        const std::optional<glm::vec3>& origin = std::nullopt,
        const AlignmentOptions& options = {}
    );
    
    /**
     * @brief Apply an interactive transformation to a mesh
     * 
     * @param mesh Mesh to transform
     * @param transform 4x4 transformation matrix
     * @param options Alignment options
     * @return AlignmentResult
     */
    static AlignmentResult alignInteractive(
        MeshData& mesh,
        const glm::mat4& transform,
        const AlignmentOptions& options = {}
    );
    
    /**
     * @brief Align mesh B to mesh A using N point correspondences
     * 
     * Uses least squares to find optimal rigid transformation.
     * Requires minimum 3 non-collinear point pairs.
     * 
     * @param meshB Mesh to transform (source)
     * @param meshA Reference mesh (target, unchanged)
     * @param pointPairs Point correspondences (source -> target)
     * @param options Alignment options
     * @return AlignmentResult with transformation
     */
    static AlignmentResult alignByNPoints(
        MeshData& meshB,
        const MeshData& meshA,
        const std::vector<PointPair>& pointPairs,
        const AlignmentOptions& options = {}
    );
    
    /**
     * @brief Fine align mesh B to mesh A using ICP
     * 
     * @param meshB Mesh to transform (modified)
     * @param meshA Reference mesh (unchanged)
     * @param maxIterations Maximum ICP iterations
     * @param convergenceThreshold Stop when change < threshold
     * @param progress Progress callback
     * @return AlignmentResult with final transformation
     */
    static AlignmentResult fineAlign(
        MeshData& meshB,
        const MeshData& meshA,
        int maxIterations = 50,
        float convergenceThreshold = 1e-5f,
        ProgressCallback progress = nullptr
    );
    
    /**
     * @brief Compute alignment error between two point sets
     * 
     * @param sourcePoints Source points
     * @param targetPoints Target points (same size as source)
     * @return Pair of (RMS error, max error)
     */
    static std::pair<float, float> computeError(
        const std::vector<glm::vec3>& sourcePoints,
        const std::vector<glm::vec3>& targetPoints
    );
    
    /**
     * @brief Compute centroid of a point set
     */
    static glm::vec3 computeCentroid(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Compute best-fit rigid transform using SVD
     * 
     * Finds optimal rotation and translation to align source to target points.
     * Uses Kabsch/Umeyama algorithm.
     * 
     * @param sourcePoints Source point set
     * @param targetPoints Target point set
     * @param weights Optional per-point weights
     * @return Transformation matrix
     */
    static glm::mat4 computeRigidTransform(
        const std::vector<glm::vec3>& sourcePoints,
        const std::vector<glm::vec3>& targetPoints,
        const std::vector<float>& weights = {}
    );
    
private:
    /// Get unit direction vector for a WCS axis
    static glm::vec3 getAxisDirection(WCSAxis axis);
    
    /// Build rotation matrix from two axes
    static glm::mat3 buildRotationFromAxes(
        const glm::vec3& fromPrimary,
        const glm::vec3& fromSecondary,
        const glm::vec3& toPrimary,
        const glm::vec3& toSecondary
    );
};

} // namespace geometry
} // namespace dc3d
