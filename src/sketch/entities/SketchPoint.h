#pragma once

#include "SketchEntity.h"

namespace dc {
namespace sketch {

/**
 * @brief A reference point entity in a sketch
 * 
 * Points are used for construction geometry and as constraint references.
 * They have zero length and evaluate to the same position for all parameters.
 */
class SketchPoint : public SketchEntity {
public:
    using Ptr = std::shared_ptr<SketchPoint>;
    
    /**
     * @brief Construct a point at the given position
     * @param position 2D position in sketch coordinates
     */
    explicit SketchPoint(const glm::vec2& position = glm::vec2(0.0f));

    /**
     * @brief Get the point position
     */
    const glm::vec2& getPosition() const { return m_position; }

    /**
     * @brief Set the point position
     */
    void setPosition(const glm::vec2& position) { m_position = position; }

    // SketchEntity interface
    glm::vec2 evaluate(float t) const override;
    glm::vec2 tangent(float t) const override;
    BoundingBox2D boundingBox() const override;
    float length() const override;

private:
    glm::vec2 m_position;
};

} // namespace sketch
} // namespace dc
