#pragma once

#include "Sketch.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace dc {
namespace sketch {

/**
 * @brief A polyline representing a section of a mesh
 */
struct Polyline {
    std::vector<glm::vec3> points;
    bool isClosed = false;
    
    /**
     * @brief Get the length of the polyline
     */
    float length() const;
    
    /**
     * @brief Reverse the direction of the polyline
     */
    void reverse();
    
    /**
     * @brief Simplify the polyline by removing collinear points
     * @param tolerance Angular tolerance in radians
     */
    void simplify(float tolerance = 1e-4f);
    
    /**
     * @brief Check if polyline is valid (has at least 2 points)
     */
    bool isValid() const { return points.size() >= 2; }
};

/**
 * @brief A simple triangle mesh for section operations
 */
struct SimpleMesh {
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;  // Triangle indices (3 per triangle)
    
    /**
     * @brief Get number of triangles
     */
    size_t numTriangles() const { return indices.size() / 3; }
    
    /**
     * @brief Get triangle vertices
     */
    void getTriangle(size_t index, glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) const;
};

/**
 * @brief A plane for section operations
 */
struct SectionPlane {
    glm::vec3 origin{0.0f};
    glm::vec3 normal{0.0f, 0.0f, 1.0f};
    
    /**
     * @brief Create from point and normal
     */
    static SectionPlane fromPointNormal(const glm::vec3& point, const glm::vec3& normal);
    
    /**
     * @brief Create from SketchPlane
     */
    static SectionPlane fromSketchPlane(const SketchPlane& sketchPlane);
    
    /**
     * @brief Get signed distance to point
     */
    float signedDistance(const glm::vec3& point) const;
    
    /**
     * @brief Check which side of plane a point is on
     * @return > 0 for front, < 0 for back, 0 for on plane
     */
    int classify(const glm::vec3& point, float tolerance = 1e-6f) const;
};

/**
 * @brief Result of a section operation
 */
struct SectionResult {
    std::vector<Polyline> polylines;
    bool success = false;
    std::string errorMessage;
    
    /**
     * @brief Get total number of points across all polylines
     */
    size_t totalPoints() const;
    
    /**
     * @brief Get bounding box of all polylines
     */
    void getBoundingBox(glm::vec3& minPt, glm::vec3& maxPt) const;
};

/**
 * @brief Creates cross-sections of meshes
 * 
 * Computes the intersection of a triangle mesh with a plane,
 * producing a set of polylines representing the section profile.
 */
class SectionCreator {
public:
    using Ptr = std::shared_ptr<SectionCreator>;
    
    SectionCreator() = default;
    
    /**
     * @brief Create a section of a mesh with a plane
     * @param mesh The mesh to section
     * @param plane The cutting plane
     * @return Section result with polylines
     */
    SectionResult createSection(const SimpleMesh& mesh, const SectionPlane& plane);
    
    /**
     * @brief Create a section and convert to sketch entities
     * @param mesh The mesh to section
     * @param plane The sketch plane to create entities on
     * @return Sketch containing the section profile
     */
    Sketch::Ptr createSketchFromSection(const SimpleMesh& mesh, const SketchPlane& plane);
    
    /**
     * @brief Set tolerance for coincident points
     */
    void setPointTolerance(float tolerance) { m_pointTolerance = tolerance; }
    
    /**
     * @brief Set tolerance for collinear simplification
     */
    void setSimplifyTolerance(float tolerance) { m_simplifyTolerance = tolerance; }
    
    /**
     * @brief Enable/disable automatic polyline simplification
     */
    void setAutoSimplify(bool enable) { m_autoSimplify = enable; }
    
    static Ptr create();

private:
    float m_pointTolerance = 1e-6f;
    float m_simplifyTolerance = 1e-4f;
    bool m_autoSimplify = true;
    
    /**
     * @brief Segment of a triangle edge that intersects the plane
     */
    struct EdgeSegment {
        glm::vec3 start;
        glm::vec3 end;
        size_t triangleIndex;
        int edgeIndex;  // Which edge of the triangle (0, 1, or 2)
    };
    
    /**
     * @brief Intersect a single triangle with the plane
     * @return 0, 1, or 2 intersection points
     */
    std::vector<glm::vec3> intersectTriangle(const glm::vec3& v0, const glm::vec3& v1, 
                                              const glm::vec3& v2, const SectionPlane& plane);
    
    /**
     * @brief Intersect a line segment with the plane
     * @param result Intersection point if exists
     * @return true if intersection exists
     */
    bool intersectSegment(const glm::vec3& p0, const glm::vec3& p1, 
                         const SectionPlane& plane, glm::vec3& result);
    
    /**
     * @brief Chain edge segments into polylines
     */
    std::vector<Polyline> chainSegments(std::vector<EdgeSegment>& segments);
    
    /**
     * @brief Check if two points are coincident
     */
    bool areCoincident(const glm::vec3& a, const glm::vec3& b) const;
    
    /**
     * @brief Find segment that starts near the given point
     */
    int findConnectedSegment(const std::vector<EdgeSegment>& segments, 
                            const std::vector<bool>& used,
                            const glm::vec3& point);
};

} // namespace sketch
} // namespace dc
