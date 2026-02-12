/**
 * @file AlignmentTool.cpp
 * @brief Implementation of interactive alignment tool
 */

#include "AlignmentTool.h"
#include "../../renderer/TransformGizmo.h"
#include "../../geometry/Alignment.h"
#include "../../geometry/MeshData.h"

#include <QtMath>
#include <QMatrix4x4>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace dc {

AlignmentTool::AlignmentTool(Viewport* viewport, QObject* parent)
    : QObject(parent)
    , m_viewport(viewport)
    , m_gizmo(std::make_unique<TransformGizmo>())
{
}

AlignmentTool::~AlignmentTool() = default;

void AlignmentTool::activate() {
    if (!m_active) {
        m_active = true;
        resetTransform();
        emit activeChanged(true);
    }
}

void AlignmentTool::deactivate() {
    if (m_active) {
        m_active = false;
        clearTarget();
        emit activeChanged(false);
    }
}

void AlignmentTool::setTargetMesh(uint64_t meshId, std::shared_ptr<dc3d::geometry::MeshData> mesh) {
    m_targetMeshId = meshId;
    m_targetMesh = mesh;
    resetTransform();
    
    if (mesh) {
        // Position gizmo at mesh centroid
        glm::vec3 centroid = mesh->centroid();
        m_gizmo->setPosition(QVector3D(centroid.x, centroid.y, centroid.z));
    }
}

void AlignmentTool::clearTarget() {
    m_targetMeshId = 0;
    m_targetMesh.reset();
    resetTransform();
}

void AlignmentTool::setTransformMode(TransformMode mode) {
    if (m_transformMode != mode) {
        m_transformMode = mode;
        m_gizmo->setMode(static_cast<int>(mode));
        emit transformModeChanged(mode);
    }
}

void AlignmentTool::setAxisConstraint(AxisConstraint constraint) {
    if (m_axisConstraint != constraint) {
        m_axisConstraint = constraint;
        emit axisConstraintChanged(constraint);
    }
}

void AlignmentTool::setTransformSpace(TransformSpace space) {
    m_transformSpace = space;
}

void AlignmentTool::setSnapSettings(const SnapSettings& settings) {
    m_snapSettings = settings;
}

void AlignmentTool::setSnapEnabled(bool enabled) {
    m_snapSettings.enabled = enabled;
}

void AlignmentTool::setTranslation(const QVector3D& t) {
    m_translation = t;
    updatePreviewTransform();
    emit transformChanged();
}

void AlignmentTool::setRotation(const QVector3D& r) {
    m_rotation = r;
    updatePreviewTransform();
    emit transformChanged();
}

void AlignmentTool::setScale(const QVector3D& s) {
    m_scale = s;
    updatePreviewTransform();
    emit transformChanged();
}

QMatrix4x4 AlignmentTool::transformMatrix() const {
    QMatrix4x4 matrix;
    matrix.translate(m_translation);
    matrix.rotate(m_rotation.x(), 1, 0, 0);
    matrix.rotate(m_rotation.y(), 0, 1, 0);
    matrix.rotate(m_rotation.z(), 0, 0, 1);
    matrix.scale(m_scale);
    return matrix;
}

void AlignmentTool::applyTransform() {
    if (!m_targetMesh) {
        return;
    }
    
    // Convert QMatrix4x4 to glm::mat4
    QMatrix4x4 qtMatrix = transformMatrix();
    glm::mat4 glmMatrix;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            glmMatrix[j][i] = qtMatrix(i, j);
        }
    }
    
    // Apply transformation using Alignment class
    dc3d::geometry::AlignmentOptions options;
    options.preview = false;
    
    dc3d::geometry::AlignmentResult result = 
        dc3d::geometry::Alignment::alignInteractive(*m_targetMesh, glmMatrix, options);
    
    // Reset tool state
    resetTransform();
    updateGizmoPosition();
    
    emit transformApplied(result);
}

void AlignmentTool::cancelTransform() {
    resetTransform();
    emit transformCancelled();
}

void AlignmentTool::resetTransform() {
    m_translation = QVector3D(0, 0, 0);
    m_rotation = QVector3D(0, 0, 0);
    m_scale = QVector3D(1, 1, 1);
    emit transformChanged();
}

void AlignmentTool::handleMousePress(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) {
    Q_UNUSED(modifiers);
    
    if (!m_active || !m_targetMesh) {
        return;
    }
    
    if (buttons & Qt::LeftButton) {
        // Check if clicking on gizmo
        GizmoHitResult hit = m_gizmo->hitTest(pos, m_viewport);
        if (hit.hit) {
            m_dragging = true;
            m_dragStart = pos;
            
            // Store starting values based on mode
            switch (m_transformMode) {
                case TransformMode::Translate:
                    m_dragStartValue = m_translation;
                    break;
                case TransformMode::Rotate:
                    m_dragStartValue = m_rotation;
                    break;
                case TransformMode::Scale:
                    m_dragStartValue = m_scale;
                    break;
            }
            
            // Set axis constraint from hit
            m_gizmo->setActiveAxis(hit.axis);
        }
    }
}

