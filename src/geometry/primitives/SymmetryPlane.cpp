/**
 * @file SymmetryPlane.cpp
 * @brief Implementation of symmetry plane detection
 */

#include "SymmetryPlane.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace dc3d {
namespace geometry {

SymmetryPlane::SymmetryPlane(const Plane& plane)
    : m_plane(plane)
{
}

SymmetryResult SymmetryPlane::detect(const MeshData& mesh, 
                                      const SymmetryOptions& options) {
    return detect(mesh.vertices(), options);
}

SymmetryResult SymmetryPlane::detect(const std::vector<glm::vec3>& points,
                                      const SymmetryOptions& options) {
    SymmetryResult bestResult;
    
    if (points.size() < 4) {
        bestResult.errorMessage = "Need at least 4 points";
        return bestResult;
    }
    
    // Compute bounds for tolerance
    glm::vec3 center, extent;
    computeBoundsAndCenter(points, center, extent);
    float tolerance = options.matchTolerance * glm::length(extent);
    
    // Generate candidate planes
    auto candidates = generateCandidates(points, options);
    
    // Test each candidate
    float bestQuality = -1.0f;
    
    for (const auto& candidate : candidates) {
        // Quick evaluation
        float quality = evaluateSymmetry(points, candidate, tolerance);
        
        if (quality > bestQuality) {
            bestQuality = quality;
            m_plane = candidate;
        }
    }
    
    // Refine the best plane
    if (bestQuality > 0.0f) {
        m_plane = refineGradient(points, m_plane, options.refinementSteps);
        
        // Get detailed result
        bestResult.found = true;
        bestResult.symmetryPlane = m_plane;
        bestResult.quality = evaluateSymmetry(points, m_plane, tolerance);
        
        // Compute detailed metrics
        float sumDev = 0.0f;
        float maxDev = 0.0f;
        size_t matched = 0;
        
        for (const auto& p : points) {
            glm::vec3 reflected = reflectPoint(p);
            
            // Find closest point to reflection
            float minDist = std::numeric_limits<float>::max();
            for (const auto& q : points) {
                float d = glm::length(reflected - q);
                minDist = std::min(minDist, d);
            }
            
            if (minDist <= tolerance) {
                sumDev += minDist;
                maxDev = std::max(maxDev, minDist);
                ++matched;
            }
        }
        
        if (matched > 0) {
            bestResult.avgDeviation = sumDev / matched;
        }
        bestResult.maxDeviation = maxDev;
        bestResult.matchedPairs = matched / 2;  // Pairs
    }
    
    return bestResult;
}

SymmetryResult SymmetryPlane::detectForSelection(const MeshData& mesh,
                                                  const std::vector<uint32_t>& selectedFaces,
                                                  const SymmetryOptions& options) {
    std::vector<glm::vec3> points;
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    for (uint32_t faceIdx : selectedFaces) {
        size_t baseIdx = static_cast<size_t>(faceIdx) * 3;
        if (baseIdx + 2 >= indices.size()) continue;
        
        for (int i = 0; i < 3; ++i) {
            points.push_back(vertices[indices[baseIdx + i]]);
        }
    }
    
    return detect(points, options);
}

float SymmetryPlane::evaluateSymmetry(const MeshData& mesh, 
                                       const Plane& plane,
                                       float tolerance) {
    return evaluateSymmetry(mesh.vertices(), plane, tolerance);
}

float SymmetryPlane::evaluateSymmetry(const std::vector<glm::vec3>& points,
                                       const Plane& plane,
                                       float tolerance) {
    if (points.empty()) return 0.0f;
    
    // Store original plane and temporarily use test plane
    Plane originalPlane = m_plane;
    m_plane = plane;
    
    size_t matchedCount = 0;
    
    for (const auto& p : points) {
        glm::vec3 reflected = reflectPoint(p);
        
        // Find closest point
        for (const auto& q : points) {
            if (glm::length(reflected - q) <= tolerance) {
                ++matchedCount;
                break;
            }
        }
    }
    
    m_plane = originalPlane;
    
    return static_cast<float>(matchedCount) / static_cast<float>(points.size());
}

SymmetryResult SymmetryPlane::evaluateDetailed(const MeshData& mesh,
                                                const Plane& plane,
                                                float tolerance) {
    SymmetryResult result;
    result.symmetryPlane = plane;
    
    const auto& points = mesh.vertices();
    if (points.empty()) {
        result.errorMessage = "Empty mesh";
        return result;
    }
    
    // Store original plane
    Plane originalPlane = m_plane;
    m_plane = plane;
    
    float sumDev = 0.0f;
    float maxDev = 0.0f;
    size_t matched = 0;
    
    for (const auto& p : points) {
        glm::vec3 reflected = reflectPoint(p);
        
        float minDist = std::numeric_limits<float>::max();
        for (const auto& q : points) {
            float d = glm::length(reflected - q);
            minDist = std::min(minDist, d);
        }
        
        if (minDist <= tolerance) {
            sumDev += minDist;
            maxDev = std::max(maxDev, minDist);
            ++matched;
        }
    }
    
    m_plane = originalPlane;
    
    result.found = matched > points.size() / 2;
    result.quality = static_cast<float>(matched) / points.size();
    result.avgDeviation = (matched > 0) ? sumDev / matched : 0.0f;
    result.maxDeviation = maxDev;
    result.matchedPairs = matched / 2;
    
    return result;
}

Plane SymmetryPlane::refine(const MeshData& mesh,
                            const Plane& initialPlane,
                            int iterations) {
    return refineGradient(mesh.vertices(), initialPlane, iterations);
}

Plane SymmetryPlane::refineGradient(const std::vector<glm::vec3>& points,
                                     const Plane& initialPlane,
                                     int iterations) {
    Plane current = initialPlane;
    
    glm::vec3 center, extent;
    computeBoundsAndCenter(points, center, extent);
    float tolerance = 0.01f * glm::length(extent);
    float stepSize = 0.1f;
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Evaluate current quality
        float baseQuality = evaluateSymmetry(points, current, tolerance);
        
        // Try adjusting normal direction
        glm::vec3 bestNormal = current.normal();
        float bestQuality = baseQuality;
        
        // Sample perturbations
        for (int i = 0; i < 6; ++i) {
            glm::vec3 delta(0.0f);
            delta[i / 2] = (i % 2 == 0) ? stepSize : -stepSize;
            
            glm::vec3 testNormal = glm::normalize(current.normal() + delta);
            Plane testPlane = Plane::fromPointAndNormal(current.getPointOnPlane(), testNormal);
            
            float q = evaluateSymmetry(points, testPlane, tolerance);
            if (q > bestQuality) {
                bestQuality = q;
                bestNormal = testNormal;
            }
        }
        
        // Try adjusting distance
        float bestDist = current.distance();
        for (float dDist : {-stepSize, stepSize}) {
            Plane testPlane(current.normal(), current.distance() + dDist);
            float q = evaluateSymmetry(points, testPlane, tolerance);
            if (q > bestQuality) {
                bestQuality = q;
                bestNormal = current.normal();
                bestDist = current.distance() + dDist;
            }
        }
        
        current = Plane(bestNormal, bestDist);
        
        // Reduce step size
        stepSize *= 0.8f;
    }
    
