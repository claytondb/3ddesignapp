#pragma once

#include "SketchEntity.h"

namespace dc {
namespace sketch {

/**
 * @brief A circle in 2D sketch space
 */
class SketchCircle : public SketchEntity {
public:
    using Ptr = std::shared_ptr<SketchCircle>;
    using ConstPtr = std::shared_ptr<const SketchCircle>;
    
    /**
     * @brief Construct a circle from center and radius
     */
    SketchCircle(const glm::vec2& center, float radius);
    
    /**
     * @brief Create circle from three points
     */
    static Ptr createFromThreePoints(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);
    
    /**
     * @brief Create circle from center and a point on circumference
     */
    static Ptr createFromCenterAndPoint(const glm::vec2& center, const glm::vec2& point);
    
    // Getters
    const glm::vec2& getCenter() const { return m_center; }
    float getRadius() const { return m_radius; }
    float getDiameter() const { return 2.0f * m_radius; }
    float getArea() const;
    float getCircumference() const;
    
    // Setters
    void setCenter(const glm::vec2& center) { m_center = center; }
    void setRadius(float radius) { m_radius = radius; }
    
    /**
     * @brief Get a point on the circle at given angle
     * @param angle Angle in radians from positive X axis
     */
    glm::vec2 pointAtAngle(float angle) const;
    
    /**
     * @brief Check if a point is inside the circle
     */
    bool containsPoint(const glm::vec2& point) const;
    
    /**
     * @brief Find intersection points with a line
     * @param lineStart Start of line segment
     * @param lineEnd End of line segment
     * @return 0, 1, or 2 intersection parameters (along line)
     */
    std::vector<float> intersectLine(const glm::vec2& lineStart, const glm::vec2& lineEnd) const;
    
    // SketchEntity interface
    glm::vec2 evaluate(float t) const override;
    glm::vec2 tangent(float t) const override;
    BoundingBox2D boundingBox() const override;
    float length() const override;
    float closestParameter(const glm::vec2& point) const override;
    SketchEntity::Ptr clone() const override;
    
    /**
     * @brief Create a new circle
     */
    static Ptr create(const glm::vec2& center, float radius);

private:
    glm::vec2 m_center;
    float m_radius;
};

} // namespace sketch
} // namespace dc
