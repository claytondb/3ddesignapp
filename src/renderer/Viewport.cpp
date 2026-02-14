/**
 * @file Viewport.cpp
 * @brief Implementation of OpenGL viewport widget with mesh rendering
 */

#include "Viewport.h"
#include "Camera.h"
#include "GridRenderer.h"
#include "ShaderProgram.h"
#include "SelectionRenderer.h"
#include "TransformGizmo.h"
#include "Picking.h"
#include "geometry/MeshData.h"
#include "core/Selection.h"
#include "app/Application.h"
#include "ui/ViewPresetsWidget.h"

#include <algorithm>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
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

// Gradient background shaders - professional look like Blender/Maya
static const char* GRADIENT_VERTEX_SHADER = R"(
#version 410 core

layout(location = 0) in vec2 position;

out vec2 vUV;

void main() {
    vUV = position * 0.5 + 0.5;  // Map from [-1,1] to [0,1]
    gl_Position = vec4(position, 0.999, 1.0);  // Far depth to render behind everything
}
)";

static const char* GRADIENT_FRAGMENT_SHADER = R"(
#version 410 core

in vec2 vUV;

uniform vec3 topColor;
uniform vec3 bottomColor;

out vec4 fragColor;

void main() {
    // Smooth gradient from bottom to top with slight vignette
    float t = vUV.y;
    
    // Apply slight ease for smoother gradient
    t = t * t * (3.0 - 2.0 * t);
    
    vec3 color = mix(bottomColor, topColor, t);
    
    // Subtle radial vignette (darker at edges)
    vec2 center = vUV - 0.5;
    float vignette = 1.0 - dot(center, center) * 0.15;
    color *= vignette;
    
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
    if (m_gizmo) {
        m_gizmo->cleanup();
    }
    doneCurrent();
}

