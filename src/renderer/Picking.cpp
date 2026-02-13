/**
 * @file Picking.cpp
 * @brief Implementation of ray casting and picking utilities
 */

#include "Picking.h"
#include "Camera.h"
#include "../geometry/MeshData.h"

#include <QMatrix4x4>
#include <algorithm>
#include <cmath>

namespace dc3d {
namespace renderer {

Picking::Picking()
{
}

// ============================================================================
// Mesh Management
// ============================================================================

void Picking::addMesh(uint32_t meshId, const geometry::MeshData* mesh,
                      const glm::mat4& transform)
{
    // Check if mesh already exists
    for (auto& m : m_meshes) {
        if (m.meshId == meshId) {
            m.mesh = mesh;
            m.transform = transform;
            m.inverseTransform = glm::inverse(transform);
            m.bvh = std::make_shared<geometry::BVH>(*mesh);
            return;
        }
    }
    
    // Add new mesh
    PickableMesh pm;
    pm.meshId = meshId;
    pm.mesh = mesh;
    pm.transform = transform;
    pm.inverseTransform = glm::inverse(transform);
    pm.bvh = std::make_shared<geometry::BVH>(*mesh);
    pm.visible = true;
    
    m_meshes.push_back(std::move(pm));
}

void Picking::updateTransform(uint32_t meshId, const glm::mat4& transform)
{
    for (auto& m : m_meshes) {
        if (m.meshId == meshId) {
            m.transform = transform;
            m.inverseTransform = glm::inverse(transform);
            return;
        }
    }
}

void Picking::setMeshVisible(uint32_t meshId, bool visible)
{
    for (auto& m : m_meshes) {
        if (m.meshId == meshId) {
            m.visible = visible;
            return;
        }
    }
}

void Picking::removeMesh(uint32_t meshId)
{
    m_meshes.erase(
        std::remove_if(m_meshes.begin(), m_meshes.end(),
                      [meshId](const PickableMesh& m) { return m.meshId == meshId; }),
        m_meshes.end());
}

void Picking::rebuildBVH(uint32_t meshId)
{
    for (auto& m : m_meshes) {
        if (m.meshId == meshId && m.mesh) {
            m.bvh = std::make_shared<geometry::BVH>(*m.mesh);
            return;
        }
    }
}

void Picking::clear()
{
    m_meshes.clear();
}

// ============================================================================
// Ray Generation
// ============================================================================

glm::mat4 Picking::toGlm(const QMatrix4x4& m)
{
    glm::mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = m(j, i);  // QMatrix4x4 uses row-major
        }
    }
    return result;
}

geometry::Ray Picking::screenToRay(const QPoint& screenPos,
                                   const QSize& viewportSize,
                                   const QMatrix4x4& viewMatrix,
                                   const QMatrix4x4& projMatrix)
{
    // Convert screen coords to NDC
    float x = (2.0f * screenPos.x()) / viewportSize.width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / viewportSize.height();  // Flip Y
    
    // Create clip-space points on near and far planes
    QVector4D nearClip(x, y, -1.0f, 1.0f);
    QVector4D farClip(x, y, 1.0f, 1.0f);
    
    // Get inverse view-projection matrix
    QMatrix4x4 invVP = (projMatrix * viewMatrix).inverted();
    
    // Unproject to world space
    QVector4D nearWorld = invVP * nearClip;
    QVector4D farWorld = invVP * farClip;
    
    // Perspective divide
    if (std::abs(nearWorld.w()) > 1e-10f) {
        nearWorld /= nearWorld.w();
    }
    if (std::abs(farWorld.w()) > 1e-10f) {
        farWorld /= farWorld.w();
    }
    
    glm::vec3 origin(nearWorld.x(), nearWorld.y(), nearWorld.z());
    glm::vec3 farPoint(farWorld.x(), farWorld.y(), farWorld.z());
    glm::vec3 direction = glm::normalize(farPoint - origin);
    
    geometry::Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    
    return ray;
}

geometry::Ray Picking::screenToRay(const QPoint& screenPos,
                                   const QSize& viewportSize,
                                   const dc::Camera& camera)
{
    return screenToRay(screenPos, viewportSize,
                      camera.viewMatrix(), camera.projectionMatrix());
}

geometry::Ray Picking::transformRay(const geometry::Ray& ray,
                                    const glm::mat4& inverseTransform)
{
    geometry::Ray localRay;
    
    // Transform origin as a point
    glm::vec4 origin4 = inverseTransform * glm::vec4(ray.origin, 1.0f);
    localRay.origin = glm::vec3(origin4);
    
    // Transform direction as a vector (no translation)
    glm::vec4 dir4 = inverseTransform * glm::vec4(ray.direction, 0.0f);
    localRay.direction = glm::normalize(glm::vec3(dir4));
    
    localRay.tMin = ray.tMin;
    localRay.tMax = ray.tMax;
    
    return localRay;
}

