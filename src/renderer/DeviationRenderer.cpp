/**
 * @file DeviationRenderer.cpp
 * @brief Implementation of deviation colormap rendering
 */

#include "DeviationRenderer.h"
#include "../geometry/MeshData.h"
#include <QOpenGLShader>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

namespace dc {

// Vertex shader for deviation-colored mesh
static const char* deviationVertexShader = R"(
#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in float deviation;

uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform float u_MinVal;
uniform float u_MaxVal;

out vec3 v_Normal;
out float v_Deviation;
out float v_NormalizedDev;

void main() {
    gl_Position = u_MVP * vec4(position, 1.0);
    v_Normal = mat3(u_Model) * normal;
    v_Deviation = deviation;
    
    // Normalize deviation to [0, 1] range
    float range = u_MaxVal - u_MinVal;
    if (range > 0.0) {
        v_NormalizedDev = clamp((deviation - u_MinVal) / range, 0.0, 1.0);
    } else {
        v_NormalizedDev = 0.5;
    }
}
)";

// Fragment shader for deviation-colored mesh
static const char* deviationFragmentShader = R"(
#version 410 core

in vec3 v_Normal;
in float v_Deviation;
in float v_NormalizedDev;

uniform int u_ColormapType;

out vec4 fragColor;

// Blue-Green-Red colormap
vec3 blueGreenRed(float t) {
    if (t < 0.5) {
        // Blue to Green
        float s = t * 2.0;
        return vec3(0.0, s, 1.0 - s);
    } else {
        // Green to Red
        float s = (t - 0.5) * 2.0;
        return vec3(s, 1.0 - s, 0.0);
    }
}

// Rainbow colormap
vec3 rainbow(float t) {
    float h = (1.0 - t) * 0.8;  // Hue from red to purple
    float s = 1.0;
    float v = 1.0;
    
    // HSV to RGB
    float c = v * s;
    float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
    float m = v - c;
    
    vec3 rgb;
    float h6 = h * 6.0;
    if (h6 < 1.0) rgb = vec3(c, x, 0.0);
    else if (h6 < 2.0) rgb = vec3(x, c, 0.0);
    else if (h6 < 3.0) rgb = vec3(0.0, c, x);
    else if (h6 < 4.0) rgb = vec3(0.0, x, c);
    else if (h6 < 5.0) rgb = vec3(x, 0.0, c);
    else rgb = vec3(c, 0.0, x);
    
    return rgb + m;
}

// Cool-Warm diverging colormap
vec3 coolWarm(float t) {
    // Blue at 0, white at 0.5, red at 1
    vec3 cool = vec3(0.231, 0.298, 0.753);  // Blue
    vec3 warm = vec3(0.706, 0.016, 0.150);  // Red
    vec3 white = vec3(0.867, 0.867, 0.867);
    
    if (t < 0.5) {
        return mix(cool, white, t * 2.0);
    } else {
        return mix(white, warm, (t - 0.5) * 2.0);
    }
}

// Viridis colormap (perceptually uniform)
vec3 viridis(float t) {
    const vec3 c0 = vec3(0.267, 0.004, 0.329);
    const vec3 c1 = vec3(0.282, 0.140, 0.458);
    const vec3 c2 = vec3(0.254, 0.265, 0.530);
    const vec3 c3 = vec3(0.163, 0.471, 0.558);
    const vec3 c4 = vec3(0.134, 0.659, 0.518);
    const vec3 c5 = vec3(0.477, 0.821, 0.318);
    const vec3 c6 = vec3(0.993, 0.906, 0.144);
    
    if (t < 0.167) return mix(c0, c1, t / 0.167);
    else if (t < 0.333) return mix(c1, c2, (t - 0.167) / 0.167);
    else if (t < 0.5) return mix(c2, c3, (t - 0.333) / 0.167);
    else if (t < 0.667) return mix(c3, c4, (t - 0.5) / 0.167);
    else if (t < 0.833) return mix(c4, c5, (t - 0.667) / 0.167);
    else return mix(c5, c6, (t - 0.833) / 0.167);
}

