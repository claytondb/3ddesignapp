/**
 * @file SketchMode.h
 * @brief 2D Sketch mode controller
 * 
 * Manages the sketch editing workflow:
 * - Enter/exit sketch mode
 * - Viewport orthographic projection
 * - Sketch plane management
 * - Tool switching
 */

#pragma once

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <memory>
#include <vector>

namespace dc3d {
namespace geometry {
class SketchData;
class SketchPlane;
}
}

namespace dc {

class Viewport;
class SketchToolbox;
class SketchViewport;
class SketchTool;

/**
 * @brief Available sketch tools
 */
enum class SketchToolType {
    None,
    Select,
    Line,
    Arc,
    Circle,
    Spline,
    Rectangle,
    Point,
    Trim,
    Extend,
    Offset,
    Mirror,
    Dimension,
    ConstraintHorizontal,
    ConstraintVertical,
    ConstraintCoincident,
    ConstraintParallel,
    ConstraintPerpendicular,
    ConstraintTangent,
    ConstraintEqual,
    ConstraintFix
};

/**
 * @brief Sketch plane orientation
 */
enum class SketchPlaneType {
    XY,     ///< Front plane (Z = constant)
    XZ,     ///< Top plane (Y = constant)
    YZ,     ///< Right plane (X = constant)
    Custom  ///< User-defined plane
};

/**
 * @brief Grid settings for sketch mode
 */
struct SketchGridSettings {
    bool visible = true;
    float majorSpacing = 10.0f;     ///< Major grid line spacing (mm)
    float minorDivisions = 5;       ///< Minor divisions per major
    QColor majorColor{100, 100, 100, 180};
    QColor minorColor{60, 60, 60, 100};
    bool snapToGrid = true;
    float snapDistance = 5.0f;      ///< Pixels
};

/**
 * @brief Snap settings for sketch
 */
struct SketchSnapSettings {
    bool enabled = true;
    bool snapToEndpoints = true;
    bool snapToMidpoints = true;
    bool snapToCenter = true;
    bool snapToIntersection = true;
    bool snapToGrid = true;
    bool snapToHorizontal = true;
    bool snapToVertical = true;
    float snapRadius = 10.0f;       ///< Pixels
};

/**
 * @brief 2D Sketch mode controller
 * 
 * Controls the complete sketch editing workflow including
 * viewport configuration, tool management, and sketch data.
 */
class SketchMode : public QObject {
    Q_OBJECT
    
public:
    explicit SketchMode(Viewport* viewport, QObject* parent = nullptr);
    ~SketchMode() override;
    
    // ---- Mode State ----
    
    /**
     * @brief Enter sketch mode with a new sketch
     * @param planeType Type of sketch plane
     * @param offset Offset from origin along plane normal
     */
    void enterSketchMode(SketchPlaneType planeType, float offset = 0.0f);
    
    /**
     * @brief Enter sketch mode with custom plane
     * @param origin Plane origin point
     * @param normal Plane normal vector
     * @param xAxis X axis direction on plane
     */
    void enterSketchMode(const QVector3D& origin, const QVector3D& normal, const QVector3D& xAxis);
    
    /**
     * @brief Enter sketch mode to edit existing sketch
     * @param sketch Existing sketch data
     */
    void editSketch(std::shared_ptr<dc3d::geometry::SketchData> sketch);
    
    /**
     * @brief Exit sketch mode
     * @param save Whether to save changes
     */
    void exitSketchMode(bool save = true);
    
    /**
     * @brief Check if sketch mode is active
     */
    bool isActive() const { return m_active; }
    
    // ---- Sketch Data ----
    
    /**
     * @brief Get current sketch data
     */
    std::shared_ptr<dc3d::geometry::SketchData> sketchData() const { return m_sketchData; }
    
    /**
     * @brief Get sketch plane
     */
    std::shared_ptr<dc3d::geometry::SketchPlane> sketchPlane() const { return m_sketchPlane; }
    
    // ---- Tool Management ----
    
    /**
     * @brief Set active sketch tool
     */
    void setActiveTool(SketchToolType toolType);
    
    /**
     * @brief Get active tool type
     */
    SketchToolType activeTool() const { return m_activeToolType; }
    
    /**
     * @brief Get active tool instance
     */
    SketchTool* activeToolInstance() const { return m_activeTool; }
    
    /**
     * @brief Cancel current tool operation
     */
    void cancelCurrentOperation();
    
    /**
     * @brief Finish current tool operation
     */
    void finishCurrentOperation();
    
    // ---- Grid Settings ----
    
    /**
     * @brief Set grid settings
     */
    void setGridSettings(const SketchGridSettings& settings);
    
    /**
     * @brief Get grid settings
     */
    const SketchGridSettings& gridSettings() const { return m_gridSettings; }
    
    /**
     * @brief Toggle grid visibility
     */
    void setGridVisible(bool visible);
    
    /**
     * @brief Toggle grid snapping
     */
    void setGridSnap(bool enabled);
    
    // ---- Snap Settings ----
    
