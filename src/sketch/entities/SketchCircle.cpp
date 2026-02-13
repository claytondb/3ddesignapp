#include "SketchCircle.h"
#include <cmath>
#include <stdexcept>

namespace dc {
namespace sketch {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

SketchCircle::SketchCircle(const glm::vec2& center, float radius)
    : SketchEntity(SketchEntityType::Circle)
    , m_center(center)
    , m_radius(radius)
{
    // FIX #11: Validate radius
    if (radius <= 0.0f) {
        throw std::invalid_argument("Circle radius must be positive");
    }
}

SketchCircle::Ptr SketchCircle::createFromThreePoints(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
    // Find circumcenter
    float ax = p1.x, ay = p1.y;
    float bx = p2.x, by = p2.y;
    float cx = p3.x, cy = p3.y;
    
    float d = 2.0f * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    if (std::abs(d) < 1e-10f) {
        // FIX #12: Throw exception instead of returning nullptr silently
        throw std::invalid_argument("Cannot create circle from collinear points");
    }
    
    float aSq = ax * ax + ay * ay;
    float bSq = bx * bx + by * by;
    float cSq = cx * cx + cy * cy;
    
    float centerX = (aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / d;
    float centerY = (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / d;
    
    glm::vec2 center(centerX, centerY);
    float radius = glm::length(p1 - center);
    
    return std::make_shared<SketchCircle>(center, radius);
}

SketchCircle::Ptr SketchCircle::createFromCenterAndPoint(const glm::vec2& center, const glm::vec2& point) {
    float radius = glm::length(point - center);
    return std::make_shared<SketchCircle>(center, radius);
}

float SketchCircle::getArea() const {
    return PI * m_radius * m_radius;
}

float SketchCircle::getCircumference() const {
    return TWO_PI * m_radius;
}

glm::vec2 SketchCircle::pointAtAngle(float angle) const {
    return m_center + m_radius * glm::vec2(std::cos(angle), std::sin(angle));
}

bool SketchCircle::containsPoint(const glm::vec2& point) const {
    return glm::length(point - m_center) <= m_radius;
}

std::vector<float> SketchCircle::intersectLine(const glm::vec2& lineStart, const glm::vec2& lineEnd) const {
    std::vector<float> results;
    
    glm::vec2 d = lineEnd - lineStart;
    glm::vec2 f = lineStart - m_center;
    
    float a = glm::dot(d, d);
    float b = 2.0f * glm::dot(f, d);
    float c = glm::dot(f, f) - m_radius * m_radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0) {
        return results;  // No intersection
    }
    
    if (discriminant < 1e-10f) {
        // One intersection (tangent)
        float t = -b / (2.0f * a);
        if (t >= 0.0f && t <= 1.0f) {
            results.push_back(t);
        }
    } else {
        // Two intersections
        float sqrtDisc = std::sqrt(discriminant);
        float t1 = (-b - sqrtDisc) / (2.0f * a);
        float t2 = (-b + sqrtDisc) / (2.0f * a);
        
        if (t1 >= 0.0f && t1 <= 1.0f) {
            results.push_back(t1);
        }
        if (t2 >= 0.0f && t2 <= 1.0f) {
            results.push_back(t2);
        }
    }
    
    return results;
}

glm::vec2 SketchCircle::evaluate(float t) const {
    float angle = t * TWO_PI;
    return m_center + m_radius * glm::vec2(std::cos(angle), std::sin(angle));
}

glm::vec2 SketchCircle::tangent(float t) const {
    float angle = t * TWO_PI;
    return glm::vec2(-std::sin(angle), std::cos(angle));
}

BoundingBox2D SketchCircle::boundingBox() const {
    BoundingBox2D box;
    box.min = m_center - glm::vec2(m_radius);
    box.max = m_center + glm::vec2(m_radius);
    return box;
}

float SketchCircle::length() const {
    return getCircumference();
}

float SketchCircle::closestParameter(const glm::vec2& point) const {
    glm::vec2 toPoint = point - m_center;
    float angle = std::atan2(toPoint.y, toPoint.x);
    if (angle < 0) {
        angle += TWO_PI;
    }
    return angle / TWO_PI;
}

SketchEntity::Ptr SketchCircle::clone() const {
    auto copy = std::make_shared<SketchCircle>(m_center, m_radius);
    copy->setConstruction(isConstruction());
    return copy;
}

SketchCircle::Ptr SketchCircle::create(const glm::vec2& center, float radius) {
    return std::make_shared<SketchCircle>(center, radius);
}

} // namespace sketch
} // namespace dc
