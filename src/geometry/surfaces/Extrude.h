/**
 * @file Extrude.h
 * @brief Linear extrusion of 2D sketches to create 3D surfaces/solids
 * 
 * Supports:
 * - Basic linear extrusion
 * - Draft angle for tapered extrusion
 * - Two-sided (symmetric) extrusion
 * - Cap ends for solid creation
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
 * @brief Options for extrusion operation
 */
struct ExtrudeOptions {
    glm::vec3 direction{0.0f, 0.0f, 1.0f};  ///< Extrusion direction (normalized)
    float distance = 1.0f;                    ///< Extrusion distance
    float draftAngle = 0.0f;                  ///< Draft angle in degrees (0 = straight)
    bool twoSided = false;                    ///< Extrude in both directions
    float twoSidedRatio = 0.5f;               ///< Ratio of distance in negative direction
    bool capEnds = true;                      ///< Create caps for solid
    int tessellationU = 32;                   ///< Tessellation along profile
    int tessellationV = 4;                    ///< Tessellation along extrusion
};

/**
 * @brief Result of extrusion operation
 */
struct ExtrudeResult {
    bool success = false;
    std::string errorMessage;
    
    std::vector<NURBSSurface> surfaces;       ///< NURBS surfaces if requested
    MeshData mesh;                             ///< Tessellated mesh
    
    // Topology info
    std::vector<uint32_t> capStartFaces;       ///< Face indices of start cap
    std::vector<uint32_t> capEndFaces;         ///< Face indices of end cap
    std::vector<uint32_t> lateralFaces;        ///< Face indices of lateral surface
};

/**
 * @brief 2D profile representation for extrusion
 * 
 * Can represent a single closed contour or multiple contours
 * (outer boundary + holes)
 */
class ExtrudeProfile {
public:
    ExtrudeProfile() = default;
    
    /**
     * @brief Create profile from a single closed polyline
     * @param points 2D or 3D points defining the profile (will be projected to plane)
     */
    explicit ExtrudeProfile(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Add outer boundary
     */
    void setOuterBoundary(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Add a hole (inner boundary)
     */
    void addHole(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Set the profile plane
     */
    void setPlane(const glm::vec3& origin, const glm::vec3& normal);
    
    /**
     * @brief Get outer boundary
     */
    const std::vector<glm::vec3>& outerBoundary() const { return m_outer; }
    
    /**
     * @brief Get holes
     */
    const std::vector<std::vector<glm::vec3>>& holes() const { return m_holes; }
    
    /**
     * @brief Get plane origin
     */
    const glm::vec3& planeOrigin() const { return m_planeOrigin; }
    
    /**
     * @brief Get plane normal
     */
    const glm::vec3& planeNormal() const { return m_planeNormal; }
    
    /**
     * @brief Check if profile is closed and valid
     */
    bool isValid() const;
    
    /**
     * @brief Compute area (positive for CCW, negative for CW)
     */
    float signedArea() const;
    
    /**
     * @brief Ensure outer boundary is CCW and holes are CW
     */
    void ensureCorrectWinding();
    
    /**
     * @brief Get centroid of outer boundary
     */
    glm::vec3 centroid() const;
    
    /**
     * @brief Get bounding box
     */
    BoundingBox boundingBox() const;

private:
    std::vector<glm::vec3> m_outer;
    std::vector<std::vector<glm::vec3>> m_holes;
    glm::vec3 m_planeOrigin{0.0f};
    glm::vec3 m_planeNormal{0.0f, 0.0f, 1.0f};
};

/**
 * @brief Linear extrusion operations
 */
class Extrude {
public:
    Extrude() = default;
    
    /**
     * @brief Basic linear extrusion
     * @param profile The 2D profile to extrude
     * @param direction Extrusion direction (will be normalized)
     * @param distance Extrusion distance
     * @return Extrusion result with mesh and optional surfaces
     */
    static ExtrudeResult extrude(const ExtrudeProfile& profile,
                                  const glm::vec3& direction,
                                  float distance);
    
    /**
     * @brief Linear extrusion with full options
     */
    static ExtrudeResult extrude(const ExtrudeProfile& profile,
                                  const ExtrudeOptions& options);
    
    /**
     * @brief Extrusion with draft angle (tapered extrusion)
     * @param profile The 2D profile
     * @param direction Extrusion direction
     * @param distance Extrusion distance
     * @param draftAngle Draft angle in degrees (positive = expand)
     */
    static ExtrudeResult extrudeWithDraft(const ExtrudeProfile& profile,
                                           const glm::vec3& direction,
                                           float distance,
                                           float draftAngle);
    
    /**
     * @brief Two-sided extrusion (symmetric about profile plane)
     * @param profile The 2D profile
     * @param direction Extrusion direction
     * @param totalDistance Total distance (split between both sides)
     * @param ratio Ratio of distance in positive direction (0.5 = symmetric)
     */
    static ExtrudeResult extrudeTwoSided(const ExtrudeProfile& profile,
                                          const glm::vec3& direction,
                                          float totalDistance,
                                          float ratio = 0.5f);
    
    /**
     * @brief Extrude along a normal direction of the profile plane
     */
    static ExtrudeResult extrudeNormal(const ExtrudeProfile& profile,
                                        float distance,
                                        const ExtrudeOptions& options = ExtrudeOptions());
    
    /**
     * @brief Create NURBS surfaces for the extrusion
     * @return Vector of surfaces (lateral + optional caps)
     */
    static std::vector<NURBSSurface> createSurfaces(const ExtrudeProfile& profile,
                                                     const ExtrudeOptions& options);
    
    /**
     * @brief Create only the lateral surface mesh (no caps)
     */
    static MeshData createLateralMesh(const ExtrudeProfile& profile,
                                       const ExtrudeOptions& options);
    
    /**
     * @brief Create cap mesh for start or end
     */
    static MeshData createCapMesh(const ExtrudeProfile& profile,
                                   const glm::vec3& offset,
                                   bool flipNormals);

private:
    // Helper to apply draft to a profile point
    static glm::vec3 applyDraft(const glm::vec3& point,
                                 const glm::vec3& profileCenter,
                                 const glm::vec3& direction,
                                 float distance,
                                 float draftAngle);
    
    // Triangulate a planar polygon with holes
    static MeshData triangulatePolygon(const std::vector<glm::vec3>& outer,
                                        const std::vector<std::vector<glm::vec3>>& holes,
                                        const glm::vec3& normal);
};

} // namespace geometry
} // namespace dc3d
