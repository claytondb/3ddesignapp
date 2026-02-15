/**
 * @file TransformGizmo.cpp
 * @brief Implementation of visual 3D transform gizmo
 */

#include "TransformGizmo.h"
#include "Viewport.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QQuaternion>
#include <QtMath>

namespace dc {

// Shader source code - using #version 410 for consistency with other project shaders
static const char* gizmoVertexShader = R"(
    #version 410 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 uMVP;
    void main() {
        gl_Position = uMVP * vec4(aPos, 1.0);
    }
)";

static const char* gizmoFragmentShader = R"(
    #version 410 core
    uniform vec4 uColor;
    out vec4 fragColor;
    void main() {
        fragColor = uColor;
    }
)";

TransformGizmo::TransformGizmo(QObject* parent)
    : QObject(parent)
{
}

TransformGizmo::~TransformGizmo()
{
    // Note: cleanup() should only be called explicitly when OpenGL context is current.
    // The owner must call cleanup() before destruction to avoid OpenGL calls
    // without a valid context, which causes undefined behavior or crashes.
}

void TransformGizmo::cleanup()
{
    if (!m_initialized) {
        return;
    }
    
    // Clean up translate gizmo resources
    if (m_translateVBO) {
        m_translateVBO->destroy();
        m_translateVBO.reset();
    }
    if (m_translateVAO) {
        m_translateVAO->destroy();
        m_translateVAO.reset();
    }
    
    // Clean up rotate gizmo resources
    if (m_rotateVBO) {
        m_rotateVBO->destroy();
        m_rotateVBO.reset();
    }
    if (m_rotateVAO) {
        m_rotateVAO->destroy();
        m_rotateVAO.reset();
    }
    
    // Clean up scale gizmo resources
    if (m_scaleVBO) {
        m_scaleVBO->destroy();
        m_scaleVBO.reset();
    }
    if (m_scaleVAO) {
        m_scaleVAO->destroy();
        m_scaleVAO.reset();
    }
    
    // Clean up shader
    m_shader.reset();
    
    m_initialized = false;
}

void TransformGizmo::initialize() {
    if (m_initialized) {
        return;
    }
    
    // Create shader
    m_shader = std::make_unique<QOpenGLShaderProgram>();
    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, gizmoVertexShader);
    m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, gizmoFragmentShader);
    m_shader->link();
    
    createTranslateGeometry();
    createRotateGeometry();
    createScaleGeometry();
    
    m_initialized = true;
}

void TransformGizmo::createTranslateGeometry() {
    // Create arrows for each axis
    std::vector<float> vertices;
    
    // Arrow shaft length and head dimensions
    float shaftLength = 0.8f;
    float headLength = 0.2f;
    float headRadius = 0.08f;
    int segments = 12;
    
    auto addArrow = [&](const QVector3D& dir) {
        QVector3D perpA, perpB;
        if (qAbs(dir.x()) < 0.9f) {
            perpA = QVector3D::crossProduct(dir, QVector3D(1, 0, 0)).normalized();
        } else {
            perpA = QVector3D::crossProduct(dir, QVector3D(0, 1, 0)).normalized();
        }
        perpB = QVector3D::crossProduct(dir, perpA).normalized();
        
        // Shaft (line)
        vertices.push_back(0); vertices.push_back(0); vertices.push_back(0);
        vertices.push_back(dir.x() * shaftLength);
        vertices.push_back(dir.y() * shaftLength);
        vertices.push_back(dir.z() * shaftLength);
        
        // Cone head
        QVector3D tipPoint = dir;
        QVector3D baseCenter = dir * shaftLength;
        
        for (int i = 0; i < segments; ++i) {
            float angle1 = 2.0f * M_PI * i / segments;
            float angle2 = 2.0f * M_PI * (i + 1) / segments;
            
            QVector3D p1 = baseCenter + headRadius * (perpA * qCos(angle1) + perpB * qSin(angle1));
            QVector3D p2 = baseCenter + headRadius * (perpA * qCos(angle2) + perpB * qSin(angle2));
            
            // Triangle from tip to edge
            vertices.push_back(tipPoint.x()); vertices.push_back(tipPoint.y()); vertices.push_back(tipPoint.z());
            vertices.push_back(p1.x()); vertices.push_back(p1.y()); vertices.push_back(p1.z());
            vertices.push_back(p2.x()); vertices.push_back(p2.y()); vertices.push_back(p2.z());
        }
    };
    
    addArrow(QVector3D(1, 0, 0)); // X
    addArrow(QVector3D(0, 1, 0)); // Y
    addArrow(QVector3D(0, 0, 1)); // Z
    
    m_translateArrowVertices = static_cast<int>(vertices.size() / 3);
    
    m_translateVAO = std::make_unique<QOpenGLVertexArrayObject>();
    m_translateVAO->create();
    m_translateVAO->bind();
    
    m_translateVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    m_translateVBO->create();
    m_translateVBO->bind();
    m_translateVBO->allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    m_translateVAO->release();
}

