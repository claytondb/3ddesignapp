/**
 * @file GridRenderer.cpp
 * @brief Implementation of ground grid and axes renderer
 */

#include "GridRenderer.h"
#include "ShaderProgram.h"
#include "Camera.h"
#include <QDebug>
#include <vector>
#include <cmath>

namespace dc {

// Grid shader source (embedded for simplicity)
static const char* gridVertexShader = R"(
#version 410 core

layout(location = 0) in vec3 position;

uniform mat4 mvp;
uniform vec3 cameraPos;

out float vDistance;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    vDistance = length(position.xz - cameraPos.xz);
}
)";

static const char* gridFragmentShader = R"(
#version 410 core

in float vDistance;

uniform vec3 lineColor;
uniform float fadeDistance;
uniform float maxDistance;

out vec4 fragColor;

void main() {
    // Fade based on distance from camera
    float fadeFactor = 1.0 - smoothstep(fadeDistance * 0.5, maxDistance, vDistance);
    
    if (fadeFactor <= 0.0) {
        discard;
    }
    
    fragColor = vec4(lineColor, fadeFactor);
}
)";

// Line shader for axes
static const char* lineVertexShader = R"(
#version 410 core

layout(location = 0) in vec3 position;
layout(location = 3) in vec4 color;

uniform mat4 mvp;

out vec4 vColor;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    vColor = color;
}
)";

static const char* lineFragmentShader = R"(
#version 410 core

in vec4 vColor;

out vec4 fragColor;

void main() {
    fragColor = vColor;
}
)";

GridRenderer::GridRenderer()
{
}

GridRenderer::~GridRenderer()
{
    // Note: cleanup() should only be called explicitly when OpenGL context is current.
    // The owner must call cleanup() before destruction to avoid OpenGL calls
    // without a valid context, which causes undefined behavior or crashes.
}

bool GridRenderer::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    initializeOpenGLFunctions();
    
    if (!loadShaders()) {
        return false;
    }
    
    createGridGeometry();
    createAxisGeometry();
    
    m_initialized = true;
    return true;
}

void GridRenderer::cleanup()
{
    if (!m_initialized) {
        return;
    }
    
    m_gridVAO.destroy();
    m_gridVBO.destroy();
    m_axisVAO.destroy();
    m_axisVBO.destroy();
    
    m_gridShader.reset();
    m_lineShader.reset();
    
    m_initialized = false;
}

bool GridRenderer::loadShaders()
{
    m_gridShader = std::make_unique<ShaderProgram>();
    if (!m_gridShader->loadFromSource(gridVertexShader, gridFragmentShader)) {
        qWarning() << "Failed to load grid shader:" << m_gridShader->errorLog();
        return false;
    }
    
    m_lineShader = std::make_unique<ShaderProgram>();
    if (!m_lineShader->loadFromSource(lineVertexShader, lineFragmentShader)) {
        qWarning() << "Failed to load line shader:" << m_lineShader->errorLog();
        return false;
    }
    
    return true;
}

void GridRenderer::createGridGeometry()
{
    std::vector<float> vertices;
    
    const float halfSize = m_settings.gridSize * 0.5f;
    
    // Calculate number of lines
    int majorCount = static_cast<int>(m_settings.gridSize / m_settings.majorSpacing) + 1;
    int minorCount = static_cast<int>(m_settings.gridSize / m_settings.minorSpacing) + 1;
    
    // Reserve space (each line = 2 vertices * 3 floats)
    vertices.reserve((majorCount * 4 + minorCount * 4) * 3);
    
    // Generate minor lines first (so major lines render on top)
    m_minorLineOffset = 0;
    for (int i = 0; i < minorCount; ++i) {
        float pos = -halfSize + i * m_settings.minorSpacing;
        
        // Skip if this position is a major line
        float majorMod = std::fmod(std::abs(pos), m_settings.majorSpacing);
        if (majorMod < 0.001f || std::abs(majorMod - m_settings.majorSpacing) < 0.001f) {
            continue;
        }
        
        // X-axis aligned line (parallel to X)
        vertices.push_back(-halfSize); vertices.push_back(0.0f); vertices.push_back(pos);
        vertices.push_back(halfSize);  vertices.push_back(0.0f); vertices.push_back(pos);
        
        // Z-axis aligned line (parallel to Z)
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(-halfSize);
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(halfSize);
    }
    m_minorLineCount = (vertices.size() / 3) / 2;
    
    // Generate major lines
    m_majorLineOffset = static_cast<int>(vertices.size() / 3);
    for (int i = 0; i < majorCount; ++i) {
        float pos = -halfSize + i * m_settings.majorSpacing;
        
        // X-axis aligned line
        vertices.push_back(-halfSize); vertices.push_back(0.0f); vertices.push_back(pos);
        vertices.push_back(halfSize);  vertices.push_back(0.0f); vertices.push_back(pos);
        
        // Z-axis aligned line
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(-halfSize);
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(halfSize);
    }
    m_majorLineCount = (vertices.size() / 3 - m_majorLineOffset) / 2;
    
    // Create VAO and VBO
    m_gridVAO.create();
    m_gridVAO.bind();
    
    m_gridVBO.create();
    m_gridVBO.bind();
    m_gridVBO.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    m_gridVAO.release();
    m_gridVBO.release();
}