    /**
     * @brief Set snap settings
     */
    void setSnapSettings(const SketchSnapSettings& settings);
    
    /**
     * @brief Get snap settings
     */
    const SketchSnapSettings& snapSettings() const { return m_snapSettings; }
    
    // ---- Coordinate Conversion ----
    
    /**
     * @brief Convert screen coordinates to sketch plane 2D
     * @param screenPos Screen position
     * @return 2D position on sketch plane
     */
    QPointF screenToSketch(const QPoint& screenPos) const;
    
    /**
     * @brief Convert sketch 2D to screen coordinates
     * @param sketchPos 2D position on sketch plane
     * @return Screen position
     */
    QPoint sketchToScreen(const QPointF& sketchPos) const;
    
    /**
     * @brief Convert sketch 2D to 3D world coordinates
     * @param sketchPos 2D position on sketch plane
     * @return 3D world position
     */
    QVector3D sketchToWorld(const QPointF& sketchPos) const;
    
    /**
     * @brief Get snap point near cursor
     * @param screenPos Screen position
     * @param snapType Output snap type
     * @return Snapped 2D position, or original if no snap
     */
    QPointF getSnapPoint(const QPoint& screenPos, QString& snapType) const;
    
    // ---- View Control ----
    
    /**
     * @brief Set viewport to look at sketch plane
     */
    void lookAtSketchPlane();
    
    /**
     * @brief Zoom to fit sketch contents
     */
    void zoomToFit();
    
    // ---- UI Components ----
    
    /**
     * @brief Get sketch toolbox widget
     */
    SketchToolbox* toolbox() const { return m_toolbox; }
    
    /**
     * @brief Get sketch viewport overlay
     */
    SketchViewport* viewportOverlay() const { return m_viewportOverlay; }
    
    // ---- Undo/Redo ----
    
    /**
     * @brief Undo last sketch operation
     */
    void undo();
    
    /**
     * @brief Redo last undone operation
     */
    void redo();
    
    /**
     * @brief Check if undo is available
     */
    bool canUndo() const;
    
    /**
     * @brief Check if redo is available
     */
    bool canRedo() const;

signals:
    /**
     * @brief Emitted when sketch mode is entered/exited
     */
    void modeChanged(bool active);
    
    /**
     * @brief Emitted when active tool changes
     */
    void activeToolChanged(SketchToolType toolType);
    
    /**
     * @brief Emitted when sketch data changes
     */
    void sketchModified();
    
    /**
     * @brief Emitted when grid settings change
     */
    void gridSettingsChanged();
    
    /**
     * @brief Emitted when snap settings change
     */
    void snapSettingsChanged();
    
    /**
     * @brief Emitted when current snap point changes
     */
    void snapPointChanged(const QPointF& point, const QString& snapType);
    
    /**
     * @brief Emitted to request sketch save
     */
    void saveRequested(std::shared_ptr<dc3d::geometry::SketchData> sketch);
    
    /**
     * @brief Emitted when undo/redo state changes
     */
    void undoStateChanged(bool canUndo, bool canRedo);

public slots:
    /**
     * @brief Handle mouse press in viewport
     */
    void handleMousePress(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    
    /**
     * @brief Handle mouse move in viewport
     */
    void handleMouseMove(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    
    /**
     * @brief Handle mouse release in viewport
     */
    void handleMouseRelease(const QPoint& pos, Qt::MouseButtons buttons);
    
    /**
     * @brief Handle mouse double-click
     */
    void handleDoubleClick(const QPoint& pos, Qt::MouseButtons buttons);
    
    /**
     * @brief Handle key press
     */
    void handleKeyPress(QKeyEvent* event);
    
    /**
     * @brief Handle key release
     */
    void handleKeyRelease(QKeyEvent* event);
    
private:
    void setupViewport();
    void restoreViewport();
    void createTools();
    void destroyTools();
    void updateSnapIndicator(const QPoint& screenPos);
    QMatrix4x4 computePlaneTransform() const;
    
    Viewport* m_viewport;
    SketchToolbox* m_toolbox;
    SketchViewport* m_viewportOverlay;
    
    bool m_active = false;
    std::shared_ptr<dc3d::geometry::SketchData> m_sketchData;
    std::shared_ptr<dc3d::geometry::SketchPlane> m_sketchPlane;
    
    // Saved viewport state
    QMatrix4x4 m_savedViewMatrix;
    QMatrix4x4 m_savedProjectionMatrix;
    bool m_savedOrthographic;
    
    // Tools
    SketchToolType m_activeToolType = SketchToolType::None;
    SketchTool* m_activeTool = nullptr;
    std::map<SketchToolType, std::unique_ptr<SketchTool>> m_tools;
    
    // Settings
    SketchGridSettings m_gridSettings;
    SketchSnapSettings m_snapSettings;
    
    // Current snap state
    QPointF m_currentSnapPoint;
    QString m_currentSnapType;
    
    // Modifier state
    bool m_shiftPressed = false;
    bool m_ctrlPressed = false;
};

} // namespace dc
