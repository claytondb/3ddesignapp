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
    
    // Wrap yaw to [0, 360) using fmod for efficiency with extreme values
    m_yaw = std::fmod(m_yaw, 360.0f);
    if (m_yaw < 0.0f) m_yaw += 360.0f;
    
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
    
    float newYaw = m_yaw;
    float newPitch = m_pitch;
    
    switch (view) {
        case StandardView::Front:
            newYaw = 0.0f;
            newPitch = 0.0f;
            break;
            
        case StandardView::Back:
            newYaw = 180.0f;
            newPitch = 0.0f;
            break;
            
        case StandardView::Top:
            newYaw = 0.0f;
            newPitch = 89.9f;  // Nearly 90 to avoid gimbal lock
            break;
            
        case StandardView::Bottom:
            newYaw = 0.0f;
            newPitch = -89.9f;
            break;
            
        case StandardView::Left:
            newYaw = -90.0f;
            newPitch = 0.0f;
            break;
            
        case StandardView::Right:
            newYaw = 90.0f;
            newPitch = 0.0f;
            break;
            
        case StandardView::Isometric:
            newYaw = 45.0f;
            newPitch = 35.264f;  // arctan(1/sqrt(2)) for true isometric
            break;
    }
    
    // Calculate new position
    float yawRad = qDegreesToRadians(newYaw);
    float pitchRad = qDegreesToRadians(newPitch);
    
    float cosPitch = qCos(pitchRad);
    QVector3D newPosition;
    newPosition.setX(target.x() + distance * cosPitch * qSin(yawRad));
    newPosition.setY(target.y() + distance * qSin(pitchRad));
    newPosition.setZ(target.z() + distance * cosPitch * qCos(yawRad));
    
    if (m_animationEnabled) {
        startAnimation(newPosition, target, newYaw, newPitch, distance);
    } else {
        m_yaw = newYaw;
        m_pitch = newPitch;
        m_position = newPosition;
        updateViewMatrix();
    }
}

void Camera::fitToView(const BoundingBox& bounds, float padding)
{
    if (!bounds.isValid()) {
        return;
    }
    
    // Calculate new target (center of bounding box)
    QVector3D newTarget = bounds.center();
    
    // Calculate required distance to fit bounding box
    float diagonal = bounds.diagonal() * padding;
    float newRadius = m_orbitRadius;
    
    if (m_isPerspective) {
        // For perspective, calculate distance based on FOV
        float fovRad = qDegreesToRadians(m_fov);
        float halfFov = fovRad * 0.5f;
        newRadius = (diagonal * 0.5f) / qTan(halfFov);
    } else {
        // For orthographic, adjust the view width
        m_orthoWidth = diagonal;
        newRadius = diagonal;
        updateProjectionMatrix();
    }
    
    // Clamp distance
    newRadius = std::clamp(newRadius, m_minDistance, m_maxDistance);
    
    // Calculate new position with current orientation
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);
    
    float cosPitch = qCos(pitchRad);
    QVector3D newPosition;
    newPosition.setX(newTarget.x() + newRadius * cosPitch * qSin(yawRad));
    newPosition.setY(newTarget.y() + newRadius * qSin(pitchRad));
    newPosition.setZ(newTarget.z() + newRadius * cosPitch * qCos(yawRad));
    
    if (m_animationEnabled) {
        startAnimation(newPosition, newTarget, m_yaw, m_pitch, newRadius);
    } else {
        m_target = newTarget;
        m_orbitRadius = newRadius;
        m_position = newPosition;
        updateViewMatrix();
    }
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

// ---- Animation Methods ----

void Camera::startAnimation(const QVector3D& targetPos, const QVector3D& targetTarget,
                            float targetYaw, float targetPitch, float targetRadius)
{
    // Store start state
    m_startPosition = m_position;
    m_startTarget = m_target;
    m_startYaw = m_yaw;
    m_startPitch = m_pitch;
    m_startRadius = m_orbitRadius;
    
    // Store end state
    m_endPosition = targetPos;
    m_endTarget = targetTarget;
    m_endYaw = targetYaw;
    m_endPitch = targetPitch;
    m_endRadius = targetRadius;
    
    // Handle yaw wraparound (take shortest path)
    float yawDiff = m_endYaw - m_startYaw;
    if (yawDiff > 180.0f) {
        m_startYaw += 360.0f;
    } else if (yawDiff < -180.0f) {
        m_endYaw += 360.0f;
    }
    
    // Start animation
    m_animationTime = 0.0f;
    m_isAnimating = true;
}

bool Camera::updateAnimation(float deltaTime)
{
    if (!m_isAnimating) {
        return false;
    }
    
    m_animationTime += deltaTime;
    float t = m_animationTime / m_animationDuration;
    
    if (t >= 1.0f) {
        // Animation complete - snap to final values
        m_position = m_endPosition;
        m_target = m_endTarget;
        m_yaw = std::fmod(m_endYaw, 360.0f);
        if (m_yaw < 0.0f) m_yaw += 360.0f;
        m_pitch = m_endPitch;
        m_orbitRadius = m_endRadius;
        m_isAnimating = false;
        updateViewMatrix();
        return false;
    }
    
    // Apply easing
    float easedT = easeInOutCubic(t);
    
    // Interpolate values
    m_position = m_startPosition + (m_endPosition - m_startPosition) * easedT;
    m_target = m_startTarget + (m_endTarget - m_startTarget) * easedT;
    m_yaw = m_startYaw + (m_endYaw - m_startYaw) * easedT;
    m_pitch = m_startPitch + (m_endPitch - m_startPitch) * easedT;
    m_orbitRadius = m_startRadius + (m_endRadius - m_startRadius) * easedT;
    
    updateViewMatrix();
    return true;
}

void Camera::applyAnimationState()
{
    // Recalculate position from orbit parameters
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);
    
    float cosPitch = qCos(pitchRad);
    m_position.setX(m_target.x() + m_orbitRadius * cosPitch * qSin(yawRad));
    m_position.setY(m_target.y() + m_orbitRadius * qSin(pitchRad));
    m_position.setZ(m_target.z() + m_orbitRadius * cosPitch * qCos(yawRad));
    
    updateViewMatrix();
}

float Camera::easeInOutCubic(float t)
{
    // Smooth ease-in-out curve
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = 2.0f * t - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }
}

} // namespace dc
