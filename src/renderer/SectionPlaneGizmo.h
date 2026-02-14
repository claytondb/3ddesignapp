/**
 * @file SectionPlaneGizmo.h
 * @brief Interactive section plane gizmo for viewport manipulation
 * 
 * Features:
 * - Draggable plane in viewport
 * - Rotation handles for plane orientation
 * - Offset handles for plane position
 * - Visual plane representation with grid
 * - Multiple section planes support
 * - Section cap visualization
 * - Animation support for presentations
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
#include <QTimer>
#include <memory>
#include <vector>

namespace dc {

class Viewport;

/**
 * @brief Section plane axis orientation
 */
enum class SectionAxis {
    X = 0,      ///< YZ plane (X normal)
    Y = 1,      ///< XZ plane (Y normal)  
    Z = 2,      ///< XY plane (Z normal)
    Custom = 3  ///< User-defined normal
};

/**
 * @brief Individual section plane data
 */
struct SectionPlane {
    int id = -1;
    QVector3D normal{0, 0, 1};
    QVector3D origin{0, 0, 0};
    float offset = 0.0f;
    bool enabled = true;
    bool showCap = false;
    QColor planeColor{100, 150, 220, 128};
    QColor capColor{200, 100, 80, 200};
    
    /**
     * @brief Get plane equation coefficients (ax + by + cz + d = 0)
     */
    QVector4D equation() const {
        float d = -QVector3D::dotProduct(normal, origin + normal * offset);
        return QVector4D(normal.x(), normal.y(), normal.z(), d);
    }
};

/**
 * @brief Animation parameters for section planes
 */
struct SectionAnimation {
    bool playing = false;
    float startOffset = 0.0f;
    float endOffset = 100.0f;
    float duration = 3.0f;      // seconds
    float currentTime = 0.0f;
    bool loop = false;
    bool pingPong = false;
    bool reverse = false;
};

/**
 * @brief Hit result for gizmo interaction
 */
struct SectionGizmoHitResult {
    bool hit = false;
    int planeId = -1;
    int handleType = -1;    // 0=plane, 1=translate, 2=rotateX, 3=rotateY, 4=rotateZ
    float distance = 0.0f;
    QVector3D hitPoint;
};

/**
 * @brief Interactive section plane gizmo for viewport
 */
class SectionPlaneGizmo : public QObject {
    Q_OBJECT
    
public:
    explicit SectionPlaneGizmo(QObject* parent = nullptr);
    ~SectionPlaneGizmo() override;
    
    // ---- OpenGL ----
    
    void initialize();
    void cleanup();
    void render(const QMatrix4x4& view, const QMatrix4x4& projection, 
                const QSize& viewportSize);
    bool isInitialized() const { return m_initialized; }
    
    // ---- Section Planes ----
    
    /**
     * @brief Add a new section plane
     * @param axis Initial axis orientation
     * @return ID of the new plane
     */
    int addSectionPlane(SectionAxis axis = SectionAxis::Z);
    
    /**
     * @brief Remove a section plane by ID
     */
    void removeSectionPlane(int id);
    
    /**
     * @brief Clear all section planes
     */
    void clearSectionPlanes();
    
    /**
     * @brief Get number of section planes
     */
    int planeCount() const { return static_cast<int>(m_planes.size()); }
    
    /**
     * @brief Get plane by ID
     */
    SectionPlane* plane(int id);
    const SectionPlane* plane(int id) const;
    
    /**
     * @brief Get all planes
     */
    const std::vector<SectionPlane>& planes() const { return m_planes; }
    
    /**
     * @brief Set active/selected plane
     */
    void setActivePlane(int id);
    
    /**
     * @brief Get active plane ID
     */
    int activePlaneId() const { return m_activePlaneId; }
    
    // ---- Quick Presets ----
    
    /**
     * @brief Set plane to X axis (YZ plane)
     */
    void setPlaneAxisX(int planeId = -1);
    
    /**
     * @brief Set plane to Y axis (XZ plane)
     */
    void setPlaneAxisY(int planeId = -1);
    
    /**
     * @brief Set plane to Z axis (XY plane)
     */
    void setPlaneAxisZ(int planeId = -1);
    
    /**
     * @brief Set plane through object center
     */
    void setPlaneAtCenter(int planeId, const QVector3D& center);
    
    // ---- Section Properties ----
    
    /**
     * @brief Set plane offset
     */
    void setPlaneOffset(int planeId, float offset);
    
    /**
     * @brief Set plane normal
     */
    void setPlaneNormal(int planeId, const QVector3D& normal);
    
    /**
     * @brief Set plane origin
     */
    void setPlaneOrigin(int planeId, const QVector3D& origin);
    
    /**
     * @brief Enable/disable plane
     */
    void setPlaneEnabled(int planeId, bool enabled);
    
    // ---- Section Cap ----
    
    /**
     * @brief Enable section cap (solid face at cut)
     */
    void setShowCap(int planeId, bool show);
    
    /**
     * @brief Check if cap is shown
     */
    bool showCap(int planeId) const;
    
    /**
     * @brief Set cap color
     */
    void setCapColor(int planeId, const QColor& color);
    
    // ---- Bounds ----
    
