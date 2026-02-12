#include "SketchPoint.h"

namespace dc {

SketchPoint::SketchPoint(const glm::dvec2& position)
    : SketchEntity(SketchEntityType::Point)
    , m_position(position)
{
}

glm::dvec2 SketchPoint::evaluate(double /*t*/) const {
    return m_position;
}

glm::dvec2 SketchPoint::getStartPoint() const {
    return m_position;
}

glm::dvec2 SketchPoint::getEndPoint() const {
    return m_position;
}

glm::dvec2 SketchPoint::getTangent(double /*t*/) const {
    // Points have no direction, return zero vector
    return glm::dvec2(0.0);
}

int SketchPoint::getDegreesOfFreedom() const {
    return 2; // X and Y position
}

void SketchPoint::getVariables(std::vector<double>& vars) const {
    vars.push_back(m_position.x);
    vars.push_back(m_position.y);
}

int SketchPoint::setVariables(const std::vector<double>& vars, int offset) {
    if (offset + 2 <= static_cast<int>(vars.size())) {
        m_position.x = vars[offset];
        m_position.y = vars[offset + 1];
    }
    return 2;
}

std::unique_ptr<SketchEntity> SketchPoint::clone() const {
    auto copy = std::make_unique<SketchPoint>(m_position);
    copy->setConstruction(isConstruction());
    copy->setLocked(isLocked());
    return copy;
}

double SketchPoint::getLength() const {
    return 0.0;
}

void SketchPoint::getBoundingBox(glm::dvec2& minPt, glm::dvec2& maxPt) const {
    minPt = m_position;
    maxPt = m_position;
}

} // namespace dc
