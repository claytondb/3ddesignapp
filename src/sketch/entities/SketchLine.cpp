#include "SketchLine.h"
#include <cmath>
#include <algorithm>

namespace dc {
namespace sketch {

SketchLine::SketchLine(const glm::vec2& start, const glm::vec2& end)
    : SketchEntity(SketchEntityType::Line)
    , m_start(start)
    , m_end(end)
{
}

glm::vec2 SketchLine::midpoint() const {
    return (m_start + m_end) * 0.5f;
}

float SketchLine::angle() const {
    glm::vec2 dir = m_end - m_start;
    return std::atan2(dir.y, dir.x);
}

glm::vec2 SketchLine::direction() const {
    glm::vec2 dir = m_end - m_start;
    float len = glm::length(dir);
    if (len < 1e-10f) {
        return glm::vec2(1.0f, 0.0f);
    }
    return dir / len;
}

bool SketchLine::containsPoint(const glm::vec2& point, float tolerance) const {
    glm::vec2 lineVec = m_end - m_start;
    glm::vec2 pointVec = point - m_start;
    
    float lineLen = glm::length(lineVec);
    if (lineLen < 1e-10f) {
        return glm::length(pointVec) <= tolerance;
    }
    
    // Project point onto line
    float t = glm::dot(pointVec, lineVec) / (lineLen * lineLen);
    
    // Check if projection is within segment
    if (t < -tolerance / lineLen || t > 1.0f + tolerance / lineLen) {
        return false;
    }
    
    // Check distance from line
    glm::vec2 projected = m_start + t * lineVec;
    return glm::length(point - projected) <= tolerance;
}

glm::vec2 SketchLine::evaluate(float t) const {
    return m_start + t * (m_end - m_start);
}

glm::vec2 SketchLine::tangent(float /*t*/) const {
    return direction();
}

BoundingBox2D SketchLine::boundingBox() const {
    BoundingBox2D box;
    box.expand(m_start);
    box.expand(m_end);
    return box;
}

float SketchLine::length() const {
    return glm::length(m_end - m_start);
}

float SketchLine::closestParameter(const glm::vec2& point) const {
    glm::vec2 lineVec = m_end - m_start;
    float lenSq = glm::dot(lineVec, lineVec);
    
    if (lenSq < 1e-10f) {
        return 0.0f;
    }
    
    float t = glm::dot(point - m_start, lineVec) / lenSq;
    return std::clamp(t, 0.0f, 1.0f);
}

SketchEntity::Ptr SketchLine::clone() const {
    auto copy = std::make_shared<SketchLine>(m_start, m_end);
    copy->setConstruction(isConstruction());
    return copy;
}

SketchLine::Ptr SketchLine::create(const glm::vec2& start, const glm::vec2& end) {
    return std::make_shared<SketchLine>(start, end);
}

} // namespace sketch
} // namespace dc