    return current;
}

glm::vec3 SymmetryPlane::reflectPoint(const glm::vec3& point) const {
    float dist = m_plane.distanceToPoint(point);
    return point - 2.0f * dist * m_plane.normal();
}

MeshData SymmetryPlane::reflectMesh(const MeshData& mesh) const {
    MeshData reflected;
    
    // Copy and reflect vertices
    const auto& srcVerts = mesh.vertices();
    reflected.vertices().reserve(srcVerts.size());
    
    for (const auto& v : srcVerts) {
        reflected.vertices().push_back(reflectPoint(v));
    }
    
    // Copy indices but reverse winding
    const auto& srcIndices = mesh.indices();
    reflected.indices().reserve(srcIndices.size());
    
    for (size_t i = 0; i < srcIndices.size(); i += 3) {
        reflected.indices().push_back(srcIndices[i]);
        reflected.indices().push_back(srcIndices[i + 2]);  // Swap to reverse winding
        reflected.indices().push_back(srcIndices[i + 1]);
    }
    
    // Compute new normals
    reflected.computeNormals();
    
    return reflected;
}

int SymmetryPlane::findReflectionMatch(const glm::vec3& point,
                                        const std::vector<glm::vec3>& candidates,
                                        float tolerance) const {
    glm::vec3 reflected = reflectPoint(point);
    float tolerance2 = tolerance * tolerance;
    
    int bestIdx = -1;
    float bestDist2 = tolerance2;
    
    for (size_t i = 0; i < candidates.size(); ++i) {
        float d2 = glm::length2(reflected - candidates[i]);
        if (d2 < bestDist2) {
            bestDist2 = d2;
            bestIdx = static_cast<int>(i);
        }
    }
    
    return bestIdx;
}

