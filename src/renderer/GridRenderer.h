/**
 * @file GridRenderer.h
 * @brief Ground plane grid and coordinate axes renderer
 * 
 * Renders an infinite-looking ground grid on the XZ plane with:
 * - Major and minor grid lines
 * - Distance-based fading
 * - RGB coordinate axes (X=Red, Y=Green, Z=Blue)
 */

#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>

namespace dc {

class ShaderProgram;
class Camera;

/**
 * @brief Grid rendering settings
 */
struct GridSettings {
    float gridSize = 100.0f;          // Total grid extent
    float majorSpacing = 10.0f;       // Major line spacing
    float minorSpacing = 1.0f;        // Minor line spacing
    float fadeDistance = 80.0f;       // Distance at which grid fades out
    
    QVector3D majorColor{0.4f, 0.4f, 0.4f};
    QVector3D minorColor{0.25f, 0.25f, 0.25f};
    
    float majorLineWidth = 1.5f;
    float minorLineWidth = 1.0f;
    
    bool showAxes = true;
    float axisLength = 10.0f;
    float axisLineWidth = 2.0f;
};

/**
 * @brief Renders ground plane grid and coordinate axes
 */
class GridRenderer : protected QOpenGLFunctions_4_1_Core {
public:
    GridRenderer();
    ~GridRenderer();
    
    // Non-copyable
    GridRenderer(const GridRenderer&) = delete;
    GridRenderer& operator=(const GridRenderer&) = delete;
    
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
     * @brief Render the grid
     * @param camera Camera for view/projection matrices
     */
    void render(const Camera& camera);
    
    /**
     * @brief Render only the coordinate axes
     * @param camera Camera for view/projection matrices
     */
    void renderAxes(const Camera& camera);
    
    /**
     * @brief Get mutable grid settings
     */
    GridSettings& settings() { return m_settings; }
    
    /**
     * @brief Get const grid settings
     */
    const GridSettings& settings() const { return m_settings; }
    
    /**
     * @brief Set visibility
     */
    void setVisible(bool visible) { m_visible = visible; }
    
    /**
     * @brief Check visibility
     */
    bool isVisible() const { return m_visible; }

private:
    void createGridGeometry();
    void createAxisGeometry();
    bool loadShaders();
    
    GridSettings m_settings;
    bool m_visible = true;
    bool m_initialized = false;
    
    // Grid geometry
    QOpenGLVertexArrayObject m_gridVAO;
    QOpenGLBuffer m_gridVBO{QOpenGLBuffer::VertexBuffer};
    int m_majorLineCount = 0;
    int m_minorLineCount = 0;
    int m_majorLineOffset = 0;
    int m_minorLineOffset = 0;
    
    // Axis geometry
    QOpenGLVertexArrayObject m_axisVAO;
    QOpenGLBuffer m_axisVBO{QOpenGLBuffer::VertexBuffer};
    
    // Shaders
    std::unique_ptr<ShaderProgram> m_gridShader;
    std::unique_ptr<ShaderProgram> m_lineShader;
};

} // namespace dc
