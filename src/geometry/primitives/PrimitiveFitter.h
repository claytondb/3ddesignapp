/**
 * @file PrimitiveFitter.h
 * @brief Unified primitive fitting interface with auto-detection
 * 
 * Provides a high-level interface for fitting primitives to mesh selections,
 * including automatic primitive type detection.
 */

#pragma once

#include <vector>
#include <variant>
#include <memory>
#include <glm/glm.hpp>

#include "Plane.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Sphere.h"
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Type of primitive
 */
enum class PrimitiveType {
    Unknown,
    Plane,
    Cylinder,
    Cone,
    Sphere
};

/**
 * @brief Convert primitive type to string
 */
inline const char* primitiveTypeToString(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Plane: return "Plane";
        case PrimitiveType::Cylinder: return "Cylinder";
        case PrimitiveType::Cone: return "Cone";
        case PrimitiveType::Sphere: return "Sphere";
        default: return "Unknown";
    }
}

/**
 * @brief Holds any fitted primitive type
 */
using Primitive = std::variant<Plane, Cylinder, Cone, Sphere>;

/**
 * @brief Result of primitive fitting
 */
struct FitResult {
    bool success = false;
    PrimitiveType type = PrimitiveType::Unknown;
    Primitive primitive;              ///< The fitted primitive
    
    // Quality metrics
    float rmsError = 0.0f;           ///< Root mean square error
    float maxError = 0.0f;           ///< Maximum deviation
    float confidence = 0.0f;         ///< Confidence in fit (0-1)
    size_t inlierCount = 0;          ///< Points within tolerance
    size_t totalPoints = 0;          ///< Total points used
    float inlierRatio = 0.0f;        ///< inlierCount / totalPoints
    
    std::string errorMessage;
    
    /**
     * @brief Get typed primitive (throws if wrong type)
     */
    template<typename T>
    const T& get() const { return std::get<T>(primitive); }
    
    template<typename T>
    T& get() { return std::get<T>(primitive); }
    
    /**
     * @brief Check if holds specific type
     */
    template<typename T>
    bool holds() const { return std::holds_alternative<T>(primitive); }
};

/**
 * @brief Detection scores for each primitive type
 */
struct DetectionScores {
    float plane = 0.0f;
    float cylinder = 0.0f;
    float cone = 0.0f;
    float sphere = 0.0f;
    
    PrimitiveType best() const;
    float bestScore() const;
};

/**
 * @brief Options for primitive fitting
 */
struct FitOptions {
    // RANSAC options
    int ransacIterations = 500;      ///< Number of RANSAC iterations
    float inlierThreshold = 0.01f;   ///< Distance threshold for inliers (absolute)
    float inlierThresholdRel = 0.01f;///< Relative to bounding box diagonal
    bool useRelativeThreshold = true;///< Use relative threshold
    
    // Refinement
    int refinementIterations = 10;   ///< Iterative refinement steps
    bool useNormals = true;          ///< Use surface normals for fitting
    
    // Detection
    float detectionThreshold = 0.7f; ///< Minimum confidence for detection
    bool tryAllTypes = false;        ///< Fit all types and return best
};

/**
 * @brief Unified primitive fitting interface
 * 
 * Usage:
 * @code
 * PrimitiveFitter fitter;
 * 
 * // Auto-detect and fit
 * auto result = fitter.fitAuto(mesh, selectedFaces);
 * if (result.success) {
 *     switch (result.type) {
 *         case PrimitiveType::Plane: ... break;
 *         case PrimitiveType::Cylinder: ... break;
 *         // etc
 *     }
 * }
 * 
 * // Or fit specific type
 * auto planeResult = fitter.fitPlane(mesh, selectedFaces);
 * @endcode
 */
class PrimitiveFitter {
public:
    PrimitiveFitter() = default;
    
    /**
     * @brief Set fitting options
     */
    void setOptions(const FitOptions& options) { m_options = options; }
    const FitOptions& options() const { return m_options; }
    
    // ---- Auto Detection ----
    
    /**
     * @brief Detect best primitive type for selection
     * @param mesh The mesh
     * @param selectedFaces Indices of selected faces
     * @return Detection scores for each type
     */
    DetectionScores detectPrimitiveType(const MeshData& mesh, 
                                         const std::vector<uint32_t>& selectedFaces);
    
    /**
     * @brief Detect from point cloud
     */
    DetectionScores detectPrimitiveType(const std::vector<glm::vec3>& points,
                                         const std::vector<glm::vec3>& normals = {});
    
    // ---- Unified Fitting ----
    
    /**
     * @brief Automatically detect type and fit
     */
    FitResult fitAuto(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces);
    
    /**
     * @brief Fit specific primitive type
     */
    FitResult fitPrimitive(const MeshData& mesh, 
                           const std::vector<uint32_t>& selectedFaces,
                           PrimitiveType type);
    
    /**
     * @brief Fit all types and return best
     */
    FitResult fitBest(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces);
    
    // ---- Type-Specific Fitting ----
    
    FitResult fitPlane(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces);
    FitResult fitCylinder(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces);
    FitResult fitCone(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces);
    FitResult fitSphere(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces);
    
    // ---- Point Cloud Fitting ----
    
    FitResult fitAuto(const std::vector<glm::vec3>& points,
                      const std::vector<glm::vec3>& normals = {});
    
    FitResult fitPlane(const std::vector<glm::vec3>& points);
    FitResult fitCylinder(const std::vector<glm::vec3>& points,
                          const std::vector<glm::vec3>& normals = {});
    FitResult fitCone(const std::vector<glm::vec3>& points,
                      const std::vector<glm::vec3>& normals = {});
    FitResult fitSphere(const std::vector<glm::vec3>& points);

private:
    FitOptions m_options;
    
    // Helper to extract points and normals from selection
    void extractFromSelection(const MeshData& mesh, 
                             const std::vector<uint32_t>& selectedFaces,
                             std::vector<glm::vec3>& points,
                             std::vector<glm::vec3>& normals);
    
    // Compute inlier threshold from bounds
    float computeThreshold(const std::vector<glm::vec3>& points);
    
    // Compute confidence from fit metrics
    float computeConfidence(float rmsError, float threshold, 
                           size_t inliers, size_t total);
};

} // namespace geometry
} // namespace dc3d
