/**
 * @file Camera.h
 * @brief Orbit camera with view and projection matrices for 3D viewport
 * 
 * Provides an arcball-style orbit camera with support for:
 * - Orbit, pan, and zoom operations
 * - Standard view presets (Front, Back, Top, etc.)
 * - Fit-to-view functionality
 */

#pragma once

#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>

namespace dc {

/**
 * @brief Axis-aligned bounding box for fit-to-view calculations
 */
struct BoundingBox {
    QVector3D min{-1.0f, -1.0f, -1.0f};
    QVector3D max{1.0f, 1.0f, 1.0f};
    
    QVector3D center() const { return (min + max) * 0.5f; }
    QVector3D size() const { return max - min; }
    float diagonal() const { return size().length(); }
    
    bool isValid() const { 
        return min.x() <= max.x() && min.y() <= max.y() && min.z() <= max.z(); 
    }
};

/**
 * @brief Standard view orientations
 */
enum class StandardView {
    Front,      // Looking along -Z
    Back,       // Looking along +Z
    Top,        // Looking along -Y
    Bottom,     // Looking along +Y
    Left,       // Looking along +X
    Right,      // Looking along -X
    Isometric   // 45° view from corner
};

/**
 * @brief Orbit camera for 3D viewport navigation
 * 
 * Uses a target-based orbit system where the camera rotates around
 * a focal point. Supports smooth transitions between views.
 */
class Camera {
public:
    Camera();
    ~Camera() = default;
    
    // Non-copyable but movable
    Camera(const Camera&) = default;
    Camera& operator=(const Camera&) = default;
    Camera(Camera&&) = default;
    Camera& operator=(Camera&&) = default;
    
    // ---- Animation ----
    
    /**
     * @brief Check if camera is currently animating
     */
    bool isAnimating() const { return m_isAnimating; }
    
    /**
     * @brief Update animation state (call each frame)
     * @param deltaTime Time since last update in seconds
     * @return true if animation is still in progress
     */
    bool updateAnimation(float deltaTime);
    
    /**
     * @brief Set animation duration for view transitions
     * @param seconds Duration in seconds (default 0.3)
     */
    void setAnimationDuration(float seconds) { m_animationDuration = seconds; }
    
    /**
     * @brief Enable or disable smooth animation for view changes
     */
    void setAnimationEnabled(bool enabled) { m_animationEnabled = enabled; }
    
    // ---- Matrix Access ----
    
    /**
     * @brief Get the view matrix (world -> camera space)
     */
    const QMatrix4x4& viewMatrix() const { return m_viewMatrix; }
    
    /**
     * @brief Get the projection matrix (camera -> clip space)
     */
    const QMatrix4x4& projectionMatrix() const { return m_projectionMatrix; }
    
    /**
     * @brief Get combined view-projection matrix
     */
    QMatrix4x4 viewProjectionMatrix() const { return m_projectionMatrix * m_viewMatrix; }
    
    /**
     * @brief Get the camera's world position
     */
    QVector3D position() const { return m_position; }
    
    /**
     * @brief Get the camera's target/focal point
     */
    QVector3D target() const { return m_target; }
    
    /**
     * @brief Get the camera's up vector
     */
    QVector3D upVector() const { return m_up; }
    
    /**
     * @brief Get the forward direction (normalized)
     */
    QVector3D forwardVector() const;
    
    /**
     * @brief Get the right direction (normalized)
     */
    QVector3D rightVector() const;
    
    // ---- Navigation ----
    
    /**
     * @brief Orbit the camera around the target point
     * @param deltaX Horizontal rotation in degrees
     * @param deltaY Vertical rotation in degrees
     */
    void orbit(float deltaX, float deltaY);
    
    /**
     * @brief Pan the camera parallel to the view plane
     * @param deltaX Horizontal pan (screen-space)
     * @param deltaY Vertical pan (screen-space)
     */
    void pan(float deltaX, float deltaY);
    
    /**
     * @brief Zoom by moving camera toward/away from target
     * @param delta Positive = zoom in, negative = zoom out
     */
    void zoom(float delta);
    
    /**
     * @brief Dolly (move camera forward/backward along view axis)
     * @param delta Distance to move (positive = forward)
     */
    void dolly(float delta);
    
    // ---- View Setup ----
    
    /**
     * @brief Set to a standard view orientation
     * @param view The standard view to apply
     */
    void setStandardView(StandardView view);
    