void TransformGizmo::createRotateGeometry() {
    // Create circles for each axis
    std::vector<float> vertices;
    
    int segments = 64;
    float radius = 0.9f;
    
    auto addCircle = [&](const QVector3D& normal) {
        QVector3D perpA, perpB;
        if (qAbs(normal.x()) < 0.9f) {
            perpA = QVector3D::crossProduct(normal, QVector3D(1, 0, 0)).normalized();
        } else {
            perpA = QVector3D::crossProduct(normal, QVector3D(0, 1, 0)).normalized();
        }
        perpB = QVector3D::crossProduct(normal, perpA).normalized();
        
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            QVector3D p = radius * (perpA * qCos(angle) + perpB * qSin(angle));
            vertices.push_back(p.x());
            vertices.push_back(p.y());
            vertices.push_back(p.z());
        }
    };
    
    addCircle(QVector3D(1, 0, 0)); // X - rotates around X
    addCircle(QVector3D(0, 1, 0)); // Y - rotates around Y
    addCircle(QVector3D(0, 0, 1)); // Z - rotates around Z
    
    m_rotateCircleVertices = segments + 1;
    
    m_rotateVAO = std::make_unique<QOpenGLVertexArrayObject>();
    m_rotateVAO->create();
    m_rotateVAO->bind();
    
    m_rotateVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    m_rotateVBO->create();
    m_rotateVBO->bind();
    m_rotateVBO->allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    m_rotateVAO->release();
}

void TransformGizmo::createScaleGeometry() {
    // Create axes with cubes at ends
    std::vector<float> vertices;
    
    float shaftLength = 0.85f;
    float boxSize = 0.1f;
    
    auto addScaleAxis = [&](const QVector3D& dir) {
        // Shaft line
        vertices.push_back(0); vertices.push_back(0); vertices.push_back(0);
        vertices.push_back(dir.x() * shaftLength);
        vertices.push_back(dir.y() * shaftLength);
        vertices.push_back(dir.z() * shaftLength);
        
        // Box at end (simplified as 12 lines forming cube edges)
        QVector3D center = dir;
        QVector3D perpA, perpB;
        if (qAbs(dir.x()) < 0.9f) {
            perpA = QVector3D::crossProduct(dir, QVector3D(1, 0, 0)).normalized();
        } else {
            perpA = QVector3D::crossProduct(dir, QVector3D(0, 1, 0)).normalized();
        }
        perpB = QVector3D::crossProduct(dir, perpA).normalized();
        
        // 8 corners of box
        QVector3D corners[8];
        for (int i = 0; i < 8; ++i) {
            float dx = (i & 1) ? boxSize : -boxSize;
            float dy = (i & 2) ? boxSize : -boxSize;
            float dz = (i & 4) ? boxSize : -boxSize;
            corners[i] = center + dx * dir + dy * perpA + dz * perpB;
        }
        
        // 12 edges
        int edges[12][2] = {
            {0,1}, {2,3}, {4,5}, {6,7},
            {0,2}, {1,3}, {4,6}, {5,7},
            {0,4}, {1,5}, {2,6}, {3,7}
        };
        
        for (int e = 0; e < 12; ++e) {
            vertices.push_back(corners[edges[e][0]].x());
            vertices.push_back(corners[edges[e][0]].y());
            vertices.push_back(corners[edges[e][0]].z());
            vertices.push_back(corners[edges[e][1]].x());
            vertices.push_back(corners[edges[e][1]].y());
            vertices.push_back(corners[edges[e][1]].z());
        }
    };
    
    addScaleAxis(QVector3D(1, 0, 0)); // X
    addScaleAxis(QVector3D(0, 1, 0)); // Y
    addScaleAxis(QVector3D(0, 0, 1)); // Z
    
    m_scaleBoxVertices = static_cast<int>(vertices.size() / 3);
    
    m_scaleVAO = std::make_unique<QOpenGLVertexArrayObject>();
    m_scaleVAO->create();
    m_scaleVAO->bind();
    
    m_scaleVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    m_scaleVBO->create();
    m_scaleVBO->bind();
    m_scaleVBO->allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    m_scaleVAO->release();
}

