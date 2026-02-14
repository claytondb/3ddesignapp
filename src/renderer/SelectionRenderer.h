/**
 * @file SelectionRenderer.h
 * @brief Renders selection highlights for meshes
 * 
 * Provides visual feedback for selected objects, faces, vertices, and edges:
 * - Object outlines
 * - Face highlighting with transparency
 * - Vertex point rendering
 * - Edge line rendering
 * - Box selection rectangle overlay
 */

#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <memory>
#include <vector>
#include <set>
#include <cstdint>
#include <glm/glm.hpp>

#include "../core/Selection.h"

// Forward declarations
class QOpenGLShaderProgram;

namespace dc {
    class Camera;
}

namespace dc3d {

namespace geometry {
    class MeshData;
}

namespace renderer {

/**
 * @brief Mesh data needed for selection rendering
 */
struct SelectionMeshInfo {
    uint32_t meshId = 0;
    const geometry::MeshData* mesh = nullptr;
    glm::mat4 transform = glm::mat4(1.0f);
    bool visible = true;
    
    // OpenGL handles (may be shared with main mesh renderer)
    uint32_t vao = 0;
    uint32_t vboPosition = 0;
    uint32_t vboNormal = 0;
    uint32_t ebo = 0;
};

/**
 * @brief Selection rendering configuration
 */
struct SelectionRenderConfig {
    // Colors
    glm::vec4 objectColor{1.0f, 0.58f, 0.0f, 1.0f};   // Orange
    glm::vec4 faceColor{0.3f, 0.6f, 1.0f, 0.5f};      // Blue
    glm::vec4 vertexColor{0.2f, 1.0f, 0.3f, 1.0f};    // Green
    glm::vec4 edgeColor{1.0f, 0.9f, 0.2f, 1.0f};      // Yellow
    glm::vec4 hoverColor{1.0f, 0.75f, 0.4f, 0.6f};    // Lighter orange for hover
    glm::vec4 boxSelectColor{0.3f, 0.6f, 1.0f, 0.2f}; // Light blue
    
    // Rendering options
    float outlineWidth = 3.0f;         // Outline width in pixels (screen-space)
    float vertexSize = 10.0f;          // Point size for vertices
    float edgeWidth = 2.5f;            // Line width for edges
    bool xrayMode = false;             // Draw through geometry
    float opacity = 1.0f;              // Overall opacity
    bool showHover = true;             // Show hover highlighting
};

/**
 * @brief Renders selection highlights
 */
class SelectionRenderer : protected QOpenGLFunctions_4_1_Core {
public:
    SelectionRenderer();
    ~SelectionRenderer();
    
    /**
     * @brief Initialize OpenGL resources
     * @return true on success
     */
    bool initialize();
    
    /**
     * @brief Clean up OpenGL resources
     */
    void cleanup();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return m_initialized; }
    
    // ---- Mesh Management ----
    
    /**
     * @brief Register a mesh for selection rendering
     * @param meshId Unique mesh ID
     * @param mesh Mesh data pointer
     * @param transform World transform
     */
    void addMesh(uint32_t meshId, const geometry::MeshData* mesh,
                 const glm::mat4& transform = glm::mat4(1.0f));
    
    /**
     * @brief Update mesh transform
     */
    void updateTransform(uint32_t meshId, const glm::mat4& transform);
    
    /**
     * @brief Remove a mesh
     */
    void removeMesh(uint32_t meshId);
    
    /**
     * @brief Clear all meshes
     */
    void clearMeshes();
    
    // ---- Rendering ----
    
    /**
     * @brief Render selection highlights
     * @param camera Current camera
     * @param selection Current selection state
     */
    void render(const dc::Camera& camera, const core::Selection& selection);
    
    /**
     * @brief Render hover highlight (pre-selection feedback)
     * @param camera Current camera
     * @param hitInfo Current hover target
     * @param mode Selection mode
     */
    void renderHover(const dc::Camera& camera, 
                     const core::HitInfo& hitInfo,
                     core::SelectionMode mode);
    
    /**
     * @brief Render box selection rectangle
     * @param startPos Start point (screen coords)
     * @param endPos End point (screen coords)
     * @param viewportSize Viewport dimensions
     */
    void renderBoxSelection(const QPoint& startPos, 
                           const QPoint& endPos,
                           const QSize& viewportSize);
    
    // ---- Configuration ----
    
    /**
     * @brief Get render configuration
     */
    SelectionRenderConfig& config() { return m_config; }
    const SelectionRenderConfig& config() const { return m_config; }
    
    /**
     * @brief Set X-ray mode (draw selection through geometry)
     */
    void setXRayMode(bool enabled) { m_config.xrayMode = enabled; }
    
    /**
     * @brief Get X-ray mode state
     */
    bool xrayMode() const { return m_config.xrayMode; }

private:
    // Shader setup
    bool loadShaders();
    
    // Rendering helpers
    void renderObjectSelection(const dc::Camera& camera,
                              const std::vector<uint32_t>& meshIds);
    
    void renderFaceSelection(const dc::Camera& camera,
                            uint32_t meshId,
                            const std::vector<uint32_t>& faceIndices);
    
    void renderVertexSelection(const dc::Camera& camera,
                              uint32_t meshId,
                              const std::vector<uint32_t>& vertexIndices);
    
    void renderEdgeSelection(const dc::Camera& camera,
                            uint32_t meshId,
                            const std::vector<uint32_t>& edgeIndices);
    
    // Find mesh info by ID
    SelectionMeshInfo* findMesh(uint32_t meshId);
    const SelectionMeshInfo* findMesh(uint32_t meshId) const;
    
    // Create GPU buffers for mesh
    void createMeshBuffers(SelectionMeshInfo& info);
    void deleteMeshBuffers(SelectionMeshInfo& info);
    
    // Convert QMatrix to glm
    static glm::mat4 toGlm(const QMatrix4x4& m);
    
    // State
    bool m_initialized = false;
    SelectionRenderConfig m_config;
    std::vector<SelectionMeshInfo> m_meshes;
    
    // Shaders
    std::unique_ptr<QOpenGLShaderProgram> m_selectionShader;
    std::unique_ptr<QOpenGLShaderProgram> m_pointShader;
    std::unique_ptr<QOpenGLShaderProgram> m_lineShader;
    std::unique_ptr<QOpenGLShaderProgram> m_boxShader;
    
    // Box selection quad
    uint32_t m_boxVAO = 0;
    uint32_t m_boxVBO = 0;
    
    // Point/line buffers (dynamic)
    uint32_t m_pointVAO = 0;
    uint32_t m_pointVBO = 0;
    uint32_t m_lineVAO = 0;
    uint32_t m_lineVBO = 0;
};

} // namespace renderer
} // namespace dc3d
