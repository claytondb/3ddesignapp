/**
 * @file Viewport.h
 * @brief Qt OpenGL widget for 3D viewport rendering
 * 
 * Provides the main 3D rendering surface with:
 * - Mouse navigation (orbit, pan, zoom)
 * - Keyboard shortcuts for view presets
 * - Grid and axis rendering
 * - Scene rendering orchestration
 */

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QElapsedTimer>
#include <memory>

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
 * @brief OpenGL viewport widget for 3D rendering
 * 
 * This widget provides the main 3D view with camera controls,
 * grid rendering, and scene display.
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
    
    // ---- Performance ----
    
    /**
     * @brief Get current frames per second
     */
    float fps() const { return m_fps; }
    
    /**
     * @brief Enable/disable FPS display
     */
    void setShowFPS(bool show) { m_showFPS = show; }

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

private:
    void setupOpenGLState();
    void renderGrid();
    void renderScene();
    void updateFPS();
    
    QVector3D screenToWorld(const QPoint& screenPos, float depth = 0.0f) const;
    QVector3D unprojectMouse(const QPoint& pos) const;
    
    // Camera
    std::unique_ptr<Camera> m_camera;
    
    // Renderers
    std::unique_ptr<GridRenderer> m_gridRenderer;
    
    // Navigation state
    NavigationMode m_navMode = NavigationMode::None;
    QPoint m_lastMousePos;
    bool m_shiftPressed = false;
    bool m_ctrlPressed = false;
    bool m_altPressed = false;
    
    // Display settings
    DisplayMode m_displayMode = DisplayMode::Shaded;
    QColor m_backgroundColor{45, 50, 55};
    
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
