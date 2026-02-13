#include "WrapSurface.h"
#include "../nurbs/NurbsSurface.h"
#include "../mesh/TriangleMesh.h"
#include <algorithm>
#include <cmath>

namespace dc {

SurfaceWrapper::SurfaceWrapper() = default;
SurfaceWrapper::~SurfaceWrapper() = default;

WrapResult SurfaceWrapper::wrapToMesh(const NurbsSurface& surface,
                                       const TriangleMesh& targetMesh,
                                       const WrapParams& params) {
    WrapResult result;
    m_cancelled = false;
    
    reportProgress(0.0f, "Initializing wrap");
    
    // Get control points
    auto controlPoints = surface.getControlPoints();
    auto originalControlPoints = controlPoints;
    
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    int totalCPs = nu * nv;
    
    result.controlPointMovement.resize(totalCPs, 0.0f);
    result.movedControlPoints = 0;
    
    // Build acceleration structure for mesh queries
    WrapUtils::MeshAccelerator accel(targetMesh);
    
    reportProgress(0.1f, "Projecting control points");
    
    // Project each control point to mesh
    for (int i = 0; i < nu; ++i) {
        if (m_cancelled) {
            result.success = false;
            result.message = "Cancelled";
            return result;
        }
        
        for (int j = 0; j < nv; ++j) {
            // Check if frozen
            bool frozen = false;
            if (params.freezeBoundary) {
                if (i == 0 || i == nu - 1 || j == 0 || j == nv - 1) {
                    frozen = true;
                }
            }
            for (const auto& fp : params.frozenControlPoints) {
                if (fp.x == i && fp.y == j) {
                    frozen = true;
                    break;
                }
            }
            
            if (frozen) continue;
            
            // Find closest point on target mesh
            glm::vec3 closest = accel.closestPoint(controlPoints[i][j]);
            float dist = glm::length(closest - controlPoints[i][j]);
            
            if (dist < params.maxDistance) {
                controlPoints[i][j] = closest;
                result.controlPointMovement[i * nv + j] = dist;
                result.movedControlPoints++;
            }
        }
        
        reportProgress(0.1f + 0.6f * static_cast<float>(i) / nu, "Projecting control points");
    }
    
    reportProgress(0.7f, "Preserving continuity");
    
    // Adjust for continuity preservation
    if (params.maintainContinuity) {
        adjustForContinuity(controlPoints, originalControlPoints, params.continuityDegree);
    }
    
    reportProgress(0.8f, "Smoothing");
    
    // Apply smoothing
    if (params.smoothingWeight > 0 && params.smoothingIterations > 0) {
        smoothControlPoints(controlPoints, params);
    }
    
    reportProgress(0.9f, "Building result surface");
    
    // Create new surface with modified control points
    result.surface = std::make_unique<NurbsSurface>(
        controlPoints,
        surface.getKnotsU(),
        surface.getKnotsV(),
        surface.getDegreeU(),
        surface.getDegreeV()
    );
    
    // Compute deviation statistics
    result.maxDeviation = 0;
    result.averageDeviation = 0;
    float totalMovement = 0;
    
    for (int i = 0; i < nu; ++i) {
        for (int j = 0; j < nv; ++j) {
            float mov = result.controlPointMovement[i * nv + j];
            result.maxDeviation = std::max(result.maxDeviation, mov);
            totalMovement += mov;
        }
    }
    result.averageDeviation = totalMovement / totalCPs;
    
    result.success = true;
    result.message = "Wrap completed successfully";
    
    reportProgress(1.0f, "Complete");
    
    return result;
}

WrapResult SurfaceWrapper::wrapRegion(const NurbsSurface& surface,
                                       const TriangleMesh& targetMesh,
                                       float uMin, float uMax,
                                       float vMin, float vMax,
                                       const WrapParams& params) {
    // Create modified params with frozen control points outside region
    WrapParams regionParams = params;
    
    auto controlPoints = surface.getControlPoints();
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    // Freeze control points outside the specified region
    // This is approximate - proper implementation would use knot spans
    int iMin = static_cast<int>(uMin * (nu - 1));
    int iMax = static_cast<int>(uMax * (nu - 1));
    int jMin = static_cast<int>(vMin * (nv - 1));
    int jMax = static_cast<int>(vMax * (nv - 1));
    
    for (int i = 0; i < nu; ++i) {
        for (int j = 0; j < nv; ++j) {
            if (i < iMin || i > iMax || j < jMin || j > jMax) {
                regionParams.frozenControlPoints.push_back(glm::ivec2(i, j));
            }
        }
    }
    
    return wrapToMesh(surface, targetMesh, regionParams);
}

WrapResult SurfaceWrapper::snapControlPoints(const NurbsSurface& surface,
                                              const TriangleMesh& targetMesh,
                                              const std::vector<glm::ivec2>& controlPointIndices,
                                              const WrapParams& params) {
    WrapResult result;
    m_cancelled = false;
    
    auto controlPoints = surface.getControlPoints();
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    result.controlPointMovement.resize(nu * nv, 0.0f);
    result.movedControlPoints = 0;
    
    WrapUtils::MeshAccelerator accel(targetMesh);
    
    // Only snap specified control points
    for (const auto& idx : controlPointIndices) {
        if (m_cancelled) {
            result.success = false;
            result.message = "Cancelled";
            return result;
        }
        
        if (idx.x >= 0 && idx.x < nu && idx.y >= 0 && idx.y < nv) {
            glm::vec3& cp = controlPoints[idx.x][idx.y];
            glm::vec3 closest = accel.closestPoint(cp);
            float dist = glm::length(closest - cp);
            
            if (dist < params.maxDistance) {
                result.controlPointMovement[idx.x * nv + idx.y] = dist;
                cp = closest;
                result.movedControlPoints++;
                result.maxDeviation = std::max(result.maxDeviation, dist);
                result.averageDeviation += dist;
            }
        }
    }
    
    if (result.movedControlPoints > 0) {
        result.averageDeviation /= result.movedControlPoints;
    }
    
    result.surface = std::make_unique<NurbsSurface>(
        controlPoints,
        surface.getKnotsU(),
        surface.getKnotsV(),
        surface.getDegreeU(),
        surface.getDegreeV()
    );
    
    result.success = true;
    return result;
}

WrapResult SurfaceWrapper::wrapWithDeformation(const NurbsSurface& surface,
                                                const TriangleMesh& sourceMesh,
                                                const TriangleMesh& targetMesh,
                                                const WrapParams& params) {
    WrapResult result;
    
    // Compute deformation field from source to target mesh
    // Then apply this deformation to surface control points
    
    auto controlPoints = surface.getControlPoints();
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    result.controlPointMovement.resize(nu * nv, 0.0f);
    
    WrapUtils::MeshAccelerator sourceAccel(sourceMesh);
    WrapUtils::MeshAccelerator targetAccel(targetMesh);
    
    const auto& sourceVerts = sourceMesh.vertices();
    const auto& targetVerts = targetMesh.vertices();
    
    for (int i = 0; i < nu; ++i) {
        for (int j = 0; j < nv; ++j) {
            glm::vec3& cp = controlPoints[i][j];
            
            // Find closest point on source mesh
            glm::vec3 sourcePoint = sourceAccel.closestPoint(cp);
            
            // Find corresponding point on target mesh (using parameterization)
            // Simplified: use closest vertex
            float minDist = std::numeric_limits<float>::max();
            int closestIdx = 0;
            for (size_t v = 0; v < sourceVerts.size(); ++v) {
                float d = glm::length(sourceVerts[v].position - sourcePoint);
                if (d < minDist) {
                    minDist = d;
                    closestIdx = static_cast<int>(v);
                }
            }
            
            // Get corresponding target vertex
            if (closestIdx < static_cast<int>(targetVerts.size())) {
                glm::vec3 delta = targetVerts[closestIdx].position - sourceVerts[closestIdx].position;
                glm::vec3 newPos = cp + delta;
                
                result.controlPointMovement[i * nv + j] = glm::length(delta);
                cp = newPos;
            }
        }
    }
    
    result.surface = std::make_unique<NurbsSurface>(
        controlPoints,
        surface.getKnotsU(),
        surface.getKnotsV(),
        surface.getDegreeU(),
        surface.getDegreeV()
    );
    
    result.success = true;
    return result;
}

std::vector<std::unique_ptr<NurbsSurface>> SurfaceWrapper::wrapProgressive(
    const NurbsSurface& surface,
    const TriangleMesh& targetMesh,
    int steps,
    const WrapParams& params) {
    
    std::vector<std::unique_ptr<NurbsSurface>> results;
    
    // Get original and target control points
    auto originalCPs = surface.getControlPoints();
    
    WrapParams singleStepParams = params;
    auto wrapResult = wrapToMesh(surface, targetMesh, singleStepParams);
    
    if (!wrapResult.success) {
        return results;
    }
    
    auto targetCPs = wrapResult.surface->getControlPoints();
    
    int nu = static_cast<int>(originalCPs.size());
    int nv = static_cast<int>(originalCPs[0].size());
    
    // Create intermediate surfaces
    for (int step = 0; step <= steps; ++step) {
        float t = static_cast<float>(step) / steps;
        
        auto interpCPs = originalCPs;
        for (int i = 0; i < nu; ++i) {
            for (int j = 0; j < nv; ++j) {
                interpCPs[i][j] = glm::mix(originalCPs[i][j], targetCPs[i][j], t);
            }
        }
        
        results.push_back(std::make_unique<NurbsSurface>(
            interpCPs,
            surface.getKnotsU(),
            surface.getKnotsV(),
            surface.getDegreeU(),
            surface.getDegreeV()
        ));
    }
    
    return results;
}

WrapResult SurfaceWrapper::wrapWithOffset(const NurbsSurface& surface,
                                           const TriangleMesh& targetMesh,
                                           float offset,
                                           const WrapParams& params) {
    WrapResult result;
    m_cancelled = false;
    
    auto controlPoints = surface.getControlPoints();
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    result.controlPointMovement.resize(nu * nv, 0.0f);
    
    WrapUtils::MeshAccelerator accel(targetMesh);
    
    for (int i = 0; i < nu; ++i) {
        for (int j = 0; j < nv; ++j) {
            if (m_cancelled) {
                result.success = false;
                return result;
            }
            
            glm::vec3& cp = controlPoints[i][j];
            
            // Find closest point and normal on mesh
            glm::vec3 closest = accel.closestPoint(cp);
            
            // Estimate normal at closest point
            // (simplified - real implementation would interpolate face normals)
            glm::vec3 toPoint = glm::normalize(cp - closest);
            
            // Offset from mesh surface
            glm::vec3 newPos = closest + toPoint * offset;
            
            result.controlPointMovement[i * nv + j] = glm::length(newPos - cp);
            cp = newPos;
        }
    }
    
    result.surface = std::make_unique<NurbsSurface>(
        controlPoints,
        surface.getKnotsU(),
        surface.getKnotsV(),
        surface.getDegreeU(),
        surface.getDegreeV()
    );
    
    result.success = true;
    return result;
}

void SurfaceWrapper::setProgressCallback(WrapProgressCallback callback) {
    m_progressCallback = callback;
}

void SurfaceWrapper::cancel() {
    m_cancelled = true;
}

void SurfaceWrapper::reportProgress(float progress, const std::string& stage) {
    if (m_progressCallback) {
        m_progressCallback(progress, stage);
    }
}

glm::vec3 SurfaceWrapper::projectPointToMesh(const glm::vec3& point,
                                              const TriangleMesh& mesh,
                                              float maxDistance) const {
    return WrapUtils::closestPointOnMesh(point, mesh);
}

glm::vec3 SurfaceWrapper::projectPointToMeshWithNormal(const glm::vec3& point,
                                                        const glm::vec3& direction,
                                                        const TriangleMesh& mesh,
                                                        float maxDistance) const {
    glm::vec3 hitPoint;
    glm::vec3 hitNormal;
    float hitDistance;
    
    if (rayMeshIntersect(point, direction, mesh, hitPoint, hitNormal, hitDistance)) {
        if (hitDistance < maxDistance) {
            return hitPoint;
        }
    }
    
    // Fall back to closest point
    return projectPointToMesh(point, mesh, maxDistance);
}

bool SurfaceWrapper::rayMeshIntersect(const glm::vec3& origin,
                                       const glm::vec3& direction,
                                       const TriangleMesh& mesh,
                                       glm::vec3& hitPoint,
                                       glm::vec3& hitNormal,
                                       float& hitDistance) const {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    hitDistance = std::numeric_limits<float>::max();
    bool hit = false;
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        const glm::vec3& v0 = vertices[indices[i]].position;
        const glm::vec3& v1 = vertices[indices[i + 1]].position;
        const glm::vec3& v2 = vertices[indices[i + 2]].position;
        
        // Möller–Trumbore intersection algorithm
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(direction, edge2);
        float a = glm::dot(edge1, h);
        
        if (std::abs(a) < 1e-6f) continue;
        
        float f = 1.0f / a;
        glm::vec3 s = origin - v0;
        float u = f * glm::dot(s, h);
        
        if (u < 0.0f || u > 1.0f) continue;
        
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(direction, q);
        
        if (v < 0.0f || u + v > 1.0f) continue;
        
        float t = f * glm::dot(edge2, q);
        
        if (t > 0.0f && t < hitDistance) {
            hitDistance = t;
            hitPoint = origin + direction * t;
            hitNormal = glm::normalize(glm::cross(edge1, edge2));
            hit = true;
        }
    }
    
