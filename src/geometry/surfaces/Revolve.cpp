/**
 * @file Revolve.cpp
 * @brief Revolution surface implementation
 */

#include "Revolve.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace dc3d {
namespace geometry {

// ============================================================================
// RevolveProfile Implementation
// ============================================================================

RevolveProfile::RevolveProfile(const std::vector<glm::vec3>& points, bool closed)
    : m_points(points), m_closed(closed) {
}

void RevolveProfile::setPoints(const std::vector<glm::vec3>& points) {
    m_points = points;
}

float RevolveProfile::distanceToAxis(const RevolutionAxis& axis, size_t pointIndex) const {
    if (pointIndex >= m_points.size()) return 0.0f;
    
    const glm::vec3& p = m_points[pointIndex];
    
    // Project point onto axis
    glm::vec3 v = p - axis.origin;
    float t = glm::dot(v, axis.direction);
    glm::vec3 closestOnAxis = axis.origin + t * axis.direction;
    
    return glm::length(p - closestOnAxis);
}

bool RevolveProfile::intersectsAxis(const RevolutionAxis& axis) const {
    for (size_t i = 0; i < m_points.size(); ++i) {
        if (distanceToAxis(axis, i) < 1e-6f) {
            return true;
        }
    }
    return false;
}

BoundingBox RevolveProfile::boundingBox() const {
    BoundingBox bbox;
    for (const auto& p : m_points) {
        bbox.expand(p);
    }
    return bbox;
}

// ============================================================================
// Revolve Implementation
// ============================================================================

glm::vec3 Revolve::rotateAroundAxis(const glm::vec3& point,
                                     const RevolutionAxis& axis,
                                     float angleDeg) {
    glm::mat4 rot = getRotationMatrix(axis, angleDeg);
    glm::vec4 p = rot * glm::vec4(point, 1.0f);
    return glm::vec3(p);
}

glm::mat4 Revolve::getRotationMatrix(const RevolutionAxis& axis, float angleDeg) {
    float angleRad = glm::radians(angleDeg);
    
    // Translate to origin, rotate, translate back
    glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -axis.origin);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleRad, axis.direction);
    glm::mat4 fromOrigin = glm::translate(glm::mat4(1.0f), axis.origin);
    
    return fromOrigin * rotation * toOrigin;
}

RevolveResult Revolve::revolve(const RevolveProfile& profile,
                                const RevolutionAxis& axis,
                                float angle) {
    RevolveOptions options;
    options.axis = axis;
    options.endAngle = angle;
    return revolve(profile, options);
}