void AlignmentTool::handleMouseMove(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) {
    Q_UNUSED(buttons);
    Q_UNUSED(modifiers);
    
    if (!m_active || !m_targetMesh) {
        return;
    }
    
    if (m_dragging) {
        // Calculate delta
        QPoint delta = pos - m_dragStart;
        float sensitivity = 0.01f;
        
        QVector3D deltaValue(0, 0, 0);
        
        // Determine which components to modify based on active axis
        int activeAxis = m_gizmo->activeAxis();
        if (activeAxis >= 0 && activeAxis < 3) {
            // Single axis: use horizontal mouse movement
            float d = delta.x() * sensitivity;
            deltaValue[activeAxis] = d;
        } else if (activeAxis == 3) {
            // XY plane
            deltaValue.setX(delta.x() * sensitivity);
            deltaValue.setY(-delta.y() * sensitivity);
        } else if (activeAxis == 4) {
            // XZ plane
            deltaValue.setX(delta.x() * sensitivity);
            deltaValue.setZ(-delta.y() * sensitivity);
        } else if (activeAxis == 5) {
            // YZ plane
            deltaValue.setY(delta.x() * sensitivity);
            deltaValue.setZ(-delta.y() * sensitivity);
        } else {
            // Free movement
            deltaValue.setX(delta.x() * sensitivity);
            deltaValue.setY(-delta.y() * sensitivity);
        }
        
        // Apply based on mode
        switch (m_transformMode) {
            case TransformMode::Translate:
                m_translation = m_dragStartValue + deltaValue;
                if (m_snapSettings.enabled) {
                    m_translation = applySnap(m_translation, TransformMode::Translate);
                }
                break;
                
            case TransformMode::Rotate:
                // Rotation uses degrees, scale delta appropriately
                deltaValue *= 10.0f;
                m_rotation = m_dragStartValue + deltaValue;
                if (m_snapSettings.enabled) {
                    m_rotation = applySnap(m_rotation, TransformMode::Rotate);
                }
                break;
                
            case TransformMode::Scale:
                // Scale is multiplicative from 1.0
                QVector3D scaleOffset = deltaValue;
                m_scale = m_dragStartValue + scaleOffset;
                // Clamp scale to positive values
                m_scale.setX(qMax(0.01f, m_scale.x()));
                m_scale.setY(qMax(0.01f, m_scale.y()));
                m_scale.setZ(qMax(0.01f, m_scale.z()));
                if (m_snapSettings.enabled) {
                    m_scale = applySnap(m_scale, TransformMode::Scale);
                }
                break;
        }
        
        updatePreviewTransform();
        emit transformChanged();
    } else {
        // Hover highlight
        GizmoHitResult hit = m_gizmo->hitTest(pos, m_viewport);
        m_gizmo->setHoverAxis(hit.hit ? hit.axis : -1);
    }
}

void AlignmentTool::handleMouseRelease(const QPoint& pos, Qt::MouseButtons buttons) {
    Q_UNUSED(pos);
    Q_UNUSED(buttons);
    
    m_dragging = false;
    m_gizmo->setActiveAxis(-1);
}

void AlignmentTool::updateGizmoPosition() {
    if (m_targetMesh) {
        glm::vec3 centroid = m_targetMesh->centroid();
        m_gizmo->setPosition(QVector3D(centroid.x, centroid.y, centroid.z) + m_translation);
    }
}

QVector3D AlignmentTool::applySnap(const QVector3D& value, TransformMode mode) const {
    QVector3D result = value;
    float snap = 1.0f;
    
    switch (mode) {
        case TransformMode::Translate:
            snap = m_snapSettings.translateSnap;
            break;
        case TransformMode::Rotate:
            snap = m_snapSettings.rotateSnap;
            break;
        case TransformMode::Scale:
            snap = m_snapSettings.scaleSnap;
            break;
    }
    
    if (snap > 0.0f) {
        result.setX(qRound(value.x() / snap) * snap);
        result.setY(qRound(value.y() / snap) * snap);
        result.setZ(qRound(value.z() / snap) * snap);
    }
    
    return result;
}

void AlignmentTool::updatePreviewTransform() {
    updateGizmoPosition();
    
    // In a full implementation, this would update the viewport preview
    // to show the mesh with the current transformation applied
}

} // namespace dc
