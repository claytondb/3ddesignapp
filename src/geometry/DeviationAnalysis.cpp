/**
 * @file DeviationAnalysis.cpp
 * @brief Implementation of mesh deviation analysis
 */

#include "DeviationAnalysis.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace dc3d {
namespace geometry {

// FIX Bug 28: Define named constants for magic numbers
namespace {
    constexpr float EPSILON_RAY = 1e-7f;         // For ray-triangle intersection
    constexpr int NUM_SIGN_RAYS = 5;             // Number of rays for sign determination majority vote
    constexpr int RANDOM_SEED = 42;              // Seed for reproducible random directions
} // anonymous namespace

// ============================================================================
// KDBox
// ============================================================================

float KDBox::distanceSquared(const glm::vec3& point) const {
    float dx = std::max(0.0f, std::max(min.x - point.x, point.x - max.x));
    float dy = std::max(0.0f, std::max(min.y - point.y, point.y - max.y));
    float dz = std::max(0.0f, std::max(min.z - point.z, point.z - max.z));
    return dx * dx + dy * dy + dz * dz;
}

// ============================================================================
// KDTree
// ============================================================================

void KDTree::build(const MeshData& mesh, ProgressCallback progress) {
    mesh_ = &mesh;
    
    size_t faceCount = mesh.faceCount();
    if (faceCount == 0) {
        root_ = nullptr;
        return;
    }
    
    std::vector<uint32_t> triangleIndices(faceCount);
    std::iota(triangleIndices.begin(), triangleIndices.end(), 0);
    
    if (progress) progress(0.1f);
    
    root_ = buildRecursive(triangleIndices, 0);
    
    if (progress) progress(1.0f);
}

std::unique_ptr<KDNode> KDTree::buildRecursive(
    std::vector<uint32_t>& triangleIndices,
    int depth
) {
    if (triangleIndices.empty()) {
        return nullptr;
    }
    
    auto node = std::make_unique<KDNode>();
    
    // Compute bounds of all triangles
    for (uint32_t ti : triangleIndices) {
        KDBox triBounds = computeTriangleBounds(ti);
        node->bounds.expand(triBounds.min);
        node->bounds.expand(triBounds.max);
    }
    
    // Leaf node if small enough
    if (triangleIndices.size() == 1) {
        node->triangleIndex = triangleIndices[0];
        node->splitAxis = -1;
        return node;
    }
    
    // Choose split axis (cycle through X, Y, Z)
    int axis = depth % 3;
    node->splitAxis = axis;
    
    // Sort by centroid along split axis
    std::sort(triangleIndices.begin(), triangleIndices.end(),
        [this, axis](uint32_t a, uint32_t b) {
            glm::vec3 ca = computeTriangleCentroid(a);
            glm::vec3 cb = computeTriangleCentroid(b);
            return ca[axis] < cb[axis];
        }
    );
    
    // Split at median
    size_t mid = triangleIndices.size() / 2;
    glm::vec3 midCentroid = computeTriangleCentroid(triangleIndices[mid]);
    node->splitPos = midCentroid[axis];
    
    // If only 2 triangles, make this a leaf
    if (triangleIndices.size() <= 4) {
        node->triangleIndex = triangleIndices[0];  // Store first, search all
        node->splitAxis = -1;
        
        // Store remaining triangles
        if (triangleIndices.size() > 1) {
            std::vector<uint32_t> leftTris(triangleIndices.begin(), triangleIndices.begin() + mid);
            std::vector<uint32_t> rightTris(triangleIndices.begin() + mid, triangleIndices.end());
            node->left = buildRecursive(leftTris, depth + 1);
            node->right = buildRecursive(rightTris, depth + 1);
            node->splitAxis = axis;
        }
        return node;
    }
    
    // Split triangles
    std::vector<uint32_t> leftTris(triangleIndices.begin(), triangleIndices.begin() + mid);
    std::vector<uint32_t> rightTris(triangleIndices.begin() + mid, triangleIndices.end());
    
    node->left = buildRecursive(leftTris, depth + 1);
    node->right = buildRecursive(rightTris, depth + 1);
    
    return node;
}

float KDTree::findClosestPoint(
    const glm::vec3& point,
    glm::vec3& closestPoint,
    uint32_t& closestTriangle
) const {
    if (!root_) {
        closestPoint = point;
        closestTriangle = 0;
        return std::numeric_limits<float>::max();
    }
    
    float bestDistSq = std::numeric_limits<float>::max();
    findClosestRecursive(root_.get(), point, bestDistSq, closestPoint, closestTriangle);
    
    return std::sqrt(bestDistSq);
}

float KDTree::findClosestDistance(const glm::vec3& point) const {
    glm::vec3 closestPoint;
    uint32_t closestTriangle;
    return findClosestPoint(point, closestPoint, closestTriangle);
}

void KDTree::findClosestRecursive(
    const KDNode* node,
    const glm::vec3& point,
    float& bestDistSq,
    glm::vec3& bestPoint,
    uint32_t& bestTriangle
) const {
    if (!node) return;
    
    // Early out if node's bounding box is farther than current best
    float boxDistSq = node->bounds.distanceSquared(point);
    if (boxDistSq >= bestDistSq) {
        return;
    }
    
    // Leaf node: check triangle
    if (node->splitAxis < 0) {
        glm::vec3 closest = closestPointOnTriangle(point, node->triangleIndex);
        float distSq = glm::dot(closest - point, closest - point);
        
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestPoint = closest;
            bestTriangle = node->triangleIndex;
        }
        return;
    }
    
