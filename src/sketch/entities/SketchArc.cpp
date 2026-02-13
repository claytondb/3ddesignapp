#include "SketchArc.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace dc {
namespace sketch {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

SketchArc::SketchArc(const glm::vec2& center, float radius, float startAngle, float endAngle)
    : SketchEntity(SketchEntityType::Arc)
    , m_center(center)
    , m_radius(radius)
    , m_startAngle(startAngle)
    , m_endAngle(endAngle)
{
    // FIX #11: Validate radius
    if (radius <= 0.0f) {
        throw std::invalid_argument("Arc radius must be positive");
    }
    
    // FIX #8: Determine CCW direction from angle difference
    float sweep = endAngle - startAngle;
    while (sweep > PI) sweep -= TWO_PI;
    while (sweep < -PI) sweep += TWO_PI;
    m_ccw = (sweep >= 0);
}

SketchArc::Ptr SketchArc::createFromThreePoints(const glm::vec2& start, const glm::vec2& mid, const glm::vec2& end) {
    // Find circumcenter of triangle formed by three points
    float ax = start.x, ay = start.y;
    float bx = mid.x, by = mid.y;
    float cx = end.x, cy = end.y;
    
    float d = 2.0f * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    if (std::abs(d) < 1e-10f) {
        // FIX #12: Throw exception instead of returning nullptr silently
        throw std::invalid_argument("Cannot create arc from collinear points");
    }
    
    float aSq = ax * ax + ay * ay;
    float bSq = bx * bx + by * by;
    float cSq = cx * cx + cy * cy;
    
    float centerX = (aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / d;
    float centerY = (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / d;
    
    glm::vec2 center(centerX, centerY);
    float radius = glm::length(start - center);
    
    float startAngle = std::atan2(start.y - centerY, start.x - centerX);
    float midAngle = std::atan2(mid.y - centerY, mid.x - centerX);
    float endAngle = std::atan2(end.y - centerY, end.x - centerX);
    
    // Determine direction based on mid point
    float sweep1 = midAngle - startAngle;
    float sweep2 = endAngle - midAngle;
    
    // Normalize
    while (sweep1 > PI) sweep1 -= TWO_PI;
    while (sweep1 < -PI) sweep1 += TWO_PI;
    while (sweep2 > PI) sweep2 -= TWO_PI;
    while (sweep2 < -PI) sweep2 += TWO_PI;
    
    // If both sweeps have same sign, mid is between start and end
    if (sweep1 * sweep2 > 0) {
        return std::make_shared<SketchArc>(center, radius, startAngle, endAngle);
    } else {
        // Mid is on the "long way around", swap direction
        return std::make_shared<SketchArc>(center, radius, startAngle, 
            endAngle + (sweep1 > 0 ? -TWO_PI : TWO_PI));
    }
}

SketchArc::Ptr SketchArc::createFromBulge(const glm::vec2& start, const glm::vec2& end, float bulge) {
    if (std::abs(bulge) < 1e-10f) {
        return nullptr;  // Straight line, not an arc
    }
    
    glm::vec2 chord = end - start;
    float chordLen = glm::length(chord);
    
    if (chordLen < 1e-10f) {
        return nullptr;
    }
    
    // Calculate arc properties from bulge
    float theta = 4.0f * std::atan(bulge);  // Included angle
    float radius = chordLen / (2.0f * std::sin(std::abs(theta) / 2.0f));
    
    // Find center
    glm::vec2 chordMid = (start + end) * 0.5f;
    glm::vec2 chordDir = chord / chordLen;
    glm::vec2 perpDir(-chordDir.y, chordDir.x);
    
    float sagitta = radius * (1.0f - std::cos(std::abs(theta) / 2.0f));
    float apothem = radius - sagitta;
    
    if (bulge > 0) {
        perpDir = -perpDir;
    }
    
    glm::vec2 center = chordMid + perpDir * apothem;
    
    float startAngle = std::atan2(start.y - center.y, start.x - center.x);
    float endAngle = std::atan2(end.y - center.y, end.x - center.x);
    
    return std::make_shared<SketchArc>(center, radius, startAngle, endAngle);
}

glm::vec2 SketchArc::startPoint() const {
    return m_center + m_radius * glm::vec2(std::cos(m_startAngle), std::sin(m_startAngle));
}

glm::vec2 SketchArc::endPoint() const {
    return m_center + m_radius * glm::vec2(std::cos(m_endAngle), std::sin(m_endAngle));
}

glm::vec2 SketchArc::midPoint() const {
    // FIX #9: Use sweepAngle() for correct midpoint on reflex arcs
    float midAngle = m_startAngle + sweepAngle() * 0.5f;
    return m_center + m_radius * glm::vec2(std::cos(midAngle), std::sin(midAngle));
}

float SketchArc::sweepAngle() const {
    // FIX #8: Use stored direction flag for correct reflex arc handling
    float sweep = m_endAngle - m_startAngle;
    
    // Normalize to [-2π, 2π] first
    while (sweep > TWO_PI) sweep -= TWO_PI;
    while (sweep < -TWO_PI) sweep += TWO_PI;
    
    // Adjust based on stored direction
    if (m_ccw && sweep < 0) {
        sweep += TWO_PI;
    } else if (!m_ccw && sweep > 0) {
        sweep -= TWO_PI;
    }
    
    return sweep;
}

bool SketchArc::isCCW() const {
    return sweepAngle() > 0;
}

void SketchArc::reverse() {
    std::swap(m_startAngle, m_endAngle);
    m_ccw = !m_ccw;  // FIX #8: Also reverse direction flag
}

float SketchArc::normalizeAngle(float angle) {
    while (angle > PI) angle -= TWO_PI;
    while (angle < -PI) angle += TWO_PI;
    return angle;
}

float SketchArc::angleAtParam(float t) const {
    return m_startAngle + t * sweepAngle();
}

glm::vec2 SketchArc::evaluate(float t) const {
    float angle = angleAtParam(t);
    return m_center + m_radius * glm::vec2(std::cos(angle), std::sin(angle));
}

glm::vec2 SketchArc::tangent(float t) const {
    float angle = angleAtParam(t);
    glm::vec2 radial(std::cos(angle), std::sin(angle));
    // Tangent is perpendicular to radial direction
    glm::vec2 tang(-radial.y, radial.x);
    if (!isCCW()) {
        tang = -tang;
    }
    return tang;
}

BoundingBox2D SketchArc::boundingBox() const {
    BoundingBox2D box;
    
    // Add endpoints
    box.expand(startPoint());
    box.expand(endPoint());
    
    // Check if arc crosses any cardinal directions
    float sweep = sweepAngle();
    float start = m_startAngle;
    
    // Check each cardinal angle (0, π/2, π, -π/2)
    float cardinals[] = {0, PI / 2, PI, -PI / 2};
    for (float cardinal : cardinals) {
        float diff = normalizeAngle(cardinal - start);
        if ((sweep > 0 && diff > 0 && diff < sweep) ||
            (sweep < 0 && diff < 0 && diff > sweep)) {
            box.expand(m_center + m_radius * glm::vec2(std::cos(cardinal), std::sin(cardinal)));
        }
    }
    
    return box;
}

float SketchArc::length() const {
    return std::abs(m_radius * sweepAngle());
}

float SketchArc::closestParameter(const glm::vec2& point) const {
    glm::vec2 toPoint = point - m_center;
    float pointAngle = std::atan2(toPoint.y, toPoint.x);
    
    float sweep = sweepAngle();
    float diff = normalizeAngle(pointAngle - m_startAngle);
    
    // Check if point angle is within arc range
    if ((sweep > 0 && diff >= 0 && diff <= sweep) ||
        (sweep < 0 && diff <= 0 && diff >= sweep)) {
        return diff / sweep;
    }
    
    // Point is outside arc range, return closest endpoint
    float distToStart = glm::length(point - startPoint());
    float distToEnd = glm::length(point - endPoint());
    return distToStart < distToEnd ? 0.0f : 1.0f;
}

SketchEntity::Ptr SketchArc::clone() const {
    auto copy = std::make_shared<SketchArc>(m_center, m_radius, m_startAngle, m_endAngle);
    copy->setConstruction(isConstruction());
    return copy;
}

std::vector<glm::vec2> SketchArc::tessellate(int numSamples) const {
    std::vector<glm::vec2> points;
    points.reserve(numSamples + 1);
    
    float sweep = sweepAngle();
    for (int i = 0; i <= numSamples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSamples);
        float angle = m_startAngle + t * sweep;
        points.emplace_back(m_center + m_radius * glm::vec2(std::cos(angle), std::sin(angle)));
    }
    
    return points;
}

SketchArc::Ptr SketchArc::create(const glm::vec2& center, float radius, float startAngle, float endAngle) {
    return std::make_shared<SketchArc>(center, radius, startAngle, endAngle);
}

} // namespace sketch
} // namespace dc
