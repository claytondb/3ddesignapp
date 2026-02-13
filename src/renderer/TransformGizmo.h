/**
 * @file TransformGizmo.h
 * @brief Visual 3D gizmo for interactive transformations
 * 
 * Provides a 3D manipulator for translate, rotate, and scale operations:
 * - Visual rendering with axis colors (X=red, Y=green, Z=blue)
 * - Mouse interaction for dragging
 * - Axis highlighting on hover
 * - Screen-space consistent size
 */

#pragma once

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QColor>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <memory>

namespace dc {

class Viewport;

/**
 * @brief Result of gizmo hit testing
 */
struct GizmoHitResult {
    bool hit = false;
    int axis = -1;      ///< 0=X, 1=Y, 2=Z, 3=XY, 4=XZ, 5=YZ, 6=all
    float distance = 0.0f;
};

/**
 * @brief Gizmo mode
 */
enum class GizmoMode {
    Translate = 0,
    Rotate = 1,
    Scale = 2
};

/**
 * @brief Visual 3D transform gizmo
 */
class TransformGizmo : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(TransformGizmo)
    
public:
    explicit TransformGizmo(QObject* parent = nullptr);
    ~TransformGizmo() override;
    
    // ---- Rendering ----
    
    /**
     * @brief Initialize OpenGL resources
     */
    void initialize();
    
    /**
     * @brief Clean up OpenGL resources
     * @note Must be called while OpenGL context is current, before destruction
     */
    void cleanup();
    
    /**
     * @brief Render the gizmo
     * @param view View matrix
     * @param projection Projection matrix
     * @param viewportSize Viewport size for screen-space sizing
     */
    void render(const QMatrix4x4& view, const QMatrix4x4& projection, const QSize& viewportSize);
    
    /**
     * @brief Check if gizmo is initialized
     */
    bool isInitialized() const { return m_initialized; }
    
    // ---- Position and Mode ----
    
    /**
     * @brief Set gizmo position in world space
     */
    void setPosition(const QVector3D& pos);
    
    /**
     * @brief Get gizmo position
     */
    QVector3D position() const { return m_position; }
    
    /**
     * @brief Set gizmo orientation
     */
    void setOrientation(const QQuaternion& orientation);
    
    /**
     * @brief Get gizmo orientation
     */
    QQuaternion orientation() const { return m_orientation; }
    
    /**
     * @brief Set gizmo mode
     */
    void setMode(int mode);
    void setMode(GizmoMode mode) { setMode(static_cast<int>(mode)); }
    
    /**
     * @brief Get current mode
     */
    GizmoMode mode() const { return m_mode; }
    
    // ---- Interaction ----
    
    /**
     * @brief Perform hit test for mouse position
     * @param screenPos Mouse position in screen coordinates
     * @param viewport Viewport for unprojection
     * @return Hit result with axis information
     */
    GizmoHitResult hitTest(const QPoint& screenPos, const Viewport* viewport) const;
    
    /**
     * @brief Set active (dragging) axis
     */
    void setActiveAxis(int axis);
    
    /**
     * @brief Get active axis
     */
    int activeAxis() const { return m_activeAxis; }
    
    /**
     * @brief Set hover-highlighted axis
     */
    void setHoverAxis(int axis);
    
    /**
     * @brief Get hover axis
     */
    int hoverAxis() const { return m_hoverAxis; }
    
    // ---- Visual Settings ----
    
    /**
     * @brief Set base size of the gizmo
     */
    void setSize(float size);
    
    /**
     * @brief Get base size
     */
    float size() const { return m_size; }
    
    /**
     * @brief Enable/disable screen-space consistent sizing
     */
    void setScreenSpaceSizing(bool enabled);
    
    /**
     * @brief Check if screen-space sizing is enabled
     */
    bool screenSpaceSizing() const { return m_screenSpaceSizing; }
    
    /**
     * @brief Set visibility
     */
    void setVisible(bool visible);
    
    /**
     * @brief Check visibility
     */
    bool isVisible() const { return m_visible; }
    
    // ---- Colors ----
    
    /**
     * @brief Set axis colors
     */
    void setAxisColors(const QColor& x, const QColor& y, const QColor& z);
    
    /**
     * @brief Set highlight color
     */
    void setHighlightColor(const QColor& color);
    
    /**
     * @brief Set selection color
     */
    void setSelectionColor(const QColor& color);

signals:
    /**
     * @brief Emitted when gizmo dragging starts
     */
    void dragStarted(int axis);
    
    /**
     * @brief Emitted during gizmo dragging
     */
    void dragging(int axis, const QVector3D& delta);
    
    /**
     * @brief Emitted when gizmo dragging ends
     */
    void dragEnded(int axis);

private:
    void createTranslateGeometry();
    void createRotateGeometry();
    void createScaleGeometry();
    void renderTranslate(const QMatrix4x4& mvp, float scale);
    void renderRotate(const QMatrix4x4& mvp, float scale);
    void renderScale(const QMatrix4x4& mvp, float scale);
    
    QColor getAxisColor(int axis, bool highlight, bool selected) const;
    float computeScreenScale(const QMatrix4x4& view, const QMatrix4x4& projection, 
                             const QSize& viewportSize) const;
    
    bool m_initialized = false;
    bool m_visible = true;
    
    // Transform
    QVector3D m_position{0, 0, 0};
    QQuaternion m_orientation;
    
    // Mode and interaction
    GizmoMode m_mode = GizmoMode::Translate;
    int m_activeAxis = -1;
    int m_hoverAxis = -1;
    
    // Visual settings
    float m_size = 1.0f;
    bool m_screenSpaceSizing = true;
    float m_screenSize = 100.0f; // Pixels
    
    // Colors
    QColor m_axisColorX{230, 50, 50};
    QColor m_axisColorY{50, 180, 50};
    QColor m_axisColorZ{50, 100, 230};
    QColor m_highlightColor{255, 255, 100};
    QColor m_selectionColor{255, 200, 50};
    
    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> m_shader;
    
    // Translate gizmo
    std::unique_ptr<QOpenGLVertexArrayObject> m_translateVAO;
    std::unique_ptr<QOpenGLBuffer> m_translateVBO;
    int m_translateArrowVertices = 0;
    
    // Rotate gizmo
    std::unique_ptr<QOpenGLVertexArrayObject> m_rotateVAO;
    std::unique_ptr<QOpenGLBuffer> m_rotateVBO;
    int m_rotateCircleVertices = 0;
    
    // Scale gizmo
    std::unique_ptr<QOpenGLVertexArrayObject> m_scaleVAO;
    std::unique_ptr<QOpenGLBuffer> m_scaleVBO;
    int m_scaleBoxVertices = 0;
};

} // namespace dc
