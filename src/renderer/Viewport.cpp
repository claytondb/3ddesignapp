/**
 * @file Viewport.cpp
 * @brief Implementation of OpenGL viewport widget with mesh rendering
 */

#include "Viewport.h"
#include "Camera.h"
#include "GridRenderer.h"
#include "ShaderProgram.h"
#include "SelectionRenderer.h"
#include "geometry/MeshData.h"
#include "core/Selection.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QtMath>
#include <QFile>

namespace dc {

// Embedded mesh shaders (fallback if resource loading fails)
static const char* MESH_VERTEX_SHADER = R"(
#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec3 vViewPosition;

void main() {
    vec4 worldPos = model * vec4(position, 1.0);
    vWorldPosition = worldPos.xyz;
    vWorldNormal = normalize(normalMatrix * normal);
    
    vec4 viewPos = view * worldPos;
    vViewPosition = viewPos.xyz;
    
    gl_Position = projection * viewPos;
}
)";

static const char* MESH_FRAGMENT_SHADER = R"(
#version 410 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vViewPosition;

uniform vec3 baseColor;
uniform vec3 cameraPosition;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float ambientStrength;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(vWorldNormal);
    
    // Handle back faces
    if (!gl_FrontFacing) {
        normal = -normal;
    }
    
    vec3 viewDir = normalize(cameraPosition - vWorldPosition);
    vec3 lightDirection = normalize(-lightDir);
    
    // Ambient
    vec3 ambient = ambientStrength * baseColor;
    
    // Diffuse (Lambert)
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor * baseColor;
    
    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDirection + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = 0.3 * spec * lightColor;
    
    // Fill light from opposite side
    vec3 fillLightDir = normalize(vec3(1.0, 0.5, 1.0));
    float fillDiff = max(dot(normal, fillLightDir), 0.0);
    vec3 fill = 0.15 * fillDiff * baseColor;
    
    // Combine
    vec3 color = ambient + diffuse + specular + fill;
    
    // Tone mapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    fragColor = vec4(color, 1.0);
}
)";

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
    
    // Clean up mesh GPU data
    for (auto& pair : m_meshGPUData) {
        if (pair.second && pair.second->valid) {
            pair.second->vbo.destroy();
            pair.second->ebo.destroy();
            pair.second->vao.destroy();
        }
    }
    m_meshGPUData.clear();
    
    m_gridRenderer->cleanup();
    if (m_selectionRenderer) {
        m_selectionRenderer->cleanup();
    }
    doneCurrent();
}

