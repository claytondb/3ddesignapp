/**
 * @file SnapManager.cpp
 * @brief Implementation of SnapManager for precise positioning
 */

#include "SnapManager.h"
#include "geometry/MeshData.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dc3d {
namespace core {

SnapManager::SnapManager(QObject* parent)
    : QObject(parent)
{
}

void SnapManager::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(enabled);
    }
}

void SnapManager::toggleEnabled()
{
    setEnabled(!m_enabled);
}

void SnapManager::setGridSnapEnabled(bool enabled)
{
    if (m_settings.gridSnapEnabled != enabled) {
        m_settings.gridSnapEnabled = enabled;
        emit settingsChanged();
    }
}

void SnapManager::setObjectSnapEnabled(bool enabled)
{
    if (m_settings.objectSnapEnabled != enabled) {
        m_settings.objectSnapEnabled = enabled;
        emit settingsChanged();
    }
}

void SnapManager::setGridSize(float size)
{
    if (size > 0.0f && m_settings.gridSize != size) {
        m_settings.gridSize = size;
        emit settingsChanged();
    }
}

void SnapManager::setSnapTolerance(float pixels)
{
    if (pixels > 0.0f && m_settings.snapTolerance != pixels) {
        m_settings.snapTolerance = pixels;
        emit settingsChanged();
    }
}

glm::vec3 SnapManager::snapToGrid(const glm::vec3& point) const
{
    float gridStep = m_settings.effectiveGridSize();
    
    return glm::vec3(
        std::round(point.x / gridStep) * gridStep,
        std::round(point.y / gridStep) * gridStep,
        std::round(point.z / gridStep) * gridStep
    );
}

SnapResult SnapManager::findSnapTarget(const glm::vec3& point,
                                        uint64_t excludeMeshId) const
{
    if (!m_enabled) {
        return SnapResult{};
    }
    
    SnapResult bestResult;
    float bestDist = std::numeric_limits<float>::max();
    
    // Try object snapping first (higher priority)
    if (m_settings.objectSnapEnabled) {
        if (m_settings.snapToVertices) {
            auto vertexSnap = findVertexSnap(point, excludeMeshId);
            if (vertexSnap && vertexSnap.distance < bestDist &&
                vertexSnap.distance < m_settings.worldTolerance) {
                bestResult = vertexSnap;
                bestDist = vertexSnap.distance;
            }
        }
        
        if (m_settings.snapToEdgeMidpoints) {
            auto edgeSnap = findEdgeSnap(point, excludeMeshId);
            if (edgeSnap && edgeSnap.distance < bestDist &&
                edgeSnap.distance < m_settings.worldTolerance) {
                bestResult = edgeSnap;
                bestDist = edgeSnap.distance;
            }
        }
        
        if (m_settings.snapToFaceCenters) {
            auto faceSnap = findFaceSnap(point, excludeMeshId);
            if (faceSnap && faceSnap.distance < bestDist &&
                faceSnap.distance < m_settings.worldTolerance) {
                bestResult = faceSnap;
                bestDist = faceSnap.distance;
            }
        }
    }
    
    // If object snap found, use it
    if (bestResult.snapped) {
        m_activeSnap = bestResult;
        return bestResult;
    }
    
    // Fall back to grid snapping
    if (m_settings.gridSnapEnabled) {
        glm::vec3 gridPoint = snapToGrid(point);
        float gridDist = glm::length(gridPoint - point);
        
        if (gridDist < m_settings.worldTolerance) {
            bestResult.snapped = true;
            bestResult.type = SnapType::Grid;
            bestResult.position = gridPoint;
            bestResult.distance = gridDist;
        }
    }
    
    m_activeSnap = bestResult;
    return bestResult;
}

