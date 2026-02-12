/**
 * @file MeshAnalysis.h
 * @brief Comprehensive mesh analysis and statistics
 * 
 * Provides tools for analyzing mesh geometry including:
 * - Basic statistics (vertex/face/edge counts)
 * - Geometric properties (area, volume, centroid)
 * - Topology analysis (watertight, non-manifold, holes)
 * - Quality metrics (edge lengths, aspect ratios)
 * - Curvature estimation
 */

#pragma once

#include "MeshData.h"
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>

namespace dc3d {
namespace geometry {

/**
 * @brief Edge representation for topology analysis
 */
struct Edge {
    uint32_t v0;
    uint32_t v1;
    
    Edge(uint32_t a, uint32_t b) : v0(std::min(a, b)), v1(std::max(a, b)) {}
    
    bool operator==(const Edge& other) const {
        return v0 == other.v0 && v1 == other.v1;
    }
};

/**
 * @brief Hash function for Edge
 */
struct EdgeHash {
    size_t operator()(const Edge& e) const {
        return std::hash<uint64_t>()(static_cast<uint64_t>(e.v0) << 32 | e.v1);
    }
};

/**
 * @brief Information about a hole in the mesh
 */
struct HoleInfo {
    std::vector<uint32_t> boundaryVertices;  ///< Vertices forming the hole boundary
    float perimeter = 0.0f;                   ///< Length of the boundary
    glm::vec3 centroid{0.0f};                 ///< Center of the hole
    float estimatedArea = 0.0f;               ///< Estimated area of the hole
};

/**
 * @brief Aspect ratio distribution buckets
 */
struct AspectRatioDistribution {
    size_t excellent = 0;   ///< Ratio < 1.5 (near equilateral)
    size_t good = 0;        ///< Ratio 1.5 - 3.0
    size_t fair = 0;        ///< Ratio 3.0 - 6.0
    size_t poor = 0;        ///< Ratio 6.0 - 10.0
    size_t terrible = 0;    ///< Ratio > 10.0 (nearly degenerate)
};

/**
 * @brief Comprehensive mesh statistics
 */
struct MeshAnalysisStats {
    // Basic counts
    size_t vertexCount = 0;
    size_t faceCount = 0;
    size_t edgeCount = 0;
    
    // Bounding box and centroid
    BoundingBox bounds;
    glm::vec3 centroid{0.0f};
    
    // Surface and volume
    float surfaceArea = 0.0f;
    float volume = 0.0f;           ///< Only valid if closed
    bool volumeValid = false;      ///< True if mesh is closed
    
    // Edge length statistics
    float minEdgeLength = 0.0f;
    float maxEdgeLength = 0.0f;
    float avgEdgeLength = 0.0f;
    float stddevEdgeLength = 0.0f;
    
    // Face quality
    AspectRatioDistribution aspectRatios;
    float minFaceArea = 0.0f;
    float maxFaceArea = 0.0f;
    float avgFaceArea = 0.0f;
    
    // Topology
    size_t nonManifoldEdgeCount = 0;     ///< Edges shared by != 2 faces
    size_t nonManifoldVertexCount = 0;   ///< Vertices with non-manifold topology
    size_t boundaryEdgeCount = 0;        ///< Edges shared by only 1 face
    size_t holeCount = 0;
    std::vector<HoleInfo> holes;
    
    // Mesh type
    bool isWatertight = false;
    bool isManifold = false;
    bool hasConsistentWinding = false;
    
    // Data flags
    bool hasNormals = false;
    bool hasUVs = false;
    
    // Degenerate elements
    size_t degenerateFaceCount = 0;
    size_t isolatedVertexCount = 0;
};

/**
 * @brief Curvature types
 */
enum class CurvatureType {
    Mean,       ///< Mean curvature (H)
    Gaussian,   ///< Gaussian curvature (K)
    Principal1, ///< First principal curvature (k1)
    Principal2, ///< Second principal curvature (k2)
    Maximum,    ///< max(|k1|, |k2|)
    Minimum     ///< min(|k1|, |k2|)
};

/**
 * @brief Curvature statistics
 */
struct CurvatureStats {
    float minCurvature = 0.0f;
    float maxCurvature = 0.0f;
    float avgCurvature = 0.0f;
    float stddevCurvature = 0.0f;
};

/**
 * @brief Mesh analysis utilities
 * 
 * Provides comprehensive analysis of triangle meshes including
 * statistics, topology checking, and quality metrics.
 */
class MeshAnalysis {
public:
    /**
     * @brief Perform comprehensive mesh analysis
     * @param mesh Input mesh to analyze
     * @param progress Optional progress callback
     * @return Complete analysis statistics
     */
    static MeshAnalysisStats analyze(
        const MeshData& mesh,
        ProgressCallback progress = nullptr
    );
    
