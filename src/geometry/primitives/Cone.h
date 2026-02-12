/**
 * @file Cone.h
 * @brief Cone primitive for geometric fitting
 * 
 * Represents a cone with apex, axis direction, and half-angle.
 * Supports iterative fitting from point clouds.
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Cone fitting result with quality metrics
 */
struct ConeFitResult {
    bool success = false;
    float rmsError = 0.0f;          ///< Root mean square error
    float maxError = 0.0f;          ///< Maximum deviation
    size_t inlierCount = 0;         ///< Points within threshold
    std::string errorMessage;
};

/**
 * @brief Cone fitting options
 */
struct ConeFitOptions {
    int ransacIterations = 500;     ///< Number of RANSAC iterations
    float inlierThreshold = 0.01f;  ///< Distance threshold for inliers
    int refinementIterations = 20;  ///< Iterative refinement steps
    bool useNormals = true;         ///< Use normals for fitting
};

/**
 * @brief 3D cone primitive
 * 
 * Represented by:
 * - Apex: the tip of the cone
 * - Axis: unit direction from apex toward base
 * - Half-angle: angle between axis and surface (radians)
 * - Height: distance from apex to base (optional, for truncated cones)
 */
class Cone {
public:
    Cone() = default;
    
    /**
     * @brief Construct cone with all parameters
     */
    Cone(const glm::vec3& apex, const glm::vec3& axis, float halfAngle, float height = 1.0f);
    
    // ---- Fitting ----
    
    /**
     * @brief Fit cone to point cloud using iterative method
     * @param points Input points
     * @param options Fitting options
     * @return Fit result with quality metrics
     */
    ConeFitResult fitToPoints(const std::vector<glm::vec3>& points,
                              const ConeFitOptions& options = ConeFitOptions());
    
    /**
     * @brief Fit cone to points with known normals
     */
    ConeFitResult fitToPointsWithNormals(const std::vector<glm::vec3>& points,
                                          const std::vector<glm::vec3>& normals,
                                          const ConeFitOptions& options = ConeFitOptions());
    
    /**
     * @brief Fit cone to selected faces of a mesh
     */
    ConeFitResult fitToSelection(const MeshData& mesh, 
                                  const std::vector<uint32_t>& selectedFaces,
                                  const ConeFitOptions& options = ConeFitOptions());
    
    // ---- Queries ----
    
    /**
     * @brief Signed distance from point to cone surface
     * @return Positive if outside, negative if inside
     */
    float distanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Unsigned distance
     */
    float absoluteDistanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Project point onto cone surface
     */
    glm::vec3 projectPoint(const glm::vec3& point) const;
    
    /**
     * @brief Check if point is inside the cone
     */
    bool containsPoint(const glm::vec3& point) const;
    
    /**
     * @brief Get radius at a given height from apex
     */
    float radiusAtHeight(float height) const;
    
    /**
     * @brief Get base center and radius
     */
    void getBase(glm::vec3& center, float& radius) const;
    
    /**
     * @brief Intersect ray with cone
     * @return Number of intersections (0, 1, or 2)
     */
    int intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                     float& t1, float& t2) const;
    
    // ---- Accessors ----
    
    const glm::vec3& apex() const { return m_apex; }
    const glm::vec3& axis() const { return m_axis; }
    float halfAngle() const { return m_halfAngle; }
    float height() const { return m_height; }
    
    void setApex(const glm::vec3& a) { m_apex = a; }
    void setAxis(const glm::vec3& a) { m_axis = glm::normalize(a); }
    void setHalfAngle(float a) { m_halfAngle = a; }
    void setHeight(float h) { m_height = h; }
    
    /**
     * @brief Get half-angle in degrees
     */
    float halfAngleDegrees() const;
    
    /**
     * @brief Set half-angle in degrees
     */
    void setHalfAngleDegrees(float degrees);
    
    /**
     * @brief Check if cone is valid
     */
    bool isValid() const;
    
    /**
     * @brief Get lateral surface area
     */
    float surfaceArea() const;
    
    /**
     * @brief Get volume
     */
    float volume() const;
    
    // ---- Transformations ----
    
    void transform(const glm::mat4& matrix);
    Cone transformed(const glm::mat4& matrix) const;
    
    // ---- Sampling ----
    
    /**
     * @brief Generate points on cone surface
     */
    std::vector<glm::vec3> sampleSurface(int radialSegments = 32, 
                                          int heightSegments = 8) const;

private:
    glm::vec3 m_apex{0.0f};             ///< Cone apex (tip)
    glm::vec3 m_axis{0.0f, 1.0f, 0.0f}; ///< Unit axis from apex to base
    float m_halfAngle = 0.5f;            ///< Half-angle in radians
    float m_height = 1.0f;               ///< Height from apex to base
    
    // Cached values
    float m_cosHalfAngle = 0.0f;
    float m_sinHalfAngle = 0.0f;
    void updateTrigCache();
    
    // Internal fitting helpers
    glm::vec3 estimateAxisFromNormals(const std::vector<glm::vec3>& normals);
    void fitApexAndAngle(const std::vector<glm::vec3>& points, const glm::vec3& axis);
    void refineIteratively(const std::vector<glm::vec3>& points, 
                          const std::vector<glm::vec3>& normals,
                          int iterations);
};

} // namespace geometry
} // namespace dc3d
