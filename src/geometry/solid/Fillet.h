/**
 * @file Fillet.h
 * @brief Fillet (rounding) operations for solid edges and faces
 * 
 * Implements rolling ball algorithm for creating smooth rounded
 * transitions between faces. Supports constant and variable radius
 * fillets with G1/G2 continuity options.
 */

#pragma once

#include "Solid.h"
#include "../NURBSSurface.h"

#include <vector>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

/**
 * @brief Continuity type for fillet surfaces
 */
enum class FilletContinuity {
    G0,     ///< Positional continuity only (tangent discontinuity)
    G1,     ///< Tangent continuity (smooth appearance)
    G2      ///< Curvature continuity (reflection continuity)
};

/**
 * @brief Fillet profile type
 */
enum class FilletProfile {
    Circular,       ///< Standard circular arc profile
    Conic,          ///< Conic section (elliptical)
    Curvature,      ///< Curvature continuous blend
    Chamfer,        ///< Linear (actually a chamfer, not fillet)
    Custom          ///< User-defined profile curve
};

/**
 * @brief Defines radius at a specific point along an edge
 */
struct FilletRadiusPoint {
    float parameter = 0.0f;     ///< Position along edge (0-1)
    float radius = 1.0f;        ///< Radius at this point
    float rho = 0.5f;           ///< Conic parameter for conic profiles
};

/**
 * @brief Options for fillet operations
 */
struct FilletOptions {
    /// Default radius for constant-radius fillets
    float radius = 1.0f;
    
    /// Fillet profile type
    FilletProfile profile = FilletProfile::Circular;
    
    /// Surface continuity requirement
    FilletContinuity continuity = FilletContinuity::G1;
    
    /// Number of segments for fillet surface
    int segments = 8;
    
    /// Whether to propagate to tangent-connected edges
    bool tangentPropagation = true;
    
    /// Angle threshold for tangent propagation (radians)
    float tangentAngleThreshold = 0.087266f;  // ~5 degrees
    
    /// Whether to allow variable radius
    bool variableRadius = false;
    
    /// Variable radius control points (if variableRadius is true)
    std::vector<FilletRadiusPoint> radiusPoints;
    
    /// Tolerance for geometric calculations
    float tolerance = 1e-6f;
    
    /// Maximum allowed radius (prevents impossible fillets)
    float maxRadius = std::numeric_limits<float>::max();
    
    /// Whether to trim the original faces
    bool trimFaces = true;
    
    /// Whether to automatically handle corners (3+ edges meeting)
    bool handleCorners = true;
    
    /// Conic parameter for conic profiles (0.5 = circular)
    float rho = 0.5f;
    
    /// Progress callback
    ProgressCallback progress = nullptr;
};

/**
 * @brief Result of a fillet operation
 */
struct FilletResult {
    bool success = false;
    std::string error;
    
    /// Resulting solid with fillets applied
    std::optional<Solid> solid;
    
    /// Indices of newly created fillet faces
    std::vector<uint32_t> filletFaces;
    
    /// Indices of modified original faces
    std::vector<uint32_t> modifiedFaces;
    
    /// Statistics
    struct Stats {
        int edgesProcessed = 0;
        int filletFacesCreated = 0;
        int cornersProcessed = 0;
        float computeTimeMs = 0.0f;
    } stats;
    
    bool ok() const { return success; }
    explicit operator bool() const { return ok(); }
};

/**
 * @brief Represents a fillet surface between two faces
 */
struct FilletSurface {
    uint32_t edgeIndex;                 ///< Source edge being filleted
    uint32_t face0Index;                ///< First adjacent face
    uint32_t face1Index;                ///< Second adjacent face
    
    std::vector<glm::vec3> controlPoints;   ///< Surface control points
    std::vector<glm::vec3> spinePoints;     ///< Points along the fillet spine
    std::vector<float> radii;               ///< Radius at each spine point
    
    /// Generate mesh from fillet surface
    std::vector<SolidFace> generateFaces(std::vector<SolidVertex>& vertices,
                                         int segments) const;
};