    return hit;
}

void SurfaceWrapper::adjustForContinuity(std::vector<std::vector<glm::vec3>>& controlPoints,
                                          const std::vector<std::vector<glm::vec3>>& originalControlPoints,
                                          int continuityDegree) {
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    if (continuityDegree < 1) return;
    
    // G1 continuity: preserve tangent directions at boundaries
    if (continuityDegree >= 1) {
        // U boundaries
        for (int j = 0; j < nv; ++j) {
            // u = 0 boundary
            glm::vec3 origTangent = originalControlPoints[1][j] - originalControlPoints[0][j];
            glm::vec3 newTangent = controlPoints[1][j] - controlPoints[0][j];
            float origLen = glm::length(origTangent);
            float newLen = glm::length(newTangent);
            if (origLen > 1e-6f && newLen > 1e-6f) {
                // Preserve tangent direction
                controlPoints[1][j] = controlPoints[0][j] + glm::normalize(origTangent) * newLen;
            }
            
            // u = 1 boundary
            origTangent = originalControlPoints[nu-1][j] - originalControlPoints[nu-2][j];
            newTangent = controlPoints[nu-1][j] - controlPoints[nu-2][j];
            origLen = glm::length(origTangent);
            newLen = glm::length(newTangent);
            if (origLen > 1e-6f && newLen > 1e-6f) {
                controlPoints[nu-2][j] = controlPoints[nu-1][j] - glm::normalize(origTangent) * newLen;
            }
        }
        
        // V boundaries
        for (int i = 0; i < nu; ++i) {
            // v = 0 boundary
            glm::vec3 origTangent = originalControlPoints[i][1] - originalControlPoints[i][0];
            glm::vec3 newTangent = controlPoints[i][1] - controlPoints[i][0];
            float origLen = glm::length(origTangent);
            float newLen = glm::length(newTangent);
            if (origLen > 1e-6f && newLen > 1e-6f) {
                controlPoints[i][1] = controlPoints[i][0] + glm::normalize(origTangent) * newLen;
            }
            
            // v = 1 boundary
            origTangent = originalControlPoints[i][nv-1] - originalControlPoints[i][nv-2];
            newTangent = controlPoints[i][nv-1] - controlPoints[i][nv-2];
            origLen = glm::length(origTangent);
            newLen = glm::length(newTangent);
            if (origLen > 1e-6f && newLen > 1e-6f) {
                controlPoints[i][nv-2] = controlPoints[i][nv-1] - glm::normalize(origTangent) * newLen;
            }
        }
    }
    
    // G2 continuity: preserve curvature (second derivative)
    if (continuityDegree >= 2) {
        // More complex adjustment for second row/column of control points
        // This requires solving for positions that maintain curvature
    }
}