    // Internal node: traverse children
    float splitDist = point[node->splitAxis] - node->splitPos;
    
    KDNode* nearChild = (splitDist < 0) ? node->left.get() : node->right.get();
    KDNode* farChild = (splitDist < 0) ? node->right.get() : node->left.get();
    
    // Search near child first
    findClosestRecursive(nearChild, point, bestDistSq, bestPoint, bestTriangle);
    
    // Only search far child if it could contain closer point
    if (splitDist * splitDist < bestDistSq) {
        findClosestRecursive(farChild, point, bestDistSq, bestPoint, bestTriangle);
    }
}

glm::vec3 KDTree::closestPointOnTriangle(
    const glm::vec3& point,
    uint32_t triangleIndex
) const {
    const auto& vertices = mesh_->vertices();
    const auto& indices = mesh_->indices();
    
    const glm::vec3& v0 = vertices[indices[triangleIndex * 3]];
    const glm::vec3& v1 = vertices[indices[triangleIndex * 3 + 1]];
    const glm::vec3& v2 = vertices[indices[triangleIndex * 3 + 2]];
    
    return DeviationAnalysis::closestPointOnTriangle(point, v0, v1, v2);
}

KDBox KDTree::computeTriangleBounds(uint32_t triangleIndex) const {
    const auto& vertices = mesh_->vertices();
    const auto& indices = mesh_->indices();
    
    const glm::vec3& v0 = vertices[indices[triangleIndex * 3]];
    const glm::vec3& v1 = vertices[indices[triangleIndex * 3 + 1]];
    const glm::vec3& v2 = vertices[indices[triangleIndex * 3 + 2]];
    
    KDBox box;
    box.expand(v0);
    box.expand(v1);
    box.expand(v2);
    return box;
}

glm::vec3 KDTree::computeTriangleCentroid(uint32_t triangleIndex) const {
    const auto& vertices = mesh_->vertices();
    const auto& indices = mesh_->indices();
    
    const glm::vec3& v0 = vertices[indices[triangleIndex * 3]];
    const glm::vec3& v1 = vertices[indices[triangleIndex * 3 + 1]];
    const glm::vec3& v2 = vertices[indices[triangleIndex * 3 + 2]];
    
    return (v0 + v1 + v2) / 3.0f;
}

// ============================================================================
// DeviationAnalysis
// ============================================================================

