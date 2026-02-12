/**
 * @file MeshRepair.h
 * @brief Mesh repair tools for fixing common mesh problems
 * 
 * Provides functions to detect and fix:
 * - Floating/outlier triangles
 * - Holes (boundary loops)
 * - Duplicate vertices
 * - Degenerate faces
 * - Non-manifold geometry
 */

#pragma once

#include "MeshData.h"
#include "HalfEdgeMesh.h"

#include <glm/glm.hpp>

#include <vector>
#include <unordered_set>
#include <functional>

namespace dc3d {
namespace geometry {

/**
 * @brief Options for hole filling
 */
struct HoleFillOptions {
    size_t maxEdges = 100;          ///< Maximum boundary loop size to fill
    bool triangulate = true;        ///< Triangulate filled holes
    bool smooth = false;            ///< Smooth filled region
    int smoothIterations = 3;       ///< Smoothing iterations for filled region
    bool fairFill = false;          ///< Use fairing to create smooth fill
};

/**
 * @brief Information about a detected hole
 */
struct HoleInfo {
    std::vector<uint32_t> boundaryVertices;  ///< Vertices forming the boundary loop
    float perimeter = 0.0f;                   ///< Total length of boundary
    float estimatedArea = 0.0f;               ///< Estimated area if filled
    glm::vec3 centroid{0.0f};                 ///< Center of the hole
    glm::vec3 normal{0.0f, 1.0f, 0.0f};       ///< Estimated normal direction
};

/**
 * @brief Result from mesh repair operations
 */
struct RepairResult {
    size_t itemsFixed = 0;
    size_t itemsRemoved = 0;
    size_t verticesAdded = 0;
    size_t facesAdded = 0;
    std::string message;
    bool success = true;
};

/**
 * @brief Mesh repair utilities
 * 
 * Usage:
 * @code
 *     // Remove outliers
 *     auto result = MeshRepair::removeOutliers(mesh, 0.01f);
 *     
 *     // Fill holes
 *     auto holes = MeshRepair::detectHoles(mesh);
 *     for (const auto& hole : holes) {
 *         if (hole.boundaryVertices.size() <= 50) {
 *             MeshRepair::fillHole(mesh, hole);
 *         }
 *     }
 *     
 *     // Fix non-manifold
 *     MeshRepair::makeManifold(mesh);
 * @endcode
 */
class MeshRepair {
public:
    // =========================================================================
    // Outlier Detection and Removal
    // =========================================================================
    
    /**
     * @brief Remove isolated/floating triangles (outliers)
     * 
     * Removes triangles that are disconnected from the main mesh body
     * or have abnormally large edge lengths.
     * 
     * @param mesh Mesh to repair (modified in place)
     * @param threshold Distance threshold as ratio of mesh diagonal
     * @param progress Optional progress callback
     * @return Repair result with statistics
     */
    static RepairResult removeOutliers(
        MeshData& mesh, 
        float threshold = 0.01f,
        ProgressCallback progress = nullptr);
    
    /**
     * @brief Find connected components
     * @param mesh Input mesh
     * @return Vector of components, each containing face indices
     */
    static std::vector<std::vector<uint32_t>> findConnectedComponents(
        const MeshData& mesh);
    
    /**
     * @brief Keep only the largest connected component
     * @param mesh Mesh to modify
     * @return Number of faces removed
     */
    static size_t keepLargestComponent(MeshData& mesh);
    
    // =========================================================================
    // Hole Detection and Filling
    // =========================================================================
    
    /**
     * @brief Detect all holes in the mesh
     * @param mesh Input mesh
     * @return List of detected holes
     */
    static std::vector<HoleInfo> detectHoles(const MeshData& mesh);
    
    /**
     * @brief Fill a specific hole
     * @param mesh Mesh to modify
     * @param hole Hole information
     * @param options Fill options
     * @return Repair result
     */
    static RepairResult fillHole(
        MeshData& mesh, 
        const HoleInfo& hole,
        const HoleFillOptions& options = HoleFillOptions{});
    
    /**
     * @brief Fill all holes up to maxEdges
     * @param mesh Mesh to modify
     * @param maxEdges Maximum hole size to fill
     * @param progress Optional progress callback
     * @return Repair result
     */
    static RepairResult fillHoles(
        MeshData& mesh, 
        size_t maxEdges = 100,
        ProgressCallback progress = nullptr);
    
    // =========================================================================
    // Duplicate Vertex Handling
    // =========================================================================
    
    /**
     * @brief Remove duplicate vertices within tolerance
     * 
     * Merges vertices that are within the specified distance.
     * Updates face indices accordingly.
     * 
     * @param mesh Mesh to modify
     * @param tolerance Distance tolerance for merging
     * @param progress Optional progress callback
     * @return Number of vertices removed
     */
    static size_t removeDuplicateVertices(
        MeshData& mesh, 
        float tolerance = 1e-6f,
        ProgressCallback progress = nullptr);
    
