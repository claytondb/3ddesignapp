/**
 * @file PrimitiveRenderer.cpp
 * @brief Implementation of primitive rendering
 */

#include "PrimitiveRenderer.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <glm/gtc/constants.hpp>

// Note: OpenGL headers would normally be included here
// For now, we provide the implementation structure
// #include <GL/glew.h>

namespace dc3d {
namespace renderer {

PrimitiveRenderer::PrimitiveRenderer() = default;

PrimitiveRenderer::~PrimitiveRenderer() {
    cleanup();
}

PrimitiveRenderer::PrimitiveRenderer(PrimitiveRenderer&& other) noexcept
    : m_options(std::move(other.m_options))
    , m_mesh(std::move(other.m_mesh))
    , m_hasPrimitive(other.m_hasPrimitive)
    , m_type(other.m_type)
    , m_primitive(std::move(other.m_primitive))
    , m_referencePoints(std::move(other.m_referencePoints))
    , m_vao(other.m_vao)
    , m_vbo(other.m_vbo)
    , m_ebo(other.m_ebo)
    , m_wireframeEbo(other.m_wireframeEbo)
    , m_shaderProgram(other.m_shaderProgram)
    , m_initialized(other.m_initialized)
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_wireframeEbo = 0;
    other.m_shaderProgram = 0;
    other.m_initialized = false;
}

PrimitiveRenderer& PrimitiveRenderer::operator=(PrimitiveRenderer&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_options = std::move(other.m_options);
        m_mesh = std::move(other.m_mesh);
        m_hasPrimitive = other.m_hasPrimitive;
        m_type = other.m_type;
        m_primitive = std::move(other.m_primitive);
        m_referencePoints = std::move(other.m_referencePoints);
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_wireframeEbo = other.m_wireframeEbo;
        m_shaderProgram = other.m_shaderProgram;
        m_initialized = other.m_initialized;
        
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_wireframeEbo = 0;
        other.m_shaderProgram = 0;
        other.m_initialized = false;
    }
    return *this;
}

void PrimitiveRenderer::setOptions(const PrimitiveRenderOptions& options) {
    m_options = options;
    
    // Regenerate mesh if primitive exists
    if (m_hasPrimitive) {
        switch (m_type) {
            case geometry::PrimitiveType::Plane:
                generatePlaneMesh(std::get<geometry::Plane>(m_primitive));
                break;
            case geometry::PrimitiveType::Cylinder:
                generateCylinderMesh(std::get<geometry::Cylinder>(m_primitive));
                break;
            case geometry::PrimitiveType::Cone:
                generateConeMesh(std::get<geometry::Cone>(m_primitive));
                break;
            case geometry::PrimitiveType::Sphere:
                generateSphereMesh(std::get<geometry::Sphere>(m_primitive));
                break;
            default:
                break;
        }
        
        if (m_initialized) {
            uploadMesh();
        }
    }
}

void PrimitiveRenderer::setFitResult(const geometry::FitResult& result) {
    if (!result.success) {
        clear();
        return;
    }
    
    m_primitive = result.primitive;
    m_type = result.type;
    m_hasPrimitive = true;
    
    switch (result.type) {
        case geometry::PrimitiveType::Plane:
            generatePlaneMesh(result.get<geometry::Plane>());
            break;
        case geometry::PrimitiveType::Cylinder:
            generateCylinderMesh(result.get<geometry::Cylinder>());
            break;
        case geometry::PrimitiveType::Cone:
            generateConeMesh(result.get<geometry::Cone>());
            break;
        case geometry::PrimitiveType::Sphere:
            generateSphereMesh(result.get<geometry::Sphere>());
            break;
        default:
            clear();
            return;
    }
    
    if (m_initialized) {
        uploadMesh();
    }
}

void PrimitiveRenderer::setPrimitive(const geometry::Plane& plane) {
    m_primitive = plane;
    m_type = geometry::PrimitiveType::Plane;
    m_hasPrimitive = true;
    generatePlaneMesh(plane);
    if (m_initialized) uploadMesh();
}