void Viewport::initializeGL()
{
    initializeOpenGLFunctions();
    
    qDebug() << "OpenGL Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << "GLSL Version:" << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    qDebug() << "Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    
    setupOpenGLState();
    setupMeshShader();
    
    // Initialize grid renderer
    if (!m_gridRenderer->initialize()) {
        qWarning() << "Failed to initialize grid renderer";
    }
    
    // Initialize selection renderer
    m_selectionRenderer = std::make_unique<dc3d::renderer::SelectionRenderer>();
    if (!m_selectionRenderer->initialize()) {
        qWarning() << "Failed to initialize selection renderer";
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

void Viewport::setupMeshShader()
{
    m_meshShader = std::make_unique<ShaderProgram>();
    
    // Try loading from resources first
    if (!m_meshShader->loadFromResources(":/shaders/mesh.vert", ":/shaders/mesh.frag")) {
        qDebug() << "Could not load shaders from resources, using embedded shaders";
        if (!m_meshShader->loadFromSource(MESH_VERTEX_SHADER, MESH_FRAGMENT_SHADER)) {
            qWarning() << "Failed to compile mesh shader:" << m_meshShader->errorLog();
        }
    }
}

void Viewport::paintGL()
{
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render grid
    renderGrid();
    
    // Render meshes
    renderMeshes();
    
    // Render selection highlights
    if (m_selectionRenderer && m_selection) {
        m_selectionRenderer->render(*m_camera, *m_selection);
    }
    
    // Render box selection overlay if active
    if (m_isBoxSelecting && m_selectionRenderer) {
        m_selectionRenderer->renderBoxSelection(m_mouseDownPos, m_lastMousePos, size());
    }
    
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

void Viewport::renderMeshes()
{
    if (m_meshGPUData.empty() || !m_meshShader || !m_meshShader->isValid()) {
        return;
    }
    
    m_meshShader->bind();
    
    // Set common uniforms
    QMatrix4x4 model;
    model.setToIdentity();
    QMatrix4x4 view = m_camera->viewMatrix();
    QMatrix4x4 projection = m_camera->projectionMatrix();
    QMatrix3x3 normalMatrix = model.normalMatrix();
    
    m_meshShader->setUniform("model", model);
    m_meshShader->setUniform("view", view);
    m_meshShader->setUniform("projection", projection);
    m_meshShader->setUniform("normalMatrix", normalMatrix);
    m_meshShader->setUniform("cameraPosition", m_camera->position());
    
    // Lighting
    m_meshShader->setUniform("lightDir", QVector3D(-0.5f, -0.7f, -0.5f));
    m_meshShader->setUniform("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_meshShader->setUniform("ambientStrength", 0.2f);
    
    // Material color
    m_meshShader->setUniform("baseColor", QVector3D(0.7f, 0.7f, 0.75f));
    
    // Render based on display mode
    switch (m_displayMode) {
        case DisplayMode::Shaded:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
            for (auto& pair : m_meshGPUData) {
                if (pair.second && pair.second->valid) {
                    renderMesh(*pair.second);
                }
            }
            break;
            
        case DisplayMode::Wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_CULL_FACE);
            glLineWidth(1.0f);
            m_meshShader->setUniform("baseColor", QVector3D(0.3f, 0.6f, 0.9f));
            for (auto& pair : m_meshGPUData) {
                if (pair.second && pair.second->valid) {
                    renderMesh(*pair.second);
                }
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
            break;
            
        case DisplayMode::ShadedWireframe:
            // First pass: shaded
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
            for (auto& pair : m_meshGPUData) {
                if (pair.second && pair.second->valid) {
                    renderMesh(*pair.second);
                }
            }
            
            // Second pass: wireframe overlay
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_CULL_FACE);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0f, -1.0f);
            glLineWidth(1.0f);
            m_meshShader->setUniform("baseColor", QVector3D(0.1f, 0.1f, 0.1f));
            for (auto& pair : m_meshGPUData) {
                if (pair.second && pair.second->valid) {
                    renderMesh(*pair.second);
                }
            }
            glDisable(GL_POLYGON_OFFSET_LINE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
            break;
            
        case DisplayMode::XRay:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_CULL_FACE);
            m_meshShader->setUniform("baseColor", QVector3D(0.5f, 0.7f, 0.9f));
            for (auto& pair : m_meshGPUData) {
                if (pair.second && pair.second->valid) {
                    renderMesh(*pair.second);
                }
            }
            glEnable(GL_CULL_FACE);
            break;
            
        case DisplayMode::DeviationMap:
            // Fall back to shaded for now
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            for (auto& pair : m_meshGPUData) {
                if (pair.second && pair.second->valid) {
                    renderMesh(*pair.second);
                }
            }
            break;
    }
    
    m_meshShader->release();
}

void Viewport::renderMesh(MeshGPUData& gpuData)
{
    if (!gpuData.valid) return;
    
    gpuData.vao.bind();
    glDrawElements(GL_TRIANGLES, gpuData.indexCount, GL_UNSIGNED_INT, nullptr);
    gpuData.vao.release();
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

// ---- Mesh Management ----

void Viewport::addMesh(uint64_t id, std::shared_ptr<dc3d::geometry::MeshData> mesh)
{
    if (!mesh) {
        qWarning() << "Viewport::addMesh - null mesh";
        return;
    }
    
    m_meshes[id] = mesh;
    
    // Upload to GPU if OpenGL is initialized
    if (m_initialized) {
        makeCurrent();
        uploadMeshToGPU(id, *mesh);
        doneCurrent();
        update();
    }
    
    // Add to selection renderer
    if (m_selectionRenderer) {
        m_selectionRenderer->addMesh(static_cast<uint32_t>(id), mesh.get());
    }
    
    qDebug() << "Viewport::addMesh - Added mesh" << id 
             << "with" << mesh->vertexCount() << "vertices";
}

void Viewport::removeMesh(uint64_t id)
{
    auto meshIt = m_meshes.find(id);
    if (meshIt != m_meshes.end()) {
        m_meshes.erase(meshIt);
    }
    
    auto gpuIt = m_meshGPUData.find(id);
    if (gpuIt != m_meshGPUData.end()) {
        if (m_initialized) {
            makeCurrent();
            if (gpuIt->second && gpuIt->second->valid) {
                gpuIt->second->vbo.destroy();
                gpuIt->second->ebo.destroy();
                gpuIt->second->vao.destroy();
            }
            doneCurrent();
        }
        m_meshGPUData.erase(gpuIt);
    }
    
    // Remove from selection renderer
    if (m_selectionRenderer) {
        m_selectionRenderer->removeMesh(static_cast<uint32_t>(id));
    }
    
    update();
    qDebug() << "Viewport::removeMesh - Removed mesh" << id;
}

void Viewport::clearMeshes()
{
    if (m_initialized) {
        makeCurrent();
        for (auto& pair : m_meshGPUData) {
            if (pair.second && pair.second->valid) {
                pair.second->vbo.destroy();
                pair.second->ebo.destroy();
                pair.second->vao.destroy();
            }
        }
        doneCurrent();
    }
    
    m_meshes.clear();
    m_meshGPUData.clear();
    update();
}

bool Viewport::hasMesh(uint64_t id) const
{
    return m_meshes.find(id) != m_meshes.end();
}

void Viewport::uploadMeshToGPU(uint64_t id, const dc3d::geometry::MeshData& mesh)
{
    if (mesh.isEmpty()) {
        qWarning() << "Viewport::uploadMeshToGPU - empty mesh";
        return;
    }
    
    auto gpuData = std::make_unique<MeshGPUData>();
    
    // Create and bind VAO
    gpuData->vao.create();
    gpuData->vao.bind();
    
    // Interleave vertex data: position (3) + normal (3) = 6 floats per vertex
    const auto& vertices = mesh.vertices();
    const auto& normals = mesh.normals();
    const auto& indices = mesh.indices();
    
    bool hasNormals = mesh.hasNormals();
    size_t floatsPerVertex = hasNormals ? 6 : 3;
    std::vector<float> interleavedData(vertices.size() * floatsPerVertex);
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t offset = i * floatsPerVertex;
        interleavedData[offset + 0] = vertices[i].x;
        interleavedData[offset + 1] = vertices[i].y;
        interleavedData[offset + 2] = vertices[i].z;
        
        if (hasNormals) {
            interleavedData[offset + 3] = normals[i].x;
            interleavedData[offset + 4] = normals[i].y;
            interleavedData[offset + 5] = normals[i].z;
        }
    }
    
    // Create and fill VBO
    gpuData->vbo.create();
    gpuData->vbo.bind();
    gpuData->vbo.allocate(interleavedData.data(), 
                          static_cast<int>(interleavedData.size() * sizeof(float)));
    
    // Set up vertex attributes
    int stride = static_cast<int>(floatsPerVertex * sizeof(float));
    
    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    
    // Normal attribute (location 1)
    if (hasNormals) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 
                              reinterpret_cast<void*>(3 * sizeof(float)));
    }
    
    // Create and fill EBO (index buffer)
    gpuData->ebo.create();
    gpuData->ebo.bind();
    gpuData->ebo.allocate(indices.data(), 
                          static_cast<int>(indices.size() * sizeof(uint32_t)));
    
    gpuData->indexCount = static_cast<uint32_t>(indices.size());
    gpuData->vertexCount = static_cast<uint32_t>(vertices.size());
    
    // Store bounds
    const auto& bounds = mesh.boundingBox();
    gpuData->boundsMin = QVector3D(bounds.min.x, bounds.min.y, bounds.min.z);
    gpuData->boundsMax = QVector3D(bounds.max.x, bounds.max.y, bounds.max.z);
    
    // Unbind VBO and EBO before releasing VAO to prevent corrupting default VAO state
    gpuData->vbo.release();
    gpuData->ebo.release();
    gpuData->vao.release();
    gpuData->valid = true;
    
    m_meshGPUData[id] = std::move(gpuData);
    
    qDebug() << "Viewport::uploadMeshToGPU - Uploaded mesh" << id 
             << "with" << mesh.vertexCount() << "vertices,"
             << mesh.faceCount() << "faces";
}