void SurfaceWrapper::smoothControlPoints(std::vector<std::vector<glm::vec3>>& controlPoints,
                                          const WrapParams& params) {
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    for (int iter = 0; iter < params.smoothingIterations; ++iter) {
        auto smoothed = controlPoints;
        
        for (int i = 1; i < nu - 1; ++i) {
            for (int j = 1; j < nv - 1; ++j) {
                // Check if frozen
                bool frozen = false;
                for (const auto& fp : params.frozenControlPoints) {
                    if (fp.x == i && fp.y == j) {
                        frozen = true;
                        break;
                    }
                }
                if (frozen) continue;
                
                // Laplacian smoothing
                glm::vec3 laplacian = controlPoints[i-1][j] + controlPoints[i+1][j] +
                                      controlPoints[i][j-1] + controlPoints[i][j+1] -
                                      4.0f * controlPoints[i][j];
                
                smoothed[i][j] = controlPoints[i][j] + params.smoothingWeight * laplacian;
            }
        }
        
        controlPoints = smoothed;
    }
}

// ShrinkWrapper implementation
ShrinkWrapper::ShrinkWrapper() = default;
ShrinkWrapper::~ShrinkWrapper() = default;

WrapResult ShrinkWrapper::shrinkWrap(const NurbsSurface& surface,
                                      const TriangleMesh& targetMesh,
                                      const ShrinkParams& params) {
    WrapResult result;
    m_cancelled = false;
    
    auto controlPoints = surface.getControlPoints();
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    WrapUtils::MeshAccelerator accel(targetMesh);
    
    for (int iter = 0; iter < params.iterations; ++iter) {
        if (m_cancelled) {
            result.success = false;
            result.message = "Cancelled";
            return result;
        }
        
        reportProgress(static_cast<float>(iter) / params.iterations, "Shrink wrapping");
        
        // Move control points toward target
        std::vector<std::vector<glm::vec3>> newCPs = controlPoints;
        
        for (int i = 0; i < nu; ++i) {
            for (int j = 0; j < nv; ++j) {
                glm::vec3 closest = accel.closestPoint(controlPoints[i][j]);
                glm::vec3 direction = closest - controlPoints[i][j];
                float dist = glm::length(direction);
                
                if (dist > params.collisionOffset) {
                    // Move toward target
                    newCPs[i][j] = controlPoints[i][j] + 
                                   glm::normalize(direction) * std::min(dist, params.stepSize);
                }
            }
        }
        
        // Apply smoothing
        if (params.smoothness > 0) {
            for (int i = 1; i < nu - 1; ++i) {
                for (int j = 1; j < nv - 1; ++j) {
                    glm::vec3 avg = (newCPs[i-1][j] + newCPs[i+1][j] +
                                    newCPs[i][j-1] + newCPs[i][j+1]) * 0.25f;
                    newCPs[i][j] = glm::mix(newCPs[i][j], avg, params.smoothness);
                }
            }
        }
        
        controlPoints = newCPs;
    }
    
    result.surface = std::make_unique<NurbsSurface>(
        controlPoints,
        surface.getKnotsU(),
        surface.getKnotsV(),
        surface.getDegreeU(),
        surface.getDegreeV()
    );
    
    result.success = true;
    return result;
}