void PrimitiveRenderer::setPrimitive(const geometry::Cylinder& cylinder) {
    m_primitive = cylinder;
    m_type = geometry::PrimitiveType::Cylinder;
    m_hasPrimitive = true;
    generateCylinderMesh(cylinder);
    if (m_initialized) uploadMesh();
}

void PrimitiveRenderer::setPrimitive(const geometry::Cone& cone) {
    m_primitive = cone;
    m_type = geometry::PrimitiveType::Cone;
    m_hasPrimitive = true;
    generateConeMesh(cone);
    if (m_initialized) uploadMesh();
}

void PrimitiveRenderer::setPrimitive(const geometry::Sphere& sphere) {
    m_primitive = sphere;
    m_type = geometry::PrimitiveType::Sphere;
    m_hasPrimitive = true;
    generateSphereMesh(sphere);
    if (m_initialized) uploadMesh();
}

void PrimitiveRenderer::clear() {
    m_mesh.vertices.clear();
    m_mesh.indices.clear();
    m_mesh.wireframeIndices.clear();
    m_mesh.labels.clear();
    m_hasPrimitive = false;
    m_type = geometry::PrimitiveType::Unknown;
}

void PrimitiveRenderer::setReferenceMesh(const geometry::MeshData& mesh) {
    m_referencePoints = mesh.vertices();
    updateDeviationColors();
}

void PrimitiveRenderer::setReferencePoints(const std::vector<glm::vec3>& points) {
    m_referencePoints = points;
    updateDeviationColors();
}

void PrimitiveRenderer::updateDeviationColors() {
    if (!m_hasPrimitive || m_referencePoints.empty()) return;
    
    for (auto& vertex : m_mesh.vertices) {
        float deviation = computeDeviation(vertex.position);
        vertex.color = deviationToColor(deviation);
    }
    
    if (m_initialized) {
        uploadMesh();
    }
}

bool PrimitiveRenderer::initialize() {
    // OpenGL initialization would go here
    // For now, mark as initialized
    m_initialized = true;
    
    // Compile shaders
    if (!compileShaders()) {
        return false;
    }
    
    // Create VAO, VBO, EBO
    // glGenVertexArrays(1, &m_vao);
    // glGenBuffers(1, &m_vbo);
    // glGenBuffers(1, &m_ebo);
    // glGenBuffers(1, &m_wireframeEbo);
    
    if (m_hasPrimitive) {
        uploadMesh();
    }
    
    return true;
}

void PrimitiveRenderer::cleanup() {
    if (!m_initialized) return;
    
    // Delete OpenGL resources
    // if (m_vao) glDeleteVertexArrays(1, &m_vao);
    // if (m_vbo) glDeleteBuffers(1, &m_vbo);
    // if (m_ebo) glDeleteBuffers(1, &m_ebo);
    // if (m_wireframeEbo) glDeleteBuffers(1, &m_wireframeEbo);
    // if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    
    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_wireframeEbo = 0;
    m_shaderProgram = 0;
    m_initialized = false;
}

void PrimitiveRenderer::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!m_initialized || !m_hasPrimitive) return;
    
    // Bind shader and set uniforms
    // glUseProgram(m_shaderProgram);
    setShaderUniforms(view, projection);
    
    // Enable blending for transparency
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Bind VAO
    // glBindVertexArray(m_vao);
    
    // Draw solid
    if (m_options.showSolid) {
        if (!m_options.backfaceCulling) {
            // glDisable(GL_CULL_FACE);
        }
        
        // Set uniform color
        // glUniform4fv(colorLoc, 1, &m_options.overlayColor[0]);
        
        // glDrawElements(GL_TRIANGLES, m_mesh.indices.size(), GL_UNSIGNED_INT, 0);
    }
    
    // Draw wireframe
    if (m_options.showWireframe) {
        renderWireframe(view, projection);
    }
    
    // glBindVertexArray(0);
}

