#pragma once

#include "SketchEntity.h"

namespace dc {
namespace sketch {

/**
 * @brief A circular arc in 2D sketch space
 * 
 * The arc is defined by its center, radius, and start/end angles.
 * Angles are in radians, measured counter-clockwise from positive X axis.
 */
class SketchArc : public SketchEntity {
public:
    using Ptr = std::shared_ptr<SketchArc>;
    using ConstPtr = std::shared_ptr<const SketchArc>;
    
    /**
     * @brief Construct an arc from center, radius, and angles
     * @param center Center point
     * @param radius Arc radius
     * @param startAngle Start angle in radians
     * @param endAngle End angle in radians
     */
    SketchArc(const glm::vec2& center, float radius, float startAngle, float endAngle);
    
    /**
     * @brief Create arc from three points
     */
    static Ptr createFromThreePoints(const glm::vec2& start, const glm::vec2& mid, const glm::vec2& end);
    
    /**
     * @brief Create arc from start, end, and bulge factor
     * @param bulge Tangent of 1/4 the included angle (positive = CCW)
     */
    static Ptr createFromBulge(const glm::vec2& start, const glm::vec2& end, float bulge);
    
    // Getters
    const glm::vec2& getCenter() const { return m_center; }
    float getRadius() const { return m_radius; }
    float getStartAngle() const { return m_startAngle; }
    float getEndAngle() const { return m_endAngle; }
    
    // Setters
    void setCenter(const glm::vec2& center) { m_center = center; }
    void setRadius(float radius) { m_radius = radius; }
    void setStartAngle(float angle) { m_startAngle = angle; }
    void setEndAngle(float angle) { m_endAngle = angle; }
    
    /**
     * @brief Get the start point of the arc
     */
    glm::vec2 startPoint() const;
    
    /**
     * @brief Get the end point of the arc
     */
    glm::vec2 endPoint() const;
    
    /**
     * @brief Get the midpoint of the arc
     */
    glm::vec2 midPoint() const;
    
    /**
     * @brief Get the sweep angle (always positive for CCW, negative for CW)
     */
    float sweepAngle() const;
    
    /**
     * @brief Check if the arc goes counter-clockwise
     */
    bool isCCW() const;
    
    /**
     * @brief Reverse the direction of the arc
     */
    void reverse();
    
    // SketchEntity interface
    glm::vec2 evaluate(float t) const override;
    glm::vec2 tangent(float t) const override;
    BoundingBox2D boundingBox() const override;
    float length() const override;
    float closestParameter(const glm::vec2& point) const override;
    SketchEntity::Ptr clone() const override;
    std::vector<glm::vec2> tessellate(int numSamples = 32) const override;
    
    /**
     * @brief Create a new arc
     */
    static Ptr create(const glm::vec2& center, float radius, float startAngle, float endAngle);

private:
    glm::vec2 m_center;
    float m_radius;
    float m_startAngle;
    float m_endAngle;
    bool m_ccw = true;  // FIX #8: Store direction explicitly for reflex arcs
    
    /**
     * @brief Normalize angle to range [-π, π]
     */
    static float normalizeAngle(float angle);
    
    /**
     * @brief Get angle at parameter t
     */
    float angleAtParam(float t) const;
};

} // namespace sketch
} // namespace dc
