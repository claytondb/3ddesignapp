/**
 * @file Cone.cpp
 * @brief Implementation of Cone primitive fitting
 */

#include "Cone.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace dc3d {
namespace geometry {

Cone::Cone(const glm::vec3& apex, const glm::vec3& axis, float halfAngle, float height)
    : m_apex(apex)
    , m_axis(glm::normalize(axis))
    , m_halfAngle(halfAngle)
    , m_height(height)
{
    updateTrigCache();
}

void Cone::updateTrigCache() {
    m_cosHalfAngle = std::cos(m_halfAngle);
    m_sinHalfAngle = std::sin(m_halfAngle);
}

ConeFitResult Cone::fitToPoints(const std::vector<glm::vec3>& points,
                                 const ConeFitOptions& options) {
    ConeFitResult result;
    
    if (points.size() < 6) {
        result.errorMessage = "Need at least 6 points to fit a cone";
        return result;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, points.size() - 1);
    
    Cone bestCone;
    size_t bestInliers = 0;
    float bestError = std::numeric_limits<float>::max();
    
    // RANSAC loop
    for (int iter = 0; iter < options.ransacIterations; ++iter) {
        // Sample random points
        std::vector<glm::vec3> sample;
        for (int i = 0; i < 6; ++i) {
            sample.push_back(points[dist(gen)]);
        }
        
        // Estimate axis from point distribution
        glm::vec3 centroid(0.0f);
        for (const auto& p : sample) centroid += p;
        centroid /= 6.0f;
        
        // PCA for axis
        float cov[3][3] = {{0}};
        for (const auto& p : sample) {
            glm::vec3 d = p - centroid;
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
        
        glm::vec3 axis(1, 0, 0);
        for (int i = 0; i < 30; ++i) {
            glm::vec3 Av(
                cov[0][0] * axis.x + cov[0][1] * axis.y + cov[0][2] * axis.z,
                cov[1][0] * axis.x + cov[1][1] * axis.y + cov[1][2] * axis.z,
                cov[2][0] * axis.x + cov[2][1] * axis.y + cov[2][2] * axis.z
            );
            float len = glm::length(Av);
            if (len > 1e-10f) axis = Av / len;
        }
        
        Cone candidate;
        candidate.m_axis = axis;
        candidate.fitApexAndAngle(sample, axis);
        candidate.updateTrigCache();
        
        if (!candidate.isValid()) continue;
        
        // Count inliers
        size_t inliers = 0;
        float sumError = 0.0f;
        for (const auto& p : points) {
            float d = candidate.absoluteDistanceToPoint(p);
            if (d <= options.inlierThreshold) {
                ++inliers;
                sumError += d * d;
            }
        }
        
        if (inliers > bestInliers || 
            (inliers == bestInliers && sumError < bestError)) {
            bestInliers = inliers;
            bestError = sumError;
            bestCone = candidate;
        }
    }
    
    if (bestInliers < 6) {
        result.errorMessage = "Could not find enough inliers";
        return result;
    }
    
    // Refit using all inliers
    std::vector<glm::vec3> inlierPoints;
    for (const auto& p : points) {
        if (bestCone.absoluteDistanceToPoint(p) <= options.inlierThreshold) {
            inlierPoints.push_back(p);
        }
    }
    
    *this = bestCone;
    std::vector<glm::vec3> emptyNormals;
    refineIteratively(inlierPoints, emptyNormals, options.refinementIterations);
    
    // Compute error metrics
    float sumSqError = 0.0f;
    result.maxError = 0.0f;
    for (const auto& p : points) {
        float d = absoluteDistanceToPoint(p);
        sumSqError += d * d;
        result.maxError = std::max(result.maxError, d);
    }
    
    result.rmsError = std::sqrt(sumSqError / static_cast<float>(points.size()));
    result.inlierCount = bestInliers;
    result.success = true;
    
    return result;
}

ConeFitResult Cone::fitToPointsWithNormals(const std::vector<glm::vec3>& points,
                                            const std::vector<glm::vec3>& normals,
                                            const ConeFitOptions& options) {
    ConeFitResult result;
    
    if (points.size() < 6 || normals.size() != points.size()) {
        result.errorMessage = "Need at least 6 points with matching normals";
        return result;
    }
    
    // Estimate axis from normals
    glm::vec3 axis = estimateAxisFromNormals(normals);
    
    m_axis = axis;
    fitApexAndAngle(points, axis);
    updateTrigCache();
    
    if (options.refinementIterations > 0) {
        refineIteratively(points, normals, options.refinementIterations);
    }
    
    // Compute error metrics
    float sumSqError = 0.0f;
    result.maxError = 0.0f;
    result.inlierCount = 0;
    
    for (const auto& p : points) {
        float d = absoluteDistanceToPoint(p);
        sumSqError += d * d;
        result.maxError = std::max(result.maxError, d);
        if (d <= options.inlierThreshold) ++result.inlierCount;
    }
    
    result.rmsError = std::sqrt(sumSqError / static_cast<float>(points.size()));
    result.success = isValid();
    
    return result;
}

ConeFitResult Cone::fitToSelection(const MeshData& mesh, 
                                    const std::vector<uint32_t>& selectedFaces,
                                    const ConeFitOptions& options) {
    ConeFitResult result;
    
    if (selectedFaces.empty()) {
        result.errorMessage = "No faces selected";
        return result;
    }
    
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    for (uint32_t faceIdx : selectedFaces) {
        size_t baseIdx = static_cast<size_t>(faceIdx) * 3;
        if (baseIdx + 2 >= indices.size()) continue;
        
        glm::vec3 faceNormal = mesh.faceNormal(faceIdx);
        
        for (int i = 0; i < 3; ++i) {
            points.push_back(vertices[indices[baseIdx + i]]);
            normals.push_back(faceNormal);
        }
    }
    
    if (options.useNormals && !normals.empty()) {
        return fitToPointsWithNormals(points, normals, options);
    } else {
        return fitToPoints(points, options);
    }
}

float Cone::distanceToPoint(const glm::vec3& point) const {
    glm::vec3 v = point - m_apex;
    float axisProj = glm::dot(v, m_axis);
    
    // Point behind apex
    if (axisProj < 0.0f) {
        return glm::length(v);
    }
    
    // Beyond base
    if (axisProj > m_height) {
        glm::vec3 baseCenter = m_apex + m_height * m_axis;
        float baseRadius = radiusAtHeight(m_height);
        glm::vec3 toPoint = point - baseCenter;
        float radialDist = glm::length(toPoint - glm::dot(toPoint, m_axis) * m_axis);
        
        if (radialDist <= baseRadius) {
            return axisProj - m_height;  // Above base
        }
        // Outside base radius
    }
    
    // Distance to cone surface
    glm::vec3 radialVec = v - axisProj * m_axis;
    float radialDist = glm::length(radialVec);
    
    // Expected radius at this height
    float expectedRadius = axisProj * m_sinHalfAngle / m_cosHalfAngle;
    
    // Distance along normal to surface
    float surfaceDist = (radialDist - expectedRadius) * m_cosHalfAngle;
    
    return surfaceDist;
}

float Cone::absoluteDistanceToPoint(const glm::vec3& point) const {
    return std::abs(distanceToPoint(point));
}

glm::vec3 Cone::projectPoint(const glm::vec3& point) const {
    glm::vec3 v = point - m_apex;
    float axisProj = glm::dot(v, m_axis);
    
    if (axisProj <= 0.0f) {
        return m_apex;
    }
    
    axisProj = std::min(axisProj, m_height);
    
    glm::vec3 radialVec = v - axisProj * m_axis;
    float radialDist = glm::length(radialVec);
    
    float expectedRadius = radiusAtHeight(axisProj);
    
    if (radialDist < 1e-10f) {
        // On axis, pick arbitrary direction
        glm::vec3 arbitrary = (std::abs(m_axis.x) < 0.9f) 
                             ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        radialVec = glm::normalize(glm::cross(m_axis, arbitrary));
    } else {
        radialVec /= radialDist;
    }
    
    return m_apex + axisProj * m_axis + expectedRadius * radialVec;
}

bool Cone::containsPoint(const glm::vec3& point) const {
    glm::vec3 v = point - m_apex;
    float axisProj = glm::dot(v, m_axis);
    
    if (axisProj < 0.0f || axisProj > m_height) return false;
    
    glm::vec3 radialVec = v - axisProj * m_axis;
    float radialDist = glm::length(radialVec);
    float expectedRadius = radiusAtHeight(axisProj);
    
    return radialDist <= expectedRadius;
}

float Cone::radiusAtHeight(float height) const {
    return height * std::tan(m_halfAngle);
}

void Cone::getBase(glm::vec3& center, float& radius) const {
    center = m_apex + m_height * m_axis;
    radius = radiusAtHeight(m_height);
}

int Cone::intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                       float& t1, float& t2) const {
    glm::vec3 co = rayOrigin - m_apex;
    
    float cos2 = m_cosHalfAngle * m_cosHalfAngle;
    float dv = glm::dot(rayDir, m_axis);
    float cv = glm::dot(co, m_axis);
    
    float a = dv * dv - cos2;
    float b = 2.0f * (dv * cv - glm::dot(rayDir, co) * cos2);
    float c = cv * cv - glm::dot(co, co) * cos2;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) return 0;
    
