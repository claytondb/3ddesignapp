/**
 * @file BVH.cpp
 * @brief Implementation of Bounding Volume Hierarchy
 */

#include "BVH.h"
#include "MeshData.h"
#include <algorithm>
#include <cmath>

namespace dc3d {
namespace geometry {

// ============================================================================
// AABB Implementation
// ============================================================================

bool AABB::intersect(const Ray& ray, float& tNear, float& tFar) const
{
    // Slab-based ray-AABB intersection
    // FIX: Handle axis-aligned rays (division by zero)
    const float eps = 1e-10f;
    glm::vec3 invDir;
    for (int i = 0; i < 3; ++i) {
        invDir[i] = std::abs(ray.direction[i]) > eps ? 
            1.0f / ray.direction[i] : 
            std::copysign(1e10f, ray.direction[i]);
    }
    
    glm::vec3 t0 = (min - ray.origin) * invDir;
    glm::vec3 t1 = (max - ray.origin) * invDir;
    
    glm::vec3 tSmall = glm::min(t0, t1);
    glm::vec3 tLarge = glm::max(t0, t1);
    
    tNear = std::max({tSmall.x, tSmall.y, tSmall.z, ray.tMin});
    tFar = std::min({tLarge.x, tLarge.y, tLarge.z, ray.tMax});
    
    return tNear <= tFar;
}

// ============================================================================
// BVH Implementation
// ============================================================================

BVH::BVH(const MeshData& mesh)
{
    build(mesh);
}

void BVH::build(const MeshData& mesh)
{
    clear();
    
    if (mesh.isEmpty()) {
        return;
    }
    
    // Copy mesh data for thread safety
    m_vertices = mesh.vertices();
    m_indices = mesh.indices();
    
    size_t numTriangles = mesh.faceCount();
    if (numTriangles == 0) {
        return;
    }
    
    // Build primitive info list
    std::vector<PrimitiveInfo> primitiveInfo(numTriangles);
    
    for (size_t i = 0; i < numTriangles; ++i) {
        uint32_t i0 = m_indices[i * 3 + 0];
        uint32_t i1 = m_indices[i * 3 + 1];
        uint32_t i2 = m_indices[i * 3 + 2];
        
        const glm::vec3& v0 = m_vertices[i0];
        const glm::vec3& v1 = m_vertices[i1];
        const glm::vec3& v2 = m_vertices[i2];
        
        primitiveInfo[i].index = static_cast<uint32_t>(i);
        primitiveInfo[i].bounds.reset();
        primitiveInfo[i].bounds.expand(v0);
        primitiveInfo[i].bounds.expand(v1);
        primitiveInfo[i].bounds.expand(v2);
        primitiveInfo[i].centroid = (v0 + v1 + v2) / 3.0f;
    }
    
    // Reserve nodes (rough estimate: 2*n - 1 for full binary tree)
    m_nodes.reserve(numTriangles * 2);
    m_primitiveIndices.reserve(numTriangles);
    
    // Build tree recursively
    m_maxDepth = 0;
    buildRecursive(primitiveInfo, 0, static_cast<uint32_t>(numTriangles), 0);
}

void BVH::clear()
{
    m_nodes.clear();
    m_primitiveIndices.clear();
    m_vertices.clear();
    m_indices.clear();
    m_maxDepth = 0;
}

uint32_t BVH::buildRecursive(std::vector<PrimitiveInfo>& prims,
                              uint32_t start, uint32_t end, int depth)
{
    m_maxDepth = std::max(m_maxDepth, depth);
    
    uint32_t nodeIndex = static_cast<uint32_t>(m_nodes.size());
    m_nodes.emplace_back();
    BVHNode& node = m_nodes[nodeIndex];
    
    // Compute bounds of all primitives in range
    node.bounds.reset();
    for (uint32_t i = start; i < end; ++i) {
        node.bounds.expand(prims[i].bounds);
    }
    
    uint32_t numPrims = end - start;
    
    // Create leaf if few primitives or max depth reached
    if (numPrims <= MAX_LEAF_SIZE || depth >= MAX_DEPTH) {
        node.firstPrim = static_cast<uint32_t>(m_primitiveIndices.size());
        node.primCount = numPrims;
        
        for (uint32_t i = start; i < end; ++i) {
            m_primitiveIndices.push_back(prims[i].index);
        }
        
        return nodeIndex;
    }
    
    // Compute centroid bounds for splitting
    AABB centroidBounds;
    for (uint32_t i = start; i < end; ++i) {
        centroidBounds.expand(prims[i].centroid);
    }
    
    int axis = centroidBounds.longestAxis();
    
    // Check for degenerate case (all centroids in same place)
    float extent = centroidBounds.max[axis] - centroidBounds.min[axis];
    if (extent < 1e-10f) {
        // Can't split meaningfully, create leaf
        node.firstPrim = static_cast<uint32_t>(m_primitiveIndices.size());
        node.primCount = numPrims;
        
        for (uint32_t i = start; i < end; ++i) {
            m_primitiveIndices.push_back(prims[i].index);
        }
        
        return nodeIndex;
    }
    
    // Use SAH (Surface Area Heuristic) for best split
    constexpr int NUM_BUCKETS = 12;
    
    struct Bucket {
        uint32_t count = 0;
        AABB bounds;
    };
    Bucket buckets[NUM_BUCKETS];
    
    for (uint32_t i = start; i < end; ++i) {
        int b = static_cast<int>(
            NUM_BUCKETS * (prims[i].centroid[axis] - centroidBounds.min[axis]) / extent);
        b = std::clamp(b, 0, NUM_BUCKETS - 1);
        buckets[b].count++;
        buckets[b].bounds.expand(prims[i].bounds);
    }
    
    // Compute costs for splitting after each bucket
    float costs[NUM_BUCKETS - 1];
    
    for (int i = 0; i < NUM_BUCKETS - 1; ++i) {
        AABB b0, b1;
        uint32_t count0 = 0, count1 = 0;
        
        for (int j = 0; j <= i; ++j) {
            if (buckets[j].count > 0) {
                b0.expand(buckets[j].bounds);
                count0 += buckets[j].count;
            }
        }
        
        for (int j = i + 1; j < NUM_BUCKETS; ++j) {
            if (buckets[j].count > 0) {
                b1.expand(buckets[j].bounds);
                count1 += buckets[j].count;
            }
        }
        
        float area0 = b0.isValid() ? b0.surfaceArea() : 0.0f;
        float area1 = b1.isValid() ? b1.surfaceArea() : 0.0f;
        costs[i] = 0.125f + (count0 * area0 + count1 * area1) / node.bounds.surfaceArea();
    }
    
    // Find best split
    float minCost = costs[0];
    int minBucket = 0;
    for (int i = 1; i < NUM_BUCKETS - 1; ++i) {
        if (costs[i] < minCost) {
            minCost = costs[i];
            minBucket = i;
        }
    }
    
    // Check if split is worth it
    float leafCost = static_cast<float>(numPrims);
    
    if (numPrims > MAX_LEAF_SIZE || minCost < leafCost) {
        // Partition primitives
        auto midIter = std::partition(
            prims.begin() + start,
            prims.begin() + end,
            [&](const PrimitiveInfo& pi) {
                int b = static_cast<int>(
                    NUM_BUCKETS * (pi.centroid[axis] - centroidBounds.min[axis]) / extent);
                b = std::clamp(b, 0, NUM_BUCKETS - 1);
                return b <= minBucket;
            });
        
        uint32_t mid = static_cast<uint32_t>(midIter - prims.begin());
        
        // Ensure we actually split
        if (mid == start || mid == end) {
            mid = (start + end) / 2;
            std::nth_element(
                prims.begin() + start,
                prims.begin() + mid,
                prims.begin() + end,
                [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) {
                    return a.centroid[axis] < b.centroid[axis];
                });
        }
        
        // Build children
        node.leftChild = buildRecursive(prims, start, mid, depth + 1);
        node.rightChild = buildRecursive(prims, mid, end, depth + 1);
        node.primCount = 0;  // Internal node
        
        return nodeIndex;
    }
    
    // Create leaf
    node.firstPrim = static_cast<uint32_t>(m_primitiveIndices.size());
    node.primCount = numPrims;
    
    for (uint32_t i = start; i < end; ++i) {
        m_primitiveIndices.push_back(prims[i].index);
    }
    
    return nodeIndex;
}

bool BVH::intersectTriangle(const Ray& ray, uint32_t triIndex,
                            float& t, glm::vec3& bary) const
{
    // FIX: Bounds check on triangle and vertex indices
    if (triIndex * 3 + 2 >= m_indices.size()) return false;
    
    // Möller–Trumbore intersection algorithm
    uint32_t i0 = m_indices[triIndex * 3 + 0];
    uint32_t i1 = m_indices[triIndex * 3 + 1];
    uint32_t i2 = m_indices[triIndex * 3 + 2];
    
    // FIX: Validate vertex indices
    if (i0 >= m_vertices.size() || i1 >= m_vertices.size() || i2 >= m_vertices.size()) {
        return false;
    }
    
    const glm::vec3& v0 = m_vertices[i0];
    const glm::vec3& v1 = m_vertices[i1];
    const glm::vec3& v2 = m_vertices[i2];
    
    glm::vec3 e1 = v1 - v0;
    glm::vec3 e2 = v2 - v0;
    
    glm::vec3 h = glm::cross(ray.direction, e2);
    float a = glm::dot(e1, h);
    
    // Check if ray is parallel to triangle
    if (std::abs(a) < 1e-10f) {
        return false;
    }
    
    float f = 1.0f / a;
    glm::vec3 s = ray.origin - v0;
    float u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    
    glm::vec3 q = glm::cross(s, e1);
    float v = f * glm::dot(ray.direction, q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    
    t = f * glm::dot(e2, q);
    
    if (t < ray.tMin || t > ray.tMax) {
        return false;
    }
    
    bary = glm::vec3(1.0f - u - v, u, v);  // w, u, v (barycentric)
    return true;
}

void BVH::intersectNode(const Ray& ray, uint32_t nodeIndex,
                        BVHHitResult& result) const
{
    const BVHNode& node = m_nodes[nodeIndex];
    
    float tNear, tFar;
    if (!node.bounds.intersect(ray, tNear, tFar)) {
        return;
    }
    
    // Early out if we already have a closer hit
    if (tNear > result.t) {
        return;
    }
    
    if (node.isLeaf()) {
        // Test all triangles in leaf
        for (uint32_t i = 0; i < node.primCount; ++i) {
            uint32_t triIndex = m_primitiveIndices[node.firstPrim + i];
            
            float t;
            glm::vec3 bary;
            if (intersectTriangle(ray, triIndex, t, bary)) {
                if (t < result.t) {
                    result.hit = true;
                    result.t = t;
                    result.faceIndex = triIndex;
                    result.point = ray.at(t);
                    result.barycentric = bary;
                    
                    // Get vertex indices
                    result.indices[0] = m_indices[triIndex * 3 + 0];
                    result.indices[1] = m_indices[triIndex * 3 + 1];
                    result.indices[2] = m_indices[triIndex * 3 + 2];
                    
                    // Compute interpolated normal
                    const glm::vec3& v0 = m_vertices[result.indices[0]];
                    const glm::vec3& v1 = m_vertices[result.indices[1]];
                    const glm::vec3& v2 = m_vertices[result.indices[2]];
                    result.normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                }
            }
        }
    } else {
        // Traverse children (front-to-back)
        const BVHNode& left = m_nodes[node.leftChild];
        const BVHNode& right = m_nodes[node.rightChild];
        
        float tLeftNear, tLeftFar, tRightNear, tRightFar;
        bool hitLeft = left.bounds.intersect(ray, tLeftNear, tLeftFar);
        bool hitRight = right.bounds.intersect(ray, tRightNear, tRightFar);
        
        if (hitLeft && hitRight) {
            // Visit closer child first
            if (tLeftNear < tRightNear) {
                intersectNode(ray, node.leftChild, result);
                if (tRightNear < result.t) {
                    intersectNode(ray, node.rightChild, result);
                }
            } else {
                intersectNode(ray, node.rightChild, result);
                if (tLeftNear < result.t) {
                    intersectNode(ray, node.leftChild, result);
                }
            }
        } else if (hitLeft) {
            intersectNode(ray, node.leftChild, result);
        } else if (hitRight) {
            intersectNode(ray, node.rightChild, result);
        }
    }
}

BVHHitResult BVH::intersect(const Ray& ray) const
{
    BVHHitResult result;
    
    if (m_nodes.empty()) {
        return result;
    }
    
    intersectNode(ray, 0, result);
    return result;
}

bool BVH::intersectNodeAny(const Ray& ray, uint32_t nodeIndex, float maxDist) const
{
    const BVHNode& node = m_nodes[nodeIndex];
    
    float tNear, tFar;
    if (!node.bounds.intersect(ray, tNear, tFar)) {
        return false;
    }
    
    if (tNear > maxDist) {
        return false;
    }
    
    if (node.isLeaf()) {
        for (uint32_t i = 0; i < node.primCount; ++i) {
            uint32_t triIndex = m_primitiveIndices[node.firstPrim + i];
            
            float t;
            glm::vec3 bary;
            if (intersectTriangle(ray, triIndex, t, bary)) {
                if (t < maxDist) {
                    return true;
                }
            }
        }
        return false;
    }
    
    return intersectNodeAny(ray, node.leftChild, maxDist) ||
           intersectNodeAny(ray, node.rightChild, maxDist);
}

bool BVH::intersectAny(const Ray& ray, float maxDist) const
{
    if (m_nodes.empty()) {
        return false;
    }
    
    return intersectNodeAny(ray, 0, maxDist);
}

bool BVH::aabbInFrustum(const AABB& box, const glm::vec4 frustumPlanes[6]) const
{
    // Test AABB against each frustum plane
    for (int i = 0; i < 6; ++i) {
        glm::vec3 normal(frustumPlanes[i].x, frustumPlanes[i].y, frustumPlanes[i].z);
        float d = frustumPlanes[i].w;
        
        // Find the p-vertex (furthest along normal direction)
        glm::vec3 p = box.min;
        if (normal.x >= 0) p.x = box.max.x;
        if (normal.y >= 0) p.y = box.max.y;
        if (normal.z >= 0) p.z = box.max.z;
        
        // If p-vertex is outside, box is fully outside
        if (glm::dot(normal, p) + d < 0) {
            return false;
        }
    }
    
    return true;
}

void BVH::queryFrustumNode(uint32_t nodeIndex, const glm::vec4 frustumPlanes[6],
                           std::vector<uint32_t>& results) const
{
    const BVHNode& node = m_nodes[nodeIndex];
    
    if (!aabbInFrustum(node.bounds, frustumPlanes)) {
        return;
    }
    
    if (node.isLeaf()) {
        for (uint32_t i = 0; i < node.primCount; ++i) {
            results.push_back(m_primitiveIndices[node.firstPrim + i]);
        }
    } else {
        queryFrustumNode(node.leftChild, frustumPlanes, results);
        queryFrustumNode(node.rightChild, frustumPlanes, results);
    }
}

std::vector<uint32_t> BVH::queryFrustum(const glm::vec4 frustumPlanes[6]) const
{
    std::vector<uint32_t> results;
    
    if (!m_nodes.empty()) {
        queryFrustumNode(0, frustumPlanes, results);
    }
    
    return results;
}

std::vector<uint32_t> BVH::queryAABB(const AABB& box) const
{
    std::vector<uint32_t> results;
    
    if (m_nodes.empty()) {
        return results;
    }
    
    // Stack-based traversal
    std::vector<uint32_t> stack;
    stack.reserve(64);
    stack.push_back(0);
    
    while (!stack.empty()) {
        uint32_t nodeIndex = stack.back();
        stack.pop_back();
        
        const BVHNode& node = m_nodes[nodeIndex];
        
        // Check AABB overlap
        if (box.max.x < node.bounds.min.x || box.min.x > node.bounds.max.x ||
            box.max.y < node.bounds.min.y || box.min.y > node.bounds.max.y ||
            box.max.z < node.bounds.min.z || box.min.z > node.bounds.max.z) {
            continue;
        }
        
        if (node.isLeaf()) {
            for (uint32_t i = 0; i < node.primCount; ++i) {
                results.push_back(m_primitiveIndices[node.firstPrim + i]);
            }
        } else {
            stack.push_back(node.leftChild);
            stack.push_back(node.rightChild);
        }
    }
    
    return results;
}

// ============================================================================
// Utility Functions
// ============================================================================

glm::vec3 closestPointOnTriangle(const glm::vec3& p,
                                 const glm::vec3& v0,
                                 const glm::vec3& v1,
                                 const glm::vec3& v2,
                                 glm::vec3& baryOut)
{
    glm::vec3 ab = v1 - v0;
    glm::vec3 ac = v2 - v0;
    glm::vec3 ap = p - v0;
    
    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    
    // Vertex region outside v0
    if (d1 <= 0.0f && d2 <= 0.0f) {
        baryOut = glm::vec3(1.0f, 0.0f, 0.0f);
        return v0;
    }
    
    glm::vec3 bp = p - v1;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    
    // Vertex region outside v1
    if (d3 >= 0.0f && d4 <= d3) {
        baryOut = glm::vec3(0.0f, 1.0f, 0.0f);
        return v1;
    }
    
    // Edge region v0-v1
    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        baryOut = glm::vec3(1.0f - v, v, 0.0f);
        return v0 + v * ab;
    }
    
    glm::vec3 cp = p - v2;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    
    // Vertex region outside v2
    if (d6 >= 0.0f && d5 <= d6) {
        baryOut = glm::vec3(0.0f, 0.0f, 1.0f);
        return v2;
    }
    
    // Edge region v0-v2
    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        baryOut = glm::vec3(1.0f - w, 0.0f, w);
        return v0 + w * ac;
    }
    
    // Edge region v1-v2
    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        baryOut = glm::vec3(0.0f, 1.0f - w, w);
        return v1 + w * (v2 - v1);
    }
    
    // Inside triangle
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    baryOut = glm::vec3(1.0f - v - w, v, w);
    return v0 + ab * v + ac * w;
}

float distanceToEdge(const glm::vec3& p,
                     const glm::vec3& e0,
                     const glm::vec3& e1,
                     float& t)
{
    glm::vec3 edge = e1 - e0;
    float len2 = glm::dot(edge, edge);
    
    if (len2 < 1e-10f) {
        t = 0.0f;
        return glm::length(p - e0);
    }
    
    t = glm::clamp(glm::dot(p - e0, edge) / len2, 0.0f, 1.0f);
    glm::vec3 closest = e0 + t * edge;
    return glm::length(p - closest);
}

} // namespace geometry
} // namespace dc3d
