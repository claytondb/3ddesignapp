/**
 * @file Viewport.h
 * @brief Qt OpenGL widget for 3D viewport rendering
 * 
 * Provides the main 3D rendering surface with:
 * - Mouse navigation (orbit, pan, zoom)
 * - Keyboard shortcuts for view presets
 * - Grid and axis rendering
 * - Mesh rendering with shading
 * - Mouse-based selection (click and box selection)
 * - Selection highlighting
 * - Professional gradient background
 * - Viewport info overlay (view name, selection count)
 * - View presets toolbar
 */

#pragma once

#include <QVector3D>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>
#include <memory>
#include <unordered_map>
#include <cstdint>

namespace dc {
class TransformGizmo;
class ViewPresetsWidget;
class MeasureTool;
enum class GizmoMode;
enum class AxisConstraint;
enum class CoordinateSpace;
enum class PivotPoint;
}

namespace dc3d {
namespace geometry {
class MeshData;
}
namespace core {
class Selection;
enum class SelectionMode;
enum class SelectionOp;
}
namespace renderer {
class Picking;
class SelectionRenderer;
class BoxSelector;
}
}

// Include HitInfo for hover tracking
#include "core/Selection.h"

// Forward declare
namespace dc3d { namespace renderer { class SelectionRenderer; } }

namespace dc {

class Camera;
class GridRenderer;
class ShaderProgram;
struct BoundingBox;

/**
 * @brief Navigation mode for mouse interaction
 */
enum class NavigationMode {
    None,
    Orbit,
    Pan,
    Zoom
};

/**
 * @brief Display mode for viewport rendering
 */
enum class DisplayMode {
    Shaded,
    Wireframe,
    ShadedWireframe,
    XRay,
    DeviationMap
};

/**
 * @brief GPU resources for a single mesh
 */
struct MeshGPUData {
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ebo{QOpenGLBuffer::IndexBuffer};
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    QVector3D boundsMin;
    QVector3D boundsMax;
    bool valid = false;
};

/**
 * @brief OpenGL viewport widget for 3D rendering
 * 
 * This widget provides the main 3D view with camera controls,
 * grid rendering, mesh display, and scene lighting.
 */
class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {
    Q_OBJECT

public:
    explicit Viewport(QWidget* parent = nullptr);
    ~Viewport() override;
    
    // ---- Camera Access ----
    
    /**
     * @brief Get the camera (const)
     */
    const Camera& camera() const { return *m_camera; }
    
    /**
     * @brief Get the camera (mutable)
     */
    Camera& camera() { return *m_camera; }
    
    // ---- Mesh Management ----
    
    /**
     * @brief Add a mesh to render
     * @param id Unique mesh identifier
     * @param mesh Mesh data to render
     */
    void addMesh(uint64_t id, std::shared_ptr<dc3d::geometry::MeshData> mesh);
    
    /**
     * @brief Remove a mesh from rendering
     * @param id Mesh identifier to remove
     */
    void removeMesh(uint64_t id);
    
    /**
     * @brief Clear all meshes
     */
    void clearMeshes();
    
    /**
     * @brief Check if a mesh exists
     */
    bool hasMesh(uint64_t id) const;
    
    /**
     * @brief Set the selection manager for rendering highlights
     * @note Uses weak_ptr to avoid dangling pointer if Selection is destroyed
     */
    void setSelection(dc3d::core::Selection* selection) { m_selection = selection; }
    
    /**
     * @brief Set the measure tool for overlay rendering
     * @param measureTool Pointer to the MeasureTool instance
     */
    void setMeasureTool(MeasureTool* measureTool) { m_measureTool = measureTool; }
    
    /**
     * @brief Get the measure tool
     */
    MeasureTool* measureTool() const { return m_measureTool; }
    
    // ---- View Control ----
    
    /**
     * @brief Set to a standard view
     * @param viewName One of: "front", "back", "top", "bottom", "left", "right", "isometric"
     */
    void setStandardView(const QString& viewName);
    
    /**
     * @brief Fit view to show all visible geometry
     */
    void fitView();
    
    /**
     * @brief Fit view to specific bounds
     * @param bounds Bounding box to fit
     */
    void fitView(const BoundingBox& bounds);
    
    /**
     * @brief Fit view to current selection
     * Falls back to fitView() if nothing selected
     */
    void zoomToSelection();
    
    /**
     * @brief Reset view to default
     */
    void resetView();
    
    // ---- Display Settings ----
    