// Magma colormap
vec3 magma(float t) {
    const vec3 c0 = vec3(0.001, 0.000, 0.014);
    const vec3 c1 = vec3(0.282, 0.141, 0.459);
    const vec3 c2 = vec3(0.716, 0.215, 0.475);
    const vec3 c3 = vec3(0.994, 0.624, 0.427);
    const vec3 c4 = vec3(0.987, 0.991, 0.749);
    
    if (t < 0.25) return mix(c0, c1, t / 0.25);
    else if (t < 0.5) return mix(c1, c2, (t - 0.25) / 0.25);
    else if (t < 0.75) return mix(c2, c3, (t - 0.5) / 0.25);
    else return mix(c3, c4, (t - 0.75) / 0.25);
}

vec3 grayscale(float t) {
    return vec3(t);
}

vec3 getColormapColor(float t) {
    switch (u_ColormapType) {
        case 0: return blueGreenRed(t);
        case 1: return rainbow(t);
        case 2: return coolWarm(t);
        case 3: return viridis(t);
        case 4: return magma(t);
        case 5: return grayscale(t);
        default: return blueGreenRed(t);
    }
}

void main() {
    vec3 color = getColormapColor(v_NormalizedDev);
    
    // Simple diffuse shading
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal = normalize(v_Normal);
    float diffuse = max(dot(normal, lightDir), 0.0) * 0.6 + 0.4;
    
    fragColor = vec4(color * diffuse, 1.0);
}
)";

// Vertex shader for legend quad
static const char* legendVertexShader = R"(
#version 410 core

layout(location = 0) in vec2 position;
layout(location = 1) in float texCoord;

out float v_TexCoord;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    v_TexCoord = texCoord;
}
)";

// Fragment shader for legend
static const char* legendFragmentShader = R"(
#version 410 core

in float v_TexCoord;

uniform int u_ColormapType;

out vec4 fragColor;

// Same colormap functions as mesh shader
vec3 blueGreenRed(float t) {
    if (t < 0.5) {
        float s = t * 2.0;
        return vec3(0.0, s, 1.0 - s);
    } else {
        float s = (t - 0.5) * 2.0;
        return vec3(s, 1.0 - s, 0.0);
    }
}

vec3 coolWarm(float t) {
    vec3 cool = vec3(0.231, 0.298, 0.753);
    vec3 warm = vec3(0.706, 0.016, 0.150);
    vec3 white = vec3(0.867, 0.867, 0.867);
    
    if (t < 0.5) {
        return mix(cool, white, t * 2.0);
    } else {
        return mix(white, warm, (t - 0.5) * 2.0);
    }
}

vec3 viridis(float t) {
    const vec3 c0 = vec3(0.267, 0.004, 0.329);
    const vec3 c1 = vec3(0.282, 0.140, 0.458);
    const vec3 c2 = vec3(0.254, 0.265, 0.530);
    const vec3 c3 = vec3(0.163, 0.471, 0.558);
    const vec3 c4 = vec3(0.134, 0.659, 0.518);
    const vec3 c5 = vec3(0.477, 0.821, 0.318);
    const vec3 c6 = vec3(0.993, 0.906, 0.144);
    
    if (t < 0.167) return mix(c0, c1, t / 0.167);
    else if (t < 0.333) return mix(c1, c2, (t - 0.167) / 0.167);
    else if (t < 0.5) return mix(c2, c3, (t - 0.333) / 0.167);
    else if (t < 0.667) return mix(c3, c4, (t - 0.5) / 0.167);
    else if (t < 0.833) return mix(c4, c5, (t - 0.667) / 0.167);
    else return mix(c5, c6, (t - 0.833) / 0.167);
}

vec3 magma(float t) {
    const vec3 c0 = vec3(0.001, 0.000, 0.014);
    const vec3 c1 = vec3(0.282, 0.141, 0.459);
    const vec3 c2 = vec3(0.716, 0.215, 0.475);
    const vec3 c3 = vec3(0.994, 0.624, 0.427);
    const vec3 c4 = vec3(0.987, 0.991, 0.749);
    
    if (t < 0.25) return mix(c0, c1, t / 0.25);
    else if (t < 0.5) return mix(c1, c2, (t - 0.25) / 0.25);
    else if (t < 0.75) return mix(c2, c3, (t - 0.5) / 0.25);
    else return mix(c3, c4, (t - 0.75) / 0.25);
}

