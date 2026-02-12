#pragma once

#include "SketchEntity.h"

namespace dc {
namespace sketch {

/**
 * @brief A line segment in 2D sketch space
 */
class SketchLine : public SketchEntity {
public:
    using Ptr = std::shared_ptr<SketchLine>;
    using ConstPtr = std::shared_ptr<const SketchLine>;
    
    /**
     * @brief Construct a line from two points
     */
    SketchLine(const glm::vec2& start, const glm::vec2& end);
    
    /**
     * @brief Get the start point
     */
    const glm::vec2& getStart() const { return m_start; }
    
    /**
     * @brief Get the end point
     */
    const glm::vec2& getEnd() const { return m_end; }
    
    /**
     * @brief Set the start point
     */
    void setStart(const glm::vec2& start) { m_start = start; }
    
    /**
     * @brief Set the end point
     */
    void setEnd(const glm::vec2& end) { m_end = end; }
    
    /**
     * @brief Get the midpoint of the line
     */
    glm::vec2 midpoint() const;
    
    /**
     * @brief Get the angle of the line in radians
     * @return Angle from positive X axis, in range [-π, π]
     */
    float angle() const;
    
    /**
     * @brief Get the direction vector (normalized)
     */
    glm::vec2 direction() const;
    
    /**
     * @brief Check if a point lies on this line segment
     * @param point Point to test
     * @param tolerance Distance tolerance
     */
    bool containsPoint(const glm::vec2& point, float tolerance = 1e-5f) const;
    
    // SketchEntity interface
    glm::vec2 evaluate(float t) const override;
    glm::vec2 tangent(float t) const override;
    BoundingBox2D boundingBox() const override;
    float length() const override;
    float closestParameter(const glm::vec2& point) const override;
    Ptr clone() const override;
    
    /**
     * @brief Create a new line
     */
    static Ptr create(const glm::vec2& start, const glm::vec2& end);

private:
    glm::vec2 m_start;
    glm::vec2 m_end;
};

} // namespace sketch
} // namespace dc
