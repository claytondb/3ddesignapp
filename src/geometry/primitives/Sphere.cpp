/**
 * @file Sphere.cpp
 * @brief Implementation of Sphere primitive fitting
 */

#include "Sphere.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>

namespace dc3d {
namespace geometry {

Sphere::Sphere(const glm::vec3& center, float radius)
    : m_center(center)
    , m_radius(radius)
{
}

SphereFitResult Sphere::fitToPoints(const std::vector<glm::vec3>& points,
                                     const SphereFitOptions& options) {
    SphereFitResult result;
    
    if (points.size() < 4) {
        result.errorMessage = "Need at least 4 points to fit a sphere";
        return result;
    }
    
    if (options.useAlgebraicFit) {
        fitAlgebraic(points);
    } else {
        // Initialize with bounding sphere then refine
        glm::vec3 minP = points[0], maxP = points[0];
        for (const auto& p : points) {
            minP = glm::min(minP, p);
            maxP = glm::max(maxP, p);
        }
        m_center = (minP + maxP) * 0.5f;
        m_radius = glm::length(maxP - minP) * 0.5f;
        
        fitGeometric(points, 20);
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

SphereFitResult Sphere::fitRANSAC(const std::vector<glm::vec3>& points,
                                   const SphereFitOptions& options) {
    SphereFitResult result;
    
    if (points.size() < 4) {
        result.errorMessage = "Need at least 4 points";
        return result;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, points.size() - 1);
    
    Sphere bestSphere;
    size_t bestInliers = 0;
    float bestError = std::numeric_limits<float>::max();
    
    for (int iter = 0; iter < options.ransacIterations; ++iter) {
        // Pick 4 random points
        size_t i1 = dist(gen), i2 = dist(gen), i3 = dist(gen), i4 = dist(gen);
        
        if (i1 == i2 || i1 == i3 || i1 == i4 || i2 == i3 || i2 == i4 || i3 == i4) {
            continue;
        }
        
        Sphere candidate;
        if (!candidate.fitFromFourPoints(points[i1], points[i2], points[i3], points[i4])) {
            continue;
        }
        
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
            bestSphere = candidate;
        }
    }
    
    if (bestInliers < 4) {
        result.errorMessage = "Could not find enough inliers";
        return result;
    }
    
    // Refit using all inliers
    std::vector<glm::vec3> inlierPoints;
    for (const auto& p : points) {
        if (bestSphere.absoluteDistanceToPoint(p) <= options.inlierThreshold) {
            inlierPoints.push_back(p);
        }
    }
    
    *this = bestSphere;
    fitAlgebraic(inlierPoints);
    
    // Final error metrics
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

SphereFitResult Sphere::fitToSelection(const MeshData& mesh, 
                                        const std::vector<uint32_t>& selectedFaces,
                                        const SphereFitOptions& options) {
    SphereFitResult result;
    
    if (selectedFaces.empty()) {
        result.errorMessage = "No faces selected";
        return result;
    }
    
    std::vector<glm::vec3> points;
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    for (uint32_t faceIdx : selectedFaces) {
        size_t baseIdx = static_cast<size_t>(faceIdx) * 3;
        if (baseIdx + 2 >= indices.size()) continue;
        
        points.push_back(vertices[indices[baseIdx]]);
        points.push_back(vertices[indices[baseIdx + 1]]);
        points.push_back(vertices[indices[baseIdx + 2]]);
    }
    
    return fitToPoints(points, options);
}

bool Sphere::fitFromFourPoints(const glm::vec3& p1, const glm::vec3& p2,
                                const glm::vec3& p3, const glm::vec3& p4) {
    // Solve for center equidistant from all 4 points
    // |c - p1|^2 = |c - p2|^2 = |c - p3|^2 = |c - p4|^2
    // This gives us 3 linear equations in c
    
    glm::vec3 d1 = p2 - p1;
    glm::vec3 d2 = p3 - p1;
    glm::vec3 d3 = p4 - p1;
    
    float b1 = 0.5f * (glm::dot(p2, p2) - glm::dot(p1, p1));
    float b2 = 0.5f * (glm::dot(p3, p3) - glm::dot(p1, p1));
    float b3 = 0.5f * (glm::dot(p4, p4) - glm::dot(p1, p1));
    
    // Solve 3x3 system using Cramer's rule
    float det = d1.x * (d2.y * d3.z - d2.z * d3.y) 
              - d1.y * (d2.x * d3.z - d2.z * d3.x) 
              + d1.z * (d2.x * d3.y - d2.y * d3.x);
    
    if (std::abs(det) < 1e-10f) {
        return false;  // Coplanar or degenerate
    }
    
    float invDet = 1.0f / det;
    
    m_center.x = invDet * (b1 * (d2.y * d3.z - d2.z * d3.y) 
                         - d1.y * (b2 * d3.z - d2.z * b3) 
                         + d1.z * (b2 * d3.y - d2.y * b3));
    
    m_center.y = invDet * (d1.x * (b2 * d3.z - d2.z * b3) 
                         - b1 * (d2.x * d3.z - d2.z * d3.x) 
                         + d1.z * (d2.x * b3 - b2 * d3.x));
    
    m_center.z = invDet * (d1.x * (d2.y * b3 - b2 * d3.y) 
                         - d1.y * (d2.x * b3 - b2 * d3.x) 
                         + b1 * (d2.x * d3.y - d2.y * d3.x));
    
    m_radius = glm::length(p1 - m_center);
    
    return m_radius > 0.0f && m_radius < 1e10f;
}

float Sphere::distanceToPoint(const glm::vec3& point) const {
    return glm::length(point - m_center) - m_radius;
}

float Sphere::absoluteDistanceToPoint(const glm::vec3& point) const {
    return std::abs(distanceToPoint(point));
}

glm::vec3 Sphere::projectPoint(const glm::vec3& point) const {
    glm::vec3 dir = point - m_center;
    float len = glm::length(dir);
    
    if (len < 1e-10f) {
        return m_center + glm::vec3(m_radius, 0, 0);
    }
    
    return m_center + (m_radius / len) * dir;
}

bool Sphere::containsPoint(const glm::vec3& point) const {
    return glm::length2(point - m_center) <= m_radius * m_radius;
}

glm::vec3 Sphere::normalAt(const glm::vec3& surfacePoint) const {
    return glm::normalize(surfacePoint - m_center);
}

int Sphere::intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                         float& t1, float& t2) const {
    glm::vec3 oc = rayOrigin - m_center;
    
    float a = glm::dot(rayDir, rayDir);
    float b = 2.0f * glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - m_radius * m_radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) return 0;
    
    if (discriminant < 1e-10f) {
        t1 = t2 = -b / (2.0f * a);
        return 1;
    }
    
    float sqrtD = std::sqrt(discriminant);
    float inv2a = 0.5f / a;
    
    t1 = (-b - sqrtD) * inv2a;
    t2 = (-b + sqrtD) * inv2a;
    
    return 2;
}

bool Sphere::intersectsSphere(const Sphere& other) const {
    float dist = glm::length(m_center - other.m_center);
    return dist <= (m_radius + other.m_radius);
}

void Sphere::getBoundingBox(glm::vec3& min, glm::vec3& max) const {
    min = m_center - glm::vec3(m_radius);
    max = m_center + glm::vec3(m_radius);
}

bool Sphere::isValid() const {
    return m_radius > 0.0f && 
           m_radius < 1e10f &&
           std::isfinite(m_center.x) && 
           std::isfinite(m_center.y) && 
           std::isfinite(m_center.z);
}

float Sphere::surfaceArea() const {
    return 4.0f * glm::pi<float>() * m_radius * m_radius;
}

float Sphere::volume() const {
    return (4.0f / 3.0f) * glm::pi<float>() * m_radius * m_radius * m_radius;
}

void Sphere::transform(const glm::mat4& matrix) {
    glm::vec4 tc = matrix * glm::vec4(m_center, 1.0f);
    m_center = glm::vec3(tc) / tc.w;
    
    // Scale radius by average scale factor
    glm::vec3 scale(
        glm::length(glm::vec3(matrix[0])),
        glm::length(glm::vec3(matrix[1])),
        glm::length(glm::vec3(matrix[2]))
    );
    m_radius *= (scale.x + scale.y + scale.z) / 3.0f;
}

Sphere Sphere::transformed(const glm::mat4& matrix) const {
    Sphere result = *this;
    result.transform(matrix);
    return result;
}

std::vector<glm::vec3> Sphere::sampleSurface(int latSegments, int lonSegments) const {
    std::vector<glm::vec3> points;
    
    // Top pole
    points.push_back(m_center + glm::vec3(0, m_radius, 0));
    
    // Middle rings
    for (int lat = 1; lat < latSegments; ++lat) {
        float phi = glm::pi<float>() * static_cast<float>(lat) / latSegments;
        float y = m_radius * std::cos(phi);
        float ringRadius = m_radius * std::sin(phi);
        
        for (int lon = 0; lon < lonSegments; ++lon) {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(lon) / lonSegments;
            float x = ringRadius * std::cos(theta);
            float z = ringRadius * std::sin(theta);
            points.push_back(m_center + glm::vec3(x, y, z));
        }
    }
    
    // Bottom pole
    points.push_back(m_center + glm::vec3(0, -m_radius, 0));
    
    return points;
}

std::vector<glm::vec3> Sphere::sampleUniform(int numPoints) const {
    std::vector<glm::vec3> points;
    points.reserve(numPoints);
    
    // Use golden spiral for uniform distribution
    float goldenRatio = (1.0f + std::sqrt(5.0f)) / 2.0f;
    float angleIncrement = glm::two_pi<float>() * goldenRatio;
    
    for (int i = 0; i < numPoints; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
        float phi = std::acos(1.0f - 2.0f * t);
        float theta = angleIncrement * static_cast<float>(i);
        
        float x = std::sin(phi) * std::cos(theta);
        float y = std::sin(phi) * std::sin(theta);
        float z = std::cos(phi);
        
        points.push_back(m_center + m_radius * glm::vec3(x, y, z));
    }
    
    return points;
}

void Sphere::fitAlgebraic(const std::vector<glm::vec3>& points) {
    // Algebraic sphere fit: minimize sum of (x^2 + y^2 + z^2 - 2*cx*x - 2*cy*y - 2*cz*z + c^2 - r^2)^2
    // Linearize: let A = 2*cx, B = 2*cy, C = 2*cz, D = cx^2 + cy^2 + cz^2 - r^2
    // Then: x^2 + y^2 + z^2 = A*x + B*y + C*z + D
    // Solve least squares: [x y z 1] * [A B C D]' = x^2 + y^2 + z^2
    
    size_t n = points.size();
    
    // Build normal equations: (X^T * X) * params = X^T * b
    float XtX[4][4] = {{0}};
    float Xtb[4] = {0};
    
    for (const auto& p : points) {
        float x = p.x, y = p.y, z = p.z;
        float rhs = x * x + y * y + z * z;
        
        float row[4] = {x, y, z, 1.0f};
        
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                XtX[i][j] += row[i] * row[j];
            }
            Xtb[i] += row[i] * rhs;
        }
    }
    
