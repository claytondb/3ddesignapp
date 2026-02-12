/**
 * @file Cylinder.cpp
 * @brief Implementation of Cylinder primitive fitting
 */

#include "Cylinder.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace dc3d {
namespace geometry {

Cylinder::Cylinder(const glm::vec3& center, const glm::vec3& axis, float radius, float height)
    : m_center(center)
    , m_axis(glm::normalize(axis))
    , m_radius(radius)
    , m_height(height)
{
}

CylinderFitResult Cylinder::fitToPoints(const std::vector<glm::vec3>& points,
                                         const CylinderFitOptions& options) {
    CylinderFitResult result;
    
    if (points.size() < 6) {
        result.errorMessage = "Need at least 6 points to fit a cylinder";
        return result;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, points.size() - 1);
    
    Cylinder bestCylinder;
    size_t bestInliers = 0;
    float bestError = std::numeric_limits<float>::max();
    
    // RANSAC loop
    for (int iter = 0; iter < options.ransacIterations; ++iter) {
        // Pick 6 random points
        std::vector<glm::vec3> sample;
        for (int i = 0; i < 6; ++i) {
            sample.push_back(points[dist(gen)]);
        }
        
        // Estimate axis from point distribution (PCA)
        glm::vec3 centroid(0.0f);
        for (const auto& p : sample) centroid += p;
        centroid /= 6.0f;
        
        // Covariance matrix
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
        
        // Power iteration for dominant eigenvector (axis)
        glm::vec3 axis(1.0f, 1.0f, 1.0f);
        for (int i = 0; i < 30; ++i) {
            glm::vec3 Av(
                cov[0][0] * axis.x + cov[0][1] * axis.y + cov[0][2] * axis.z,
                cov[1][0] * axis.x + cov[1][1] * axis.y + cov[1][2] * axis.z,
                cov[2][0] * axis.x + cov[2][1] * axis.y + cov[2][2] * axis.z
            );
            float len = glm::length(Av);
            if (len > 1e-10f) axis = Av / len;
        }
        
        // Try this as a candidate
        Cylinder candidate;
        candidate.m_axis = axis;
        candidate.fitRadiusAndCenter(sample, axis);
        candidate.computeHeightRange(sample);
        
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
            bestCylinder = candidate;
        }
    }
    
    if (bestInliers < 6) {
        result.errorMessage = "Could not find enough inliers";
        return result;
    }
    
    // Collect inliers and refit
    std::vector<glm::vec3> inlierPoints;
    for (const auto& p : points) {
        if (bestCylinder.absoluteDistanceToPoint(p) <= options.inlierThreshold) {
            inlierPoints.push_back(p);
        }
    }
    
    *this = bestCylinder;
    refineIteratively(inlierPoints, options.refinementIterations);
    
