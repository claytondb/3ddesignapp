#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace dc {
namespace sketch {

/**
 * @brief Types of sketch entities
 */
enum class SketchEntityType {
    Point,
    Line,
    Arc,
    Circle,
    Spline,
    Ellipse
};

/**
 * @brief Axis-aligned bounding box in 2D
 */
struct BoundingBox2D {
    glm::vec2 min{std::numeric_limits<float>::max()};
    glm::vec2 max{std::numeric_limits<float>::lowest()};
    
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y;
    }
    
    void expand(const glm::vec2& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    
    void expand(const BoundingBox2D& other) {
        if (other.isValid()) {
            min = glm::min(min, other.min);
            max = glm::max(max, other.max);
        }
    }
    
    glm::vec2 center() const {
        return (min + max) * 0.5f;
    }
    
    glm::vec2 size() const {
        return max - min;
    }
};

/**
 * @brief Base class for all sketch entities
 * 
 * Sketch entities are 2D geometric primitives that live in the
 * local coordinate system of a sketch plane.
 */
class SketchEntity {
public:
    using Ptr = std::shared_ptr<SketchEntity>;
    using ConstPtr = std::shared_ptr<const SketchEntity>;
    
    explicit SketchEntity(SketchEntityType type);
    virtual ~SketchEntity() = default;
    
    // Non-copyable but movable
    SketchEntity(const SketchEntity&) = delete;
    SketchEntity& operator=(const SketchEntity&) = delete;
    SketchEntity(SketchEntity&&) = default;
    SketchEntity& operator=(SketchEntity&&) = default;
    
    /**
     * @brief Get the unique ID of this entity
     */
    uint64_t getId() const { return m_id; }
    
    /**
     * @brief Get the type of this entity
     */
    SketchEntityType getType() const { return m_type; }
    
    /**
     * @brief Check if this is a construction entity
     * Construction entities are used as references but don't
     * contribute to the final profile.
     */
    bool isConstruction() const { return m_isConstruction; }
    
    /**
     * @brief Set construction mode
     */
    void setConstruction(bool construction) { m_isConstruction = construction; }
    
    /**
     * @brief Check if this entity is selected
     */
    bool isSelected() const { return m_isSelected; }
    
    /**
     * @brief Set selection state
     */
    void setSelected(bool selected) { m_isSelected = selected; }
    
    /**
     * @brief Evaluate the entity at parameter t
     * @param t Parameter value, typically in [0, 1]
     * @return Point on the entity at parameter t
     */
    virtual glm::vec2 evaluate(float t) const = 0;
    
    /**
     * @brief Get the tangent direction at parameter t
     * @param t Parameter value
     * @return Normalized tangent vector
     */
    virtual glm::vec2 tangent(float t) const = 0;
    
    /**
     * @brief Compute the bounding box of this entity
     */
    virtual BoundingBox2D boundingBox() const = 0;
    
    /**
     * @brief Get the arc length of this entity
     */
    virtual float length() const = 0;
    
    /**
     * @brief Sample points along the entity for rendering
     * @param numSamples Number of points to generate
     * @return Vector of points along the entity
     */
    virtual std::vector<glm::vec2> tessellate(int numSamples = 32) const;
    
    /**
     * @brief Find the closest point on the entity to a given point
     * @param point Query point
     * @return Parameter t of the closest point
     */
    virtual float closestParameter(const glm::vec2& point) const = 0;
    
    /**
     * @brief Clone this entity
     */
    virtual Ptr clone() const = 0;

protected:
    uint64_t m_id;
    SketchEntityType m_type;
    bool m_isConstruction = false;
    bool m_isSelected = false;
    
private:
    static uint64_t s_nextId;
};

} // namespace sketch
} // namespace dc