RevolveResult Revolve::revolve(const RevolveProfile& profile,
                                const RevolveOptions& options) {
    RevolveResult result;
    
    if (!profile.isValid()) {
        result.errorMessage = "Invalid profile: must have at least 2 points";
        return result;
    }
    
    float sweepAngle = options.sweepAngle();
    if (std::abs(sweepAngle) < 0.1f) {
        result.errorMessage = "Revolution angle too small";
        return result;
    }
    
    const auto& profilePoints = profile.points();
    int numProfile = static_cast<int>(profilePoints.size());
    int numCircum = options.circumferentialSegments;
    
    // For full revolution, we don't need an extra column
    bool fullRevolution = options.isFullRevolution();
    int numCols = fullRevolution ? numCircum : numCircum + 1;
    
    MeshData& mesh = result.mesh;
    
    // Generate vertices by rotating profile points
    float angleStep = sweepAngle / numCircum;
    
    for (int j = 0; j < numCols; ++j) {
        float angle = options.startAngle + j * angleStep;
        glm::mat4 rot = getRotationMatrix(options.axis, angle);
        
        for (int i = 0; i < numProfile; ++i) {
            glm::vec4 p = rot * glm::vec4(profilePoints[i], 1.0f);
            mesh.addVertex(glm::vec3(p));
        }
    }
    
    // Generate faces
    for (int j = 0; j < numCircum; ++j) {
        int nextJ = (j + 1) % numCols;
        
        for (int i = 0; i < numProfile - 1; ++i) {
            uint32_t v00 = j * numProfile + i;
            uint32_t v10 = j * numProfile + i + 1;
            uint32_t v01 = nextJ * numProfile + i;
            uint32_t v11 = nextJ * numProfile + i + 1;
            
            // Check for degenerate quads (when profile point is on axis)
            float dist0 = profile.distanceToAxis(options.axis, i);
            float dist1 = profile.distanceToAxis(options.axis, i + 1);
            
            if (dist0 < 1e-6f) {
                // Point i is on axis, create single triangle
                if (dist1 > 1e-6f) {
                    mesh.addFace(v00, v10, v11);
                }
            } else if (dist1 < 1e-6f) {
                // Point i+1 is on axis, create single triangle
                mesh.addFace(v00, v10, v01);
            } else {
                // Normal quad - split into two triangles
                mesh.addFace(v00, v10, v11);
                mesh.addFace(v00, v11, v01);
            }
        }
        
        // If profile is closed, connect last to first
        if (profile.isClosed()) {
            uint32_t v00 = j * numProfile + numProfile - 1;
            uint32_t v10 = j * numProfile;
            uint32_t v01 = nextJ * numProfile + numProfile - 1;
            uint32_t v11 = nextJ * numProfile;
            
            mesh.addFace(v00, v10, v11);
            mesh.addFace(v00, v11, v01);
        }
    }
    
    // Create caps for partial revolution
    if (!fullRevolution && options.capEnds && !profile.isClosed()) {
        // Start cap
        MeshData startCap = createCapMesh(profile, options.axis, options.startAngle, true);
        uint32_t startBase = static_cast<uint32_t>(mesh.vertexCount());
        for (size_t i = 0; i < startCap.vertexCount(); ++i) {
            mesh.addVertex(startCap.vertices()[i]);
        }
        for (size_t i = 0; i < startCap.faceCount(); ++i) {
            uint32_t i0 = startCap.indices()[i * 3];
            uint32_t i1 = startCap.indices()[i * 3 + 1];
            uint32_t i2 = startCap.indices()[i * 3 + 2];
            mesh.addFace(startBase + i0, startBase + i1, startBase + i2);
            result.capStartFaces.push_back(static_cast<uint32_t>(mesh.faceCount() - 1));
        }
        
        // End cap
        MeshData endCap = createCapMesh(profile, options.axis, options.endAngle, false);
        uint32_t endBase = static_cast<uint32_t>(mesh.vertexCount());
        for (size_t i = 0; i < endCap.vertexCount(); ++i) {
            mesh.addVertex(endCap.vertices()[i]);
        }
        for (size_t i = 0; i < endCap.faceCount(); ++i) {
            uint32_t i0 = endCap.indices()[i * 3];
            uint32_t i1 = endCap.indices()[i * 3 + 1];
            uint32_t i2 = endCap.indices()[i * 3 + 2];
            mesh.addFace(endBase + i0, endBase + i1, endBase + i2);
            result.capEndFaces.push_back(static_cast<uint32_t>(mesh.faceCount() - 1));
        }
    }
    
    mesh.computeNormals();
    result.success = true;
    return result;
}

RevolveResult Revolve::revolve(const std::vector<glm::vec3>& profilePoints,
                                const RevolutionAxis& axis,
                                float angle,
                                bool closedProfile) {
    RevolveProfile profile(profilePoints, closedProfile);
    return revolve(profile, axis, angle);
}

