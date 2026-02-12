/**
 * @file PlanarSurface.cpp
 * @brief Planar surface creation implementation
 */

#include "PlanarSurface.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace dc3d {
namespace geometry {

// ============================================================================
// PlanarProfile Implementation
// ============================================================================

PlanarProfile::PlanarProfile(const std::vector<glm::vec3>& outerBoundary)
    : m_outer(outerBoundary) {
}

void PlanarProfile::setOuterBoundary(const std::vector<glm::vec3>& points) {
    m_outer = points;
}

void PlanarProfile::addHole(const std::vector<glm::vec3>& points) {
    if (points.size() >= 3) {
        m_holes.push_back(points);
    }
}

void PlanarProfile::clearHoles() {
    m_holes.clear();
}

glm::vec3 PlanarProfile::normal() const {
    if (m_outer.size() < 3) return glm::vec3(0, 0, 1);
    
    // Newell's method for polygon normal
    glm::vec3 n(0.0f);
    for (size_t i = 0; i < m_outer.size(); ++i) {
        const glm::vec3& curr = m_outer[i];
        const glm::vec3& next = m_outer[(i + 1) % m_outer.size()];
        n.x += (curr.y - next.y) * (curr.z + next.z);
        n.y += (curr.z - next.z) * (curr.x + next.x);
        n.z += (curr.x - next.x) * (curr.y + next.y);
    }
    
    float len = glm::length(n);
    return (len > 1e-10f) ? n / len : glm::vec3(0, 0, 1);
}

glm::vec3 PlanarProfile::centroid() const {
    if (m_outer.empty()) return glm::vec3(0.0f);
    
    glm::vec3 c(0.0f);
    for (const auto& p : m_outer) {
        c += p;
    }
    return c / static_cast<float>(m_outer.size());
}

float PlanarProfile::signedArea() const {
    if (m_outer.size() < 3) return 0.0f;
    
    glm::vec3 n = normal();
    glm::vec3 c = centroid();
    
    // Create local 2D coordinate system
    glm::vec3 u = glm::normalize(glm::cross(n, 
        std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(n, u);
    
    float area = 0.0f;
    for (size_t i = 0; i < m_outer.size(); ++i) {
        const glm::vec3& p0 = m_outer[i] - c;
        const glm::vec3& p1 = m_outer[(i + 1) % m_outer.size()] - c;
        
        float x0 = glm::dot(p0, u);
        float y0 = glm::dot(p0, v);
        float x1 = glm::dot(p1, u);
        float y1 = glm::dot(p1, v);
        
        area += (x0 * y1 - x1 * y0);
    }
    
    return area * 0.5f;
}

float PlanarProfile::area() const {
    float totalArea = std::abs(signedArea());
    
    // Subtract hole areas
    glm::vec3 n = normal();
    glm::vec3 c = centroid();
    glm::vec3 u = glm::normalize(glm::cross(n, 
        std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(n, u);
    
    for (const auto& hole : m_holes) {
        float holeArea = 0.0f;
        for (size_t i = 0; i < hole.size(); ++i) {
            const glm::vec3& p0 = hole[i] - c;
            const glm::vec3& p1 = hole[(i + 1) % hole.size()] - c;
            
            float x0 = glm::dot(p0, u);
            float y0 = glm::dot(p0, v);
            float x1 = glm::dot(p1, u);
            float y1 = glm::dot(p1, v);
            
            holeArea += (x0 * y1 - x1 * y0);
        }
        totalArea -= std::abs(holeArea * 0.5f);
    }
    
    return totalArea;
}

BoundingBox PlanarProfile::boundingBox() const {
    BoundingBox bbox;
    for (const auto& p : m_outer) {
        bbox.expand(p);
    }
    return bbox;
}

void PlanarProfile::ensureCorrectWinding() {
    // Outer should be CCW (positive signed area)
    if (signedArea() < 0) {
        std::reverse(m_outer.begin(), m_outer.end());
    }
    
    // Holes should be CW (negative signed area relative to outer)
    for (auto& hole : m_holes) {
        // Compute hole signed area
        glm::vec3 n = normal();
        glm::vec3 c = centroid();
        glm::vec3 u = glm::normalize(glm::cross(n, 
            std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
        glm::vec3 v = glm::cross(n, u);
        
        float holeArea = 0.0f;
        for (size_t i = 0; i < hole.size(); ++i) {
            const glm::vec3& p0 = hole[i] - c;
            const glm::vec3& p1 = hole[(i + 1) % hole.size()] - c;
            
            float x0 = glm::dot(p0, u);
            float y0 = glm::dot(p0, v);
            float x1 = glm::dot(p1, u);
            float y1 = glm::dot(p1, v);
            
            holeArea += (x0 * y1 - x1 * y0);
        }
        
        // If positive, reverse to make CW
        if (holeArea > 0) {
            std::reverse(hole.begin(), hole.end());
        }
    }
}

void PlanarProfile::flattenToPlane() {
    if (m_outer.size() < 3) return;
    
    glm::vec3 n = normal();
    glm::vec3 c = centroid();
    
    // Project all points onto plane through centroid
    auto projectToPlane = [&](glm::vec3& p) {
        float dist = glm::dot(p - c, n);
        p -= dist * n;
    };
    
    for (auto& p : m_outer) {
        projectToPlane(p);
    }
    
    for (auto& hole : m_holes) {
        for (auto& p : hole) {
            projectToPlane(p);
        }
    }
}

// ============================================================================
// PlanarSurface Implementation
// ============================================================================

PlanarSurfaceResult PlanarSurface::createPlanar(const PlanarProfile& profile,
                                                 const PlanarSurfaceOptions& options) {
    PlanarSurfaceResult result;
    
    if (!profile.isValid()) {
        result.errorMessage = "Invalid profile: need at least 3 points";
        return result;
    }
    
    PlanarProfile cleanProfile = profile;
    cleanProfile.ensureCorrectWinding();
    cleanProfile.flattenToPlane();
    
    result.normal = cleanProfile.normal();
    result.centroid = cleanProfile.centroid();
    result.area = cleanProfile.area();
    
    if (options.flipNormal) {
        result.normal = -result.normal;
    }
    
    // Triangulate
    result.mesh = triangulate(cleanProfile.outerBoundary(), 
                               cleanProfile.holes(),
                               result.normal);
    
    if (options.flipNormal) {
        result.mesh.flipNormals();
    }
    
    // Handle thickness
    if (options.thickness > 0) {
        MeshData backMesh = result.mesh;
        
        // Offset back face
        for (size_t i = 0; i < backMesh.vertexCount(); ++i) {
            backMesh.vertices()[i] -= result.normal * options.thickness;
        }
        backMesh.flipNormals();
        
        // Merge meshes
        uint32_t baseIdx = static_cast<uint32_t>(result.mesh.vertexCount());
        for (size_t i = 0; i < backMesh.vertexCount(); ++i) {
            result.mesh.addVertex(backMesh.vertices()[i]);
        }
        for (size_t i = 0; i < backMesh.faceCount(); ++i) {
            result.mesh.addFace(baseIdx + backMesh.indices()[i * 3],
                               baseIdx + backMesh.indices()[i * 3 + 1],
                               baseIdx + backMesh.indices()[i * 3 + 2]);
        }
        
        // Add side faces
        const auto& outer = cleanProfile.outerBoundary();
        for (size_t i = 0; i < outer.size(); ++i) {
            size_t next = (i + 1) % outer.size();
            
            glm::vec3 p0 = outer[i];
            glm::vec3 p1 = outer[next];
            glm::vec3 p2 = outer[i] - result.normal * options.thickness;
            glm::vec3 p3 = outer[next] - result.normal * options.thickness;
            
            uint32_t v0 = result.mesh.addVertex(p0);
            uint32_t v1 = result.mesh.addVertex(p1);
            uint32_t v2 = result.mesh.addVertex(p2);
            uint32_t v3 = result.mesh.addVertex(p3);
            
            result.mesh.addFace(v0, v1, v3);
            result.mesh.addFace(v0, v3, v2);
        }
        
        result.mesh.computeNormals();
    }
    
    // Create NURBS surface
    result.surface = createNurbsSurface(cleanProfile);
    
    result.success = true;
    return result;
}

PlanarSurfaceResult PlanarSurface::createPlanar(const std::vector<glm::vec3>& boundary,
                                                 const PlanarSurfaceOptions& options) {
    PlanarProfile profile(boundary);
    return createPlanar(profile, options);
}

PlanarSurfaceResult PlanarSurface::createPlanarWithHoles(const std::vector<glm::vec3>& outer,
                                                          const std::vector<std::vector<glm::vec3>>& holes,
                                                          const PlanarSurfaceOptions& options) {
    PlanarProfile profile(outer);
    for (const auto& hole : holes) {
        profile.addHole(hole);
    }
    return createPlanar(profile, options);
}

MeshData PlanarSurface::createRectangle(const glm::vec3& center,
                                         const glm::vec3& normal,
                                         float width,
                                         float height) {
    MeshData mesh;
    
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 u = glm::normalize(glm::cross(n, 
        std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(n, u);
    
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    
    mesh.addVertex(center - u * hw - v * hh, n);
    mesh.addVertex(center + u * hw - v * hh, n);
    mesh.addVertex(center + u * hw + v * hh, n);
    mesh.addVertex(center - u * hw + v * hh, n);
    
    mesh.addFace(0, 1, 2);
    mesh.addFace(0, 2, 3);
    
    return mesh;
}

MeshData PlanarSurface::createDisk(const glm::vec3& center,
                                    const glm::vec3& normal,
                                    float radius,
                                    int segments) {
    MeshData mesh;
    
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 u = glm::normalize(glm::cross(n, 
        std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(n, u);
    
    // Center vertex
    mesh.addVertex(center, n);
    
    // Edge vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        glm::vec3 p = center + radius * (std::cos(angle) * u + std::sin(angle) * v);
        mesh.addVertex(p, n);
    }
    
    // Triangles
    for (int i = 0; i < segments; ++i) {
        int next = (i % segments) + 1;
        int nextNext = ((i + 1) % segments) + 1;
        mesh.addFace(0, next, nextNext);
    }
    
    return mesh;
}

MeshData PlanarSurface::createAnnulus(const glm::vec3& center,
                                       const glm::vec3& normal,
                                       float innerRadius,
                                       float outerRadius,
                                       int segments) {
    MeshData mesh;
    
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 u = glm::normalize(glm::cross(n, 
        std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(n, u);
    
    // Inner circle vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        glm::vec3 p = center + innerRadius * (std::cos(angle) * u + std::sin(angle) * v);
        mesh.addVertex(p, n);
    }
    
    // Outer circle vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        glm::vec3 p = center + outerRadius * (std::cos(angle) * u + std::sin(angle) * v);
        mesh.addVertex(p, n);
    }
    
    // Quads between circles
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        
        uint32_t inner0 = i;
        uint32_t inner1 = next;
        uint32_t outer0 = segments + i;
        uint32_t outer1 = segments + next;
        
        mesh.addFace(inner0, outer0, outer1);
        mesh.addFace(inner0, outer1, inner1);
    }
    
    return mesh;
}

MeshData PlanarSurface::createEllipse(const glm::vec3& center,
                                       const glm::vec3& normal,
                                       float radiusX,
                                       float radiusY,
                                       int segments) {
    MeshData mesh;
    
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 u = glm::normalize(glm::cross(n, 
        std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(n, u);
    
    // Center vertex
    mesh.addVertex(center, n);
    
    // Edge vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        glm::vec3 p = center + radiusX * std::cos(angle) * u + radiusY * std::sin(angle) * v;
        mesh.addVertex(p, n);
    }
    
    // Triangles
    for (int i = 0; i < segments; ++i) {
        int next = (i % segments) + 1;
        int nextNext = ((i + 1) % segments) + 1;
        mesh.addFace(0, next, nextNext);
    }
    
    return mesh;
}

MeshData PlanarSurface::createRegularPolygon(const glm::vec3& center,
                                              const glm::vec3& normal,
                                              float radius,
                                              int sides) {
    return createDisk(center, normal, radius, sides);
}

NURBSSurface PlanarSurface::createNurbsSurface(const PlanarProfile& profile) {
    BoundingBox bbox = profile.boundingBox();
    glm::vec3 n = profile.normal();
    
    // Create a bilinear surface covering the bounding box
    glm::vec3 center = bbox.center();
    glm::vec3 u = glm::normalize(glm::cross(n, 
        std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 v = glm::cross(n, u);
    
    glm::vec3 dim = bbox.dimensions();
    float hw = std::max(dim.x, std::max(dim.y, dim.z)) * 0.6f;
    
    return NURBSSurface::createBilinear(
        center - u * hw - v * hw,
        center + u * hw - v * hw,
        center - u * hw + v * hw,
        center + u * hw + v * hw
    );
}

MeshData PlanarSurface::triangulate(const std::vector<glm::vec3>& outer,
                                     const std::vector<std::vector<glm::vec3>>& holes,
                                     const glm::vec3& normal) {
    if (holes.empty()) {
        return earClipTriangulate(outer, normal);
    }
    
    // Merge holes into outer polygon
    std::vector<glm::vec3> merged = mergePolygonWithHoles(outer, holes);
    return earClipTriangulate(merged, normal);
}

bool PlanarSurface::isPointInPolygon(const glm::vec3& point,
                                      const std::vector<glm::vec3>& polygon) {
    if (polygon.size() < 3) return false;
    
    // Use ray casting in 2D (project to XY, XZ, or YZ based on normal)
    int crossings = 0;
    
    for (size_t i = 0; i < polygon.size(); ++i) {
        const glm::vec3& p1 = polygon[i];
        const glm::vec3& p2 = polygon[(i + 1) % polygon.size()];
        
        // Simple 2D ray casting using X and Y
        if ((p1.y <= point.y && p2.y > point.y) || (p2.y <= point.y && p1.y > point.y)) {
            float t = (point.y - p1.y) / (p2.y - p1.y);
            float xIntersect = p1.x + t * (p2.x - p1.x);
            if (point.x < xIntersect) {
                crossings++;
            }
        }
    }
    
    return (crossings % 2) == 1;
}

bool PlanarSurface::isEar(const std::vector<size_t>& indices,
                          const std::vector<glm::vec3>& vertices,
                          size_t prev, size_t curr, size_t next,
                          const glm::vec3& normal) {
    const glm::vec3& p0 = vertices[indices[prev]];
    const glm::vec3& p1 = vertices[indices[curr]];
    const glm::vec3& p2 = vertices[indices[next]];
    
    // Check convexity
    glm::vec3 cross = glm::cross(p1 - p0, p2 - p1);
    if (glm::dot(cross, normal) < 1e-6f) {
        return false;  // Reflex vertex
    }
    
    // Check if any other vertex is inside the triangle
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i == prev || i == curr || i == next) continue;
        
        const glm::vec3& pt = vertices[indices[i]];
        
        // Point in triangle test using barycentric coordinates
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
            return false;  // Point inside triangle
        }
    }
    
    return true;
}

MeshData PlanarSurface::earClipTriangulate(const std::vector<glm::vec3>& polygon,
                                            const glm::vec3& normal) {
    MeshData mesh;
    
    if (polygon.size() < 3) return mesh;
    
    // Add all vertices
    for (const auto& p : polygon) {
        mesh.addVertex(p, normal);
    }
    
    // Create index list
    std::vector<size_t> indices;
    for (size_t i = 0; i < polygon.size(); ++i) {
        indices.push_back(i);
    }
    
    // Ear clipping
    while (indices.size() > 3) {
        bool earFound = false;
        
        for (size_t i = 0; i < indices.size() && !earFound; ++i) {
            size_t prev = (i + indices.size() - 1) % indices.size();
            size_t next = (i + 1) % indices.size();
            
            if (isEar(indices, polygon, prev, i, next, normal)) {
                mesh.addFace(static_cast<uint32_t>(indices[prev]),
                            static_cast<uint32_t>(indices[i]),
                            static_cast<uint32_t>(indices[next]));
                indices.erase(indices.begin() + i);
                earFound = true;
            }
        }
        
        if (!earFound) {
            // Degenerate polygon, just add remaining as triangle
            break;
        }
    }
    
    // Add final triangle
    if (indices.size() == 3) {
        mesh.addFace(static_cast<uint32_t>(indices[0]),
                    static_cast<uint32_t>(indices[1]),
                    static_cast<uint32_t>(indices[2]));
    }
    
    return mesh;
}

std::vector<glm::vec3> PlanarSurface::mergePolygonWithHoles(
    const std::vector<glm::vec3>& outer,
    const std::vector<std::vector<glm::vec3>>& holes) {
    
    if (holes.empty()) return outer;
    
    std::vector<glm::vec3> result = outer;
    
    for (const auto& hole : holes) {
        if (hole.size() < 3) continue;
        
        // Find rightmost point in hole
        size_t holeRightmost = 0;
        float maxX = -std::numeric_limits<float>::max();
        for (size_t i = 0; i < hole.size(); ++i) {
            if (hole[i].x > maxX) {
                maxX = hole[i].x;
                holeRightmost = i;
            }
        }
        
        // Find closest visible point on outer boundary
        size_t outerClosest = 0;
        float minDist = std::numeric_limits<float>::max();
        
        for (size_t i = 0; i < result.size(); ++i) {
            float dist = glm::length(result[i] - hole[holeRightmost]);
            if (dist < minDist) {
                // Check visibility (simplified - just use distance)
                minDist = dist;
                outerClosest = i;
            }
        }
        
        // Create bridge: insert hole into outer at outerClosest
        std::vector<glm::vec3> newResult;
        
        // Copy outer up to and including closest point
        for (size_t i = 0; i <= outerClosest; ++i) {
            newResult.push_back(result[i]);
        }
        
        // Insert hole starting from rightmost
        for (size_t i = 0; i <= hole.size(); ++i) {
            newResult.push_back(hole[(holeRightmost + i) % hole.size()]);
        }
        
        // Bridge back
        newResult.push_back(result[outerClosest]);
        
        // Continue with rest of outer
        for (size_t i = outerClosest + 1; i < result.size(); ++i) {
            newResult.push_back(result[i]);
        }
        
        result = newResult;
    }
    
    return result;
}

} // namespace geometry
} // namespace dc3d
