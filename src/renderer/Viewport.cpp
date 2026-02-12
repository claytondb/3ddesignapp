/**
 * @file Viewport.cpp
 * @brief Implementation of OpenGL viewport widget
 */

#include "Viewport.h"
#include "Camera.h"
#include "GridRenderer.h"
#include "ShaderProgram.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QtMath>

namespace dc {

Viewport::Viewport(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_camera(std::make_unique<Camera>())
    , m_gridRenderer(std::make_unique<GridRenderer>())
{
    // Enable keyboard focus
    setFocusPolicy(Qt::StrongFocus);
    
    // Enable mouse tracking for cursor position updates
    setMouseTracking(true);
    
    // Set minimum size
    setMinimumSize(400, 300);
    
    // Request OpenGL 4.1 core profile
    QSurfaceFormat format;
    format.setVersion(4, 1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);  // MSAA
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    setFormat(format);
    
    // Initialize frame timer
    m_frameTimer.start();
}

Viewport::~Viewport()
{
    makeCurrent();
    m_gridRenderer->cleanup();
    doneCurrent();
}

void Viewport::initializeGL()
{
    initializeOpenGLFunctions();
    
    qDebug() << "OpenGL Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << "GLSL Version:" << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    qDebug() << "Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    
    setupOpenGLState();
    
    // Initialize grid renderer
    if (!m_gridRenderer->initialize()) {
        qWarning() << "Failed to initialize grid renderer";
    }
    
    // Set initial camera position
    m_camera->lookAt(
        QVector3D(10.0f, 8.0f, 10.0f),  // Position
        QVector3D(0.0f, 0.0f, 0.0f),    // Target
        QVector3D(0.0f, 1.0f, 0.0f)     // Up
    );
    
    m_initialized = true;
}

void Viewport::setupOpenGLState()
{
    // Set clear color
    glClearColor(
        m_backgroundColor.redF(),
        m_backgroundColor.greenF(),
        m_backgroundColor.blueF(),
        1.0f
    );
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable back-face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Enable multisampling
    glEnable(GL_MULTISAMPLE);
    
    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Viewport::paintGL()
{
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render grid
    renderGrid();
    
    // Render scene objects
    renderScene();
    
    // Update FPS counter
    updateFPS();
}

void Viewport::resizeGL(int w, int h)
{
    if (h == 0) h = 1;  // Prevent division by zero
    
    glViewport(0, 0, w, h);
    
    // Update camera aspect ratio
    float aspectRatio = static_cast<float>(w) / static_cast<float>(h);
    m_camera->setAspectRatio(aspectRatio);
    
    emit cameraChanged();
}

void Viewport::renderGrid()
{
    if (m_gridRenderer && m_gridRenderer->isVisible()) {
        m_gridRenderer->render(*m_camera);
    }
}

void Viewport::renderScene()
{
    // TODO: Render scene objects from SceneManager
    // This will be implemented when scene management is added
}

void Viewport::updateFPS()
{
    m_frameCount++;
    
    qint64 elapsed = m_frameTimer.elapsed();
    if (elapsed - m_lastFPSUpdate >= 1000) {
        m_fps = m_frameCount * 1000.0f / (elapsed - m_lastFPSUpdate);
        m_frameCount = 0;
        m_lastFPSUpdate = elapsed;
    }
}

// ---- Mouse Events ----

void Viewport::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    
    if (event->button() == Qt::MiddleButton) {
        if (m_shiftPressed) {
            m_navMode = NavigationMode::Pan;
        } else {
            m_navMode = NavigationMode::Orbit;
        }
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::RightButton) {
        // Could be used for context menu or zoom
        m_navMode = NavigationMode::Zoom;
        setCursor(Qt::SizeVerCursor);
    }
    
    event->accept();
}

void Viewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_navMode = NavigationMode::None;
        setCursor(Qt::ArrowCursor);
    }
    
    event->accept();
}

void Viewport::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();
    
    switch (m_navMode) {
        case NavigationMode::Orbit:
            m_camera->orbit(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            emit cameraChanged();
            update();
            break;
            
        case NavigationMode::Pan:
            m_camera->pan(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
            emit cameraChanged();
            update();
            break;
            
        case NavigationMode::Zoom:
            m_camera->zoom(static_cast<float>(-delta.y()) * 0.1f);
            emit cameraChanged();
            update();
            break;
            
        case NavigationMode::None:
            // Update cursor position for status bar
            {
                QVector3D worldPos = unprojectMouse(event->pos());
                emit cursorMoved(worldPos);
            }
            break;
    }
    
    event->accept();
}

void Viewport::wheelEvent(QWheelEvent* event)
{
    // Zoom with scroll wheel
    float delta = event->angleDelta().y() / 120.0f;  // Standard wheel delta
    m_camera->zoom(delta);
    
    emit cameraChanged();
    update();
    event->accept();
}

// ---- Keyboard Events ----

void Viewport::keyPressEvent(QKeyEvent* event)
{
    // Track modifier keys
    if (event->key() == Qt::Key_Shift) {
        m_shiftPressed = true;
        if (m_navMode == NavigationMode::Orbit) {
            m_navMode = NavigationMode::Pan;
        }
    } else if (event->key() == Qt::Key_Control) {
        m_ctrlPressed = true;
    } else if (event->key() == Qt::Key_Alt) {
        m_altPressed = true;
    }
    
    // View shortcuts
    switch (event->key()) {
        case Qt::Key_F:
            fitView();
            break;
            
        case Qt::Key_G:
            setGridVisible(!isGridVisible());
            update();
            break;
            
        // Display mode shortcuts
        case Qt::Key_1:
            setDisplayMode(DisplayMode::Shaded);
            break;
        case Qt::Key_2:
            setDisplayMode(DisplayMode::Wireframe);
            break;
        case Qt::Key_3:
            setDisplayMode(DisplayMode::ShadedWireframe);
            break;
        case Qt::Key_4:
            setDisplayMode(DisplayMode::XRay);
            break;
        case Qt::Key_5:
            setDisplayMode(DisplayMode::DeviationMap);
            break;
            
        // Numpad views
        case Qt::Key_1 + Qt::KeypadModifier:
            if (m_ctrlPressed) {
                setStandardView("back");
            } else {
                setStandardView("front");
            }
            break;
        case Qt::Key_3 + Qt::KeypadModifier:
            if (m_ctrlPressed) {
                setStandardView("left");
            } else {
                setStandardView("right");
            }
            break;
        case Qt::Key_7 + Qt::KeypadModifier:
            if (m_ctrlPressed) {
                setStandardView("bottom");
            } else {
                setStandardView("top");
            }
            break;
        case Qt::Key_0 + Qt::KeypadModifier:
            setStandardView("isometric");
            break;
            
        default:
            QOpenGLWidget::keyPressEvent(event);
            return;
    }
    
    event->accept();
}

void Viewport::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_shiftPressed = false;
        if (m_navMode == NavigationMode::Pan) {
            m_navMode = NavigationMode::Orbit;
        }
    } else if (event->key() == Qt::Key_Control) {
        m_ctrlPressed = false;
    } else if (event->key() == Qt::Key_Alt) {
        m_altPressed = false;
    }
    
    QOpenGLWidget::keyReleaseEvent(event);
}

