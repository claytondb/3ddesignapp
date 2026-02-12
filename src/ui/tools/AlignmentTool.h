/**
 * @file AlignmentTool.h
 * @brief Interactive alignment tool with 3D transform gizmo
 * 
 * Provides manual mesh alignment through:
 * - 3D transform gizmo (translate, rotate, scale)
 * - Axis constraints
 * - Snap to grid
 * - Numeric input
 */

#pragma once

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <memory>

namespace dc3d {
namespace geometry {
class MeshData;
struct AlignmentResult;
}
}

namespace dc {

class Viewport;
class TransformGizmo;

/**
 * @brief Transform mode for the gizmo
 */
enum class TransformMode {
    Translate,
    Rotate,
    Scale
};

/**
 * @brief Axis constraint for transformations
 */
enum class AxisConstraint {
    None,   ///< Free transformation
    X,      ///< Constrain to X axis
    Y,      ///< Constrain to Y axis
    Z,      ///< Constrain to Z axis
    XY,     ///< Constrain to XY plane
    XZ,     ///< Constrain to XZ plane
    YZ      ///< Constrain to YZ plane
};

/**
 * @brief Transform space for transformations
 */
enum class TransformSpace {
    World,  ///< World coordinate system
    Local   ///< Local (object) coordinate system
};

/**
 * @brief Snap settings for transformations
 */
struct SnapSettings {
    bool enabled = false;
    float translateSnap = 1.0f;     ///< Snap increment for translation
    float rotateSnap = 15.0f;       ///< Snap increment for rotation (degrees)
    float scaleSnap = 0.1f;         ///< Snap increment for scale
};

/**
 * @brief Interactive alignment tool for manual mesh positioning
 */
class AlignmentTool : public QObject {
    Q_OBJECT
    
public:
    explicit AlignmentTool(Viewport* viewport, QObject* parent = nullptr);
    ~AlignmentTool() override;
    
    // ---- Tool State ----
    
    /**
     * @brief Activate the alignment tool
     */
    void activate();
    
    /**
     * @brief Deactivate the alignment tool
     */
    void deactivate();
    
    /**
     * @brief Check if tool is active
     */
    bool isActive() const { return m_active; }
    
    // ---- Target Selection ----
    
    /**
     * @brief Set the mesh to transform
     * @param meshId ID of the mesh in viewport
     * @param mesh Pointer to mesh data
     */
    void setTargetMesh(uint64_t meshId, std::shared_ptr<dc3d::geometry::MeshData> mesh);
    
    /**
     * @brief Clear the target mesh
     */
    void clearTarget();
    
    /**
     * @brief Get target mesh ID
     */
    uint64_t targetMeshId() const { return m_targetMeshId; }
    
    // ---- Transform Mode ----
    
    /**
     * @brief Set transform mode
     */
    void setTransformMode(TransformMode mode);
    
    /**
     * @brief Get current transform mode
     */
    TransformMode transformMode() const { return m_transformMode; }
    
    /**
     * @brief Set axis constraint
     */
    void setAxisConstraint(AxisConstraint constraint);
    
    /**
     * @brief Get current axis constraint
     */
    AxisConstraint axisConstraint() const { return m_axisConstraint; }
    
    /**
     * @brief Set transform space
     */
    void setTransformSpace(TransformSpace space);
    
    /**
     * @brief Get current transform space
     */
    TransformSpace transformSpace() const { return m_transformSpace; }
    
    // ---- Snap Settings ----
    
    /**
     * @brief Set snap settings
     */
    void setSnapSettings(const SnapSettings& settings);
    
    /**
     * @brief Get snap settings
     */
    const SnapSettings& snapSettings() const { return m_snapSettings; }
    
    /**
     * @brief Toggle snap enabled
     */
    void setSnapEnabled(bool enabled);
    
    // ---- Current Transform ----
    
    /**
     * @brief Get current translation offset
     */
    QVector3D translation() const { return m_translation; }
    
    /**
     * @brief Get current rotation (Euler angles in degrees)
     */
    QVector3D rotation() const { return m_rotation; }
    
    /**
     * @brief Get current scale factors
     */
    QVector3D scale() const { return m_scale; }
    
    /**
     * @brief Set translation numerically
     */
    void setTranslation(const QVector3D& t);
    
    /**
     * @brief Set rotation numerically (Euler angles in degrees)
     */
    void setRotation(const QVector3D& r);
    
    /**
     * @brief Set scale numerically
     */
    void setScale(const QVector3D& s);
    
    /**
     * @brief Get combined transform matrix
     */
    QMatrix4x4 transformMatrix() const;
    
    // ---- Actions ----
    
    /**
     * @brief Apply current transformation to mesh
     */
    void applyTransform();
    
    /**
     * @brief Cancel transformation and reset
     */
    void cancelTransform();
    
    /**
     * @brief Reset to identity transform
     */
    void resetTransform();
    
    // ---- Gizmo Access ----
    
    /**
     * @brief Get the transform gizmo
     */
    TransformGizmo* gizmo() const { return m_gizmo.get(); }
    
signals:
    /**
     * @brief Emitted when tool is activated/deactivated
     */
    void activeChanged(bool active);
    
    /**
     * @brief Emitted when transform mode changes
     */
    void transformModeChanged(TransformMode mode);
    
    /**
     * @brief Emitted when axis constraint changes
     */
    void axisConstraintChanged(AxisConstraint constraint);
    
    /**
     * @brief Emitted when transform values change
     */
    void transformChanged();
    
    /**
     * @brief Emitted when transformation is applied
     */
    void transformApplied(const dc3d::geometry::AlignmentResult& result);
    
    /**
     * @brief Emitted when transformation is cancelled
     */
    void transformCancelled();

public slots:
    /**
     * @brief Handle mouse press for gizmo interaction
     */
    void handleMousePress(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    
    /**
     * @brief Handle mouse move for gizmo interaction
     */
    void handleMouseMove(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    
    /**
     * @brief Handle mouse release
     */
    void handleMouseRelease(const QPoint& pos, Qt::MouseButtons buttons);
    
private:
    void updateGizmoPosition();
    QVector3D applySnap(const QVector3D& value, TransformMode mode) const;
    void updatePreviewTransform();
    
    Viewport* m_viewport;
    std::unique_ptr<TransformGizmo> m_gizmo;
    
    bool m_active = false;
    uint64_t m_targetMeshId = 0;
    std::shared_ptr<dc3d::geometry::MeshData> m_targetMesh;
    
    TransformMode m_transformMode = TransformMode::Translate;
    AxisConstraint m_axisConstraint = AxisConstraint::None;
    TransformSpace m_transformSpace = TransformSpace::World;
    SnapSettings m_snapSettings;
    
    // Current transform values
    QVector3D m_translation{0, 0, 0};
    QVector3D m_rotation{0, 0, 0};
    QVector3D m_scale{1, 1, 1};
    
    // Gizmo interaction state
    bool m_dragging = false;
    QPoint m_dragStart;
    QVector3D m_dragStartValue;
};

} // namespace dc