void PrimitiveRenderer::renderWireframe(const glm::mat4& view, const glm::mat4& projection) {
    if (!m_initialized || !m_hasPrimitive) return;
    
    // glUseProgram(m_shaderProgram);
    setShaderUniforms(view, projection);
    
    // Set wireframe color
    // glUniform4fv(colorLoc, 1, &m_options.wireframeColor[0]);
    
    // glLineWidth(m_options.wireframeWidth);
    
    // glBindVertexArray(m_vao);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_wireframeEbo);
    // glDrawElements(GL_LINES, m_mesh.wireframeIndices.size(), GL_UNSIGNED_INT, 0);
    // glBindVertexArray(0);
}

void PrimitiveRenderer::renderLabels(const glm::mat4& view, const glm::mat4& projection,
                                      int screenWidth, int screenHeight) {
    // Text rendering would be implemented with a text rendering library
    // This provides the label data; actual rendering depends on the app's text system
    
    for (const auto& label : m_mesh.labels) {
        glm::vec2 screenPos = PrimitiveRenderUtils::projectToScreen(
            label.position, view, projection, screenWidth, screenHeight);
        
        // Render text at screenPos
        // textRenderer.drawText(label.text, screenPos.x, screenPos.y, label.size);
    }
}

void PrimitiveRenderer::generatePlaneMesh(const geometry::Plane& plane, float size) {
    m_mesh.vertices.clear();
    m_mesh.indices.clear();
    m_mesh.wireframeIndices.clear();
    m_mesh.labels.clear();
    
    // Get plane basis
    glm::vec3 u, v;
    plane.getBasis(u, v);
    glm::vec3 center = plane.getPointOnPlane();
    glm::vec3 normal = plane.normal();
    
    // Create quad vertices
    float halfSize = size * 0.5f;
    
    glm::vec3 corners[4] = {
        center - halfSize * u - halfSize * v,
        center + halfSize * u - halfSize * v,
        center + halfSize * u + halfSize * v,
        center - halfSize * u + halfSize * v
    };
    
    for (int i = 0; i < 4; ++i) {
        PrimitiveVertex vertex;
        vertex.position = corners[i];
        vertex.normal = normal;
        vertex.color = m_options.overlayColor;
        vertex.texCoord = glm::vec2(
            (i == 1 || i == 2) ? 1.0f : 0.0f,
            (i >= 2) ? 1.0f : 0.0f
        );
        m_mesh.vertices.push_back(vertex);
    }
    
    // Triangle indices
    m_mesh.indices = {0, 1, 2, 0, 2, 3};
    
    // Wireframe (quad outline)
    m_mesh.wireframeIndices = {0, 1, 1, 2, 2, 3, 3, 0};
    
    // Labels
    if (m_options.showDimensions) {
        generatePlaneLabels(plane);
    }
}

