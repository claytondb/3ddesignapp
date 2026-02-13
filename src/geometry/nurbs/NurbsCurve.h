/**
 * @file NurbsCurve.h
 * @brief NURBS curve representation for CAD modeling
 * 
 * Non-Uniform Rational B-Spline curve with:
 * - Control points with weights
 * - Knot vector
 * - Curve evaluation and tessellation
 */

#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

/**
 * @brief Weighted control point for NURBS curves
 */
struct CurveControlPoint {
    glm::vec3 position{0.0f};
    float weight = 1.0f;
    
    CurveControlPoint() = default;
    CurveControlPoint(const glm::vec3& pos, float w = 1.0f) : position(pos), weight(w) {}
};

/**
 * @brief NURBS curve representation
 */
class NurbsCurve {
public:
    NurbsCurve() = default;
    ~NurbsCurve() = default;
    
    // Move and copy
    NurbsCurve(NurbsCurve&&) noexcept = default;
    NurbsCurve& operator=(NurbsCurve&&) noexcept = default;
    NurbsCurve(const NurbsCurve&) = default;
    NurbsCurve& operator=(const NurbsCurve&) = default;
    
    /**
     * @brief Create a NURBS curve with specified parameters
     */
    bool create(const std::vector<CurveControlPoint>& controlPoints,
                const std::vector<float>& knots,
                int degree);
    
    /**
     * @brief Evaluate curve at parameter t
     * @param t Parameter value [0, 1]
     * @return Position on curve
     */
    glm::vec3 evaluate(float t) const;
    
    /**
     * @brief Get curve tangent at parameter t
     */
    glm::vec3 tangent(float t) const;
    
    /**
     * @brief Evaluate curve derivative at parameter t
     * @param t Parameter value [0, 1]
     * @param order Derivative order (1 = tangent, 2 = curvature direction)
     * @return Derivative vector
     */
    glm::vec3 evaluateDerivative(float t, int order) const {
        if (order == 1) {
            return tangent(t);
        }
        // Higher order: finite difference approximation
        const float h = 0.001f;
        glm::vec3 d1_plus = evaluateDerivative(std::min(t + h, 1.0f), order - 1);
        glm::vec3 d1_minus = evaluateDerivative(std::max(t - h, 0.0f), order - 1);
        return (d1_plus - d1_minus) / (2.0f * h);
    }
    
    /**
     * @brief Get curve degree
     */
    int degree() const { return m_degree; }
    
    /**
     * @brief Get number of control points
     */
    size_t controlPointCount() const { return m_controlPoints.size(); }
    
    /**
     * @brief Get control points
     */
    const std::vector<CurveControlPoint>& controlPoints() const { return m_controlPoints; }
    
    /**
     * @brief Get knot vector
     */
    const std::vector<float>& knots() const { return m_knots; }
    
    /**
     * @brief Check if curve is valid
     */
    bool isValid() const;
    
    /**
     * @brief Get parameter domain
     */
    void getDomain(float& tMin, float& tMax) const;
    
    /**
     * @brief Sample curve as polyline
     */
    std::vector<glm::vec3> sample(int numSamples) const;
    
private:
    std::vector<CurveControlPoint> m_controlPoints;
    std::vector<float> m_knots;
    int m_degree = 3;
    
    float basisFunction(int i, int p, float t) const;
    int findKnotSpan(float t) const;
};

} // namespace geometry
} // namespace dc3d

// Also provide in dc namespace for compatibility
namespace dc {
    using NurbsCurve = dc3d::geometry::NurbsCurve;
}
