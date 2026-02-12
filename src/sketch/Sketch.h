#pragma once

#include "entities/SketchEntity.h"
#include "entities/SketchLine.h"
#include "entities/SketchArc.h"
#include "entities/SketchCircle.h"
#include "entities/SketchSpline.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace dc {
namespace sketch {

/**
 * @brief A plane in 3D space defined by origin and normal
 */
struct SketchPlane {
    glm::vec3 origin{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 0.0f, 1.0f};
    glm::vec3 xAxis{1.0f, 0.0f, 0.0f};   // Local X direction
    glm::vec3 yAxis{0.0f, 1.0f, 0.0f};   // Local Y direction
    
    /**
     * @brief Create a plane from origin and normal
     * X and Y axes are computed automatically
     */
    static SketchPlane fromOriginNormal(const glm::vec3& origin, const glm::vec3& normal);
    
    /**
     * @brief Create XY plane at given Z offset
     */
    static SketchPlane XY(float zOffset = 0.0f);
    
    /**
     * @brief Create XZ plane at given Y offset
     */
    static SketchPlane XZ(float yOffset = 0.0f);
    
    /**
     * @brief Create YZ plane at given X offset
     */
    static SketchPlane YZ(float xOffset = 0.0f);
    
    /**
     * @brief Transform a 2D point on the sketch to 3D world coordinates
     */
    glm::vec3 toWorld(const glm::vec2& local) const;
    
    /**
     * @brief Transform a 3D world point to 2D local coordinates
     * Assumes the point lies on the plane
     */
    glm::vec2 toLocal(const glm::vec3& world) const;
    
    /**
     * @brief Get the transformation matrix from local to world
     */
    glm::mat4 getTransformMatrix() const;
    
    /**
     * @brief Get the inverse transformation matrix (world to local)
     */
    glm::mat4 getInverseTransformMatrix() const;
    
    /**
     * @brief Check if a point lies on this plane (within tolerance)
     */
    bool containsPoint(const glm::vec3& point, float tolerance = 1e-5f) const;
    
    /**
     * @brief Get distance from a point to this plane
     */
    float distanceToPoint(const glm::vec3& point) const;
    
    /**
     * @brief Project a 3D point onto this plane
     */
    glm::vec3 projectPoint(const glm::vec3& point) const;
};

/**
 * @brief A 2D sketch containing geometric entities
 * 
 * Sketches live on a plane in 3D space and contain 2D geometry
 * that can be used to create 3D features through operations like
 * extrude, revolve, sweep, etc.
 */
class Sketch {
public:
    using Ptr = std::shared_ptr<Sketch>;
    using ConstPtr = std::shared_ptr<const Sketch>;
    
    /**
     * @brief Construct a sketch on the given plane
     */
    explicit Sketch(const SketchPlane& plane = SketchPlane::XY());
    
    ~Sketch() = default;
    
    // Non-copyable but movable
    Sketch(const Sketch&) = delete;
    Sketch& operator=(const Sketch&) = delete;
    Sketch(Sketch&&) = default;
    Sketch& operator=(Sketch&&) = default;
    
    /**
     * @brief Get the sketch ID
     */
    uint64_t getId() const { return m_id; }
    
    /**
     * @brief Get/set the sketch name
     */
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    /**
     * @brief Get the sketch plane
     */
    const SketchPlane& getPlane() const { return m_plane; }
    
    /**
     * @brief Set the sketch plane
     */
    void setPlane(const SketchPlane& plane) { m_plane = plane; }
    
    // ==================== Entity Management ====================
    
    /**
     * @brief Add an entity to the sketch
     * @return The entity's ID
     */
    uint64_t addEntity(SketchEntity::Ptr entity);
    
    /**
     * @brief Remove an entity by ID
     * @return true if entity was found and removed
     */
    bool removeEntity(uint64_t entityId);
    
    /**
     * @brief Get an entity by ID
     */
    SketchEntity::Ptr getEntity(uint64_t entityId);
    SketchEntity::ConstPtr getEntity(uint64_t entityId) const;
    
    /**
     * @brief Get all entities
     */
    const std::vector<SketchEntity::Ptr>& getEntities() const { return m_entities; }
    
