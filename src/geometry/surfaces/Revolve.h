/**
 * @file Revolve.h
 * @brief Revolution of 2D sketches around an axis to create 3D surfaces/solids
 * 
 * Supports:
 * - Full 360° revolution
 * - Partial revolution (any angle)
 * - Cap ends for solid creation
 * - NURBS surface output
 */

#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../NURBSSurface.h"
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Axis definition for revolution
 */
struct RevolutionAxis {
    glm::vec3 origin{0.0f};      ///< Point on the axis
    glm::vec3 direction{0.0f, 1.0f, 0.0f};  ///< Axis direction (will be normalized)
    
    RevolutionAxis() = default;
    RevolutionAxis(const glm::vec3& org, const glm::vec3& dir)
        : origin(org), direction(glm::normalize(dir)) {}
    
    /// Create axis from two points
    static RevolutionAxis fromPoints(const glm::vec3& p1, const glm::vec3& p2) {
        return RevolutionAxis(p1, p2 - p1);
    }
    
    /// Standard axes
    static RevolutionAxis xAxis(const glm::vec3& origin = glm::vec3(0.0f)) {
        return RevolutionAxis(origin, glm::vec3(1, 0, 0));
    }
    static RevolutionAxis yAxis(const glm::vec3& origin = glm::vec3(0.0f)) {
        return RevolutionAxis(origin, glm::vec3(0, 1, 0));
    }
    static RevolutionAxis zAxis(const glm::vec3& origin = glm::vec3(0.0f)) {
        return RevolutionAxis(origin, glm::vec3(0, 0, 1));
    }
};

/**
 * @brief Options for revolution operation
 */
struct RevolveOptions {
    RevolutionAxis axis;
    float startAngle = 0.0f;          ///< Start angle in degrees
    float endAngle = 360.0f;          ///< End angle in degrees (360 = full revolution)
    bool capEnds = true;              ///< Create caps for partial revolution
    int circumferentialSegments = 32; ///< Segments around the revolution
    int profileSegments = 1;          ///< Segments along profile edges (for curved profiles)
    
    /// Get total angle of revolution
    float sweepAngle() const { return endAngle - startAngle; }
    
    /// Check if this is a full revolution
    bool isFullRevolution() const { return std::abs(sweepAngle() - 360.0f) < 0.1f; }
};

/**
 * @brief Result of revolution operation
 */
struct RevolveResult {
    bool success = false;
    std::string errorMessage;
    
    std::vector<NURBSSurface> surfaces;       ///< NURBS surfaces
    MeshData mesh;                             ///< Tessellated mesh
    
    // Topology info
    std::vector<uint32_t> capStartFaces;       ///< Face indices of start cap
    std::vector<uint32_t> capEndFaces;         ///< Face indices of end cap
    std::vector<uint32_t> lateralFaces;        ///< Face indices of revolution surface
};

/**
 * @brief Profile for revolution (polyline in 2D)
 */
class RevolveProfile {
public:
    RevolveProfile() = default;
    
    /**
     * @brief Create profile from points
     * @param points Points defining the profile (in order along profile)
     * @param closed Whether the profile is closed (forms a loop)
     */
    explicit RevolveProfile(const std::vector<glm::vec3>& points, bool closed = false);
    
    /**
     * @brief Set profile points
     */
    void setPoints(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Set whether profile is closed
     */
    void setClosed(bool closed) { m_closed = closed; }
    
    /**
     * @brief Get profile points
     */
    const std::vector<glm::vec3>& points() const { return m_points; }
    
    /**
     * @brief Check if profile is closed
     */
    bool isClosed() const { return m_closed; }
    
    /**
     * @brief Check if profile is valid for revolution
     */
    bool isValid() const { return m_points.size() >= 2; }
    
    /**
     * @brief Get distance from a point to the axis
     */
    float distanceToAxis(const RevolutionAxis& axis, size_t pointIndex) const;
    
    /**
     * @brief Check if profile intersects the axis
     */
    bool intersectsAxis(const RevolutionAxis& axis) const;
    
    /**
     * @brief Get bounding box
     */
    BoundingBox boundingBox() const;

private:
    std::vector<glm::vec3> m_points;
    bool m_closed = false;
};

/**
 * @brief Revolution surface operations
 */
class Revolve {
public:
    Revolve() = default;
    
    /**
     * @brief Revolve a profile around an axis
     * @param profile The profile to revolve
     * @param axis The axis of revolution
     * @param angle Revolution angle in degrees (360 = full)
     * @return Revolution result with mesh
     */
    static RevolveResult revolve(const RevolveProfile& profile,
                                  const RevolutionAxis& axis,
                                  float angle = 360.0f);
    
    /**
     * @brief Revolve with full options
     */
    static RevolveResult revolve(const RevolveProfile& profile,
                                  const RevolveOptions& options);
    
    /**
     * @brief Revolve from points directly
     */
    static RevolveResult revolve(const std::vector<glm::vec3>& profilePoints,
                                  const RevolutionAxis& axis,
                                  float angle = 360.0f,
                                  bool closedProfile = false);
    
    /**
     * @brief Create a surface of revolution as NURBS
     */
    static std::vector<NURBSSurface> createSurfaces(const RevolveProfile& profile,
                                                     const RevolveOptions& options);
    
    /**
     * @brief Create only the revolution surface mesh (no caps)
     */
    static MeshData createRevolutionMesh(const RevolveProfile& profile,
                                          const RevolveOptions& options);
    
    /**
     * @brief Create a simple torus
     * @param majorRadius Distance from axis to center of tube
     * @param minorRadius Radius of the tube
     * @param axis Revolution axis
     * @param segments Circumferential segments for both major and minor circles
     */
    static MeshData createTorus(float majorRadius, float minorRadius,
                                 const RevolutionAxis& axis = RevolutionAxis::yAxis(),
                                 int segments = 32);
    
    /**
     * @brief Create a cone of revolution
     * @param baseRadius Radius at the base
     * @param height Height of the cone
     * @param axis Revolution axis (direction points to apex)
     */
    static MeshData createCone(float baseRadius, float height,
                                const RevolutionAxis& axis = RevolutionAxis::yAxis(),
                                int segments = 32);
    
    /**
     * @brief Create a sphere by revolving a semicircle
     */
    static MeshData createSphere(float radius,
                                  const glm::vec3& center = glm::vec3(0.0f),
                                  int segments = 32);

private:
    // Rotate a point around an axis
    static glm::vec3 rotateAroundAxis(const glm::vec3& point,
                                       const RevolutionAxis& axis,
                                       float angleDeg);
    
    // Get rotation matrix for angle around axis
    static glm::mat4 getRotationMatrix(const RevolutionAxis& axis, float angleDeg);
    
    // Create cap mesh for partial revolution
    static MeshData createCapMesh(const RevolveProfile& profile,
                                   const RevolutionAxis& axis,
                                   float angle,
                                   bool flipNormals);
};

} // namespace geometry
} // namespace dc3d
