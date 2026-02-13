#include "FreeformTool.h"
#include "../../geometry/freeform/QuadMesh.h"
#include "../../geometry/nurbs/NurbsSurface.h"
#include "../../scene/SceneObject.h"
#include "../../render/Renderer.h"
#include "../imgui/imgui.h"
#include <algorithm>

namespace dc {

FreeformTool::FreeformTool() = default;
FreeformTool::~FreeformTool() = default;

void FreeformTool::activate() {
    Tool::activate();
    m_selectedVertices.clear();
    m_selectedEdges.clear();
    m_selectedFaces.clear();
    m_hoveredVertex = -1;
    m_hoveredEdge = -1;
    m_hoveredFace = -1;
}

void FreeformTool::deactivate() {
    Tool::deactivate();
    m_isDragging = false;
    m_isBoxSelecting = false;
    m_isSculpting = false;
}

void FreeformTool::setEditMode(FreeformEditMode mode) {
    if (m_editMode != mode) {
        m_editMode = mode;
        m_isDragging = false;
        m_isSculpting = false;
        
        // Adjust selection mode based on edit mode
        if (mode == FreeformEditMode::Crease) {
            m_selectMode = FreeformSelectMode::Edge;
        } else if (mode == FreeformEditMode::ExtrudeFace) {
            m_selectMode = FreeformSelectMode::Face;
        }
    }
}

void FreeformTool::setSelectMode(FreeformSelectMode mode) {
    if (m_selectMode != mode) {
        m_selectMode = mode;
        // Clear selection when changing mode
        m_selectedVertices.clear();
        m_selectedEdges.clear();
        m_selectedFaces.clear();
    }
}

void FreeformTool::setSubdivisionLevel(int level) {
    level = std::max(0, std::min(level, 5));
    if (m_subdivisionLevel != level) {
        m_subdivisionLevel = level;
        m_subdividedDirty = true;
    }
}

void FreeformTool::setBrushSettings(const SculptBrushSettings& settings) {
    m_brushSettings = settings;
}

bool FreeformTool::onMousePress(const MouseEvent& event) {
    if (!m_quadMesh) return false;
    
    m_lastMousePos = event.position;
    
    if (event.button == MouseButton::Left) {
        if (m_editMode == FreeformEditMode::Select) {
            if (event.modifiers & ModifierKey::Shift) {
                // Box selection start
                m_isBoxSelecting = true;
                m_boxSelectStart = event.position;
                m_boxSelectEnd = event.position;
            } else {
                // Single element selection
                pickElement(event.position, event.modifiers & ModifierKey::Ctrl);
            }
            return true;
        }
        else if (m_editMode == FreeformEditMode::Move) {
            if (!m_selectedVertices.empty()) {
                beginDrag(event.position);
                return true;
            }
        }
        else if (m_editMode == FreeformEditMode::Sculpt) {
            m_isSculpting = true;
            // Apply initial sculpt
            glm::vec3 worldPos = screenToWorld(event.position, 0.5f);
            glm::vec3 direction = glm::normalize(getCamera()->getPosition() - worldPos);
            applySculptBrush(worldPos, -direction);
            return true;
        }
        else if (m_editMode == FreeformEditMode::Crease) {
            int edge = pickEdge(event.position);
            if (edge >= 0) {
                // Toggle crease on this edge
                const auto& halfEdges = m_quadMesh->halfEdges();
                if (halfEdges[edge].isCrease) {
                    m_quadMesh->setCreaseEdge(
                        halfEdges[halfEdges[edge].prevIdx].vertexIdx,
                        halfEdges[edge].vertexIdx,
                        0.0f
                    );
                } else {
                    m_quadMesh->setCreaseEdge(
                        halfEdges[halfEdges[edge].prevIdx].vertexIdx,
                        halfEdges[edge].vertexIdx,
                        m_creaseWeight
                    );
                }
                m_subdividedDirty = true;
                return true;
            }
        }
        else if (m_editMode == FreeformEditMode::AddControlPoint) {
            // Add control point at clicked location
            glm::vec3 worldPos = screenToWorld(event.position, 0.5f);
            m_quadMesh->addVertex(worldPos);
            m_subdividedDirty = true;
            return true;
        }
    }
    
    return false;
}

bool FreeformTool::onMouseRelease(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        if (m_isBoxSelecting) {
            m_isBoxSelecting = false;
            boxSelect(
                glm::min(m_boxSelectStart, m_boxSelectEnd),
                glm::max(m_boxSelectStart, m_boxSelectEnd),
                event.modifiers & ModifierKey::Ctrl
            );
            return true;
        }
        
        if (m_isDragging) {
            endDrag();
            return true;
        }
        
        if (m_isSculpting) {
            m_isSculpting = false;
            return true;
        }
    }
    