std::vector<float> DeviationAnalysis::computeDeviation(
    const MeshData& meshA,
    const MeshData& meshB,
    const DeviationConfig& config,
    ProgressCallback progress
) {
    const auto& vertices = meshA.vertices();
    std::vector<float> deviations(vertices.size(), 0.0f);
    
    if (vertices.empty() || meshB.isEmpty()) {
        return deviations;
    }
    
    // Build KD-tree for acceleration
    KDTree kdTree;
    if (config.useKDTree) {
        kdTree.build(meshB, [&progress](float p) {
            if (progress) return progress(p * 0.2f);
            return true;
        });
    }
    
    // Compute distances
    size_t totalVertices = vertices.size();
    for (size_t i = 0; i < totalVertices; ++i) {
        const glm::vec3& point = vertices[i];
        
        if (config.useKDTree && kdTree.isBuilt()) {
            deviations[i] = kdTree.findClosestDistance(point);
        } else {
            glm::vec3 closestPoint;
            deviations[i] = pointToMeshDistance(point, meshB, closestPoint);
        }
        
        if (progress && (i % 1000 == 0)) {
            float p = 0.2f + 0.8f * static_cast<float>(i) / totalVertices;
            if (!progress(p)) break;
        }
    }
    
    if (progress) progress(1.0f);
    
    return deviations;
}

std::vector<float> DeviationAnalysis::computeSignedDeviation(
    const MeshData& meshA,
    const MeshData& meshB,
    ProgressCallback progress
) {
    const auto& vertices = meshA.vertices();
    std::vector<float> deviations(vertices.size(), 0.0f);
    
    if (vertices.empty() || meshB.isEmpty()) {
        return deviations;
    }
    
    // Build KD-tree for acceleration
    KDTree kdTree;
    kdTree.build(meshB, [&progress](float p) {
        if (progress) return progress(p * 0.2f);
        return true;
    });
    
    // Compute signed distances
    size_t totalVertices = vertices.size();
    for (size_t i = 0; i < totalVertices; ++i) {
        const glm::vec3& point = vertices[i];
        
        glm::vec3 closestPoint;
        uint32_t closestTriangle;
        float distance = kdTree.findClosestPoint(point, closestPoint, closestTriangle);
        
        // Determine sign using ray casting
        // Use multiple random rays and take majority vote for robustness
        // FIX Bug 16: Use thread_local to make function thread-safe and reentrant
        thread_local std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        int insideCount = 0;
        const int numRays = 5;
        
        for (int r = 0; r < numRays; ++r) {
            glm::vec3 direction = glm::normalize(glm::vec3(
                dist(rng), dist(rng), dist(rng)
            ));
            
            int intersections = countRayIntersections(point, direction, meshB);
            if (intersections % 2 == 1) {
                insideCount++;
            }
        }
        
        bool inside = insideCount > numRays / 2;
        deviations[i] = inside ? -distance : distance;
        
        if (progress && (i % 1000 == 0)) {
            float p = 0.2f + 0.8f * static_cast<float>(i) / totalVertices;
            if (!progress(p)) break;
        }
    }
    
    if (progress) progress(1.0f);
    
    return deviations;
}

