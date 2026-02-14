/**
 * @file SnapIndicatorRenderer.cpp
 * @brief Implementation of SnapIndicatorRenderer
 */

#include "SnapIndicatorRenderer.h"
#include "ShaderProgram.h"
#include "Camera.h"
#include "core/SnapManager.h"

#include <cmath>
#include <vector>

namespace dc {

// Simple shader for 2D indicator rendering
static const char* indicatorVertexShader = R"(
#version 410 core

layout(location = 0) in vec2 position;

uniform vec2 screenPos;
uniform float size;
uniform vec2 viewportSize;

void main() {
    // Convert from pixel coords to NDC
    vec2 pos = screenPos + position * size;
    vec2 ndc = (pos / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // Flip Y
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

static const char* indicatorFragmentShader = R"(
#version 410 core

uniform vec4 color;

out vec4 fragColor;

void main() {
    fragColor = color;
}
)";

SnapIndicatorRenderer::SnapIndicatorRenderer()
{
}

SnapIndicatorRenderer::~SnapIndicatorRenderer()
{
}

bool SnapIndicatorRenderer::initialize()
{
    if (m_initialized) return true;
    
    initializeOpenGLFunctions();
    
    // Create shader
    m_shader = std::make_unique<ShaderProgram>();
    if (!m_shader->loadFromSource(indicatorVertexShader, indicatorFragmentShader)) {
        return false;
    }
    
    createGeometry();
    
    m_initialized = true;
    return true;
}

void SnapIndicatorRenderer::cleanup()
{
    if (!m_initialized) return;
    
    m_vao.destroy();
    m_vbo.destroy();
    m_shader.reset();
    
    m_initialized = false;
}

void SnapIndicatorRenderer::createGeometry()
{
    std::vector<float> vertices;
    
    // Circle (for vertex snap) - unit circle approximation
    m_circleOffset = 0;
    const int circleSegments = 16;
    for (int i = 0; i <= circleSegments; ++i) {
        float angle = 2.0f * 3.14159f * float(i) / float(circleSegments);
        vertices.push_back(std::cos(angle));
        vertices.push_back(std::sin(angle));
    }
    m_circleCount = circleSegments + 1;
    
    // Triangle (for edge midpoint snap) - pointing up
    m_triangleOffset = static_cast<int>(vertices.size() / 2);
    vertices.push_back(0.0f);  vertices.push_back(-1.0f);   // Top
    vertices.push_back(-0.866f); vertices.push_back(0.5f);  // Bottom left
    vertices.push_back(0.866f);  vertices.push_back(0.5f);  // Bottom right
    vertices.push_back(0.0f);  vertices.push_back(-1.0f);   // Close
    m_triangleCount = 4;
    
    // Square (for face center snap)
    m_squareOffset = static_cast<int>(vertices.size() / 2);
    vertices.push_back(-0.7f); vertices.push_back(-0.7f);
    vertices.push_back(0.7f);  vertices.push_back(-0.7f);
    vertices.push_back(0.7f);  vertices.push_back(0.7f);
    vertices.push_back(-0.7f); vertices.push_back(0.7f);
    vertices.push_back(-0.7f); vertices.push_back(-0.7f);
    m_squareCount = 5;
    
    // Cross (for grid snap)
    m_crossOffset = static_cast<int>(vertices.size() / 2);
    // Horizontal line
    vertices.push_back(-1.0f); vertices.push_back(0.0f);
    vertices.push_back(1.0f);  vertices.push_back(0.0f);
    // Vertical line
    vertices.push_back(0.0f);  vertices.push_back(-1.0f);
    vertices.push_back(0.0f);  vertices.push_back(1.0f);
    m_crossCount = 4;
    
    // Diamond (for origin snap)
    m_diamondOffset = static_cast<int>(vertices.size() / 2);
    vertices.push_back(0.0f);  vertices.push_back(-1.0f);
    vertices.push_back(0.7f);  vertices.push_back(0.0f);
    vertices.push_back(0.0f);  vertices.push_back(1.0f);
    vertices.push_back(-0.7f); vertices.push_back(0.0f);
    vertices.push_back(0.0f);  vertices.push_back(-1.0f);
    m_diamondCount = 5;
    
    // Upload to GPU
    m_vao.create();
    m_vao.bind();
    
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    
    m_vao.release();
    m_vbo.release();
}

void SnapIndicatorRenderer::render(const dc3d::core::SnapResult& snapResult,
                                    const Camera& camera,
                                    const QSize& viewportSize)
{
    if (!m_initialized || !snapResult.snapped) return;
    
    QVector3D worldPos(snapResult.position.x, 
                       snapResult.position.y, 
                       snapResult.position.z);
    
    renderIndicator(snapResult.type, worldPos, camera, viewportSize);
}

void SnapIndicatorRenderer::renderIndicator(dc3d::core::SnapType type,
                                             const QVector3D& position,
                                             const Camera& camera,
                                             const QSize& viewportSize)
{
    using dc3d::core::SnapType;
    
    // Project world position to screen
    QMatrix4x4 mvp = camera.viewProjectionMatrix();
    QVector4D clipPos = mvp * QVector4D(position, 1.0f);
    
    if (clipPos.w() <= 0.0f) return;  // Behind camera
    
    QVector3D ndc = clipPos.toVector3D() / clipPos.w();
    QVector2D screenPos(
        (ndc.x() * 0.5f + 0.5f) * viewportSize.width(),
        (1.0f - (ndc.y() * 0.5f + 0.5f)) * viewportSize.height()
    );
    
    // Get color and shape
    QColor color = colorForSnapType(type);
    int offset, count;
    GLenum mode;
    
    switch (type) {
        case SnapType::Vertex:
            offset = m_circleOffset;
            count = m_circleCount;
            mode = GL_LINE_LOOP;
            break;
        case SnapType::Edge:
        case SnapType::EdgeMid:
            offset = m_triangleOffset;
            count = m_triangleCount;
            mode = GL_LINE_LOOP;
            break;
        case SnapType::Face:
        case SnapType::FaceCenter:
            offset = m_squareOffset;
            count = m_squareCount;
            mode = GL_LINE_LOOP;
            break;
        case SnapType::Grid:
            offset = m_crossOffset;
            count = m_crossCount;
            mode = GL_LINES;
            break;
        case SnapType::Origin:
            offset = m_diamondOffset;
            count = m_diamondCount;
            mode = GL_LINE_LOOP;
            break;
        default:
            return;
    }
    
    // Set up rendering state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    
    m_shader->bind();
    m_shader->setUniform("screenPos", screenPos);
    m_shader->setUniform("size", m_indicatorSize);
    m_shader->setUniform("viewportSize", QVector2D(viewportSize.width(), viewportSize.height()));
    m_shader->setUniform("color", QVector4D(color.redF(), color.greenF(), 
                                            color.blueF(), color.alphaF()));
    
    m_vao.bind();
    glDrawArrays(mode, offset, count);
    m_vao.release();
    
    m_shader->release();
    
    // Restore state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
}

QColor SnapIndicatorRenderer::colorForSnapType(dc3d::core::SnapType type) const
{
    using dc3d::core::SnapType;
    
    switch (type) {
        case SnapType::Vertex:
            return m_vertexColor;
        case SnapType::Edge:
        case SnapType::EdgeMid:
            return m_edgeColor;
        case SnapType::Face:
        case SnapType::FaceCenter:
            return m_faceColor;
        case SnapType::Grid:
            return m_gridColor;
        case SnapType::Origin:
            return m_originColor;
        default:
            return QColor(255, 255, 255);
    }
}

} // namespace dc
