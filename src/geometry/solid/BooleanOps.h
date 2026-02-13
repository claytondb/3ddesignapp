/**
 * @file BooleanOps.h
 * @brief Boolean operations on solid bodies (Union, Subtract, Intersect)
 * 
 * Implements CSG (Constructive Solid Geometry) boolean operations using
 * a mesh-based approach with BSP trees for robust intersection handling.
 */

#pragma once

#include "Solid.h"
#include "../MeshData.h"
#include "../BVH.h"

#include <vector>
#include <memory>
#include <functional>
#include <optional>

#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

/**
 * @brief Type of boolean operation
 */
enum class BooleanOperation {
    Union,      ///< A ∪ B - Combine both solids
    Subtract,   ///< A - B - Cut B from A
    Intersect   ///< A ∩ B - Keep only common volume
};

/**
 * @brief Options for boolean operations
 */
struct BooleanOptions {
    /// Tolerance for geometric comparisons
    float tolerance = 1e-6f;
    
    /// Epsilon for coplanar face detection
    float coplanarEpsilon = 1e-5f;
    
    /// Whether to keep original solids intact
    bool keepOriginals = true;
    
    /// Whether to merge coplanar faces in result
    bool mergeCoplanarFaces = true;
    
    /// Whether to triangulate result
    bool triangulate = true;
    
    /// Maximum iterations for intersection refinement
    int maxIterations = 100;
    
    /// Progress callback
    ProgressCallback progress = nullptr;
};

/**
 * @brief Result of a boolean operation
 */
struct BooleanResult {
    bool success = false;
    std::string error;
    
    /// Result solid (if successful)
    std::optional<Solid> solid;
    
    /// Statistics
    struct Stats {
        int intersectionCount = 0;      ///< Number of face-face intersections
        int newVertexCount = 0;         ///< Vertices created at intersections
        int newFaceCount = 0;           ///< New faces created
        int removedFaceCount = 0;       ///< Faces removed
        float computeTimeMs = 0.0f;     ///< Computation time in milliseconds
    } stats;
    
    bool ok() const { return success; }
    explicit operator bool() const { return ok(); }
};

/**
 * @brief BSP tree node for boolean operations
 * 
 * Each node represents a partitioning plane and contains polygons
 * that lie on that plane.
 */
class BSPNode {
public:
    BSPNode() = default;
    
    /// Create a BSP tree from a list of polygons (faces)
    static std::unique_ptr<BSPNode> build(std::vector<SolidFace>& faces,
                                          std::vector<SolidVertex>& vertices,
                                          float epsilon = 1e-5f);
    
    /// Classify a point relative to this node's plane
    enum class Classification { Front, Back, Coplanar, Spanning };
    Classification classifyPoint(const glm::vec3& point) const;
    
    /// Classify a polygon relative to this node's plane
    Classification classifyFace(const SolidFace& face,
                               const std::vector<SolidVertex>& vertices) const;
    
    /// Clip polygon by this BSP tree
    void clipPolygons(std::vector<SolidFace>& faces,
                     std::vector<SolidVertex>& vertices) const;
    
    /// Clip coplanar polygons (with vertices for split operations)
    void clipTo(BSPNode* other, std::vector<SolidVertex>& vertices);
    
    /// Clip coplanar polygons (backward compatibility overload)
    void clipTo(BSPNode* other);
    
    /// Get all polygons from this tree
    void allPolygons(std::vector<SolidFace>& result) const;
    
    /// Invert the tree (swap inside/outside)
    void invert();
    
    /// Get plane normal
    const glm::vec3& normal() const { return plane_.normal; }
    
    /// Get plane distance
    float distance() const { return plane_.distance; }

private:
    struct Plane {
        glm::vec3 normal{0, 0, 1};
        float distance = 0;
        
        Plane() = default;
        Plane(const glm::vec3& n, float d) : normal(n), distance(d) {}
        
        static Plane fromPoints(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
    };
    
    Plane plane_;
    std::vector<SolidFace> coplanarFaces_;
    std::unique_ptr<BSPNode> front_;
    std::unique_ptr<BSPNode> back_;
    float epsilon_ = 1e-5f;
    
    void splitFace(const SolidFace& face, const std::vector<SolidVertex>& vertices,
                   std::vector<SolidFace>& coplanarFront,
                   std::vector<SolidFace>& coplanarBack,
                   std::vector<SolidFace>& front,
                   std::vector<SolidFace>& back,
                   std::vector<SolidVertex>& newVertices) const;
};

/**
 * @brief Boolean operations on solid bodies
 * 
 * Implements CSG operations using BSP trees for robust handling of
 * edge cases like coplanar faces and near-tangent intersections.
 * 
 * Algorithm:
 * 1. Build BSP trees for both input solids
 * 2. Clip polygons against each other based on operation type
 * 3. Merge clipped polygons into result solid
 * 4. Clean up degenerate geometry
 * 
 * For robust operations on complex models, exact arithmetic or
 * snap rounding may be needed.
 */
class BooleanOps {
public:
    BooleanOps() = default;
    ~BooleanOps() = default;
    
    // ===================
    // Main Operations
    // ===================
    
    /**
     * @brief Compute boolean union of two solids
     * @param solidA First operand
     * @param solidB Second operand
     * @param options Operation options
     * @return Result containing merged solid
     * 
     * Union combines both solids, removing internal geometry.
     */
    static BooleanResult booleanUnion(const Solid& solidA, const Solid& solidB,
                                      const BooleanOptions& options = {});
    
