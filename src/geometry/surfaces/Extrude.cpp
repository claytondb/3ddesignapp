/**
 * @file Extrude.cpp
 * @brief Linear extrusion implementation
 */

#include "Extrude.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace dc3d {
namespace geometry {

// ============================================================================
// ExtrudeProfile Implementation
// ============================================================================

ExtrudeProfile::ExtrudeProfile(const std::vector<glm::vec3>& points) {
    setOuterBoundary(points);
}

void ExtrudeProfile::setOuterBoundary(const std::vector<glm::vec3>& points) {
    m_outer = points;
    
    if (points.size() >= 3) {
        // Compute plane from first 3 non-collinear points
        m_planeOrigin = points[0];
        
        // Find normal using Newell's method for robustness
        m_planeNormal = glm::vec3(0.0f);
        for (size_t i = 0; i < points.size(); ++i) {
            const glm::vec3& curr = points[i];
            const glm::vec3& next = points[(i + 1) % points.size()];
            m_planeNormal.x += (curr.y - next.y) * (curr.z + next.z);
            m_planeNormal.y += (curr.z - next.z) * (curr.x + next.x);
            m_planeNormal.z += (curr.x - next.x) * (curr.y + next.y);
        }
        
        float len = glm::length(m_planeNormal);
        if (len > 1e-10f) {
            m_planeNormal /= len;
        } else {
            m_planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }
}

void ExtrudeProfile::addHole(const std::vector<glm::vec3>& points) {
    if (points.size() >= 3) {
        m_holes.push_back(points);
    }
}

void ExtrudeProfile::setPlane(const glm::vec3& origin, const glm::vec3& normal) {
    m_planeOrigin = origin;
    m_planeNormal = glm::normalize(normal);
}

bool ExtrudeProfile::isValid() const {
    return m_outer.size() >= 3;
}

float ExtrudeProfile::signedArea() const {
    if (m_outer.size() < 3) return 0.0f;
    
    // Project to 2D in the plane
    glm::vec3 u = glm::normalize(glm::cross(m_planeNormal, 
        std::abs(m_planeNormal.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(m_planeNormal, u);
    
    float area = 0.0f;
    for (size_t i = 0; i < m_outer.size(); ++i) {
        const glm::vec3& p0 = m_outer[i] - m_planeOrigin;
        const glm::vec3& p1 = m_outer[(i + 1) % m_outer.size()] - m_planeOrigin;
        
        float x0 = glm::dot(p0, u);
        float y0 = glm::dot(p0, v);
        float x1 = glm::dot(p1, u);
        float y1 = glm::dot(p1, v);
        
        area += (x0 * y1 - x1 * y0);
    }
    
    return area * 0.5f;
}

void ExtrudeProfile::ensureCorrectWinding() {
    // Outer boundary should be CCW (positive area)
    if (signedArea() < 0) {
        std::reverse(m_outer.begin(), m_outer.end());
    }
    
    // Holes should be CW (negative area when computed independently)
    // But we just reverse them from the outer direction
    for (auto& hole : m_holes) {
        std::reverse(hole.begin(), hole.end());
    }
}

glm::vec3 ExtrudeProfile::centroid() const {
    if (m_outer.empty()) return glm::vec3(0.0f);
    
    glm::vec3 c(0.0f);
    for (const auto& p : m_outer) {
        c += p;
    }
    return c / static_cast<float>(m_outer.size());
}

BoundingBox ExtrudeProfile::boundingBox() const {
    BoundingBox bbox;
    for (const auto& p : m_outer) {
        bbox.expand(p);
    }
    for (const auto& hole : m_holes) {
        for (const auto& p : hole) {
            bbox.expand(p);
        }
    }
    return bbox;
}

// ============================================================================
// Extrude Implementation
// ============================================================================

ExtrudeResult Extrude::extrude(const ExtrudeProfile& profile,
                                const glm::vec3& direction,
                                float distance) {
    ExtrudeOptions options;
    options.direction = direction;
    options.distance = distance;
    return extrude(profile, options);
}

ExtrudeResult Extrude::extrude(const ExtrudeProfile& profile,
                                const ExtrudeOptions& options) {
    ExtrudeResult result;
    
    if (!profile.isValid()) {
        result.errorMessage = "Invalid profile: must have at least 3 points";
        return result;
    }
    
    if (options.distance <= 0) {
        result.errorMessage = "Extrusion distance must be positive";
        return result;
    }
    
    glm::vec3 dir = glm::normalize(options.direction);
    
    // Calculate actual distances
    float distPos = options.distance;
    float distNeg = 0.0f;
    
    if (options.twoSided) {
        distPos = options.distance * options.twoSidedRatio;
        distNeg = options.distance * (1.0f - options.twoSidedRatio);
    }
    
    const auto& outer = profile.outerBoundary();
    const auto& holes = profile.holes();
    glm::vec3 profileCenter = profile.centroid();
    
    MeshData& mesh = result.mesh;
    
    // Build lateral surface
    auto buildLateralSurface = [&](const std::vector<glm::vec3>& contour, bool isHole) {
        size_t n = contour.size();
        int baseVertex = static_cast<int>(mesh.vertexCount());
        
        // Create vertices for start and end profiles
        for (size_t i = 0; i < n; ++i) {
            glm::vec3 pStart = contour[i];
            if (options.twoSided) {
                pStart = pStart - dir * distNeg;
            }
            
            glm::vec3 pEnd = contour[i];
            if (options.draftAngle != 0.0f) {
                pEnd = applyDraft(contour[i], profileCenter, dir, distPos, options.draftAngle);
            } else {
                pEnd = contour[i] + dir * distPos;
            }
            
            if (options.twoSided) {
                pEnd = pEnd - dir * distNeg + dir * distPos;
            }
            
            mesh.addVertex(pStart);
            mesh.addVertex(pEnd);
        }
        
        // Create faces
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            
            int v0 = baseVertex + i * 2;       // start[i]
            int v1 = baseVertex + i * 2 + 1;   // end[i]
            int v2 = baseVertex + next * 2;    // start[next]
            int v3 = baseVertex + next * 2 + 1;// end[next]
            
            if (isHole) {
                // Reversed winding for holes
                mesh.addFace(v0, v1, v3);
                mesh.addFace(v0, v3, v2);
            } else {
                mesh.addFace(v0, v2, v3);
                mesh.addFace(v0, v3, v1);
            }
        }
    };
    
    // Build lateral surfaces for outer boundary and holes
    buildLateralSurface(outer, false);
    for (const auto& hole : holes) {
        buildLateralSurface(hole, true);
    }
    
    // Create caps if requested
    if (options.capEnds) {
        // Start cap
        std::vector<glm::vec3> startOuter = outer;
        if (options.twoSided) {
            for (auto& p : startOuter) {
                p = p - dir * distNeg;
            }
        }
        
        std::vector<std::vector<glm::vec3>> startHoles = holes;
        if (options.twoSided) {
            for (auto& hole : startHoles) {
                for (auto& p : hole) {
                    p = p - dir * distNeg;
                }
            }
        }
        
        MeshData startCap = triangulatePolygon(startOuter, startHoles, -dir);
        
        uint32_t capStartBase = static_cast<uint32_t>(mesh.vertexCount());
        for (size_t i = 0; i < startCap.vertexCount(); ++i) {
            mesh.addVertex(startCap.vertices()[i]);
        }
        for (size_t i = 0; i < startCap.faceCount(); ++i) {
            uint32_t i0 = startCap.indices()[i * 3];
            uint32_t i1 = startCap.indices()[i * 3 + 1];
            uint32_t i2 = startCap.indices()[i * 3 + 2];
            mesh.addFace(capStartBase + i0, capStartBase + i2, capStartBase + i1);
            result.capStartFaces.push_back(static_cast<uint32_t>(mesh.faceCount() - 1));
        }
        
        // End cap
        std::vector<glm::vec3> endOuter;
        for (const auto& p : outer) {
            if (options.draftAngle != 0.0f) {
                endOuter.push_back(applyDraft(p, profileCenter, dir, distPos, options.draftAngle));
            } else {
                endOuter.push_back(p + dir * distPos);
            }
        }
        
        std::vector<std::vector<glm::vec3>> endHoles;
        for (const auto& hole : holes) {
            std::vector<glm::vec3> endHole;
            for (const auto& p : hole) {
                if (options.draftAngle != 0.0f) {
                    endHole.push_back(applyDraft(p, profileCenter, dir, distPos, options.draftAngle));
                } else {
                    endHole.push_back(p + dir * distPos);
                }
            }
            endHoles.push_back(endHole);
        }
        
        MeshData endCap = triangulatePolygon(endOuter, endHoles, dir);
        
        uint32_t capEndBase = static_cast<uint32_t>(mesh.vertexCount());
        for (size_t i = 0; i < endCap.vertexCount(); ++i) {
            mesh.addVertex(endCap.vertices()[i]);
        }
        for (size_t i = 0; i < endCap.faceCount(); ++i) {
            uint32_t i0 = endCap.indices()[i * 3];
            uint32_t i1 = endCap.indices()[i * 3 + 1];
            uint32_t i2 = endCap.indices()[i * 3 + 2];
            mesh.addFace(capEndBase + i0, capEndBase + i1, capEndBase + i2);
            result.capEndFaces.push_back(static_cast<uint32_t>(mesh.faceCount() - 1));
        }
    }
    
    mesh.computeNormals();
    result.success = true;
    return result;
}

ExtrudeResult Extrude::extrudeWithDraft(const ExtrudeProfile& profile,
                                         const glm::vec3& direction,
                                         float distance,
                                         float draftAngle) {
    ExtrudeOptions options;
    options.direction = direction;
    options.distance = distance;
    options.draftAngle = draftAngle;
    return extrude(profile, options);
}

ExtrudeResult Extrude::extrudeTwoSided(const ExtrudeProfile& profile,
                                        const glm::vec3& direction,
                                        float totalDistance,
                                        float ratio) {
    ExtrudeOptions options;
    options.direction = direction;
    options.distance = totalDistance;
    options.twoSided = true;
    options.twoSidedRatio = ratio;
    return extrude(profile, options);
}

ExtrudeResult Extrude::extrudeNormal(const ExtrudeProfile& profile,
                                      float distance,
                                      const ExtrudeOptions& options) {
    ExtrudeOptions opts = options;
    opts.direction = profile.planeNormal();
    opts.distance = distance;
    return extrude(profile, opts);
}

std::vector<NURBSSurface> Extrude::createSurfaces(const ExtrudeProfile& profile,
                                                   const ExtrudeOptions& options) {
    std::vector<NURBSSurface> surfaces;
    
    if (!profile.isValid()) return surfaces;
    
    glm::vec3 dir = glm::normalize(options.direction);
    const auto& outer = profile.outerBoundary();
    glm::vec3 profileCenter = profile.centroid();
    
    // Create ruled surface for each edge of the profile
    for (size_t i = 0; i < outer.size(); ++i) {
        glm::vec3 p0 = outer[i];
        glm::vec3 p1 = outer[(i + 1) % outer.size()];
        
        glm::vec3 p0End, p1End;
        if (options.draftAngle != 0.0f) {
            p0End = applyDraft(p0, profileCenter, dir, options.distance, options.draftAngle);
            p1End = applyDraft(p1, profileCenter, dir, options.distance, options.draftAngle);
        } else {
            p0End = p0 + dir * options.distance;
            p1End = p1 + dir * options.distance;
        }
        
        // Create bilinear surface patch
        NURBSSurface patch = NURBSSurface::createBilinear(p0, p1, p0End, p1End);
        surfaces.push_back(patch);
    }
    
    return surfaces;
}

MeshData Extrude::createLateralMesh(const ExtrudeProfile& profile,
                                     const ExtrudeOptions& options) {
    ExtrudeOptions opts = options;
    opts.capEnds = false;
    return extrude(profile, opts).mesh;
}

MeshData Extrude::createCapMesh(const ExtrudeProfile& profile,
                                 const glm::vec3& offset,
                                 bool flipNormals) {
    std::vector<glm::vec3> outer;
    for (const auto& p : profile.outerBoundary()) {
        outer.push_back(p + offset);
    }
    
    std::vector<std::vector<glm::vec3>> holes;
    for (const auto& hole : profile.holes()) {
        std::vector<glm::vec3> h;
        for (const auto& p : hole) {
            h.push_back(p + offset);
        }
        holes.push_back(h);
    }
    
    glm::vec3 normal = profile.planeNormal();
    if (flipNormals) normal = -normal;
    
    return triangulatePolygon(outer, holes, normal);
}

glm::vec3 Extrude::applyDraft(const glm::vec3& point,
                               const glm::vec3& profileCenter,
                               const glm::vec3& direction,
                               float distance,
                               float draftAngle) {
    // Calculate radial direction from center
    glm::vec3 radial = point - profileCenter;
    float radialDist = glm::length(radial);
    
    if (radialDist < 1e-10f) {
        // Point is at center, just translate
        return point + direction * distance;
    }
    
    radial /= radialDist;
    
    // Calculate draft offset
    float draftRad = glm::radians(draftAngle);
    float draftOffset = distance * std::tan(draftRad);
    
    return point + direction * distance + radial * draftOffset;
}

MeshData Extrude::triangulatePolygon(const std::vector<glm::vec3>& outer,
                                      const std::vector<std::vector<glm::vec3>>& holes,
                                      const glm::vec3& normal) {
    MeshData mesh;
    
    if (outer.size() < 3) return mesh;
    
    // Simple ear clipping triangulation for polygon without holes
    // For production, use a proper triangulation library like earcut
    
    if (holes.empty()) {
        // Simple fan triangulation for convex polygons
        // For concave polygons, this should use ear clipping
        for (size_t i = 0; i < outer.size(); ++i) {
            mesh.addVertex(outer[i]);
        }
        
        // Check if polygon is convex (simplified check)
        bool isConvex = true;
        for (size_t i = 0; i < outer.size() && isConvex; ++i) {
            const glm::vec3& p0 = outer[i];
            const glm::vec3& p1 = outer[(i + 1) % outer.size()];
            const glm::vec3& p2 = outer[(i + 2) % outer.size()];
            
            glm::vec3 cross = glm::cross(p1 - p0, p2 - p1);
            if (glm::dot(cross, normal) < -1e-6f) {
                isConvex = false;
            }
        }
        
        if (isConvex) {
            // Fan triangulation
            for (size_t i = 1; i < outer.size() - 1; ++i) {
                mesh.addFace(0, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1));
            }
        } else {
            // Ear clipping for concave polygons
            std::vector<size_t> remaining;
            for (size_t i = 0; i < outer.size(); ++i) {
                remaining.push_back(i);
            }
            
            while (remaining.size() > 3) {
                bool earFound = false;
                
                for (size_t i = 0; i < remaining.size() && !earFound; ++i) {
                    size_t prev = (i + remaining.size() - 1) % remaining.size();
                    size_t next = (i + 1) % remaining.size();
                    
                    const glm::vec3& p0 = outer[remaining[prev]];
                    const glm::vec3& p1 = outer[remaining[i]];
                    const glm::vec3& p2 = outer[remaining[next]];
                    
                    // Check if this is a convex vertex
                    glm::vec3 cross = glm::cross(p1 - p0, p2 - p1);
                    if (glm::dot(cross, normal) < 1e-6f) continue;
                    
                    // Check if any other vertex is inside this triangle
                    bool hasInside = false;
                    for (size_t j = 0; j < remaining.size() && !hasInside; ++j) {
                        if (j == prev || j == i || j == next) continue;
                        
                        const glm::vec3& pt = outer[remaining[j]];
                        
                        // Point in triangle test
                        glm::vec3 v0 = p2 - p0;
                        glm::vec3 v1 = p1 - p0;
                        glm::vec3 v2 = pt - p0;
                        
                        float dot00 = glm::dot(v0, v0);
                        float dot01 = glm::dot(v0, v1);
                        float dot02 = glm::dot(v0, v2);
                        float dot11 = glm::dot(v1, v1);
                        float dot12 = glm::dot(v1, v2);
                        
                        float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
                        float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
                        float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
                        
                        if (u >= 0 && v >= 0 && u + v < 1) {
                            hasInside = true;
                        }
                    }
                    
                    if (!hasInside) {
                        mesh.addFace(static_cast<uint32_t>(remaining[prev]),
                                    static_cast<uint32_t>(remaining[i]),
                                    static_cast<uint32_t>(remaining[next]));
                        remaining.erase(remaining.begin() + i);
                        earFound = true;
                    }
                }
                
                if (!earFound) {
                    // Fallback: just add remaining as triangles
                    break;
                }
            }
            
            if (remaining.size() == 3) {
                mesh.addFace(static_cast<uint32_t>(remaining[0]),
                            static_cast<uint32_t>(remaining[1]),
                            static_cast<uint32_t>(remaining[2]));
            }
        }
    } else {
        // For polygons with holes, we need constrained triangulation
        // This is a simplified version - production code should use earcut or similar
        
        // First, add all vertices
        for (const auto& p : outer) {
            mesh.addVertex(p);
        }
        for (const auto& hole : holes) {
            for (const auto& p : hole) {
                mesh.addVertex(p);
            }
        }
        
        // Simple approach: triangulate outer, then remove triangles that overlap holes
        // This is not correct but serves as a placeholder
        for (size_t i = 1; i < outer.size() - 1; ++i) {
            mesh.addFace(0, static_cast<uint32_t>(i), static_cast<uint32_t>(i + 1));
        }
    }
    
    return mesh;
}

} // namespace geometry
} // namespace dc3d
