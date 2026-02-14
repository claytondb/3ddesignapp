/**
 * @file SectionPlaneGizmo.cpp
 * @brief Implementation of interactive section plane gizmo
 */

#include "SectionPlaneGizmo.h"
#include "Viewport.h"
#include "Camera.h"

#include <QOpenGLFunctions>
#include <QtMath>
#include <algorithm>

namespace dc {

// ============================================================================
// Constructor / Destructor
// ============================================================================

SectionPlaneGizmo::SectionPlaneGizmo(QObject* parent)
    : QObject(parent)
{
    m_animTimer = std::make_unique<QTimer>(this);
    m_animTimer->setInterval(16); // ~60 FPS
    connect(m_animTimer.get(), &QTimer::timeout, this, &SectionPlaneGizmo::onAnimationTick);
}

SectionPlaneGizmo::~SectionPlaneGizmo()
{
    if (m_animTimer->isActive()) {
        m_animTimer->stop();
    }
}

// ============================================================================
// OpenGL
// ============================================================================

void SectionPlaneGizmo::initialize()
{
    if (m_initialized) return;
    
    // Create shader
    m_shader = std::make_unique<QOpenGLShaderProgram>();
    
    // Vertex shader
    const char* vertSrc = R"(
        #version 410 core
        layout(location = 0) in vec3 position;
        uniform mat4 mvp;
        void main() {
            gl_Position = mvp * vec4(position, 1.0);
        }
    )";
    
    // Fragment shader
    const char* fragSrc = R"(
        #version 410 core
        uniform vec4 color;
        out vec4 fragColor;
        void main() {
            fragColor = color;
        }
    )";
    
    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc);
    m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc);
    m_shader->link();
    
    createPlaneGeometry();
    createHandleGeometry();
    
    m_initialized = true;
}

void SectionPlaneGizmo::cleanup()
{
    if (!m_initialized) return;
    
    m_shader.reset();
    m_planeVAO.reset();
    m_planeVBO.reset();
    m_handleVAO.reset();
    m_handleVBO.reset();
    m_arrowVAO.reset();
    m_arrowVBO.reset();
    
    m_initialized = false;
}

void SectionPlaneGizmo::createPlaneGeometry()
{
    // Create plane quad with grid lines
    std::vector<float> vertices;
    
    // Main quad (two triangles)
    float s = 0.5f; // Half size, scaled later
    
    // Triangle 1
    vertices.insert(vertices.end(), {-s, -s, 0.0f});
    vertices.insert(vertices.end(), { s, -s, 0.0f});
    vertices.insert(vertices.end(), { s,  s, 0.0f});
    
    // Triangle 2
    vertices.insert(vertices.end(), {-s, -s, 0.0f});
    vertices.insert(vertices.end(), { s,  s, 0.0f});
    vertices.insert(vertices.end(), {-s,  s, 0.0f});
    
    m_planeVertices = 6;
    
    // Create VAO/VBO
    m_planeVAO = std::make_unique<QOpenGLVertexArrayObject>();
    m_planeVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    
    m_planeVAO->create();
    m_planeVAO->bind();
    
    m_planeVBO->create();
    m_planeVBO->bind();
    m_planeVBO->allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    m_planeVBO->release();
    m_planeVAO->release();
}