    return false;
}

bool FreeformTool::onMouseMove(const MouseEvent& event) {
    if (!m_quadMesh) return false;
    
    m_lastMousePos = event.position;
    
    // Update hover state
    if (m_selectMode == FreeformSelectMode::Vertex) {
        m_hoveredVertex = pickVertex(event.position);
    } else if (m_selectMode == FreeformSelectMode::Edge || 
               m_selectMode == FreeformSelectMode::EdgeLoop) {
        m_hoveredEdge = pickEdge(event.position);
    } else if (m_selectMode == FreeformSelectMode::Face ||
               m_selectMode == FreeformSelectMode::FaceLoop) {
        m_hoveredFace = pickFace(event.position);
    }
    
    if (m_isBoxSelecting) {
        m_boxSelectEnd = event.position;
        return true;
    }
    
    if (m_isDragging) {
        updateDrag(event.position);
        return true;
    }
    
    if (m_isSculpting) {
        glm::vec3 worldPos = screenToWorld(event.position, 0.5f);
        glm::vec3 direction = glm::normalize(getCamera()->getPosition() - worldPos);
        applySculptBrush(worldPos, -direction);
        return true;
    }
    
    return false;
}

bool FreeformTool::onMouseWheel(const WheelEvent& event) {
    if (m_editMode == FreeformEditMode::Sculpt) {
        // Adjust brush radius
        float factor = (event.delta > 0) ? 1.1f : 0.9f;
        m_brushSettings.radius *= factor;
        m_brushSettings.radius = std::max(0.01f, std::min(m_brushSettings.radius, 10.0f));
        return true;
    }
    return false;
}

bool FreeformTool::onKeyPress(const KeyEvent& event) {
    switch (event.key) {
        case Key::S:
            setEditMode(FreeformEditMode::Select);
            return true;
        case Key::G:
            setEditMode(FreeformEditMode::Move);
            return true;
        case Key::B:
            setEditMode(FreeformEditMode::Sculpt);
            return true;
        case Key::C:
            setEditMode(FreeformEditMode::Crease);
            return true;
        case Key::E:
            if (!m_selectedFaces.empty()) {
                extrudeSelection(0.1f);
            }
            return true;
        case Key::L:
            insertEdgeLoop();
            return true;
        case Key::Delete:
        case Key::Backspace:
            deleteSelection();
            return true;
        case Key::A:
            if (event.modifiers & ModifierKey::Ctrl) {
                selectAll();
            }
            return true;
        case Key::D:
            if (event.modifiers & ModifierKey::Ctrl) {
                selectNone();
            }
            return true;
        case Key::Num1:
            setSelectMode(FreeformSelectMode::Vertex);
            return true;
        case Key::Num2:
            setSelectMode(FreeformSelectMode::Edge);
            return true;
        case Key::Num3:
            setSelectMode(FreeformSelectMode::Face);
            return true;
        case Key::Plus:
        case Key::Equal:
            setSubdivisionLevel(m_subdivisionLevel + 1);
            return true;
        case Key::Minus:
            setSubdivisionLevel(m_subdivisionLevel - 1);
            return true;
        default:
            break;
    }
    return false;
}

bool FreeformTool::onKeyRelease(const KeyEvent& event) {
    return false;
}

void FreeformTool::render(Renderer& renderer) {
    if (!m_quadMesh) return;
    
    // Update subdivided mesh if needed
    if (m_subdividedDirty) {
        updateSubdividedMesh();
    }
    
    // Render subdivided surface
    if (m_subdividedMesh) {
        auto vertexBuffer = m_subdividedMesh->getVertexBuffer();
        auto indexBuffer = m_subdividedMesh->getIndexBuffer();
        renderer.drawMesh(vertexBuffer.data(), indexBuffer.data(),
                          static_cast<int>(indexBuffer.size()));
    }
    
    // Render control mesh if enabled
    if (m_showControlMesh) {
        renderControlMesh(renderer);
    }
    
    // Render control points
    renderControlPoints(renderer);
    
    // Render crease edges
    renderCreaseEdges(renderer);
    
    // Render selection
    renderSelection(renderer);
}