// ============================================================================
// Single Pick
// ============================================================================

core::HitInfo Picking::pick(const QPoint& screenPos,
                            const QSize& viewportSize,
                            const dc::Camera& camera) const
{
    geometry::Ray worldRay = screenToRay(screenPos, viewportSize, camera);
    return pick(worldRay);
}

core::HitInfo Picking::pick(const geometry::Ray& worldRay) const
{
    core::HitInfo bestHit;
    bestHit.hit = false;
    bestHit.distance = std::numeric_limits<float>::max();
    
    for (const auto& pm : m_meshes) {
        if (!pm.visible || !pm.bvh || !pm.bvh->isValid()) {
            continue;
        }
        
        // Transform ray to mesh local space
        geometry::Ray localRay = transformRay(worldRay, pm.inverseTransform);
        
        // Test intersection
        geometry::BVHHitResult bvhHit = pm.bvh->intersect(localRay);
        
        if (bvhHit.hit) {
            // Transform hit point to world space
            glm::vec4 worldPoint4 = pm.transform * glm::vec4(bvhHit.point, 1.0f);
            glm::vec3 worldPoint(worldPoint4);
            
            // Transform normal to world space
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(pm.transform)));
            glm::vec3 worldNormal = glm::normalize(normalMatrix * bvhHit.normal);
            
            // Calculate world-space distance
            float worldDist = glm::length(worldPoint - worldRay.origin);
            
            if (worldDist < bestHit.distance) {
                bestHit.hit = true;
                bestHit.meshId = pm.meshId;
                bestHit.faceIndex = bvhHit.faceIndex;
                bestHit.hitPoint = worldPoint;
                bestHit.hitNormal = worldNormal;
                bestHit.barycentricCoords = bvhHit.barycentric;
                bestHit.distance = worldDist;
                bestHit.vertexIndices[0] = bvhHit.indices[0];
                bestHit.vertexIndices[1] = bvhHit.indices[1];
                bestHit.vertexIndices[2] = bvhHit.indices[2];
                
                // Enhance with closest vertex/edge (requires mesh data)
                if (pm.mesh) {
                    enhanceHitInfo(bestHit, *pm.mesh);
                }
            }
        }
    }
    
    return bestHit;
}

// ============================================================================
// Box Selection
// ============================================================================

void Picking::buildSelectionFrustum(const QRect& rect,
                                    const QSize& viewportSize,
                                    const QMatrix4x4& viewMatrix,
                                    const QMatrix4x4& projMatrix,
                                    glm::vec4 planes[6]) const
{
    // Get the 4 corners of the selection rect in NDC
    float x0 = (2.0f * rect.left()) / viewportSize.width() - 1.0f;
    float x1 = (2.0f * rect.right()) / viewportSize.width() - 1.0f;
    float y0 = 1.0f - (2.0f * rect.bottom()) / viewportSize.height();
    float y1 = 1.0f - (2.0f * rect.top()) / viewportSize.height();
    
    // Get view-projection matrix
    QMatrix4x4 vp = projMatrix * viewMatrix;
    glm::mat4 glmVP = toGlm(vp);
    
    // Extract 6 frustum planes from the modified VP matrix
    // We'll modify the projection to match our selection rect
    
    // For a full-screen frustum: planes from rows of VP matrix
    // For selection rect: we need custom planes
    
    // Left plane (x >= x0)
    planes[0] = glm::vec4(
        glmVP[0][3] + glmVP[0][0] - x0 * (glmVP[0][3]),
        glmVP[1][3] + glmVP[1][0] - x0 * (glmVP[1][3]),
        glmVP[2][3] + glmVP[2][0] - x0 * (glmVP[2][3]),
        glmVP[3][3] + glmVP[3][0] - x0 * (glmVP[3][3])
    );
    
    // Right plane (x <= x1)
    planes[1] = glm::vec4(
        glmVP[0][3] - glmVP[0][0] + x1 * (glmVP[0][3]),
        glmVP[1][3] - glmVP[1][0] + x1 * (glmVP[1][3]),
        glmVP[2][3] - glmVP[2][0] + x1 * (glmVP[2][3]),
        glmVP[3][3] - glmVP[3][0] + x1 * (glmVP[3][3])
    );
    
    // Bottom plane (y >= y0)
    planes[2] = glm::vec4(
        glmVP[0][3] + glmVP[0][1] - y0 * (glmVP[0][3]),
        glmVP[1][3] + glmVP[1][1] - y0 * (glmVP[1][3]),
        glmVP[2][3] + glmVP[2][1] - y0 * (glmVP[2][3]),
        glmVP[3][3] + glmVP[3][1] - y0 * (glmVP[3][3])
    );
    
    // Top plane (y <= y1)
    planes[3] = glm::vec4(
        glmVP[0][3] - glmVP[0][1] + y1 * (glmVP[0][3]),
        glmVP[1][3] - glmVP[1][1] + y1 * (glmVP[1][3]),
        glmVP[2][3] - glmVP[2][1] + y1 * (glmVP[2][3]),
        glmVP[3][3] - glmVP[3][1] + y1 * (glmVP[3][3])
    );
    
    // Near plane
    planes[4] = glm::vec4(
        glmVP[0][3] + glmVP[0][2],
        glmVP[1][3] + glmVP[1][2],
        glmVP[2][3] + glmVP[2][2],
        glmVP[3][3] + glmVP[3][2]
    );
    
    // Far plane
    planes[5] = glm::vec4(
        glmVP[0][3] - glmVP[0][2],
        glmVP[1][3] - glmVP[1][2],
        glmVP[2][3] - glmVP[2][2],
        glmVP[3][3] - glmVP[3][2]
    );
    
    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(glm::vec3(planes[i]));
        if (len > 1e-10f) {
            planes[i] /= len;
        }
    }
}