    float sqrtD = std::sqrt(discriminant);
    float inv2a = 0.5f / a;
    
    t1 = (-b - sqrtD) * inv2a;
    t2 = (-b + sqrtD) * inv2a;
    
    // Check height bounds and ensure on correct nappe
    int count = 0;
    float validT[2];
    
    for (float t : {t1, t2}) {
        glm::vec3 p = rayOrigin + t * rayDir;
        float h = glm::dot(p - m_apex, m_axis);
        if (h >= 0.0f && h <= m_height) {
            validT[count++] = t;
        }
    }
    
    if (count >= 1) t1 = validT[0];
    if (count >= 2) t2 = validT[1];
    
    return count;
}

float Cone::halfAngleDegrees() const {
    return glm::degrees(m_halfAngle);
}

void Cone::setHalfAngleDegrees(float degrees) {
    m_halfAngle = glm::radians(degrees);
    updateTrigCache();
}

bool Cone::isValid() const {
    return m_halfAngle > 0.0f && 
           m_halfAngle < glm::half_pi<float>() &&
           m_height > 0.0f &&
           std::abs(glm::length(m_axis) - 1.0f) < 0.01f &&
           std::isfinite(m_apex.x) && std::isfinite(m_apex.y) && std::isfinite(m_apex.z);
}

