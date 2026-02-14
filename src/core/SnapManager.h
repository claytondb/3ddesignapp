/**
 * @file SnapManager.h
 * @brief Snapping system for precise object positioning
 * 
 * Provides:
 * - Grid snapping: Objects snap to grid intersections when moved
 * - Object snapping: Snap to vertices, edges, faces of other objects
 * - Snap indicators: Visual feedback showing snap targets
 * - Configurable settings: Grid size, snap tolerance
 */

#ifndef DC3D_CORE_SNAPMANAGER_H
#define DC3D_CORE_SNAPMANAGER_H

#include <QObject>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace dc3d {
namespace geometry {
class MeshData;
}

namespace core {

/**
 * @brief Type of snap target
 */
enum class SnapType {
    None,       ///< No snapping
    Grid,       ///< Snap to grid intersection
    Vertex,     ///< Snap to mesh vertex
    Edge,       ///< Snap to edge midpoint or along edge
    EdgeMid,    ///< Snap to edge midpoint
    Face,       ///< Snap to face center
    FaceCenter, ///< Snap to face centroid
    Origin      ///< Snap to object origin
};

/**
 * @brief Result of a snap query
 */
struct SnapResult {
    bool snapped = false;       ///< Whether snapping occurred
    SnapType type = SnapType::None;
    glm::vec3 position{0.0f};   ///< Snapped world position
    glm::vec3 normal{0.0f, 1.0f, 0.0f};  ///< Surface normal at snap point
    uint64_t meshId = 0;        ///< Mesh ID if snapped to object
    uint32_t elementIndex = 0;  ///< Vertex/edge/face index
    float distance = 0.0f;      ///< Distance from original point
    
    explicit operator bool() const { return snapped; }
};

/**
 * @brief Configuration for snap behavior
 */
struct SnapSettings {
    // Grid snapping
    bool gridSnapEnabled = true;
    float gridSize = 1.0f;       ///< Grid cell size (units)
    float gridSubdivisions = 1;  ///< Number of subdivisions per cell
    
    // Object snapping
    bool objectSnapEnabled = true;
    bool snapToVertices = true;
    bool snapToEdges = true;
    bool snapToEdgeMidpoints = true;
    bool snapToFaces = true;
    bool snapToFaceCenters = true;
    bool snapToOrigins = true;
    
    // Snap tolerance
    float snapTolerance = 10.0f;   ///< Screen-space snap distance (pixels)
    float worldTolerance = 0.5f;   ///< World-space snap distance (units)
    
    // Visual settings
    bool showSnapIndicator = true;
    float indicatorSize = 8.0f;
    
    /**
     * @brief Get effective grid spacing
     */
    float effectiveGridSize() const {
        return gridSize / gridSubdivisions;
    }
};

/**
 * @brief Candidate snap target for rendering indicators
 */
struct SnapCandidate {
    SnapType type;
    glm::vec3 position;
    glm::vec3 normal;
    uint64_t meshId = 0;
    float screenDistance = 0.0f;  ///< Distance from cursor in pixels
};

/**
 * @brief Manager for snapping behavior during transforms
 */
class SnapManager : public QObject {
    Q_OBJECT

public:
    explicit SnapManager(QObject* parent = nullptr);
    ~SnapManager() override = default;
    
    // ---- Enable/Disable ----
    
    /**
     * @brief Enable or disable all snapping
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Toggle snapping on/off
     */
    void toggleEnabled();
    
    /**
     * @brief Enable or disable grid snapping
     */
    void setGridSnapEnabled(bool enabled);
    bool isGridSnapEnabled() const { return m_settings.gridSnapEnabled; }
    
    /**
     * @brief Enable or disable object snapping
     */
    void setObjectSnapEnabled(bool enabled);
    bool isObjectSnapEnabled() const { return m_settings.objectSnapEnabled; }
    
    // ---- Settings ----
    
    /**
     * @brief Get current snap settings
     */
    const SnapSettings& settings() const { return m_settings; }
    
    /**
     * @brief Get mutable snap settings
     */
    SnapSettings& settings() { return m_settings; }
    
    /**
     * @brief Set grid size
     */
    void setGridSize(float size);
    float gridSize() const { return m_settings.gridSize; }
    