void Viewport::focusInEvent(QFocusEvent* event)
{
    QOpenGLWidget::focusInEvent(event);
}

void Viewport::focusOutEvent(QFocusEvent* event)
{
    // Reset modifier state when focus is lost
    m_shiftPressed = false;
    m_ctrlPressed = false;
    m_altPressed = false;
    m_navMode = NavigationMode::None;
    setCursor(Qt::ArrowCursor);
    
    QOpenGLWidget::focusOutEvent(event);
}

// ---- View Control Methods ----

void Viewport::setStandardView(const QString& viewName)
{
    QString name = viewName.toLower();
    
    if (name == "front") {
        m_camera->setStandardView(StandardView::Front);
    } else if (name == "back") {
        m_camera->setStandardView(StandardView::Back);
    } else if (name == "top") {
        m_camera->setStandardView(StandardView::Top);
    } else if (name == "bottom") {
        m_camera->setStandardView(StandardView::Bottom);
    } else if (name == "left") {
        m_camera->setStandardView(StandardView::Left);
    } else if (name == "right") {
        m_camera->setStandardView(StandardView::Right);
    } else if (name == "isometric" || name == "iso") {
        m_camera->setStandardView(StandardView::Isometric);
    }
    
    emit cameraChanged();
    update();
}

void Viewport::fitView()
{
    // Default bounds if no scene
    BoundingBox defaultBounds;
    defaultBounds.min = QVector3D(-10, -10, -10);
    defaultBounds.max = QVector3D(10, 10, 10);
    
    // TODO: Get actual scene bounds from SceneManager
    fitView(defaultBounds);
}

void Viewport::fitView(const BoundingBox& bounds)
{
    m_camera->fitToView(bounds);
    emit cameraChanged();
    update();
}

void Viewport::resetView()
{
    m_camera->reset();
    emit cameraChanged();
    update();
}

// ---- Display Settings ----

void Viewport::setDisplayMode(DisplayMode mode)
{
    if (m_displayMode != mode) {
        m_displayMode = mode;
        emit displayModeChanged(mode);
        update();
    }
}

void Viewport::setGridVisible(bool visible)
{
    if (m_gridRenderer) {
        m_gridRenderer->setVisible(visible);
    }
}

bool Viewport::isGridVisible() const
{
    return m_gridRenderer ? m_gridRenderer->isVisible() : false;
}

void Viewport::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    if (m_initialized) {
        makeCurrent();
        glClearColor(color.redF(), color.greenF(), color.blueF(), 1.0f);
        doneCurrent();
        update();
    }
}

// ---- Coordinate Conversion ----

QVector3D Viewport::screenToWorld(const QPoint& screenPos, float depth) const
{
    // Get viewport dimensions
    float x = static_cast<float>(screenPos.x());
    float y = static_cast<float>(height() - screenPos.y());  // Flip Y
    float w = static_cast<float>(width());
    float h = static_cast<float>(height());
    
    // Convert to NDC
    float ndcX = (2.0f * x / w) - 1.0f;
    float ndcY = (2.0f * y / h) - 1.0f;
    float ndcZ = 2.0f * depth - 1.0f;
    
    QVector4D clipCoords(ndcX, ndcY, ndcZ, 1.0f);
    
    // Inverse view-projection
    QMatrix4x4 invVP = m_camera->viewProjectionMatrix().inverted();
    QVector4D worldCoords = invVP * clipCoords;
    
    if (qFuzzyIsNull(worldCoords.w())) {
        return QVector3D();
    }
    
    return worldCoords.toVector3D() / worldCoords.w();
}

QVector3D Viewport::unprojectMouse(const QPoint& pos) const
{
    // Project onto XZ plane (Y=0)
    QVector3D nearPoint = screenToWorld(pos, 0.0f);
    QVector3D farPoint = screenToWorld(pos, 1.0f);
    
    QVector3D ray = (farPoint - nearPoint).normalized();
    QVector3D origin = nearPoint;
    
    // Intersect with Y=0 plane
    if (qFuzzyIsNull(ray.y())) {
        return QVector3D();
    }
    
    float t = -origin.y() / ray.y();
    if (t < 0) {
        return QVector3D();
    }
    
    return origin + ray * t;
}

} // namespace dc
