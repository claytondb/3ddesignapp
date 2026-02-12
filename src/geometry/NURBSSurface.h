/**
 * @file NURBSSurface.h
 * @brief NURBS surface representation for CAD modeling
 * 
 * Non-Uniform Rational B-Spline surface with:
 * - Control point grid with weights
 * - Knot vectors in U and V directions
 * - Surface evaluation and tessellation
 */

#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Weighted control point for NURBS
 */
struct ControlPoint {
    glm::vec3 position{0.0f};
    float weight = 1.0f;
    
    ControlPoint() = default;
    ControlPoint(const glm::vec3& pos, float w = 1.0f) : position(pos), weight(w) {}
    
    /// Get homogeneous coordinates (x*w, y*w, z*w, w)
    glm::vec4 homogeneous() const {
        return glm::vec4(position * weight, weight);
    }
    
    /// Create from homogeneous coordinates
    static ControlPoint fromHomogeneous(const glm::vec4& h) {
        if (std::abs(h.w) < 1e-10f) {
            return ControlPoint(glm::vec3(h), 0.0f);
        }
        return ControlPoint(glm::vec3(h) / h.w, h.w);
    }
};

/**
 * @brief Surface derivative information at a point
 */
struct SurfacePoint {
    glm::vec3 position;      ///< Point on surface
    glm::vec3 normal;        ///< Surface normal
    glm::vec3 tangentU;      ///< Tangent in U direction
    glm::vec3 tangentV;      ///< Tangent in V direction
    float u, v;              ///< Parameter values
};

/**
 * @brief Tessellation options for surface conversion to mesh
 */
struct TessellationOptions {
    int uDivisions = 32;        ///< Divisions in U direction
    int vDivisions = 32;        ///< Divisions in V direction
    bool adaptive = false;      ///< Use adaptive tessellation based on curvature
    float adaptiveTolerance = 0.01f; ///< Chord height tolerance for adaptive
    bool computeNormals = true; ///< Compute vertex normals
    bool computeUVs = true;     ///< Compute texture coordinates
};

/**
 * @brief NURBS surface representation
 * 
 * A NURBS surface is defined by:
 * - Control point grid (m+1 x n+1 points)
 * - Knot vectors in U (size m+p+2) and V (size n+q+2) directions
 * - Degrees p (U) and q (V)
 */
class NURBSSurface {
public:
    NURBSSurface() = default;
    ~NURBSSurface() = default;
    
    // Move and copy
    NURBSSurface(NURBSSurface&&) noexcept = default;
    NURBSSurface& operator=(NURBSSurface&&) noexcept = default;
    NURBSSurface(const NURBSSurface&) = default;
    NURBSSurface& operator=(const NURBSSurface&) = default;
    
    // ==================
    // Construction
    // ==================
    
    /**
     * @brief Create a NURBS surface with specified parameters
     * @param controlPoints Control point grid (row-major, numU x numV)
     * @param numU Number of control points in U direction
     * @param numV Number of control points in V direction
     * @param knotsU Knot vector in U direction
     * @param knotsV Knot vector in V direction
     * @param degreeU Degree in U direction
     * @param degreeV Degree in V direction
     */
    bool create(const std::vector<ControlPoint>& controlPoints,
                int numU, int numV,
                const std::vector<float>& knotsU,
                const std::vector<float>& knotsV,
                int degreeU, int degreeV);
    
    /**
     * @brief Create a Bezier surface (special case of NURBS)
     */
    bool createBezier(const std::vector<ControlPoint>& controlPoints,
                      int numU, int numV);
    
    /**
     * @brief Create a bilinear surface from 4 corner points
     */
    static NURBSSurface createBilinear(const glm::vec3& p00, const glm::vec3& p10,
                                        const glm::vec3& p01, const glm::vec3& p11);
    
    /**
     * @brief Create a planar surface from boundary curves
     */
    static NURBSSurface createPlanar(const std::vector<glm::vec3>& boundary);
    
    // ==================
    // Evaluation
    // ==================
    
    /**
     * @brief Evaluate surface point at parameters (u, v)
     * @param u Parameter in U direction [0, 1]
     * @param v Parameter in V direction [0, 1]
     * @return Position on surface
     */
    glm::vec3 evaluate(float u, float v) const;
    