void FreeformTool::renderOverlay(Renderer& renderer) {
    // Render box selection rectangle
    if (m_isBoxSelecting) {
        renderer.drawRect2D(m_boxSelectStart, m_boxSelectEnd, 
                           glm::vec4(0.2f, 0.5f, 1.0f, 0.3f));
        renderer.drawRectOutline2D(m_boxSelectStart, m_boxSelectEnd,
                                   glm::vec4(0.2f, 0.5f, 1.0f, 1.0f));
    }
    
    // Render brush cursor in sculpt mode
    if (m_editMode == FreeformEditMode::Sculpt) {
        renderBrushCursor(renderer);
    }
}

void FreeformTool::updateSubdividedMesh() {
    if (!m_quadMesh) {
        m_subdividedMesh = nullptr;
        return;
    }
    
    if (m_subdivisionLevel > 0) {
        m_subdividedMesh = m_quadMesh->subdivide(m_subdivisionLevel);
        m_subdividedMesh->computeLimitPositions();
        m_subdividedMesh->computeLimitNormals();
    } else {
        m_subdividedMesh = nullptr;
    }
    
    m_subdividedDirty = false;
}

void FreeformTool::pickElement(const glm::vec2& screenPos, bool addToSelection) {
    if (!addToSelection) {
        m_selectedVertices.clear();
        m_selectedEdges.clear();
        m_selectedFaces.clear();
    }
    
    switch (m_selectMode) {
        case FreeformSelectMode::Vertex: {
            int idx = pickVertex(screenPos);
            if (idx >= 0) {
                auto it = std::find(m_selectedVertices.begin(), m_selectedVertices.end(), idx);
                if (it != m_selectedVertices.end()) {
                    m_selectedVertices.erase(it);
                } else {
                    m_selectedVertices.push_back(idx);
                }
            }
            break;
        }
        case FreeformSelectMode::Edge: {
            int idx = pickEdge(screenPos);
            if (idx >= 0) {
                auto it = std::find(m_selectedEdges.begin(), m_selectedEdges.end(), idx);
                if (it != m_selectedEdges.end()) {
                    m_selectedEdges.erase(it);
                } else {
                    m_selectedEdges.push_back(idx);
                }
            }
            break;
        }
        case FreeformSelectMode::Face: {
            int idx = pickFace(screenPos);
            if (idx >= 0) {
                auto it = std::find(m_selectedFaces.begin(), m_selectedFaces.end(), idx);
                if (it != m_selectedFaces.end()) {
                    m_selectedFaces.erase(it);
                } else {
                    m_selectedFaces.push_back(idx);
                }
            }
            break;
        }
        case FreeformSelectMode::EdgeLoop: {
            int idx = pickEdge(screenPos);
            if (idx >= 0 && m_quadMesh) {
                m_quadMesh->selectEdgeLoop(idx);
                m_selectedVertices = m_quadMesh->getSelectedVertices();
            }
            break;
        }
        case FreeformSelectMode::FaceLoop: {
            int idx = pickEdge(screenPos);
            if (idx >= 0 && m_quadMesh) {
                m_quadMesh->selectFaceLoop(idx);
                m_selectedFaces = m_quadMesh->getSelectedFaces();
            }
            break;
        }
    }
}

void FreeformTool::boxSelect(const glm::vec2& min, const glm::vec2& max, bool addToSelection) {
    if (!m_quadMesh) return;
    
    if (!addToSelection) {
        m_selectedVertices.clear();
    }
    
    const auto& vertices = m_quadMesh->vertices();
    
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        glm::vec2 screenPos = worldToScreen(vertices[i].position);
        
        if (screenPos.x >= min.x && screenPos.x <= max.x &&
            screenPos.y >= min.y && screenPos.y <= max.y) {
            if (std::find(m_selectedVertices.begin(), m_selectedVertices.end(), i) == 
                m_selectedVertices.end()) {
                m_selectedVertices.push_back(i);
            }
        }
    }
}

int FreeformTool::pickVertex(const glm::vec2& screenPos) const {
    if (!m_quadMesh) return -1;
    
    const float pickRadius = 10.0f;
    float minDist = pickRadius;
    int closest = -1;
    
    const auto& vertices = m_quadMesh->vertices();
    
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        glm::vec2 vertScreenPos = worldToScreen(vertices[i].position);
        float dist = glm::length(vertScreenPos - screenPos);
        
        if (dist < minDist) {
            minDist = dist;
            closest = i;
        }
    }
    
    return closest;
}

