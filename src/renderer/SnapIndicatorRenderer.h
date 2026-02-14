/**
 * @file SnapIndicatorRenderer.h
 * @brief Visual indicators for snap targets
 * 
 * Renders snap point indicators:
 * - Vertex snap: Small circle
 * - Edge midpoint: Triangle
 * - Face center: Square
 * - Grid snap: Cross
 */

#ifndef DC_SNAPINDICATORRENDERER_H
#define DC_SNAPINDICATORRENDERER_H

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <QSize>
#include <memory>

namespace dc3d {
namespace core {
struct SnapResult;
enum class SnapType;
}
}

namespace dc {

class ShaderProgram;
class Camera;

/**
 * @brief Renders snap indicator symbols at snap points
 */
class SnapIndicatorRenderer : protected QOpenGLFunctions_4_1_Core {
public:
    SnapIndicatorRenderer();
    ~SnapIndicatorRenderer();
    
    // Non-copyable
    SnapIndicatorRenderer(const SnapIndicatorRenderer&) = delete;
    SnapIndicatorRenderer& operator=(const SnapIndicatorRenderer&) = delete;
    
    /**
     * @brief Initialize OpenGL resources
     */
    bool initialize();
    
    /**
     * @brief Clean up OpenGL resources
     */
    void cleanup();
    
    /**
     * @brief Render snap indicator at position
     * @param snapResult Current snap result
     * @param camera Camera for matrices
     * @param viewportSize Viewport dimensions
     */
    void render(const dc3d::core::SnapResult& snapResult,
                const Camera& camera,
                const QSize& viewportSize);
    
    /**
     * @brief Set indicator size in pixels
     */
    void setIndicatorSize(float size) { m_indicatorSize = size; }
    float indicatorSize() const { return m_indicatorSize; }
    
    /**
     * @brief Set indicator colors
     */
    void setVertexColor(const QColor& color) { m_vertexColor = color; }
    void setEdgeColor(const QColor& color) { m_edgeColor = color; }
    void setFaceColor(const QColor& color) { m_faceColor = color; }
    void setGridColor(const QColor& color) { m_gridColor = color; }

private:
    void createGeometry();
    void renderIndicator(dc3d::core::SnapType type,
                        const QVector3D& position,
                        const Camera& camera,
                        const QSize& viewportSize);
    QColor colorForSnapType(dc3d::core::SnapType type) const;
    
    bool m_initialized = false;
    float m_indicatorSize = 12.0f;
    
    // Colors for different snap types
    QColor m_vertexColor{255, 200, 50};   // Yellow/gold for vertices
    QColor m_edgeColor{50, 200, 255};     // Cyan for edges
    QColor m_faceColor{255, 100, 100};    // Red for faces
    QColor m_gridColor{100, 255, 100};    // Green for grid
    QColor m_originColor{255, 150, 50};   // Orange for origin
    
    // Geometry for different indicator types
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    
    // Shader
    std::unique_ptr<ShaderProgram> m_shader;
    
    // Vertex offsets for different shapes
    int m_circleOffset = 0;
    int m_circleCount = 0;
    int m_triangleOffset = 0;
    int m_triangleCount = 0;
    int m_squareOffset = 0;
    int m_squareCount = 0;
    int m_crossOffset = 0;
    int m_crossCount = 0;
    int m_diamondOffset = 0;
    int m_diamondCount = 0;
};

} // namespace dc

#endif // DC_SNAPINDICATORRENDERER_H
