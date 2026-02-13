/**
 * @file DeviationRenderer.h
 * @brief Colormap-based visualization of mesh deviations
 * 
 * Renders meshes with per-vertex colors based on deviation values,
 * using a blue-green-red colormap to show signed distances.
 */

#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QColor>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace dc3d {
namespace geometry {
class MeshData;
}
}

namespace dc {

class ShaderProgram;

/**
 * @brief Colormap type for deviation visualization
 */
enum class DeviationColormap {
    BlueGreenRed,    ///< Blue (-) → Green (0) → Red (+)
    Rainbow,         ///< Full rainbow spectrum
    CoolWarm,        ///< Blue → White → Red (diverging)
    Viridis,         ///< Perceptually uniform (blue-green-yellow)
    Magma,           ///< Perceptually uniform (dark-magenta-yellow)
    Grayscale        ///< Black → White
};

/**
 * @brief Legend position for deviation display
 */
enum class LegendPosition {
    None,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

/**
 * @brief Configuration for deviation rendering
 */
struct DeviationRenderConfig {
    DeviationColormap colormap = DeviationColormap::BlueGreenRed;
    LegendPosition legendPosition = LegendPosition::BottomRight;
    
    float minValue = -1.0f;      ///< Minimum value for color mapping
    float maxValue = 1.0f;       ///< Maximum value for color mapping
    bool autoRange = true;       ///< Auto-compute range from data
    
    float legendWidth = 30.0f;   ///< Legend width in pixels
    float legendHeight = 200.0f; ///< Legend height in pixels
    float legendMargin = 20.0f;  ///< Margin from viewport edge
    
    bool showLabels = true;      ///< Show min/max labels
    int numTicks = 5;            ///< Number of tick marks on legend
    
    float transparency = 1.0f;   ///< Overall transparency (1.0 = opaque)
};

/**
 * @brief GPU data for deviation-colored mesh
 */
struct DeviationMeshGPU {
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ebo{QOpenGLBuffer::IndexBuffer};
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    bool valid = false;
};

/**
 * @brief Renders meshes with deviation colormaps
 * 
 * Provides visualization of per-vertex deviation values using
 * color gradients. Includes a legend overlay for reference.
 */
class DeviationRenderer : protected QOpenGLFunctions_4_1_Core {
public:
    DeviationRenderer();
    ~DeviationRenderer();
    
    /**
     * @brief Initialize OpenGL resources
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Release OpenGL resources
     */
    void cleanup();
    
    /**
     * @brief Check if renderer is initialized
     */
    bool isInitialized() const { return m_initialized; }
    
    // ---- Data Setup ----
    
    /**
     * @brief Set deviation data for a mesh
     * @param meshId Mesh identifier
     * @param mesh Source mesh geometry
     * @param deviations Per-vertex deviation values
     * @param minVal Minimum value for color mapping (-inf for auto)
     * @param maxVal Maximum value for color mapping (inf for auto)
     */
    void setDeviationData(
        uint64_t meshId,
        const dc3d::geometry::MeshData& mesh,
        const std::vector<float>& deviations,
        float minVal = -std::numeric_limits<float>::infinity(),
        float maxVal = std::numeric_limits<float>::infinity()
    );
    
    /**
     * @brief Update deviation values without re-uploading geometry
     * @param meshId Mesh identifier
     * @param deviations New deviation values
     */
    void updateDeviationValues(
        uint64_t meshId,
        const std::vector<float>& deviations
    );
    
    /**
     * @brief Remove deviation data for a mesh
     * @param meshId Mesh identifier
     */
    void removeDeviationData(uint64_t meshId);
    
    /**
     * @brief Clear all deviation data
     */
    void clearAll();
    
    // ---- Rendering ----
    
    /**
     * @brief Render deviation-colored meshes
     * @param viewMatrix View transformation
     * @param projMatrix Projection transformation
     */
    void render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    
    /**
     * @brief Render the color legend overlay
     * @param viewportWidth Viewport width in pixels
     * @param viewportHeight Viewport height in pixels
     */
    void renderLegend(int viewportWidth, int viewportHeight);
    
    // ---- Configuration ----
    
    /**
     * @brief Set rendering configuration
     */
    void setConfig(const DeviationRenderConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const DeviationRenderConfig& config() const { return config_; }
    
    /**
     * @brief Set color range manually
     * @param minVal Minimum value (maps to blue)
     * @param maxVal Maximum value (maps to red)
     */
    void setRange(float minVal, float maxVal);
    
    /**
     * @brief Enable auto-range based on data
     */
    void setAutoRange(bool enabled);
    
    /**
     * @brief Set colormap type
     */
    void setColormap(DeviationColormap colormap);
    
    /**
     * @brief Get current color range
     */
    void getRange(float& minVal, float& maxVal) const;
    
    // ---- Color Utilities ----
    
    /**
     * @brief Map a deviation value to color
     * @param value Deviation value
     * @return Color for the value
     */
    QColor valueToColor(float value) const;
    
    /**
     * @brief Get color at normalized position [0,1] in colormap
     * @param t Normalized position
     * @return Color at position
     */
    QColor colormapSample(float t) const;

private:
    void setupShaders();
    void setupLegendGeometry();
    
    glm::vec3 sampleColormap(float t) const;
    glm::vec3 sampleBlueGreenRed(float t) const;
    glm::vec3 sampleRainbow(float t) const;
    glm::vec3 sampleCoolWarm(float t) const;
    glm::vec3 sampleViridis(float t) const;
    glm::vec3 sampleMagma(float t) const;
    glm::vec3 sampleGrayscale(float t) const;
    
    void uploadMeshData(
        uint64_t meshId,
        const dc3d::geometry::MeshData& mesh,
        const std::vector<float>& deviations
    );
    
    void computeAutoRange(const std::vector<float>& deviations);
    
    bool m_initialized = false;  // Standardized naming (m_ prefix)
    DeviationRenderConfig config_;
    
    // Shaders
    std::unique_ptr<QOpenGLShaderProgram> meshShader_;
    std::unique_ptr<QOpenGLShaderProgram> legendShader_;
    
    // Uniform locations
    int mvpLoc_ = -1;
    int modelLoc_ = -1;
    int minValLoc_ = -1;
    int maxValLoc_ = -1;
    int colormapTypeLoc_ = -1;
    
    // Legend geometry
    QOpenGLVertexArrayObject legendVAO_;
    QOpenGLBuffer legendVBO_{QOpenGLBuffer::VertexBuffer};
    
    // Per-mesh GPU data
    std::unordered_map<uint64_t, std::unique_ptr<DeviationMeshGPU>> meshData_;
    
    // Data range
    float dataMin_ = 0.0f;
    float dataMax_ = 1.0f;
};

} // namespace dc
