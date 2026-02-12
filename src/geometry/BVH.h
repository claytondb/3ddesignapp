/**
 * @file BVH.h
 * @brief Bounding Volume Hierarchy for efficient ray-mesh intersection
 * 
 * Provides O(log n) ray-triangle intersection for picking operations.
 * Uses a top-down SAH (Surface Area Heuristic) construction method.
 */

#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <limits>
#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

// Forward declaration
class MeshData;

/**
 * @brief Ray for intersection tests
 */
struct Ray {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    float tMin = 0.0001f;   ///< Minimum t value (avoid self-intersection)
    float tMax = std::numeric_limits<float>::max();  ///< Maximum t value
    
    Ray() = default;
    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(glm::normalize(d)) {}
    
    glm::vec3 at(float t) const { return origin + direction * t; }
};

/**
 * @brief Result of a ray-BVH intersection
 */
struct BVHHitResult {
    bool hit = false;
    float t = std::numeric_limits<float>::max();     ///< Distance along ray
    uint32_t faceIndex = 0;                          ///< Triangle index
    glm::vec3 point{0.0f};                          ///< World hit point
    glm::vec3 normal{0.0f};                         ///< Surface normal
    glm::vec3 barycentric{0.0f};                    ///< Barycentric coordinates (u, v, w)
    
    // Vertex indices of hit triangle
    uint32_t indices[3] = {0, 0, 0};
};

/**
 * @brief Axis-aligned bounding box for BVH nodes
 */
struct AABB {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};
    
    AABB() = default;
    AABB(const glm::vec3& min_, const glm::vec3& max_) : min(min_), max(max_) {}
    
    /// Expand to include a point
    void expand(const glm::vec3& p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
    
    /// Expand to include another AABB
    void expand(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }
    
    /// Get center of the box
    glm::vec3 center() const { return (min + max) * 0.5f; }
    
    /// Get extent (half-size)
    glm::vec3 extent() const { return (max - min) * 0.5f; }
    
    /// Get diagonal
    glm::vec3 diagonal() const { return max - min; }
    
    /// Get surface area (for SAH)
    float surfaceArea() const {
        glm::vec3 d = diagonal();
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }
    
    /// Get longest axis (0=x, 1=y, 2=z)
    int longestAxis() const {
        glm::vec3 d = diagonal();
        if (d.x > d.y && d.x > d.z) return 0;
        if (d.y > d.z) return 1;
        return 2;
    }
    
    /// Check if AABB is valid
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
    
    /// Reset to invalid state
    void reset() {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::lowest());
    }
    
    /// Ray-AABB intersection test
    bool intersect(const Ray& ray, float& tNear, float& tFar) const;
};

/**
 * @brief BVH node structure
 */
struct BVHNode {
    AABB bounds;
    uint32_t leftChild = 0;   ///< Index of left child (0 if leaf)
    uint32_t rightChild = 0;  ///< Index of right child (0 if leaf)
    uint32_t firstPrim = 0;   ///< First primitive index (for leaves)
    uint32_t primCount = 0;   ///< Number of primitives (0 if internal node)
    
    bool isLeaf() const { return primCount > 0; }
};

/**
 * @brief Primitive info for BVH construction
 */
struct PrimitiveInfo {
    uint32_t index;     ///< Original triangle index
    AABB bounds;        ///< Triangle bounding box
    glm::vec3 centroid; ///< Triangle centroid
};

/**
 * @brief Bounding Volume Hierarchy for efficient ray tracing
 * 
 * Usage:
 * 1. Create BVH with mesh data: BVH bvh(meshData);
 * 2. Query with ray: BVHHitResult result = bvh.intersect(ray);
 */
class BVH {
public:
    BVH() = default;
    
    /**
     * @brief Build BVH from mesh data
     * @param mesh The mesh to build from
     */
    explicit BVH(const MeshData& mesh);
    
    /**
     * @brief Build BVH from mesh data
     * @param mesh The mesh to build from
     */
    void build(const MeshData& mesh);
    
    /**
     * @brief Clear the BVH
     */
    void clear();
    
    /**
     * @brief Check if BVH is valid
     */
    bool isValid() const { return !m_nodes.empty() && !m_vertices.empty(); }
    
    /**
     * @brief Find closest ray intersection
     * @param ray The ray to test
     * @return Hit result with intersection info
     */
    BVHHitResult intersect(const Ray& ray) const;
    
    /**
     * @brief Check if ray intersects anything (shadow ray test)
     * @param ray The ray to test
     * @param maxDist Maximum distance to check
     * @return true if any intersection found
     */
    bool intersectAny(const Ray& ray, float maxDist = std::numeric_limits<float>::max()) const;
    
    /**
     * @brief Get all triangles that potentially intersect a frustum/box
     * For box selection queries
     * @param frustumPlanes 6 frustum planes (normals pointing inward)
     * @return Vector of triangle indices
     */
    std::vector<uint32_t> queryFrustum(const glm::vec4 frustumPlanes[6]) const;
    
    /**
     * @brief Get all triangles that intersect an AABB
     * @param box The AABB to test
     * @return Vector of triangle indices
     */
    std::vector<uint32_t> queryAABB(const AABB& box) const;
    
    /**
     * @brief Get bounds of entire BVH
     */
    const AABB& bounds() const { return m_nodes.empty() ? m_emptyAABB : m_nodes[0].bounds; }
    
    /**
     * @brief Get node count (for debugging/stats)
     */
    size_t nodeCount() const { return m_nodes.size(); }
    
    /**
     * @brief Get max depth (for debugging/stats)
     */
    int maxDepth() const { return m_maxDepth; }

private:
    // Build methods
    uint32_t buildRecursive(std::vector<PrimitiveInfo>& prims, 
                            uint32_t start, uint32_t end, int depth);
    
    // Intersection helpers
    bool intersectTriangle(const Ray& ray, uint32_t triIndex,
                          float& t, glm::vec3& bary) const;
    
    void intersectNode(const Ray& ray, uint32_t nodeIndex,
                      BVHHitResult& result) const;
    
    bool intersectNodeAny(const Ray& ray, uint32_t nodeIndex, float maxDist) const;
    
    void queryFrustumNode(uint32_t nodeIndex, const glm::vec4 frustumPlanes[6],
                         std::vector<uint32_t>& results) const;
    
    bool aabbInFrustum(const AABB& box, const glm::vec4 frustumPlanes[6]) const;
    
    // Data
    std::vector<BVHNode> m_nodes;
    std::vector<uint32_t> m_primitiveIndices;  ///< Reordered triangle indices
    
    // Mesh reference data (copied for thread safety)
    std::vector<glm::vec3> m_vertices;
    std::vector<uint32_t> m_indices;
    
    int m_maxDepth = 0;
    static constexpr int MAX_LEAF_SIZE = 4;  ///< Max triangles per leaf
    static constexpr int MAX_DEPTH = 64;     ///< Max tree depth
    
    AABB m_emptyAABB;  ///< Empty AABB for empty BVH
};

/**
 * @brief Utility: Compute closest point on triangle to a given point
 */
glm::vec3 closestPointOnTriangle(const glm::vec3& p,
                                 const glm::vec3& v0,
                                 const glm::vec3& v1,
                                 const glm::vec3& v2,
                                 glm::vec3& baryOut);

/**
 * @brief Utility: Compute distance from point to edge
 */
float distanceToEdge(const glm::vec3& p,
                     const glm::vec3& e0,
                     const glm::vec3& e1,
                     float& t);

} // namespace geometry
} // namespace dc3d
