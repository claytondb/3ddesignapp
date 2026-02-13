/**
 * @file SelectionRenderer.cpp
 * @brief Implementation of selection highlight rendering
 */

#include "SelectionRenderer.h"
#include "Camera.h"
#include "../geometry/MeshData.h"

#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <algorithm>

namespace dc3d {
namespace renderer {

SelectionRenderer::SelectionRenderer()
{
}

SelectionRenderer::~SelectionRenderer()
{
    // Note: cleanup() should only be called explicitly when OpenGL context is current.
    // The owner must call cleanup() before destruction to avoid OpenGL calls
    // without a valid context, which causes undefined behavior or crashes.
}

bool SelectionRenderer::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    initializeOpenGLFunctions();
    
    if (!loadShaders()) {
        return false;
    }
    
    // Create box selection quad VAO/VBO
    glGenVertexArrays(1, &m_boxVAO);
    glGenBuffers(1, &m_boxVBO);
    
    glBindVertexArray(m_boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind VBO before releasing VAO
    glBindVertexArray(0);
    
    // Create point VAO/VBO
    glGenVertexArrays(1, &m_pointVAO);
    glGenBuffers(1, &m_pointVBO);
    
    glBindVertexArray(m_pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind VBO before releasing VAO
    glBindVertexArray(0);
    
    // Create line VAO/VBO
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);
    
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind VBO before releasing VAO
    glBindVertexArray(0);
    
    m_initialized = true;
    return true;
}

void SelectionRenderer::cleanup()
{
    if (!m_initialized) {
        return;
    }
    
    // Clean up mesh buffers
    for (auto& mesh : m_meshes) {
        deleteMeshBuffers(mesh);
    }
    m_meshes.clear();
    
    // Clean up box selection buffers
    if (m_boxVAO) {
        glDeleteVertexArrays(1, &m_boxVAO);
        m_boxVAO = 0;
    }
    if (m_boxVBO) {
        glDeleteBuffers(1, &m_boxVBO);
        m_boxVBO = 0;
    }
    
    // Clean up point buffers
    if (m_pointVAO) {
        glDeleteVertexArrays(1, &m_pointVAO);
        m_pointVAO = 0;
    }
    if (m_pointVBO) {
        glDeleteBuffers(1, &m_pointVBO);
        m_pointVBO = 0;
    }
    
    // Clean up line buffers
    if (m_lineVAO) {
        glDeleteVertexArrays(1, &m_lineVAO);
        m_lineVAO = 0;
    }
    if (m_lineVBO) {
        glDeleteBuffers(1, &m_lineVBO);
        m_lineVBO = 0;
    }
    
    // Clean up shaders
    m_selectionShader.reset();
    m_pointShader.reset();
    m_lineShader.reset();
    m_boxShader.reset();
    
    m_initialized = false;
}

bool SelectionRenderer::loadShaders()
{
    // Selection shader (for faces and object outlines)
    m_selectionShader = std::make_unique<QOpenGLShaderProgram>();
    
    if (!m_selectionShader->addShaderFromSourceFile(
            QOpenGLShader::Vertex, ":/shaders/selection.vert")) {
        qWarning() << "Failed to load selection vertex shader:" 
                   << m_selectionShader->log();
        return false;
    }
    
    if (!m_selectionShader->addShaderFromSourceFile(
            QOpenGLShader::Fragment, ":/shaders/selection.frag")) {
        qWarning() << "Failed to load selection fragment shader:"
                   << m_selectionShader->log();
        return false;
    }
    
    if (!m_selectionShader->link()) {
        qWarning() << "Failed to link selection shader:"
                   << m_selectionShader->log();
        return false;
    }
    
    // Simple point shader
    m_pointShader = std::make_unique<QOpenGLShaderProgram>();
    
    const char* pointVert = R"(
        #version 410 core
        layout(location = 0) in vec3 position;
        uniform mat4 mvp;
        uniform float pointSize;
        void main() {
            gl_Position = mvp * vec4(position, 1.0);
            gl_PointSize = pointSize;
        }
    )";
    
