/**
 * @file Picking.h
 * @brief Ray casting and picking utilities for mouse-based selection
 * 
 * Provides:
 * - Generate rays from mouse position + camera
 * - Query BVH for intersections
 * - Determine closest vertex/edge from hit point
 * - Box selection frustum generation
 */

#pragma once

#include <QMatrix4x4>
#include <QPoint>
#include <QRect>
#include <memory>
#include <vector>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../geometry/BVH.h"
#include "../core/Selection.h"

// Forward declarations
namespace dc {
    class Camera;
}

namespace dc3d {
namespace geometry {
    class MeshData;
}

namespace renderer {

/**
 * @brief Mesh instance for picking (mesh data + transform + ID)
 */
struct PickableMesh {
    uint32_t meshId = 0;
    const geometry::MeshData* mesh = nullptr;
    std::shared_ptr<geometry::BVH> bvh;
    glm::mat4 transform = glm::mat4(1.0f);
    glm::mat4 inverseTransform = glm::mat4(1.0f);
    bool visible = true;
};

/**
 * @brief Picking system for mouse-based selection
 * 
 * Handles:
 * - Single-click picking (ray cast)
 * - Box selection (frustum query)
 * - Vertex/Edge snapping from face hit
 */
class Picking {
public:
    Picking();
    ~Picking() = default;
    
    // ---- Mesh Management ----
    
    /**
     * @brief Add a mesh for picking
     * @param meshId Unique ID for the mesh
     * @param mesh Pointer to mesh data (must remain valid)
     * @param transform World transform matrix
     */
    void addMesh(uint32_t meshId, const geometry::MeshData* mesh, 
                 const glm::mat4& transform = glm::mat4(1.0f));
    
    /**
     * @brief Update mesh transform
     */
    void updateTransform(uint32_t meshId, const glm::mat4& transform);
    
    /**
     * @brief Update mesh visibility
     */
    void setMeshVisible(uint32_t meshId, bool visible);
    
    /**
     * @brief Remove a mesh from picking
     */
    void removeMesh(uint32_t meshId);
    
    /**
     * @brief Rebuild BVH for a mesh (call after mesh data changes)
     */
    void rebuildBVH(uint32_t meshId);
    
    /**
     * @brief Clear all meshes
     */
    void clear();
    
    // ---- Ray Generation ----
    
    /**
     * @brief Generate a world-space ray from screen coordinates
     * @param screenPos Mouse position (top-left origin)
     * @param viewportSize Viewport dimensions
     * @param viewMatrix Camera view matrix
     * @param projMatrix Camera projection matrix
     * @return World-space ray
     */
    static geometry::Ray screenToRay(const QPoint& screenPos,
                                     const QSize& viewportSize,
                                     const QMatrix4x4& viewMatrix,
                                     const QMatrix4x4& projMatrix);
    
    /**
     * @brief Generate ray from dc::Camera
     */
    static geometry::Ray screenToRay(const QPoint& screenPos,
                                     const QSize& viewportSize,
                                     const dc::Camera& camera);
    
    // ---- Single Pick ----
    
    /**
     * @brief Pick at a screen position
     * @param screenPos Mouse position
     * @param viewportSize Viewport dimensions  
     * @param camera Camera for ray generation
     * @return Hit info including mesh ID, face, point, etc.
     */
    core::HitInfo pick(const QPoint& screenPos,
                       const QSize& viewportSize,
                       const dc::Camera& camera) const;
    
    /**
     * @brief Pick using a pre-computed ray
     */
    core::HitInfo pick(const geometry::Ray& worldRay) const;
    
    // ---- Box Selection ----
    
    /**
     * @brief Get all faces/objects within a screen-space rectangle
     * @param rect Selection rectangle in screen coords
     * @param viewportSize Viewport dimensions
     * @param camera Camera for frustum generation
     * @param mode What to select (Object, Face, etc.)
     * @return List of selection elements
     */
    std::vector<core::SelectionElement> boxSelect(
        const QRect& rect,
        const QSize& viewportSize,
        const dc::Camera& camera,
        core::SelectionMode mode) const;
    
    // ---- Hit Processing ----
    
    /**
     * @brief Determine closest vertex to a hit point
     * @param hit The hit result from picking
     * @return Index of closest vertex (from hit.vertexIndices)
     */
    static uint32_t findClosestVertex(const core::HitInfo& hit);
    
    /**
     * @brief Determine closest edge to a hit point
     * @param hit The hit result from picking  
     * @return Edge index (0, 1, or 2 for edges 0-1, 1-2, 2-0)
     */
    static int findClosestEdge(const core::HitInfo& hit);
    
    /**
     * @brief Enhance hit info with closest vertex/edge data
     * @param hit Hit result to enhance (modified in place)
     * @param mesh Mesh data for vertex positions
     */
    static void enhanceHitInfo(core::HitInfo& hit, const geometry::MeshData& mesh);

private:
    /**
     * @brief Build selection frustum from screen rect
     */
    void buildSelectionFrustum(const QRect& rect,
                               const QSize& viewportSize,
                               const QMatrix4x4& viewMatrix,
                               const QMatrix4x4& projMatrix,
                               glm::vec4 planes[6]) const;
    
    /**
     * @brief Transform ray to mesh local space
     */
    static geometry::Ray transformRay(const geometry::Ray& ray,
                                      const glm::mat4& inverseTransform);
    
    /**
     * @brief Convert QMatrix4x4 to glm::mat4
     */
    static glm::mat4 toGlm(const QMatrix4x4& m);
    
    std::vector<PickableMesh> m_meshes;
};

/**
 * @brief Box selection helper - manages drag state
 */
class BoxSelector {
public:
    BoxSelector() = default;
    
    /**
     * @brief Start box selection at mouse position
     */
    void begin(const QPoint& pos);
    
    /**
     * @brief Update box selection (mouse move)
     */
    void update(const QPoint& pos);
    
    /**
     * @brief End box selection
     */
    void end();
    
    /**
     * @brief Cancel box selection
     */
    void cancel();
    
    /**
     * @brief Check if currently selecting
     */
    bool isActive() const { return m_active; }
    
    /**
     * @brief Get selection rectangle (normalized, positive width/height)
     */
    QRect rect() const;
    
    /**
     * @brief Get start point
     */
    QPoint startPoint() const { return m_startPos; }
    
    /**
     * @brief Get current end point
     */
    QPoint endPoint() const { return m_currentPos; }
    
    /**
     * @brief Check if selection is large enough to be valid
     * Small drags are treated as clicks
     */
    bool isValidSelection(int minSize = 5) const;

private:
    bool m_active = false;
    QPoint m_startPos;
    QPoint m_currentPos;
};

} // namespace renderer
} // namespace dc3d