void PrimitiveRenderer::generateCylinderMesh(const geometry::Cylinder& cylinder) {
    m_mesh.vertices.clear();
    m_mesh.indices.clear();
    m_mesh.wireframeIndices.clear();
    m_mesh.labels.clear();
    
    int radial = m_options.radialSegments;
    int height = m_options.heightSegments;
    
    glm::vec3 axis = cylinder.axis();
    glm::vec3 center = cylinder.center();
    float r = cylinder.radius();
    float h = cylinder.height();
    
    // Get basis perpendicular to axis
    glm::vec3 u, v;
    glm::vec3 arbitrary = (std::abs(axis.x) < 0.9f) 
                         ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    u = glm::normalize(glm::cross(axis, arbitrary));
    v = glm::cross(axis, u);
    
    float halfH = h * 0.5f;
    
    // Generate lateral surface
    for (int j = 0; j <= height; ++j) {
        float t = static_cast<float>(j) / height;
        float y = -halfH + t * h;
        
        for (int i = 0; i <= radial; ++i) {
            float angle = glm::two_pi<float>() * i / radial;
            glm::vec3 radialDir = std::cos(angle) * u + std::sin(angle) * v;
            
            PrimitiveVertex vertex;
            vertex.position = center + y * axis + r * radialDir;
            vertex.normal = radialDir;
            vertex.color = m_options.overlayColor;
            vertex.texCoord = glm::vec2(static_cast<float>(i) / radial, t);
            m_mesh.vertices.push_back(vertex);
        }
    }
    
    // Triangle indices for lateral surface
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < radial; ++i) {
            uint32_t base = j * (radial + 1) + i;
            m_mesh.indices.push_back(base);
            m_mesh.indices.push_back(base + radial + 1);
            m_mesh.indices.push_back(base + 1);
            
            m_mesh.indices.push_back(base + 1);
            m_mesh.indices.push_back(base + radial + 1);
            m_mesh.indices.push_back(base + radial + 2);
        }
    }
    
    // End caps
    uint32_t capStartIdx = static_cast<uint32_t>(m_mesh.vertices.size());
    
    // Bottom cap
    PrimitiveVertex centerBottom;
    centerBottom.position = center - halfH * axis;
    centerBottom.normal = -axis;
    centerBottom.color = m_options.overlayColor;
    centerBottom.texCoord = glm::vec2(0.5f, 0.5f);
    m_mesh.vertices.push_back(centerBottom);
    
    for (int i = 0; i <= radial; ++i) {
        float angle = glm::two_pi<float>() * i / radial;
        glm::vec3 radialDir = std::cos(angle) * u + std::sin(angle) * v;
        
        PrimitiveVertex vertex;
        vertex.position = center - halfH * axis + r * radialDir;
        vertex.normal = -axis;
        vertex.color = m_options.overlayColor;
        vertex.texCoord = glm::vec2(0.5f + 0.5f * std::cos(angle), 
                                    0.5f + 0.5f * std::sin(angle));
        m_mesh.vertices.push_back(vertex);
    }
    
    // Bottom cap triangles
    for (int i = 0; i < radial; ++i) {
        m_mesh.indices.push_back(capStartIdx);
        m_mesh.indices.push_back(capStartIdx + i + 2);
        m_mesh.indices.push_back(capStartIdx + i + 1);
    }
    
    // Top cap
    uint32_t topCapStart = static_cast<uint32_t>(m_mesh.vertices.size());
    
    PrimitiveVertex centerTop;
    centerTop.position = center + halfH * axis;
    centerTop.normal = axis;
    centerTop.color = m_options.overlayColor;
    centerTop.texCoord = glm::vec2(0.5f, 0.5f);
    m_mesh.vertices.push_back(centerTop);
    
    for (int i = 0; i <= radial; ++i) {
        float angle = glm::two_pi<float>() * i / radial;
        glm::vec3 radialDir = std::cos(angle) * u + std::sin(angle) * v;
        
        PrimitiveVertex vertex;
        vertex.position = center + halfH * axis + r * radialDir;
        vertex.normal = axis;
        vertex.color = m_options.overlayColor;
        vertex.texCoord = glm::vec2(0.5f + 0.5f * std::cos(angle), 
                                    0.5f + 0.5f * std::sin(angle));
        m_mesh.vertices.push_back(vertex);
    }
    
    // Top cap triangles
    for (int i = 0; i < radial; ++i) {
        m_mesh.indices.push_back(topCapStart);
        m_mesh.indices.push_back(topCapStart + i + 1);
        m_mesh.indices.push_back(topCapStart + i + 2);
    }
    
    // Wireframe: circles at top and bottom, plus vertical lines
    for (int i = 0; i < radial; ++i) {
        // Bottom circle
        m_mesh.wireframeIndices.push_back(i);
        m_mesh.wireframeIndices.push_back(i + 1);
        
        // Top circle
        uint32_t topRowStart = height * (radial + 1);
        m_mesh.wireframeIndices.push_back(topRowStart + i);
        m_mesh.wireframeIndices.push_back(topRowStart + i + 1);
        
        // Vertical lines (every few segments)
        if (i % 8 == 0) {
            m_mesh.wireframeIndices.push_back(i);
            m_mesh.wireframeIndices.push_back(topRowStart + i);
        }
    }
    
    // Labels
    if (m_options.showDimensions) {
        generateCylinderLabels(cylinder);
    }
}