vec3 rainbow(float t) {
    float h = (1.0 - t) * 0.8;
    float c = 1.0;
    float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
    
    vec3 rgb;
    float h6 = h * 6.0;
    if (h6 < 1.0) rgb = vec3(c, x, 0.0);
    else if (h6 < 2.0) rgb = vec3(x, c, 0.0);
    else if (h6 < 3.0) rgb = vec3(0.0, c, x);
    else if (h6 < 4.0) rgb = vec3(0.0, x, c);
    else if (h6 < 5.0) rgb = vec3(x, 0.0, c);
    else rgb = vec3(c, 0.0, x);
    
    return rgb;
}

vec3 grayscale(float t) {
    return vec3(t);
}

vec3 getColormapColor(float t) {
    switch (u_ColormapType) {
        case 0: return blueGreenRed(t);
        case 1: return rainbow(t);
        case 2: return coolWarm(t);
        case 3: return viridis(t);
        case 4: return magma(t);
        case 5: return grayscale(t);
        default: return blueGreenRed(t);
    }
}

void main() {
    fragColor = vec4(getColormapColor(v_TexCoord), 1.0);
}
)";

DeviationRenderer::DeviationRenderer() = default;

DeviationRenderer::~DeviationRenderer() {
    cleanup();
}

bool DeviationRenderer::initialize() {
    initializeOpenGLFunctions();
    
    setupShaders();
    setupLegendGeometry();
    
    initialized_ = true;
    return true;
}

void DeviationRenderer::cleanup() {
    meshShader_.reset();
    legendShader_.reset();
    
    legendVAO_.destroy();
    legendVBO_.destroy();
    
    meshData_.clear();
    
    initialized_ = false;
}

void DeviationRenderer::setupShaders() {
    // Mesh shader
    meshShader_ = std::make_unique<QOpenGLShaderProgram>();
    meshShader_->addShaderFromSourceCode(QOpenGLShader::Vertex, deviationVertexShader);
    meshShader_->addShaderFromSourceCode(QOpenGLShader::Fragment, deviationFragmentShader);
    meshShader_->link();
    
    mvpLoc_ = meshShader_->uniformLocation("u_MVP");
    modelLoc_ = meshShader_->uniformLocation("u_Model");
    minValLoc_ = meshShader_->uniformLocation("u_MinVal");
    maxValLoc_ = meshShader_->uniformLocation("u_MaxVal");
    colormapTypeLoc_ = meshShader_->uniformLocation("u_ColormapType");
    
    // Legend shader
    legendShader_ = std::make_unique<QOpenGLShaderProgram>();
    legendShader_->addShaderFromSourceCode(QOpenGLShader::Vertex, legendVertexShader);
    legendShader_->addShaderFromSourceCode(QOpenGLShader::Fragment, legendFragmentShader);
    legendShader_->link();
}

void DeviationRenderer::setupLegendGeometry() {
    // Legend is a vertical quad strip with varying texture coordinate
    // Will be positioned in render based on config
    
    legendVAO_.create();
    legendVBO_.create();
    
    legendVAO_.bind();
    legendVBO_.bind();
    
    // Vertices: position (x, y) + texcoord (t)
    // Create a vertical strip with 32 segments for smooth gradient
    const int segments = 32;
    std::vector<float> vertices;
    vertices.reserve(segments * 2 * 3);  // 2 vertices per segment, 3 floats each
    
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        // Left vertex
        vertices.push_back(-1.0f);  // x (will be transformed)
        vertices.push_back(t * 2.0f - 1.0f);  // y [-1, 1]
        vertices.push_back(t);  // texcoord
        // Right vertex
        vertices.push_back(1.0f);
        vertices.push_back(t * 2.0f - 1.0f);
        vertices.push_back(t);
    }
    
    legendVBO_.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 
                          reinterpret_cast<void*>(2 * sizeof(float)));
    
    legendVAO_.release();
    legendVBO_.release();
}

