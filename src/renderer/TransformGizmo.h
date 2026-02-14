/**
 * @file TransformGizmo.h
 * @brief Visual 3D gizmo for interactive transformations
 * 
 * Provides a 3D manipulator for translate, rotate, and scale operations:
 * - Visual rendering with axis colors (X=red, Y=green, Z=blue)
 * - Mouse interaction for dragging
 * - Axis highlighting on hover
 * - Screen-space consistent size
 * - Axis/plane constraints
 * - Local/world coordinate space
 * - Multiple pivot point options
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
 * @brief Axis constraint for transformations
 */
enum class AxisConstraint {
    None = 0,       ///< Free transform (all axes)
    X = 1,          ///< Constrain to X axis only
    Y = 2,          ///< Constrain to Y axis only
    Z = 3,          ///< Constrain to Z axis only
    PlaneXY = 4,    ///< Constrain to XY plane (exclude Z)
    PlaneXZ = 5,    ///< Constrain to XZ plane (exclude Y)
    PlaneYZ = 6     ///< Constrain to YZ plane (exclude X)
};

/**
 * @brief Coordinate space for transformations
 */
enum class CoordinateSpace {
    World = 0,      ///< Transform in world coordinates
    Local = 1       ///< Transform in object's local coordinates
};

/**
 * @brief Pivot point options for transformations
 */
enum class PivotPoint {
    BoundingBoxCenter = 0,  ///< Center of selection bounding box
    ObjectOrigin = 1,       ///< Object's local origin
    WorldOrigin = 2,        ///< World origin (0,0,0)
    Cursor3D = 3,           ///< 3D cursor position
    ActiveElement = 4       ///< Active/last selected element
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
    
    // ---- Axis Constraints ----
    
    /**
     * @brief Set axis constraint for transformation
     * @param constraint The axis or plane to constrain to
     */
    void setAxisConstraint(AxisConstraint constraint);
    
    /**
     * @brief Get current axis constraint
     */
    AxisConstraint axisConstraint() const { return m_axisConstraint; }
    
    /**
     * @brief Clear any axis constraint (return to free transform)
     */
    void clearAxisConstraint() { setAxisConstraint(AxisConstraint::None); }
    
    /**
     * @brief Check if an axis is constrained
     * @param axis 0=X, 1=Y, 2=Z
     * @return true if that axis is locked/constrained
     */
    bool isAxisConstrained(int axis) const;
    
    /**
     * @brief Get constraint direction vector (for single axis constraint)
     */
    QVector3D constraintDirection() const;
    
    /**
     * @brief Get constraint plane normal (for plane constraint)
     */
    QVector3D constraintPlaneNormal() const;
    
    // ---- Coordinate Space ----
    
    /**
     * @brief Set coordinate space for transformation
     * @param space World or Local coordinates
     */
    void setCoordinateSpace(CoordinateSpace space);
    
    /**
     * @brief Get current coordinate space
     */
    CoordinateSpace coordinateSpace() const { return m_coordinateSpace; }
    
    /**
     * @brief Toggle between world and local coordinates
     */
    void toggleCoordinateSpace();
    
    /**
     * @brief Check if using local coordinates
     */
    bool isLocalSpace() const { return m_coordinateSpace == CoordinateSpace::Local; }
    
    // ---- Pivot Point ----
    
    /**
     * @brief Set pivot point mode
     * @param pivot The pivot point option
     */
    void setPivotPoint(PivotPoint pivot);
    
    /**
     * @brief Get current pivot point mode
     */
    PivotPoint pivotPoint() const { return m_pivotPoint; }
    
    /**
     * @brief Set custom pivot position (for Cursor3D mode)
     */
    void setCustomPivotPosition(const QVector3D& pos);
    
    /**
     * @brief Get custom pivot position
     */
    QVector3D customPivotPosition() const { return m_customPivot; }
    
    /**
     * @brief Cycle through pivot point options
     */
    void cyclePivotPoint();
    
    // ---- Numeric Input ----
    
    /**
     * @brief Check if numeric input mode is active
     */
    bool isNumericInputActive() const { return m_numericInputActive; }
    
    /**
     * @brief Start numeric input mode
     */
    void startNumericInput();
    
    /**
     * @brief End numeric input mode
     * @param apply True to apply the input, false to cancel
     */
    void endNumericInput(bool apply);
    
    /**
     * @brief Append character to numeric input
     */
    void appendNumericInput(QChar c);
    
    /**
     * @brief Delete last character from numeric input
     */
    void backspaceNumericInput();
    
