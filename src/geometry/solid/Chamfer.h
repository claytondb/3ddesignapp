/**
 * @file Chamfer.h
 * @brief Chamfer (beveling) operations for solid edges
 * 
 * Implements edge beveling with symmetric, asymmetric, and angle-based
 * chamfer options. Creates flat cut surfaces at solid edges.
 */

#pragma once

#include "Solid.h"

#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

/**
 * @brief Type of chamfer specification
 */
enum class ChamferType {
    Symmetric,      ///< Equal distance on both faces
    Asymmetric,     ///< Different distances on each face
    AngleDistance,  ///< Angle + distance specification
    TwoDistances    ///< Two distances from edge
};

/**
 * @brief Defines chamfer at a specific point along an edge
 */
struct ChamferPoint {
    float parameter = 0.0f;     ///< Position along edge (0-1)
    float distance1 = 1.0f;     ///< Distance on first face
    float distance2 = 1.0f;     ///< Distance on second face
};

/**
 * @brief Options for chamfer operations
 */
struct ChamferOptions {
    /// Chamfer type
    ChamferType type = ChamferType::Symmetric;
    
    /// Primary distance (used differently based on type)
    float distance = 1.0f;
    
    /// Secondary distance (for asymmetric/two-distance types)
    float distance2 = 1.0f;
    
    /// Angle in radians (for angle-distance type)
    float angle = glm::quarter_pi<float>();  // 45 degrees
    
    /// Whether to allow variable chamfer along edge
    bool variableChamfer = false;
    
    /// Variable chamfer control points
    std::vector<ChamferPoint> chamferPoints;
    
    /// Whether to propagate to tangent-connected edges
    bool tangentPropagation = true;
    
    /// Angle threshold for tangent propagation (radians)
    float tangentAngleThreshold = 0.087266f;  // ~5 degrees
    
    /// Tolerance for geometric calculations
    float tolerance = 1e-6f;
    
    /// Whether to handle corners where multiple edges meet
    bool handleCorners = true;
    
    /// Progress callback
    ProgressCallback progress = nullptr;
};

/**
 * @brief Result of a chamfer operation
 */
struct ChamferResult {
    bool success = false;
    std::string error;
    
    /// Resulting solid with chamfers applied
    std::optional<Solid> solid;
    
    /// Indices of newly created chamfer faces
    std::vector<uint32_t> chamferFaces;
    
    /// Indices of modified original faces
    std::vector<uint32_t> modifiedFaces;
    
    /// Statistics
    struct Stats {
        int edgesProcessed = 0;
        int chamferFacesCreated = 0;
        int cornersProcessed = 0;
        float computeTimeMs = 0.0f;
    } stats;
    
    bool ok() const { return success; }
    explicit operator bool() const { return ok(); }
};

/**
 * @brief Represents a chamfer surface between two faces
 */
struct ChamferSurface {
    uint32_t edgeIndex;             ///< Source edge being chamfered
    uint32_t face0Index;            ///< First adjacent face
    uint32_t face1Index;            ///< Second adjacent face
    
    /// Chamfer boundary vertices
    std::vector<glm::vec3> boundary0;   ///< Boundary on face 0
    std::vector<glm::vec3> boundary1;   ///< Boundary on face 1
    
    /// Generate mesh faces from chamfer surface
    std::vector<SolidFace> generateFaces(std::vector<SolidVertex>& vertices) const;
};

/**
 * @brief Chamfer operations for solid bodies
 * 
 * Creates beveled (flat) edges by:
 * 1. Computing offset curves on adjacent faces
 * 2. Creating flat chamfer surface between curves
 * 3. Trimming original faces
 * 4. Stitching chamfer surfaces to trimmed faces
 */
class Chamfer {
public:
    Chamfer() = default;
    ~Chamfer() = default;
    
    // ===================
    // Edge Chamfers
    // ===================
    
    /**
     * @brief Apply symmetric chamfer to specified edges
     * @param solid Input solid
     * @param edgeIndices Edges to chamfer
     * @param distance Chamfer distance (equal on both faces)
     * @param options Additional options
     * @return Result containing chamfered solid
     */
    static ChamferResult chamferEdges(const Solid& solid,
                                      const std::vector<uint32_t>& edgeIndices,
                                      float distance,
                                      const ChamferOptions& options = {});
    
    /**
     * @brief Apply asymmetric chamfer to specified edges
     * @param solid Input solid
     * @param edgeIndices Edges to chamfer
     * @param distance1 Distance on first face
     * @param distance2 Distance on second face
     * @param options Additional options
     * @return Result containing chamfered solid
     */
    static ChamferResult chamferEdgesAsymmetric(const Solid& solid,
                                                const std::vector<uint32_t>& edgeIndices,
                                                float distance1, float distance2,
                                                const ChamferOptions& options = {});
    
    /**
     * @brief Apply angle-based chamfer to specified edges
     * @param solid Input solid
     * @param edgeIndices Edges to chamfer
     * @param distance Distance on reference face
     * @param angle Angle from reference face (radians)
     * @param options Additional options
     * @return Result containing chamfered solid
     */
    static ChamferResult chamferEdgesAngle(const Solid& solid,
                                           const std::vector<uint32_t>& edgeIndices,
                                           float distance, float angle,
                                           const ChamferOptions& options = {});
    