void PrimitiveRenderer::generateConeMesh(const geometry::Cone& cone) {
    m_mesh.vertices.clear();
    m_mesh.indices.clear();
    m_mesh.wireframeIndices.clear();
    m_mesh.labels.clear();
    
    int radial = m_options.radialSegments;
    int heightSegs = m_options.heightSegments;
    
    glm::vec3 apex = cone.apex();
    glm::vec3 axis = cone.axis();
    float halfAngle = cone.halfAngle();
    float h = cone.height();
    
    // Get basis
    glm::vec3 u, v;
    glm::vec3 arbitrary = (std::abs(axis.x) < 0.9f) 
                         ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    u = glm::normalize(glm::cross(axis, arbitrary));
    v = glm::cross(axis, u);
    
    // Surface normal makes angle (pi/2 - halfAngle) with axis
    float normalAngle = glm::half_pi<float>() - halfAngle;
    
    // Generate lateral surface
    for (int j = 0; j <= heightSegs; ++j) {
        float t = static_cast<float>(j) / heightSegs;
        float y = t * h;
        float r = y * std::tan(halfAngle);
        
        for (int i = 0; i <= radial; ++i) {
            float angle = glm::two_pi<float>() * i / radial;
            glm::vec3 radialDir = std::cos(angle) * u + std::sin(angle) * v;
            
            PrimitiveVertex vertex;
            vertex.position = apex + y * axis + r * radialDir;
            
            // Normal: combination of radial and axis components
            vertex.normal = glm::normalize(std::cos(normalAngle) * radialDir + 
                                          std::sin(normalAngle) * axis);
            vertex.color = m_options.overlayColor;
            vertex.texCoord = glm::vec2(static_cast<float>(i) / radial, t);
            m_mesh.vertices.push_back(vertex);
        }
    }
    
    // Triangle indices
    for (int j = 0; j < heightSegs; ++j) {
        for (int i = 0; i < radial; ++i) {
            uint32_t base = j * (radial + 1) + i;
            
            if (j > 0 || true) {  // Include apex triangles
                m_mesh.indices.push_back(base);
                m_mesh.indices.push_back(base + radial + 1);
                m_mesh.indices.push_back(base + 1);
            }
            
            m_mesh.indices.push_back(base + 1);
            m_mesh.indices.push_back(base + radial + 1);
            m_mesh.indices.push_back(base + radial + 2);
        }
    }
    
    // Base cap
    uint32_t capStart = static_cast<uint32_t>(m_mesh.vertices.size());
    float baseR = h * std::tan(halfAngle);
    glm::vec3 baseCenter = apex + h * axis;
    
    PrimitiveVertex centerVert;
    centerVert.position = baseCenter;
    centerVert.normal = axis;
    centerVert.color = m_options.overlayColor;
    centerVert.texCoord = glm::vec2(0.5f, 0.5f);
    m_mesh.vertices.push_back(centerVert);
    
    for (int i = 0; i <= radial; ++i) {
        float angle = glm::two_pi<float>() * i / radial;
        glm::vec3 radialDir = std::cos(angle) * u + std::sin(angle) * v;
        
        PrimitiveVertex vertex;
        vertex.position = baseCenter + baseR * radialDir;
        vertex.normal = axis;
        vertex.color = m_options.overlayColor;
        vertex.texCoord = glm::vec2(0.5f + 0.5f * std::cos(angle), 
                                    0.5f + 0.5f * std::sin(angle));
        m_mesh.vertices.push_back(vertex);
    }
    
    for (int i = 0; i < radial; ++i) {
        m_mesh.indices.push_back(capStart);
        m_mesh.indices.push_back(capStart + i + 1);
        m_mesh.indices.push_back(capStart + i + 2);
    }
    
    // Wireframe
    uint32_t baseRow = heightSegs * (radial + 1);
    for (int i = 0; i < radial; ++i) {
        // Base circle
        m_mesh.wireframeIndices.push_back(baseRow + i);
        m_mesh.wireframeIndices.push_back(baseRow + i + 1);
        
        // Lines from apex
        if (i % 8 == 0) {
            m_mesh.wireframeIndices.push_back(0);  // Apex
            m_mesh.wireframeIndices.push_back(baseRow + i);
        }
    }
    
    // Labels
    if (m_options.showDimensions) {
        generateConeLabels(cone);
    }
}

