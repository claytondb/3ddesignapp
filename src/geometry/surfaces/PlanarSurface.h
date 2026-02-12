/**
 * @file PlanarSurface.h
 * @brief Planar surface creation from closed profiles
 * 
 * Creates flat surfaces from closed 2D sketches/profiles.
 * Supports:
 * - Simple closed profiles
 * - Profiles with holes
 * - Polygon triangulation
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
 * @brief Options for planar surface creation
 */
struct PlanarSurfaceOptions {
    bool flipNormal = false;         ///< Flip the surface normal
    bool bothSides = false;          ///< Create geometry for both sides
    float thickness = 0.0f;          ///< Add thickness (creates a thin solid)
};

/**
 * @brief Result of planar surface creation
 */
struct PlanarSurfaceResult {
    bool success = false;
    std::string errorMessage;
    
    MeshData mesh;                    ///< Triangulated mesh
    NURBSSurface surface;             ///< NURBS surface representation
    
    glm::vec3 normal;                 ///< Surface normal
    glm::vec3 centroid;               ///< Surface centroid
    float area;                       ///< Surface area
};

/**
 * @brief Closed planar profile for surface creation
 */
class PlanarProfile {
public:
    PlanarProfile() = default;
    
    /**
     * @brief Create profile from outer boundary points
     */
    explicit PlanarProfile(const std::vector<glm::vec3>& outerBoundary);
    
    /**
     * @brief Set outer boundary
     */
    void setOuterBoundary(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Add a hole
     */
    void addHole(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Clear all holes
     */
    void clearHoles();
    
    /**
     * @brief Get outer boundary
     */
    const std::vector<glm::vec3>& outerBoundary() const { return m_outer; }
    
    /**
     * @brief Get holes
     */
    const std::vector<std::vector<glm::vec3>>& holes() const { return m_holes; }
    
    /**
     * @brief Check if profile is valid
     */
    bool isValid() const { return m_outer.size() >= 3; }
    
    /**
     * @brief Get computed plane normal
     */
    glm::vec3 normal() const;
    
    /**
     * @brief Get centroid
     */
    glm::vec3 centroid() const;
    
    /**
     * @brief Get area (excluding holes)
     */
    float area() const;
    
    /**
     * @brief Get bounding box
     */
    BoundingBox boundingBox() const;
    
    /**
     * @brief Ensure correct winding (outer CCW, holes CW)
     */
    void ensureCorrectWinding();
    
    /**
     * @brief Project all points onto the best-fit plane
     */
    void flattenToPlane();

private:
    std::vector<glm::vec3> m_outer;
    std::vector<std::vector<glm::vec3>> m_holes;
    
    // Compute signed area for winding check
    float signedArea() const;
};

/**
 * @brief Planar surface creation operations
 */
class PlanarSurface {
public:
    PlanarSurface() = default;
    
    /**
     * @brief Create planar surface from closed profile
     * @param profile Closed profile with optional holes
     * @param options Surface options
     * @return Result with triangulated mesh
     */
    static PlanarSurfaceResult createPlanar(const PlanarProfile& profile,
                                             const PlanarSurfaceOptions& options = PlanarSurfaceOptions());
    
    /**
     * @brief Create planar surface from boundary points
     */
    static PlanarSurfaceResult createPlanar(const std::vector<glm::vec3>& boundary,
                                             const PlanarSurfaceOptions& options = PlanarSurfaceOptions());
    
    /**
     * @brief Create planar surface with holes
     */
    static PlanarSurfaceResult createPlanarWithHoles(const std::vector<glm::vec3>& outer,
                                                      const std::vector<std::vector<glm::vec3>>& holes,
                                                      const PlanarSurfaceOptions& options = PlanarSurfaceOptions());
    
    /**
     * @brief Create a rectangular planar surface
     * @param center Center point
     * @param normal Surface normal
     * @param width Width (in local X direction)
     * @param height Height (in local Y direction)
     */
    static MeshData createRectangle(const glm::vec3& center,
                                     const glm::vec3& normal,
                                     float width,
                                     float height);
    
    /**
     * @brief Create a circular planar surface (disk)
     * @param center Center point
     * @param normal Surface normal
     * @param radius Disk radius
     * @param segments Number of segments
     */
    static MeshData createDisk(const glm::vec3& center,
                                const glm::vec3& normal,
                                float radius,
                                int segments = 32);
    
    /**
     * @brief Create an annular (ring) planar surface
     * @param center Center point
     * @param normal Surface normal
     * @param innerRadius Inner radius
     * @param outerRadius Outer radius
     * @param segments Number of segments
     */
    static MeshData createAnnulus(const glm::vec3& center,
                                   const glm::vec3& normal,
                                   float innerRadius,
                                   float outerRadius,
                                   int segments = 32);
    
    /**
     * @brief Create an elliptical planar surface
     */
    static MeshData createEllipse(const glm::vec3& center,
                                   const glm::vec3& normal,
                                   float radiusX,
                                   float radiusY,
                                   int segments = 32);
    
    /**
     * @brief Create a regular polygon planar surface
     * @param center Center point
     * @param normal Surface normal
     * @param radius Circumradius
     * @param sides Number of sides
     */
    static MeshData createRegularPolygon(const glm::vec3& center,
                                          const glm::vec3& normal,
                                          float radius,
                                          int sides);
    
    /**
     * @brief Create NURBS surface representation
     */
    static NURBSSurface createNurbsSurface(const PlanarProfile& profile);
    
    /**
     * @brief Triangulate a polygon with holes using ear clipping
     * @param outer Outer boundary points (CCW)
     * @param holes Hole boundaries (CW each)
     * @param normal Surface normal for orientation
     * @return Triangulated mesh
     */
    static MeshData triangulate(const std::vector<glm::vec3>& outer,
                                 const std::vector<std::vector<glm::vec3>>& holes,
                                 const glm::vec3& normal);

private:
    // Ear clipping triangulation
    static MeshData earClipTriangulate(const std::vector<glm::vec3>& polygon,
                                        const glm::vec3& normal);
    
    // Check if point is inside polygon
    static bool isPointInPolygon(const glm::vec3& point,
                                  const std::vector<glm::vec3>& polygon);
    
    // Check if ear is valid (no other vertices inside)
    static bool isEar(const std::vector<size_t>& indices,
                      const std::vector<glm::vec3>& vertices,
                      size_t prev, size_t curr, size_t next,
                      const glm::vec3& normal);
    
    // Merge outer boundary and holes into single polygon
    static std::vector<glm::vec3> mergePolygonWithHoles(
        const std::vector<glm::vec3>& outer,
        const std::vector<std::vector<glm::vec3>>& holes);
};

} // namespace geometry
} // namespace dc3d
