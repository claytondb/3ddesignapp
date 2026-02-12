/**
 * @file PrimitiveRenderer.h
 * @brief Rendering for fitted primitives with deviation visualization
 * 
 * Provides OpenGL-based rendering of:
 * - Fitted primitive overlays (transparent)
 * - Dimension labels
 * - Deviation heatmaps
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "../geometry/primitives/Plane.h"
#include "../geometry/primitives/Cylinder.h"
#include "../geometry/primitives/Cone.h"
#include "../geometry/primitives/Sphere.h"
#include "../geometry/primitives/PrimitiveFitter.h"
#include "../geometry/MeshData.h"

namespace dc3d {
namespace renderer {

/**
 * @brief Rendering options for primitives
 */
struct PrimitiveRenderOptions {
    // Overlay appearance
    glm::vec4 overlayColor{0.2f, 0.6f, 1.0f, 0.3f};  ///< RGBA color
    glm::vec4 wireframeColor{0.1f, 0.3f, 0.8f, 0.8f};
    float wireframeWidth = 1.5f;
    bool showWireframe = true;
    bool showSolid = true;
    bool backfaceCulling = false;
    
    // Tessellation
    int radialSegments = 48;       ///< For cylinder/cone/sphere
    int heightSegments = 16;       ///< For cylinder/cone
    int latitudeSegments = 24;     ///< For sphere
    
    // Labels
    bool showDimensions = true;
    bool showPrimitiveType = true;
    glm::vec3 labelColor{1.0f, 1.0f, 1.0f};
    float labelScale = 1.0f;
    
    // Deviation visualization
    bool showDeviation = false;
    float deviationScale = 1.0f;   ///< Scale factor for color mapping
    glm::vec3 lowDeviationColor{0.0f, 1.0f, 0.0f};   ///< Green
    glm::vec3 highDeviationColor{1.0f, 0.0f, 0.0f};  ///< Red
    float deviationThreshold = 0.01f;  ///< Max deviation for full color
};

/**
 * @brief Label info for rendering
 */
struct DimensionLabel {
    std::string text;
    glm::vec3 position;
    glm::vec3 normal;        ///< For billboard orientation
    float size = 12.0f;      ///< Font size in pixels
};

/**
 * @brief Vertex data for primitive mesh
 */
struct PrimitiveVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;         ///< For deviation visualization
    glm::vec2 texCoord;
};

/**
 * @brief Generated mesh data for a primitive
 */
struct PrimitiveMesh {
    std::vector<PrimitiveVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> wireframeIndices;  ///< Line indices
    std::vector<DimensionLabel> labels;
};

/**
 * @brief Renders fitted primitives as overlays
 * 
 * This class generates geometry for primitives and provides
 * methods to render them with various visualization options.
 * 
 * Usage:
 * @code
 * PrimitiveRenderer renderer;
 * renderer.setOptions(options);
 * 
 * // From fit result
 * renderer.setFitResult(fitResult);
 * 
 * // Or specific primitive
 * renderer.setPrimitive(cylinder);
 * 
 * // In render loop
 * renderer.render(viewMatrix, projMatrix);
 * @endcode
 */
class PrimitiveRenderer {
public:
    PrimitiveRenderer();
    ~PrimitiveRenderer();
    
    // Disable copy (OpenGL resources)
    PrimitiveRenderer(const PrimitiveRenderer&) = delete;
    PrimitiveRenderer& operator=(const PrimitiveRenderer&) = delete;
    
    // Move is ok
    PrimitiveRenderer(PrimitiveRenderer&&) noexcept;
    PrimitiveRenderer& operator=(PrimitiveRenderer&&) noexcept;
    
    // ---- Options ----
    
    void setOptions(const PrimitiveRenderOptions& options);
    const PrimitiveRenderOptions& options() const { return m_options; }
    
    // ---- Primitive Setup ----
    
    /**
     * @brief Set primitive from fit result
     */
    void setFitResult(const geometry::FitResult& result);
    
    /**
     * @brief Set specific primitive type
     */
    void setPrimitive(const geometry::Plane& plane);
    void setPrimitive(const geometry::Cylinder& cylinder);
    void setPrimitive(const geometry::Cone& cone);
    void setPrimitive(const geometry::Sphere& sphere);
    