void SymmetryPlane::rotateNormal(const glm::vec3& axis, float angleRadians) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleRadians, axis);
    glm::vec3 newNormal = glm::vec3(rotation * glm::vec4(m_plane.normal(), 0.0f));
    m_plane.setNormal(newNormal);
}

void SymmetryPlane::translate(float distance) {
    m_plane.setDistance(m_plane.distance() + distance);
}

std::vector<Plane> SymmetryPlane::generateCandidates(const std::vector<glm::vec3>& points,
                                                      const SymmetryOptions& options) {
    std::vector<Plane> candidates;
    
    // Compute center
    glm::vec3 center(0.0f);
    for (const auto& p : points) center += p;
    center /= static_cast<float>(points.size());
    
    // Axis-aligned planes through center
    if (options.testAxisAligned) {
        candidates.push_back(Plane::fromPointAndNormal(center, glm::vec3(1, 0, 0)));
        candidates.push_back(Plane::fromPointAndNormal(center, glm::vec3(0, 1, 0)));
        candidates.push_back(Plane::fromPointAndNormal(center, glm::vec3(0, 0, 1)));
    }
    
    // PCA-based planes
    if (options.testPCA) {
        // Build covariance matrix
        float cov[3][3] = {{0}};
        for (const auto& p : points) {
            glm::vec3 d = p - center;
            cov[0][0] += d.x * d.x;
            cov[0][1] += d.x * d.y;
            cov[0][2] += d.x * d.z;
            cov[1][1] += d.y * d.y;
            cov[1][2] += d.y * d.z;
            cov[2][2] += d.z * d.z;
        }
        cov[1][0] = cov[0][1];
        cov[2][0] = cov[0][2];
        cov[2][1] = cov[1][2];
        
        // Find eigenvectors via power iteration
        glm::vec3 eigenvecs[3];
        eigenvecs[0] = glm::vec3(1, 0, 0);
        
        for (int e = 0; e < 3; ++e) {
            glm::vec3 v(1.0f, 0.5f, 0.25f);
            
            // Power iteration
            for (int iter = 0; iter < 30; ++iter) {
                glm::vec3 Av(
                    cov[0][0] * v.x + cov[0][1] * v.y + cov[0][2] * v.z,
                    cov[1][0] * v.x + cov[1][1] * v.y + cov[1][2] * v.z,
                    cov[2][0] * v.x + cov[2][1] * v.y + cov[2][2] * v.z
                );
                
                // Deflate previous eigenvectors
                for (int prev = 0; prev < e; ++prev) {
                    float proj = glm::dot(Av, eigenvecs[prev]);
                    Av -= proj * eigenvecs[prev];
                }
                
                float len = glm::length(Av);
                if (len > 1e-10f) v = Av / len;
            }
            
            eigenvecs[e] = v;
            candidates.push_back(Plane::fromPointAndNormal(center, v));
        }
    }
    
    // Add diagonal planes
    glm::vec3 diagonals[] = {
        glm::normalize(glm::vec3(1, 1, 0)),
        glm::normalize(glm::vec3(1, 0, 1)),
        glm::normalize(glm::vec3(0, 1, 1)),
        glm::normalize(glm::vec3(1, 1, 1)),
        glm::normalize(glm::vec3(1, -1, 0)),
        glm::normalize(glm::vec3(1, 0, -1)),
    };
    
    for (const auto& n : diagonals) {
        candidates.push_back(Plane::fromPointAndNormal(center, n));
    }
    
    return candidates;
}

void SymmetryPlane::computeBoundsAndCenter(const std::vector<glm::vec3>& points,
                                            glm::vec3& center, glm::vec3& extent) {
    if (points.empty()) {
        center = glm::vec3(0);
        extent = glm::vec3(1);
        return;
    }
    
    glm::vec3 minP = points[0], maxP = points[0];
    glm::vec3 sum(0);
    
    for (const auto& p : points) {
        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
        sum += p;
    }
    
    center = sum / static_cast<float>(points.size());
    extent = maxP - minP;
}