    /**
     * @brief Set snap tolerance in pixels
     */
    void setSnapTolerance(float pixels);
    float snapTolerance() const { return m_settings.snapTolerance; }
    
    // ---- Snapping Operations ----
    
    /**
     * @brief Snap a point to the grid
     * @param point World position to snap
     * @return Snapped position
     */
    glm::vec3 snapToGrid(const glm::vec3& point) const;
    
    /**
     * @brief Find the best snap target for a world position
     * @param point World position to snap
     * @param excludeMeshId Optional mesh to exclude (self-snapping)
     * @return SnapResult with snapped position if found
     */
    SnapResult findSnapTarget(const glm::vec3& point, 
                              uint64_t excludeMeshId = 0) const;
    
    /**
     * @brief Find snap target with screen-space tolerance
     * @param point World position
     * @param screenPos Screen position for tolerance checking
     * @param viewMatrix Camera view matrix
     * @param projMatrix Camera projection matrix  
     * @param viewportSize Viewport dimensions
     * @param excludeMeshId Mesh to exclude
     * @return SnapResult
     */
    SnapResult findSnapTarget(const glm::vec3& point,
                              const glm::vec2& screenPos,
                              const glm::mat4& viewMatrix,
                              const glm::mat4& projMatrix,
                              const glm::vec2& viewportSize,
                              uint64_t excludeMeshId = 0) const;
    
    /**
     * @brief Apply snapping to a position
     * @param point Original world position
     * @param excludeMeshId Mesh to exclude from object snapping
     * @return Snapped position (may be unchanged if no snap found)
     */
    glm::vec3 snap(const glm::vec3& point, uint64_t excludeMeshId = 0) const;
    
    // ---- Snap Candidates (for rendering) ----
    
    /**
     * @brief Get all potential snap targets near a point
     * @param point World position
     * @param viewMatrix Camera view matrix
     * @param projMatrix Camera projection matrix
     * @param viewportSize Viewport dimensions
     * @param maxCandidates Maximum candidates to return
     * @return Vector of snap candidates sorted by distance
     */
    std::vector<SnapCandidate> findSnapCandidates(
        const glm::vec3& point,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        const glm::vec2& viewportSize,
        size_t maxCandidates = 5) const;
    
    /**
     * @brief Get the current active snap result
     * Updated during snap operations for indicator rendering
     */
    const SnapResult& activeSnap() const { return m_activeSnap; }
    
    // ---- Mesh Registration ----
    
    /**
     * @brief Register a mesh for object snapping
     * @param id Mesh identifier
     * @param mesh Mesh data
     * @param transform Current mesh transform
     */
    void registerMesh(uint64_t id, 
                      std::shared_ptr<geometry::MeshData> mesh,
                      const glm::mat4& transform = glm::mat4(1.0f));
    
    /**
     * @brief Update mesh transform
     */
    void updateMeshTransform(uint64_t id, const glm::mat4& transform);
    
    /**
     * @brief Unregister a mesh
     */
    void unregisterMesh(uint64_t id);
    
    /**
     * @brief Clear all registered meshes
     */
    void clearMeshes();

signals:
    /**
     * @brief Emitted when snap settings change
     */
    void settingsChanged();
    
    /**
     * @brief Emitted when snap enable state changes
     */
    void enabledChanged(bool enabled);
    
    /**
     * @brief Emitted when active snap changes
     */
    void activeSnapChanged(const SnapResult& result);

private:
    struct RegisteredMesh {
        uint64_t id;
        std::shared_ptr<geometry::MeshData> mesh;
        glm::mat4 transform;
        
        // Cached snap points (world space)
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> edgeMidpoints;
        std::vector<glm::vec3> faceCenters;
        glm::vec3 origin;
    };
    
    void rebuildSnapCache(RegisteredMesh& regMesh);
    
    SnapResult findVertexSnap(const glm::vec3& point, 
                              uint64_t excludeMeshId) const;
    SnapResult findEdgeSnap(const glm::vec3& point,
                            uint64_t excludeMeshId) const;
    SnapResult findFaceSnap(const glm::vec3& point,
                            uint64_t excludeMeshId) const;
    
    bool m_enabled = true;
    SnapSettings m_settings;
    mutable SnapResult m_activeSnap;
    
    std::vector<RegisteredMesh> m_meshes;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_SNAPMANAGER_H
