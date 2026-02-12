/**
 * @file MeshSmoothing.h
 * @brief Mesh smoothing algorithms: Laplacian, Taubin, and HC smoothing
 * 
 * Provides various smoothing methods to reduce noise while preserving
 * features and volume.
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
 * @brief Smoothing algorithm type
 */
enum class SmoothingAlgorithm {
    Laplacian,      ///< Simple Laplacian (may cause shrinkage)
    Taubin,         ///< Taubin λ/μ smoothing (prevents shrinkage)
    HCLaplacian,    ///< Humphrey's Classes algorithm (preserves volume)
    Cotangent       ///< Cotangent-weighted Laplacian (preserves shape better)
};

/**
 * @brief Options for mesh smoothing
 */
struct SmoothingOptions {
    SmoothingAlgorithm algorithm = SmoothingAlgorithm::Laplacian;
    
    int iterations = 1;             ///< Number of smoothing iterations
    float lambda = 0.5f;            ///< Smoothing factor (0-1, higher = more smoothing)
    
    // Taubin parameters
    float mu = -0.53f;              ///< Taubin inflation factor (should be < -lambda)
    
    // HC parameters
    float alpha = 0.0f;             ///< HC: influence of original position (0-1)
    float beta = 0.5f;              ///< HC: influence of previous iteration (0-1)
    
    bool preserveBoundary = true;   ///< Don't move boundary vertices
    bool preserveFeatures = false;  ///< Preserve sharp edges (by angle threshold)
    float featureAngle = 45.0f;     ///< Angle threshold for features (degrees)
    
    std::unordered_set<uint32_t> lockedVertices;  ///< Vertices that shouldn't move
};

/**
 * @brief Result information from smoothing
 */
struct SmoothingResult {
    int iterationsPerformed = 0;
    float averageDisplacement = 0.0f;
    float maxDisplacement = 0.0f;
    size_t verticesMoved = 0;
    size_t boundaryVerticesSkipped = 0;
    bool cancelled = false;
};

/**
 * @brief Mesh smoothing algorithms
 * 
 * Usage:
 * @code
 *     SmoothingOptions opts;
 *     opts.algorithm = SmoothingAlgorithm::Taubin;
 *     opts.iterations = 10;
 *     opts.preserveBoundary = true;
 *     
 *     auto result = MeshSmoother::smooth(mesh, opts, [](float p) {
 *         return true;  // Continue
 *     });
 * @endcode
 */
class MeshSmoother {
public:
    /**
     * @brief Smooth a mesh in-place
     * @param mesh Mesh to smooth (modified in place)
     * @param options Smoothing options
     * @param progress Optional progress callback
     * @return Smoothing statistics
     */
    static SmoothingResult smooth(
        MeshData& mesh,
        const SmoothingOptions& options,
        ProgressCallback progress = nullptr);
    
    /**
     * @brief Smooth a mesh (simple interface)
     * @param mesh Mesh to smooth (modified in place)
     * @param algorithm Smoothing algorithm
     * @param iterations Number of iterations
     * @param factor Smoothing factor (lambda)
     */
    static void smooth(
        MeshData& mesh,
        SmoothingAlgorithm algorithm,
        int iterations,
        float factor);
    
    /**
     * @brief Laplacian smoothing (single pass)
     * @param mesh Mesh to smooth
     * @param lambda Smoothing factor
     * @param preserveBoundary Whether to keep boundary fixed
     * @return Number of vertices moved
     */
    static size_t laplacianSmooth(
        MeshData& mesh,
        float lambda,
        bool preserveBoundary = true);
    
    /**
     * @brief Taubin smoothing (single pass = λ then μ)
     * @param mesh Mesh to smooth
     * @param lambda Shrinking factor
     * @param mu Inflation factor (typically negative, < -lambda)
     * @param preserveBoundary Whether to keep boundary fixed
     * @return Number of vertices moved
     */
    static size_t taubinSmooth(
        MeshData& mesh,
        float lambda,
        float mu,
        bool preserveBoundary = true);
    
    /**
     * @brief HC Laplacian smoothing (single iteration)
     * @param mesh Mesh to smooth
     * @param originalPositions Original vertex positions (before any smoothing)
     * @param alpha Original position influence
     * @param beta Previous iteration influence
     * @param preserveBoundary Whether to keep boundary fixed
     * @return Number of vertices moved
     */
    static size_t hcSmooth(
        MeshData& mesh,
        const std::vector<glm::vec3>& originalPositions,
        float alpha,
        float beta,
        bool preserveBoundary = true);
    
private:
    MeshSmoother() = default;
    
    // Helper functions
    static std::vector<std::vector<uint32_t>> buildAdjacencyList(const MeshData& mesh);
    static std::unordered_set<uint32_t> findBoundaryVertices(const MeshData& mesh);
    static std::unordered_set<uint32_t> findFeatureVertices(
        const MeshData& mesh, 
        float angleThreshold);
    
    static glm::vec3 computeLaplacian(
        const MeshData& mesh,
        uint32_t vertexIdx,
        const std::vector<std::vector<uint32_t>>& adjacency);
    
    static glm::vec3 computeCotangentLaplacian(
        const MeshData& mesh,
        uint32_t vertexIdx,
        const std::vector<std::vector<uint32_t>>& adjacency,
        const std::vector<std::vector<uint32_t>>& vertexFaces);
};

/**
 * @brief Smoothing operation state for advanced use
 */
class SmoothingState {
public:
    SmoothingState(MeshData& mesh, const SmoothingOptions& options);
    
    /// Perform single smoothing iteration
    void iterate();
    
    /// Get current iteration count
    int currentIteration() const { return currentIteration_; }
    
    /// Check if smoothing is complete
    bool isComplete() const { return currentIteration_ >= options_.iterations; }
    
    /// Get statistics
    SmoothingResult getResult() const;
    
private:
    MeshData& mesh_;
    SmoothingOptions options_;
    int currentIteration_ = 0;
    
    std::vector<glm::vec3> originalPositions_;
    std::vector<glm::vec3> previousPositions_;
    std::vector<std::vector<uint32_t>> adjacency_;
    std::unordered_set<uint32_t> fixedVertices_;
    
    float totalDisplacement_ = 0.0f;
    float maxDisplacement_ = 0.0f;
    size_t verticesMoved_ = 0;
};

} // namespace geometry
} // namespace dc3d