    /**
     * @brief Fit the view to show the given bounding box
     * @param bounds The bounding box to fit
     * @param padding Extra padding factor (1.0 = tight fit, 1.2 = 20% padding)
     */
    void fitToView(const BoundingBox& bounds, float padding = 1.1f);
    
    /**
     * @brief Set camera to look at a specific target from a position
     * @param position Camera position
     * @param target Point to look at
     * @param up Up vector (default Y-up)
     */
    void lookAt(const QVector3D& position, const QVector3D& target, 
                const QVector3D& up = QVector3D(0, 1, 0));
    
    /**
     * @brief Reset camera to default state
     */
    void reset();
    
    // ---- Projection Setup ----
    
    /**
     * @brief Set perspective projection parameters
     * @param fov Vertical field of view in degrees
     * @param aspectRatio Width / Height
     * @param nearPlane Near clipping plane distance
     * @param farPlane Far clipping plane distance
     */
    void setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);
    
    /**
     * @brief Set orthographic projection parameters
     * @param width View width in world units
     * @param aspectRatio Width / Height
     * @param nearPlane Near clipping plane distance
     * @param farPlane Far clipping plane distance
     */
    void setOrthographic(float width, float aspectRatio, float nearPlane, float farPlane);
    
    /**
     * @brief Update aspect ratio (e.g., on viewport resize)
     * @param aspectRatio New width / height ratio
     */
    void setAspectRatio(float aspectRatio);
    
    /**
     * @brief Toggle between perspective and orthographic projection
     */
    void toggleProjectionMode();
    
    /**
     * @brief Check if using perspective projection
     */
    bool isPerspective() const { return m_isPerspective; }
    
    // ---- Settings ----
    
    /**
     * @brief Set orbit sensitivity
     * @param sensitivity Degrees per pixel of mouse movement
     */
    void setOrbitSensitivity(float sensitivity) noexcept { m_orbitSensitivity = sensitivity; }
    
    /**
     * @brief Set pan sensitivity
     * @param sensitivity World units per pixel of mouse movement
     */
    void setPanSensitivity(float sensitivity) noexcept { m_panSensitivity = sensitivity; }
    
    /**
     * @brief Set zoom sensitivity
     * @param sensitivity Scale factor per scroll step
     */
    void setZoomSensitivity(float sensitivity) noexcept { m_zoomSensitivity = sensitivity; }
    
    /**
     * @brief Set minimum distance from target
     */
    void setMinDistance(float distance) noexcept { m_minDistance = distance; }
    
    /**
     * @brief Set maximum distance from target
     */
    void setMaxDistance(float distance) noexcept { m_maxDistance = distance; }
    
    /**
     * @brief Get distance from camera to target
     */
    float distance() const { return (m_position - m_target).length(); }

private:
    void updateViewMatrix();
    void updateProjectionMatrix();
    void clampPitch();
    void startAnimation(const QVector3D& targetPos, const QVector3D& targetTarget, 
                        float targetYaw, float targetPitch, float targetRadius);
    void applyAnimationState();
    static float easeInOutCubic(float t);
    
    // Camera state
    QVector3D m_position{0.0f, 5.0f, 10.0f};
    QVector3D m_target{0.0f, 0.0f, 0.0f};
    QVector3D m_up{0.0f, 1.0f, 0.0f};
    
    // Orbit angles (in degrees)
    float m_yaw = 0.0f;     // Horizontal rotation
    float m_pitch = -30.0f; // Vertical rotation (negative = looking down)
    float m_orbitRadius = 10.0f;
    
    // Animation state
    bool m_animationEnabled = true;
    bool m_isAnimating = false;
    float m_animationDuration = 0.3f;  // seconds
    float m_animationTime = 0.0f;
    
    // Animation start/end values
    QVector3D m_startPosition;
    QVector3D m_startTarget;
    float m_startYaw = 0.0f;
    float m_startPitch = 0.0f;
    float m_startRadius = 0.0f;
    
    QVector3D m_endPosition;
    QVector3D m_endTarget;
    float m_endYaw = 0.0f;
    float m_endPitch = 0.0f;
    float m_endRadius = 0.0f;
    
    // Matrices
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;
    
    // Projection settings
    bool m_isPerspective = true;
    float m_fov = 45.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 10000.0f;
    float m_orthoWidth = 10.0f;
    
    // Navigation sensitivity
    float m_orbitSensitivity = 0.3f;
    float m_panSensitivity = 0.01f;
    float m_zoomSensitivity = 0.1f;
    
    // Distance constraints
    float m_minDistance = 0.1f;
    float m_maxDistance = 100000.0f;
};

} // namespace dc