DeviationStats DeviationAnalysis::computeStats(
    const std::vector<float>& deviations,
    float toleranceThreshold
) {
    DeviationStats stats;
    stats.totalPoints = deviations.size();
    stats.toleranceThreshold = toleranceThreshold;
    
    if (deviations.empty()) {
        return stats;
    }
    
    // Convert to absolute values for unsigned stats
    std::vector<float> absDeviations = deviations;
    for (float& d : absDeviations) {
        d = std::abs(d);
    }
    
    // Min/max
    auto [minIt, maxIt] = std::minmax_element(absDeviations.begin(), absDeviations.end());
    stats.minDeviation = *minIt;
    stats.maxDeviation = *maxIt;
    
    // Average and RMS
    double sum = 0.0;
    double sqSum = 0.0;
    for (float d : absDeviations) {
        sum += d;
        sqSum += d * d;
    }
    stats.avgDeviation = static_cast<float>(sum / deviations.size());
    stats.rmsDeviation = static_cast<float>(std::sqrt(sqSum / deviations.size()));
    
    // Standard deviation
    double varSum = 0.0;
    for (float d : absDeviations) {
        double diff = d - stats.avgDeviation;
        varSum += diff * diff;
    }
    stats.stddevDeviation = static_cast<float>(std::sqrt(varSum / deviations.size()));
    
    // Percentiles
    std::vector<float> sorted = absDeviations;
    std::sort(sorted.begin(), sorted.end());
    
    // FIX: Add explicit bounds checking for percentile calculation
    auto percentile = [&sorted](float p) -> float {
        if (sorted.empty()) return 0.0f;
        size_t idx = std::min(
            static_cast<size_t>(p * (sorted.size() - 1)),
            sorted.size() - 1
        );
        return sorted[idx];
    };
    
    stats.percentile50 = percentile(0.5f);
    stats.percentile90 = percentile(0.9f);
    stats.percentile95 = percentile(0.95f);
    stats.percentile99 = percentile(0.99f);
    
    // Count within tolerance
    stats.pointsWithinTolerance = std::count_if(
        absDeviations.begin(), absDeviations.end(),
        [toleranceThreshold](float d) { return d <= toleranceThreshold; }
    );
    
    return stats;
}

DeviationStats DeviationAnalysis::computeSignedStats(
    const std::vector<float>& deviations,
    float toleranceThreshold
) {
    DeviationStats stats = computeStats(deviations, toleranceThreshold);
    
    if (deviations.empty()) {
        return stats;
    }
    
    // Signed statistics
    auto [minIt, maxIt] = std::minmax_element(deviations.begin(), deviations.end());
    stats.minSigned = *minIt;
    stats.maxSigned = *maxIt;
    
    double signedSum = std::accumulate(deviations.begin(), deviations.end(), 0.0);
    stats.avgSigned = static_cast<float>(signedSum / deviations.size());
    
    return stats;
}

float DeviationAnalysis::pointToMeshDistance(
    const glm::vec3& point,
    const MeshData& mesh,
    glm::vec3& closestPoint
) {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    float minDistSq = std::numeric_limits<float>::max();
    closestPoint = point;
    
    size_t faceCount = indices.size() / 3;
    for (size_t fi = 0; fi < faceCount; ++fi) {
        const glm::vec3& v0 = vertices[indices[fi * 3]];
        const glm::vec3& v1 = vertices[indices[fi * 3 + 1]];
        const glm::vec3& v2 = vertices[indices[fi * 3 + 2]];
        
        glm::vec3 closest = closestPointOnTriangle(point, v0, v1, v2);
        float distSq = glm::dot(closest - point, closest - point);
        
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestPoint = closest;
        }
    }
    
    return std::sqrt(minDistSq);
}

float DeviationAnalysis::pointToMeshSignedDistance(
    const glm::vec3& point,
    const MeshData& mesh,
    glm::vec3& closestPoint
) {
    float distance = pointToMeshDistance(point, mesh, closestPoint);
    
    // Determine sign using ray casting
    int intersections = countRayIntersections(point, glm::vec3(1, 0, 0), mesh);
    
    return (intersections % 2 == 1) ? -distance : distance;
}

std::vector<size_t> DeviationAnalysis::createHistogram(
    const std::vector<float>& deviations,
    size_t numBins,
    float minVal,
    float maxVal
) {
    std::vector<size_t> histogram(numBins, 0);
    
    if (deviations.empty()) {
        return histogram;
    }
    
    // Auto-compute range if needed
    if (minVal >= maxVal) {
        auto [minIt, maxIt] = std::minmax_element(deviations.begin(), deviations.end());
        minVal = *minIt;
        maxVal = *maxIt;
    }
    
    float range = maxVal - minVal;
    if (range <= 0) {
        // FIX Bug 27: Document behavior when all values are identical
        // All values go in bin 0, which is technically correct for a single-value distribution
        histogram[0] = deviations.size();
        return histogram;
    }
    
    for (float d : deviations) {
        float normalized = (d - minVal) / range;
        size_t bin = static_cast<size_t>(normalized * (numBins - 1));
        bin = std::min(bin, numBins - 1);
        histogram[bin]++;
    }
    
    return histogram;
}

