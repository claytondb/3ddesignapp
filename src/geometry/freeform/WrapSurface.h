#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <glm/glm.hpp>
#include "../NURBSSurface.h"

namespace dc {

// Use ControlPoint from NURBSSurface
using dc3d::geometry::ControlPoint;

// Forward declarations
class NurbsSurface;
class TriangleMesh;

/**
 * @brief Parameters for surface wrapping operations
 */
struct WrapParams {
    // Projection settings
    float maxDistance = 1.0f;           // Maximum projection distance
    float stepSize = 0.01f;             // Step size for iterative projection
    int maxIterations = 100;            // Maximum iterations per point
    
    // Continuity preservation
    bool maintainContinuity = true;     // Preserve surface continuity
    int continuityDegree = 2;           // G0=0, G1=1, G2=2
    
    // Constraint regions
    bool freezeBoundary = false;        // Don't move boundary control points
    std::vector<glm::ivec2> frozenControlPoints;  // Specific frozen CPs (i, j)
    
    // Smoothing
    float smoothingWeight = 0.1f;       // Post-wrap smoothing
    int smoothingIterations = 3;
};

/**
 * @brief Result of wrapping operation
 */
struct WrapResult {
    std::unique_ptr<NurbsSurface> surface;
    float maxDeviation;
    float averageDeviation;
    int movedControlPoints;
    std::vector<float> controlPointMovement;  // Movement distance per CP
    bool success;
    std::string message;
};

/**
 * @brief Progress callback for wrapping operations
 */
using WrapProgressCallback = std::function<void(float progress, const std::string& stage)>;

/**
 * @brief Surface wrapping and projection to mesh targets
 * 
 * Projects NURBS surface control points onto target meshes while
 * maintaining surface continuity and smoothness.
 */
class SurfaceWrapper {
public:
    SurfaceWrapper();
    ~SurfaceWrapper();
    
    // Basic wrapping
    WrapResult wrapToMesh(const NurbsSurface& surface,
                           const TriangleMesh& targetMesh,
                           const WrapParams& params = {});
    
    // Partial wrapping (region)
    WrapResult wrapRegion(const NurbsSurface& surface,
                           const TriangleMesh& targetMesh,
                           float uMin, float uMax,
                           float vMin, float vMax,
                           const WrapParams& params = {});
    
    // Control point snapping
    WrapResult snapControlPoints(const NurbsSurface& surface,
                                  const TriangleMesh& targetMesh,
                                  const std::vector<glm::ivec2>& controlPointIndices,
                                  const WrapParams& params = {});
    
    // Deformation-based wrapping
    WrapResult wrapWithDeformation(const NurbsSurface& surface,
                                    const TriangleMesh& sourceMesh,
                                    const TriangleMesh& targetMesh,
                                    const WrapParams& params = {});
    
    // Progressive wrapping (animate)
    std::vector<std::unique_ptr<NurbsSurface>> wrapProgressive(
        const NurbsSurface& surface,
        const TriangleMesh& targetMesh,
        int steps,
        const WrapParams& params = {});
    
    // Offset wrapping
    WrapResult wrapWithOffset(const NurbsSurface& surface,
                               const TriangleMesh& targetMesh,
                               float offset,
                               const WrapParams& params = {});
    
    // Progress
    void setProgressCallback(WrapProgressCallback callback);
    void cancel();
    
private:
    WrapProgressCallback m_progressCallback;
    bool m_cancelled = false;
    
    // Projection
    glm::vec3 projectPointToMesh(const glm::vec3& point,
                                  const TriangleMesh& mesh,
                                  float maxDistance) const;
    
    glm::vec3 projectPointToMeshWithNormal(const glm::vec3& point,
                                            const glm::vec3& direction,
                                            const TriangleMesh& mesh,
                                            float maxDistance) const;
    
    // Ray-mesh intersection
    bool rayMeshIntersect(const glm::vec3& origin,
                           const glm::vec3& direction,
                           const TriangleMesh& mesh,
                           glm::vec3& hitPoint,
                           glm::vec3& hitNormal,
                           float& hitDistance) const;
    
    // Continuity preservation
    void adjustForContinuity(std::vector<std::vector<ControlPoint>>& controlPoints,
                              const std::vector<std::vector<ControlPoint>>& originalControlPoints,
                              int continuityDegree);
    
    // Smoothing
    void smoothControlPoints(std::vector<std::vector<ControlPoint>>& controlPoints,
                              const WrapParams& params);
    
    void reportProgress(float progress, const std::string& stage);
};

/**
 * @brief Shrink wrap algorithm for organic surface fitting
 */
class ShrinkWrapper {
public:
    ShrinkWrapper();
    ~ShrinkWrapper();
    
    struct ShrinkParams {
        int iterations = 50;
        float stepSize = 0.1f;
        float smoothness = 0.5f;
        bool preserveVolume = false;
        float collisionOffset = 0.001f;
    };
    
    // Shrink wrap a surface onto target
    WrapResult shrinkWrap(const NurbsSurface& surface,
                           const TriangleMesh& targetMesh,
                           const ShrinkParams& params = ShrinkParams{});
    
    // Shrink wrap with constraints
    WrapResult shrinkWrapConstrained(const NurbsSurface& surface,
                                      const TriangleMesh& targetMesh,
                                      const std::vector<glm::vec3>& constraintPoints,
                                      const std::vector<glm::vec3>& constraintPositions,
                                      const ShrinkParams& params = ShrinkParams{});
    
    void setProgressCallback(WrapProgressCallback callback);
    void cancel();
    
private:
    WrapProgressCallback m_progressCallback;
    bool m_cancelled = false;
    
    void reportProgress(float progress, const std::string& stage);
};

/**
 * @brief Utility functions for surface wrapping
 */
namespace WrapUtils {
    
    // Compute distance field from mesh
    std::vector<float> computeDistanceField(const TriangleMesh& mesh,
                                             const glm::vec3& minBound,
                                             const glm::vec3& maxBound,
                                             int resolution);
    
    // Sample distance at point from precomputed field
    float sampleDistanceField(const std::vector<float>& field,
                               const glm::vec3& point,
                               const glm::vec3& minBound,
                               const glm::vec3& cellSize,
                               int resolution);
    
    // Compute closest point on mesh (brute force)
    glm::vec3 closestPointOnMesh(const glm::vec3& point, const TriangleMesh& mesh);
    
    // Compute closest point on triangle
    glm::vec3 closestPointOnTriangle(const glm::vec3& point,
                                      const glm::vec3& v0,
                                      const glm::vec3& v1,
                                      const glm::vec3& v2);
    
    // Build acceleration structure for mesh queries
    class MeshAccelerator {
    public:
        MeshAccelerator(const TriangleMesh& mesh);
        
        glm::vec3 closestPoint(const glm::vec3& point) const;
        bool rayIntersect(const glm::vec3& origin, const glm::vec3& direction,
                          glm::vec3& hitPoint, float& hitDistance) const;
        
    private:
        const TriangleMesh& m_mesh;
        // BVH or octree data
        struct BVHNode {
            glm::vec3 boundsMin;
            glm::vec3 boundsMax;
            int leftChild;
            int rightChild;
            std::vector<int> triangles;
        };
        std::vector<BVHNode> m_bvh;
        
        void buildBVH();
    };
}

} // namespace dc