int FreeformTool::pickEdge(const glm::vec2& screenPos) const {
    if (!m_quadMesh) return -1;
    
    const float pickRadius = 8.0f;
    float minDist = pickRadius;
    int closest = -1;
    
    const auto& vertices = m_quadMesh->vertices();
    const auto& halfEdges = m_quadMesh->halfEdges();
    
    for (int i = 0; i < static_cast<int>(halfEdges.size()); ++i) {
        // Skip duplicate edges (only process one half-edge per edge)
        if (halfEdges[i].twinIdx != -1 && halfEdges[i].twinIdx < i) continue;
        
        int v0 = halfEdges[halfEdges[i].prevIdx].vertexIdx;
        int v1 = halfEdges[i].vertexIdx;
        
        glm::vec2 p0 = worldToScreen(vertices[v0].position);
        glm::vec2 p1 = worldToScreen(vertices[v1].position);
        
        // Distance from point to line segment
        glm::vec2 edge = p1 - p0;
        float len = glm::length(edge);
        if (len < 1e-6f) continue;
        
        glm::vec2 dir = edge / len;
        glm::vec2 toPoint = screenPos - p0;
        float t = glm::dot(toPoint, dir);
        t = std::max(0.0f, std::min(t, len));
        
        glm::vec2 closest = p0 + dir * t;
        float dist = glm::length(screenPos - closest);
        
        if (dist < minDist) {
            minDist = dist;
            closest = i;
        }
    }
    
    return closest;
}

int FreeformTool::pickFace(const glm::vec2& screenPos) const {
    if (!m_quadMesh) return -1;
    
    // Ray cast to find closest face
    glm::vec3 rayOrigin = getCamera()->getPosition();
    glm::vec3 rayDir = screenToWorldRay(screenPos);
    
    float minT = std::numeric_limits<float>::max();
    int closest = -1;
    
    const auto& vertices = m_quadMesh->vertices();
    const auto& faces = m_quadMesh->faces();
    
    for (int f = 0; f < static_cast<int>(faces.size()); ++f) {
        // Get face vertices
        auto faceVerts = m_quadMesh->getFaceVertices(f);
        if (faceVerts.size() < 3) continue;
        
        // Triangle intersection (split quad if needed)
        for (size_t i = 1; i < faceVerts.size() - 1; ++i) {
            const glm::vec3& v0 = vertices[faceVerts[0]].position;
            const glm::vec3& v1 = vertices[faceVerts[i]].position;
            const glm::vec3& v2 = vertices[faceVerts[i + 1]].position;
            
            // Möller–Trumbore intersection
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 h = glm::cross(rayDir, edge2);
            float a = glm::dot(edge1, h);
            
            if (std::abs(a) < 1e-6f) continue;
            
            float f = 1.0f / a;
            glm::vec3 s = rayOrigin - v0;
            float u = f * glm::dot(s, h);
            
            if (u < 0.0f || u > 1.0f) continue;
            
            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(rayDir, q);
            
            if (v < 0.0f || u + v > 1.0f) continue;
            
            float t = f * glm::dot(edge2, q);
            
            if (t > 0.0f && t < minT) {
                minT = t;
                closest = f;
            }
        }
    }
    
    return closest;
}

void FreeformTool::beginDrag(const glm::vec2& screenPos) {
    if (m_selectedVertices.empty()) return;
    
    m_isDragging = true;
    m_dragStart = screenPos;
    
    // Compute drag plane (perpendicular to view direction, through selection center)
    glm::vec3 center(0);
    const auto& vertices = m_quadMesh->vertices();
    
    for (int idx : m_selectedVertices) {
        center += vertices[idx].position;
    }
    center /= static_cast<float>(m_selectedVertices.size());
    
    m_dragPlaneOrigin = center;
    m_dragPlaneNormal = glm::normalize(getCamera()->getPosition() - center);
    
    // Store starting positions
    m_dragStartPositions.clear();
    for (int idx : m_selectedVertices) {
        m_dragStartPositions.push_back(vertices[idx].position);
    }
}