std::vector<core::SelectionElement> Picking::boxSelect(
    const QRect& rect,
    const QSize& viewportSize,
    const dc::Camera& camera,
    core::SelectionMode mode) const
{
    std::vector<core::SelectionElement> results;
    
    if (!rect.isValid() || rect.width() < 2 || rect.height() < 2) {
        return results;
    }
    
    // Build selection frustum
    glm::vec4 planes[6];
    buildSelectionFrustum(rect, viewportSize,
                         camera.viewMatrix(), camera.projectionMatrix(),
                         planes);
    
    for (const auto& pm : m_meshes) {
        if (!pm.visible || !pm.bvh || !pm.bvh->isValid() || !pm.mesh) {
            continue;
        }
        
        // Transform frustum planes to mesh local space
        glm::vec4 localPlanes[6];
        glm::mat4 invTranspose = glm::transpose(pm.inverseTransform);
        
        for (int i = 0; i < 6; ++i) {
            localPlanes[i] = invTranspose * planes[i];
            // Re-normalize
            float len = glm::length(glm::vec3(localPlanes[i]));
            if (len > 1e-10f) {
                localPlanes[i] /= len;
            }
        }
        
        if (mode == core::SelectionMode::Object) {
            // Check if mesh bounds intersect frustum
            const auto& bounds = pm.bvh->bounds();
            
            bool inside = true;
            for (int i = 0; i < 6 && inside; ++i) {
                glm::vec3 normal(localPlanes[i].x, localPlanes[i].y, localPlanes[i].z);
                float d = localPlanes[i].w;
                
                // Find p-vertex
                glm::vec3 p = bounds.min;
                if (normal.x >= 0) p.x = bounds.max.x;
                if (normal.y >= 0) p.y = bounds.max.y;
                if (normal.z >= 0) p.z = bounds.max.z;
                
                if (glm::dot(normal, p) + d < 0) {
                    inside = false;
                }
            }
            
            if (inside) {
                core::SelectionElement elem;
                elem.meshId = pm.meshId;
                elem.elementIndex = 0;
                elem.mode = core::SelectionMode::Object;
                results.push_back(elem);
            }
        } else {
            // Query BVH for faces in frustum
            std::vector<uint32_t> faceIndices = pm.bvh->queryFrustum(localPlanes);
            
            if (mode == core::SelectionMode::Face) {
                for (uint32_t faceIdx : faceIndices) {
                    core::SelectionElement elem;
                    elem.meshId = pm.meshId;
                    elem.elementIndex = faceIdx;
                    elem.mode = core::SelectionMode::Face;
                    results.push_back(elem);
                }
            } else if (mode == core::SelectionMode::Vertex) {
                // Collect unique vertices from selected faces
                std::set<uint32_t> vertexSet;
                const auto& indices = pm.mesh->indices();
                
                for (uint32_t faceIdx : faceIndices) {
                    vertexSet.insert(indices[faceIdx * 3 + 0]);
                    vertexSet.insert(indices[faceIdx * 3 + 1]);
                    vertexSet.insert(indices[faceIdx * 3 + 2]);
                }
                
                for (uint32_t vIdx : vertexSet) {
                    core::SelectionElement elem;
                    elem.meshId = pm.meshId;
                    elem.elementIndex = vIdx;
                    elem.mode = core::SelectionMode::Vertex;
                    results.push_back(elem);
                }
            } else if (mode == core::SelectionMode::Edge) {
                // Collect unique edges from selected faces
                std::set<uint64_t> edgeSet;
                const auto& indices = pm.mesh->indices();
                
                auto makeEdgeKey = [](uint32_t v1, uint32_t v2) -> uint64_t {
                    if (v1 > v2) std::swap(v1, v2);
                    return (static_cast<uint64_t>(v2) << 32) | v1;
                };
                
                for (uint32_t faceIdx : faceIndices) {
                    uint32_t i0 = indices[faceIdx * 3 + 0];
                    uint32_t i1 = indices[faceIdx * 3 + 1];
                    uint32_t i2 = indices[faceIdx * 3 + 2];
                    
                    edgeSet.insert(makeEdgeKey(i0, i1));
                    edgeSet.insert(makeEdgeKey(i1, i2));
                    edgeSet.insert(makeEdgeKey(i2, i0));
                }
                
                for (uint64_t edgeKey : edgeSet) {
                    // Fix: Use 32-bit masks to match the 32-bit shift in makeEdgeKey
                    // The edge key is packed as: (v2 << 32) | v1
                    uint32_t v1 = static_cast<uint32_t>(edgeKey & 0xFFFFFFFF);
                    uint32_t v2 = static_cast<uint32_t>(edgeKey >> 32);
                    
                    core::SelectionElement elem;
                    elem.meshId = pm.meshId;
                    // Note: elementIndex is 32-bit, so we pack with 16-bit for storage
                    // This limits edge selection to meshes with <65536 vertices
                    elem.elementIndex = (v2 << 16) | (v1 & 0xFFFF);
                    elem.mode = core::SelectionMode::Edge;
                    results.push_back(elem);
                }
            }
        }
    }
    
    return results;
}

