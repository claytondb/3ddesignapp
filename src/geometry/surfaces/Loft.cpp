/**
 * @file Loft.cpp
 * @brief Lofting implementation
 */

#include "Loft.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace dc3d {
namespace geometry {

// ============================================================================
// LoftSection Implementation
// ============================================================================

LoftSection::LoftSection(const std::vector<glm::vec3>& points, bool closed)
    : m_points(points), m_closed(closed) {
}

void LoftSection::setPoints(const std::vector<glm::vec3>& points) {
    m_points = points;
}

glm::vec3 LoftSection::centroid() const {
    if (m_points.empty()) return glm::vec3(0.0f);
    
    glm::vec3 c(0.0f);
    for (const auto& p : m_points) {
        c += p;
    }
    return c / static_cast<float>(m_points.size());
}

glm::vec3 LoftSection::normal() const {
    if (m_points.size() < 3) return glm::vec3(0, 0, 1);
    
    // Use Newell's method
    glm::vec3 n(0.0f);
    for (size_t i = 0; i < m_points.size(); ++i) {
        const glm::vec3& curr = m_points[i];
        const glm::vec3& next = m_points[(i + 1) % m_points.size()];
        n.x += (curr.y - next.y) * (curr.z + next.z);
        n.y += (curr.z - next.z) * (curr.x + next.x);
        n.z += (curr.x - next.x) * (curr.y + next.y);
    }
    
    float len = glm::length(n);
    return (len > 1e-10f) ? n / len : glm::vec3(0, 0, 1);
}

float LoftSection::perimeter() const {
    if (m_points.size() < 2) return 0.0f;
    
    float len = 0.0f;
    for (size_t i = 0; i < m_points.size() - 1; ++i) {
        len += glm::length(m_points[i + 1] - m_points[i]);
    }
    if (m_closed) {
        len += glm::length(m_points.front() - m_points.back());
    }
    return len;
}

LoftSection LoftSection::resampled(int numPoints) const {
    if (m_points.size() < 2 || numPoints < 2) {
        return *this;
    }
    
    // Calculate total length and cumulative lengths
    std::vector<float> cumLen;
    cumLen.push_back(0.0f);
    
    for (size_t i = 0; i < m_points.size() - 1; ++i) {
        float segLen = glm::length(m_points[i + 1] - m_points[i]);
        cumLen.push_back(cumLen.back() + segLen);
    }
    if (m_closed) {
        float segLen = glm::length(m_points.front() - m_points.back());
        cumLen.push_back(cumLen.back() + segLen);
    }
    
    float totalLen = cumLen.back();
    if (totalLen < 1e-10f) return *this;
    
    // Resample
    std::vector<glm::vec3> newPoints;
    int effectiveNum = m_closed ? numPoints : numPoints;
    
    for (int i = 0; i < effectiveNum; ++i) {
        float targetLen = (m_closed ? i : i) * totalLen / (m_closed ? numPoints : numPoints - 1);
        
        // Find segment
        size_t seg = 0;
        while (seg < cumLen.size() - 1 && cumLen[seg + 1] < targetLen) {
            ++seg;
        }
        
        // Interpolate within segment
        float segStart = cumLen[seg];
        float segEnd = (seg + 1 < cumLen.size()) ? cumLen[seg + 1] : totalLen;
        float t = (segEnd > segStart) ? (targetLen - segStart) / (segEnd - segStart) : 0.0f;
        
        size_t idx0 = seg % m_points.size();
        size_t idx1 = (seg + 1) % m_points.size();
        
        newPoints.push_back(m_points[idx0] * (1.0f - t) + m_points[idx1] * t);
    }
    
    return LoftSection(newPoints, m_closed);
}

void LoftSection::reverse() {
    std::reverse(m_points.begin(), m_points.end());
}

void LoftSection::rotateStartPoint(int offset) {
    if (m_points.empty() || !m_closed) return;
    
    offset = offset % static_cast<int>(m_points.size());
    if (offset < 0) offset += m_points.size();
    
    std::rotate(m_points.begin(), m_points.begin() + offset, m_points.end());
}

int LoftSection::findBestAlignment(const LoftSection& other) const {
    if (m_points.empty() || other.m_points.empty()) return 0;
    
    int bestOffset = 0;
    float minDist = std::numeric_limits<float>::max();
    
    int n = std::min(m_points.size(), other.m_points.size());
    
    for (size_t offset = 0; offset < m_points.size(); ++offset) {
        float totalDist = 0.0f;
        for (int i = 0; i < n; ++i) {
            size_t idx = (i + offset) % m_points.size();
            size_t otherIdx = i % other.m_points.size();
            totalDist += glm::length(m_points[idx] - other.m_points[otherIdx]);
        }
        
        if (totalDist < minDist) {
            minDist = totalDist;
            bestOffset = static_cast<int>(offset);
        }
    }
    
    return bestOffset;
}

// ============================================================================
// LoftGuide Implementation
// ============================================================================

LoftGuide::LoftGuide(const std::vector<glm::vec3>& points)
    : m_points(points) {
}

void LoftGuide::setPoints(const std::vector<glm::vec3>& points) {
    m_points = points;
}

glm::vec3 LoftGuide::evaluate(float t) const {
    if (m_points.empty()) return glm::vec3(0.0f);
    if (m_points.size() == 1) return m_points[0];
    
    t = std::clamp(t, 0.0f, 1.0f);
    
    float fIdx = t * (m_points.size() - 1);
    int idx = static_cast<int>(fIdx);
    float frac = fIdx - idx;
    
    if (idx >= static_cast<int>(m_points.size()) - 1) {
        return m_points.back();
    }
    
    return m_points[idx] * (1.0f - frac) + m_points[idx + 1] * frac;
}

glm::vec3 LoftGuide::tangent(float t) const {
    if (m_points.size() < 2) return glm::vec3(0, 0, 1);
    
    const float dt = 0.01f;
    glm::vec3 p0 = evaluate(std::max(0.0f, t - dt));
    glm::vec3 p1 = evaluate(std::min(1.0f, t + dt));
    
    glm::vec3 tan = p1 - p0;
    float len = glm::length(tan);
    return (len > 1e-10f) ? tan / len : glm::vec3(0, 0, 1);
}

// ============================================================================
// Loft Implementation
// ============================================================================

std::vector<LoftSection> Loft::prepareSections(const std::vector<LoftSection>& sections,
                                                const LoftOptions& options) {
    if (sections.size() < 2) return sections;
    
    // Find the maximum point count
    int maxPoints = 0;
    for (const auto& s : sections) {
        maxPoints = std::max(maxPoints, static_cast<int>(s.points().size()));
    }
    maxPoints = std::max(maxPoints, options.sectionSegments);
    
    // Resample all sections to same point count
    std::vector<LoftSection> prepared;
    for (const auto& s : sections) {
        prepared.push_back(s.resampled(maxPoints));
    }
    
    // Align start points
    if (options.alignSections && !prepared.empty()) {
        for (size_t i = 1; i < prepared.size(); ++i) {
            int offset = prepared[i].findBestAlignment(prepared[i - 1]);
            prepared[i].rotateStartPoint(offset);
        }
    }
    
    // Ensure consistent orientation
    if (options.reorientSections && prepared.size() >= 2) {
        for (size_t i = 1; i < prepared.size(); ++i) {
            // Check if reversing would reduce total distance
            float normalDist = 0.0f;
            float reversedDist = 0.0f;
            
            const auto& prev = prepared[i - 1].points();
            const auto& curr = prepared[i].points();
            
            for (size_t j = 0; j < curr.size(); ++j) {
                normalDist += glm::length(curr[j] - prev[j]);
                reversedDist += glm::length(curr[curr.size() - 1 - j] - prev[j]);
            }
            
            if (reversedDist < normalDist * 0.9f) {
                prepared[i].reverse();
            }
        }
    }
    
    return prepared;
}

LoftResult Loft::loft(const std::vector<LoftSection>& sections,
                      const LoftOptions& options) {
    LoftResult result;
    
    if (sections.size() < 2) {
        result.errorMessage = "Need at least 2 sections for lofting";
        return result;
    }
    
    // Validate sections
    for (size_t i = 0; i < sections.size(); ++i) {
        if (!sections[i].isValid()) {
            result.errorMessage = "Invalid section at index " + std::to_string(i);
            return result;
        }
    }
    
    // Prepare sections
    std::vector<LoftSection> prepared = prepareSections(sections, options);
    
    // Create mesh
    result.mesh = createMesh(prepared, options);
    result.success = true;
    
    return result;
}

LoftResult Loft::loftWithGuides(const std::vector<LoftSection>& sections,
                                 const std::vector<LoftGuide>& guides,
                                 const LoftOptions& options) {
    // For now, ignore guides and do regular loft
    // Full implementation would deform sections to follow guides
    LoftOptions opts = options;
    opts.useGuides = !guides.empty();
    return loft(sections, opts);
}

LoftResult Loft::ruledLoft(const LoftSection& section1,
                            const LoftSection& section2,
                            int segments) {
    LoftOptions options;
    options.ruled = true;
    options.loftSegments = segments;
    
    return loft({section1, section2}, options);
}

LoftResult Loft::loft(const std::vector<std::vector<glm::vec3>>& sectionPoints,
                      bool closedSections,
                      const LoftOptions& options) {
    std::vector<LoftSection> sections;
    for (const auto& pts : sectionPoints) {
        sections.emplace_back(pts, closedSections);
    }
    return loft(sections, options);
}

MeshData Loft::createMesh(const std::vector<LoftSection>& sections,
                           const LoftOptions& options) {
    MeshData mesh;
    
    if (sections.size() < 2) return mesh;
    
    int numSections = static_cast<int>(sections.size());
    int numPoints = static_cast<int>(sections[0].points().size());
    bool closedSection = sections[0].isClosed();
    
    int loftSteps = options.loftSegments;
    int totalRows = (numSections - 1) * loftSteps + 1;
    
    // Generate vertices
    for (int row = 0; row < totalRows; ++row) {
        float globalT = static_cast<float>(row) / (totalRows - 1);
        
        // Find which sections to interpolate between
        float sectionF = globalT * (numSections - 1);
        int sectionIdx = static_cast<int>(sectionF);
        float localT = sectionF - sectionIdx;
        
        if (sectionIdx >= numSections - 1) {
            sectionIdx = numSections - 2;
            localT = 1.0f;
        }
        
        const auto& s0 = sections[sectionIdx].points();
        const auto& s1 = sections[sectionIdx + 1].points();
        
        for (int col = 0; col < numPoints; ++col) {
            glm::vec3 p0 = s0[col % s0.size()];
            glm::vec3 p1 = s1[col % s1.size()];
            
            glm::vec3 p;
            if (options.ruled) {
                // Linear interpolation
                p = p0 * (1.0f - localT) + p1 * localT;
            } else {
                // Smooth interpolation (Hermite or Catmull-Rom could be used)
                // Using simple smooth-step for now
                float smoothT = localT * localT * (3.0f - 2.0f * localT);
                p = p0 * (1.0f - smoothT) + p1 * smoothT;
            }
            
            mesh.addVertex(p);
        }
    }
    
    // Generate faces
    for (int row = 0; row < totalRows - 1; ++row) {
        int effectiveCols = closedSection ? numPoints : numPoints - 1;
        
        for (int col = 0; col < effectiveCols; ++col) {
            int nextCol = (col + 1) % numPoints;
            
            uint32_t v00 = row * numPoints + col;
            uint32_t v10 = row * numPoints + nextCol;
            uint32_t v01 = (row + 1) * numPoints + col;
            uint32_t v11 = (row + 1) * numPoints + nextCol;
            
            mesh.addFace(v00, v10, v11);
            mesh.addFace(v00, v11, v01);
        }
    }
    
    // Handle closed loft
    if (options.closed && sections.size() >= 3) {
        // Connect last row to first row
        int lastRow = totalRows - 1;
        int effectiveCols = closedSection ? numPoints : numPoints - 1;
        
        for (int col = 0; col < effectiveCols; ++col) {
            int nextCol = (col + 1) % numPoints;
            
            uint32_t v00 = lastRow * numPoints + col;
            uint32_t v10 = lastRow * numPoints + nextCol;
            uint32_t v01 = col;  // First row
            uint32_t v11 = nextCol;
            
            mesh.addFace(v00, v10, v11);
            mesh.addFace(v00, v11, v01);
        }
    }
    
    mesh.computeNormals();
    return mesh;
}

std::vector<NURBSSurface> Loft::createSurfaces(const std::vector<LoftSection>& sections,
                                                const LoftOptions& options) {
    std::vector<NURBSSurface> surfaces;
    
    if (sections.size() < 2) return surfaces;
    
    std::vector<LoftSection> prepared = prepareSections(sections, options);
    
    int numU = static_cast<int>(prepared[0].points().size());
    int numV = static_cast<int>(prepared.size());
    
    // Create control points grid
    std::vector<ControlPoint> cps(numU * numV);
    
    for (int j = 0; j < numV; ++j) {
        const auto& pts = prepared[j].points();
        for (int i = 0; i < numU; ++i) {
            cps[j * numU + i] = ControlPoint(pts[i % pts.size()]);
        }
    }
    
    // Create knot vectors
    int degreeU = std::min(3, numU - 1);
    int degreeV = std::min(3, numV - 1);
    
    std::vector<float> knotsU(numU + degreeU + 1);
    std::vector<float> knotsV(numV + degreeV + 1);
    
    // Clamped uniform knot vectors
    for (int i = 0; i <= degreeU; ++i) knotsU[i] = 0.0f;
    for (int i = degreeU + 1; i < numU; ++i) {
        knotsU[i] = static_cast<float>(i - degreeU) / (numU - degreeU);
    }
    for (int i = numU; i < static_cast<int>(knotsU.size()); ++i) knotsU[i] = 1.0f;
    
    for (int i = 0; i <= degreeV; ++i) knotsV[i] = 0.0f;
    for (int i = degreeV + 1; i < numV; ++i) {
        knotsV[i] = static_cast<float>(i - degreeV) / (numV - degreeV);
    }
    for (int i = numV; i < static_cast<int>(knotsV.size()); ++i) knotsV[i] = 1.0f;
    
    NURBSSurface surface;
    surface.create(cps, numU, numV, knotsU, knotsV, degreeU, degreeV);
    surfaces.push_back(surface);
    
    return surfaces;
}

NURBSSurface Loft::blendSurfaces(const NURBSSurface& surface1, int edge1,
                                  const NURBSSurface& surface2, int edge2,
                                  float blendFactor) {
    // Extract boundary curves and create a blend surface
    // This is a simplified implementation
    
    std::vector<glm::vec3> curve1, curve2;
    std::vector<glm::vec3> dummy1, dummy2;
    
    surface1.getBoundaries(curve1, dummy1, dummy2, dummy1, 20);
    surface2.getBoundaries(curve2, dummy1, dummy2, dummy1, 20);
    
    // Select appropriate boundary based on edge parameter
    // 0=uMin, 1=uMax, 2=vMin, 3=vMax
    
    // Create ruled surface between the two curves
    LoftSection s1(curve1, false);
    LoftSection s2(curve2, false);
    
    auto result = ruledLoft(s1, s2, 8);
    
    NURBSSurface blendSurf;
    auto surfaces = createSurfaces({s1, s2}, LoftOptions());
    if (!surfaces.empty()) {
        blendSurf = surfaces[0];
    }
    
    return blendSurf;
}

} // namespace geometry
} // namespace dc3d
