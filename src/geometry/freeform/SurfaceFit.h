#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

namespace dc {

// Forward declarations
class NurbsSurface;
class TriangleMesh;
class QuadMesh;
class NurbsCurve;

/**
 * @brief Boundary condition types for surface fitting
 */
enum class BoundaryCondition {
    Free,           // No constraint
    Position,       // Match position
    Tangent,        // Match tangent (G1)
    Curvature,      // Match curvature (G2)
    Fixed           // Fixed control points
};

/**
 * @brief Result of surface fitting operation
 */
struct SurfaceFitResult {
    std::unique_ptr<NurbsSurface> surface;
    float maxDeviation;
    float rmsDeviation;
    float averageDeviation;
    int iterations;
    bool converged;
    std::string message;
};

/**
 * @brief Parameters for surface fitting
 */
struct SurfaceFitParams {
    // Surface properties
    int uDegree = 3;
    int vDegree = 3;
    int uControlPoints = 8;
    int vControlPoints = 8;
    
    // Fitting tolerance
    float deviationTolerance = 0.01f;
    int maxIterations = 100;
    float convergenceThreshold = 0.0001f;
    
    // Boundary conditions
    BoundaryCondition uMinCondition = BoundaryCondition::Free;
    BoundaryCondition uMaxCondition = BoundaryCondition::Free;
    BoundaryCondition vMinCondition = BoundaryCondition::Free;
    BoundaryCondition vMaxCondition = BoundaryCondition::Free;
    
    // Smoothness
    float smoothingWeight = 0.1f;       // Regularization
    float fairingWeight = 0.01f;        // Energy minimization
    
    // Adaptive refinement
    bool adaptiveRefinement = true;
    int maxRefinementLevel = 3;
};

/**
 * @brief Constraint for surface fitting
 */
struct FitConstraint {
    glm::vec2 uv;           // Parameter location
    glm::vec3 position;     // Target position
    glm::vec3 normal;       // Target normal (for G1)
    glm::vec3 tangentU;     // U tangent direction
    glm::vec3 tangentV;     // V tangent direction
    float curvatureU;       // U curvature (for G2)
    float curvatureV;       // V curvature (for G2)
    BoundaryCondition type;
    float weight;           // Constraint weight
};

/**
 * @brief Progress callback for fitting operations
 */
using FitProgressCallback = std::function<void(float progress, float currentDeviation)>;

/**
 * @brief NURBS surface fitting to point clouds, meshes, and curve networks
 * 
 * Implements various fitting algorithms including:
 * - Least squares approximation
 * - Deviation-based refinement
 * - Curvature-based smoothing
 * - Multi-patch fitting with continuity
 */
class SurfaceFitter {
public:
    SurfaceFitter();
    ~SurfaceFitter();
    
    // Basic fitting
    SurfaceFitResult fitToPoints(const std::vector<glm::vec3>& points,
                                  const SurfaceFitParams& params = {});
    
    SurfaceFitResult fitToPointsWithNormals(const std::vector<glm::vec3>& points,
                                             const std::vector<glm::vec3>& normals,
                                             const SurfaceFitParams& params = {});
    
    // Mesh region fitting
    SurfaceFitResult fitToMeshRegion(const TriangleMesh& mesh,
                                      const std::vector<int>& faceIndices,
                                      const SurfaceFitParams& params = {});
    
    SurfaceFitResult fitToQuadMesh(const QuadMesh& mesh,
                                    const SurfaceFitParams& params = {});
    
    // Curve network fitting
    SurfaceFitResult fitToCurveNetwork(const std::vector<std::shared_ptr<NurbsCurve>>& curves,
                                        const SurfaceFitParams& params = {});
    
    SurfaceFitResult fitWithBoundaryCurves(const std::vector<glm::vec3>& points,
                                            const std::vector<std::shared_ptr<NurbsCurve>>& boundaries,
                                            const SurfaceFitParams& params = {});
    
    // Constrained fitting
    void addConstraint(const FitConstraint& constraint);
    void clearConstraints();
    SurfaceFitResult fitWithConstraints(const std::vector<glm::vec3>& points,
                                         const SurfaceFitParams& params = {});
    
    // Refinement
    SurfaceFitResult refine(const NurbsSurface& surface,
                             const std::vector<glm::vec3>& targetPoints,
                             const SurfaceFitParams& params = {});
    
    SurfaceFitResult refineAdaptive(const NurbsSurface& surface,
                                     const std::vector<glm::vec3>& targetPoints,
                                     float tolerance);
    
    // Multi-patch fitting
    std::vector<SurfaceFitResult> fitMultiPatch(const TriangleMesh& mesh,
                                                 const std::vector<std::vector<int>>& patches,
                                                 int continuity,
                                                 const SurfaceFitParams& params = {});
    
    // Progress
    void setProgressCallback(FitProgressCallback callback);
    void cancel();
    
private:
    std::vector<FitConstraint> m_constraints;
    FitProgressCallback m_progressCallback;
    bool m_cancelled = false;
    
    // Parameterization
    std::vector<glm::vec2> parameterizePoints(const std::vector<glm::vec3>& points,
                                               const glm::vec3& uDir,
                                               const glm::vec3& vDir,
                                               const glm::vec3& origin);
    
    std::vector<glm::vec2> parameterizeFromMesh(const std::vector<glm::vec3>& points,
                                                 const TriangleMesh& mesh,
                                                 const std::vector<int>& faceIndices);
    
    // Fitting core
    std::unique_ptr<NurbsSurface> solveLinearFit(const std::vector<glm::vec3>& points,
                                                  const std::vector<glm::vec2>& params,
                                                  const SurfaceFitParams& fitParams);
    
    void applyBoundaryConditions(std::vector<std::vector<glm::vec3>>& controlPoints,
                                  const SurfaceFitParams& params);
    
    void applySmoothingRegularization(std::vector<std::vector<glm::vec3>>& controlPoints,
                                       float weight);
    
    float computeDeviation(const NurbsSurface& surface,
                            const std::vector<glm::vec3>& points,
                            const std::vector<glm::vec2>& params);
    
    // Adaptive refinement
    std::vector<glm::vec2> findHighDeviationRegions(const NurbsSurface& surface,
                                                     const std::vector<glm::vec3>& points,
                                                     const std::vector<glm::vec2>& params,
                                                     float threshold);
    
    std::unique_ptr<NurbsSurface> refineInRegions(const NurbsSurface& surface,
                                                   const std::vector<glm::vec2>& regions);
    
    void reportProgress(float progress, float deviation);
};

/**
 * @brief Utility functions for surface fitting
 */
namespace SurfaceFitUtils {
    
    // Estimate initial control point count
    std::pair<int, int> estimateControlPointCount(const std::vector<glm::vec3>& points,
                                                   int degree, float tolerance);
    
    // Compute optimal parameterization
    std::vector<glm::vec2> computeChordLengthParameterization(const std::vector<glm::vec3>& points);
    std::vector<glm::vec2> computeCentripetalParameterization(const std::vector<glm::vec3>& points);
    
    // Fitting quality metrics
    float computeRMSDeviation(const NurbsSurface& surface,
                               const std::vector<glm::vec3>& points);
    
    float computeMaxDeviation(const NurbsSurface& surface,
                               const std::vector<glm::vec3>& points);
    
    // Surface analysis
    float computeSurfaceEnergy(const NurbsSurface& surface);
    float computeFairnessMetric(const NurbsSurface& surface);
}

} // namespace dc
