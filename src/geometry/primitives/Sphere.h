/**
 * @file Sphere.h
 * @brief Sphere primitive for geometric fitting
 * 
 * Represents a sphere with center and radius.
 * Supports algebraic and geometric fitting from point clouds.
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Sphere fitting result with quality metrics
 */
struct SphereFitResult {
    bool success = false;
    float rmsError = 0.0f;          ///< Root mean square error
    float maxError = 0.0f;          ///< Maximum deviation from surface
    size_t inlierCount = 0;         ///< Points within threshold
    std::string errorMessage;
};

/**
 * @brief Sphere fitting options
 */
struct SphereFitOptions {
    int ransacIterations = 200;     ///< Number of RANSAC iterations
    float inlierThreshold = 0.01f;  ///< Distance threshold for inliers
    bool useAlgebraicFit = true;    ///< Use algebraic (faster) vs geometric fit
};

/**
 * @brief 3D sphere primitive
 * 
 * Simple representation: center point and radius.
 */
class Sphere {
public:
    Sphere() = default;
    
    /**
     * @brief Construct sphere with center and radius
     */
    Sphere(const glm::vec3& center, float radius);
    
    // ---- Fitting ----
    
    /**
     * @brief Fit sphere to point cloud using algebraic method
     * 
     * Uses the algebraic sphere fit which minimizes:
     * sum of (|p - c|^2 - r^2)^2
     * This is fast and works well for nearly spherical point sets.
     * 
     * @param points Input points (minimum 4 required)
     * @param options Fitting options
     */
    SphereFitResult fitToPoints(const std::vector<glm::vec3>& points,
                                 const SphereFitOptions& options = SphereFitOptions());
    
    /**
     * @brief Fit sphere using RANSAC for robustness to outliers
     */
    SphereFitResult fitRANSAC(const std::vector<glm::vec3>& points,
                               const SphereFitOptions& options = SphereFitOptions());
    
    /**
     * @brief Fit sphere to selected faces of a mesh
     */
    SphereFitResult fitToSelection(const MeshData& mesh, 
                                    const std::vector<uint32_t>& selectedFaces,
                                    const SphereFitOptions& options = SphereFitOptions());
    
    /**
     * @brief Fit sphere from exactly 4 points (circumsphere)
     * @return true if a unique sphere exists through the 4 points
     */
    bool fitFromFourPoints(const glm::vec3& p1, const glm::vec3& p2,
                           const glm::vec3& p3, const glm::vec3& p4);
    
    // ---- Queries ----
    
    /**
     * @brief Signed distance from point to sphere surface
     * @return Positive if outside, negative if inside
     */
    float distanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Unsigned distance
     */
    float absoluteDistanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Project point onto sphere surface
     */
    glm::vec3 projectPoint(const glm::vec3& point) const;
    
    /**
     * @brief Check if point is inside sphere
     */
    bool containsPoint(const glm::vec3& point) const;
    
    /**
     * @brief Get normal at a point on the sphere
     */
    glm::vec3 normalAt(const glm::vec3& surfacePoint) const;
    
    /**
     * @brief Intersect ray with sphere
     * @param rayOrigin Ray starting point
     * @param rayDir Ray direction (normalized)
     * @param t1, t2 Output: distances to intersections (t1 < t2)
     * @return Number of intersections (0, 1, or 2)
     */
    int intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                     float& t1, float& t2) const;
    
    /**
     * @brief Check if this sphere intersects another
     */
    bool intersectsSphere(const Sphere& other) const;
    
    /**
     * @brief Get bounding box
     */
    void getBoundingBox(glm::vec3& min, glm::vec3& max) const;
    
    // ---- Accessors ----
    
    const glm::vec3& center() const { return m_center; }
    float radius() const { return m_radius; }
    
    void setCenter(const glm::vec3& c) { m_center = c; }
    void setRadius(float r) { m_radius = r; }
    
    /**
     * @brief Check if sphere is valid
     */
    bool isValid() const;
    
    /**
     * @brief Get surface area (4 * pi * r^2)
     */
    float surfaceArea() const;
    
    /**
     * @brief Get volume (4/3 * pi * r^3)
     */
    float volume() const;
    
    /**
     * @brief Get diameter
     */
    float diameter() const { return 2.0f * m_radius; }
    
    // ---- Transformations ----
    
    void transform(const glm::mat4& matrix);
    Sphere transformed(const glm::mat4& matrix) const;
    
    // ---- Sampling ----
    
    /**
     * @brief Generate points on sphere surface using spherical coordinates
     */
    std::vector<glm::vec3> sampleSurface(int latSegments = 16, 
                                          int lonSegments = 32) const;
    
    /**
     * @brief Generate points uniformly distributed on sphere surface
     */
    std::vector<glm::vec3> sampleUniform(int numPoints) const;

private:
    glm::vec3 m_center{0.0f};
    float m_radius = 1.0f;
    
    // Internal fitting helpers
    void fitAlgebraic(const std::vector<glm::vec3>& points);
    void fitGeometric(const std::vector<glm::vec3>& points, int iterations = 10);
};

} // namespace geometry
} // namespace dc3d