void SectionPlaneGizmo::createHandleGeometry()
{
    // Create translate arrow
    std::vector<float> arrowVerts;
    
    // Arrow shaft (line)
    arrowVerts.insert(arrowVerts.end(), {0.0f, 0.0f, 0.0f});
    arrowVerts.insert(arrowVerts.end(), {0.0f, 0.0f, 1.0f});
    
    // Arrow head cone (simplified as lines)
    const int segments = 8;
    const float coneRadius = 0.1f;
    const float coneHeight = 0.2f;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = static_cast<float>(i) / segments * 2.0f * M_PI;
        float angle2 = static_cast<float>(i + 1) / segments * 2.0f * M_PI;
        
        // Cone base
        float x1 = std::cos(angle1) * coneRadius;
        float y1 = std::sin(angle1) * coneRadius;
        float x2 = std::cos(angle2) * coneRadius;
        float y2 = std::sin(angle2) * coneRadius;
        float z = 1.0f - coneHeight;
        
        // Line from tip
        arrowVerts.insert(arrowVerts.end(), {0.0f, 0.0f, 1.0f});
        arrowVerts.insert(arrowVerts.end(), {x1, y1, z});
        
        // Base edge
        arrowVerts.insert(arrowVerts.end(), {x1, y1, z});
        arrowVerts.insert(arrowVerts.end(), {x2, y2, z});
    }
    
    m_arrowVertices = static_cast<int>(arrowVerts.size() / 3);
    
    m_arrowVAO = std::make_unique<QOpenGLVertexArrayObject>();
    m_arrowVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    
    m_arrowVAO->create();
    m_arrowVAO->bind();
    
    m_arrowVBO->create();
    m_arrowVBO->bind();
    m_arrowVBO->allocate(arrowVerts.data(), static_cast<int>(arrowVerts.size() * sizeof(float)));
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    m_arrowVBO->release();
    m_arrowVAO->release();
    
    // Create handle sphere (for rotation)
    std::vector<float> handleVerts;
    const int rings = 8;
    const int sectors = 8;
    const float radius = 0.08f;
    
    for (int r = 0; r < rings; ++r) {
        float theta1 = static_cast<float>(r) / rings * M_PI;
        float theta2 = static_cast<float>(r + 1) / rings * M_PI;
        
        for (int s = 0; s < sectors; ++s) {
            float phi1 = static_cast<float>(s) / sectors * 2.0f * M_PI;
            float phi2 = static_cast<float>(s + 1) / sectors * 2.0f * M_PI;
            
            // Two triangles per quad
            QVector3D v1(radius * std::sin(theta1) * std::cos(phi1),
                         radius * std::sin(theta1) * std::sin(phi1),
                         radius * std::cos(theta1));
            QVector3D v2(radius * std::sin(theta2) * std::cos(phi1),
                         radius * std::sin(theta2) * std::sin(phi1),
                         radius * std::cos(theta2));
            QVector3D v3(radius * std::sin(theta2) * std::cos(phi2),
                         radius * std::sin(theta2) * std::sin(phi2),
                         radius * std::cos(theta2));
            QVector3D v4(radius * std::sin(theta1) * std::cos(phi2),
                         radius * std::sin(theta1) * std::sin(phi2),
                         radius * std::cos(theta1));
            
            handleVerts.insert(handleVerts.end(), {v1.x(), v1.y(), v1.z()});
            handleVerts.insert(handleVerts.end(), {v2.x(), v2.y(), v2.z()});
            handleVerts.insert(handleVerts.end(), {v3.x(), v3.y(), v3.z()});
            
            handleVerts.insert(handleVerts.end(), {v1.x(), v1.y(), v1.z()});
            handleVerts.insert(handleVerts.end(), {v3.x(), v3.y(), v3.z()});
            handleVerts.insert(handleVerts.end(), {v4.x(), v4.y(), v4.z()});
        }
    }
    
    m_handleVertices = static_cast<int>(handleVerts.size() / 3);
    
    m_handleVAO = std::make_unique<QOpenGLVertexArrayObject>();
    m_handleVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    
    m_handleVAO->create();
    m_handleVAO->bind();
    
    m_handleVBO->create();
    m_handleVBO->bind();
    m_handleVBO->allocate(handleVerts.data(), static_cast<int>(handleVerts.size() * sizeof(float)));
    
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    m_handleVBO->release();
    m_handleVAO->release();
}

void SectionPlaneGizmo::render(const QMatrix4x4& view, const QMatrix4x4& projection,
                                const QSize& viewportSize)
{
    if (!m_initialized || !m_visible || m_planes.empty()) return;
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    
    // Enable blending for transparency
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable(GL_DEPTH_TEST);
    
    m_shader->bind();
    
    // Calculate screen-space consistent scale
    float scale = m_planeSize;
    
    // Render each plane
    for (const auto& plane : m_planes) {
        if (!plane.enabled) continue;
        
        bool isActive = (plane.id == m_activePlaneId);
        renderPlane(plane, view, projection, scale, isActive);
        
        if (isActive) {
            renderHandles(plane, view, projection, scale * 0.3f);
        }
        
        if (plane.showCap) {
            renderCap(plane, view, projection, scale);
        }
    }
    
    m_shader->release();
    f->glDisable(GL_BLEND);
}