    /**
     * @brief Set mesh bounds for offset range calculation
     */
    void setMeshBounds(const QVector3D& min, const QVector3D& max);
    
    /**
     * @brief Get offset range based on bounds and plane orientation
     */
    void getOffsetRange(int planeId, float& minOffset, float& maxOffset) const;
    
    // ---- Interaction ----
    
    /**
     * @brief Hit test for mouse position
     */
    SectionGizmoHitResult hitTest(const QPoint& screenPos, const Viewport* viewport) const;
    
    /**
     * @brief Begin dragging interaction
     */
    void beginDrag(const QPoint& screenPos, const Viewport* viewport);
    
    /**
     * @brief Update drag interaction
     */
    void updateDrag(const QPoint& screenPos, const Viewport* viewport);
    
    /**
     * @brief End dragging interaction
     */
    void endDrag();
    
    /**
     * @brief Check if currently dragging
     */
    bool isDragging() const { return m_dragging; }
    
    /**
     * @brief Set hover state
     */
    void setHoverHandle(int handleType) { m_hoverHandle = handleType; }
    
    // ---- Animation ----
    
    /**
     * @brief Start animation playback
     */
    void playAnimation(int planeId = -1);
    
    /**
     * @brief Pause animation
     */
    void pauseAnimation();
    
    /**
     * @brief Stop animation and reset
     */
    void stopAnimation();
    
    /**
     * @brief Check if animation is playing
     */
    bool isAnimating() const { return m_animation.playing; }
    
    /**
     * @brief Set animation parameters
     */
    void setAnimationRange(float start, float end);
    void setAnimationDuration(float seconds);
    void setAnimationLoop(bool loop);
    void setAnimationPingPong(bool pingPong);
    
    /**
     * @brief Get animation parameters
     */
    const SectionAnimation& animation() const { return m_animation; }
    
    // ---- Visual Settings ----
    
    void setPlaneSize(float size) { m_planeSize = size; }
    float planeSize() const { return m_planeSize; }
    
    void setHandleSize(float size) { m_handleSize = size; }
    float handleSize() const { return m_handleSize; }
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
    void setPlaneOpacity(float opacity) { m_planeOpacity = opacity; }
    float planeOpacity() const { return m_planeOpacity; }

signals:
    /**
     * @brief Emitted when plane offset changes (for preview)
     */
    void planeOffsetChanged(int planeId, float offset);
    
    /**
     * @brief Emitted when plane normal changes
     */
    void planeNormalChanged(int planeId, const QVector3D& normal);
    
    /**
     * @brief Emitted when plane is added/removed
     */
    void planesChanged();
    
    /**
     * @brief Emitted when animation frame updates
     */
    void animationFrameUpdated(int planeId, float offset);
    
    /**
     * @brief Emitted when dragging starts
     */
    void dragStarted(int planeId);
    
    /**
     * @brief Emitted when dragging ends
     */
    void dragEnded(int planeId);

private slots:
    void onAnimationTick();

private:
    void createPlaneGeometry();
    void createHandleGeometry();
    void renderPlane(const SectionPlane& plane, const QMatrix4x4& view, 
                     const QMatrix4x4& projection, float scale, bool active);
    void renderHandles(const SectionPlane& plane, const QMatrix4x4& view,
                       const QMatrix4x4& projection, float scale);
    void renderCap(const SectionPlane& plane, const QMatrix4x4& view,
                   const QMatrix4x4& projection, float scale);
    
    int nextPlaneId();
    
    bool m_initialized = false;
    bool m_visible = true;
    bool m_dragging = false;
    
    // Section planes
    std::vector<SectionPlane> m_planes;
    int m_activePlaneId = -1;
    int m_nextId = 0;
    
    // Mesh bounds for offset range
    QVector3D m_meshMin{-100, -100, -100};
    QVector3D m_meshMax{100, 100, 100};
    
    // Interaction state
    int m_dragHandle = -1;
    int m_hoverHandle = -1;
    QPoint m_dragStartPos;
    float m_dragStartOffset = 0.0f;
    QVector3D m_dragStartNormal;
    
    // Animation
    SectionAnimation m_animation;
    std::unique_ptr<QTimer> m_animTimer;
    int m_animPlaneId = -1;
    
    // Visual settings
    float m_planeSize = 100.0f;
    float m_handleSize = 10.0f;
    float m_planeOpacity = 0.3f;
    
    // Colors
    QColor m_planeColor{100, 150, 220, 128};
    QColor m_handleColor{255, 200, 50};
    QColor m_activeColor{50, 200, 100};
    QColor m_hoverColor{255, 255, 150};
    
    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> m_shader;
    std::unique_ptr<QOpenGLVertexArrayObject> m_planeVAO;
    std::unique_ptr<QOpenGLBuffer> m_planeVBO;
    int m_planeVertices = 0;
    
    std::unique_ptr<QOpenGLVertexArrayObject> m_handleVAO;
    std::unique_ptr<QOpenGLBuffer> m_handleVBO;
    int m_handleVertices = 0;
    
    std::unique_ptr<QOpenGLVertexArrayObject> m_arrowVAO;
    std::unique_ptr<QOpenGLBuffer> m_arrowVBO;
    int m_arrowVertices = 0;
};

} // namespace dc