    // =========================================================================
    // Degenerate Face Handling
    // =========================================================================
    
    /**
     * @brief Remove degenerate faces (zero area or duplicate vertices)
     * @param mesh Mesh to modify
     * @return Number of faces removed
     */
    static size_t removeDegenerateFaces(MeshData& mesh);
    
    /**
     * @brief Detect degenerate faces
     * @param mesh Input mesh
     * @param areaThreshold Minimum area threshold
     * @return List of degenerate face indices
     */
    static std::vector<uint32_t> detectDegenerateFaces(
        const MeshData& mesh, 
        float areaThreshold = 1e-10f);
    
    // =========================================================================
    // Manifold Repair
    // =========================================================================
    
    /**
     * @brief Make mesh manifold by fixing non-manifold edges and vertices
     * 
     * Fixes:
     * - Edges shared by more than 2 faces (splits them)
     * - Vertices with non-disk neighborhoods (duplicates them)
     * 
     * @param mesh Mesh to modify
     * @param progress Optional progress callback
     * @return Repair result
     */
    static RepairResult makeManifold(
        MeshData& mesh,
        ProgressCallback progress = nullptr);
    
    /**
     * @brief Detect non-manifold edges
     * @param mesh Input mesh
     * @return List of vertex pairs forming non-manifold edges
     */
    static std::vector<std::pair<uint32_t, uint32_t>> detectNonManifoldEdges(
        const MeshData& mesh);
    
    /**
     * @brief Detect non-manifold vertices
     * @param mesh Input mesh
     * @return List of non-manifold vertex indices
     */
    static std::vector<uint32_t> detectNonManifoldVertices(
        const MeshData& mesh);
    
    /**
     * @brief Check if mesh is manifold
     * @param mesh Input mesh
     * @return true if mesh is manifold
     */
    static bool isManifold(const MeshData& mesh);
    
    // =========================================================================
    // Orientation and Consistency
    // =========================================================================
    
    /**
     * @brief Make face orientations consistent
     * 
     * Ensures all faces have consistent winding order (all CCW or all CW).
     * 
     * @param mesh Mesh to modify
     * @return Number of faces flipped
     */
    static size_t makeOrientationConsistent(MeshData& mesh);
    
    /**
     * @brief Orient all faces outward (for closed meshes)
     * @param mesh Mesh to modify
     * @return true if successful
     */
    static bool orientOutward(MeshData& mesh);
    
    // =========================================================================
    // Comprehensive Repair
    // =========================================================================
    
    /**
     * @brief Perform comprehensive mesh repair
     * 
     * Applies multiple repair operations in optimal order:
     * 1. Remove duplicate vertices
     * 2. Remove degenerate faces
     * 3. Remove outliers
     * 4. Make manifold
     * 5. Make orientation consistent
     * 6. Fill small holes (optional)
     * 
     * @param mesh Mesh to repair
     * @param fillSmallHoles Whether to fill holes with <= 20 edges
     * @param progress Optional progress callback
     * @return Combined repair result
     */
    static RepairResult repairAll(
        MeshData& mesh,
        bool fillSmallHoles = true,
        ProgressCallback progress = nullptr);
    
private:
    MeshRepair() = default;
    
    // Helper functions
    static void triangulateHoleSimple(
        MeshData& mesh, 
        const std::vector<uint32_t>& boundary);
    
    static void triangulateHoleEarClipping(
        MeshData& mesh, 
        const std::vector<uint32_t>& boundary,
        const glm::vec3& normal);
    
    static void triangulateHoleMinArea(
        MeshData& mesh,
        const std::vector<uint32_t>& boundary,
        const glm::vec3& centroid);
};

/**
 * @brief Statistics about mesh health
 */
struct MeshHealthReport {
    bool isValid = false;
    bool isManifold = false;
    bool isClosed = false;
    bool isOrientable = false;
    
    size_t duplicateVertices = 0;
    size_t degenerateFaces = 0;
    size_t nonManifoldEdges = 0;
    size_t nonManifoldVertices = 0;
    size_t boundaryEdges = 0;
    size_t holeCount = 0;
    size_t componentCount = 0;
    
    std::string summary() const;
};

/**
 * @brief Analyze mesh health
 * @param mesh Input mesh
 * @param progress Optional progress callback
 * @return Health report
 */
MeshHealthReport analyzeMeshHealth(
    const MeshData& mesh,
    ProgressCallback progress = nullptr);

} // namespace geometry
} // namespace dc3d