    const char* pointFrag = R"(
        #version 410 core
        uniform vec4 color;
        out vec4 fragColor;
        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            if (length(coord) > 0.5) discard;
            fragColor = color;
        }
    )";
    
    if (!m_pointShader->addShaderFromSourceCode(QOpenGLShader::Vertex, pointVert) ||
        !m_pointShader->addShaderFromSourceCode(QOpenGLShader::Fragment, pointFrag) ||
        !m_pointShader->link()) {
        qWarning() << "Failed to create point shader";
        return false;
    }
    
    // Simple line shader
    m_lineShader = std::make_unique<QOpenGLShaderProgram>();
    
    const char* lineVert = R"(
        #version 410 core
        layout(location = 0) in vec3 position;
        uniform mat4 mvp;
        void main() {
            gl_Position = mvp * vec4(position, 1.0);
        }
    )";
    
    const char* lineFrag = R"(
        #version 410 core
        uniform vec4 color;
        out vec4 fragColor;
        void main() {
            fragColor = color;
        }
    )";
    
    if (!m_lineShader->addShaderFromSourceCode(QOpenGLShader::Vertex, lineVert) ||
        !m_lineShader->addShaderFromSourceCode(QOpenGLShader::Fragment, lineFrag) ||
        !m_lineShader->link()) {
        qWarning() << "Failed to create line shader";
        return false;
    }
    
    // Box selection shader (2D quad)
    m_boxShader = std::make_unique<QOpenGLShaderProgram>();
    
    const char* boxVert = R"(
        #version 410 core
        layout(location = 0) in vec2 position;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";
    
    const char* boxFrag = R"(
        #version 410 core
        uniform vec4 fillColor;
        uniform vec4 borderColor;
        uniform vec2 rectMin;
        uniform vec2 rectMax;
        uniform float borderWidth;
        out vec4 fragColor;
        void main() {
            vec2 pos = gl_FragCoord.xy;
            
            // Check if near border
            float distLeft = pos.x - rectMin.x;
            float distRight = rectMax.x - pos.x;
            float distBottom = pos.y - rectMin.y;
            float distTop = rectMax.y - pos.y;
            
            float minDist = min(min(distLeft, distRight), min(distBottom, distTop));
            
            if (minDist < borderWidth) {
                fragColor = borderColor;
            } else {
                fragColor = fillColor;
            }
        }
    )";
    
    if (!m_boxShader->addShaderFromSourceCode(QOpenGLShader::Vertex, boxVert) ||
        !m_boxShader->addShaderFromSourceCode(QOpenGLShader::Fragment, boxFrag) ||
        !m_boxShader->link()) {
        qWarning() << "Failed to create box shader";
        return false;
    }
    
    return true;
}

glm::mat4 SelectionRenderer::toGlm(const QMatrix4x4& m)
{
    glm::mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = m(j, i);
        }
    }
    return result;
}

// ============================================================================
// Mesh Management
// ============================================================================

void SelectionRenderer::addMesh(uint32_t meshId, const geometry::MeshData* mesh,
                                const glm::mat4& transform)
{
    // Check if already exists
    if (SelectionMeshInfo* existing = findMesh(meshId)) {
        existing->mesh = mesh;
        existing->transform = transform;
        
        if (m_initialized) {
            deleteMeshBuffers(*existing);
            createMeshBuffers(*existing);
        }
        return;
    }
    
    SelectionMeshInfo info;
    info.meshId = meshId;
    info.mesh = mesh;
    info.transform = transform;
    info.visible = true;
    
    if (m_initialized && mesh) {
        createMeshBuffers(info);
    }
    
    m_meshes.push_back(std::move(info));
}

void SelectionRenderer::updateTransform(uint32_t meshId, const glm::mat4& transform)
{
    if (SelectionMeshInfo* mesh = findMesh(meshId)) {
        mesh->transform = transform;
    }
}