SnapResult SnapManager::findSnapTarget(const glm::vec3& point,
                                        const glm::vec2& screenPos,
                                        const glm::mat4& viewMatrix,
                                        const glm::mat4& projMatrix,
                                        const glm::vec2& viewportSize,
                                        uint64_t excludeMeshId) const
{
    if (!m_enabled) {
        return SnapResult{};
    }
    
    SnapResult bestResult;
    float bestScreenDist = std::numeric_limits<float>::max();
    
    auto worldToScreen = [&](const glm::vec3& worldPos) -> glm::vec2 {
        glm::vec4 clip = projMatrix * viewMatrix * glm::vec4(worldPos, 1.0f);
        if (std::abs(clip.w) < 1e-6f) return glm::vec2(-1e6f);
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        return glm::vec2(
            (ndc.x * 0.5f + 0.5f) * viewportSize.x,
            (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y
        );
    };
    
    // Check object snap points
    if (m_settings.objectSnapEnabled) {
        for (const auto& regMesh : m_meshes) {
            if (regMesh.id == excludeMeshId) continue;
            
            // Check vertices
            if (m_settings.snapToVertices) {
                for (size_t i = 0; i < regMesh.vertices.size(); ++i) {
                    glm::vec2 sp = worldToScreen(regMesh.vertices[i]);
                    float screenDist = glm::length(sp - screenPos);
                    
                    if (screenDist < m_settings.snapTolerance &&
                        screenDist < bestScreenDist) {
                        bestResult.snapped = true;
                        bestResult.type = SnapType::Vertex;
                        bestResult.position = regMesh.vertices[i];
                        bestResult.meshId = regMesh.id;
                        bestResult.elementIndex = static_cast<uint32_t>(i);
                        bestResult.distance = glm::length(regMesh.vertices[i] - point);
                        bestScreenDist = screenDist;
                    }
                }
            }
            
            // Check edge midpoints
            if (m_settings.snapToEdgeMidpoints) {
                for (size_t i = 0; i < regMesh.edgeMidpoints.size(); ++i) {
                    glm::vec2 sp = worldToScreen(regMesh.edgeMidpoints[i]);
                    float screenDist = glm::length(sp - screenPos);
                    
                    if (screenDist < m_settings.snapTolerance &&
                        screenDist < bestScreenDist) {
                        bestResult.snapped = true;
                        bestResult.type = SnapType::EdgeMid;
                        bestResult.position = regMesh.edgeMidpoints[i];
                        bestResult.meshId = regMesh.id;
                        bestResult.elementIndex = static_cast<uint32_t>(i);
                        bestResult.distance = glm::length(regMesh.edgeMidpoints[i] - point);
                        bestScreenDist = screenDist;
                    }
                }
            }
            
            // Check face centers
            if (m_settings.snapToFaceCenters) {
                for (size_t i = 0; i < regMesh.faceCenters.size(); ++i) {
                    glm::vec2 sp = worldToScreen(regMesh.faceCenters[i]);
                    float screenDist = glm::length(sp - screenPos);
                    
                    if (screenDist < m_settings.snapTolerance &&
                        screenDist < bestScreenDist) {
                        bestResult.snapped = true;
                        bestResult.type = SnapType::FaceCenter;
                        bestResult.position = regMesh.faceCenters[i];
                        bestResult.meshId = regMesh.id;
                        bestResult.elementIndex = static_cast<uint32_t>(i);
                        bestResult.distance = glm::length(regMesh.faceCenters[i] - point);
                        bestScreenDist = screenDist;
                    }
                }
            }
            
            // Check origin
            if (m_settings.snapToOrigins) {
                glm::vec2 sp = worldToScreen(regMesh.origin);
                float screenDist = glm::length(sp - screenPos);
                
                if (screenDist < m_settings.snapTolerance &&
                    screenDist < bestScreenDist) {
                    bestResult.snapped = true;
                    bestResult.type = SnapType::Origin;
                    bestResult.position = regMesh.origin;
                    bestResult.meshId = regMesh.id;
                    bestResult.distance = glm::length(regMesh.origin - point);
                    bestScreenDist = screenDist;
                }
            }
        }
    }
    
    // If object snap found, use it
    if (bestResult.snapped) {
        m_activeSnap = bestResult;
        return bestResult;
    }
    
    // Fall back to grid snapping
    if (m_settings.gridSnapEnabled) {
        glm::vec3 gridPoint = snapToGrid(point);
        glm::vec2 gridScreen = worldToScreen(gridPoint);
        float gridScreenDist = glm::length(gridScreen - screenPos);
        
        // Grid snap uses world tolerance, not screen
        float gridWorldDist = glm::length(gridPoint - point);
        if (gridWorldDist < m_settings.worldTolerance * 2.0f) {
            bestResult.snapped = true;
            bestResult.type = SnapType::Grid;
            bestResult.position = gridPoint;
            bestResult.distance = gridWorldDist;
        }
    }
    
    m_activeSnap = bestResult;
    return bestResult;
}

glm::vec3 SnapManager::snap(const glm::vec3& point, uint64_t excludeMeshId) const
{
    SnapResult result = findSnapTarget(point, excludeMeshId);
    return result.snapped ? result.position : point;
}

std::vector<SnapCandidate> SnapManager::findSnapCandidates(
    const glm::vec3& point,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const glm::vec2& viewportSize,
    size_t maxCandidates) const
{
    std::vector<SnapCandidate> candidates;
    
    if (!m_enabled) return candidates;
    
    auto worldToScreen = [&](const glm::vec3& worldPos) -> glm::vec2 {
        glm::vec4 clip = projMatrix * viewMatrix * glm::vec4(worldPos, 1.0f);
        if (std::abs(clip.w) < 1e-6f) return glm::vec2(-1e6f);
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        return glm::vec2(
            (ndc.x * 0.5f + 0.5f) * viewportSize.x,
            (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y
        );
    };
    
    glm::vec2 screenPos = worldToScreen(point);
    
    // Collect candidates from all meshes
    for (const auto& regMesh : m_meshes) {
        if (m_settings.snapToVertices) {
            for (const auto& v : regMesh.vertices) {
                glm::vec2 sp = worldToScreen(v);
                float dist = glm::length(sp - screenPos);
                if (dist < m_settings.snapTolerance * 2.0f) {
                    candidates.push_back({SnapType::Vertex, v, glm::vec3(0,1,0), 
                                         regMesh.id, dist});
                }
            }
        }
        
        if (m_settings.snapToEdgeMidpoints) {
            for (const auto& e : regMesh.edgeMidpoints) {
                glm::vec2 sp = worldToScreen(e);
                float dist = glm::length(sp - screenPos);
                if (dist < m_settings.snapTolerance * 2.0f) {
                    candidates.push_back({SnapType::EdgeMid, e, glm::vec3(0,1,0),
                                         regMesh.id, dist});
                }
            }
        }
        
        if (m_settings.snapToFaceCenters) {
            for (const auto& f : regMesh.faceCenters) {
                glm::vec2 sp = worldToScreen(f);
                float dist = glm::length(sp - screenPos);
                if (dist < m_settings.snapTolerance * 2.0f) {
                    candidates.push_back({SnapType::FaceCenter, f, glm::vec3(0,1,0),
                                         regMesh.id, dist});
                }
            }
        }
    }
    
    // Sort by screen distance
    std::sort(candidates.begin(), candidates.end(),
              [](const SnapCandidate& a, const SnapCandidate& b) {
                  return a.screenDistance < b.screenDistance;
              });
    
    // Limit results
    if (candidates.size() > maxCandidates) {
        candidates.resize(maxCandidates);
    }
    
    return candidates;
}

void SnapManager::registerMesh(uint64_t id,
                                std::shared_ptr<geometry::MeshData> mesh,
                                const glm::mat4& transform)
{
    // Remove existing if present
    unregisterMesh(id);
    
    RegisteredMesh regMesh;
    regMesh.id = id;
    regMesh.mesh = mesh;
    regMesh.transform = transform;
    
    rebuildSnapCache(regMesh);
    
    m_meshes.push_back(std::move(regMesh));
}

void SnapManager::updateMeshTransform(uint64_t id, const glm::mat4& transform)
{
    for (auto& regMesh : m_meshes) {
        if (regMesh.id == id) {
            regMesh.transform = transform;
            rebuildSnapCache(regMesh);
            break;
        }
    }
}

void SnapManager::unregisterMesh(uint64_t id)
{
    m_meshes.erase(
        std::remove_if(m_meshes.begin(), m_meshes.end(),
                       [id](const RegisteredMesh& rm) { return rm.id == id; }),
        m_meshes.end()
    );
}

void SnapManager::clearMeshes()
{
    m_meshes.clear();
}

void SnapManager::rebuildSnapCache(RegisteredMesh& regMesh)
{
    if (!regMesh.mesh) return;
    
    const auto& mesh = *regMesh.mesh;
    const auto& transform = regMesh.transform;
    
    // Transform vertices
    regMesh.vertices.clear();
    regMesh.vertices.reserve(mesh.vertexCount());
    
    const auto& verts = mesh.vertices();
    for (size_t i = 0; i < verts.size(); ++i) {
        glm::vec4 worldVert = transform * glm::vec4(verts[i], 1.0f);
        regMesh.vertices.push_back(glm::vec3(worldVert));
    }
    
    // Compute edge midpoints and face centers from triangles
    regMesh.edgeMidpoints.clear();
    regMesh.faceCenters.clear();
    
    const auto& indices = mesh.indices();
    size_t faceCount = indices.size() / 3;
    
    regMesh.faceCenters.reserve(faceCount);
    // Rough estimate: 3 edges per face, but shared, so roughly 1.5x faces
    regMesh.edgeMidpoints.reserve(faceCount * 2);
    
    // Track edges we've seen (to avoid duplicates)
    // Simple approach: just collect all midpoints, duplicates will be near each other
    std::vector<std::pair<uint32_t, uint32_t>> seenEdges;
    
    for (size_t f = 0; f < faceCount; ++f) {
        uint32_t i0 = indices[f * 3 + 0];
        uint32_t i1 = indices[f * 3 + 1];
        uint32_t i2 = indices[f * 3 + 2];
        
        const glm::vec3& v0 = regMesh.vertices[i0];
        const glm::vec3& v1 = regMesh.vertices[i1];
        const glm::vec3& v2 = regMesh.vertices[i2];
        
        // Face center
        regMesh.faceCenters.push_back((v0 + v1 + v2) / 3.0f);
        
        // Edge midpoints (check for duplicates)
        auto addEdge = [&](uint32_t a, uint32_t b) {
            if (a > b) std::swap(a, b);
            auto edge = std::make_pair(a, b);
            if (std::find(seenEdges.begin(), seenEdges.end(), edge) == seenEdges.end()) {
                seenEdges.push_back(edge);
                regMesh.edgeMidpoints.push_back(
                    (regMesh.vertices[a] + regMesh.vertices[b]) * 0.5f);
            }
        };
        
        addEdge(i0, i1);
        addEdge(i1, i2);
        addEdge(i2, i0);
    }
    
    // Origin (centroid of mesh)
    glm::vec3 origin(0.0f);
    for (const auto& v : regMesh.vertices) {
        origin += v;
    }
    if (!regMesh.vertices.empty()) {
        origin /= static_cast<float>(regMesh.vertices.size());
    }
    regMesh.origin = origin;
}

SnapResult SnapManager::findVertexSnap(const glm::vec3& point,
                                        uint64_t excludeMeshId) const
{
    SnapResult best;
    float bestDist = std::numeric_limits<float>::max();
    
    for (const auto& regMesh : m_meshes) {
        if (regMesh.id == excludeMeshId) continue;
        
        for (size_t i = 0; i < regMesh.vertices.size(); ++i) {
            float dist = glm::length(regMesh.vertices[i] - point);
            if (dist < bestDist) {
                bestDist = dist;
                best.snapped = true;
                best.type = SnapType::Vertex;
                best.position = regMesh.vertices[i];
                best.meshId = regMesh.id;
                best.elementIndex = static_cast<uint32_t>(i);
                best.distance = dist;
            }
        }
    }
    
    return best;
}

SnapResult SnapManager::findEdgeSnap(const glm::vec3& point,
                                      uint64_t excludeMeshId) const
{
    SnapResult best;
    float bestDist = std::numeric_limits<float>::max();
    
    for (const auto& regMesh : m_meshes) {
        if (regMesh.id == excludeMeshId) continue;
        
        for (size_t i = 0; i < regMesh.edgeMidpoints.size(); ++i) {
            float dist = glm::length(regMesh.edgeMidpoints[i] - point);
            if (dist < bestDist) {
                bestDist = dist;
                best.snapped = true;
                best.type = SnapType::EdgeMid;
                best.position = regMesh.edgeMidpoints[i];
                best.meshId = regMesh.id;
                best.elementIndex = static_cast<uint32_t>(i);
                best.distance = dist;
            }
        }
    }
    
    return best;
}

SnapResult SnapManager::findFaceSnap(const glm::vec3& point,
                                      uint64_t excludeMeshId) const
{
    SnapResult best;
    float bestDist = std::numeric_limits<float>::max();
    
    for (const auto& regMesh : m_meshes) {
        if (regMesh.id == excludeMeshId) continue;
        
        for (size_t i = 0; i < regMesh.faceCenters.size(); ++i) {
            float dist = glm::length(regMesh.faceCenters[i] - point);
            if (dist < bestDist) {
                bestDist = dist;
                best.snapped = true;
                best.type = SnapType::FaceCenter;
                best.position = regMesh.faceCenters[i];
                best.meshId = regMesh.id;
                best.elementIndex = static_cast<uint32_t>(i);
                best.distance = dist;
            }
        }
    }
    
    return best;
}

} // namespace core
} // namespace dc3d
