#pragma once

#include "SketchEntity.h"
#include <vector>

namespace dc {
namespace sketch {

/**
 * @brief A B-spline curve in 2D sketch space
 * 
 * This implements a cubic B-spline with uniform knot vector.
 * The curve passes through the first and last control points.
 */
class SketchSpline : public SketchEntity {
public:
    using Ptr = std::shared_ptr<SketchSpline>;
    using ConstPtr = std::shared_ptr<const SketchSpline>;
    
    /**
     * @brief Construct a spline from control points
     * @param controlPoints At least 2 control points required
     * @param degree Spline degree (default 3 for cubic)
     */
    explicit SketchSpline(const std::vector<glm::vec2>& controlPoints, int degree = 3);
    
    /**
     * @brief Create a spline that interpolates through given points
     * Uses chord-length parameterization
     */
    static Ptr createInterpolating(const std::vector<glm::vec2>& points, int degree = 3);
    
    // Getters
    const std::vector<glm::vec2>& getControlPoints() const { return m_controlPoints; }
    int getDegree() const { return m_degree; }
    int getNumControlPoints() const { return static_cast<int>(m_controlPoints.size()); }
    
    /**
     * @brief Get a specific control point
     */
    const glm::vec2& getControlPoint(int index) const;
    
    /**
     * @brief Set a specific control point
     */
    void setControlPoint(int index, const glm::vec2& point);
    
    /**
     * @brief Add a control point at the end
     */
    void addControlPoint(const glm::vec2& point);
    
    /**
     * @brief Insert a control point at index
     */
    void insertControlPoint(int index, const glm::vec2& point);
    
    /**
     * @brief Remove a control point
     */
    void removeControlPoint(int index);
    
    /**
     * @brief Get the start point of the spline
     */
    glm::vec2 startPoint() const;
    
    /**
     * @brief Get the end point of the spline
     */
    glm::vec2 endPoint() const;
    
    /**
     * @brief Get curvature at parameter t
     */
    float curvature(float t) const;
    
    // SketchEntity interface
    glm::vec2 evaluate(float t) const override;
    glm::vec2 tangent(float t) const override;
    BoundingBox2D boundingBox() const override;
    float length() const override;
    float closestParameter(const glm::vec2& point) const override;
    SketchEntity::Ptr clone() const override;
    std::vector<glm::vec2> tessellate(int numSamples = 64) const override;
    
    /**
     * @brief Create a new spline
     */
    static Ptr create(const std::vector<glm::vec2>& controlPoints, int degree = 3);

private:
    std::vector<glm::vec2> m_controlPoints;
    std::vector<float> m_knots;
    int m_degree;
    
    /**
     * @brief Generate uniform knot vector
     */
    void generateKnots();
    
    /**
     * @brief Evaluate B-spline basis function
     * @param i Index of basis function
     * @param p Degree
     * @param t Parameter value
     * @return Basis function value
     */
    float basisFunction(int i, int p, float t) const;
    
    /**
     * @brief Evaluate derivative of basis function
     */
    float basisDerivative(int i, int p, float t) const;
    
    /**
     * @brief Evaluate second derivative of basis function
     */
    float basisSecondDerivative(int i, int p, float t) const;
    
    /**
     * @brief Convert global parameter t to local knot span parameter
     */
    float globalToLocal(float t) const;
    
    /**
     * @brief Calculate first derivative at parameter t
     */
    glm::vec2 derivative(float t) const;
    
    /**
     * @brief Calculate second derivative at parameter t
     */
    glm::vec2 secondDerivative(float t) const;
};

} // namespace sketch
} // namespace dc