void FreeformTool::updateDrag(const glm::vec2& screenPos) {
    if (!m_isDragging || !m_quadMesh) return;
    
    glm::vec3 startWorld = projectOntoPlane(m_dragStart, m_dragPlaneOrigin, m_dragPlaneNormal);
    glm::vec3 currentWorld = projectOntoPlane(screenPos, m_dragPlaneOrigin, m_dragPlaneNormal);
    
    glm::vec3 delta = currentWorld - startWorld;
    
    for (size_t i = 0; i < m_selectedVertices.size(); ++i) {
        m_quadMesh->moveVertex(m_selectedVertices[i], m_dragStartPositions[i] + delta);
    }
    
    m_subdividedDirty = true;
}

void FreeformTool::endDrag() {
    m_isDragging = false;
    m_dragStartPositions.clear();
}

void FreeformTool::applySculptBrush(const glm::vec3& center, const glm::vec3& direction) {
    if (!m_quadMesh) return;
    
    const auto& vertices = m_quadMesh->vertices();
    
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        float dist = glm::length(vertices[i].position - center);
        
        if (dist < m_brushSettings.radius) {
            float falloff = computeBrushFalloff(dist);
            glm::vec3 displacement = direction * m_brushSettings.strength * falloff * 0.01f;
            
            glm::vec3 newPos = vertices[i].position + displacement;
            m_quadMesh->moveVertex(i, newPos);
            
            // Apply symmetry if enabled
            if (m_brushSettings.symmetry) {
                glm::vec3 mirrorPos = vertices[i].position;
                mirrorPos -= 2.0f * glm::dot(mirrorPos, m_brushSettings.symmetryAxis) * 
                             m_brushSettings.symmetryAxis;
                
                // Find corresponding vertex and move it
                for (int j = 0; j < static_cast<int>(vertices.size()); ++j) {
                    if (glm::length(vertices[j].position - mirrorPos) < 0.01f) {
                        glm::vec3 mirrorDisp = displacement;
                        mirrorDisp -= 2.0f * glm::dot(mirrorDisp, m_brushSettings.symmetryAxis) * 
                                      m_brushSettings.symmetryAxis;
                        m_quadMesh->moveVertex(j, vertices[j].position + mirrorDisp);
                        break;
                    }
                }
            }
        }
    }
    
    m_subdividedDirty = true;
}

float FreeformTool::computeBrushFalloff(float distance) const {
    float t = distance / m_brushSettings.radius;
    
    // Smooth falloff based on falloff parameter
    if (m_brushSettings.falloff < 0.5f) {
        // Sharper falloff
        float sharpness = 1.0f - m_brushSettings.falloff * 2.0f;
        return std::pow(1.0f - t, 1.0f + sharpness * 4.0f);
    } else {
        // Smoother falloff
        float smoothness = (m_brushSettings.falloff - 0.5f) * 2.0f;
        float base = 1.0f - t * t;
        return std::pow(base, 1.0f + smoothness);
    }
}

void FreeformTool::smoothSelection(float factor) {
    if (!m_quadMesh || m_selectedVertices.empty()) return;
    
    for (int idx : m_selectedVertices) {
        m_quadMesh->smoothVertex(idx, factor);
    }
    
    m_subdividedDirty = true;
}

void FreeformTool::relaxSelection(int iterations) {
    if (!m_quadMesh || m_selectedVertices.empty()) return;
    
    m_quadMesh->relaxVertices(m_selectedVertices, iterations);
    m_subdividedDirty = true;
}

void FreeformTool::flattenSelection() {
    if (!m_quadMesh || m_selectedVertices.empty()) return;
    
    const auto& vertices = m_quadMesh->vertices();
    
    // Compute average plane
    glm::vec3 center(0);
    glm::vec3 normal(0);
    
    for (int idx : m_selectedVertices) {
        center += vertices[idx].position;
        normal += vertices[idx].normal;
    }
    center /= static_cast<float>(m_selectedVertices.size());
    normal = glm::normalize(normal);
    
    // Project vertices onto plane
    for (int idx : m_selectedVertices) {
        glm::vec3 toVert = vertices[idx].position - center;
        float dist = glm::dot(toVert, normal);
        m_quadMesh->moveVertex(idx, vertices[idx].position - normal * dist);
    }
    
    m_subdividedDirty = true;
}

void FreeformTool::subdivideSelection() {
    // Subdivide selected faces
    // This is a local subdivision operation
    m_subdividedDirty = true;
}