// ============================================================================
// Hit Processing
// ============================================================================

uint32_t Picking::findClosestVertex(const core::HitInfo& hit)
{
    // Use barycentric coordinates to find closest vertex
    const glm::vec3& bary = hit.barycentricCoords;
    
    if (bary.x >= bary.y && bary.x >= bary.z) {
        return hit.vertexIndices[0];
    } else if (bary.y >= bary.z) {
        return hit.vertexIndices[1];
    } else {
        return hit.vertexIndices[2];
    }
}

int Picking::findClosestEdge(const core::HitInfo& hit)
{
    const glm::vec3& bary = hit.barycentricCoords;
    
    // Edges are: 0-1 (edge 0), 1-2 (edge 1), 2-0 (edge 2)
    // Distance from edge is proportional to the opposite barycentric coord
    // Edge 0-1: opposite is vertex 2, bary.z
    // Edge 1-2: opposite is vertex 0, bary.x  
    // Edge 2-0: opposite is vertex 1, bary.y
    
    if (bary.z <= bary.x && bary.z <= bary.y) {
        return 0;  // Edge 0-1
    } else if (bary.x <= bary.y) {
        return 1;  // Edge 1-2
    } else {
        return 2;  // Edge 2-0
    }
}

void Picking::enhanceHitInfo(core::HitInfo& hit, const geometry::MeshData& mesh)
{
    hit.closestVertex = findClosestVertex(hit);
    hit.closestEdge = findClosestEdge(hit);
}

// ============================================================================
// BoxSelector
// ============================================================================

void BoxSelector::begin(const QPoint& pos)
{
    m_active = true;
    m_startPos = pos;
    m_currentPos = pos;
}

void BoxSelector::update(const QPoint& pos)
{
    if (m_active) {
        m_currentPos = pos;
    }
}

void BoxSelector::end()
{
    m_active = false;
}

void BoxSelector::cancel()
{
    m_active = false;
    m_startPos = QPoint();
    m_currentPos = QPoint();
}

QRect BoxSelector::rect() const
{
    int x1 = std::min(m_startPos.x(), m_currentPos.x());
    int y1 = std::min(m_startPos.y(), m_currentPos.y());
    int x2 = std::max(m_startPos.x(), m_currentPos.x());
    int y2 = std::max(m_startPos.y(), m_currentPos.y());
    
    return QRect(QPoint(x1, y1), QPoint(x2, y2));
}

bool BoxSelector::isValidSelection(int minSize) const
{
    QRect r = rect();
    return r.width() >= minSize || r.height() >= minSize;
}

} // namespace renderer
} // namespace dc3d
