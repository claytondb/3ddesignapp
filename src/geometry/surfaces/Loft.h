/**
 * @file Loft.h
 * @brief Lofting operation to create surfaces through multiple cross-sections
 * 
 * Supports:
 * - Multiple sections (open or closed profiles)
 * - Guide curves for path control
 * - Tangent control at ends
 * - Smooth (spline) vs ruled lofting
 */

#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../NURBSSurface.h"
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Tangent condition at loft ends
 */
enum class LoftTangentCondition {
    None,           ///< No tangent constraint
    Normal,         ///< Perpendicular to section plane
    Custom,         ///< User-specified direction
    TangentTo       ///< Tangent to adjacent surface
};

/**
 * @brief Options for loft operation
 */
struct LoftOptions {
    bool closed = false;                    ///< Create closed loft (connect last to first)
    bool ruled = false;                     ///< Use ruled (linear) interpolation between sections
    bool alignSections = true;              ///< Automatically align section start points
    bool reorientSections = true;           ///< Ensure consistent section orientation
    
    // Tangent conditions
    LoftTangentCondition startCondition = LoftTangentCondition::None;
    LoftTangentCondition endCondition = LoftTangentCondition::None;
    glm::vec3 startTangent{0.0f, 0.0f, 1.0f};   ///< Custom start tangent
    glm::vec3 endTangent{0.0f, 0.0f, 1.0f};     ///< Custom end tangent
    float startMagnitude = 1.0f;                 ///< Tangent influence at start
    float endMagnitude = 1.0f;                   ///< Tangent influence at end
    
    // Tessellation
    int sectionSegments = 32;               ///< Segments along each section
    int loftSegments = 16;                  ///< Segments between sections
    
    // Guide curves
    bool useGuides = false;                 ///< Use guide curves
};

/**
 * @brief Result of loft operation
 */
struct LoftResult {
    bool success = false;
    std::string errorMessage;
    
    std::vector<NURBSSurface> surfaces;     ///< NURBS surface representation
    MeshData mesh;                           ///< Tessellated mesh
};

/**
 * @brief Section (cross-section) for lofting
 */
class LoftSection {
public:
    LoftSection() = default;
    
    /**
     * @brief Create section from points
     * @param points Points defining the section contour
     * @param closed Whether the section is closed
     */
    explicit LoftSection(const std::vector<glm::vec3>& points, bool closed = true);
    
    /**
     * @brief Set section points
     */
    void setPoints(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Set whether section is closed
     */
    void setClosed(bool closed) { m_closed = closed; }
    
    /**
     * @brief Get section points
     */
    const std::vector<glm::vec3>& points() const { return m_points; }
    
    /**
     * @brief Check if section is closed
     */
    bool isClosed() const { return m_closed; }
    
    /**
     * @brief Check if section is valid
     */
    bool isValid() const { return m_points.size() >= 2; }
    
    /**
     * @brief Get centroid
     */
    glm::vec3 centroid() const;
    
    /**
     * @brief Get normal (for closed sections)
     */
    glm::vec3 normal() const;
    
    /**
     * @brief Get perimeter length
     */
    float perimeter() const;
    
    /**
     * @brief Resample to specific point count
     */
    LoftSection resampled(int numPoints) const;
    
    /**
     * @brief Reverse point order
     */
    void reverse();
    
    /**
     * @brief Rotate start point index
     */
    void rotateStartPoint(int offset);
    
    /**
     * @brief Find closest start point alignment to another section
     */
    int findBestAlignment(const LoftSection& other) const;

private:
    std::vector<glm::vec3> m_points;
    bool m_closed = true;
};

/**
 * @brief Guide curve for loft path control
 */
class LoftGuide {
public:
    LoftGuide() = default;
    
    /**
     * @brief Create guide from points
     */
    explicit LoftGuide(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Set guide points
     */
    void setPoints(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Get guide points
     */
    const std::vector<glm::vec3>& points() const { return m_points; }
    
    /**
     * @brief Evaluate point on guide at parameter t [0,1]
     */
    glm::vec3 evaluate(float t) const;
    
    /**
     * @brief Get tangent at parameter t
     */
    glm::vec3 tangent(float t) const;
    
    /**
     * @brief Check if guide is valid
     */
    bool isValid() const { return m_points.size() >= 2; }

private:
    std::vector<glm::vec3> m_points;
};

/**
 * @brief Lofting operations
 */
class Loft {
public:
    Loft() = default;
    
    /**
     * @brief Create lofted surface through sections
     * @param sections Cross-sections to loft through
     * @param options Loft options
     * @return Loft result with mesh
     */
    static LoftResult loft(const std::vector<LoftSection>& sections,
                           const LoftOptions& options = LoftOptions());
    
    /**
     * @brief Create lofted surface with guide curves
     * @param sections Cross-sections
     * @param guides Guide curves
     * @param options Loft options
     */
    static LoftResult loftWithGuides(const std::vector<LoftSection>& sections,
                                      const std::vector<LoftGuide>& guides,
                                      const LoftOptions& options = LoftOptions());
    
    /**
     * @brief Create ruled surface between two sections
     */
    static LoftResult ruledLoft(const LoftSection& section1,
                                 const LoftSection& section2,
                                 int segments = 16);
    
    /**
     * @brief Create loft from point arrays directly
     */
    static LoftResult loft(const std::vector<std::vector<glm::vec3>>& sectionPoints,
                           bool closedSections = true,
                           const LoftOptions& options = LoftOptions());
    
    /**
     * @brief Create NURBS surfaces for the loft
     */
    static std::vector<NURBSSurface> createSurfaces(const std::vector<LoftSection>& sections,
                                                     const LoftOptions& options);
    
    /**
     * @brief Blend between two surfaces
     * @param surface1 First surface
     * @param edge1 Edge parameter (0=uMin, 1=uMax, 2=vMin, 3=vMax)
     * @param surface2 Second surface
     * @param edge2 Edge parameter for second surface
     */
    static NURBSSurface blendSurfaces(const NURBSSurface& surface1, int edge1,
                                       const NURBSSurface& surface2, int edge2,
                                       float blendFactor = 0.5f);

private:
    // Prepare sections for lofting (resample, align, orient)
    static std::vector<LoftSection> prepareSections(const std::vector<LoftSection>& sections,
                                                     const LoftOptions& options);
    
    // Interpolate between sections
    static std::vector<glm::vec3> interpolateSections(const std::vector<LoftSection>& sections,
                                                       int pointIndex,
                                                       float t,
                                                       bool smooth);
    
    // Create mesh from prepared sections
    static MeshData createMesh(const std::vector<LoftSection>& sections,
                                const LoftOptions& options);
};

} // namespace geometry
} // namespace dc3d