BoundingBox Viewport::computeSceneBounds() const
{
    BoundingBox bounds;
    bounds.min = QVector3D(std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max());
    bounds.max = QVector3D(std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest());
    
    bool hasBounds = false;
    for (const auto& pair : m_meshGPUData) {
        if (pair.second && pair.second->valid) {
            bounds.min.setX(std::min(bounds.min.x(), pair.second->boundsMin.x()));
            bounds.min.setY(std::min(bounds.min.y(), pair.second->boundsMin.y()));
            bounds.min.setZ(std::min(bounds.min.z(), pair.second->boundsMin.z()));
            bounds.max.setX(std::max(bounds.max.x(), pair.second->boundsMax.x()));
            bounds.max.setY(std::max(bounds.max.y(), pair.second->boundsMax.y()));
            bounds.max.setZ(std::max(bounds.max.z(), pair.second->boundsMax.z()));
            hasBounds = true;
        }
    }
    
    if (!hasBounds) {
        bounds.min = QVector3D(-10, -10, -10);
        bounds.max = QVector3D(10, 10, 10);
    }
    
    return bounds;
}

// ---- Mouse Events ----

void Viewport::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    m_mouseDownPos = event->pos();
    
    if (event->button() == Qt::MiddleButton) {
        if (m_shiftPressed) {
            m_navMode = NavigationMode::Pan;
        } else {
            m_navMode = NavigationMode::Orbit;
        }
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::RightButton) {
        m_navMode = NavigationMode::Zoom;
        setCursor(Qt::SizeVerCursor);
    } else if (event->button() == Qt::LeftButton) {
        // Left click could be selection or box selection start
        m_isBoxSelecting = false;
    }
    
    event->accept();
}

void Viewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_navMode = NavigationMode::None;
        setCursor(Qt::ArrowCursor);
    } else if (event->button() == Qt::LeftButton) {
        // Check if this was a click or a drag
        QPoint delta = event->pos() - m_mouseDownPos;
        bool isDrag = (delta.manhattanLength() > 5);
        
        if (isDrag && m_isBoxSelecting) {
            // Complete box selection
            QRect selectionRect = QRect(m_mouseDownPos, event->pos()).normalized();
            emit boxSelectionComplete(selectionRect, m_shiftPressed);
        } else if (!isDrag) {
            // Single click selection
            emit selectionClick(event->pos(), m_shiftPressed, m_ctrlPressed);
        }
        
        m_isBoxSelecting = false;
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
    float delta = event->angleDelta().y() / 120.0f;
    m_camera->zoom(delta);
    
    emit cameraChanged();
    update();
    event->accept();
}

// ---- Keyboard Events ----

void Viewport::keyPressEvent(QKeyEvent* event)
{
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
    
    switch (event->key()) {
        case Qt::Key_F:
            fitView();
            break;
            
        case Qt::Key_G:
            setGridVisible(!isGridVisible());
            update();
            break;
            
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
    m_shiftPressed = false;
    m_ctrlPressed = false;
    m_altPressed = false;
    m_navMode = NavigationMode::None;
    m_isBoxSelecting = false;  // Reset box selection state on focus loss
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
    BoundingBox bounds = computeSceneBounds();
    fitView(bounds);
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
    float x = static_cast<float>(screenPos.x());
    float y = static_cast<float>(height() - screenPos.y());
    float w = static_cast<float>(width());
    float h = static_cast<float>(height());
    
    float ndcX = (2.0f * x / w) - 1.0f;
    float ndcY = (2.0f * y / h) - 1.0f;
    float ndcZ = 2.0f * depth - 1.0f;
    
    QVector4D clipCoords(ndcX, ndcY, ndcZ, 1.0f);
    
    QMatrix4x4 invVP = m_camera->viewProjectionMatrix().inverted();
    QVector4D worldCoords = invVP * clipCoords;
    
    if (qFuzzyIsNull(worldCoords.w())) {
        return QVector3D();
    }
    
    return worldCoords.toVector3D() / worldCoords.w();
}

QVector3D Viewport::unprojectMouse(const QPoint& pos) const
{
    QVector3D nearPoint = screenToWorld(pos, 0.0f);
    QVector3D farPoint = screenToWorld(pos, 1.0f);
    
    QVector3D ray = (farPoint - nearPoint).normalized();
    QVector3D origin = nearPoint;
    
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