void PrimitiveRenderer::generateSphereMesh(const geometry::Sphere& sphere) {
    m_mesh.vertices.clear();
    m_mesh.indices.clear();
    m_mesh.wireframeIndices.clear();
    m_mesh.labels.clear();
    
    int lat = m_options.latitudeSegments;
    int lon = m_options.radialSegments;
    
    glm::vec3 center = sphere.center();
    float r = sphere.radius();
    
    // Generate vertices
    for (int j = 0; j <= lat; ++j) {
        float phi = glm::pi<float>() * j / lat;
        float y = r * std::cos(phi);
        float ringR = r * std::sin(phi);
        
        for (int i = 0; i <= lon; ++i) {
            float theta = glm::two_pi<float>() * i / lon;
            float x = ringR * std::cos(theta);
            float z = ringR * std::sin(theta);
            
            PrimitiveVertex vertex;
            vertex.position = center + glm::vec3(x, y, z);
            vertex.normal = glm::normalize(glm::vec3(x, y, z));
            vertex.color = m_options.overlayColor;
            vertex.texCoord = glm::vec2(static_cast<float>(i) / lon, 
                                        static_cast<float>(j) / lat);
            m_mesh.vertices.push_back(vertex);
        }
    }
    
    // Triangle indices
    for (int j = 0; j < lat; ++j) {
        for (int i = 0; i < lon; ++i) {
            uint32_t base = j * (lon + 1) + i;
            
            m_mesh.indices.push_back(base);
            m_mesh.indices.push_back(base + lon + 1);
            m_mesh.indices.push_back(base + 1);
            
            m_mesh.indices.push_back(base + 1);
            m_mesh.indices.push_back(base + lon + 1);
            m_mesh.indices.push_back(base + lon + 2);
        }
    }
    
    // Wireframe: latitude and longitude lines
    // Equator
    uint32_t equatorRow = (lat / 2) * (lon + 1);
    for (int i = 0; i < lon; ++i) {
        m_mesh.wireframeIndices.push_back(equatorRow + i);
        m_mesh.wireframeIndices.push_back(equatorRow + i + 1);
    }
    
    // A few longitude lines
    for (int i = 0; i < lon; i += lon / 8) {
        for (int j = 0; j < lat; ++j) {
            uint32_t idx = j * (lon + 1) + i;
            m_mesh.wireframeIndices.push_back(idx);
            m_mesh.wireframeIndices.push_back(idx + lon + 1);
        }
    }
    
    // Labels
    if (m_options.showDimensions) {
        generateSphereLabels(sphere);
    }
}

void PrimitiveRenderer::generatePlaneLabels(const geometry::Plane& plane) {
    if (!m_options.showPrimitiveType) return;
    
    DimensionLabel label;
    label.text = "Plane";
    label.position = plane.getPointOnPlane();
    label.normal = plane.normal();
    label.size = 14.0f * m_options.labelScale;
    m_mesh.labels.push_back(label);
}

