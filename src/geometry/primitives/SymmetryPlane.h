/**
 * @file SymmetryPlane.h
 * @brief Detection and analysis of plane symmetry in meshes
 * 
 * Provides algorithms to detect planes of symmetry and measure
 * reflection quality for mesh geometry.
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Plane.h"
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Result of symmetry detection
 */
struct SymmetryResult {
    bool found = false;
    Plane symmetryPlane;             ///< Detected plane of symmetry
    float quality = 0.0f;            ///< Reflection quality (0-1, higher = more symmetric)
    float avgDeviation = 0.0f;       ///< Average deviation after reflection
    float maxDeviation = 0.0f;       ///< Maximum deviation
    size_t matchedPairs = 0;         ///< Number of matched point pairs
    std::string errorMessage;
};

/**
 * @brief Options for symmetry detection
 */
struct SymmetryOptions {
    float matchTolerance = 0.01f;    ///< Tolerance for point matching (relative to bbox)
    int candidatePlanes = 6;         ///< Number of candidate planes to test
    bool testAxisAligned = true;     ///< Test axis-aligned planes (XY, XZ, YZ)
    bool testPCA = true;             ///< Test PCA-derived planes
    int refinementSteps = 5;         ///< Iterative refinement iterations
};

/**
 * @brief Detects and analyzes planar symmetry in meshes
 * 
 * Supports:
 * - Automatic symmetry plane detection
 * - Symmetry quality evaluation
 * - Interactive plane adjustment
 * - Multiple candidate plane testing
 */
class SymmetryPlane {
public:
    SymmetryPlane() = default;
    
    /**
     * @brief Construct from existing plane
     */
    explicit SymmetryPlane(const Plane& plane);
    
    // ---- Detection ----
    
    /**
     * @brief Detect the best plane of symmetry for a mesh
     * @param mesh Input mesh
     * @param options Detection options
     * @return Detection result with plane and quality
     */
    SymmetryResult detect(const MeshData& mesh, 
                          const SymmetryOptions& options = SymmetryOptions());
    
    /**
     * @brief Detect symmetry from point cloud
     */
    SymmetryResult detect(const std::vector<glm::vec3>& points,
                          const SymmetryOptions& options = SymmetryOptions());
    
    /**
     * @brief Detect symmetry for selected faces only
     */
    SymmetryResult detectForSelection(const MeshData& mesh,
                                       const std::vector<uint32_t>& selectedFaces,
                                       const SymmetryOptions& options = SymmetryOptions());
    
    // ---- Evaluation ----
    
    /**
     * @brief Evaluate symmetry quality for a given plane
     * @param mesh Input mesh
     * @param plane Candidate symmetry plane
     * @param tolerance Matching tolerance
     * @return Quality score (0-1)
     */
    float evaluateSymmetry(const MeshData& mesh, 
                           const Plane& plane,
                           float tolerance = 0.01f);
    
    /**
     * @brief Evaluate symmetry for point cloud
     */
    float evaluateSymmetry(const std::vector<glm::vec3>& points,
                           const Plane& plane,
                           float tolerance = 0.01f);
    
    /**
     * @brief Get detailed symmetry metrics
     */
    SymmetryResult evaluateDetailed(const MeshData& mesh,
                                     const Plane& plane,
                                     float tolerance = 0.01f);
    
    // ---- Adjustment ----
    
    /**
     * @brief Refine symmetry plane position
     * @param mesh Input mesh
     * @param initialPlane Starting plane
     * @param iterations Refinement iterations
     * @return Refined plane
     */
    Plane refine(const MeshData& mesh,
                 const Plane& initialPlane,
                 int iterations = 10);
    
    /**
     * @brief Refine using gradient descent
     */
    Plane refineGradient(const std::vector<glm::vec3>& points,
                         const Plane& initialPlane,
                         int iterations = 20);
    
    // ---- Reflection ----
    
    /**
     * @brief Reflect a point across the symmetry plane
     */
    glm::vec3 reflectPoint(const glm::vec3& point) const;
    
    /**
     * @brief Reflect a mesh across the plane (returns new mesh)
     */
    MeshData reflectMesh(const MeshData& mesh) const;
    
    /**
     * @brief Find the closest point and its reflection pair
     * @param point Query point
     * @param candidates Candidate points to search
     * @param tolerance Maximum distance for matching
     * @return Index of matched point, or -1 if no match
     */
    int findReflectionMatch(const glm::vec3& point,
                            const std::vector<glm::vec3>& candidates,
                            float tolerance) const;
    
    // ---- Accessors ----
    
    const Plane& plane() const { return m_plane; }
    void setPlane(const Plane& p) { m_plane = p; }
    
    /**
     * @brief Rotate plane normal around axis
     */
    void rotateNormal(const glm::vec3& axis, float angleRadians);
    
    /**
     * @brief Translate plane
     */
    void translate(float distance);
    
    // ---- Candidate Generation ----
    
    /**
     * @brief Generate candidate symmetry planes to test
     * @param points Point cloud
     * @param options Detection options
     * @return List of candidate planes
     */
    static std::vector<Plane> generateCandidates(const std::vector<glm::vec3>& points,
                                                  const SymmetryOptions& options);

private:
    Plane m_plane;
    
    // Internal helpers
    void computeBoundsAndCenter(const std::vector<glm::vec3>& points,
                                glm::vec3& center, glm::vec3& extent);
    
    Plane fitSymmetryPlanePCA(const std::vector<glm::vec3>& points);
};

/**
 * @brief Multi-plane symmetry detector (for objects with multiple symmetry planes)
 */
class MultiSymmetryDetector {
public:
    /**
     * @brief Find all significant planes of symmetry
     * @param mesh Input mesh
     * @param minQuality Minimum quality threshold
     * @param maxPlanes Maximum planes to find
     * @return List of symmetry results, sorted by quality
     */
    std::vector<SymmetryResult> detectAll(const MeshData& mesh,
                                           float minQuality = 0.7f,
                                           int maxPlanes = 3);
    
    /**
     * @brief Check for rotational symmetry (multiple planes through same axis)
     * @param mesh Input mesh
     * @param axis Test axis
     * @param foldCount Number of symmetry folds to test (2, 3, 4, 6, etc.)
     * @return Quality of rotational symmetry
     */
    float checkRotationalSymmetry(const MeshData& mesh,
                                  const glm::vec3& axis,
                                  int foldCount);
};

} // namespace geometry
} // namespace dc3d