void SelectionRenderer::removeMesh(uint32_t meshId)
{
    auto it = std::find_if(m_meshes.begin(), m_meshes.end(),
                          [meshId](const SelectionMeshInfo& m) { return m.meshId == meshId; });
    
    if (it != m_meshes.end()) {
        if (m_initialized) {
            deleteMeshBuffers(*it);
        }
        m_meshes.erase(it);
    }
}

void SelectionRenderer::clearMeshes()
{
    if (m_initialized) {
        for (auto& mesh : m_meshes) {
            deleteMeshBuffers(mesh);
        }
    }
    m_meshes.clear();
}

SelectionMeshInfo* SelectionRenderer::findMesh(uint32_t meshId)
{
    for (auto& mesh : m_meshes) {
        if (mesh.meshId == meshId) {
            return &mesh;
        }
    }
    return nullptr;
}

const SelectionMeshInfo* SelectionRenderer::findMesh(uint32_t meshId) const
{
    for (const auto& mesh : m_meshes) {
        if (mesh.meshId == meshId) {
            return &mesh;
        }
    }
    return nullptr;
}

void SelectionRenderer::createMeshBuffers(SelectionMeshInfo& info)
{
    if (!info.mesh || info.mesh->isEmpty()) {
        return;
    }
    
    const auto& vertices = info.mesh->vertices();
    const auto& normals = info.mesh->normals();
    const auto& indices = info.mesh->indices();
    
    glGenVertexArrays(1, &info.vao);
    glBindVertexArray(info.vao);
    
    // Position buffer
    glGenBuffers(1, &info.vboPosition);
    glBindBuffer(GL_ARRAY_BUFFER, info.vboPosition);
    glBufferData(GL_ARRAY_BUFFER, 
                 vertices.size() * sizeof(glm::vec3),
                 vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    
    // Normal buffer
    if (!normals.empty()) {
        glGenBuffers(1, &info.vboNormal);
        glBindBuffer(GL_ARRAY_BUFFER, info.vboNormal);
        glBufferData(GL_ARRAY_BUFFER,
                     normals.size() * sizeof(glm::vec3),
                     normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(1);
    }
    
    // Index buffer
    glGenBuffers(1, &info.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, info.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(uint32_t),
                 indices.data(), GL_STATIC_DRAW);
    
    glBindVertexArray(0);
}

void SelectionRenderer::deleteMeshBuffers(SelectionMeshInfo& info)
{
    if (info.vao) {
        glDeleteVertexArrays(1, &info.vao);
        info.vao = 0;
    }
    if (info.vboPosition) {
        glDeleteBuffers(1, &info.vboPosition);
        info.vboPosition = 0;
    }
    if (info.vboNormal) {
        glDeleteBuffers(1, &info.vboNormal);
        info.vboNormal = 0;
    }
    if (info.ebo) {
        glDeleteBuffers(1, &info.ebo);
        info.ebo = 0;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void SelectionRenderer::render(const dc::Camera& camera, const core::Selection& selection)
{
    if (!m_initialized || selection.isEmpty()) {
        return;
    }
    
    // Group selection by mesh
    std::map<uint32_t, std::vector<uint32_t>> selectionByMesh;
    
    for (const auto& elem : selection.selectedElements()) {
        selectionByMesh[elem.meshId].push_back(elem.elementIndex);
    }
    
    // Setup rendering state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    if (m_config.xrayMode) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    
    core::SelectionMode mode = selection.mode();
    
    switch (mode) {
        case core::SelectionMode::Object:
            renderObjectSelection(camera, selection.selectedMeshIds());
            break;
            
        case core::SelectionMode::Face:
            for (const auto& [meshId, indices] : selectionByMesh) {
                renderFaceSelection(camera, meshId, indices);
            }
            break;
            
        case core::SelectionMode::Vertex:
            for (const auto& [meshId, indices] : selectionByMesh) {
                renderVertexSelection(camera, meshId, indices);
            }
            break;
            
        case core::SelectionMode::Edge:
            for (const auto& [meshId, indices] : selectionByMesh) {
                renderEdgeSelection(camera, meshId, indices);
            }
            break;
    }
    
    // Restore state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void SelectionRenderer::renderHover(const dc::Camera& camera,
                                    const core::HitInfo& hitInfo,
                                    core::SelectionMode mode)
{
    if (!m_initialized || !hitInfo.hit) {
        return;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // Render with hover color
    switch (mode) {
        case core::SelectionMode::Object:
            renderObjectSelection(camera, {hitInfo.meshId});
            break;
            
        case core::SelectionMode::Face:
            renderFaceSelection(camera, hitInfo.meshId, {hitInfo.faceIndex});
            break;
            
        case core::SelectionMode::Vertex:
            renderVertexSelection(camera, hitInfo.meshId, {hitInfo.closestVertex});
            break;
            
        case core::SelectionMode::Edge:
            {
                // Encode edge
                uint32_t v1, v2;
                if (hitInfo.closestEdge == 0) {
                    v1 = hitInfo.vertexIndices[0];
                    v2 = hitInfo.vertexIndices[1];
                } else if (hitInfo.closestEdge == 1) {
                    v1 = hitInfo.vertexIndices[1];
                    v2 = hitInfo.vertexIndices[2];
                } else {
                    v1 = hitInfo.vertexIndices[2];
                    v2 = hitInfo.vertexIndices[0];
                }
                if (v1 > v2) std::swap(v1, v2);
                uint32_t edgeIndex = (v2 << 16) | v1;
                renderEdgeSelection(camera, hitInfo.meshId, {edgeIndex});
            }
            break;
    }
    
    glDepthFunc(GL_LESS);
}

void SelectionRenderer::renderObjectSelection(const dc::Camera& camera,
                                              const std::vector<uint32_t>& meshIds)
{
    if (!m_selectionShader) return;
    
    m_selectionShader->bind();
    
    QMatrix4x4 view = camera.viewMatrix();
    QMatrix4x4 proj = camera.projectionMatrix();
    
    m_selectionShader->setUniformValue("view", view);
    m_selectionShader->setUniformValue("projection", proj);
    m_selectionShader->setUniformValue("selectionMode", 0);
    m_selectionShader->setUniformValue("opacity", m_config.opacity);
    m_selectionShader->setUniformValue("xrayMode", m_config.xrayMode);
    m_selectionShader->setUniformValue("highlightColor",
        QVector4D(m_config.objectColor.r, m_config.objectColor.g,
                  m_config.objectColor.b, m_config.objectColor.a));
    
    QVector3D camPos = camera.position();
    m_selectionShader->setUniformValue("cameraPosition", camPos);
    
    for (uint32_t meshId : meshIds) {
        const SelectionMeshInfo* info = findMesh(meshId);
        if (!info || !info->vao || !info->mesh) continue;
        
        QMatrix4x4 model;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                model(j, i) = info->transform[i][j];
            }
        }
        
        m_selectionShader->setUniformValue("model", model);
        
        QMatrix3x3 normalMatrix = model.normalMatrix();
        m_selectionShader->setUniformValue("normalMatrix", normalMatrix);
        
        glBindVertexArray(info->vao);
        
        // First pass: fill
        m_selectionShader->setUniformValue("highlightPass", 0);
        m_selectionShader->setUniformValue("outlineScale", 0.0f);
        glDrawElements(GL_TRIANGLES, 
                      static_cast<GLsizei>(info->mesh->indexCount()),
                      GL_UNSIGNED_INT, nullptr);
        
        // Second pass: outline
        glCullFace(GL_FRONT);  // Draw back faces expanded
        m_selectionShader->setUniformValue("highlightPass", 1);
        m_selectionShader->setUniformValue("outlineScale", m_config.outlineWidth);
        glDrawElements(GL_TRIANGLES,
                      static_cast<GLsizei>(info->mesh->indexCount()),
                      GL_UNSIGNED_INT, nullptr);
        glCullFace(GL_BACK);
        
        glBindVertexArray(0);
    }
    
    m_selectionShader->release();
}

void SelectionRenderer::renderFaceSelection(const dc::Camera& camera,
                                            uint32_t meshId,
                                            const std::vector<uint32_t>& faceIndices)
{
    if (!m_selectionShader) return;
    
    const SelectionMeshInfo* info = findMesh(meshId);
    if (!info || !info->vao || !info->mesh) return;
    
    m_selectionShader->bind();
    
    QMatrix4x4 view = camera.viewMatrix();
    QMatrix4x4 proj = camera.projectionMatrix();
    
    QMatrix4x4 model;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            model(j, i) = info->transform[i][j];
        }
    }
    
    m_selectionShader->setUniformValue("model", model);
    m_selectionShader->setUniformValue("view", view);
    m_selectionShader->setUniformValue("projection", proj);
    m_selectionShader->setUniformValue("normalMatrix", model.normalMatrix());
    m_selectionShader->setUniformValue("selectionMode", 1);
    m_selectionShader->setUniformValue("highlightPass", 0);
    m_selectionShader->setUniformValue("outlineScale", 0.0f);
    m_selectionShader->setUniformValue("opacity", m_config.opacity);
    m_selectionShader->setUniformValue("xrayMode", m_config.xrayMode);
    m_selectionShader->setUniformValue("highlightColor",
        QVector4D(m_config.faceColor.r, m_config.faceColor.g,
                  m_config.faceColor.b, m_config.faceColor.a));
    m_selectionShader->setUniformValue("cameraPosition", camera.position());
    
    glBindVertexArray(info->vao);
    
    // Draw selected faces one at a time (inefficient but simple)
    // TODO: For better performance, batch into a single draw call
    for (uint32_t faceIdx : faceIndices) {
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT,
                      reinterpret_cast<void*>(faceIdx * 3 * sizeof(uint32_t)));
    }
    
    glBindVertexArray(0);
    m_selectionShader->release();
}