void PrimitiveRenderer::generateCylinderLabels(const geometry::Cylinder& cylinder) {
    // Radius label
    DimensionLabel radiusLabel;
    radiusLabel.text = "R: " + PrimitiveRenderUtils::formatDimension(cylinder.radius());
    radiusLabel.position = cylinder.center();
    radiusLabel.normal = -cylinder.axis();
    radiusLabel.size = 12.0f * m_options.labelScale;
    m_mesh.labels.push_back(radiusLabel);
    
    // Height label
    DimensionLabel heightLabel;
    heightLabel.text = "H: " + PrimitiveRenderUtils::formatDimension(cylinder.height());
    
    glm::vec3 bottom, top;
    cylinder.getEndCaps(bottom, top);
    heightLabel.position = (bottom + top) * 0.5f;
    heightLabel.normal = glm::vec3(1, 0, 0);
    heightLabel.size = 12.0f * m_options.labelScale;
    m_mesh.labels.push_back(heightLabel);
    
    // Type label
    if (m_options.showPrimitiveType) {
        DimensionLabel typeLabel;
        typeLabel.text = "Cylinder";
        typeLabel.position = top + cylinder.axis() * cylinder.radius() * 0.5f;
        typeLabel.normal = cylinder.axis();
        typeLabel.size = 14.0f * m_options.labelScale;
        m_mesh.labels.push_back(typeLabel);
    }
}

void PrimitiveRenderer::generateConeLabels(const geometry::Cone& cone) {
    glm::vec3 baseCenter;
    float baseRadius;
    cone.getBase(baseCenter, baseRadius);
    
    // Base radius
    DimensionLabel radiusLabel;
    radiusLabel.text = "R: " + PrimitiveRenderUtils::formatDimension(baseRadius);
    radiusLabel.position = baseCenter;
    radiusLabel.normal = cone.axis();
    radiusLabel.size = 12.0f * m_options.labelScale;
    m_mesh.labels.push_back(radiusLabel);
    
    // Height
    DimensionLabel heightLabel;
    heightLabel.text = "H: " + PrimitiveRenderUtils::formatDimension(cone.height());
    heightLabel.position = cone.apex() + cone.axis() * cone.height() * 0.5f;
    heightLabel.normal = glm::vec3(1, 0, 0);
    heightLabel.size = 12.0f * m_options.labelScale;
    m_mesh.labels.push_back(heightLabel);
    
    // Angle
    DimensionLabel angleLabel;
    angleLabel.text = "θ: " + PrimitiveRenderUtils::formatDimension(cone.halfAngleDegrees(), "°", 1);
    angleLabel.position = cone.apex();
    angleLabel.normal = -cone.axis();
    angleLabel.size = 12.0f * m_options.labelScale;
    m_mesh.labels.push_back(angleLabel);
    
    // Type
    if (m_options.showPrimitiveType) {
        DimensionLabel typeLabel;
        typeLabel.text = "Cone";
        typeLabel.position = baseCenter + cone.axis() * baseRadius * 0.3f;
        typeLabel.normal = cone.axis();
        typeLabel.size = 14.0f * m_options.labelScale;
        m_mesh.labels.push_back(typeLabel);
    }
}

void PrimitiveRenderer::generateSphereLabels(const geometry::Sphere& sphere) {
    // Radius
    DimensionLabel radiusLabel;
    radiusLabel.text = "R: " + PrimitiveRenderUtils::formatDimension(sphere.radius());
    radiusLabel.position = sphere.center() + glm::vec3(sphere.radius(), 0, 0);
    radiusLabel.normal = glm::vec3(1, 0, 0);
    radiusLabel.size = 12.0f * m_options.labelScale;
    m_mesh.labels.push_back(radiusLabel);
    
    // Diameter
    DimensionLabel diamLabel;
    diamLabel.text = "D: " + PrimitiveRenderUtils::formatDimension(sphere.diameter());
    diamLabel.position = sphere.center();
    diamLabel.normal = glm::vec3(0, 1, 0);
    diamLabel.size = 12.0f * m_options.labelScale;
    m_mesh.labels.push_back(diamLabel);
    
    // Type
    if (m_options.showPrimitiveType) {
        DimensionLabel typeLabel;
        typeLabel.text = "Sphere";
        typeLabel.position = sphere.center() + glm::vec3(0, sphere.radius() * 1.2f, 0);
        typeLabel.normal = glm::vec3(0, 1, 0);
        typeLabel.size = 14.0f * m_options.labelScale;
        m_mesh.labels.push_back(typeLabel);
    }
}

