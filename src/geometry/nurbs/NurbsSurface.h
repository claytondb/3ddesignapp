/**
 * @file NurbsSurface.h
 * @brief NURBS surface wrapper for compatibility
 * 
 * This is a compatibility header that wraps NURBSSurface for use in
 * freeform surface generation code.
 */

#pragma once

#include "../NURBSSurface.h"
#include <limits>
#include <algorithm>

namespace dc {

/**
 * @brief NURBS surface class - wrapper around NURBSSurface for compatibility
 * 
 * Provides a compatibility layer for code that expects a NurbsSurface type.
 * Internally uses dc3d::geometry::NURBSSurface.
 */
class NurbsSurface {
public:
    NurbsSurface() = default;
    ~NurbsSurface() = default;
    
    NurbsSurface(const NurbsSurface&) = default;
    NurbsSurface& operator=(const NurbsSurface&) = default;
    NurbsSurface(NurbsSurface&&) = default;
    NurbsSurface& operator=(NurbsSurface&&) = default;
    
    /**
     * @brief Construct from NURBSSurface
     */
    explicit NurbsSurface(const dc3d::geometry::NURBSSurface& surface)
        : m_surface(surface) {}
    
    /**
     * @brief Get U degree
     */
    int degreeU() const { return m_surface.degreeU(); }
    
    /**
     * @brief Get V degree
     */
    int degreeV() const { return m_surface.degreeV(); }
    
    /**
     * @brief Get control point count in U direction
     */
    size_t controlPointCountU() const { return static_cast<size_t>(m_surface.numControlPointsU()); }
    
    /**
     * @brief Get control point count in V direction
     */
    size_t controlPointCountV() const { return static_cast<size_t>(m_surface.numControlPointsV()); }
    
    /**
     * @brief Get all control points
     */
    const std::vector<dc3d::geometry::ControlPoint>& getControlPoints() const { 
        return m_surface.controlPoints(); 
    }
    
    /**
     * @brief Get knot vector in U direction
     */
    const std::vector<float>& getKnotsU() const { return m_surface.knotsU(); }
    
    /**
     * @brief Get knot vector in V direction
     */
    const std::vector<float>& getKnotsV() const { return m_surface.knotsV(); }
    
    /**
     * @brief Get degree in U direction (alias for degreeU)
     */
    int getDegreeU() const { return m_surface.degreeU(); }
    
    /**
     * @brief Get degree in V direction (alias for degreeV)
     */
    int getDegreeV() const { return m_surface.degreeV(); }
    
    /**
     * @brief Evaluate surface at parameter
     */
    glm::vec3 evaluate(float u, float v) const { return m_surface.evaluate(u, v); }
    
    /**
     * @brief Construct from control points and knot vectors
     * @param controlPoints 2D vector of ControlPoint (row-major)
     * @param knotsU Knot vector in U direction
     * @param knotsV Knot vector in V direction
     * @param degreeU Degree in U direction
     * @param degreeV Degree in V direction
     */
    NurbsSurface(const std::vector<std::vector<dc3d::geometry::ControlPoint>>& controlPoints,
                 const std::vector<float>& knotsU,
                 const std::vector<float>& knotsV,
                 int degreeU,
                 int degreeV) {
        if (controlPoints.empty() || controlPoints[0].empty()) {
            return;
        }
        
        int numU = static_cast<int>(controlPoints.size());
        int numV = static_cast<int>(controlPoints[0].size());
        
        // Flatten 2D to 1D (row-major)
        std::vector<dc3d::geometry::ControlPoint> flatCPs;
        flatCPs.reserve(numU * numV);
        for (int j = 0; j < numV; ++j) {
            for (int i = 0; i < numU; ++i) {
                flatCPs.push_back(controlPoints[i][j]);
            }
        }
        
        m_surface.create(flatCPs, numU, numV, knotsU, knotsV, degreeU, degreeV);
    }
    
    /**
     * @brief Construct from glm::vec3 control points (with weight=1)
     */
    NurbsSurface(const std::vector<std::vector<glm::vec3>>& controlPoints,
                 const std::vector<float>& knotsU,
                 const std::vector<float>& knotsV,
                 int degreeU,
                 int degreeV) {
        if (controlPoints.empty() || controlPoints[0].empty()) {
            return;
        }
        
        int numU = static_cast<int>(controlPoints.size());
        int numV = static_cast<int>(controlPoints[0].size());
        
        // Flatten 2D to 1D (row-major), converting to ControlPoint
        std::vector<dc3d::geometry::ControlPoint> flatCPs;
        flatCPs.reserve(numU * numV);
        for (int j = 0; j < numV; ++j) {
            for (int i = 0; i < numU; ++i) {
                flatCPs.push_back(dc3d::geometry::ControlPoint{controlPoints[i][j], 1.0f});
            }
        }
        
        m_surface.create(flatCPs, numU, numV, knotsU, knotsV, degreeU, degreeV);
    }
    