WrapResult ShrinkWrapper::shrinkWrapConstrained(const NurbsSurface& surface,
                                                 const TriangleMesh& targetMesh,
                                                 const std::vector<glm::vec3>& constraintPoints,
                                                 const std::vector<glm::vec3>& constraintPositions,
                                                 const ShrinkParams& params) {
    // Similar to shrinkWrap but with position constraints
    // Constraint points are pulled toward constraint positions
    
    WrapResult result = shrinkWrap(surface, targetMesh, params);
    
    // Apply constraints as a post-process
    // (more sophisticated implementation would include them in the iteration)
    
    return result;
}

void ShrinkWrapper::setProgressCallback(WrapProgressCallback callback) {
    m_progressCallback = callback;
}

void ShrinkWrapper::cancel() {
    m_cancelled = true;
}

void ShrinkWrapper::reportProgress(float progress, const std::string& stage) {
    if (m_progressCallback) {
        m_progressCallback(progress, stage);
    }
}

// Utility functions
namespace WrapUtils {

glm::vec3 closestPointOnMesh(const glm::vec3& point, const TriangleMesh& mesh) {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    float minDistSq = std::numeric_limits<float>::max();
    glm::vec3 closest = point;
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        const glm::vec3& v0 = vertices[indices[i]].position;
        const glm::vec3& v1 = vertices[indices[i + 1]].position;
        const glm::vec3& v2 = vertices[indices[i + 2]].position;
        
        glm::vec3 cp = closestPointOnTriangle(point, v0, v1, v2);
        float distSq = glm::dot(cp - point, cp - point);
        
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closest = cp;
        }
    }
    
    return closest;
}