void SectionPlaneGizmo::renderPlane(const SectionPlane& plane, const QMatrix4x4& view,
                                     const QMatrix4x4& projection, float scale, bool active)
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    
    // Build transformation matrix
    QMatrix4x4 model;
    QVector3D pos = plane.origin + plane.normal * plane.offset;
    model.translate(pos);
    
    // Rotate to align with normal
    QVector3D up(0, 0, 1);
    if (std::abs(QVector3D::dotProduct(plane.normal, up)) > 0.99f) {
        up = QVector3D(0, 1, 0);
    }
    QVector3D right = QVector3D::crossProduct(up, plane.normal).normalized();
    up = QVector3D::crossProduct(plane.normal, right).normalized();
    
    QMatrix4x4 rot;
    rot.setColumn(0, QVector4D(right, 0));
    rot.setColumn(1, QVector4D(up, 0));
    rot.setColumn(2, QVector4D(plane.normal, 0));
    rot.setColumn(3, QVector4D(0, 0, 0, 1));
    
    model *= rot;
    model.scale(scale);
    
    QMatrix4x4 mvp = projection * view * model;
    m_shader->setUniformValue("mvp", mvp);
    
    // Set color (with transparency)
    QColor color = active ? m_activeColor : plane.planeColor;
    color.setAlphaF(m_planeOpacity);
    m_shader->setUniformValue("color", QVector4D(
        color.redF(), color.greenF(), color.blueF(), color.alphaF()));
    
    // Render plane quad
    m_planeVAO->bind();
    f->glDrawArrays(GL_TRIANGLES, 0, m_planeVertices);
    m_planeVAO->release();
    
    // Render border (solid color)
    f->glLineWidth(active ? 3.0f : 2.0f);
    color.setAlphaF(1.0f);
    m_shader->setUniformValue("color", QVector4D(
        color.redF(), color.greenF(), color.blueF(), 1.0f));
    
    // Draw border lines manually
    float s = 0.5f;
    float borderVerts[] = {
        -s, -s, 0.0f,  s, -s, 0.0f,
         s, -s, 0.0f,  s,  s, 0.0f,
         s,  s, 0.0f, -s,  s, 0.0f,
        -s,  s, 0.0f, -s, -s, 0.0f
    };
    
    GLuint borderVBO;
    f->glGenBuffers(1, &borderVBO);
    f->glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(borderVerts), borderVerts, GL_STATIC_DRAW);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glDrawArrays(GL_LINES, 0, 8);
    f->glDeleteBuffers(1, &borderVBO);
    
    f->glLineWidth(1.0f);
}

void SectionPlaneGizmo::renderHandles(const SectionPlane& plane, const QMatrix4x4& view,
                                       const QMatrix4x4& projection, float scale)
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    
    QVector3D pos = plane.origin + plane.normal * plane.offset;
    
    // Render translate arrow (normal direction)
    QMatrix4x4 model;
    model.translate(pos);
    
    // Align arrow with normal
    QVector3D up(0, 0, 1);
    if (std::abs(QVector3D::dotProduct(plane.normal, up)) > 0.99f) {
        up = QVector3D(0, 1, 0);
    }
    QVector3D right = QVector3D::crossProduct(up, plane.normal).normalized();
    up = QVector3D::crossProduct(plane.normal, right).normalized();
    
    QMatrix4x4 rot;
    rot.setColumn(0, QVector4D(right, 0));
    rot.setColumn(1, QVector4D(up, 0));
    rot.setColumn(2, QVector4D(plane.normal, 0));
    rot.setColumn(3, QVector4D(0, 0, 0, 1));
    
    model *= rot;
    model.scale(scale);
    
    QMatrix4x4 mvp = projection * view * model;
    m_shader->setUniformValue("mvp", mvp);
    
    // Arrow color (yellow for translate)
    QColor color = (m_hoverHandle == 1 || m_dragHandle == 1) ? m_hoverColor : m_handleColor;
    m_shader->setUniformValue("color", QVector4D(
        color.redF(), color.greenF(), color.blueF(), 1.0f));
    
    f->glLineWidth(2.0f);
    m_arrowVAO->bind();
    f->glDrawArrays(GL_LINES, 0, m_arrowVertices);
    m_arrowVAO->release();
    
    // Render negative direction arrow
    model.setToIdentity();
    model.translate(pos);
    model *= rot;
    model.rotate(180.0f, right);
    model.scale(scale);
    
    mvp = projection * view * model;
    m_shader->setUniformValue("mvp", mvp);
    
    m_arrowVAO->bind();
    f->glDrawArrays(GL_LINES, 0, m_arrowVertices);
    m_arrowVAO->release();
    
    f->glLineWidth(1.0f);
}