void FreeformTool::extrudeSelection(float distance) {
    if (!m_quadMesh || m_selectedFaces.empty()) return;
    
    for (int faceIdx : m_selectedFaces) {
        m_quadMesh->extrudeFace(faceIdx, distance);
    }
    
    m_subdividedDirty = true;
}

void FreeformTool::insertEdgeLoop() {
    if (!m_quadMesh || m_hoveredEdge < 0) return;
    
    m_quadMesh->insertEdgeLoop(m_hoveredEdge);
    m_subdividedDirty = true;
}

void FreeformTool::deleteSelection() {
    // Delete selected elements
    // Implementation depends on what's selected (vertices, edges, or faces)
    m_subdividedDirty = true;
}

void FreeformTool::markCreaseEdges() {
    if (!m_quadMesh) return;
    
    const auto& halfEdges = m_quadMesh->halfEdges();
    
    for (int edgeIdx : m_selectedEdges) {
        int v0 = halfEdges[halfEdges[edgeIdx].prevIdx].vertexIdx;
        int v1 = halfEdges[edgeIdx].vertexIdx;
        m_quadMesh->setCreaseEdge(v0, v1, m_creaseWeight);
    }
    
    m_subdividedDirty = true;
}

void FreeformTool::unmarkCreaseEdges() {
    if (!m_quadMesh) return;
    
    const auto& halfEdges = m_quadMesh->halfEdges();
    
    for (int edgeIdx : m_selectedEdges) {
        int v0 = halfEdges[halfEdges[edgeIdx].prevIdx].vertexIdx;
        int v1 = halfEdges[edgeIdx].vertexIdx;
        m_quadMesh->removeCreaseEdge(v0, v1);
    }
    
    m_subdividedDirty = true;
}

void FreeformTool::convertToNurbs() {
    if (!m_quadMesh) return;
    
    auto nurbs = m_quadMesh->toNurbs();
    if (nurbs && m_targetObject) {
        // Update target object with NURBS surface
        // m_targetObject->setGeometry(std::move(nurbs));
    }
}

void FreeformTool::convertToQuadMesh() {
    // Convert from other geometry types to quad mesh
}

void FreeformTool::selectAll() {
    if (!m_quadMesh) return;
    
    m_selectedVertices.clear();
    for (int i = 0; i < m_quadMesh->vertexCount(); ++i) {
        m_selectedVertices.push_back(i);
    }
}

void FreeformTool::selectNone() {
    m_selectedVertices.clear();
    m_selectedEdges.clear();
    m_selectedFaces.clear();
}

void FreeformTool::selectConnected() {
    // Grow selection to all connected vertices
}

void FreeformTool::invertSelection() {
    if (!m_quadMesh) return;
    
    std::vector<int> inverted;
    for (int i = 0; i < m_quadMesh->vertexCount(); ++i) {
        if (std::find(m_selectedVertices.begin(), m_selectedVertices.end(), i) == 
            m_selectedVertices.end()) {
            inverted.push_back(i);
        }
    }
    m_selectedVertices = inverted;
}

void FreeformTool::growSelection() {
    if (!m_quadMesh || m_selectedVertices.empty()) return;
    
    std::unordered_set<int> newSelection(m_selectedVertices.begin(), m_selectedVertices.end());
    
    for (int idx : m_selectedVertices) {
        auto neighbors = m_quadMesh->getVertexNeighbors(idx);
        for (int n : neighbors) {
            newSelection.insert(n);
        }
    }
    
    m_selectedVertices.assign(newSelection.begin(), newSelection.end());
}

void FreeformTool::shrinkSelection() {
    if (!m_quadMesh || m_selectedVertices.empty()) return;
    
    std::unordered_set<int> currentSelection(m_selectedVertices.begin(), m_selectedVertices.end());
    std::vector<int> newSelection;
    
    for (int idx : m_selectedVertices) {
        auto neighbors = m_quadMesh->getVertexNeighbors(idx);
        bool allNeighborsSelected = true;
        for (int n : neighbors) {
            if (currentSelection.find(n) == currentSelection.end()) {
                allNeighborsSelected = false;
                break;
            }
        }
        if (allNeighborsSelected) {
            newSelection.push_back(idx);
        }
    }
    
    m_selectedVertices = newSelection;
}

void FreeformTool::setTargetObject(std::shared_ptr<SceneObject> object) {
    m_targetObject = object;
    
    // Extract quad mesh from object if available
    // m_quadMesh = object->getQuadMesh();
    m_subdividedDirty = true;
}