void SelectionRenderer::renderVertexSelection(const dc::Camera& camera,
                                              uint32_t meshId,
                                              const std::vector<uint32_t>& vertexIndices)
{
    if (!m_pointShader) return;
    
    const SelectionMeshInfo* info = findMesh(meshId);
    if (!info || !info->mesh) return;
    
    const auto& vertices = info->mesh->vertices();
    
    // Collect vertex positions
    std::vector<glm::vec3> points;
    points.reserve(vertexIndices.size());
    
    for (uint32_t vIdx : vertexIndices) {
        if (vIdx < vertices.size()) {
            glm::vec4 worldPos = info->transform * glm::vec4(vertices[vIdx], 1.0f);
            points.push_back(glm::vec3(worldPos));
        }
    }
    
    if (points.empty()) return;
    
    // Upload points
    glBindVertexArray(m_pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3),
                 points.data(), GL_DYNAMIC_DRAW);
    
    // Draw
    m_pointShader->bind();
    
    QMatrix4x4 vp = camera.projectionMatrix() * camera.viewMatrix();
    m_pointShader->setUniformValue("mvp", vp);
    m_pointShader->setUniformValue("pointSize", m_config.vertexSize);
    m_pointShader->setUniformValue("color",
        QVector4D(m_config.vertexColor.r, m_config.vertexColor.g,
                  m_config.vertexColor.b, m_config.vertexColor.a));
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
    glDisable(GL_PROGRAM_POINT_SIZE);
    
    m_pointShader->release();
    glBindVertexArray(0);
}