    /**
     * @brief Set display mode
     */
    void setDisplayMode(DisplayMode mode);
    
    /**
     * @brief Get current display mode
     */
    DisplayMode displayMode() const { return m_displayMode; }
    
    /**
     * @brief Toggle grid visibility
     */
    void setGridVisible(bool visible);
    
    /**
     * @brief Check grid visibility
     */
    bool isGridVisible() const;
    
    /**
     * @brief Set background color
     */
    void setBackgroundColor(const QColor& color);
    
    /**
     * @brief Get background color
     */
    QColor backgroundColor() const { return m_backgroundColor; }
    
    /**
     * @brief Set gradient background colors
     * @param topColor Color at top of viewport
     * @param bottomColor Color at bottom of viewport
     */
    void setGradientBackground(const QColor& topColor, const QColor& bottomColor);
    
    /**
     * @brief Enable/disable gradient background
     */
    void setGradientEnabled(bool enabled);
    
    /**
     * @brief Check if gradient background is enabled
     */
    bool isGradientEnabled() const { return m_gradientEnabled; }
    
    /**
     * @brief Get current view name (e.g., "Front", "Perspective")
     */
    QString currentViewName() const { return m_currentViewName; }
    
    /**
     * @brief Get view center point
     */
    QVector3D viewCenter() const;
    
    /**
     * @brief Enable/disable viewport info overlay
     */
    void setInfoOverlayEnabled(bool enabled) { m_showInfoOverlay = enabled; update(); }
    
    /**
     * @brief Check if info overlay is enabled
     */
    bool isInfoOverlayEnabled() const { return m_showInfoOverlay; }
    
    // ---- Performance ----
    
    /**
     * @brief Get current frames per second
     */
    float fps() const { return m_fps; }
    
    /**
     * @brief Check if FPS is being shown
     */
    bool isShowingFPS() const { return m_showFPS; }
    
    /**
     * @brief Enable/disable FPS display (toggled with ` key)
     */
    void setShowFPS(bool show) { m_showFPS = show; update(); }
    
    /**
     * @brief Toggle FPS display
     */
    void toggleFPS() { setShowFPS(!m_showFPS); }
    
    // ---- Transform Gizmo ----
    
    /**
     * @brief Get transform gizmo
     */
    TransformGizmo* gizmo() const { return m_gizmo.get(); }
    
    /**
     * @brief Update gizmo position from selection
     * @param center World-space position for gizmo
     * @param visible Whether gizmo should be visible
     */
    void updateGizmo(const QVector3D& center, bool visible);
    
    /**
     * @brief Set gizmo mode
     * @param mode 0=Translate, 1=Rotate, 2=Scale
     */
    void setGizmoMode(int mode);

signals:
    /**
     * @brief Emitted when camera changes
     */
    void cameraChanged();
    
    /**
     * @brief Emitted when cursor moves (for status bar)
     * @param worldPos 3D position under cursor
     */
    void cursorMoved(const QVector3D& worldPos);
    
    /**
     * @brief Emitted when display mode changes
     */
    void displayModeChanged(DisplayMode mode);
    
    /**
     * @brief Emitted when user clicks for selection
     * @param pos Screen position of click
     * @param addToSelection Whether to add to existing selection (Shift held)
     * @param toggleSelection Whether to toggle selection (Ctrl held)
     */
    void selectionClick(const QPoint& pos, bool addToSelection, bool toggleSelection);
    
    /**
     * @brief Emitted when user completes a box selection
     * @param rect Selection rectangle in screen coords
     * @param addToSelection Whether to add to existing selection
     */
    void boxSelectionComplete(const QRect& rect, bool addToSelection);
    
    /**
     * @brief Emitted when user presses Delete key
     */
    void deleteRequested();
    
    /**
     * @brief Emitted when hover state changes
     * @param hitInfo Current hover target (or empty if nothing hovered)
     */
    void hoverChanged(const dc3d::core::HitInfo& hitInfo);
    
    /**
     * @brief Emitted when FPS is updated (once per second)
     * @param fps Current frames per second
     */
    void fpsUpdated(int fps);
    
    /**
     * @brief Emitted when transform mode changes (via W/E/R keys)
     * @param mode 0=Translate, 1=Rotate, 2=Scale
     */
    void transformModeChanged(int mode);
    
    /**
     * @brief Emitted when axis constraint changes (via X/Y/Z keys)
     * @param constraint Current axis constraint
     */
    void axisConstraintChanged(dc::AxisConstraint constraint);
    