    /**
     * @brief Get current numeric input string
     */
    QString numericInputString() const { return m_numericInput; }
    
    /**
     * @brief Get parsed numeric value
     */
    double numericInputValue() const;
    
    /**
     * @brief Get secondary value (for axis-specific input like "5,10,3")
     */
    QVector3D numericInputVector() const;
    
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
    
    /**
     * @brief Set constraint highlight color
     */
    void setConstraintColor(const QColor& color);

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
    
    /**
     * @brief Emitted when gizmo mode changes
     */
    void modeChanged(GizmoMode mode);
    
    /**
     * @brief Emitted when axis constraint changes
     */
    void axisConstraintChanged(AxisConstraint constraint);
    
    /**
     * @brief Emitted when coordinate space changes
     */
    void coordinateSpaceChanged(CoordinateSpace space);
    
    /**
     * @brief Emitted when pivot point changes
     */
    void pivotPointChanged(PivotPoint pivot);
    
    /**
     * @brief Emitted when numeric input is confirmed
     */
    void numericInputConfirmed(const QVector3D& value);

private:
    void createTranslateGeometry();
    void createRotateGeometry();
    void createScaleGeometry();
    void createConstraintIndicatorGeometry();
    void renderTranslate(const QMatrix4x4& mvp, float scale);
    void renderRotate(const QMatrix4x4& mvp, float scale);
    void renderScale(const QMatrix4x4& mvp, float scale);
    void renderConstraintIndicator(const QMatrix4x4& mvp, float scale);
    void renderNumericInputOverlay();
    
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
    
    // Axis constraint
    AxisConstraint m_axisConstraint = AxisConstraint::None;
    
    // Coordinate space
    CoordinateSpace m_coordinateSpace = CoordinateSpace::World;
    
    // Pivot point
    PivotPoint m_pivotPoint = PivotPoint::BoundingBoxCenter;
    QVector3D m_customPivot{0, 0, 0};
    
    // Numeric input
    bool m_numericInputActive = false;
    QString m_numericInput;
    
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
    QColor m_constraintColor{255, 150, 0};  // Orange for constraint indicator
    
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
    
    // Constraint indicator (line/plane)
    std::unique_ptr<QOpenGLVertexArrayObject> m_constraintVAO;
    std::unique_ptr<QOpenGLBuffer> m_constraintVBO;
    int m_constraintVertices = 0;
};

/**
 * @brief Helper to get string representation of gizmo mode
 */
inline QString gizmoModeToString(GizmoMode mode) {
    switch (mode) {
        case GizmoMode::Translate: return QStringLiteral("Move");
        case GizmoMode::Rotate: return QStringLiteral("Rotate");
        case GizmoMode::Scale: return QStringLiteral("Scale");
        default: return QStringLiteral("Unknown");
    }
}

/**
 * @brief Helper to get string representation of axis constraint
 */
inline QString axisConstraintToString(AxisConstraint constraint) {
    switch (constraint) {
        case AxisConstraint::None: return QString();
        case AxisConstraint::X: return QStringLiteral("X");
        case AxisConstraint::Y: return QStringLiteral("Y");
        case AxisConstraint::Z: return QStringLiteral("Z");
        case AxisConstraint::PlaneXY: return QStringLiteral("XY Plane");
        case AxisConstraint::PlaneXZ: return QStringLiteral("XZ Plane");
        case AxisConstraint::PlaneYZ: return QStringLiteral("YZ Plane");
        default: return QString();
    }
}

/**
 * @brief Helper to get string representation of coordinate space
 */
inline QString coordinateSpaceToString(CoordinateSpace space) {
    switch (space) {
        case CoordinateSpace::World: return QStringLiteral("World");
        case CoordinateSpace::Local: return QStringLiteral("Local");
        default: return QStringLiteral("Unknown");
    }
}

/**
 * @brief Helper to get string representation of pivot point
 */
inline QString pivotPointToString(PivotPoint pivot) {
    switch (pivot) {
        case PivotPoint::BoundingBoxCenter: return QStringLiteral("Bounding Box Center");
        case PivotPoint::ObjectOrigin: return QStringLiteral("Object Origin");
        case PivotPoint::WorldOrigin: return QStringLiteral("World Origin");
        case PivotPoint::Cursor3D: return QStringLiteral("3D Cursor");
        case PivotPoint::ActiveElement: return QStringLiteral("Active Element");
        default: return QStringLiteral("Unknown");
    }
}

} // namespace dc