void FreeformTool::renderControlPoints(Renderer& renderer) {
    if (!m_quadMesh) return;
    
    const auto& vertices = m_quadMesh->vertices();
    
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        bool selected = std::find(m_selectedVertices.begin(), m_selectedVertices.end(), i) != 
                        m_selectedVertices.end();
        bool hovered = (i == m_hoveredVertex);
        
        glm::vec4 color;
        float size;
        
        if (selected) {
            color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
            size = 8.0f;
        } else if (hovered) {
            color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
            size = 7.0f;
        } else if (vertices[i].isCorner) {
            color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for corners
            size = 6.0f;
        } else {
            color = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f); // Blue
            size = 5.0f;
        }
        
        renderer.drawPoint(vertices[i].position, color, size);
    }
}

void FreeformTool::renderControlMesh(Renderer& renderer) {
    if (!m_quadMesh) return;
    
    auto wireframeBuffer = m_quadMesh->getWireframeBuffer();
    renderer.drawLines(wireframeBuffer.data(), static_cast<int>(wireframeBuffer.size() / 6),
                       glm::vec4(0.5f, 0.5f, 0.5f, 0.5f), 1.0f);
}

void FreeformTool::renderCreaseEdges(Renderer& renderer) {
    if (!m_quadMesh) return;
    
    auto creaseEdges = m_quadMesh->getCreaseEdges();
    const auto& vertices = m_quadMesh->vertices();
    
    for (const auto& ce : creaseEdges) {
        glm::vec4 color = glm::mix(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                                   glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
                                   ce.weight);
        renderer.drawLine(vertices[ce.vertex0].position, vertices[ce.vertex1].position,
                          color, 3.0f);
    }
}

void FreeformTool::renderSelection(Renderer& renderer) {
    // Render selected faces with highlight
    if (!m_quadMesh || m_selectedFaces.empty()) return;
    
    const auto& vertices = m_quadMesh->vertices();
    
    for (int faceIdx : m_selectedFaces) {
        auto faceVerts = m_quadMesh->getFaceVertices(faceIdx);
        
        // Draw face with selection highlight
        std::vector<glm::vec3> positions;
        for (int v : faceVerts) {
            positions.push_back(vertices[v].position);
        }
        
        renderer.drawPolygon(positions, glm::vec4(1.0f, 0.5f, 0.0f, 0.3f));
    }
}

void FreeformTool::renderBrushCursor(Renderer& renderer) {
    // Draw brush circle at cursor position
    glm::vec3 worldPos = screenToWorld(m_lastMousePos, 0.5f);
    renderer.drawCircle3D(worldPos, m_brushSettings.radius, 
                          glm::vec4(1.0f, 1.0f, 1.0f, 0.5f), 32);
}

glm::vec3 FreeformTool::screenToWorld(const glm::vec2& screenPos, float depth) const {
    // Convert screen coordinates to world position at given depth
    // Implementation depends on camera/projection setup
    return glm::vec3(0);
}

glm::vec3 FreeformTool::projectOntoPlane(const glm::vec2& screenPos,
                                          const glm::vec3& planeOrigin,
                                          const glm::vec3& planeNormal) const {
    glm::vec3 rayOrigin = getCamera()->getPosition();
    glm::vec3 rayDir = screenToWorldRay(screenPos);
    
    float denom = glm::dot(planeNormal, rayDir);
    if (std::abs(denom) < 1e-6f) {
        return planeOrigin;
    }
    
    float t = glm::dot(planeOrigin - rayOrigin, planeNormal) / denom;
    return rayOrigin + rayDir * t;
}

// FreeformToolPanel implementation
FreeformToolPanel::FreeformToolPanel(FreeformTool* tool) : m_tool(tool) {}

void FreeformToolPanel::render() {
    ImGui::Text("Freeform Surface Tool");
    ImGui::Separator();
    
    renderModeSelector();
    ImGui::Separator();
    
    renderSubdivisionControls();
    ImGui::Separator();
    
    if (m_tool->getEditMode() == FreeformEditMode::Crease) {
        renderCreaseControls();
        ImGui::Separator();
    }
    
    if (m_tool->getEditMode() == FreeformEditMode::Sculpt) {
        renderSculptControls();
        ImGui::Separator();
    }
    
    renderSelectionActions();
    ImGui::Separator();
    
    renderConversionActions();
}