void TransformGizmo::render(const QMatrix4x4& view, const QMatrix4x4& projection, const QSize& viewportSize) {
    if (!m_visible || !m_initialized) {
        return;
    }
    
    // Compute scale for screen-space consistent sizing
    float scale = m_size;
    if (m_screenSpaceSizing) {
        scale = computeScreenScale(view, projection, viewportSize);
    }
    
    // Build model matrix
    QMatrix4x4 model;
    model.translate(m_position);
    model.rotate(m_orientation);
    model.scale(scale);
    
    QMatrix4x4 mvp = projection * view * model;
    
    m_shader->bind();
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glDisable(GL_DEPTH_TEST);
    f->glLineWidth(2.0f);
    
    switch (m_mode) {
        case GizmoMode::Translate:
            renderTranslate(mvp, scale);
            break;
        case GizmoMode::Rotate:
            renderRotate(mvp, scale);
            break;
        case GizmoMode::Scale:
            renderScale(mvp, scale);
            break;
    }
    
    f->glEnable(GL_DEPTH_TEST);
    f->glLineWidth(1.0f);
    
    m_shader->release();
}

void TransformGizmo::renderTranslate(const QMatrix4x4& mvp, float scale) {
    Q_UNUSED(scale);
    
    m_shader->setUniformValue("uMVP", mvp);
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    m_translateVAO->bind();
    
    // Calculate vertices per arrow
    int segments = 12;
    int verticesPerArrow = 2 + segments * 3; // shaft + cone
    
    // X axis
    QColor xColor = getAxisColor(0, m_hoverAxis == 0, m_activeAxis == 0);
    m_shader->setUniformValue("uColor", xColor);
    f->glDrawArrays(GL_LINES, 0, 2);
    f->glDrawArrays(GL_TRIANGLES, 2, segments * 3);
    
    // Y axis
    QColor yColor = getAxisColor(1, m_hoverAxis == 1, m_activeAxis == 1);
    m_shader->setUniformValue("uColor", yColor);
    f->glDrawArrays(GL_LINES, verticesPerArrow, 2);
    f->glDrawArrays(GL_TRIANGLES, verticesPerArrow + 2, segments * 3);
    
    // Z axis
    QColor zColor = getAxisColor(2, m_hoverAxis == 2, m_activeAxis == 2);
    m_shader->setUniformValue("uColor", zColor);
    f->glDrawArrays(GL_LINES, verticesPerArrow * 2, 2);
    f->glDrawArrays(GL_TRIANGLES, verticesPerArrow * 2 + 2, segments * 3);
    
    m_translateVAO->release();
}

void TransformGizmo::renderRotate(const QMatrix4x4& mvp, float scale) {
    Q_UNUSED(scale);
    
    m_shader->setUniformValue("uMVP", mvp);
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    m_rotateVAO->bind();
    
    // X circle (red, rotates around X)
    QColor xColor = getAxisColor(0, m_hoverAxis == 0, m_activeAxis == 0);
    m_shader->setUniformValue("uColor", xColor);
    f->glDrawArrays(GL_LINE_STRIP, 0, m_rotateCircleVertices);
    
    // Y circle (green, rotates around Y)
    QColor yColor = getAxisColor(1, m_hoverAxis == 1, m_activeAxis == 1);
    m_shader->setUniformValue("uColor", yColor);
    f->glDrawArrays(GL_LINE_STRIP, m_rotateCircleVertices, m_rotateCircleVertices);
    
    // Z circle (blue, rotates around Z)
    QColor zColor = getAxisColor(2, m_hoverAxis == 2, m_activeAxis == 2);
    m_shader->setUniformValue("uColor", zColor);
    f->glDrawArrays(GL_LINE_STRIP, m_rotateCircleVertices * 2, m_rotateCircleVertices);
    
    m_rotateVAO->release();
}

void TransformGizmo::renderScale(const QMatrix4x4& mvp, float scale) {
    Q_UNUSED(scale);
    
    m_shader->setUniformValue("uMVP", mvp);
    
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    m_scaleVAO->bind();
    
    // Vertices per axis: 2 (shaft) + 24 (box edges = 12 edges * 2 vertices)
    int verticesPerAxis = 2 + 24;
    
    // X axis
    QColor xColor = getAxisColor(0, m_hoverAxis == 0, m_activeAxis == 0);
    m_shader->setUniformValue("uColor", xColor);
    f->glDrawArrays(GL_LINES, 0, verticesPerAxis);
    
    // Y axis
    QColor yColor = getAxisColor(1, m_hoverAxis == 1, m_activeAxis == 1);
    m_shader->setUniformValue("uColor", yColor);
    f->glDrawArrays(GL_LINES, verticesPerAxis, verticesPerAxis);
    
    // Z axis
    QColor zColor = getAxisColor(2, m_hoverAxis == 2, m_activeAxis == 2);
    m_shader->setUniformValue("uColor", zColor);
    f->glDrawArrays(GL_LINES, verticesPerAxis * 2, verticesPerAxis);
    
    m_scaleVAO->release();
}