    /**
     * @brief Compute boolean subtraction
     * @param solidA Solid to cut from
     * @param solidB Solid to cut with (tool)
     * @param options Operation options
     * @return Result containing A - B
     * 
     * Subtract removes the volume of B from A.
     */
    static BooleanResult booleanSubtract(const Solid& solidA, const Solid& solidB,
                                         const BooleanOptions& options = {});
    
    /**
     * @brief Compute boolean intersection
     * @param solidA First operand
     * @param solidB Second operand
     * @param options Operation options
     * @return Result containing common volume
     * 
     * Intersection keeps only geometry inside both solids.
     */
    static BooleanResult booleanIntersect(const Solid& solidA, const Solid& solidB,
                                          const BooleanOptions& options = {});
    
    /**
     * @brief Generic boolean operation
     * @param op Operation type
     * @param solidA First operand
     * @param solidB Second operand
     * @param options Operation options
     * @return Result of the operation
     */
    static BooleanResult compute(BooleanOperation op,
                                 const Solid& solidA, const Solid& solidB,
                                 const BooleanOptions& options = {});
    
    // ===================
    // Batch Operations
    // ===================
    
    /**
     * @brief Union multiple solids
     * @param solids Vector of solids to combine
     * @param options Operation options
     * @return Result containing combined solid
     */
    static BooleanResult booleanUnionMultiple(const std::vector<Solid>& solids,
                                              const BooleanOptions& options = {});
    
    /**
     * @brief Subtract multiple solids from a base
     * @param base Base solid
     * @param tools Vector of solids to subtract
     * @param options Operation options
     * @return Result containing base - tools
     */
    static BooleanResult booleanSubtractMultiple(const Solid& base,
                                                  const std::vector<Solid>& tools,
                                                  const BooleanOptions& options = {});
    
    // ===================
    // CSG Tree Evaluation
    // ===================
    
    /**
     * @brief Evaluate a CSG tree
     * @param root Root node of CSG tree
     * @param options Operation options
     * @return Result solid
     */
    static BooleanResult evaluateCSGTree(const std::shared_ptr<CSGNode>& root,
                                         const BooleanOptions& options = {});
    
    // ===================
    // Utility Functions
    // ===================
    
    /**
     * @brief Check if two solids overlap
     * @param solidA First solid
     * @param solidB Second solid
     * @return true if bounding boxes overlap
     */
    static bool boundingBoxesOverlap(const Solid& solidA, const Solid& solidB);
    
    /**
     * @brief Find intersection curves between two solids
     * @param solidA First solid
     * @param solidB Second solid
     * @param tolerance Intersection tolerance
     * @return Vector of polyline curves representing intersections
     */
    static std::vector<std::vector<glm::vec3>> findIntersectionCurves(
        const Solid& solidA, const Solid& solidB, float tolerance = 1e-6f);
    
    /**
     * @brief Classify points as inside/outside a solid
     * @param solid The solid body
     * @param points Points to classify
     * @return Vector of bools (true = inside)
     */
    static std::vector<bool> classifyPoints(const Solid& solid,
                                           const std::vector<glm::vec3>& points);
    
    /**
     * @brief Check if a point is inside a solid
     * @param solid The solid body
     * @param point Point to check
     * @return true if point is inside the solid
     */
    static bool isPointInside(const Solid& solid, const glm::vec3& point);

private:
    // Internal helper methods
    static BooleanResult performBSPBoolean(const Solid& solidA, const Solid& solidB,
                                           BooleanOperation op,
                                           const BooleanOptions& options);
    
    static std::unique_ptr<BSPNode> solidToBSP(const Solid& solid);
    
    static Solid bspToSolid(BSPNode* bsp, const std::vector<SolidVertex>& vertices);
    
    static void mergeVertices(std::vector<SolidVertex>& vertices,
                             std::vector<SolidFace>& faces,
                             float tolerance);
    
    static void cleanupDegenerateFaces(std::vector<SolidFace>& faces,
                                       const std::vector<SolidVertex>& vertices,
                                       float tolerance);
    
    static bool trianglesIntersect(const glm::vec3& a0, const glm::vec3& a1, const glm::vec3& a2,
                                   const glm::vec3& b0, const glm::vec3& b1, const glm::vec3& b2,
                                   std::vector<glm::vec3>& intersectionPoints);
    
    static std::optional<glm::vec3> rayTriangleIntersect(const glm::vec3& origin,
                                                         const glm::vec3& dir,
                                                         const glm::vec3& v0,
                                                         const glm::vec3& v1,
                                                         const glm::vec3& v2);
};

/**
 * @brief Exact arithmetic utilities for robust booleans
 * 
 * Uses techniques like adaptive precision floating point or
 * interval arithmetic for robust geometric predicates.
 */
class ExactPredicates {
public:
    /**
     * @brief Exact orientation test for 4 points
     * @return positive if d is above plane(a,b,c), negative if below, 0 if on
     */
    static double orient3d(const glm::dvec3& a, const glm::dvec3& b,
                          const glm::dvec3& c, const glm::dvec3& d);
    
    /**
     * @brief Exact orientation test for 3 points (2D)
     * @return positive if c is left of line(a,b), negative if right, 0 if on
     */
    static double orient2d(const glm::dvec2& a, const glm::dvec2& b,
                          const glm::dvec2& c);
    
    /**
     * @brief Test if point is in circumsphere of tetrahedron
     * @return positive if e is inside circumsphere(a,b,c,d)
     */
    static double inSphere(const glm::dvec3& a, const glm::dvec3& b,
                          const glm::dvec3& c, const glm::dvec3& d,
                          const glm::dvec3& e);
};

} // namespace geometry
} // namespace dc3d