    /**
     * @brief Apply chamfer to edges with individual parameters
     * @param solid Input solid
     * @param edgeParams Map of edge index to (distance1, distance2) pairs
     * @param options Additional options
     * @return Result containing chamfered solid
     */
    static ChamferResult chamferEdgesWithParams(
        const Solid& solid,
        const std::unordered_map<uint32_t, std::pair<float, float>>& edgeParams,
        const ChamferOptions& options = {});
    
    /**
     * @brief Apply variable chamfer along an edge
     * @param solid Input solid
     * @param edgeIndex Edge to chamfer
     * @param chamferPoints Control points for variable chamfer
     * @param options Additional options
     * @return Result containing chamfered solid
     */
    static ChamferResult chamferEdgeVariable(
        const Solid& solid,
        uint32_t edgeIndex,
        const std::vector<ChamferPoint>& chamferPoints,
        const ChamferOptions& options = {});
    
    // ===================
    // Face Chamfers
    // ===================
    
    /**
     * @brief Apply chamfer between two faces
     * @param solid Input solid
     * @param face0Index First face
     * @param face1Index Second face (must share an edge)
     * @param options Chamfer options
     * @return Result containing chamfered solid
     */
    static ChamferResult chamferFaces(const Solid& solid,
                                      uint32_t face0Index, uint32_t face1Index,
                                      const ChamferOptions& options = {});
    
    /**
     * @brief Apply chamfer to all edges of a face
     * @param solid Input solid
     * @param faceIndex Face whose edges to chamfer
     * @param options Chamfer options
     * @return Result containing chamfered solid
     */
    static ChamferResult chamferFaceEdges(const Solid& solid,
                                          uint32_t faceIndex,
                                          const ChamferOptions& options = {});
    
    // ===================
    // Selection Helpers
    // ===================
    
    /**
     * @brief Find all edges that can be chamfered with given distance
     * @param solid Input solid
     * @param distance Chamfer distance
     * @return Vector of chamferable edge indices
     */
    static std::vector<uint32_t> findChamferableEdges(const Solid& solid, float distance);
    
    /**
     * @brief Calculate maximum chamfer distance for an edge
     * @param solid Input solid
     * @param edgeIndex Edge to check
     * @return Maximum distance that won't cause self-intersection
     */
    static float maxChamferDistance(const Solid& solid, uint32_t edgeIndex);
    
    /**
     * @brief Convert angle-based chamfer to two distances
     * @param distance Distance on reference face
     * @param angle Angle from reference face (radians)
     * @param dihedralAngle Dihedral angle between faces (radians)
     * @return Pair of (distance1, distance2)
     */
    static std::pair<float, float> angleToDistances(float distance, float angle,
                                                    float dihedralAngle);
    
    // ===================
    // Preview
    // ===================
    
    /**
     * @brief Generate preview geometry for chamfer
     * @param solid Input solid
     * @param edgeIndices Edges to preview
     * @param distance Chamfer distance
     * @return Preview mesh (chamfer surfaces only)
     */
    static MeshData generatePreview(const Solid& solid,
                                   const std::vector<uint32_t>& edgeIndices,
                                   float distance);
    
    /**
     * @brief Generate preview for asymmetric chamfer
     */
    static MeshData generatePreviewAsymmetric(const Solid& solid,
                                             const std::vector<uint32_t>& edgeIndices,
                                             float distance1, float distance2);

private:
    // Internal computation methods
    
    /**
     * @brief Compute chamfer surface for an edge
     */
    static ChamferSurface computeChamferSurface(const Solid& solid,
                                                uint32_t edgeIndex,
                                                float distance1, float distance2,
                                                const ChamferOptions& options);
    
    /**
     * @brief Compute variable chamfer surface
     */
    static ChamferSurface computeVariableChamferSurface(
        const Solid& solid,
        uint32_t edgeIndex,
        const std::vector<ChamferPoint>& chamferPoints,
        const ChamferOptions& options);
    
    /**
     * @brief Compute offset point on a face along edge normal
     */
    static glm::vec3 computeOffsetPoint(const glm::vec3& edgePoint,
                                        const glm::vec3& edgeDir,
                                        const glm::vec3& faceNormal,
                                        float distance);
    
    /**
     * @brief Compute corner chamfer where multiple edges meet
     */
    static std::vector<SolidFace> computeCornerChamfer(
        const Solid& solid,
        uint32_t vertexIndex,
        const std::vector<uint32_t>& chamferEdges,
        const std::unordered_map<uint32_t, ChamferSurface>& chamferSurfaces,
        std::vector<SolidVertex>& vertices,
        const ChamferOptions& options);
    
    /**
     * @brief Validate chamfer distances for an edge
     */
    static bool isValidChamferDistance(const Solid& solid,
                                       uint32_t edgeIndex,
                                       float distance1, float distance2);
    
    /**
     * @brief Interpolate chamfer distances along edge
     */
    static std::pair<float, float> interpolateChamfer(float t,
                                                      const std::vector<ChamferPoint>& points);
};

} // namespace geometry
} // namespace dc3d