void SelectionRenderer::renderEdgeSelection(const dc::Camera& camera,
                                            uint32_t meshId,
                                            const std::vector<uint32_t>& edgeIndices)
{
    if (!m_lineShader) return;
    
    const SelectionMeshInfo* info = findMesh(meshId);
    if (!info || !info->mesh) return;
    
    const auto& vertices = info->mesh->vertices();
    
    // Collect edge endpoints
    std::vector<glm::vec3> linePoints;
    linePoints.reserve(edgeIndices.size() * 2);
    
    for (uint32_t edgeIdx : edgeIndices) {
        uint32_t v1 = edgeIdx & 0xFFFF;
        uint32_t v2 = (edgeIdx >> 16) & 0xFFFF;
        
        if (v1 < vertices.size() && v2 < vertices.size()) {
            glm::vec4 p1 = info->transform * glm::vec4(vertices[v1], 1.0f);
            glm::vec4 p2 = info->transform * glm::vec4(vertices[v2], 1.0f);
            linePoints.push_back(glm::vec3(p1));
            linePoints.push_back(glm::vec3(p2));
        }
    }
    
    if (linePoints.empty()) return;
    
    // Upload line vertices
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, linePoints.size() * sizeof(glm::vec3),
                 linePoints.data(), GL_DYNAMIC_DRAW);
    
    // Draw
    m_lineShader->bind();
    
    QMatrix4x4 vp = camera.projectionMatrix() * camera.viewMatrix();
    m_lineShader->setUniformValue("mvp", vp);
    m_lineShader->setUniformValue("color",
        QVector4D(m_config.edgeColor.r, m_config.edgeColor.g,
                  m_config.edgeColor.b, m_config.edgeColor.a));
    
    glLineWidth(m_config.edgeWidth);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(linePoints.size()));
    glLineWidth(1.0f);
    
    m_lineShader->release();
    glBindVertexArray(0);
}