void Viewport::initializeGL()
{
    initializeOpenGLFunctions();
    
    qDebug() << "OpenGL Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << "GLSL Version:" << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    qDebug() << "Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    
    // Check MSAA support
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    qDebug() << "MSAA Samples:" << samples;
    
    setupOpenGLState();
    setupMeshShader();
    setupGradientShader();
    
    // Initialize grid renderer
    if (!m_gridRenderer->initialize()) {
        qWarning() << "Failed to initialize grid renderer";
    }
    
    // Initialize selection renderer
    m_selectionRenderer = std::make_unique<dc3d::renderer::SelectionRenderer>();
    if (!m_selectionRenderer->initialize()) {
        qWarning() << "Failed to initialize selection renderer";
    }
    
    // Initialize transform gizmo
    m_gizmo = std::make_unique<TransformGizmo>();
    m_gizmo->initialize();
    m_gizmo->setVisible(false);  // Hidden until something is selected
    m_gizmo->setScreenSpaceSizing(true);
    
    // Set initial camera position
    m_camera->lookAt(
        QVector3D(10.0f, 8.0f, 10.0f),  // Position
        QVector3D(0.0f, 0.0f, 0.0f),    // Target
        QVector3D(0.0f, 1.0f, 0.0f)     // Up
    );
    
    // Setup view presets widget (after GL init)
    setupViewPresetsWidget();
    
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

void Viewport::setupGradientShader()
{
    m_gradientShader = std::make_unique<QOpenGLShaderProgram>();
    
    if (!m_gradientShader->addShaderFromSourceCode(QOpenGLShader::Vertex, GRADIENT_VERTEX_SHADER)) {
        qWarning() << "Gradient vertex shader compile error:" << m_gradientShader->log();
        return;
    }
    
    if (!m_gradientShader->addShaderFromSourceCode(QOpenGLShader::Fragment, GRADIENT_FRAGMENT_SHADER)) {
        qWarning() << "Gradient fragment shader compile error:" << m_gradientShader->log();
        return;
    }
    
    if (!m_gradientShader->link()) {
        qWarning() << "Gradient shader link error:" << m_gradientShader->log();
        return;
    }
    
    // Create fullscreen quad VAO/VBO
    static const float quadVertices[] = {
        -1.0f, -1.0f,  // Bottom-left
         1.0f, -1.0f,  // Bottom-right
        -1.0f,  1.0f,  // Top-left
         1.0f,  1.0f   // Top-right
    };
    
    m_gradientVAO.create();
    m_gradientVAO.bind();
    
    m_gradientVBO.create();
    m_gradientVBO.bind();
    m_gradientVBO.allocate(quadVertices, sizeof(quadVertices));
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    
    m_gradientVBO.release();
    m_gradientVAO.release();
    
    qDebug() << "Gradient background shader initialized";
}

void Viewport::setupViewPresetsWidget()
{
    m_viewPresetsWidget = new ViewPresetsWidget(this, this);
    m_viewPresetsWidget->show();
    
    // Position in top-right corner
    int margin = 8;
    m_viewPresetsWidget->move(width() - m_viewPresetsWidget->width() - margin, margin);
    
    // Connect view change signal
    connect(m_viewPresetsWidget, &ViewPresetsWidget::viewChanged, this, [this](const QString& name) {
        updateViewName();
    });
}

void Viewport::renderGradientBackground()
{
    if (!m_gradientEnabled || !m_gradientShader || !m_gradientShader->isLinked()) {
        // Fall back to solid color clear
        glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), 
                     m_backgroundColor.blueF(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    
    // Clear depth but not color (gradient will fill it)
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Disable depth writing for background
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    
    m_gradientShader->bind();
    
    // Set gradient colors (convert from sRGB to linear)
    auto toLinear = [](const QColor& c) -> QVector3D {
        // Simple gamma approximation for sRGB
        float r = std::pow(c.redF(), 2.2f);
        float g = std::pow(c.greenF(), 2.2f);
        float b = std::pow(c.blueF(), 2.2f);
        return QVector3D(r, g, b);
    };
    
    m_gradientShader->setUniformValue("topColor", toLinear(m_gradientTopColor));
    m_gradientShader->setUniformValue("bottomColor", toLinear(m_gradientBottomColor));
    
    m_gradientVAO.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_gradientVAO.release();
    
    m_gradientShader->release();
    
    // Re-enable depth testing
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Viewport::updateViewName()
{
    // Determine view name based on camera orientation
    QVector3D forward = m_camera->forwardVector();
    QVector3D up = m_camera->upVector();
    
    // Threshold for considering a view as orthographic
    const float threshold = 0.98f;
    
    // Check standard views
    if (qAbs(forward.z()) > threshold && qAbs(up.y()) > threshold) {
        m_currentViewName = forward.z() > 0 ? "Back" : "Front";
    } else if (qAbs(forward.x()) > threshold && qAbs(up.y()) > threshold) {
        m_currentViewName = forward.x() > 0 ? "Left" : "Right";
    } else if (qAbs(forward.y()) > threshold) {
        m_currentViewName = forward.y() > 0 ? "Bottom" : "Top";
    } else {
        // Check for isometric-ish view
        bool isOrtho = !m_camera->isPerspective();
        m_currentViewName = isOrtho ? "Orthographic" : "Perspective";
    }
    
    update();  // Trigger repaint to update overlay
}

void Viewport::paintGL()
{
    // Update camera animation if active
    if (m_camera->isAnimating()) {
        // Calculate delta time (approximate - better to use QElapsedTimer)
        static qint64 lastTime = m_frameTimer.elapsed();
        qint64 currentTime = m_frameTimer.elapsed();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        // Cap delta time to prevent huge jumps
        deltaTime = std::min(deltaTime, 0.1f);
        
        if (m_camera->updateAnimation(deltaTime)) {
            // Animation still in progress - request another frame
            update();
        }
        emit cameraChanged();
        updateViewName();
    }
    
    // Render gradient background (or clear with solid color)
    renderGradientBackground();
    
    // Render grid
    renderGrid();
    
    // Render meshes
    renderMeshes();
    
    // Render hover highlight (pre-selection feedback)
    if (m_selectionRenderer && m_selection && m_hoverEnabled && m_hoverHitInfo.hit) {
        // Don't show hover for already-selected objects
        bool isAlreadySelected = m_selection->isObjectSelected(m_hoverHitInfo.meshId);
        if (!isAlreadySelected) {
            m_selectionRenderer->renderHover(*m_camera, m_hoverHitInfo, m_selection->mode(), size());
        }
    }
    
    // Render selection highlights
    if (m_selectionRenderer && m_selection) {
        m_selectionRenderer->render(*m_camera, *m_selection, size());
    }
    
    // Render transform gizmo on selected objects
    if (m_gizmo && m_gizmo->isVisible()) {
        m_gizmo->render(m_camera->viewMatrix(), m_camera->projectionMatrix(), size());
    }
    
    // Render box selection overlay if active
    if (m_isBoxSelecting && m_selectionRenderer) {
        m_selectionRenderer->renderBoxSelection(m_mouseDownPos, m_lastMousePos, size());
    }
    
    // Update FPS counter
    updateFPS();
    
    // Render FPS overlay if enabled
    if (m_showFPS) {
        renderFPSOverlay();
    }
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
    
    // Material properties
    m_meshShader->setUniform("baseColor", QVector3D(0.7f, 0.7f, 0.75f));
    m_meshShader->setUniform("metallic", 0.0f);
    m_meshShader->setUniform("roughness", 0.5f);
    m_meshShader->setUniform("useVertexColor", false);
    m_meshShader->setUniform("useDeviation", false);
    
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
        
        // Emit FPS update signal for status bar
        emit fpsUpdated(static_cast<int>(m_fps + 0.5f));
    }
}

void Viewport::renderFPSOverlay()
{
    // Use QPainter overlay for FPS display (rendered after GL content)
    // This is deferred to paintEvent override - Qt handles this via QPainter on QOpenGLWidget
    // For now, emit signal and let MainWindow/StatusBar display it
    // A proper implementation would use a separate overlay widget or QPainter in paintEvent
    
    // The actual rendering happens in the overridden paintEvent if needed,
    // but for simplicity, we rely on the status bar FPS display
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
    // CRITICAL FIX: Use isValid() for comprehensive validation to prevent crashes
    if (mesh.isEmpty() || !mesh.isValid()) {
        qWarning() << "Viewport::uploadMeshToGPU - empty or invalid mesh";
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
    
    // Unbind VBO before releasing VAO (VBO binding is not part of VAO state)
    // NOTE: Do NOT unbind EBO here! The EBO binding IS part of VAO state.
    // Unbinding EBO while VAO is bound would remove the index buffer association,
    // causing glDrawElements to crash when trying to read from address 0.
    gpuData->vbo.release();
    // gpuData->ebo.release();  // REMOVED - would break VAO's EBO binding!
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
    
    // Tinkercad-style camera controls:
    // - Right-click + drag = Orbit
    // - Shift + Right-click + drag = Pan
    // - Middle-click + drag = Pan
    // - Mouse wheel = Zoom
    
    if (event->button() == Qt::MiddleButton) {
        // Middle-click = Pan (Tinkercad style)
        m_navMode = NavigationMode::Pan;
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::RightButton) {
        // Right-click = Orbit, Shift+Right-click = Pan (Tinkercad style)
        if (m_shiftPressed) {
            m_navMode = NavigationMode::Pan;
        } else {
            m_navMode = NavigationMode::Orbit;
        }
        setCursor(Qt::ClosedHandCursor);
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
                
                // Track hover for pre-selection feedback
                if (m_hoverEnabled) {
                    auto* app = dc3d::Application::instance();
                    if (app && app->picking()) {
                        dc3d::core::HitInfo newHit = app->picking()->pick(event->pos(), size(), *m_camera);
                        
                        // Only update if hover target changed
                        if (newHit.hit != m_hoverHitInfo.hit || 
                            (newHit.hit && newHit.meshId != m_hoverHitInfo.meshId)) {
                            m_hoverHitInfo = newHit;
                            emit hoverChanged(m_hoverHitInfo);
                            update();
                        } else if (newHit.hit && newHit.faceIndex != m_hoverHitInfo.faceIndex) {
                            // For face mode, update on face change too
                            m_hoverHitInfo = newHit;
                            update();
                        }
                    }
                }
                
                // Check if this should start a box selection
                if (event->buttons() & Qt::LeftButton) {
                    QPoint dragDelta = event->pos() - m_mouseDownPos;
                    if (dragDelta.manhattanLength() > 5 && !m_isBoxSelecting) {
                        m_isBoxSelecting = true;
                        setCursor(Qt::CrossCursor);
                    }
                    if (m_isBoxSelecting) {
                        update();  // Redraw box selection overlay
                    }
                }
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
        // Tinkercad-style: Shift switches right-click from Orbit to Pan
        if (m_navMode == NavigationMode::Orbit) {
            m_navMode = NavigationMode::Pan;
        }
    } else if (event->key() == Qt::Key_Control) {
        m_ctrlPressed = true;
    } else if (event->key() == Qt::Key_Alt) {
        m_altPressed = true;
    }
    
    // Handle standard view shortcuts (numpad and regular keys)
    // Numpad 1/3/7 = Front/Right/Top, with Ctrl = Back/Left/Bottom
    bool handled = true;
    switch (event->key()) {
        // ========== Transform Mode Shortcuts (Blender-style) ==========
        case Qt::Key_W:
            // W = Move/Translate mode
            setGizmoMode(static_cast<int>(GizmoMode::Translate));
            emit transformModeChanged(static_cast<int>(GizmoMode::Translate));
            break;
            
        case Qt::Key_E:
            // E = Rotate mode
            setGizmoMode(static_cast<int>(GizmoMode::Rotate));
            emit transformModeChanged(static_cast<int>(GizmoMode::Rotate));
            break;
            
        case Qt::Key_R:
            // R = Scale mode (unless Ctrl is held for other function)
            if (!m_ctrlPressed) {
                setGizmoMode(static_cast<int>(GizmoMode::Scale));
                emit transformModeChanged(static_cast<int>(GizmoMode::Scale));
            } else {
                handled = false;
            }
            break;
            
        // ========== Axis Constraint Shortcuts ==========
        case Qt::Key_X:
            // X = Constrain to X axis, Shift+X = Constrain to YZ plane
            if (m_gizmo) {
                if (m_shiftPressed) {
                    m_gizmo->setAxisConstraint(AxisConstraint::PlaneYZ);
                } else {
                    // Toggle: if already X, clear; otherwise set X
                    if (m_gizmo->axisConstraint() == AxisConstraint::X) {
                        m_gizmo->clearAxisConstraint();
                    } else {
                        m_gizmo->setAxisConstraint(AxisConstraint::X);
                    }
                }
                emit axisConstraintChanged(m_gizmo->axisConstraint());
                update();
            }
            break;
            
        case Qt::Key_Y:
            // Y = Constrain to Y axis, Shift+Y = Constrain to XZ plane
            if (m_gizmo) {
                if (m_shiftPressed) {
                    m_gizmo->setAxisConstraint(AxisConstraint::PlaneXZ);
                } else {
                    if (m_gizmo->axisConstraint() == AxisConstraint::Y) {
                        m_gizmo->clearAxisConstraint();
                    } else {
                        m_gizmo->setAxisConstraint(AxisConstraint::Y);
                    }
                }
                emit axisConstraintChanged(m_gizmo->axisConstraint());
                update();
            }
            break;
            
        case Qt::Key_Z:
            // Z = Constrain to Z axis (unless Ctrl+Z for undo)
            // Shift+Z = Constrain to XY plane
            if (m_ctrlPressed) {
                // Ctrl+Z is undo - don't handle here
                handled = false;
            } else if (m_gizmo) {
                if (m_shiftPressed) {
                    m_gizmo->setAxisConstraint(AxisConstraint::PlaneXY);
                } else {
                    if (m_gizmo->axisConstraint() == AxisConstraint::Z) {
                        m_gizmo->clearAxisConstraint();
                    } else {
                        m_gizmo->setAxisConstraint(AxisConstraint::Z);
                    }
                }
                emit axisConstraintChanged(m_gizmo->axisConstraint());
                update();
            }
            break;
            
        // ========== Coordinate Space Toggle ==========
        case Qt::Key_L:
            // L = Toggle Local/World coordinate space
            if (m_gizmo) {
                m_gizmo->toggleCoordinateSpace();
                emit coordinateSpaceChanged(m_gizmo->coordinateSpace());
                update();
            }
            break;
            
        // ========== Pivot Point ==========
        case Qt::Key_Period:
            // . = Cycle through pivot point options
            if (m_gizmo) {
                m_gizmo->cyclePivotPoint();
                emit pivotPointChanged(m_gizmo->pivotPoint());
                update();
            }
            break;
            
        // ========== Numeric Input ==========
        case Qt::Key_Minus:
        case Qt::Key_0:
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9:
            // Handle numeric input during transform
            if (m_gizmo && m_gizmo->isVisible() && !m_ctrlPressed && !m_altPressed) {
                if (!m_gizmo->isNumericInputActive()) {
                    m_gizmo->startNumericInput();
                    emit numericInputStarted();
                }
                
                QChar c;
                if (event->key() == Qt::Key_Minus) {
                    c = '-';
                } else if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) {
                    c = QChar('0' + (event->key() - Qt::Key_0));
                }
                
                if (!c.isNull()) {
                    m_gizmo->appendNumericInput(c);
                    emit numericInputChanged(m_gizmo->numericInputString());
                }
            } else {
                // Fall through for view shortcuts when gizmo not active
                if (event->key() == Qt::Key_0) {
                    setStandardView("isometric");
                } else if (event->key() == Qt::Key_1) {
                    if (event->modifiers() & Qt::KeypadModifier) {
                        if (m_ctrlPressed) setStandardView("back");
                        else setStandardView("front");
                    } else {
                        setStandardView("front");
                    }
                } else if (event->key() == Qt::Key_3) {
                    if (event->modifiers() & Qt::KeypadModifier) {
                        if (m_ctrlPressed) setStandardView("left");
                        else setStandardView("right");
                    } else {
                        setStandardView("left");
                    }
                } else if (event->key() == Qt::Key_7) {
                    if (event->modifiers() & Qt::KeypadModifier) {
                        if (m_ctrlPressed) setStandardView("bottom");
                        else setStandardView("top");
                    } else {
                        setStandardView("top");
                    }
                } else {
                    handled = false;
                }
            }
            break;
            
        case Qt::Key_Comma:
            // Comma for vector input separation
            if (m_gizmo && m_gizmo->isNumericInputActive()) {
                m_gizmo->appendNumericInput(',');
                emit numericInputChanged(m_gizmo->numericInputString());
            } else {
                handled = false;
            }
            break;
            
        case Qt::Key_Tab:
            // Tab could cycle through numeric input fields
            handled = false;
            break;
            
        // ========== Standard View & Navigation Shortcuts ==========
        case Qt::Key_F:
            // Tinkercad-style: F key = Zoom to fit / focus on selection
            fitView();
            break;
            
        case Qt::Key_Home:
            // Tinkercad-style: Home key = Reset to default view
            resetView();
            break;
            
        case Qt::Key_G:
            setGridVisible(!isGridVisible());
            update();
            break;
            
        case Qt::Key_QuoteLeft:  // Backtick (`) - toggle FPS display
            toggleFPS();
            break;
            
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            // During numeric input, backspace deletes characters
            if (m_gizmo && m_gizmo->isNumericInputActive()) {
                m_gizmo->backspaceNumericInput();
                emit numericInputChanged(m_gizmo->numericInputString());
            } else {
                // Delete selected objects
                emit deleteRequested();
            }
            break;
            
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // Confirm numeric input
            if (m_gizmo && m_gizmo->isNumericInputActive()) {
                m_gizmo->endNumericInput(true);
                emit numericInputConfirmed(m_gizmo->numericInputVector());
            } else {
                handled = false;
            }
            break;
            
        case Qt::Key_Escape:
            // Cancel numeric input, box selection, or clear selection
            if (m_gizmo && m_gizmo->isNumericInputActive()) {
                m_gizmo->endNumericInput(false);
                emit numericInputCancelled();
            } else if (m_gizmo && m_gizmo->axisConstraint() != AxisConstraint::None) {
                // Clear axis constraint
                m_gizmo->clearAxisConstraint();
                emit axisConstraintChanged(m_gizmo->axisConstraint());
                update();
            } else if (m_isBoxSelecting) {
                m_isBoxSelecting = false;
                setCursor(Qt::ArrowCursor);
                update();
            } else if (m_selection && !m_selection->isEmpty()) {
                m_selection->clear();
            }
            break;
            
        case Qt::Key_A:
            // Ctrl+A = Select all (when implemented)
            if (m_ctrlPressed) {
                // TODO: Implement select all
            } else {
                handled = false;
            }
            break;
            
        default:
            handled = false;
            break;
    }
    
    if (!handled) {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }
    
    event->accept();
}

void Viewport::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_shiftPressed = false;
        // Tinkercad-style: When Shift is released during right-click drag,
        // switch back from Pan to Orbit (only if we were panning via right-click)
        // Note: Middle-click pan stays as pan regardless of Shift
        if (m_navMode == NavigationMode::Pan) {
            // Check if right mouse button is still pressed (ongoing drag)
            if (QApplication::mouseButtons() & Qt::RightButton) {
                m_navMode = NavigationMode::Orbit;
            }
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
    
    // If animation started, request continuous redraws
    if (m_camera->isAnimating()) {
        update();  // First update triggers animation loop in paintGL
    } else {
        emit cameraChanged();
        update();
    }
}

void Viewport::fitView()
{
    BoundingBox bounds = computeSceneBounds();
    fitView(bounds);
}

void Viewport::fitView(const BoundingBox& bounds)
{
    m_camera->fitToView(bounds);
    
    // If animation started, request continuous redraws
    if (m_camera->isAnimating()) {
        update();  // First update triggers animation loop in paintGL
    } else {
        emit cameraChanged();
        update();
    }
}

void Viewport::zoomToSelection()
{
    // If no selection manager or no selection, fall back to fit all
    if (!m_selection || m_selection->isEmpty()) {
        fitView();
        return;
    }
    
    // Compute bounds of selected objects
    BoundingBox selectionBounds;
    selectionBounds.min = QVector3D(std::numeric_limits<float>::max(),
                                     std::numeric_limits<float>::max(),
                                     std::numeric_limits<float>::max());
    selectionBounds.max = QVector3D(std::numeric_limits<float>::lowest(),
                                     std::numeric_limits<float>::lowest(),
                                     std::numeric_limits<float>::lowest());
    
    bool hasBounds = false;
    
    // Get selected mesh IDs
    auto selectedMeshIds = m_selection->selectedMeshIds();
    
    for (uint32_t meshId : selectedMeshIds) {
        auto it = m_meshGPUData.find(static_cast<uint64_t>(meshId));
        if (it != m_meshGPUData.end() && it->second && it->second->valid) {
            selectionBounds.min.setX(std::min(selectionBounds.min.x(), it->second->boundsMin.x()));
            selectionBounds.min.setY(std::min(selectionBounds.min.y(), it->second->boundsMin.y()));
            selectionBounds.min.setZ(std::min(selectionBounds.min.z(), it->second->boundsMin.z()));
            selectionBounds.max.setX(std::max(selectionBounds.max.x(), it->second->boundsMax.x()));
            selectionBounds.max.setY(std::max(selectionBounds.max.y(), it->second->boundsMax.y()));
            selectionBounds.max.setZ(std::max(selectionBounds.max.z(), it->second->boundsMax.z()));
            hasBounds = true;
        }
    }
    
    if (hasBounds && selectionBounds.isValid()) {
        fitView(selectionBounds);
    } else {
        // Fall back to fit all
        fitView();
    }
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

// ---- Transform Gizmo ----

void Viewport::updateGizmo(const QVector3D& center, bool visible)
{
    if (!m_gizmo) return;
    
    m_gizmo->setVisible(visible);
    if (visible) {
        m_gizmo->setPosition(center);
    }
    update();
}

void Viewport::setGizmoMode(int mode)
{
    if (!m_gizmo) return;
    
    m_gizmo->setMode(mode);
    update();
}

} // namespace dc