    /**
     * @brief Get entities of a specific type
     */
    std::vector<SketchEntity::Ptr> getEntitiesByType(SketchEntityType type) const;
    
    /**
     * @brief Get number of entities
     */
    size_t getEntityCount() const { return m_entities.size(); }
    
    /**
     * @brief Clear all entities
     */
    void clearEntities();
    
    // ==================== Convenience Creation Methods ====================
    
    /**
     * @brief Add a line to the sketch
     */
    SketchLine::Ptr addLine(const glm::vec2& start, const glm::vec2& end);
    
    /**
     * @brief Add an arc to the sketch
     */
    SketchArc::Ptr addArc(const glm::vec2& center, float radius, float startAngle, float endAngle);
    
    /**
     * @brief Add a circle to the sketch
     */
    SketchCircle::Ptr addCircle(const glm::vec2& center, float radius);
    
    /**
     * @brief Add a spline to the sketch
     */
    SketchSpline::Ptr addSpline(const std::vector<glm::vec2>& controlPoints, int degree = 3);
    
    /**
     * @brief Add a rectangle (4 lines)
     * @return Vector of the 4 line entities
     */
    std::vector<SketchLine::Ptr> addRectangle(const glm::vec2& corner1, const glm::vec2& corner2);
    
    /**
     * @brief Add a regular polygon
     * @param center Center point
     * @param radius Distance from center to vertices
     * @param sides Number of sides
     * @param startAngle Angle to first vertex
     * @return Vector of line entities forming the polygon
     */
    std::vector<SketchLine::Ptr> addPolygon(const glm::vec2& center, float radius, 
                                             int sides, float startAngle = 0.0f);
    
    // ==================== Selection ====================
    
    /**
     * @brief Select an entity
     */
    void selectEntity(uint64_t entityId);
    
    /**
     * @brief Deselect an entity
     */
    void deselectEntity(uint64_t entityId);
    
    /**
     * @brief Toggle entity selection
     */
    void toggleEntitySelection(uint64_t entityId);
    
    /**
     * @brief Clear all selections
     */
    void clearSelection();
    
    /**
     * @brief Get all selected entities
     */
    std::vector<SketchEntity::Ptr> getSelectedEntities() const;
    
    // ==================== Geometry Queries ====================
    
    /**
     * @brief Get the bounding box of all entities (in local coordinates)
     */
    BoundingBox2D boundingBox() const;
    
    /**
     * @brief Find the entity closest to a point
     * @param point Query point in local coordinates
     * @param maxDistance Maximum distance to consider
     * @return Entity pointer or nullptr if none found within distance
     */
    SketchEntity::Ptr findNearestEntity(const glm::vec2& point, float maxDistance = 1e10f) const;
    
    /**
     * @brief Find all entities within a box
     */
    std::vector<SketchEntity::Ptr> findEntitiesInBox(const BoundingBox2D& box) const;
    
    /**
     * @brief Transform a point from world to sketch local coordinates
     */
    glm::vec2 worldToLocal(const glm::vec3& worldPoint) const;
    
    /**
     * @brief Transform a point from sketch local to world coordinates
     */
    glm::vec3 localToWorld(const glm::vec2& localPoint) const;
    
    /**
     * @brief Check if the sketch is empty
     */
    bool isEmpty() const { return m_entities.empty(); }
    
    /**
     * @brief Check if the sketch forms closed profiles
     */
    bool hasClosed() const;
    
    /**
     * @brief Get all closed loops in the sketch
     * Each loop is a vector of entity IDs forming a closed path
     */
    std::vector<std::vector<uint64_t>> findClosedLoops() const;
    
    /**
     * @brief Create a new sketch
     */
    static Ptr create(const SketchPlane& plane = SketchPlane::XY());

private:
    uint64_t m_id;
    std::string m_name;
    SketchPlane m_plane;
    std::vector<SketchEntity::Ptr> m_entities;
    std::unordered_map<uint64_t, size_t> m_entityIndex;  // ID to vector index
    
    static uint64_t s_nextId;
    
    /**
     * @brief Rebuild the entity index after modifications
     */
    void rebuildIndex();
};

} // namespace sketch
} // namespace dc