/**
 * @brief Fillet operations for solid bodies
 * 
 * Creates rounded transitions between faces by:
 * 1. Computing rolling ball centers along edges
 * 2. Generating fillet surfaces
 * 3. Trimming original faces
 * 4. Stitching fillet surfaces to trimmed faces
 * 
 * The rolling ball algorithm simulates a sphere rolling along
 * the intersection of two surfaces, creating a smooth blend.
 */
class Fillet {
public:
    Fillet() = default;
    ~Fillet() = default;
    
    // ===================
    // Edge Fillets
    // ===================
    
    /**
     * @brief Apply fillet to specified edges
     * @param solid Input solid
     * @param edgeIndices Edges to fillet
     * @param options Fillet options
     * @return Result containing filleted solid
     */
    static FilletResult filletEdges(const Solid& solid,
                                    const std::vector<uint32_t>& edgeIndices,
                                    const FilletOptions& options = {});
    
    /**
     * @brief Apply fillet to edges with individual radii
     * @param solid Input solid
     * @param edgeRadii Map of edge index to radius
     * @param options Fillet options (radius field ignored)
     * @return Result containing filleted solid
     */
    static FilletResult filletEdgesWithRadii(
        const Solid& solid,
        const std::unordered_map<uint32_t, float>& edgeRadii,
        const FilletOptions& options = {});
    
    /**
     * @brief Apply variable radius fillet to an edge
     * @param solid Input solid
     * @param edgeIndex Edge to fillet
     * @param radiusPoints Radius control points along edge
     * @param options Fillet options
     * @return Result containing filleted solid
     */
    static FilletResult filletEdgeVariable(
        const Solid& solid,
        uint32_t edgeIndex,
        const std::vector<FilletRadiusPoint>& radiusPoints,
        const FilletOptions& options = {});
    
    // ===================
    // Face Fillets
    // ===================
    
    /**
     * @brief Apply fillet between two faces
     * @param solid Input solid
     * @param face0Index First face
     * @param face1Index Second face (must share an edge)
     * @param options Fillet options
     * @return Result containing filleted solid
     */
    static FilletResult filletFaces(const Solid& solid,
                                    uint32_t face0Index, uint32_t face1Index,
                                    const FilletOptions& options = {});
    
    /**
     * @brief Apply fillet to all edges of a face
     * @param solid Input solid
     * @param faceIndex Face whose edges to fillet
     * @param options Fillet options
     * @return Result containing filleted solid
     */
    static FilletResult filletFaceEdges(const Solid& solid,
                                        uint32_t faceIndex,
                                        const FilletOptions& options = {});
    
    // ===================
    // Selection Helpers
    // ===================
    
    /**
     * @brief Find all edges that can be filleted with given radius
     * @param solid Input solid
     * @param radius Fillet radius
     * @return Vector of filletable edge indices
     */
    static std::vector<uint32_t> findFilletableEdges(const Solid& solid, float radius);
    
    /**
     * @brief Calculate maximum fillet radius for an edge
     * @param solid Input solid
     * @param edgeIndex Edge to check
     * @return Maximum radius that won't cause self-intersection
     */
    static float maxFilletRadius(const Solid& solid, uint32_t edgeIndex);
    
    /**
     * @brief Find edges connected by tangent continuity
     * @param solid Input solid
     * @param startEdge Starting edge
     * @param angleThreshold Maximum angle deviation (radians)
     * @return Vector of tangent-connected edges
     */
    static std::vector<uint32_t> findTangentChain(const Solid& solid,
                                                  uint32_t startEdge,
                                                  float angleThreshold = 0.087266f);
    
    // ===================
    // Preview
    // ===================
    
    /**
     * @brief Generate preview geometry for fillet
     * @param solid Input solid
     * @param edgeIndices Edges to preview
     * @param radius Preview radius
     * @param segments Number of segments
     * @return Preview mesh (fillet surfaces only)
     */
    static MeshData generatePreview(const Solid& solid,
                                   const std::vector<uint32_t>& edgeIndices,
                                   float radius, int segments = 8);

private:
    // Internal computation methods
    
    /**
     * @brief Compute rolling ball center along an edge
     */
    static glm::vec3 computeRollingBallCenter(const glm::vec3& edgePoint,
                                              const glm::vec3& normal0,
                                              const glm::vec3& normal1,
                                              float radius);
    