    // Compute final error metrics
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

CylinderFitResult Cylinder::fitToPointsWithNormals(const std::vector<glm::vec3>& points,
                                                    const std::vector<glm::vec3>& normals,
                                                    const CylinderFitOptions& options) {
    CylinderFitResult result;
    
    if (points.size() < 6 || normals.size() != points.size()) {
        result.errorMessage = "Need at least 6 points with matching normals";
        return result;
    }
    
    // Estimate axis from normals (axis is perpendicular to radial normals)
    glm::vec3 axis = estimateAxisFromNormals(normals);
    
    // Fit with estimated axis as starting point
    m_axis = axis;
    fitRadiusAndCenter(points, axis);
    computeHeightRange(points);
    
    // Refine
    if (options.refinementIterations > 0) {
        refineIteratively(points, options.refinementIterations);
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

CylinderFitResult Cylinder::fitToSelection(const MeshData& mesh, 
                                            const std::vector<uint32_t>& selectedFaces,
                                            const CylinderFitOptions& options) {
    CylinderFitResult result;
    
    if (selectedFaces.empty()) {
        result.errorMessage = "No faces selected";
        return result;
    }
    
    // Collect vertices and normals from selected faces
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

float Cylinder::distanceToPoint(const glm::vec3& point) const {
    // Project point onto axis
    glm::vec3 toPoint = point - m_center;
    float axisProj = glm::dot(toPoint, m_axis);
    glm::vec3 radialVec = toPoint - axisProj * m_axis;
    float radialDist = glm::length(radialVec);
    
    return radialDist - m_radius;
}

float Cylinder::absoluteDistanceToPoint(const glm::vec3& point) const {
    return std::abs(distanceToPoint(point));
}

glm::vec3 Cylinder::projectPoint(const glm::vec3& point) const {
    glm::vec3 toPoint = point - m_center;
    float axisProj = glm::dot(toPoint, m_axis);
    glm::vec3 radialVec = toPoint - axisProj * m_axis;
    float radialDist = glm::length(radialVec);
    
    if (radialDist < 1e-10f) {
        // Point is on axis, project to nearest surface point
        glm::vec3 arbitrary = (std::abs(m_axis.x) < 0.9f) 
                             ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        radialVec = glm::normalize(glm::cross(m_axis, arbitrary));
    } else {
        radialVec /= radialDist;
    }
    
    // Clamp to height range
    float halfHeight = m_height * 0.5f;
    axisProj = glm::clamp(axisProj, -halfHeight, halfHeight);
    
    return m_center + axisProj * m_axis + m_radius * radialVec;
}

glm::vec3 Cylinder::closestPointOnAxis(const glm::vec3& point) const {
    glm::vec3 toPoint = point - m_center;
    float axisProj = glm::dot(toPoint, m_axis);
    return m_center + axisProj * m_axis;
}

bool Cylinder::containsPoint(const glm::vec3& point) const {
    glm::vec3 toPoint = point - m_center;
    float axisProj = glm::dot(toPoint, m_axis);
    
    // Check height
    float halfHeight = m_height * 0.5f;
    if (std::abs(axisProj) > halfHeight) return false;
    
    // Check radius
    glm::vec3 radialVec = toPoint - axisProj * m_axis;
    return glm::length2(radialVec) <= m_radius * m_radius;
}

void Cylinder::getEndCaps(glm::vec3& bottom, glm::vec3& top) const {
    float halfHeight = m_height * 0.5f;
    bottom = m_center - halfHeight * m_axis;
    top = m_center + halfHeight * m_axis;
}

int Cylinder::intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                           float& t1, float& t2) const {
    // Transform ray to cylinder local space where axis is Y
    glm::vec3 oc = rayOrigin - m_center;
    
    // Project out axis component
    float rayDirAxis = glm::dot(rayDir, m_axis);
    float ocAxis = glm::dot(oc, m_axis);
    
    glm::vec3 rayDirPerp = rayDir - rayDirAxis * m_axis;
    glm::vec3 ocPerp = oc - ocAxis * m_axis;
    
    float a = glm::dot(rayDirPerp, rayDirPerp);
    float b = 2.0f * glm::dot(ocPerp, rayDirPerp);
    float c = glm::dot(ocPerp, ocPerp) - m_radius * m_radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) return 0;
    
    float sqrtD = std::sqrt(discriminant);
    float inv2a = 0.5f / a;
    
    t1 = (-b - sqrtD) * inv2a;
    t2 = (-b + sqrtD) * inv2a;
    
    // Check height bounds
    float halfHeight = m_height * 0.5f;
    int count = 0;
    float validT[2];
    
    for (float t : {t1, t2}) {
        float h = ocAxis + t * rayDirAxis;
        if (std::abs(h) <= halfHeight) {
            validT[count++] = t;
        }
    }
    
    if (count >= 1) t1 = validT[0];
    if (count >= 2) t2 = validT[1];
    
    return count;
}

bool Cylinder::isValid() const {
    return m_radius > 0.0f && 
           m_height > 0.0f && 
           std::abs(glm::length(m_axis) - 1.0f) < 0.01f &&
           std::isfinite(m_center.x) && std::isfinite(m_center.y) && std::isfinite(m_center.z);
}

float Cylinder::surfaceArea() const {
    // Lateral + 2 caps
    return 2.0f * glm::pi<float>() * m_radius * m_height + 
           2.0f * glm::pi<float>() * m_radius * m_radius;
}

float Cylinder::volume() const {
    return glm::pi<float>() * m_radius * m_radius * m_height;
}

void Cylinder::transform(const glm::mat4& matrix) {
    // Transform center
    glm::vec4 tc = matrix * glm::vec4(m_center, 1.0f);
    m_center = glm::vec3(tc) / tc.w;
    
    // Transform axis direction
    glm::vec3 axisEnd = m_center + m_axis;
    glm::vec4 te = matrix * glm::vec4(axisEnd, 1.0f);
    axisEnd = glm::vec3(te) / te.w;
    m_axis = glm::normalize(axisEnd - m_center);
    
    // Scale radius and height (approximate for non-uniform scale)
    glm::vec3 scale(
        glm::length(glm::vec3(matrix[0])),
        glm::length(glm::vec3(matrix[1])),
        glm::length(glm::vec3(matrix[2]))
    );
    
    // Use average perpendicular scale for radius
    float avgScale = (scale.x + scale.y + scale.z) / 3.0f;
    m_radius *= avgScale;
    m_height *= avgScale;
}

Cylinder Cylinder::transformed(const glm::mat4& matrix) const {
    Cylinder result = *this;
    result.transform(matrix);
    return result;
}

std::vector<glm::vec3> Cylinder::sampleSurface(int radialSegments, 
                                                int heightSegments,
                                                bool includeCaps) const {
    std::vector<glm::vec3> points;
    
    // Get basis vectors perpendicular to axis
    glm::vec3 u, v;
    glm::vec3 arbitrary = (std::abs(m_axis.x) < 0.9f) 
                         ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    u = glm::normalize(glm::cross(m_axis, arbitrary));
    v = glm::cross(m_axis, u);
    
    float halfHeight = m_height * 0.5f;
    
    // Lateral surface
    for (int h = 0; h <= heightSegments; ++h) {
        float t = static_cast<float>(h) / heightSegments;
        float y = -halfHeight + t * m_height;
        
        for (int r = 0; r < radialSegments; ++r) {
            float angle = 2.0f * glm::pi<float>() * static_cast<float>(r) / radialSegments;
            glm::vec3 radial = std::cos(angle) * u + std::sin(angle) * v;
            points.push_back(m_center + y * m_axis + m_radius * radial);
        }
    }
    
    // Caps
    if (includeCaps) {
        for (int r = 0; r < radialSegments; ++r) {
            float angle = 2.0f * glm::pi<float>() * static_cast<float>(r) / radialSegments;
            glm::vec3 radial = std::cos(angle) * u + std::sin(angle) * v;
            
            // Bottom cap
            points.push_back(m_center - halfHeight * m_axis + m_radius * radial);
            // Top cap
            points.push_back(m_center + halfHeight * m_axis + m_radius * radial);
        }
        // Cap centers
        points.push_back(m_center - halfHeight * m_axis);
        points.push_back(m_center + halfHeight * m_axis);
    }
    
    return points;
}

glm::vec3 Cylinder::estimateAxisFromNormals(const std::vector<glm::vec3>& normals) {
    // Cylinder surface normals are perpendicular to axis
    // Find direction that minimizes |n · axis| for all normals
    
    // Build covariance-like matrix of outer products
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
    
    // Find eigenvector with smallest eigenvalue (most perpendicular to normals)
    // Use inverse power iteration
    glm::vec3 axis(1.0f, 0.5f, 0.5f);
    
    // Power iteration to find dominant eigenvector, then use cross products
    glm::vec3 v1(1, 0, 0), v2(0, 1, 0);
    for (int iter = 0; iter < 50; ++iter) {
        glm::vec3 Av(
            cov[0][0] * v1.x + cov[0][1] * v1.y + cov[0][2] * v1.z,
            cov[1][0] * v1.x + cov[1][1] * v1.y + cov[1][2] * v1.z,
            cov[2][0] * v1.x + cov[2][1] * v1.y + cov[2][2] * v1.z
        );
        float len = glm::length(Av);
        if (len > 1e-10f) v1 = Av / len;
    }
    
    // Deflate and find second eigenvector
    float lambda1 = glm::dot(v1, glm::vec3(
        cov[0][0] * v1.x + cov[0][1] * v1.y + cov[0][2] * v1.z,
        cov[1][0] * v1.x + cov[1][1] * v1.y + cov[1][2] * v1.z,
        cov[2][0] * v1.x + cov[2][1] * v1.y + cov[2][2] * v1.z
    ));
    
    float deflated[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            float vi = (i == 0) ? v1.x : (i == 1) ? v1.y : v1.z;
            float vj = (j == 0) ? v1.x : (j == 1) ? v1.y : v1.z;
            deflated[i][j] = cov[i][j] - lambda1 * vi * vj;
        }
    }
    
    for (int iter = 0; iter < 50; ++iter) {
        glm::vec3 Av(
            deflated[0][0] * v2.x + deflated[0][1] * v2.y + deflated[0][2] * v2.z,
            deflated[1][0] * v2.x + deflated[1][1] * v2.y + deflated[1][2] * v2.z,
            deflated[2][0] * v2.x + deflated[2][1] * v2.y + deflated[2][2] * v2.z
        );
        float len = glm::length(Av);
        if (len > 1e-10f) v2 = Av / len;
    }
    
    // Axis is perpendicular to the two dominant eigenvectors
    axis = glm::normalize(glm::cross(v1, v2));
    
    return axis;
}

void Cylinder::fitRadiusAndCenter(const std::vector<glm::vec3>& points, const glm::vec3& axis) {
    // Project points to 2D plane perpendicular to axis
    // Then fit a circle
    
    // Get basis for perpendicular plane
    glm::vec3 u, v;
    glm::vec3 arbitrary = (std::abs(axis.x) < 0.9f) 
                         ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    u = glm::normalize(glm::cross(axis, arbitrary));
    v = glm::cross(axis, u);
    
    // Project to 2D
    std::vector<glm::vec2> points2D;
    glm::vec3 centroid3D(0.0f);
    for (const auto& p : points) {
        centroid3D += p;
    }
    centroid3D /= static_cast<float>(points.size());
    
    for (const auto& p : points) {
        glm::vec3 rel = p - centroid3D;
        points2D.push_back(glm::vec2(glm::dot(rel, u), glm::dot(rel, v)));
    }
    
    // Fit circle using algebraic method
    // Minimize sum of (x^2 + y^2 - 2*cx*x - 2*cy*y + cx^2 + cy^2 - r^2)^2
    // Linear version: x^2 + y^2 = 2*cx*x + 2*cy*y + (r^2 - cx^2 - cy^2)
    // Let A = 2*cx, B = 2*cy, C = r^2 - cx^2 - cy^2
    // Solve: [x y 1] * [A B C]' = x^2 + y^2
    
    float sumX = 0, sumY = 0, sumXX = 0, sumYY = 0, sumXY = 0;
    float sumXXX = 0, sumYYY = 0, sumXXY = 0, sumXYY = 0;
    
    for (const auto& p : points2D) {
        float x = p.x, y = p.y;
        float xx = x * x, yy = y * y;
        sumX += x; sumY += y;
        sumXX += xx; sumYY += yy; sumXY += x * y;
        sumXXX += xx * x; sumYYY += yy * y;
        sumXXY += xx * y; sumXYY += x * yy;
    }
    
    float n = static_cast<float>(points2D.size());
    
    // Solve 3x3 system using Cramer's rule
    float A = n * sumXY - sumX * sumY;
    float B = n * sumXX - sumX * sumX;
    float C = n * sumYY - sumY * sumY;
    float D = 0.5f * (n * (sumXXX + sumXYY) - sumX * (sumXX + sumYY));
    float E = 0.5f * (n * (sumXXY + sumYYY) - sumY * (sumXX + sumYY));
    
    float denom = B * C - A * A;
    if (std::abs(denom) < 1e-10f) {
        // Degenerate, use centroid
        m_center = centroid3D;
        m_radius = 1.0f;
        return;
    }
    
    float cx2D = (D * C - E * A) / denom;
    float cy2D = (B * E - A * D) / denom;
    
    // Convert back to 3D
    m_center = centroid3D + cx2D * u + cy2D * v;
    
    // Compute radius
    float sumR = 0.0f;
    for (const auto& p : points) {
        glm::vec3 toP = p - m_center;
        float axisProj = glm::dot(toP, axis);
        glm::vec3 radial = toP - axisProj * axis;
        sumR += glm::length(radial);
    }
    m_radius = sumR / static_cast<float>(points.size());
}

void Cylinder::computeHeightRange(const std::vector<glm::vec3>& points) {
    float minH = std::numeric_limits<float>::max();
    float maxH = std::numeric_limits<float>::lowest();
    
    for (const auto& p : points) {
        float h = glm::dot(p - m_center, m_axis);
        minH = std::min(minH, h);
        maxH = std::max(maxH, h);
    }
    
    m_height = maxH - minH;
    m_center += ((maxH + minH) * 0.5f) * m_axis;  // Adjust center to midpoint
}

void Cylinder::refineIteratively(const std::vector<glm::vec3>& points, int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        // Refit radius and center with current axis
        fitRadiusAndCenter(points, m_axis);
        
        // Optionally refine axis (keeping it simple for now)
        computeHeightRange(points);
    }
}

} // namespace geometry
} // namespace dc3d
