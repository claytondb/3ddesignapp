/**
 * @file Cylinder.h
 * @brief Cylinder primitive for geometric fitting
 * 
 * Represents a cylinder with axis, center, radius, and height.
 * Supports RANSAC + least squares fitting from point clouds.
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Cylinder fitting result with quality metrics
 */
struct CylinderFitResult {
    bool success = false;
    float rmsError = 0.0f;          ///< Root mean square radial error
    float maxError = 0.0f;          ///< Maximum radial deviation
    size_t inlierCount = 0;         ///< Points within threshold
    std::string errorMessage;
};

/**
 * @brief Cylinder fitting options
 */
struct CylinderFitOptions {
    int ransacIterations = 500;     ///< Number of RANSAC iterations
    float inlierThreshold = 0.01f;  ///< Distance threshold for inliers
    int refinementIterations = 10;  ///< Iterative refinement steps
    bool useNormals = true;         ///< Use normals for axis estimation
};

/**
 * @brief 3D cylinder primitive
 * 
 * Represented by:
 * - Axis: unit direction vector
 * - Center: point on the axis (midpoint of height range)
 * - Radius: distance from axis
 * - Height: extent along axis
 */
class Cylinder {
public:
    Cylinder() = default;
    
    /**
     * @brief Construct cylinder with all parameters
     */
    Cylinder(const glm::vec3& center, const glm::vec3& axis, float radius, float height);
    
    // ---- Fitting ----
    
    /**
     * @brief Fit cylinder to point cloud using RANSAC + least squares
     * @param points Input points
     * @param options Fitting options
     * @return Fit result with quality metrics
     */
    CylinderFitResult fitToPoints(const std::vector<glm::vec3>& points,
                                   const CylinderFitOptions& options = CylinderFitOptions());
    
    /**
     * @brief Fit cylinder to points with known normals
     * @param points Input points
     * @param normals Per-point normals (used to estimate axis)
     * @param options Fitting options
     */
    CylinderFitResult fitToPointsWithNormals(const std::vector<glm::vec3>& points,
                                              const std::vector<glm::vec3>& normals,
                                              const CylinderFitOptions& options = CylinderFitOptions());
    
    /**
     * @brief Fit cylinder to selected faces of a mesh
     * @param mesh The source mesh
     * @param selectedFaces Indices of selected faces
     * @param options Fitting options
     */
    CylinderFitResult fitToSelection(const MeshData& mesh, 
                                      const std::vector<uint32_t>& selectedFaces,
                                      const CylinderFitOptions& options = CylinderFitOptions());
    
    // ---- Queries ----
    
    /**
     * @brief Radial distance from point to cylinder surface
     * @return Positive if outside, negative if inside
     */
    float distanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Unsigned radial distance
     */
    float absoluteDistanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Project point onto cylinder surface
     */
    glm::vec3 projectPoint(const glm::vec3& point) const;
    
    /**
     * @brief Get closest point on the axis to a given point
     */
    glm::vec3 closestPointOnAxis(const glm::vec3& point) const;
    
    /**
     * @brief Check if a point is inside the cylinder (including caps)
     */
    bool containsPoint(const glm::vec3& point) const;
    
    /**
     * @brief Get the two end cap centers
     */
    void getEndCaps(glm::vec3& bottom, glm::vec3& top) const;
    
    /**
     * @brief Intersect ray with cylinder
     * @param rayOrigin Ray starting point
     * @param rayDir Ray direction (normalized)
     * @param t1, t2 Output: distances to intersections (t1 < t2)
     * @return Number of intersections (0, 1, or 2)
     */
    int intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                     float& t1, float& t2) const;
    
    // ---- Accessors ----
    
    const glm::vec3& center() const { return m_center; }
    const glm::vec3& axis() const { return m_axis; }
    float radius() const { return m_radius; }
    float height() const { return m_height; }
    
    void setCenter(const glm::vec3& c) { m_center = c; }
    void setAxis(const glm::vec3& a) { m_axis = glm::normalize(a); }
    void setRadius(float r) { m_radius = r; }
    void setHeight(float h) { m_height = h; }
    
    /**
     * @brief Check if cylinder is valid
     */
    bool isValid() const;
    
    /**
     * @brief Get surface area (lateral + caps)
     */
    float surfaceArea() const;
    
    /**
     * @brief Get volume
     */
    float volume() const;
    
    // ---- Transformations ----
    
    void transform(const glm::mat4& matrix);
    Cylinder transformed(const glm::mat4& matrix) const;
    
    // ---- Sampling ----
    
    /**
     * @brief Generate points on cylinder surface for visualization
     * @param radialSegments Number of segments around circumference
     * @param heightSegments Number of segments along height
     * @param includeCaps Include end cap points
     */
    std::vector<glm::vec3> sampleSurface(int radialSegments = 32, 
                                          int heightSegments = 2,
                                          bool includeCaps = true) const;

private:
    glm::vec3 m_center{0.0f};           ///< Center point on axis
    glm::vec3 m_axis{0.0f, 1.0f, 0.0f}; ///< Unit axis direction
    float m_radius = 1.0f;               ///< Cylinder radius
    float m_height = 1.0f;               ///< Height along axis
    
    // Internal fitting helpers
    glm::vec3 estimateAxisFromNormals(const std::vector<glm::vec3>& normals);
    void fitRadiusAndCenter(const std::vector<glm::vec3>& points, const glm::vec3& axis);
    void computeHeightRange(const std::vector<glm::vec3>& points);
    void refineIteratively(const std::vector<glm::vec3>& points, int iterations);
};

} // namespace geometry
} // namespace dc3d