    /**
     * @brief Compute fillet surface for an edge
     */
    static FilletSurface computeFilletSurface(const Solid& solid,
                                              uint32_t edgeIndex,
                                              const FilletOptions& options);
    
    /**
     * @brief Compute variable radius fillet surface
     */
    static FilletSurface computeVariableFilletSurface(
        const Solid& solid,
        uint32_t edgeIndex,
        const std::vector<FilletRadiusPoint>& radiusPoints,
        const FilletOptions& options);
    
    /**
     * @brief Generate fillet profile points
     */
    static std::vector<glm::vec3> generateFilletProfile(
        const glm::vec3& center,
        const glm::vec3& start,
        const glm::vec3& end,
        float radius,
        int segments,
        FilletProfile profile,
        float rho = 0.5f);
    
    /**
     * @brief Trim face by fillet boundary
     */
    static SolidFace trimFaceByFillet(const SolidFace& face,
                                      const std::vector<SolidVertex>& vertices,
                                      const FilletSurface& fillet,
                                      std::vector<SolidVertex>& newVertices);
    
    /**
     * @brief Compute corner blend where multiple edges meet
     */
    static std::vector<SolidFace> computeCornerBlend(
        const Solid& solid,
        uint32_t vertexIndex,
        const std::vector<uint32_t>& filletEdges,
        const std::unordered_map<uint32_t, FilletSurface>& filletSurfaces,
        std::vector<SolidVertex>& vertices,
        const FilletOptions& options);
    
    /**
     * @brief Check if fillet radius is valid for edge
     */
    static bool isValidFilletRadius(const Solid& solid,
                                    uint32_t edgeIndex,
                                    float radius);
    
    /**
     * @brief Interpolate radius along edge using control points
     */
    static float interpolateRadius(float t, const std::vector<FilletRadiusPoint>& points);
};

/**
 * @brief Rolling ball fillet algorithm implementation
 * 
 * Simulates a sphere of given radius rolling along the intersection
 * of two surfaces. The fillet surface is the locus of points on the
 * sphere touching both surfaces.
 */
class RollingBallFillet {
public:
    /**
     * @brief Initialize rolling ball fillet for an edge
     * @param solid Input solid
     * @param edgeIndex Edge to fillet
     * @param radius Ball radius
     */
    RollingBallFillet(const Solid& solid, uint32_t edgeIndex, float radius);
    
    /**
     * @brief Compute ball center at parameter t along edge
     * @param t Parameter (0-1) along edge
     * @return Ball center position
     */
    glm::vec3 ballCenter(float t) const;
    
    /**
     * @brief Compute contact point on face 0 at parameter t
     */
    glm::vec3 contactPoint0(float t) const;
    
    /**
     * @brief Compute contact point on face 1 at parameter t
     */
    glm::vec3 contactPoint1(float t) const;
    
    /**
     * @brief Generate fillet surface samples
     * @param uSamples Samples along edge
     * @param vSamples Samples across fillet
     * @return Grid of surface points [u][v]
     */
    std::vector<std::vector<glm::vec3>> generateSurface(int uSamples, int vSamples) const;
    
    /**
     * @brief Check if fillet is valid (no self-intersection)
     */
    bool isValid() const { return isValid_; }
    
    /**
     * @brief Get maximum valid radius for this edge
     */
    float maxRadius() const { return maxRadius_; }

private:
    const Solid& solid_;
    uint32_t edgeIndex_;
    float radius_;
    
    // Cached data
    glm::vec3 edgeStart_;
    glm::vec3 edgeEnd_;
    glm::vec3 edgeDir_;
    float edgeLength_;
    
    glm::vec3 normal0_;     // Face 0 normal
    glm::vec3 normal1_;     // Face 1 normal
    
    glm::vec3 biNormal_;    // Normal to edge in bisector plane
    float halfAngle_;       // Half the dihedral angle
    
    bool isValid_ = true;
    float maxRadius_ = std::numeric_limits<float>::max();
    
    void computeGeometry();
};

} // namespace geometry
} // namespace dc3d