void TransformGizmo::setPosition(const QVector3D& pos) {
    m_position = pos;
}

void TransformGizmo::setOrientation(const QQuaternion& orientation) {
    m_orientation = orientation;
}

void TransformGizmo::setMode(int mode) {
    m_mode = static_cast<GizmoMode>(mode);
}

GizmoHitResult TransformGizmo::hitTest(const QPoint& screenPos, const Viewport* viewport) const {
    GizmoHitResult result;
    
    if (!m_visible || !viewport) {
        return result;
    }
    
    // Get ray from screen position
    // This would use viewport's unproject functionality
    // For now, return a simple proximity-based hit test
    
    // TODO: Implement proper ray-gizmo intersection
    // This requires unprojecting the mouse position and testing against gizmo geometry
    
    result.hit = false;
    return result;
}

void TransformGizmo::setActiveAxis(int axis) {
    m_activeAxis = axis;
}

void TransformGizmo::setHoverAxis(int axis) {
    m_hoverAxis = axis;
}

void TransformGizmo::setSize(float size) {
    m_size = size;
}

void TransformGizmo::setScreenSpaceSizing(bool enabled) {
    m_screenSpaceSizing = enabled;
}

void TransformGizmo::setVisible(bool visible) {
    m_visible = visible;
}

void TransformGizmo::setAxisColors(const QColor& x, const QColor& y, const QColor& z) {
    m_axisColorX = x;
    m_axisColorY = y;
    m_axisColorZ = z;
}

void TransformGizmo::setHighlightColor(const QColor& color) {
    m_highlightColor = color;
}

void TransformGizmo::setSelectionColor(const QColor& color) {
    m_selectionColor = color;
}

QColor TransformGizmo::getAxisColor(int axis, bool highlight, bool selected) const {
    if (selected) {
        return m_selectionColor;
    }
    if (highlight) {
        return m_highlightColor;
    }
    
    switch (axis) {
        case 0: return m_axisColorX;
        case 1: return m_axisColorY;
        case 2: return m_axisColorZ;
        default: return QColor(200, 200, 200);
    }
}

float TransformGizmo::computeScreenScale(const QMatrix4x4& view, const QMatrix4x4& projection,
                                          const QSize& viewportSize) const {
    // Transform gizmo position to clip space
    QVector4D clipPos = projection * view * QVector4D(m_position, 1.0f);
    
    if (qAbs(clipPos.w()) < 1e-6f) {
        return m_size;
    }
    
    // Compute scale factor to maintain constant screen size
    float depth = clipPos.w();
    float viewportHeight = static_cast<float>(viewportSize.height());
    
    // Get the projection's vertical field of view effect
    float projScale = 2.0f / projection(1, 1);
    
    // Desired screen size in NDC
    float desiredNDCSize = m_screenSize / viewportHeight;
    
    // World scale = NDC size * depth * projection scale factor
    return desiredNDCSize * depth * projScale;
}

// ---- Axis Constraint Implementation ----

void TransformGizmo::setAxisConstraint(AxisConstraint constraint) {
    if (m_axisConstraint != constraint) {
        m_axisConstraint = constraint;
        emit axisConstraintChanged(constraint);
    }
}

bool TransformGizmo::isAxisConstrained(int axis) const {
    switch (m_axisConstraint) {
        case AxisConstraint::X: return axis != 0;
        case AxisConstraint::Y: return axis != 1;
        case AxisConstraint::Z: return axis != 2;
        case AxisConstraint::PlaneXY: return axis == 2;
        case AxisConstraint::PlaneXZ: return axis == 1;
        case AxisConstraint::PlaneYZ: return axis == 0;
        default: return false;
    }
}

QVector3D TransformGizmo::constraintDirection() const {
    switch (m_axisConstraint) {
        case AxisConstraint::X: return QVector3D(1, 0, 0);
        case AxisConstraint::Y: return QVector3D(0, 1, 0);
        case AxisConstraint::Z: return QVector3D(0, 0, 1);
        default: return QVector3D(1, 1, 1);
    }
}

