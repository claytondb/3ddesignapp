/**
 * @file Plane.h
 * @brief Plane primitive for geometric fitting
 * 
 * Represents an infinite plane defined by normal and distance from origin.
 * Supports least-squares fitting to point clouds and mesh selections.
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Plane fitting result with quality metrics
 */
struct PlaneFitResult {
    bool success = false;
    float rmsError = 0.0f;          ///< Root mean square error of fit
    float maxError = 0.0f;          ///< Maximum distance from any point
    size_t inlierCount = 0;         ///< Points within threshold
    std::string errorMessage;
};

/**
 * @brief 3D plane primitive
 * 
 * Represented in Hesse normal form: n·x + d = 0
 * where n is the unit normal and d is the signed distance from origin.
 */
class Plane {
public:
    Plane() = default;
    
    /**
     * @brief Construct from normal and distance
     * @param normal Unit normal vector (will be normalized)
     * @param distance Signed distance from origin
     */
    Plane(const glm::vec3& normal, float distance);
    
    /**
     * @brief Construct from normal and point on plane
     * @param normal Unit normal vector
     * @param pointOnPlane Any point lying on the plane
     */
    static Plane fromPointAndNormal(const glm::vec3& point, const glm::vec3& normal);
    
    /**
     * @brief Construct from three points
     * @param p1, p2, p3 Three non-collinear points on the plane
     */
    static Plane fromThreePoints(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
    
    // ---- Fitting ----
    
    /**
     * @brief Fit plane to point cloud using least squares (SVD)
     * @param points Input points (minimum 3 required)
     * @return Fit result with quality metrics
     */
    PlaneFitResult fitToPoints(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Fit plane to selected faces of a mesh
     * @param mesh The source mesh
     * @param selectedFaces Indices of selected faces
     * @return Fit result with quality metrics
     */
    PlaneFitResult fitToSelection(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces);
    
    /**
     * @brief Fit using RANSAC for robust fitting with outliers
     * @param points Input points
     * @param distanceThreshold Max distance for inlier classification
     * @param iterations Number of RANSAC iterations
     * @return Fit result with quality metrics
     */
    PlaneFitResult fitRANSAC(const std::vector<glm::vec3>& points, 
                             float distanceThreshold = 0.01f,
                             int iterations = 100);
    
    // ---- Queries ----
    
    /**
     * @brief Signed distance from point to plane
     * @return Positive if point is on normal side, negative otherwise
     */
    float distanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Unsigned (absolute) distance from point to plane
     */
    float absoluteDistanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Project point onto plane
     * @return Closest point on plane to input point
     */
    glm::vec3 projectPoint(const glm::vec3& point) const;
    
    /**
     * @brief Check which side of plane a point is on
     * @return 1 if on positive (normal) side, -1 if negative, 0 if on plane
     */
    int whichSide(const glm::vec3& point, float tolerance = 1e-6f) const;
    
    /**
     * @brief Get arbitrary point on the plane
     */
    glm::vec3 getPointOnPlane() const;
    
    /**
     * @brief Intersect with a ray
     * @param rayOrigin Ray starting point
     * @param rayDir Ray direction (should be normalized)
     * @param t Output parameter: distance along ray to intersection
     * @return true if intersection exists (not parallel)
     */
    bool intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& t) const;
    
    /**
     * @brief Intersect with another plane
     * @param other The other plane
     * @param linePoint Output: point on intersection line
     * @param lineDir Output: direction of intersection line
     * @return true if planes intersect (not parallel)
     */
    bool intersectPlane(const Plane& other, glm::vec3& linePoint, glm::vec3& lineDir) const;
    
    // ---- Accessors ----
    
    const glm::vec3& normal() const { return m_normal; }
    float distance() const { return m_distance; }
    
    void setNormal(const glm::vec3& n) { m_normal = glm::normalize(n); }
    void setDistance(float d) { m_distance = d; }
    
    /**
     * @brief Get plane equation coefficients (a, b, c, d) where ax + by + cz + d = 0
     */
    glm::vec4 equation() const { return glm::vec4(m_normal, m_distance); }
    
    /**
     * @brief Flip the plane (reverse normal direction)
     */
    void flip();
    
    /**
     * @brief Check if plane is valid (has unit normal)
     */
    bool isValid() const;
    
    // ---- Transformations ----
    
    /**
     * @brief Transform plane by a matrix
     */
    void transform(const glm::mat4& matrix);
    
    /**
     * @brief Create transformed copy
     */
    Plane transformed(const glm::mat4& matrix) const;
    
    // ---- Basis ----
    
    /**
     * @brief Get two orthogonal vectors in the plane
     * @param u Output: first basis vector
     * @param v Output: second basis vector
     */
    void getBasis(glm::vec3& u, glm::vec3& v) const;

private:
    glm::vec3 m_normal{0.0f, 1.0f, 0.0f};  ///< Unit normal
    float m_distance = 0.0f;                ///< Signed distance from origin
    
    // Internal fitting helper using SVD
    void fitLeastSquares(const std::vector<glm::vec3>& points, glm::vec3& centroid);
};

} // namespace geometry
} // namespace dc3d