    /**
     * @brief Get control points as 2D vector for compatibility
     * @return 2D vector of ControlPoint [numU][numV]
     */
    std::vector<std::vector<dc3d::geometry::ControlPoint>> getControlPoints2D() const {
        int numU = static_cast<int>(m_surface.numControlPointsU());
        int numV = static_cast<int>(m_surface.numControlPointsV());
        
        std::vector<std::vector<dc3d::geometry::ControlPoint>> result(numU);
        for (int i = 0; i < numU; ++i) {
            result[i].resize(numV);
            for (int j = 0; j < numV; ++j) {
                result[i][j] = m_surface.controlPoint(i, j);
            }
        }
        return result;
    }
    
    /**
     * @brief Get number of control points in U direction
     */
    int numControlPointsU() const { return m_surface.numControlPointsU(); }
    
    /**
     * @brief Get number of control points in V direction
     */
    int numControlPointsV() const { return m_surface.numControlPointsV(); }
    
    /**
     * @brief Get underlying NURBSSurface
     */
    dc3d::geometry::NURBSSurface& nurbsSurface() { return m_surface; }
    const dc3d::geometry::NURBSSurface& nurbsSurface() const { return m_surface; }
    
    /**
     * @brief Generate uniform knot vector (static utility)
     * @param numControlPoints Number of control points
     * @param degree Curve degree
     * @return Uniform knot vector
     */
    static std::vector<float> generateUniformKnots(int numControlPoints, int degree) {
        int numKnots = numControlPoints + degree + 1;
        std::vector<float> knots(numKnots);
        
        for (int i = 0; i <= degree; ++i) {
            knots[i] = 0.0f;
        }
        for (int i = degree + 1; i < numControlPoints; ++i) {
            knots[i] = static_cast<float>(i - degree) / (numControlPoints - degree);
        }
        for (int i = numControlPoints; i < numKnots; ++i) {
            knots[i] = 1.0f;
        }
        return knots;
    }
    
    /**
     * @brief Evaluate B-spline basis function (static utility)
     * @param i Control point index
     * @param p Degree
     * @param t Parameter value
     * @param knots Knot vector
     * @return Basis function value
     */
    static float basisFunction(int i, int p, float t, const std::vector<float>& knots) {
        if (p == 0) {
            return (t >= knots[i] && t < knots[i + 1]) ? 1.0f : 0.0f;
        }
        
        float left = 0.0f, right = 0.0f;
        
        float denom1 = knots[i + p] - knots[i];
        if (denom1 > 1e-10f) {
            left = ((t - knots[i]) / denom1) * basisFunction(i, p - 1, t, knots);
        }
        
        float denom2 = knots[i + p + 1] - knots[i + 1];
        if (denom2 > 1e-10f) {
            right = ((knots[i + p + 1] - t) / denom2) * basisFunction(i + 1, p - 1, t, knots);
        }
        
        return left + right;
    }
    
    /**
     * @brief Find closest surface parameter to a point (stub)
     * @param point Query point
     * @return UV parameters (simple approximation)
     */
    glm::vec2 findClosestParameter(const glm::vec3& point) const {
        // Simple grid search approximation
        float bestU = 0.5f, bestV = 0.5f;
        float bestDist = std::numeric_limits<float>::max();
        
        for (int i = 0; i <= 10; ++i) {
            for (int j = 0; j <= 10; ++j) {
                float u = static_cast<float>(i) / 10.0f;
                float v = static_cast<float>(j) / 10.0f;
                glm::vec3 sp = evaluate(u, v);
                float dist = glm::dot(sp - point, sp - point);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestU = u;
                    bestV = v;
                }
            }
        }
        return glm::vec2(bestU, bestV);
    }
    
    /**
     * @brief Evaluate surface derivative (stub)
     */
    glm::vec3 evaluateDerivative(float u, float v, int uOrder, int vOrder) const {
        (void)uOrder; (void)vOrder;
        // Finite difference approximation
        const float h = 0.001f;
        if (uOrder == 1 && vOrder == 0) {
            return (evaluate(std::min(u + h, 1.0f), v) - evaluate(std::max(u - h, 0.0f), v)) / (2.0f * h);
        } else if (uOrder == 0 && vOrder == 1) {
            return (evaluate(u, std::min(v + h, 1.0f)) - evaluate(u, std::max(v - h, 0.0f))) / (2.0f * h);
        }
        // Higher order: just return zero for now
        return glm::vec3(0.0f);
    }
    
    /**
     * @brief Insert knot in U direction (stub - returns copy)
     */
    void insertKnotU(float u) { (void)u; /* Not implemented */ }
    
    /**
     * @brief Insert knot in V direction (stub - returns copy)
     */
    void insertKnotV(float v) { (void)v; /* Not implemented */ }
    
private:
    dc3d::geometry::NURBSSurface m_surface;
};

} // namespace dc