void DeviationRenderer::setDeviationData(
    uint64_t meshId,
    const dc3d::geometry::MeshData& mesh,
    const std::vector<float>& deviations,
    float minVal,
    float maxVal
) {
    if (mesh.isEmpty() || deviations.empty()) {
        return;
    }
    
    // Compute auto range if needed
    if (!std::isfinite(minVal) || !std::isfinite(maxVal) || config_.autoRange) {
        computeAutoRange(deviations);
    } else {
        dataMin_ = minVal;
        dataMax_ = maxVal;
        config_.minValue = minVal;
        config_.maxValue = maxVal;
    }
    
    uploadMeshData(meshId, mesh, deviations);
}

void DeviationRenderer::uploadMeshData(
    uint64_t meshId,
    const dc3d::geometry::MeshData& mesh,
    const std::vector<float>& deviations
) {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    const auto& normals = mesh.normals();
    
    bool hasNormals = mesh.hasNormals();
    
    // Create GPU data structure
    auto gpuData = std::make_unique<DeviationMeshGPU>();
    
    gpuData->vao.create();
    gpuData->vbo.create();
    gpuData->ebo.create();
    
    gpuData->vao.bind();
    
    // Interleaved vertex data: position (3) + normal (3) + deviation (1)
    std::vector<float> vertexData;
    vertexData.reserve(vertices.size() * 7);
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        // Position
        vertexData.push_back(vertices[i].x);
        vertexData.push_back(vertices[i].y);
        vertexData.push_back(vertices[i].z);
        
        // Normal
        if (hasNormals) {
            vertexData.push_back(normals[i].x);
            vertexData.push_back(normals[i].y);
            vertexData.push_back(normals[i].z);
        } else {
            vertexData.push_back(0.0f);
            vertexData.push_back(1.0f);
            vertexData.push_back(0.0f);
        }
        
        // Deviation
        float dev = (i < deviations.size()) ? deviations[i] : 0.0f;
        vertexData.push_back(dev);
    }
    
    gpuData->vbo.bind();
    gpuData->vbo.allocate(vertexData.data(), 
                          static_cast<int>(vertexData.size() * sizeof(float)));
    
    // Setup vertex attributes
    const int stride = 7 * sizeof(float);
    
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 
                          reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Deviation
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, 
                          reinterpret_cast<void*>(6 * sizeof(float)));
    
    // Index buffer
    gpuData->ebo.bind();
    gpuData->ebo.allocate(indices.data(), 
                          static_cast<int>(indices.size() * sizeof(uint32_t)));
    
    gpuData->indexCount = static_cast<uint32_t>(indices.size());
    gpuData->vertexCount = static_cast<uint32_t>(vertices.size());
    gpuData->valid = true;
    
    gpuData->vao.release();
    
    meshData_[meshId] = std::move(gpuData);
}

void DeviationRenderer::updateDeviationValues(
    uint64_t meshId,
    const std::vector<float>& deviations
) {
    auto it = meshData_.find(meshId);
    if (it == meshData_.end() || !it->second->valid) {
        return;
    }
    
    // Update deviation values in VBO
    // This requires re-uploading vertex data with new deviations
    // For efficiency, could use buffer sub-data if only deviation changes
    
    // Recompute auto range
    if (config_.autoRange) {
        computeAutoRange(deviations);
    }
}

void DeviationRenderer::removeDeviationData(uint64_t meshId) {
    meshData_.erase(meshId);
}

void DeviationRenderer::clearAll() {
    meshData_.clear();
}

void DeviationRenderer::render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    if (!initialized_ || meshData_.empty()) {
        return;
    }
    
    meshShader_->bind();
    
    glm::mat4 mvp = projMatrix * viewMatrix;
    glm::mat4 model = glm::mat4(1.0f);
    
    meshShader_->setUniformValue(mvpLoc_, QMatrix4x4(glm::value_ptr(mvp)).transposed());
    meshShader_->setUniformValue(modelLoc_, QMatrix4x4(glm::value_ptr(model)).transposed());
    meshShader_->setUniformValue(minValLoc_, config_.minValue);
    meshShader_->setUniformValue(maxValLoc_, config_.maxValue);
    meshShader_->setUniformValue(colormapTypeLoc_, static_cast<int>(config_.colormap));
    
    glEnable(GL_DEPTH_TEST);
    
    for (const auto& [id, gpuData] : meshData_) {
        if (!gpuData->valid) continue;
        
        gpuData->vao.bind();
        glDrawElements(GL_TRIANGLES, gpuData->indexCount, GL_UNSIGNED_INT, nullptr);
        gpuData->vao.release();
    }
    
    meshShader_->release();
}

