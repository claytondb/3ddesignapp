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
    size_t controlPointCountU() const { return m_surface.controlPointCountU(); }
    
    /**
     * @brief Get control point count in V direction
     */
    size_t controlPointCountV() const { return m_surface.controlPointCountV(); }
    
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
     * @brief Get underlying NURBSSurface
     */
    dc3d::geometry::NURBSSurface& nurbsSurface() { return m_surface; }
    const dc3d::geometry::NURBSSurface& nurbsSurface() const { return m_surface; }
    
private:
    dc3d::geometry::NURBSSurface m_surface;
};

} // namespace dc