void SelectionRenderer::renderBoxSelection(const QPoint& startPos,
                                           const QPoint& endPos,
                                           const QSize& viewportSize)
{
    if (!m_boxShader) return;
    
    // Calculate normalized coordinates
    float x1 = (2.0f * startPos.x()) / viewportSize.width() - 1.0f;
    float y1 = 1.0f - (2.0f * startPos.y()) / viewportSize.height();
    float x2 = (2.0f * endPos.x()) / viewportSize.width() - 1.0f;
    float y2 = 1.0f - (2.0f * endPos.y()) / viewportSize.height();
    
    // Ensure proper ordering
    float minX = std::min(x1, x2);
    float maxX = std::max(x1, x2);
    float minY = std::min(y1, y2);
    float maxY = std::max(y1, y2);
    
    // Create quad vertices
    float vertices[] = {
        minX, minY,
        maxX, minY,
        maxX, maxY,
        minX, maxY
    };
    
    // Upload
    glBindVertexArray(m_boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_boxVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    // Draw
    glDisable(GL_DEPTH_TEST);
    
    m_boxShader->bind();
    m_boxShader->setUniformValue("fillColor",
        QVector4D(m_config.boxSelectColor.r, m_config.boxSelectColor.g,
                  m_config.boxSelectColor.b, m_config.boxSelectColor.a));
    m_boxShader->setUniformValue("borderColor",
        QVector4D(m_config.boxSelectColor.r, m_config.boxSelectColor.g,
                  m_config.boxSelectColor.b, 1.0f));
    
    // Convert to screen coords for border detection
    float screenMinX = std::min(static_cast<float>(startPos.x()), static_cast<float>(endPos.x()));
    float screenMaxX = std::max(static_cast<float>(startPos.x()), static_cast<float>(endPos.x()));
    float screenMinY = viewportSize.height() - std::max(static_cast<float>(startPos.y()), static_cast<float>(endPos.y()));
    float screenMaxY = viewportSize.height() - std::min(static_cast<float>(startPos.y()), static_cast<float>(endPos.y()));
    
    m_boxShader->setUniformValue("rectMin", QVector2D(screenMinX, screenMinY));
    m_boxShader->setUniformValue("rectMax", QVector2D(screenMaxX, screenMaxY));
    m_boxShader->setUniformValue("borderWidth", 2.0f);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    m_boxShader->release();
    glBindVertexArray(0);
    
    glEnable(GL_DEPTH_TEST);
}

} // namespace renderer
} // namespace dc3d