void DeviationRenderer::renderLegend(int viewportWidth, int viewportHeight) {
    if (!initialized_ || config_.legendPosition == LegendPosition::None) {
        return;
    }
    
    // Calculate legend position in NDC
    float legendWidth = config_.legendWidth / viewportWidth * 2.0f;
    float legendHeight = config_.legendHeight / viewportHeight * 2.0f;
    float margin = config_.legendMargin / std::min(viewportWidth, viewportHeight) * 2.0f;
    
    float x, y;
    switch (config_.legendPosition) {
        case LegendPosition::TopLeft:
            x = -1.0f + margin;
            y = 1.0f - margin - legendHeight;
            break;
        case LegendPosition::TopRight:
            x = 1.0f - margin - legendWidth;
            y = 1.0f - margin - legendHeight;
            break;
        case LegendPosition::BottomLeft:
            x = -1.0f + margin;
            y = -1.0f + margin;
            break;
        case LegendPosition::BottomRight:
        default:
            x = 1.0f - margin - legendWidth;
            y = -1.0f + margin;
            break;
    }
    
    // Transform and render legend
    legendShader_->bind();
    legendShader_->setUniformValue("u_ColormapType", static_cast<int>(config_.colormap));
    
    // Set up transformation for legend position
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Scale and translate the legend geometry
    glViewport(
        static_cast<int>((x + 1.0f) / 2.0f * viewportWidth),
        static_cast<int>((y + 1.0f) / 2.0f * viewportHeight),
        static_cast<int>(config_.legendWidth),
        static_cast<int>(config_.legendHeight)
    );
    
    legendVAO_.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 66);  // 33 segments * 2 vertices
    legendVAO_.release();
    
    // Restore viewport
    glViewport(0, 0, viewportWidth, viewportHeight);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    
    legendShader_->release();
}

void DeviationRenderer::setConfig(const DeviationRenderConfig& config) {
    config_ = config;
}

void DeviationRenderer::setRange(float minVal, float maxVal) {
    config_.autoRange = false;
    config_.minValue = minVal;
    config_.maxValue = maxVal;
}

void DeviationRenderer::setAutoRange(bool enabled) {
    config_.autoRange = enabled;
}

void DeviationRenderer::setColormap(DeviationColormap colormap) {
    config_.colormap = colormap;
}

void DeviationRenderer::getRange(float& minVal, float& maxVal) const {
    minVal = config_.minValue;
    maxVal = config_.maxValue;
}

QColor DeviationRenderer::valueToColor(float value) const {
    float range = config_.maxValue - config_.minValue;
    float t = 0.5f;
    if (range > 0) {
        t = std::clamp((value - config_.minValue) / range, 0.0f, 1.0f);
    }
    return colormapSample(t);
}

QColor DeviationRenderer::colormapSample(float t) const {
    glm::vec3 color = sampleColormap(t);
    return QColor::fromRgbF(color.r, color.g, color.b);
}

glm::vec3 DeviationRenderer::sampleColormap(float t) const {
    switch (config_.colormap) {
        case DeviationColormap::BlueGreenRed:
            return sampleBlueGreenRed(t);
        case DeviationColormap::Rainbow:
            return sampleRainbow(t);
        case DeviationColormap::CoolWarm:
            return sampleCoolWarm(t);
        case DeviationColormap::Viridis:
            return sampleViridis(t);
        case DeviationColormap::Magma:
            return sampleMagma(t);
        case DeviationColormap::Grayscale:
            return sampleGrayscale(t);
        default:
            return sampleBlueGreenRed(t);
    }
}

glm::vec3 DeviationRenderer::sampleBlueGreenRed(float t) const {
    if (t < 0.5f) {
        float s = t * 2.0f;
        return glm::vec3(0.0f, s, 1.0f - s);
    } else {
        float s = (t - 0.5f) * 2.0f;
        return glm::vec3(s, 1.0f - s, 0.0f);
    }
}