float Cone::surfaceArea() const {
    float r = radiusAtHeight(m_height);
    float slant = m_height / m_cosHalfAngle;
    return glm::pi<float>() * r * slant + glm::pi<float>() * r * r;  // lateral + base
}

float Cone::volume() const {
    float r = radiusAtHeight(m_height);
    return (1.0f / 3.0f) * glm::pi<float>() * r * r * m_height;
}

void Cone::transform(const glm::mat4& matrix) {
    // Transform apex
    glm::vec4 ta = matrix * glm::vec4(m_apex, 1.0f);
    m_apex = glm::vec3(ta) / ta.w;
    
    // Transform axis
    glm::vec3 axisEnd = m_apex + m_axis;
    glm::vec4 te = matrix * glm::vec4(axisEnd, 1.0f);
    axisEnd = glm::vec3(te) / te.w;
    m_axis = glm::normalize(axisEnd - m_apex);
    
    // Scale height
    glm::vec3 scale(
        glm::length(glm::vec3(matrix[0])),
        glm::length(glm::vec3(matrix[1])),
        glm::length(glm::vec3(matrix[2]))
    );
    float avgScale = (scale.x + scale.y + scale.z) / 3.0f;
    m_height *= avgScale;
}

Cone Cone::transformed(const glm::mat4& matrix) const {
    Cone result = *this;
    result.transform(matrix);
    return result;
}

std::vector<glm::vec3> Cone::sampleSurface(int radialSegments, int heightSegments) const {
    std::vector<glm::vec3> points;
    
    glm::vec3 u, v;
    glm::vec3 arbitrary = (std::abs(m_axis.x) < 0.9f) 
                         ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    u = glm::normalize(glm::cross(m_axis, arbitrary));
    v = glm::cross(m_axis, u);
    
    // Apex
    points.push_back(m_apex);
    
    // Lateral surface
    for (int h = 1; h <= heightSegments; ++h) {
        float height = m_height * static_cast<float>(h) / heightSegments;
        float radius = radiusAtHeight(height);
        
        for (int r = 0; r < radialSegments; ++r) {
            float angle = 2.0f * glm::pi<float>() * static_cast<float>(r) / radialSegments;
            glm::vec3 radial = std::cos(angle) * u + std::sin(angle) * v;
            points.push_back(m_apex + height * m_axis + radius * radial);
        }
    }
    
    // Base center
    points.push_back(m_apex + m_height * m_axis);
    
    return points;
}