Plane SymmetryPlane::fitSymmetryPlanePCA(const std::vector<glm::vec3>& points) {
    glm::vec3 center, extent;
    computeBoundsAndCenter(points, center, extent);
    
    // Build covariance
    float cov[3][3] = {{0}};
    for (const auto& p : points) {
        glm::vec3 d = p - center;
        cov[0][0] += d.x * d.x;
        cov[0][1] += d.x * d.y;
        cov[0][2] += d.x * d.z;
        cov[1][1] += d.y * d.y;
        cov[1][2] += d.y * d.z;
        cov[2][2] += d.z * d.z;
    }
    cov[1][0] = cov[0][1];
    cov[2][0] = cov[0][2];
    cov[2][1] = cov[1][2];
    
    // Find smallest eigenvector
    glm::vec3 v(1, 0, 0);
    for (int iter = 0; iter < 50; ++iter) {
        glm::vec3 Av(
            cov[0][0] * v.x + cov[0][1] * v.y + cov[0][2] * v.z,
            cov[1][0] * v.x + cov[1][1] * v.y + cov[1][2] * v.z,
            cov[2][0] * v.x + cov[2][1] * v.y + cov[2][2] * v.z
        );
        float len = glm::length(Av);
        if (len > 1e-10f) v = Av / len;
    }
    
    return Plane::fromPointAndNormal(center, v);
}

// ---- MultiSymmetryDetector ----

std::vector<SymmetryResult> MultiSymmetryDetector::detectAll(const MeshData& mesh,
                                                              float minQuality,
                                                              int maxPlanes) {
    std::vector<SymmetryResult> results;
    
    SymmetryPlane detector;
    SymmetryOptions options;
    
    // Get initial best result
    auto best = detector.detect(mesh, options);
    if (best.found && best.quality >= minQuality) {
        results.push_back(best);
    }
    
    // Try to find additional planes
    // Generate more candidates and filter by quality
    auto candidates = SymmetryPlane::generateCandidates(mesh.vertices(), options);
    
    for (const auto& candidate : candidates) {
        // Skip if too similar to existing
        bool tooSimilar = false;
        for (const auto& existing : results) {
            float normalDot = std::abs(glm::dot(candidate.normal(), 
                                                existing.symmetryPlane.normal()));
            if (normalDot > 0.95f) {
                tooSimilar = true;
                break;
            }
        }
        
        if (tooSimilar) continue;
        
        auto refined = detector.refine(mesh, candidate, 10);
        auto result = detector.evaluateDetailed(mesh, refined, 0.01f);
        
        if (result.found && result.quality >= minQuality) {
            results.push_back(result);
        }
        
        if (static_cast<int>(results.size()) >= maxPlanes) break;
    }
    
    // Sort by quality
    std::sort(results.begin(), results.end(),
              [](const SymmetryResult& a, const SymmetryResult& b) {
                  return a.quality > b.quality;
              });
    
    return results;
}

float MultiSymmetryDetector::checkRotationalSymmetry(const MeshData& mesh,
                                                      const glm::vec3& axis,
                                                      int foldCount) {
    if (foldCount < 2) return 0.0f;
    
    const auto& vertices = mesh.vertices();
    if (vertices.empty()) return 0.0f;
    
    // Find center on axis
    glm::vec3 center(0);
    for (const auto& v : vertices) center += v;
    center /= static_cast<float>(vertices.size());
    
    // Compute tolerance
    float maxDist = 0.0f;
    for (const auto& v : vertices) {
        maxDist = std::max(maxDist, glm::length(v - center));
    }
    float tolerance = 0.01f * maxDist;
    
    // Test rotation
    float angleStep = glm::two_pi<float>() / foldCount;
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleStep, glm::normalize(axis));
    
    size_t matchCount = 0;
    
    for (const auto& v : vertices) {
        glm::vec3 rotated = glm::vec3(rotation * glm::vec4(v - center, 1.0f)) + center;
        
        // Find match
        for (const auto& q : vertices) {
            if (glm::length(rotated - q) <= tolerance) {
                ++matchCount;
                break;
            }
        }
    }
    
    return static_cast<float>(matchCount) / vertices.size();
}

} // namespace geometry
} // namespace dc3d