    // Solve 4x4 system using Gauss-Jordan elimination
    float aug[4][5];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            aug[i][j] = XtX[i][j];
        }
        aug[i][4] = Xtb[i];
    }
    
    for (int col = 0; col < 4; ++col) {
        // Find pivot
        int maxRow = col;
        for (int row = col + 1; row < 4; ++row) {
            if (std::abs(aug[row][col]) > std::abs(aug[maxRow][col])) {
                maxRow = row;
            }
        }
        
        // Swap rows
        for (int j = 0; j < 5; ++j) {
            std::swap(aug[col][j], aug[maxRow][j]);
        }
        
        // Eliminate
        if (std::abs(aug[col][col]) < 1e-10f) continue;
        
        float pivot = aug[col][col];
        for (int j = 0; j < 5; ++j) {
            aug[col][j] /= pivot;
        }
        
        for (int row = 0; row < 4; ++row) {
            if (row != col) {
                float factor = aug[row][col];
                for (int j = 0; j < 5; ++j) {
                    aug[row][j] -= factor * aug[col][j];
                }
            }
        }
    }
    
    // Extract center and radius
    // A = 2*cx, B = 2*cy, C = 2*cz, D = c^2 - r^2
    float A = aug[0][4], B = aug[1][4], C = aug[2][4], D = aug[3][4];
    
    m_center = glm::vec3(A * 0.5f, B * 0.5f, C * 0.5f);
    float c2 = glm::dot(m_center, m_center);
    m_radius = std::sqrt(c2 - D);
    
    if (!std::isfinite(m_radius) || m_radius <= 0.0f) {
        // Fallback: use bounding sphere
        glm::vec3 minP = points[0], maxP = points[0];
        for (const auto& p : points) {
            minP = glm::min(minP, p);
            maxP = glm::max(maxP, p);
        }
        m_center = (minP + maxP) * 0.5f;
        m_radius = glm::length(maxP - minP) * 0.5f;
    }
}

void Sphere::fitGeometric(const std::vector<glm::vec3>& points, int iterations) {
    // Iterative geometric fit: minimize sum of (|p - c| - r)^2
    // Gradient descent on center, average radius
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Compute average radius and center shift
        glm::vec3 shift(0.0f);
        float avgRadius = 0.0f;
        
        for (const auto& p : points) {
            glm::vec3 dir = p - m_center;
            float dist = glm::length(dir);
            
            if (dist > 1e-10f) {
                dir /= dist;
                shift += (dist - m_radius) * dir;
            }
            avgRadius += dist;
        }
        
        avgRadius /= static_cast<float>(points.size());
        shift /= static_cast<float>(points.size());
        
        m_center += shift;
        m_radius = avgRadius;
    }
}

} // namespace geometry
} // namespace dc3d