    /**
     * @brief Check if mesh is watertight (closed, manifold, consistent winding)
     * @param mesh Input mesh
     * @return true if mesh is watertight
     */
    static bool isWatertight(const MeshData& mesh);
    
    /**
     * @brief Check if mesh is manifold
     * @param mesh Input mesh
     * @return true if mesh has manifold topology
     */
    static bool isManifold(const MeshData& mesh);
    
    /**
     * @brief Compute per-vertex curvature
     * @param mesh Input mesh
     * @param type Type of curvature to compute
     * @return Per-vertex curvature values
     */
    static std::vector<float> computeCurvature(
        const MeshData& mesh,
        CurvatureType type = CurvatureType::Mean
    );
    
    /**
     * @brief Compute curvature statistics
     * @param curvatures Per-vertex curvature values
     * @return Statistics about the curvature
     */
    static CurvatureStats computeCurvatureStats(const std::vector<float>& curvatures);
    
    /**
     * @brief Find all boundary edges (edges with only one adjacent face)
     * @param mesh Input mesh
     * @return Vector of boundary edges
     */
    static std::vector<Edge> findBoundaryEdges(const MeshData& mesh);
    
    /**
     * @brief Find all non-manifold edges (edges with != 2 adjacent faces)
     * @param mesh Input mesh
     * @return Vector of non-manifold edges
     */
    static std::vector<Edge> findNonManifoldEdges(const MeshData& mesh);
    
    /**
     * @brief Find all holes in the mesh
     * @param mesh Input mesh
     * @return Vector of hole information
     */
    static std::vector<HoleInfo> findHoles(const MeshData& mesh);
    
    /**
     * @brief Compute triangle aspect ratio (longest edge / shortest altitude)
     * @param v0 First vertex
     * @param v1 Second vertex
     * @param v2 Third vertex
     * @return Aspect ratio (1.0 = equilateral)
     */
    static float computeTriangleAspectRatio(
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2
    );
    
    /**
     * @brief Get edge-to-face adjacency map
     * @param mesh Input mesh
     * @return Map from edge to list of adjacent face indices
     */
    static std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>
    buildEdgeFaceAdjacency(const MeshData& mesh);
    
    /**
     * @brief Get vertex-to-face adjacency
     * @param mesh Input mesh
     * @return Vector where [i] contains face indices adjacent to vertex i
     */
    static std::vector<std::vector<uint32_t>>
    buildVertexFaceAdjacency(const MeshData& mesh);
    
    /**
     * @brief Compute angle at a vertex within a triangle
     * @param v Vertex position
     * @param v1 Adjacent vertex 1
     * @param v2 Adjacent vertex 2
     * @return Angle in radians
     */
    static float computeVertexAngle(
        const glm::vec3& v,
        const glm::vec3& v1,
        const glm::vec3& v2
    );

private:
    /**
     * @brief Compute edge statistics
     */
    static void computeEdgeStatistics(
        const MeshData& mesh,
        const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap,
        MeshAnalysisStats& stats
    );
    
    /**
     * @brief Compute face quality statistics
     */
    static void computeFaceStatistics(const MeshData& mesh, MeshAnalysisStats& stats);
    
    /**
     * @brief Compute topology statistics
     */
    static void computeTopologyStatistics(
        const MeshData& mesh,
        const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap,
        MeshAnalysisStats& stats
    );
    
    /**
     * @brief Check for consistent face winding
     */
    static bool checkConsistentWinding(
        const MeshData& mesh,
        const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap
    );
    
    /**
     * @brief Trace a hole boundary starting from a boundary edge
     */
    static HoleInfo traceHole(
        const MeshData& mesh,
        const Edge& startEdge,
        std::unordered_set<Edge, EdgeHash>& visitedEdges,
        const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap
    );
};

} // namespace geometry
} // namespace dc3d
