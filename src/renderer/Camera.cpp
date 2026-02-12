/**
 * @file Camera.cpp
 * @brief Implementation of orbit camera
 */

#include "Camera.h"
#include <QtMath>
#include <algorithm>

namespace dc {

Camera::Camera()
{
    updateViewMatrix();
    updateProjectionMatrix();
}

QVector3D Camera::forwardVector() const
{
    return (m_target - m_position).normalized();
}

QVector3D Camera::rightVector() const
{
    return QVector3D::crossProduct(forwardVector(), m_up).normalized();
}

void Camera::orbit(float deltaX, float deltaY)
{
    // Apply sensitivity
    m_yaw += deltaX * m_orbitSensitivity;
    m_pitch += deltaY * m_orbitSensitivity;
    
    // Clamp pitch to avoid gimbal lock
    clampPitch();
    
    // Wrap yaw to [0, 360)
    while (m_yaw < 0.0f) m_yaw += 360.0f;
    while (m_yaw >= 360.0f) m_yaw -= 360.0f;
    
    // Recalculate camera position from spherical coordinates
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);
    
    // Convert spherical to Cartesian
    float cosPitch = qCos(pitchRad);
    m_position.setX(m_target.x() + m_orbitRadius * cosPitch * qSin(yawRad));
    m_position.setY(m_target.y() + m_orbitRadius * qSin(pitchRad));
    m_position.setZ(m_target.z() + m_orbitRadius * cosPitch * qCos(yawRad));
    
    updateViewMatrix();
}

void Camera::pan(float deltaX, float deltaY)
{
    // Get view-aligned axes
    QVector3D right = rightVector();
    QVector3D up = QVector3D::crossProduct(right, forwardVector()).normalized();
    
    // Scale pan by distance for consistent feel
    float panScale = m_panSensitivity * m_orbitRadius;
    
    // Apply pan
    QVector3D offset = right * (-deltaX * panScale) + up * (deltaY * panScale);
    m_position += offset;
    m_target += offset;
    
    updateViewMatrix();
}

void Camera::zoom(float delta)
{
    // Exponential zoom for consistent feel
    float zoomFactor = qExp(-delta * m_zoomSensitivity);
    float newRadius = m_orbitRadius * zoomFactor;
    
    // Clamp to min/max distance
    m_orbitRadius = std::clamp(newRadius, m_minDistance, m_maxDistance);
    
    // Recalculate position
    QVector3D direction = (m_position - m_target).normalized();
    m_position = m_target + direction * m_orbitRadius;
    
    updateViewMatrix();
}

void Camera::dolly(float delta)
{
    QVector3D forward = forwardVector();
    
    // Move along view direction
    m_position += forward * delta;
    m_target += forward * delta;
    
    updateViewMatrix();
}

void Camera::setStandardView(StandardView view)
{
    // Preserve current target and distance
    float distance = m_orbitRadius;
    QVector3D target = m_target;
    
    switch (view) {
        case StandardView::Front:
            m_yaw = 0.0f;
            m_pitch = 0.0f;
            break;
            
        case StandardView::Back:
            m_yaw = 180.0f;
            m_pitch = 0.0f;
            break;
            
        case StandardView::Top:
            m_yaw = 0.0f;
            m_pitch = 89.9f;  // Nearly 90 to avoid gimbal lock
            break;
            
        case StandardView::Bottom:
            m_yaw = 0.0f;
            m_pitch = -89.9f;
            break;
            
        case StandardView::Left:
            m_yaw = -90.0f;
            m_pitch = 0.0f;
            break;
            
        case StandardView::Right:
            m_yaw = 90.0f;
            m_pitch = 0.0f;
            break;
            
        case StandardView::Isometric:
            m_yaw = 45.0f;
            m_pitch = 35.264f;  // arctan(1/sqrt(2)) for true isometric
            break;
    }
    
    // Recalculate position
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);
    
    float cosPitch = qCos(pitchRad);
    m_position.setX(target.x() + distance * cosPitch * qSin(yawRad));
    m_position.setY(target.y() + distance * qSin(pitchRad));
    m_position.setZ(target.z() + distance * cosPitch * qCos(yawRad));
    
    updateViewMatrix();
}