glm::vec3 DeviationRenderer::sampleRainbow(float t) const {
    float h = (1.0f - t) * 0.8f;
    float c = 1.0f;
    float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    
    glm::vec3 rgb;
    float h6 = h * 6.0f;
    if (h6 < 1.0f) rgb = glm::vec3(c, x, 0.0f);
    else if (h6 < 2.0f) rgb = glm::vec3(x, c, 0.0f);
    else if (h6 < 3.0f) rgb = glm::vec3(0.0f, c, x);
    else if (h6 < 4.0f) rgb = glm::vec3(0.0f, x, c);
    else if (h6 < 5.0f) rgb = glm::vec3(x, 0.0f, c);
    else rgb = glm::vec3(c, 0.0f, x);
    
    return rgb;
}

glm::vec3 DeviationRenderer::sampleCoolWarm(float t) const {
    glm::vec3 cool(0.231f, 0.298f, 0.753f);
    glm::vec3 warm(0.706f, 0.016f, 0.150f);
    glm::vec3 white(0.867f, 0.867f, 0.867f);
    
    if (t < 0.5f) {
        return glm::mix(cool, white, t * 2.0f);
    } else {
        return glm::mix(white, warm, (t - 0.5f) * 2.0f);
    }
}

glm::vec3 DeviationRenderer::sampleViridis(float t) const {
    const glm::vec3 c0(0.267f, 0.004f, 0.329f);
    const glm::vec3 c1(0.282f, 0.140f, 0.458f);
    const glm::vec3 c2(0.254f, 0.265f, 0.530f);
    const glm::vec3 c3(0.163f, 0.471f, 0.558f);
    const glm::vec3 c4(0.134f, 0.659f, 0.518f);
    const glm::vec3 c5(0.477f, 0.821f, 0.318f);
    const glm::vec3 c6(0.993f, 0.906f, 0.144f);
    
    if (t < 0.167f) return glm::mix(c0, c1, t / 0.167f);
    else if (t < 0.333f) return glm::mix(c1, c2, (t - 0.167f) / 0.167f);
    else if (t < 0.5f) return glm::mix(c2, c3, (t - 0.333f) / 0.167f);
    else if (t < 0.667f) return glm::mix(c3, c4, (t - 0.5f) / 0.167f);
    else if (t < 0.833f) return glm::mix(c4, c5, (t - 0.667f) / 0.167f);
    else return glm::mix(c5, c6, (t - 0.833f) / 0.167f);
}

glm::vec3 DeviationRenderer::sampleMagma(float t) const {
    const glm::vec3 c0(0.001f, 0.000f, 0.014f);
    const glm::vec3 c1(0.282f, 0.141f, 0.459f);
    const glm::vec3 c2(0.716f, 0.215f, 0.475f);
    const glm::vec3 c3(0.994f, 0.624f, 0.427f);
    const glm::vec3 c4(0.987f, 0.991f, 0.749f);
    
    if (t < 0.25f) return glm::mix(c0, c1, t / 0.25f);
    else if (t < 0.5f) return glm::mix(c1, c2, (t - 0.25f) / 0.25f);
    else if (t < 0.75f) return glm::mix(c2, c3, (t - 0.5f) / 0.25f);
    else return glm::mix(c3, c4, (t - 0.75f) / 0.25f);
}

glm::vec3 DeviationRenderer::sampleGrayscale(float t) const {
    return glm::vec3(t);
}

void DeviationRenderer::computeAutoRange(const std::vector<float>& deviations) {
    if (deviations.empty()) {
        dataMin_ = 0.0f;
        dataMax_ = 1.0f;
        return;
    }
    
    auto [minIt, maxIt] = std::minmax_element(deviations.begin(), deviations.end());
    dataMin_ = *minIt;
    dataMax_ = *maxIt;
    
    // Symmetric range for signed values
    float absMax = std::max(std::abs(dataMin_), std::abs(dataMax_));
    config_.minValue = -absMax;
    config_.maxValue = absMax;
}

} // namespace dc