QVector3D TransformGizmo::constraintPlaneNormal() const {
    switch (m_axisConstraint) {
        case AxisConstraint::PlaneXY: return QVector3D(0, 0, 1);
        case AxisConstraint::PlaneXZ: return QVector3D(0, 1, 0);
        case AxisConstraint::PlaneYZ: return QVector3D(1, 0, 0);
        default: return QVector3D(0, 1, 0);
    }
}

// ---- Coordinate Space Implementation ----

void TransformGizmo::setCoordinateSpace(CoordinateSpace space) {
    if (m_coordinateSpace != space) {
        m_coordinateSpace = space;
        emit coordinateSpaceChanged(space);
    }
}

void TransformGizmo::toggleCoordinateSpace() {
    setCoordinateSpace(m_coordinateSpace == CoordinateSpace::World 
                       ? CoordinateSpace::Local 
                       : CoordinateSpace::World);
}

// ---- Pivot Point Implementation ----

void TransformGizmo::setPivotPoint(PivotPoint pivot) {
    if (m_pivotPoint != pivot) {
        m_pivotPoint = pivot;
        emit pivotPointChanged(pivot);
    }
}

void TransformGizmo::setCustomPivotPosition(const QVector3D& pos) {
    m_customPivot = pos;
}

void TransformGizmo::cyclePivotPoint() {
    int current = static_cast<int>(m_pivotPoint);
    int next = (current + 1) % 5;  // 5 pivot point options
    setPivotPoint(static_cast<PivotPoint>(next));
}

// ---- Numeric Input Implementation ----

void TransformGizmo::startNumericInput() {
    m_numericInputActive = true;
    m_numericInput.clear();
}

void TransformGizmo::endNumericInput(bool apply) {
    if (apply && !m_numericInput.isEmpty()) {
        emit numericInputConfirmed(numericInputVector());
    }
    m_numericInputActive = false;
    m_numericInput.clear();
}

void TransformGizmo::appendNumericInput(QChar c) {
    if (m_numericInputActive) {
        // Allow digits, minus, period, and comma for vector input
        if (c.isDigit() || c == '-' || c == '.' || c == ',') {
            m_numericInput.append(c);
        }
    }
}

void TransformGizmo::backspaceNumericInput() {
    if (m_numericInputActive && !m_numericInput.isEmpty()) {
        m_numericInput.chop(1);
    }
}

double TransformGizmo::numericInputValue() const {
    if (m_numericInput.isEmpty()) {
        return 0.0;
    }
    bool ok;
    double value = m_numericInput.toDouble(&ok);
    return ok ? value : 0.0;
}

QVector3D TransformGizmo::numericInputVector() const {
    if (m_numericInput.isEmpty()) {
        return QVector3D(0, 0, 0);
    }
    
    // Parse comma-separated values like "5,10,3"
    QStringList parts = m_numericInput.split(',');
    float x = 0, y = 0, z = 0;
    
    if (parts.size() >= 1) {
        bool ok;
        x = parts[0].toFloat(&ok);
        if (!ok) x = 0;
    }
    if (parts.size() >= 2) {
        bool ok;
        y = parts[1].toFloat(&ok);
        if (!ok) y = 0;
    }
    if (parts.size() >= 3) {
        bool ok;
        z = parts[2].toFloat(&ok);
        if (!ok) z = 0;
    }
    
    // If only one value, apply to constrained axis
    if (parts.size() == 1) {
        float val = x;
        switch (m_axisConstraint) {
            case AxisConstraint::X: return QVector3D(val, 0, 0);
            case AxisConstraint::Y: return QVector3D(0, val, 0);
            case AxisConstraint::Z: return QVector3D(0, 0, val);
            default: return QVector3D(val, val, val);
        }
    }
    
    return QVector3D(x, y, z);
}

// ---- Visual Settings ----

void TransformGizmo::setConstraintColor(const QColor& color) {
    m_constraintColor = color;
}

// ---- Constraint Indicator Geometry (stub implementations) ----

void TransformGizmo::createConstraintIndicatorGeometry() {
    // Create visual indicator for axis constraints
    // This would show a line or plane to indicate the constraint
    // For now, this is a placeholder
}

void TransformGizmo::renderConstraintIndicator(const QMatrix4x4& mvp, float scale) {
    Q_UNUSED(mvp);
    Q_UNUSED(scale);
    // Render constraint indicator if active
    // Placeholder for future implementation
}

void TransformGizmo::renderNumericInputOverlay() {
    // Render numeric input text overlay
    // This would typically be handled by the viewport's paintEvent
    // Placeholder for future implementation
}

} // namespace dc