std::vector<NURBSSurface> Revolve::createSurfaces(const RevolveProfile& profile,
                                                   const RevolveOptions& options) {
    std::vector<NURBSSurface> surfaces;
    
    if (!profile.isValid()) return surfaces;
    
    const auto& pts = profile.points();
    
    // For each edge of the profile, create a surface of revolution
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        // Create a NURBS surface for this ruled segment
        // A surface of revolution can be represented exactly with rational NURBS
        // using circular arc control points
        
        glm::vec3 p0 = pts[i];
        glm::vec3 p1 = pts[i + 1];
        
        // For simplicity, create a degree 2 surface (quadratic in V for circular arcs)
        // This is a simplified version - exact circles need weight adjustment
        
        float sweepAngle = options.sweepAngle();
        int numArcs = static_cast<int>(std::ceil(std::abs(sweepAngle) / 90.0f));
        
        int numU = 2;  // Linear along profile edge
        int numV = numArcs * 2 + 1;  // Quadratic NURBS for arc
        
        std::vector<ControlPoint> cps(numU * numV);
        
        float arcAngle = sweepAngle / numArcs;
        float halfArcRad = glm::radians(arcAngle / 2.0f);
        float w = std::cos(halfArcRad);  // Weight for middle control points
        
        for (int arc = 0; arc < numArcs; ++arc) {
            float startAngle = options.startAngle + arc * arcAngle;
            float midAngle = startAngle + arcAngle / 2.0f;
            float endAngle = startAngle + arcAngle;
            
            int baseIdx = arc * 2;
            
            for (int row = 0; row < numU; ++row) {
                glm::vec3 basePoint = (row == 0) ? p0 : p1;
                
                // Start point of arc
                glm::vec3 ps = rotateAroundAxis(basePoint, options.axis, startAngle);
                cps[row * numV + baseIdx] = ControlPoint(ps, 1.0f);
                
                // Middle control point (off the surface, weighted)
                glm::vec3 pm = rotateAroundAxis(basePoint, options.axis, midAngle);
                // Adjust for rational B-spline
                float dist = profile.distanceToAxis(options.axis, (row == 0) ? i : i + 1);
                glm::vec3 pmAdjusted = pm;  // Simplified - exact requires projection
                cps[row * numV + baseIdx + 1] = ControlPoint(pmAdjusted, w);
                
                // End point of arc (only for last arc)
                if (arc == numArcs - 1) {
                    glm::vec3 pe = rotateAroundAxis(basePoint, options.axis, endAngle);
                    cps[row * numV + baseIdx + 2] = ControlPoint(pe, 1.0f);
                }
            }
        }
        
        // Create knot vectors
        std::vector<float> knotsU = {0.0f, 0.0f, 1.0f, 1.0f};  // Degree 1
        
        std::vector<float> knotsV;
        knotsV.push_back(0.0f);
        knotsV.push_back(0.0f);
        knotsV.push_back(0.0f);
        for (int arc = 1; arc < numArcs; ++arc) {
            float t = static_cast<float>(arc) / numArcs;
            knotsV.push_back(t);
            knotsV.push_back(t);
        }
        knotsV.push_back(1.0f);
        knotsV.push_back(1.0f);
        knotsV.push_back(1.0f);
        
        NURBSSurface surface;
        surface.create(cps, numU, numV, knotsU, knotsV, 1, 2);
        surfaces.push_back(surface);
    }
    
    return surfaces;
}

MeshData Revolve::createRevolutionMesh(const RevolveProfile& profile,
                                        const RevolveOptions& options) {
    RevolveOptions opts = options;
    opts.capEnds = false;
    return revolve(profile, opts).mesh;
}