    /**
     * @brief Clear the current primitive
     */
    void clear();
    
    /**
     * @brief Check if a primitive is set
     */
    bool hasPrimitive() const { return m_hasPrimitive; }
    
    // ---- Deviation Visualization ----
    
    /**
     * @brief Set reference mesh for deviation calculation
     */
    void setReferenceMesh(const geometry::MeshData& mesh);
    
    /**
     * @brief Set reference points for deviation calculation
     */
    void setReferencePoints(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Update deviation colors based on reference
     */
    void updateDeviationColors();
    
    // ---- Rendering ----
    
    /**
     * @brief Initialize OpenGL resources (call after context creation)
     */
    bool initialize();
    
    /**
     * @brief Cleanup OpenGL resources
     */
    void cleanup();
    
    /**
     * @brief Render the primitive overlay
     * @param view View matrix
     * @param projection Projection matrix
     */
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    /**
     * @brief Render wireframe only
     */
    void renderWireframe(const glm::mat4& view, const glm::mat4& projection);
    
    /**
     * @brief Render dimension labels (requires text rendering setup)
     * @param view View matrix
     * @param projection Projection matrix
     * @param screenWidth Viewport width for label positioning
     * @param screenHeight Viewport height
     */
    void renderLabels(const glm::mat4& view, const glm::mat4& projection,
                      int screenWidth, int screenHeight);
    
    // ---- Mesh Access ----
    
    /**
     * @brief Get generated primitive mesh (for custom rendering)
     */
    const PrimitiveMesh& getMesh() const { return m_mesh; }
    
    /**
     * @brief Get dimension labels
     */
    const std::vector<DimensionLabel>& getLabels() const { return m_mesh.labels; }

private:
    PrimitiveRenderOptions m_options;
    PrimitiveMesh m_mesh;
    bool m_hasPrimitive = false;
    geometry::PrimitiveType m_type = geometry::PrimitiveType::Unknown;
    
    // Store current primitive for deviation calculation
    geometry::Primitive m_primitive;
    std::vector<glm::vec3> m_referencePoints;
    
    // OpenGL resources
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    uint32_t m_wireframeEbo = 0;
    uint32_t m_shaderProgram = 0;
    bool m_initialized = false;
    
    // Mesh generation
    void generatePlaneMesh(const geometry::Plane& plane, float size = 10.0f);
    void generateCylinderMesh(const geometry::Cylinder& cylinder);
    void generateConeMesh(const geometry::Cone& cone);
    void generateSphereMesh(const geometry::Sphere& sphere);
    
    // Label generation
    void generatePlaneLabels(const geometry::Plane& plane);
    void generateCylinderLabels(const geometry::Cylinder& cylinder);
    void generateConeLabels(const geometry::Cone& cone);
    void generateSphereLabels(const geometry::Sphere& sphere);
    
    // GPU upload
    void uploadMesh();
    
    // Shader helpers
    bool compileShaders();
    void setShaderUniforms(const glm::mat4& view, const glm::mat4& projection);
    
    // Deviation calculation
    float computeDeviation(const glm::vec3& point) const;
    glm::vec4 deviationToColor(float deviation) const;
};

/**
 * @brief Utility functions for primitive rendering
 */
namespace PrimitiveRenderUtils {
    
    /**
     * @brief Format dimension value for display
     * @param value Dimension value
     * @param unit Unit string (e.g., "mm", "in")
     * @param precision Decimal places
     */
    std::string formatDimension(float value, const std::string& unit = "mm", int precision = 2);
    
    /**
     * @brief Compute screen position of 3D point
     */
    glm::vec2 projectToScreen(const glm::vec3& worldPos,
                              const glm::mat4& view,
                              const glm::mat4& projection,
                              int screenWidth, int screenHeight);
    
    /**
     * @brief Generate circle vertices
     */
    std::vector<glm::vec3> generateCircle(const glm::vec3& center,
                                           const glm::vec3& normal,
                                           float radius, int segments);
}

} // namespace renderer
} // namespace dc3d