glm::vec3 Cone::estimateAxisFromNormals(const std::vector<glm::vec3>& normals) {
    // Cone surface normals have constant angle with axis
    // Find axis direction that minimizes variance of dot(n, axis)
    
    // Build covariance matrix
    float cov[3][3] = {{0}};
    for (const auto& n : normals) {
        cov[0][0] += n.x * n.x;
        cov[0][1] += n.x * n.y;
        cov[0][2] += n.x * n.z;
        cov[1][1] += n.y * n.y;
        cov[1][2] += n.y * n.z;
        cov[2][2] += n.z * n.z;
    }
    cov[1][0] = cov[0][1];
    cov[2][0] = cov[0][2];
    cov[2][1] = cov[1][2];
    
    // Power iteration for dominant eigenvector
    glm::vec3 axis(1, 0.5f, 0.5f);
    for (int iter = 0; iter < 50; ++iter) {
        glm::vec3 Av(
            cov[0][0] * axis.x + cov[0][1] * axis.y + cov[0][2] * axis.z,
            cov[1][0] * axis.x + cov[1][1] * axis.y + cov[1][2] * axis.z,
            cov[2][0] * axis.x + cov[2][1] * axis.y + cov[2][2] * axis.z
        );
        float len = glm::length(Av);
        if (len > 1e-10f) axis = Av / len;
    }
    
    return axis;
}

void Cone::fitApexAndAngle(const std::vector<glm::vec3>& points, const glm::vec3& axis) {
    // Project points to axis and find extent
    float minProj = std::numeric_limits<float>::max();
    float maxProj = std::numeric_limits<float>::lowest();
    
    glm::vec3 centroid(0.0f);
    for (const auto& p : points) centroid += p;
    centroid /= static_cast<float>(points.size());
    
    for (const auto& p : points) {
        float proj = glm::dot(p - centroid, axis);
        minProj = std::min(minProj, proj);
        maxProj = std::max(maxProj, proj);
    }
    
    // Estimate apex at minimum projection
    m_apex = centroid + minProj * axis;
    m_height = maxProj - minProj;
    
    // Estimate half-angle from average radial distance
    float sumAngle = 0.0f;
    int count = 0;
    
    for (const auto& p : points) {
        glm::vec3 toPoint = p - m_apex;
        float axisProj = glm::dot(toPoint, axis);
        if (axisProj > 1e-6f) {
            glm::vec3 radialVec = toPoint - axisProj * axis;
            float radialDist = glm::length(radialVec);
            sumAngle += std::atan2(radialDist, axisProj);
            ++count;
        }
    }
    
    if (count > 0) {
        m_halfAngle = sumAngle / static_cast<float>(count);
    } else {
        m_halfAngle = 0.5f;  // Default ~30 degrees
    }
    
    updateTrigCache();
}

void Cone::refineIteratively(const std::vector<glm::vec3>& points, 
                             const std::vector<glm::vec3>& normals,
                             int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        // Refine apex position
        glm::vec3 apexShift(0.0f);
        float weightSum = 0.0f;
        
        for (const auto& p : points) {
            glm::vec3 toPoint = p - m_apex;
            float axisProj = glm::dot(toPoint, m_axis);
            if (axisProj < 1e-6f) continue;
            
            float dist = distanceToPoint(p);
            float weight = 1.0f / (1.0f + std::abs(dist));
            
            // Move apex toward better fit
            apexShift += weight * dist * m_axis;
            weightSum += weight;
        }
        
        if (weightSum > 0.0f) {
            m_apex += 0.1f * apexShift / weightSum;
        }
        
        // Recompute half-angle
        fitApexAndAngle(points, m_axis);
    }
}

} // namespace geometry
} // namespace dc3d