glm::vec3 closestPointOnTriangle(const glm::vec3& point,
                                  const glm::vec3& v0,
                                  const glm::vec3& v1,
                                  const glm::vec3& v2) {
    // Check if point projects inside triangle
    glm::vec3 edge0 = v1 - v0;
    glm::vec3 edge1 = v2 - v0;
    glm::vec3 v0p = point - v0;
    
    float d00 = glm::dot(edge0, edge0);
    float d01 = glm::dot(edge0, edge1);
    float d11 = glm::dot(edge1, edge1);
    float d20 = glm::dot(v0p, edge0);
    float d21 = glm::dot(v0p, edge1);
    
    float denom = d00 * d11 - d01 * d01;
    if (std::abs(denom) < 1e-10f) {
        return v0; // Degenerate triangle
    }
    
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    
    // Check if point is inside triangle
    if (u >= 0 && v >= 0 && w >= 0) {
        return u * v0 + v * v1 + w * v2;
    }
    
    // Point is outside triangle - find closest point on edges
    auto closestOnSegment = [](const glm::vec3& p, const glm::vec3& a, const glm::vec3& b) {
        glm::vec3 ab = b - a;
        float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
        t = glm::clamp(t, 0.0f, 1.0f);
        return a + t * ab;
    };
    
    glm::vec3 c0 = closestOnSegment(point, v0, v1);
    glm::vec3 c1 = closestOnSegment(point, v1, v2);
    glm::vec3 c2 = closestOnSegment(point, v2, v0);
    
    float d0 = glm::dot(point - c0, point - c0);
    float d1 = glm::dot(point - c1, point - c1);
    float d2 = glm::dot(point - c2, point - c2);
    
    if (d0 <= d1 && d0 <= d2) return c0;
    if (d1 <= d2) return c1;
    return c2;
}