    /**
     * @brief Emitted when coordinate space changes (via L key)
     * @param space World or Local coordinates
     */
    void coordinateSpaceChanged(dc::CoordinateSpace space);
    
    /**
     * @brief Emitted when pivot point changes (via . key)
     * @param pivot Current pivot point mode
     */
    void pivotPointChanged(dc::PivotPoint pivot);
    
    /**
     * @brief Emitted when numeric input mode starts
     */
    void numericInputStarted();
    
    /**
     * @brief Emitted when numeric input text changes
     * @param text Current input string
     */
    void numericInputChanged(const QString& text);
    
    /**
     * @brief Emitted when numeric input is confirmed
     * @param value The entered value as a 3D vector
     */
    void numericInputConfirmed(const QVector3D& value);
    
    /**
     * @brief Emitted when numeric input is cancelled
     */
    void numericInputCancelled();

protected:
    // QOpenGLWidget overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    
    // Mouse events
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    
    // Keyboard events
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    
    // Focus events
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

protected:
    /**
     * @brief Override to render 2D overlays on top of GL content
     */
    void paintEvent(QPaintEvent* event) override;
    
    /**
     * @brief Override to reposition overlay widgets
     */
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupOpenGLState();
    void setupMeshShader();
    void setupGradientShader();
    void setupViewPresetsWidget();
    void renderGradientBackground();
    void renderGrid();
    void renderMeshes();
    void renderMesh(MeshGPUData& gpuData);
    void renderInfoOverlay(QPainter& painter);
    void updateFPS();
    void uploadMeshToGPU(uint64_t id, const dc3d::geometry::MeshData& mesh);
    BoundingBox computeSceneBounds() const;
    void renderFPSOverlay();
    void updateViewName();
    
    QVector3D screenToWorld(const QPoint& screenPos, float depth = 0.0f) const;
    QVector3D unprojectMouse(const QPoint& pos) const;
    
    // Camera
    std::unique_ptr<Camera> m_camera;
    
    // Renderers
    std::unique_ptr<GridRenderer> m_gridRenderer;
    std::unique_ptr<ShaderProgram> m_meshShader;
    std::unique_ptr<dc3d::renderer::SelectionRenderer> m_selectionRenderer;
    std::unique_ptr<TransformGizmo> m_gizmo;
    
    // Selection reference (raw pointer - lifetime managed by Application)
    dc3d::core::Selection* m_selection = nullptr;
    
    // Measure tool reference (raw pointer - lifetime managed by caller)
    MeasureTool* m_measureTool = nullptr;
    
    // Mesh storage
    std::unordered_map<uint64_t, std::shared_ptr<dc3d::geometry::MeshData>> m_meshes;
    std::unordered_map<uint64_t, std::unique_ptr<MeshGPUData>> m_meshGPUData;
    
    // Navigation state
    NavigationMode m_navMode = NavigationMode::None;
    QPoint m_lastMousePos;
    QPoint m_mouseDownPos;
    bool m_shiftPressed = false;
    bool m_ctrlPressed = false;
    bool m_altPressed = false;
    bool m_isBoxSelecting = false;
    
    // Hover tracking for pre-selection feedback
    dc3d::core::HitInfo m_hoverHitInfo;
    bool m_hoverEnabled = true;
    
    // Display settings
    DisplayMode m_displayMode = DisplayMode::Shaded;
    QColor m_backgroundColor{45, 50, 55};
    
    // Gradient background
    bool m_gradientEnabled = true;
    QColor m_gradientTopColor{60, 65, 75};      // Slightly lighter at top
    QColor m_gradientBottomColor{30, 32, 38};   // Darker at bottom
    std::unique_ptr<QOpenGLShaderProgram> m_gradientShader;
    QOpenGLVertexArrayObject m_gradientVAO;
    QOpenGLBuffer m_gradientVBO{QOpenGLBuffer::VertexBuffer};
    
    // Viewport info overlay
    bool m_showInfoOverlay = true;
    QString m_currentViewName{"Perspective"};
    
    // View presets widget
    ViewPresetsWidget* m_viewPresetsWidget = nullptr;
    
    // FPS tracking
    QElapsedTimer m_frameTimer;
    int m_frameCount = 0;
    float m_fps = 0.0f;
    qint64 m_lastFPSUpdate = 0;
    bool m_showFPS = false;
    
    // Initialization flag
    bool m_initialized = false;
};

} // namespace dc