void GridRenderer::createAxisGeometry()
{
    // Axis vertices: position (3) + color (4) = 7 floats per vertex
    // 3 axes * 2 vertices = 6 vertices
    std::vector<float> vertices;
    vertices.reserve(6 * 7);
    
    float len = m_settings.axisLength;
    
    // X axis (Red)
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f, 1.0f});
    vertices.insert(vertices.end(), {len,  0.0f, 0.0f,  1.0f, 0.2f, 0.2f, 1.0f});
    
    // Y axis (Green)
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f,  0.2f, 1.0f, 0.2f, 1.0f});
    vertices.insert(vertices.end(), {0.0f, len,  0.0f,  0.2f, 1.0f, 0.2f, 1.0f});
    
    // Z axis (Blue)
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f,  0.2f, 0.5f, 1.0f, 1.0f});
    vertices.insert(vertices.end(), {0.0f, 0.0f, len,   0.2f, 0.5f, 1.0f, 1.0f});
    
    m_axisVAO.create();
    m_axisVAO.bind();
    
    m_axisVBO.create();
    m_axisVBO.bind();
    m_axisVBO.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
    
    // Color attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 
                          reinterpret_cast<void*>(3 * sizeof(float)));
    
    m_axisVAO.release();
    m_axisVBO.release();
}

void GridRenderer::render(const Camera& camera)
{
    if (!m_initialized || !m_visible) {
        return;
    }
    
    // Enable blending for fade effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth writing but keep depth test for proper occlusion
    glDepthMask(GL_FALSE);
    
    QMatrix4x4 mvp = camera.viewProjectionMatrix();
    QVector3D cameraPos = camera.position();
    
    m_gridShader->bind();
    m_gridShader->setUniform("mvp", mvp);
    m_gridShader->setUniform("cameraPos", cameraPos);
    m_gridShader->setUniform("fadeDistance", m_settings.fadeDistance);
    m_gridShader->setUniform("maxDistance", m_settings.gridSize * 0.5f);
    
    m_gridVAO.bind();
    
    // Check supported line width range (glLineWidth > 1.0 may not be supported in core profile)
    GLfloat lineWidthRange[2];
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
    
    // Draw minor lines (clamp to supported range)
    float minorWidth = std::min(m_settings.minorLineWidth, lineWidthRange[1]);
    glLineWidth(minorWidth);
    m_gridShader->setUniform("lineColor", m_settings.minorColor);
    glDrawArrays(GL_LINES, m_minorLineOffset, m_minorLineCount * 2);
    
    // Draw major lines (clamp to supported range)
    float majorWidth = std::min(m_settings.majorLineWidth, lineWidthRange[1]);
    glLineWidth(majorWidth);
    m_gridShader->setUniform("lineColor", m_settings.majorColor);
    glDrawArrays(GL_LINES, m_majorLineOffset, m_majorLineCount * 2);
    
    m_gridVAO.release();
    m_gridShader->release();
    
    // Restore state
    glDepthMask(GL_TRUE);
    
    // Draw axes on top
    if (m_settings.showAxes) {
        renderAxes(camera);
    }
    
    // Restore blending state - must be done regardless of showAxes
    glDisable(GL_BLEND);
    glLineWidth(1.0f);  // Restore default line width
}

void GridRenderer::renderAxes(const Camera& camera)
{
    if (!m_initialized) {
        return;
    }
    
    QMatrix4x4 mvp = camera.viewProjectionMatrix();
    
    m_lineShader->bind();
    m_lineShader->setUniform("mvp", mvp);
    
    glLineWidth(m_settings.axisLineWidth);
    
    m_axisVAO.bind();
    glDrawArrays(GL_LINES, 0, 6);
    m_axisVAO.release();
    
    m_lineShader->release();
    
    // Reset line width
    glLineWidth(1.0f);
}

} // namespace dc
