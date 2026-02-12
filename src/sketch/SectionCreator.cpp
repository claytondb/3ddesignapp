#include "SectionCreator.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace dc {
namespace sketch {

// ==================== Polyline Implementation ====================

float Polyline::length() const {
    float len = 0.0f;
    for (size_t i = 1; i < points.size(); ++i) {
        len += glm::length(points[i] - points[i - 1]);
    }
    if (isClosed && points.size() > 2) {
        len += glm::length(points.front() - points.back());
    }
    return len;
}

void Polyline::reverse() {
    std::reverse(points.begin(), points.end());
}

void Polyline::simplify(float tolerance) {
    if (points.size() < 3) return;
    
    std::vector<glm::vec3> simplified;
    simplified.push_back(points[0]);
    
    for (size_t i = 1; i < points.size() - 1; ++i) {
        glm::vec3 v1 = glm::normalize(points[i] - points[i - 1]);
        glm::vec3 v2 = glm::normalize(points[i + 1] - points[i]);
        
        float dot = glm::dot(v1, v2);
        if (dot < 1.0f - tolerance) {
            // Not collinear, keep this point
            simplified.push_back(points[i]);
        }
    }
    
    simplified.push_back(points.back());
    points = std::move(simplified);
}

// ==================== SimpleMesh Implementation ====================

void SimpleMesh::getTriangle(size_t index, glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) const {
    size_t baseIdx = index * 3;
    if (baseIdx + 2 < indices.size()) {
        v0 = vertices[indices[baseIdx]];
        v1 = vertices[indices[baseIdx + 1]];
        v2 = vertices[indices[baseIdx + 2]];
    }
}

// ==================== SectionPlane Implementation ====================

SectionPlane SectionPlane::fromPointNormal(const glm::vec3& point, const glm::vec3& normal) {
    SectionPlane plane;
    plane.origin = point;
    plane.normal = glm::normalize(normal);
    return plane;
}

SectionPlane SectionPlane::fromSketchPlane(const SketchPlane& sketchPlane) {
    SectionPlane plane;
    plane.origin = sketchPlane.origin;
    plane.normal = sketchPlane.normal;
    return plane;
}

float SectionPlane::signedDistance(const glm::vec3& point) const {
    return glm::dot(point - origin, normal);
}

int SectionPlane::classify(const glm::vec3& point, float tolerance) const {
    float d = signedDistance(point);
    if (d > tolerance) return 1;
    if (d < -tolerance) return -1;
    return 0;
}

// ==================== SectionResult Implementation ====================

size_t SectionResult::totalPoints() const {
    size_t total = 0;
    for (const auto& pl : polylines) {
        total += pl.points.size();
    }
    return total;
}

void SectionResult::getBoundingBox(glm::vec3& minPt, glm::vec3& maxPt) const {
    minPt = glm::vec3(std::numeric_limits<float>::max());
    maxPt = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& pl : polylines) {
        for (const auto& pt : pl.points) {
            minPt = glm::min(minPt, pt);
            maxPt = glm::max(maxPt, pt);
        }
    }
}

// ==================== SectionCreator Implementation ====================

SectionResult SectionCreator::createSection(const SimpleMesh& mesh, const SectionPlane& plane) {
    SectionResult result;
    
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        result.errorMessage = "Empty mesh";
        return result;
    }
    
    // Collect all edge segments from triangle-plane intersections
    std::vector<EdgeSegment> segments;
    
    for (size_t triIdx = 0; triIdx < mesh.numTriangles(); ++triIdx) {
        glm::vec3 v0, v1, v2;
        mesh.getTriangle(triIdx, v0, v1, v2);
        
        std::vector<glm::vec3> intersections = intersectTriangle(v0, v1, v2, plane);
        
        if (intersections.size() == 2) {
            EdgeSegment seg;
            seg.start = intersections[0];
            seg.end = intersections[1];
            seg.triangleIndex = triIdx;
            segments.push_back(seg);
        }
    }
    
    if (segments.empty()) {
        result.success = true;  // No intersection is valid
        return result;
    }
    
    // Chain segments into polylines
    result.polylines = chainSegments(segments);
    
    // Optionally simplify
    if (m_autoSimplify) {
        for (auto& pl : result.polylines) {
            pl.simplify(m_simplifyTolerance);
        }
    }
    
    result.success = true;
    return result;
}

Sketch::Ptr SectionCreator::createSketchFromSection(const SimpleMesh& mesh, const SketchPlane& plane) {
    SectionPlane secPlane = SectionPlane::fromSketchPlane(plane);
    SectionResult result = createSection(mesh, secPlane);
    
    if (!result.success || result.polylines.empty()) {
        return nullptr;
    }
    
    auto sketch = Sketch::create(plane);
    
    for (const auto& polyline : result.polylines) {
        if (polyline.points.size() < 2) continue;
        
        // Convert 3D points to 2D sketch coordinates
        std::vector<glm::vec2> points2D;
        for (const auto& pt3D : polyline.points) {
            points2D.push_back(plane.toLocal(pt3D));
        }
        
        // Create line segments
        for (size_t i = 1; i < points2D.size(); ++i) {
            sketch->addLine(points2D[i - 1], points2D[i]);
        }
        
        // Close if needed
        if (polyline.isClosed && points2D.size() > 2) {
            sketch->addLine(points2D.back(), points2D.front());
        }
    }
    
    return sketch;
}