    /**
     * @brief Evaluate surface with derivatives
     * @return Full surface point info with position, normal, tangents
     */
    SurfacePoint evaluateWithDerivatives(float u, float v) const;
    
    /**
     * @brief Get surface normal at parameters
     */
    glm::vec3 normal(float u, float v) const;
    
    /**
     * @brief Get partial derivative with respect to U
     */
    glm::vec3 derivativeU(float u, float v) const;
    
    /**
     * @brief Get partial derivative with respect to V
     */
    glm::vec3 derivativeV(float u, float v) const;
    
    // ==================
    // Tessellation
    // ==================
    
    /**
     * @brief Convert surface to triangle mesh
     */
    MeshData tessellate(const TessellationOptions& options = TessellationOptions()) const;
    
    /**
     * @brief Tessellate with specified divisions
     */
    MeshData tessellate(int uDivs, int vDivs) const;
    
    /**
     * @brief Sample points on surface grid
     */
    std::vector<glm::vec3> sampleGrid(int uSamples, int vSamples) const;
    
    // ==================
    // Queries
    // ==================
    
    /// Check if surface is valid
    bool isValid() const;
    
    /// Get number of control points in U direction
    int numControlPointsU() const { return m_numU; }
    
    /// Get number of control points in V direction
    int numControlPointsV() const { return m_numV; }
    
    /// Get degree in U direction
    int degreeU() const { return m_degreeU; }
    
    /// Get degree in V direction
    int degreeV() const { return m_degreeV; }
    
    /// Get parameter domain
    void getDomain(float& uMin, float& uMax, float& vMin, float& vMax) const;
    
    /// Get surface area (approximation via tessellation)
    float surfaceArea(int samples = 50) const;
    
    /// Get bounding box
    BoundingBox boundingBox() const;
    
    // ==================
    // Control Point Access
    // ==================
    
    /// Get control point at (i, j)
    const ControlPoint& controlPoint(int i, int j) const;
    ControlPoint& controlPoint(int i, int j);
    
    /// Get all control points
    const std::vector<ControlPoint>& controlPoints() const { return m_controlPoints; }
    std::vector<ControlPoint>& controlPoints() { return m_controlPoints; }
    
    /// Get knot vectors
    const std::vector<float>& knotsU() const { return m_knotsU; }
    const std::vector<float>& knotsV() const { return m_knotsV; }
    
    // ==================
    // Modification
    // ==================
    
    /// Set control point position
    void setControlPoint(int i, int j, const glm::vec3& pos);
    
    /// Set control point weight
    void setWeight(int i, int j, float weight);
    
    /// Transform the surface
    void transform(const glm::mat4& matrix);
    
    /// Reverse surface direction (U or V)
    void reverseU();
    void reverseV();
    
    // ==================
    // Subdivision
    // ==================
    
    /// Insert knot in U direction
    void insertKnotU(float u);
    
    /// Insert knot in V direction
    void insertKnotV(float v);
    
    /// Refine by adding knots
    void refine(int uInsertions, int vInsertions);
    
    // ==================
    // Extraction
    // ==================
    
    /// Extract isocurve at constant U
    std::vector<glm::vec3> isocurveU(float u, int samples = 50) const;
    
    /// Extract isocurve at constant V
    std::vector<glm::vec3> isocurveV(float v, int samples = 50) const;
    
    /// Extract boundary curves
    void getBoundaries(std::vector<glm::vec3>& uMin, std::vector<glm::vec3>& uMax,
                       std::vector<glm::vec3>& vMin, std::vector<glm::vec3>& vMax,
                       int samples = 50) const;

private:
    std::vector<ControlPoint> m_controlPoints;
    std::vector<float> m_knotsU;
    std::vector<float> m_knotsV;
    int m_numU = 0;
    int m_numV = 0;
    int m_degreeU = 3;
    int m_degreeV = 3;
    
    // B-spline basis function helpers
    float basisFunction(int i, int p, float t, const std::vector<float>& knots) const;
    float basisFunctionDerivative(int i, int p, float t, const std::vector<float>& knots) const;
    int findKnotSpan(int n, int p, float t, const std::vector<float>& knots) const;
    
    // Internal index conversion
    int index(int i, int j) const { return j * m_numU + i; }
};

} // namespace geometry
} // namespace dc3d