void SectionPlaneGizmo::renderCap(const SectionPlane& plane, const QMatrix4x4& view,
                                   const QMatrix4x4& projection, float scale)
{
    // Cap rendering uses same plane geometry but different color and no transparency
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    
    QMatrix4x4 model;
    QVector3D pos = plane.origin + plane.normal * plane.offset;
    model.translate(pos);
    
    QVector3D up(0, 0, 1);
    if (std::abs(QVector3D::dotProduct(plane.normal, up)) > 0.99f) {
        up = QVector3D(0, 1, 0);
    }
    QVector3D right = QVector3D::crossProduct(up, plane.normal).normalized();
    up = QVector3D::crossProduct(plane.normal, right).normalized();
    
    QMatrix4x4 rot;
    rot.setColumn(0, QVector4D(right, 0));
    rot.setColumn(1, QVector4D(up, 0));
    rot.setColumn(2, QVector4D(plane.normal, 0));
    rot.setColumn(3, QVector4D(0, 0, 0, 1));
    
    model *= rot;
    model.scale(scale * 0.95f); // Slightly smaller than plane
    
    // Offset slightly along normal to avoid z-fighting
    model.translate(0, 0, 0.001f);
    
    QMatrix4x4 mvp = projection * view * model;
    m_shader->setUniformValue("mvp", mvp);
    
    QColor color = plane.capColor;
    m_shader->setUniformValue("color", QVector4D(
        color.redF(), color.greenF(), color.blueF(), color.alphaF() / 255.0f));
    
    m_planeVAO->bind();
    f->glDrawArrays(GL_TRIANGLES, 0, m_planeVertices);
    m_planeVAO->release();
}

// ============================================================================
// Section Planes
// ============================================================================

int SectionPlaneGizmo::addSectionPlane(SectionAxis axis)
{
    SectionPlane plane;
    plane.id = nextPlaneId();
    
    switch (axis) {
        case SectionAxis::X:
            plane.normal = QVector3D(1, 0, 0);
            break;
        case SectionAxis::Y:
            plane.normal = QVector3D(0, 1, 0);
            break;
        case SectionAxis::Z:
        default:
            plane.normal = QVector3D(0, 0, 1);
            break;
    }
    
    // Set origin to center of bounds
    plane.origin = (m_meshMin + m_meshMax) * 0.5f;
    plane.offset = 0.0f;
    
    m_planes.push_back(plane);
    
    if (m_activePlaneId < 0) {
        m_activePlaneId = plane.id;
    }
    
    emit planesChanged();
    return plane.id;
}

void SectionPlaneGizmo::removeSectionPlane(int id)
{
    auto it = std::remove_if(m_planes.begin(), m_planes.end(),
                             [id](const SectionPlane& p) { return p.id == id; });
    
    if (it != m_planes.end()) {
        m_planes.erase(it, m_planes.end());
        
        if (m_activePlaneId == id) {
            m_activePlaneId = m_planes.empty() ? -1 : m_planes.front().id;
        }
        
        emit planesChanged();
    }
}

void SectionPlaneGizmo::clearSectionPlanes()
{
    m_planes.clear();
    m_activePlaneId = -1;
    emit planesChanged();
}

