#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <glm/glm.hpp>

namespace dc {

// Forward declarations
class TriangleMesh;
class QuadMesh;
class NurbsSurface;

/**
 * @brief Quality metrics for auto-surfacing results
 */
struct AutoSurfaceMetrics {
    float maxDeviation;         // Maximum distance from original mesh
    float averageDeviation;     // Average deviation
    float rmsDeviation;         // Root mean square deviation
    float maxCurvatureError;    // Maximum curvature difference
    int patchCount;             // Number of surface patches
    int singularityCount;       // Number of extraordinary points
    float quadPercentage;       // Percentage of quad faces
    float processingTimeMs;     // Time taken to process
};

/**
 * @brief Parameters for auto-surfacing
 */
struct AutoSurfaceParams {
    // Target mesh quality
    int targetPatchCount = 100;         // Target number of quad patches
    float deviationTolerance = 0.01f;   // Maximum allowed deviation
    
    // Feature detection
    float featureAngleThreshold = 30.0f;    // Degrees for edge detection
    float featurePreservation = 0.8f;       // 0-1, how much to preserve features
    bool detectCreases = true;
    bool detectCorners = true;
    
    // Surface continuity
    int targetContinuity = 2;   // 0=G0, 1=G1, 2=G2
    
    // Optimization
    int maxIterations = 100;
    float convergenceThreshold = 0.001f;
    bool optimizeFlow = true;   // Align quads with principal curvatures
    
    // Output
    bool generateNurbs = true;
    int nurbsDegree = 3;
};

/**
 * @brief Feature edge for guiding quad mesh generation
 */
struct FeatureEdge {
    int vertex0;
    int vertex1;
    float sharpness;    // 0 = smooth, 1 = sharp crease
    glm::vec3 direction;
};

/**
 * @brief Feature point (corner or high-curvature point)
 */
struct FeaturePoint {
    int vertexIdx;
    glm::vec3 position;
    float importance;   // How important to preserve
    int targetValence;  // Desired valence in quad mesh
};

/**
 * @brief Progress callback for auto-surfacing
 */
using AutoSurfaceProgressCallback = std::function<void(float progress, const std::string& stage)>;

/**
 * @brief One-click automatic quad mesh and surface generation
 * 
 * Converts triangle meshes to high-quality quad meshes with optional
 * NURBS surface fitting. Implements feature-aware meshing with
 * G2 continuous surface fitting.
 */
class AutoSurface {
public:
    AutoSurface();
    ~AutoSurface();
    
    // Main interface
    std::unique_ptr<QuadMesh> generateQuadMesh(const TriangleMesh& input,
                                                const AutoSurfaceParams& params = {});
    
    std::vector<std::unique_ptr<NurbsSurface>> generateSurfaces(const TriangleMesh& input,
                                                                 const AutoSurfaceParams& params = {});
    
    // Step-by-step processing
    void setInput(const TriangleMesh& mesh);
    void detectFeatures(const AutoSurfaceParams& params);
    void generateInitialQuadMesh(const AutoSurfaceParams& params);
    void optimizeQuadMesh(const AutoSurfaceParams& params);
    void fitSurfaces(const AutoSurfaceParams& params);
    
    // Feature access
    const std::vector<FeatureEdge>& getFeatureEdges() const { return m_featureEdges; }
    const std::vector<FeaturePoint>& getFeaturePoints() const { return m_featurePoints; }
    
    // Results
    std::unique_ptr<QuadMesh> getQuadMesh();
    std::vector<std::unique_ptr<NurbsSurface>> getSurfaces();
    AutoSurfaceMetrics getMetrics() const { return m_metrics; }
    
    // Progress
    void setProgressCallback(AutoSurfaceProgressCallback callback);
    void cancel();
    bool isCancelled() const { return m_cancelled; }
    
private:
    // Input data
    std::shared_ptr<TriangleMesh> m_inputMesh;
    
    // Feature detection results
    std::vector<FeatureEdge> m_featureEdges;
    std::vector<FeaturePoint> m_featurePoints;
    
    // Generated output
    std::unique_ptr<QuadMesh> m_quadMesh;
    std::vector<std::unique_ptr<NurbsSurface>> m_surfaces;
    
    // Metrics
    AutoSurfaceMetrics m_metrics;
    
    // State
    AutoSurfaceProgressCallback m_progressCallback;
    bool m_cancelled = false;
    
    // Feature detection
    void detectSharpEdges(float angleThreshold);
    void detectCorners();
    void computePrincipalCurvatures();
    void classifyFeaturePoints();
    
    // Quad mesh generation (instant meshes style)
    void computeOrientationField();
    void computePositionField();
    void extractQuadMesh();
    void alignToFeatures();
    
    // Optimization
    void optimizeVertexPositions(int iterations);
    void optimizeConnectivity();
    void removeDegenerateFaces();
    void projectToOriginalMesh();
    
    // Surface fitting
    void segmentIntoPatches();
    void fitPatchSurfaces(int degree);
    void ensureContinuity(int targetContinuity);
    
    // Helper functions
    float computeEdgeAngle(int edge0, int edge1) const;
    glm::vec3 computeVertexNormal(int vertexIdx) const;
    std::pair<glm::vec3, glm::vec3> computePrincipalDirections(int vertexIdx) const;
    float computeDeviation(const glm::vec3& point) const;
    void reportProgress(float progress, const std::string& stage);
    
    // Orientation field data
    std::vector<glm::vec3> m_orientationField;      // Per-face orientation
    std::vector<float> m_orientationSingularities;  // Singularity indices
    
    // Position field data
    std::vector<glm::vec2> m_positionField;         // Per-vertex UV (parametric)
};

/**
 * @brief Utility functions for auto-surfacing
 */
namespace AutoSurfaceUtils {
    
    // Compute optimal patch count based on mesh complexity
    int estimatePatchCount(const TriangleMesh& mesh, float detailLevel);
    
    // Analyze mesh for best auto-surface parameters
    AutoSurfaceParams suggestParameters(const TriangleMesh& mesh);
    
    // Validate parameters
    bool validateParameters(const AutoSurfaceParams& params, std::string& error);
    
    // Compute mesh curvature statistics
    void computeCurvatureStats(const TriangleMesh& mesh,
                               float& minCurvature, float& maxCurvature,
                               float& avgCurvature);
}

} // namespace dc
