#include "SketchPoint.h"

namespace dc {
namespace sketch {

SketchPoint::SketchPoint(const glm::vec2& position)
    : SketchEntity(SketchEntityType::Point)
    , m_position(position)
{
}

glm::vec2 SketchPoint::evaluate(float /*t*/) const {
    return m_position;
}

glm::vec2 SketchPoint::tangent(float /*t*/) const {
    // Points have no direction, return zero vector
    return glm::vec2(0.0f);
}

BoundingBox2D SketchPoint::boundingBox() const {
    BoundingBox2D box;
    box.expand(m_position);
    return box;
}

float SketchPoint::length() const {
    return 0.0f;
}

} // namespace sketch
} // namespace dc