void PrimitiveRenderer::uploadMesh() {
    // Upload vertex and index data to GPU
    // glBindVertexArray(m_vao);
    // glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // glBufferData(GL_ARRAY_BUFFER, m_mesh.vertices.size() * sizeof(PrimitiveVertex),
    //              m_mesh.vertices.data(), GL_DYNAMIC_DRAW);
    //
    // ... set vertex attributes ...
    //
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_mesh.indices.size() * sizeof(uint32_t),
    //              m_mesh.indices.data(), GL_DYNAMIC_DRAW);
    //
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_wireframeEbo);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_mesh.wireframeIndices.size() * sizeof(uint32_t),
    //              m_mesh.wireframeIndices.data(), GL_DYNAMIC_DRAW);
}

bool PrimitiveRenderer::compileShaders() {
    // Shader compilation would go here
    // For brevity, returning true - actual implementation would compile GLSL shaders
    return true;
}

void PrimitiveRenderer::setShaderUniforms(const glm::mat4& view, const glm::mat4& projection) {
    // Set model/view/projection matrices
    // glm::mat4 model = glm::mat4(1.0f);
    // glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    // glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    // glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
}

float PrimitiveRenderer::computeDeviation(const glm::vec3& point) const {
    switch (m_type) {
        case geometry::PrimitiveType::Plane:
            return std::get<geometry::Plane>(m_primitive).absoluteDistanceToPoint(point);
        case geometry::PrimitiveType::Cylinder:
            return std::get<geometry::Cylinder>(m_primitive).absoluteDistanceToPoint(point);
        case geometry::PrimitiveType::Cone:
            return std::get<geometry::Cone>(m_primitive).absoluteDistanceToPoint(point);
        case geometry::PrimitiveType::Sphere:
            return std::get<geometry::Sphere>(m_primitive).absoluteDistanceToPoint(point);
        default:
            return 0.0f;
    }
}

glm::vec4 PrimitiveRenderer::deviationToColor(float deviation) const {
    float t = std::min(1.0f, deviation / m_options.deviationThreshold);
    glm::vec3 color = glm::mix(m_options.lowDeviationColor, 
                               m_options.highDeviationColor, t);
    return glm::vec4(color, m_options.overlayColor.a);
}

// ---- Utility Functions ----

namespace PrimitiveRenderUtils {

std::string formatDimension(float value, const std::string& unit, int precision) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value << " " << unit;
    return ss.str();
}

glm::vec2 projectToScreen(const glm::vec3& worldPos,
                          const glm::mat4& view,
                          const glm::mat4& projection,
                          int screenWidth, int screenHeight) {
    glm::vec4 clipPos = projection * view * glm::vec4(worldPos, 1.0f);
    
    if (std::abs(clipPos.w) < 1e-6f) {
        return glm::vec2(-1.0f);  // Behind camera
    }
    
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
    
    return glm::vec2(
        (ndc.x + 1.0f) * 0.5f * screenWidth,
        (1.0f - ndc.y) * 0.5f * screenHeight  // Flip Y for screen coords
    );
}

std::vector<glm::vec3> generateCircle(const glm::vec3& center,
                                       const glm::vec3& normal,
                                       float radius, int segments) {
    std::vector<glm::vec3> points;
    points.reserve(segments);
    
    // Get basis
    glm::vec3 u, v;
    glm::vec3 arbitrary = (std::abs(normal.x) < 0.9f) 
                         ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    u = glm::normalize(glm::cross(normal, arbitrary));
    v = glm::cross(normal, u);
    
    for (int i = 0; i < segments; ++i) {
        float angle = glm::two_pi<float>() * i / segments;
        points.push_back(center + radius * (std::cos(angle) * u + std::sin(angle) * v));
    }
    
    return points;
}

} // namespace PrimitiveRenderUtils

} // namespace renderer
} // namespace dc3d