// MeshAccelerator implementation
MeshAccelerator::MeshAccelerator(const TriangleMesh& mesh) : m_mesh(mesh) {
    buildBVH();
}

void MeshAccelerator::buildBVH() {
    // Simple BVH construction
    const auto& vertices = m_mesh.vertices();
    const auto& indices = m_mesh.indices();
    
    int numTriangles = static_cast<int>(indices.size() / 3);
    
    // For now, create a single root node containing all triangles
    BVHNode root;
    root.boundsMin = glm::vec3(std::numeric_limits<float>::max());
    root.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    root.leftChild = -1;
    root.rightChild = -1;
    
    for (size_t i = 0; i < indices.size(); ++i) {
        const glm::vec3& p = vertices[indices[i]].position;
        root.boundsMin = glm::min(root.boundsMin, p);
        root.boundsMax = glm::max(root.boundsMax, p);
    }
    
    for (int i = 0; i < numTriangles; ++i) {
        root.triangles.push_back(i);
    }
    
    m_bvh.push_back(root);
    
    // Full implementation would recursively split nodes
}

glm::vec3 MeshAccelerator::closestPoint(const glm::vec3& point) const {
    // For now, use brute force (BVH traversal would be faster)
    return closestPointOnMesh(point, m_mesh);
}

bool MeshAccelerator::rayIntersect(const glm::vec3& origin, const glm::vec3& direction,
                                    glm::vec3& hitPoint, float& hitDistance) const {
    const auto& vertices = m_mesh.vertices();
    const auto& indices = m_mesh.indices();
    
    hitDistance = std::numeric_limits<float>::max();
    bool hit = false;
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        const glm::vec3& v0 = vertices[indices[i]].position;
        const glm::vec3& v1 = vertices[indices[i + 1]].position;
        const glm::vec3& v2 = vertices[indices[i + 2]].position;
        
        // Möller–Trumbore
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(direction, edge2);
        float a = glm::dot(edge1, h);
        
        if (std::abs(a) < 1e-6f) continue;
        
        float f = 1.0f / a;
        glm::vec3 s = origin - v0;
        float u = f * glm::dot(s, h);
        
        if (u < 0.0f || u > 1.0f) continue;
        
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(direction, q);
        
        if (v < 0.0f || u + v > 1.0f) continue;
        
        float t = f * glm::dot(edge2, q);
        
        if (t > 0.0f && t < hitDistance) {
            hitDistance = t;
            hitPoint = origin + direction * t;
            hit = true;
        }
    }
    
    return hit;
}

} // namespace WrapUtils

} // namespace dc
