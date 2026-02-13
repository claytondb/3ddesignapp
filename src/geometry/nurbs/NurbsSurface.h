/**
 * @file NurbsSurface.h
 * @brief NURBS surface wrapper for compatibility
 * 
 * This is a compatibility header that wraps NURBSSurface for use in
 * freeform surface generation code.
 */

#pragma once

#include "../NURBSSurface.h"

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
    
private:
    dc3d::geometry::NURBSSurface m_surface;
};

} // namespace dc