MeshData Revolve::createCapMesh(const RevolveProfile& profile,
                                 const RevolutionAxis& axis,
                                 float angle,
                                 bool flipNormals) {
    MeshData mesh;
    
    const auto& pts = profile.points();
    if (pts.size() < 2) return mesh;
    
    // Rotate profile to the cap position
    glm::mat4 rot = getRotationMatrix(axis, angle);
    
    std::vector<glm::vec3> rotatedPts;
    for (const auto& p : pts) {
        glm::vec4 rp = rot * glm::vec4(p, 1.0f);
        rotatedPts.push_back(glm::vec3(rp));
    }
    
    // Find point on axis (for fan triangulation)
    glm::vec3 axisPoint;
    bool hasAxisPoint = false;
    
    for (size_t i = 0; i < pts.size(); ++i) {
        if (profile.distanceToAxis(axis, i) < 1e-6f) {
            glm::vec4 ap = rot * glm::vec4(pts[i], 1.0f);
            axisPoint = glm::vec3(ap);
            hasAxisPoint = true;
            break;
        }
    }
    
    // If no point on axis, use projected centroid
    if (!hasAxisPoint) {
        glm::vec3 centroid(0.0f);
        for (const auto& p : rotatedPts) {
            centroid += p;
        }
        centroid /= static_cast<float>(rotatedPts.size());
        
        // Project to axis
        glm::vec3 v = centroid - axis.origin;
        float t = glm::dot(v, axis.direction);
        axisPoint = axis.origin + t * axis.direction;
    }
    
    // Add vertices
    uint32_t centerIdx = mesh.addVertex(axisPoint);
    for (const auto& p : rotatedPts) {
        mesh.addVertex(p);
    }
    
    // Create fan triangles
    for (size_t i = 0; i < rotatedPts.size() - 1; ++i) {
        uint32_t v1 = static_cast<uint32_t>(i + 1);
        uint32_t v2 = static_cast<uint32_t>(i + 2);
        
        if (flipNormals) {
            mesh.addFace(centerIdx, v2, v1);
        } else {
            mesh.addFace(centerIdx, v1, v2);
        }
    }
    
    return mesh;
}

MeshData Revolve::createTorus(float majorRadius, float minorRadius,
                               const RevolutionAxis& axis,
                               int segments) {
    // Create a circular profile
    std::vector<glm::vec3> profile;
    
    // Position the profile circle at majorRadius from axis
    glm::vec3 center = axis.origin + glm::vec3(majorRadius, 0, 0);
    
    // Find a perpendicular to the axis in the profile plane
    glm::vec3 radialDir(1, 0, 0);
    if (std::abs(glm::dot(radialDir, axis.direction)) > 0.9f) {
        radialDir = glm::vec3(0, 1, 0);
    }
    radialDir = glm::normalize(radialDir - glm::dot(radialDir, axis.direction) * axis.direction);
    glm::vec3 profileNormal = glm::cross(axis.direction, radialDir);
    
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        glm::vec3 p = center + minorRadius * (std::cos(angle) * radialDir + 
                                               std::sin(angle) * axis.direction);
        profile.push_back(p);
    }
    
    RevolveProfile prof(profile, true);
    RevolveOptions opts;
    opts.axis = axis;
    opts.circumferentialSegments = segments;
    
    return revolve(prof, opts).mesh;
}

MeshData Revolve::createCone(float baseRadius, float height,
                              const RevolutionAxis& axis,
                              int segments) {
    // Profile is a line from apex to base edge
    glm::vec3 apex = axis.origin + axis.direction * height;
    glm::vec3 basePoint = axis.origin;
    
    // Find perpendicular direction
    glm::vec3 perp(1, 0, 0);
    if (std::abs(glm::dot(perp, axis.direction)) > 0.9f) {
        perp = glm::vec3(0, 1, 0);
    }
    perp = glm::normalize(perp - glm::dot(perp, axis.direction) * axis.direction);
    
    glm::vec3 baseEdge = basePoint + perp * baseRadius;
    
    std::vector<glm::vec3> profile = {apex, baseEdge, basePoint};
    RevolveProfile prof(profile, true);
    
    RevolveOptions opts;
    opts.axis = axis;
    opts.circumferentialSegments = segments;
    opts.capEnds = true;
    
    return revolve(prof, opts).mesh;
}

MeshData Revolve::createSphere(float radius,
                                const glm::vec3& center,
                                int segments) {
    // Profile is a semicircle
    std::vector<glm::vec3> profile;
    
    for (int i = 0; i <= segments / 2; ++i) {
        float angle = 3.14159265359f * i / (segments / 2);
        glm::vec3 p = center + radius * glm::vec3(std::sin(angle), std::cos(angle), 0.0f);
        profile.push_back(p);
    }
    
    RevolveProfile prof(profile, false);
    RevolveOptions opts;
    opts.axis = RevolutionAxis::yAxis(center);
    opts.circumferentialSegments = segments;
    opts.capEnds = false;  // Sphere is closed by profile endpoints on axis
    
    return revolve(prof, opts).mesh;
}

} // namespace geometry
} // namespace dc3d