std::vector<glm::vec3> SectionCreator::intersectTriangle(const glm::vec3& v0, const glm::vec3& v1,
                                                          const glm::vec3& v2, const SectionPlane& plane) {
    std::vector<glm::vec3> intersections;
    
    // Classify vertices
    int c0 = plane.classify(v0, m_pointTolerance);
    int c1 = plane.classify(v1, m_pointTolerance);
    int c2 = plane.classify(v2, m_pointTolerance);
    
    // Check if any vertex is on the plane
    if (c0 == 0) intersections.push_back(v0);
    if (c1 == 0) intersections.push_back(v1);
    if (c2 == 0) intersections.push_back(v2);
    
    // Check edges for intersections
    glm::vec3 ip;
    
    // Edge 0-1
    if (c0 != 0 && c1 != 0 && c0 != c1) {
        if (intersectSegment(v0, v1, plane, ip)) {
            intersections.push_back(ip);
        }
    }
    
    // Edge 1-2
    if (c1 != 0 && c2 != 0 && c1 != c2) {
        if (intersectSegment(v1, v2, plane, ip)) {
            intersections.push_back(ip);
        }
    }
    
    // Edge 2-0
    if (c2 != 0 && c0 != 0 && c2 != c0) {
        if (intersectSegment(v2, v0, plane, ip)) {
            intersections.push_back(ip);
        }
    }
    
    // Remove duplicates
    std::vector<glm::vec3> unique;
    for (const auto& pt : intersections) {
        bool isDup = false;
        for (const auto& upt : unique) {
            if (areCoincident(pt, upt)) {
                isDup = true;
                break;
            }
        }
        if (!isDup) {
            unique.push_back(pt);
        }
    }
    
    return unique;
}

bool SectionCreator::intersectSegment(const glm::vec3& p0, const glm::vec3& p1,
                                       const SectionPlane& plane, glm::vec3& result) {
    float d0 = plane.signedDistance(p0);
    float d1 = plane.signedDistance(p1);
    
    // Check if segment crosses plane
    if (d0 * d1 > 0) {
        return false;  // Both on same side
    }
    
    float denom = d0 - d1;
    if (std::abs(denom) < 1e-10f) {
        return false;  // Segment parallel to plane
    }
    
    float t = d0 / denom;
    result = p0 + t * (p1 - p0);
    return true;
}

std::vector<Polyline> SectionCreator::chainSegments(std::vector<EdgeSegment>& segments) {
    std::vector<Polyline> polylines;
    std::vector<bool> used(segments.size(), false);
    
    while (true) {
        // Find first unused segment
        int startIdx = -1;
        for (size_t i = 0; i < segments.size(); ++i) {
            if (!used[i]) {
                startIdx = static_cast<int>(i);
                break;
            }
        }
        
        if (startIdx < 0) break;  // All segments used
        
        // Start a new polyline
        Polyline polyline;
        polyline.points.push_back(segments[startIdx].start);
        polyline.points.push_back(segments[startIdx].end);
        used[startIdx] = true;
        
        // Try to extend forward
        bool extended = true;
        while (extended) {
            extended = false;
            int nextIdx = findConnectedSegment(segments, used, polyline.points.back());
            if (nextIdx >= 0) {
                used[nextIdx] = true;
                
                // Determine which end connects
                if (areCoincident(segments[nextIdx].start, polyline.points.back())) {
                    polyline.points.push_back(segments[nextIdx].end);
                } else {
                    polyline.points.push_back(segments[nextIdx].start);
                }
                extended = true;
            }
        }
        
        // Try to extend backward
        extended = true;
        while (extended) {
            extended = false;
            int prevIdx = findConnectedSegment(segments, used, polyline.points.front());
            if (prevIdx >= 0) {
                used[prevIdx] = true;
                
                // Determine which end connects
                if (areCoincident(segments[prevIdx].end, polyline.points.front())) {
                    polyline.points.insert(polyline.points.begin(), segments[prevIdx].start);
                } else {
                    polyline.points.insert(polyline.points.begin(), segments[prevIdx].end);
                }
                extended = true;
            }
        }
        
        // Check if closed
        if (polyline.points.size() > 2 && 
            areCoincident(polyline.points.front(), polyline.points.back())) {
            polyline.isClosed = true;
            polyline.points.pop_back();  // Remove duplicate closing point
        }
        
        polylines.push_back(std::move(polyline));
    }
    
    return polylines;
}

bool SectionCreator::areCoincident(const glm::vec3& a, const glm::vec3& b) const {
    return glm::length(a - b) < m_pointTolerance;
}

int SectionCreator::findConnectedSegment(const std::vector<EdgeSegment>& segments,
                                          const std::vector<bool>& used,
                                          const glm::vec3& point) {
    for (size_t i = 0; i < segments.size(); ++i) {
        if (used[i]) continue;
        
        if (areCoincident(segments[i].start, point) ||
            areCoincident(segments[i].end, point)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

SectionCreator::Ptr SectionCreator::create() {
    return std::make_shared<SectionCreator>();
}

} // namespace sketch
} // namespace dc