void FreeformToolPanel::renderModeSelector() {
    ImGui::Text("Edit Mode:");
    
    int mode = static_cast<int>(m_tool->getEditMode());
    if (ImGui::RadioButton("Select (S)", mode == 0)) {
        m_tool->setEditMode(FreeformEditMode::Select);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Move (G)", mode == 1)) {
        m_tool->setEditMode(FreeformEditMode::Move);
    }
    
    if (ImGui::RadioButton("Smooth", mode == 2)) {
        m_tool->setEditMode(FreeformEditMode::Smooth);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Crease (C)", mode == 3)) {
        m_tool->setEditMode(FreeformEditMode::Crease);
    }
    
    if (ImGui::RadioButton("Sculpt (B)", mode == 9)) {
        m_tool->setEditMode(FreeformEditMode::Sculpt);
    }
    
    ImGui::Spacing();
    ImGui::Text("Select Mode:");
    
    int selectMode = static_cast<int>(m_tool->getSelectMode());
    if (ImGui::RadioButton("Vertex (1)", selectMode == 0)) {
        m_tool->setSelectMode(FreeformSelectMode::Vertex);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Edge (2)", selectMode == 1)) {
        m_tool->setSelectMode(FreeformSelectMode::Edge);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Face (3)", selectMode == 2)) {
        m_tool->setSelectMode(FreeformSelectMode::Face);
    }
}

void FreeformToolPanel::renderSubdivisionControls() {
    ImGui::Text("Subdivision:");
    
    int level = m_tool->getSubdivisionLevel();
    if (ImGui::SliderInt("Level", &level, 0, 5)) {
        m_tool->setSubdivisionLevel(level);
    }
    
    bool showControl = m_tool->getShowControlMesh();
    if (ImGui::Checkbox("Show Control Mesh", &showControl)) {
        m_tool->setShowControlMesh(showControl);
    }
}

void FreeformToolPanel::renderCreaseControls() {
    ImGui::Text("Crease:");
    
    float weight = m_tool->getCreaseWeight();
    if (ImGui::SliderFloat("Weight", &weight, 0.0f, 1.0f)) {
        m_tool->setCreaseWeight(weight);
    }
    
    if (ImGui::Button("Mark Selected")) {
        m_tool->markCreaseEdges();
    }
    ImGui::SameLine();
    if (ImGui::Button("Unmark Selected")) {
        m_tool->unmarkCreaseEdges();
    }
}

void FreeformToolPanel::renderSculptControls() {
    ImGui::Text("Brush:");
    
    auto settings = m_tool->getBrushSettings();
    
    bool changed = false;
    changed |= ImGui::SliderFloat("Radius", &settings.radius, 0.01f, 1.0f);
    changed |= ImGui::SliderFloat("Strength", &settings.strength, 0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Falloff", &settings.falloff, 0.0f, 1.0f);
    changed |= ImGui::Checkbox("Symmetry", &settings.symmetry);
    
    if (changed) {
        m_tool->setBrushSettings(settings);
    }
}

void FreeformToolPanel::renderSelectionActions() {
    ImGui::Text("Selection:");
    
    if (ImGui::Button("All")) {
        m_tool->selectAll();
    }
    ImGui::SameLine();
    if (ImGui::Button("None")) {
        m_tool->selectNone();
    }
    ImGui::SameLine();
    if (ImGui::Button("Invert")) {
        m_tool->invertSelection();
    }
    
    if (ImGui::Button("Grow")) {
        m_tool->growSelection();
    }
    ImGui::SameLine();
    if (ImGui::Button("Shrink")) {
        m_tool->shrinkSelection();
    }
    
    ImGui::Spacing();
    
    if (ImGui::Button("Smooth")) {
        m_tool->smoothSelection();
    }
    ImGui::SameLine();
    if (ImGui::Button("Flatten")) {
        m_tool->flattenSelection();
    }
    ImGui::SameLine();
    if (ImGui::Button("Relax")) {
        m_tool->relaxSelection();
    }
}

void FreeformToolPanel::renderConversionActions() {
    ImGui::Text("Convert:");
    
    if (ImGui::Button("To NURBS")) {
        m_tool->convertToNurbs();
    }
    ImGui::SameLine();
    if (ImGui::Button("To Quad Mesh")) {
        m_tool->convertToQuadMesh();
    }
}

} // namespace dc
