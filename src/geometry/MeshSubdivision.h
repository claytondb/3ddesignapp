/**
 * @file MeshSubdivision.h
 * @brief Mesh subdivision algorithms: Loop and Catmull-Clark
 * 
 * Provides smooth surface subdivision for triangle and quad meshes.
 */

#pragma once

#include "MeshData.h"
#include "HalfEdgeMesh.h"

#include <glm/glm.hpp>

#include <vector>
#include <unordered_set>

namespace dc3d {
namespace geometry {

/**
 * @brief Subdivision algorithm type
 */
enum class SubdivisionAlgorithm {
    Loop,           ///< Loop subdivision (for triangle meshes)
    CatmullClark,   ///< Catmull-Clark subdivision (for quad/polygon meshes)
    Butterfly,      ///< Butterfly subdivision (interpolating)
    MidPoint        ///< Simple midpoint subdivision (linear)
};

/**
 * @brief Options for subdivision
 */
struct SubdivisionOptions {
    SubdivisionAlgorithm algorithm = SubdivisionAlgorithm::Loop;
    int iterations = 1;             ///< Number of subdivision iterations
    bool preserveBoundary = true;   ///< Special handling for boundary edges
    bool smoothBoundary = true;     ///< Apply smoothing rules to boundary
    
    // For selective subdivision
    std::unordered_set<uint32_t> lockedVertices;  ///< Vertices that shouldn't move
    std::unordered_set<uint32_t> sharpEdges;      ///< Edge vertex pairs to keep sharp
    
    float sharpnessWeight = 1.0f;   ///< Weight for sharp edge handling (0-1)
};

/**
 * @brief Result information from subdivision
 */
struct SubdivisionResult {
    size_t originalVertices = 0;
    size_t originalFaces = 0;
    size_t finalVertices = 0;
    size_t finalFaces = 0;
    int iterationsPerformed = 0;
    bool cancelled = false;
};

/**
 * @brief Mesh subdivision algorithms
 * 
 * Usage:
 * @code
 *     SubdivisionOptions opts;
 *     opts.algorithm = SubdivisionAlgorithm::Loop;
 *     opts.iterations = 2;
 *     
 *     auto result = MeshSubdivider::subdivide(mesh, opts, [](float p) {
 *         return true;  // Continue
 *     });
 *     
 *     // Simple interface
 *     auto smoothMesh = MeshSubdivider::subdivide(mesh, SubdivisionAlgorithm::Loop, 2);
 * @endcode
 */
class MeshSubdivider {
public:
    /**
     * @brief Subdivide a mesh
     * @param mesh Input mesh
     * @param options Subdivision options
     * @param progress Optional progress callback
     * @return Result with subdivided mesh and statistics
     */
    static Result<std::pair<MeshData, SubdivisionResult>> subdivide(
        const MeshData& mesh,
        const SubdivisionOptions& options,
        ProgressCallback progress = nullptr);
    
    /**
     * @brief Subdivide a mesh (simple interface)
     * @param mesh Input mesh
     * @param algorithm Subdivision algorithm
     * @param iterations Number of iterations
     * @return Subdivided mesh
     */
    static Result<MeshData> subdivide(
        const MeshData& mesh,
        SubdivisionAlgorithm algorithm,
        int iterations);
    
    /**
     * @brief Loop subdivision (single iteration)
     * @param mesh Input mesh (triangles only)
     * @param preserveBoundary Handle boundaries specially
     * @return Subdivided mesh
     */
    static Result<MeshData> loopSubdivide(
        const MeshData& mesh,
        bool preserveBoundary = true);
    
    /**
     * @brief Catmull-Clark subdivision (single iteration)
     * @param mesh Input mesh (any polygon type)
     * @param preserveBoundary Handle boundaries specially
     * @return Subdivided mesh
     */
    static Result<MeshData> catmullClarkSubdivide(
        const MeshData& mesh,
        bool preserveBoundary = true);
    
    /**
     * @brief Butterfly subdivision (single iteration)
     * @param mesh Input mesh (triangles only)
     * @param preserveBoundary Handle boundaries specially
     * @return Subdivided mesh
     */
    static Result<MeshData> butterflySubdivide(
        const MeshData& mesh,
        bool preserveBoundary = true);
    
    /**
     * @brief Simple midpoint subdivision (single iteration)
     * @param mesh Input mesh
     * @return Subdivided mesh (vertices not smoothed)
     */
    static Result<MeshData> midpointSubdivide(const MeshData& mesh);
    
private:
    MeshSubdivider() = default;
};

/**
 * @brief Internal subdivision state for Loop subdivision
 */
class LoopSubdivisionState {
public:
    LoopSubdivisionState(const HalfEdgeMesh& mesh, bool preserveBoundary);
    
    /// Execute one iteration of Loop subdivision
    MeshData execute();
    
private:
    const HalfEdgeMesh& mesh_;
    bool preserveBoundary_;
    
    std::unordered_set<uint32_t> boundaryVertices_;
    
    /// Compute new position for existing vertex
    glm::vec3 computeVertexPoint(uint32_t vertexIdx) const;
    
    /// Compute new position for edge midpoint
    glm::vec3 computeEdgePoint(uint32_t heIdx) const;
    
    /// Beta coefficient for vertex with valence n
    static float betaCoefficient(size_t valence);
};

/**
 * @brief Internal subdivision state for Catmull-Clark subdivision
 */
class CatmullClarkState {
public:
    CatmullClarkState(const HalfEdgeMesh& mesh, bool preserveBoundary);
    
    /// Execute one iteration
    MeshData execute();
    
private:
    const HalfEdgeMesh& mesh_;
    bool preserveBoundary_;
    
    std::unordered_set<uint32_t> boundaryVertices_;
    
    /// Compute face point (centroid)
    glm::vec3 computeFacePoint(uint32_t faceIdx) const;
    
    /// Compute edge point
    glm::vec3 computeEdgePoint(uint32_t heIdx, 
                               const std::vector<glm::vec3>& facePoints) const;
    
    /// Compute new vertex position
    glm::vec3 computeVertexPoint(uint32_t vertexIdx,
                                  const std::vector<glm::vec3>& facePoints,
                                  const std::vector<glm::vec3>& edgePoints) const;
};

/**
 * @brief Butterfly subdivision state
 */
class ButterflySubdivisionState {
public:
    ButterflySubdivisionState(const HalfEdgeMesh& mesh, bool preserveBoundary);
    
    /// Execute one iteration
    MeshData execute();
    
private:
    const HalfEdgeMesh& mesh_;
    bool preserveBoundary_;
    
    std::unordered_set<uint32_t> boundaryVertices_;
    
    /// Compute edge point using 8-point stencil
    glm::vec3 computeEdgePoint(uint32_t heIdx) const;
    
    /// Compute edge point for boundary edge
    glm::vec3 computeBoundaryEdgePoint(uint32_t heIdx) const;
};

} // namespace geometry
} // namespace dc3d