SectionPlane* SectionPlaneGizmo::plane(int id)
{
    for (auto& p : m_planes) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

const SectionPlane* SectionPlaneGizmo::plane(int id) const
{
    for (const auto& p : m_planes) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

void SectionPlaneGizmo::setActivePlane(int id)
{
    if (plane(id) || id < 0) {
        m_activePlaneId = id;
    }
}

// ============================================================================
// Quick Presets
// ============================================================================

void SectionPlaneGizmo::setPlaneAxisX(int planeId)
{
    int id = (planeId < 0) ? m_activePlaneId : planeId;
    if (auto* p = plane(id)) {
        p->normal = QVector3D(1, 0, 0);
        emit planeNormalChanged(id, p->normal);
    }
}

void SectionPlaneGizmo::setPlaneAxisY(int planeId)
{
    int id = (planeId < 0) ? m_activePlaneId : planeId;
    if (auto* p = plane(id)) {
        p->normal = QVector3D(0, 1, 0);
        emit planeNormalChanged(id, p->normal);
    }
}

void SectionPlaneGizmo::setPlaneAxisZ(int planeId)
{
    int id = (planeId < 0) ? m_activePlaneId : planeId;
    if (auto* p = plane(id)) {
        p->normal = QVector3D(0, 0, 1);
        emit planeNormalChanged(id, p->normal);
    }
}

void SectionPlaneGizmo::setPlaneAtCenter(int planeId, const QVector3D& center)
{
    if (auto* p = plane(planeId)) {
        p->origin = center;
        p->offset = 0.0f;
        emit planeOffsetChanged(planeId, 0.0f);
    }
}

// ============================================================================
// Section Properties
// ============================================================================

void SectionPlaneGizmo::setPlaneOffset(int planeId, float offset)
{
    if (auto* p = plane(planeId)) {
        p->offset = offset;
        emit planeOffsetChanged(planeId, offset);
    }
}

void SectionPlaneGizmo::setPlaneNormal(int planeId, const QVector3D& normal)
{
    if (auto* p = plane(planeId)) {
        p->normal = normal.normalized();
        emit planeNormalChanged(planeId, p->normal);
    }
}

void SectionPlaneGizmo::setPlaneOrigin(int planeId, const QVector3D& origin)
{
    if (auto* p = plane(planeId)) {
        p->origin = origin;
    }
}

void SectionPlaneGizmo::setPlaneEnabled(int planeId, bool enabled)
{
    if (auto* p = plane(planeId)) {
        p->enabled = enabled;
    }
}

// ============================================================================
// Section Cap
// ============================================================================

void SectionPlaneGizmo::setShowCap(int planeId, bool show)
{
    if (auto* p = plane(planeId)) {
        p->showCap = show;
    }
}

bool SectionPlaneGizmo::showCap(int planeId) const
{
    if (const auto* p = plane(planeId)) {
        return p->showCap;
    }
    return false;
}

void SectionPlaneGizmo::setCapColor(int planeId, const QColor& color)
{
    if (auto* p = plane(planeId)) {
        p->capColor = color;
    }
}

// ============================================================================
// Bounds
// ============================================================================

void SectionPlaneGizmo::setMeshBounds(const QVector3D& min, const QVector3D& max)
{
    m_meshMin = min;
    m_meshMax = max;
    
    // Update plane size to match bounds
    QVector3D size = max - min;
    m_planeSize = std::max({size.x(), size.y(), size.z()}) * 1.2f;
}

void SectionPlaneGizmo::getOffsetRange(int planeId, float& minOffset, float& maxOffset) const
{
    const auto* p = plane(planeId);
    if (!p) {
        minOffset = -100.0f;
        maxOffset = 100.0f;
        return;
    }
    
    // Project bounds onto plane normal
    QVector3D corners[8] = {
        QVector3D(m_meshMin.x(), m_meshMin.y(), m_meshMin.z()),
        QVector3D(m_meshMax.x(), m_meshMin.y(), m_meshMin.z()),
        QVector3D(m_meshMin.x(), m_meshMax.y(), m_meshMin.z()),
        QVector3D(m_meshMax.x(), m_meshMax.y(), m_meshMin.z()),
        QVector3D(m_meshMin.x(), m_meshMin.y(), m_meshMax.z()),
        QVector3D(m_meshMax.x(), m_meshMin.y(), m_meshMax.z()),
        QVector3D(m_meshMin.x(), m_meshMax.y(), m_meshMax.z()),
        QVector3D(m_meshMax.x(), m_meshMax.y(), m_meshMax.z())
    };
    
    minOffset = std::numeric_limits<float>::max();
    maxOffset = std::numeric_limits<float>::lowest();
    
    for (const auto& corner : corners) {
        float d = QVector3D::dotProduct(corner - p->origin, p->normal);
        minOffset = std::min(minOffset, d);
        maxOffset = std::max(maxOffset, d);
    }
    
    // Add 10% margin
    float margin = (maxOffset - minOffset) * 0.1f;
    minOffset -= margin;
    maxOffset += margin;
}

// ============================================================================
// Interaction
// ============================================================================

SectionGizmoHitResult SectionPlaneGizmo::hitTest(const QPoint& screenPos, 
                                                   const Viewport* viewport) const
{
    SectionGizmoHitResult result;
    
    if (!viewport || m_planes.empty()) return result;
    
    // TODO: Implement proper ray-plane intersection
    // For now, return basic hit result based on active plane
    
    return result;
}

void SectionPlaneGizmo::beginDrag(const QPoint& screenPos, const Viewport* viewport)
{
    if (m_activePlaneId < 0) return;
    
    auto* p = plane(m_activePlaneId);
    if (!p) return;
    
    m_dragging = true;
    m_dragStartPos = screenPos;
    m_dragStartOffset = p->offset;
    m_dragStartNormal = p->normal;
    m_dragHandle = 1; // Translate
    
    emit dragStarted(m_activePlaneId);
}

void SectionPlaneGizmo::updateDrag(const QPoint& screenPos, const Viewport* viewport)
{
    if (!m_dragging || m_activePlaneId < 0) return;
    
    auto* p = plane(m_activePlaneId);
    if (!p) return;
    
    // Simple offset adjustment based on mouse Y delta
    float delta = (m_dragStartPos.y() - screenPos.y()) * 0.5f;
    float newOffset = m_dragStartOffset + delta;
    
    // Clamp to range
    float minOff, maxOff;
    getOffsetRange(m_activePlaneId, minOff, maxOff);
    newOffset = std::clamp(newOffset, minOff, maxOff);
    
    p->offset = newOffset;
    emit planeOffsetChanged(m_activePlaneId, newOffset);
}

void SectionPlaneGizmo::endDrag()
{
    if (m_dragging) {
        m_dragging = false;
        m_dragHandle = -1;
        emit dragEnded(m_activePlaneId);
    }
}

// ============================================================================
// Animation
// ============================================================================

void SectionPlaneGizmo::playAnimation(int planeId)
{
    m_animPlaneId = (planeId < 0) ? m_activePlaneId : planeId;
    
    if (m_animPlaneId < 0) return;
    
    auto* p = plane(m_animPlaneId);
    if (!p) return;
    
    // Get offset range if not set
    if (m_animation.startOffset == m_animation.endOffset) {
        getOffsetRange(m_animPlaneId, m_animation.startOffset, m_animation.endOffset);
    }
    
    m_animation.playing = true;
    m_animation.currentTime = 0.0f;
    m_animTimer->start();
}

void SectionPlaneGizmo::pauseAnimation()
{
    m_animation.playing = false;
    m_animTimer->stop();
}

void SectionPlaneGizmo::stopAnimation()
{
    m_animation.playing = false;
    m_animation.currentTime = 0.0f;
    m_animTimer->stop();
    
    if (auto* p = plane(m_animPlaneId)) {
        p->offset = m_animation.startOffset;
        emit planeOffsetChanged(m_animPlaneId, p->offset);
    }
}

void SectionPlaneGizmo::setAnimationRange(float start, float end)
{
    m_animation.startOffset = start;
    m_animation.endOffset = end;
}

void SectionPlaneGizmo::setAnimationDuration(float seconds)
{
    m_animation.duration = std::max(0.1f, seconds);
}

void SectionPlaneGizmo::setAnimationLoop(bool loop)
{
    m_animation.loop = loop;
}

void SectionPlaneGizmo::setAnimationPingPong(bool pingPong)
{
    m_animation.pingPong = pingPong;
}

void SectionPlaneGizmo::onAnimationTick()
{
    if (!m_animation.playing) return;
    
    auto* p = plane(m_animPlaneId);
    if (!p) {
        stopAnimation();
        return;
    }
    
    // Advance time
    float dt = 0.016f; // ~60 FPS
    m_animation.currentTime += dt;
    
    float t = m_animation.currentTime / m_animation.duration;
    
    if (t >= 1.0f) {
        if (m_animation.loop) {
            if (m_animation.pingPong) {
                m_animation.reverse = !m_animation.reverse;
            }
            m_animation.currentTime = 0.0f;
            t = 0.0f;
        } else {
            t = 1.0f;
            stopAnimation();
        }
    }
    
    // Calculate offset
    float offset;
    if (m_animation.reverse) {
        offset = m_animation.endOffset + (m_animation.startOffset - m_animation.endOffset) * t;
    } else {
        offset = m_animation.startOffset + (m_animation.endOffset - m_animation.startOffset) * t;
    }
    
    p->offset = offset;
    emit animationFrameUpdated(m_animPlaneId, offset);
}

// ============================================================================
// Private
// ============================================================================

int SectionPlaneGizmo::nextPlaneId()
{
    return m_nextId++;
}

} // namespace dc