glm::vec3 DeviationAnalysis::closestPointOnTriangle(
    const glm::vec3& point,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2
) {
    // Compute vectors
    glm::vec3 edge0 = v1 - v0;
    glm::vec3 edge1 = v2 - v0;
    glm::vec3 v0p = v0 - point;
    
    float a = glm::dot(edge0, edge0);
    float b = glm::dot(edge0, edge1);
    float c = glm::dot(edge1, edge1);
    float d = glm::dot(edge0, v0p);
    float e = glm::dot(edge1, v0p);
    
    float det = a * c - b * b;
    float s = b * e - c * d;
    float t = b * d - a * e;
    
    if (s + t <= det) {
        if (s < 0) {
            if (t < 0) {
                // Region 4
                if (d < 0) {
                    t = 0;
                    s = std::clamp(-d / a, 0.0f, 1.0f);
                } else {
                    s = 0;
                    t = std::clamp(-e / c, 0.0f, 1.0f);
                }
            } else {
                // Region 3
                s = 0;
                t = std::clamp(-e / c, 0.0f, 1.0f);
            }
        } else if (t < 0) {
            // Region 5
            t = 0;
            s = std::clamp(-d / a, 0.0f, 1.0f);
        } else {
            // Region 0
            float invDet = 1.0f / det;
            s *= invDet;
            t *= invDet;
        }
    } else {
        if (s < 0) {
            // Region 2
            float tmp0 = b + d;
            float tmp1 = c + e;
            if (tmp1 > tmp0) {
                float numer = tmp1 - tmp0;
                float denom = a - 2 * b + c;
                s = std::clamp(numer / denom, 0.0f, 1.0f);
                t = 1 - s;
            } else {
                s = 0;
                t = std::clamp(-e / c, 0.0f, 1.0f);
            }
        } else if (t < 0) {
            // Region 6
            float tmp0 = b + e;
            float tmp1 = a + d;
            if (tmp1 > tmp0) {
                float numer = tmp1 - tmp0;
                float denom = a - 2 * b + c;
                t = std::clamp(numer / denom, 0.0f, 1.0f);
                s = 1 - t;
            } else {
                t = 0;
                s = std::clamp(-d / a, 0.0f, 1.0f);
            }
        } else {
            // Region 1
            float numer = (c + e) - (b + d);
            float denom = a - 2 * b + c;
            s = std::clamp(numer / denom, 0.0f, 1.0f);
            t = 1 - s;
        }
    }
    
    return v0 + s * edge0 + t * edge1;
}

bool DeviationAnalysis::rayTriangleIntersect(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    float& t
) {
    const float EPSILON = 1e-7f;
    
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(direction, edge2);
    float a = glm::dot(edge1, h);
    
    if (std::abs(a) < EPSILON) {
        return false;
    }
    
    float f = 1.0f / a;
    glm::vec3 s = origin - v0;
    float u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(direction, q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    
    t = f * glm::dot(edge2, q);
    
    return t > EPSILON;
}

int DeviationAnalysis::countRayIntersections(
    const glm::vec3& point,
    const glm::vec3& direction,
    const MeshData& mesh
) {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    int count = 0;
    size_t faceCount = indices.size() / 3;
    
    for (size_t fi = 0; fi < faceCount; ++fi) {
        const glm::vec3& v0 = vertices[indices[fi * 3]];
        const glm::vec3& v1 = vertices[indices[fi * 3 + 1]];
        const glm::vec3& v2 = vertices[indices[fi * 3 + 2]];
        
        float t;
        if (rayTriangleIntersect(point, direction, v0, v1, v2, t)) {
            count++;
        }
    }
    
    return count;
}

} // namespace geometry
} // namespace dc3d