void Camera::fitToView(const BoundingBox& bounds, float padding)
{
    if (!bounds.isValid()) {
        return;
    }
    
    // Center on bounding box
    m_target = bounds.center();
    
    // Calculate required distance to fit bounding box
    float diagonal = bounds.diagonal() * padding;
    
    if (m_isPerspective) {
        // For perspective, calculate distance based on FOV
        float fovRad = qDegreesToRadians(m_fov);
        float halfFov = fovRad * 0.5f;
        m_orbitRadius = (diagonal * 0.5f) / qTan(halfFov);
    } else {
        // For orthographic, adjust the view width
        m_orthoWidth = diagonal;
        m_orbitRadius = diagonal;
        updateProjectionMatrix();
    }
    
    // Clamp distance
    m_orbitRadius = std::clamp(m_orbitRadius, m_minDistance, m_maxDistance);
    
    // Recalculate position with current orientation
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);
    
    float cosPitch = qCos(pitchRad);
    m_position.setX(m_target.x() + m_orbitRadius * cosPitch * qSin(yawRad));
    m_position.setY(m_target.y() + m_orbitRadius * qSin(pitchRad));
    m_position.setZ(m_target.z() + m_orbitRadius * cosPitch * qCos(yawRad));
    
    updateViewMatrix();
}

void Camera::lookAt(const QVector3D& position, const QVector3D& target, const QVector3D& up)
{
    m_position = position;
    m_target = target;
    m_up = up.normalized();
    
    // Calculate orbit parameters from position/target
    QVector3D direction = position - target;
    m_orbitRadius = direction.length();
    
    if (m_orbitRadius > 0.0001f) {
        direction /= m_orbitRadius;
        
        // Extract yaw and pitch from direction
        m_pitch = qRadiansToDegrees(qAsin(direction.y()));
        m_yaw = qRadiansToDegrees(qAtan2(direction.x(), direction.z()));
    }
    
    updateViewMatrix();
}

void Camera::reset()
{
    m_position = QVector3D(0.0f, 5.0f, 10.0f);
    m_target = QVector3D(0.0f, 0.0f, 0.0f);
    m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_yaw = 0.0f;
    m_pitch = -30.0f;
    m_orbitRadius = 10.0f;
    
    updateViewMatrix();
    updateProjectionMatrix();
}

void Camera::setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    m_isPerspective = true;
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    
    updateProjectionMatrix();
}

void Camera::setOrthographic(float width, float aspectRatio, float nearPlane, float farPlane)
{
    m_isPerspective = false;
    m_orthoWidth = width;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    
    updateProjectionMatrix();
}

void Camera::setAspectRatio(float aspectRatio)
{
    m_aspectRatio = aspectRatio;
    updateProjectionMatrix();
}

void Camera::toggleProjectionMode()
{
    m_isPerspective = !m_isPerspective;
    
    if (!m_isPerspective) {
        // Set ortho width based on current view
        m_orthoWidth = m_orbitRadius * qTan(qDegreesToRadians(m_fov * 0.5f)) * 2.0f;
    }
    
    updateProjectionMatrix();
}

void Camera::updateViewMatrix()
{
    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(m_position, m_target, m_up);
}

void Camera::updateProjectionMatrix()
{
    m_projectionMatrix.setToIdentity();
    
    if (m_isPerspective) {
        m_projectionMatrix.perspective(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
    } else {
        float halfWidth = m_orthoWidth * 0.5f;
        float halfHeight = halfWidth / m_aspectRatio;
        m_projectionMatrix.ortho(-halfWidth, halfWidth, 
                                  -halfHeight, halfHeight, 
                                  m_nearPlane, m_farPlane);
    }
}

void Camera::clampPitch()
{
    // Clamp to just under 90 degrees to prevent gimbal lock
    const float maxPitch = 89.0f;
    m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);
}

} // namespace dc
