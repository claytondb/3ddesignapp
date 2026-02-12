/**
 * @file Plane.cpp
 * @brief Implementation of Plane primitive fitting
 */

#include "Plane.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <glm/gtc/matrix_transform.hpp>

namespace dc3d {
namespace geometry {

Plane::Plane(const glm::vec3& normal, float distance)
    : m_normal(glm::normalize(normal))
    , m_distance(distance)
{
}

Plane Plane::fromPointAndNormal(const glm::vec3& point, const glm::vec3& normal) {
    glm::vec3 n = glm::normalize(normal);
    float d = -glm::dot(n, point);
    return Plane(n, d);
}

Plane Plane::fromThreePoints(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
    glm::vec3 v1 = p2 - p1;
    glm::vec3 v2 = p3 - p1;
    glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
    float d = -glm::dot(normal, p1);
    return Plane(normal, d);
}

PlaneFitResult Plane::fitToPoints(const std::vector<glm::vec3>& points) {
    PlaneFitResult result;
    
    if (points.size() < 3) {
        result.errorMessage = "Need at least 3 points to fit a plane";
        return result;
    }
    
    // Compute centroid
    glm::vec3 centroid(0.0f);
    for (const auto& p : points) {
        centroid += p;
    }
    centroid /= static_cast<float>(points.size());
    
    // Fit using least squares (PCA/covariance approach)
    fitLeastSquares(points, centroid);
    
    // Compute error metrics
    float sumSqError = 0.0f;
    result.maxError = 0.0f;
    
    for (const auto& p : points) {
        float dist = absoluteDistanceToPoint(p);
        sumSqError += dist * dist;
        result.maxError = std::max(result.maxError, dist);
    }
    
    result.rmsError = std::sqrt(sumSqError / static_cast<float>(points.size()));
    result.inlierCount = points.size();
    result.success = true;
    
    return result;
}

PlaneFitResult Plane::fitToSelection(const MeshData& mesh, const std::vector<uint32_t>& selectedFaces) {
    PlaneFitResult result;
    
    if (selectedFaces.empty()) {
        result.errorMessage = "No faces selected";
        return result;
    }
    
    // Collect all vertices from selected faces
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
    
    if (points.size() < 3) {
        result.errorMessage = "Not enough vertices from selection";
        return result;
    }
    
    // Also use face normals to help orient the plane
    glm::vec3 avgNormal(0.0f);
    for (uint32_t faceIdx : selectedFaces) {
        avgNormal += mesh.faceNormal(faceIdx);
    }
    avgNormal = glm::normalize(avgNormal);
    
    // Fit to points
    result = fitToPoints(points);
    
    // Ensure normal points in same general direction as face normals
    if (glm::dot(m_normal, avgNormal) < 0.0f) {
        flip();
    }
    
    return result;
}

PlaneFitResult Plane::fitRANSAC(const std::vector<glm::vec3>& points, 
                                float distanceThreshold,
                                int iterations) {
    PlaneFitResult bestResult;
    Plane bestPlane;
    size_t bestInliers = 0;
    
    if (points.size() < 3) {
        bestResult.errorMessage = "Need at least 3 points";
        return bestResult;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, points.size() - 1);
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Pick 3 random points
        size_t i1 = dist(gen);
        size_t i2 = dist(gen);
        size_t i3 = dist(gen);
        
        // Ensure distinct points
        if (i1 == i2 || i1 == i3 || i2 == i3) continue;
        
        // Create candidate plane
        Plane candidate = Plane::fromThreePoints(points[i1], points[i2], points[i3]);
        if (!candidate.isValid()) continue;
        
        // Count inliers
        size_t inliers = 0;
        for (const auto& p : points) {
            if (candidate.absoluteDistanceToPoint(p) <= distanceThreshold) {
                ++inliers;
            }
        }
        
        if (inliers > bestInliers) {
            bestInliers = inliers;
            bestPlane = candidate;
        }
    }
    
    // Refit using all inliers
    if (bestInliers >= 3) {
        std::vector<glm::vec3> inlierPoints;
        for (const auto& p : points) {
            if (bestPlane.absoluteDistanceToPoint(p) <= distanceThreshold) {
                inlierPoints.push_back(p);
            }
        }
        
        *this = bestPlane;
        bestResult = fitToPoints(inlierPoints);
        bestResult.inlierCount = bestInliers;
    } else {
        bestResult.errorMessage = "Could not find enough inliers";
    }
    
    return bestResult;
}

float Plane::distanceToPoint(const glm::vec3& point) const {
    return glm::dot(m_normal, point) + m_distance;
}

float Plane::absoluteDistanceToPoint(const glm::vec3& point) const {
    return std::abs(distanceToPoint(point));
}

glm::vec3 Plane::projectPoint(const glm::vec3& point) const {
    float dist = distanceToPoint(point);
    return point - dist * m_normal;
}

int Plane::whichSide(const glm::vec3& point, float tolerance) const {
    float dist = distanceToPoint(point);
    if (dist > tolerance) return 1;
    if (dist < -tolerance) return -1;
    return 0;
}

glm::vec3 Plane::getPointOnPlane() const {
    return -m_distance * m_normal;
}

bool Plane::intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& t) const {
    float denom = glm::dot(m_normal, rayDir);
    if (std::abs(denom) < 1e-8f) {
        return false;  // Ray parallel to plane
    }
    
    t = -(glm::dot(m_normal, rayOrigin) + m_distance) / denom;
    return true;
}

bool Plane::intersectPlane(const Plane& other, glm::vec3& linePoint, glm::vec3& lineDir) const {
    lineDir = glm::cross(m_normal, other.m_normal);
    float dirLen = glm::length(lineDir);
    
    if (dirLen < 1e-8f) {
        return false;  // Planes are parallel
    }
    
    lineDir /= dirLen;
    
    // Find a point on the intersection line
    // Solve system: n1·p = -d1, n2·p = -d2
    // We'll find the point closest to origin
    float n1n2 = glm::dot(m_normal, other.m_normal);
    float det = 1.0f - n1n2 * n1n2;
    
    float c1 = (-m_distance + other.m_distance * n1n2) / det;
    float c2 = (-other.m_distance + m_distance * n1n2) / det;
    
    linePoint = c1 * m_normal + c2 * other.m_normal;
    
    return true;
}

void Plane::flip() {
    m_normal = -m_normal;
    m_distance = -m_distance;
}

bool Plane::isValid() const {
    float len = glm::length(m_normal);
    return len > 0.9f && len < 1.1f && std::isfinite(m_distance);
}

void Plane::transform(const glm::mat4& matrix) {
    // Transform point on plane
    glm::vec3 point = getPointOnPlane();
    glm::vec4 transformed = matrix * glm::vec4(point, 1.0f);
    point = glm::vec3(transformed) / transformed.w;
    
    // Transform normal (use inverse transpose for normals)
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(matrix)));
    glm::vec3 newNormal = glm::normalize(normalMatrix * m_normal);
    
    *this = Plane::fromPointAndNormal(point, newNormal);
}

Plane Plane::transformed(const glm::mat4& matrix) const {
    Plane result = *this;
    result.transform(matrix);
    return result;
}

void Plane::getBasis(glm::vec3& u, glm::vec3& v) const {
    // Find vector not parallel to normal
    glm::vec3 arbitrary = (std::abs(m_normal.x) < 0.9f) 
                         ? glm::vec3(1.0f, 0.0f, 0.0f) 
                         : glm::vec3(0.0f, 1.0f, 0.0f);
    
    u = glm::normalize(glm::cross(m_normal, arbitrary));
    v = glm::cross(m_normal, u);
}

void Plane::fitLeastSquares(const std::vector<glm::vec3>& points, glm::vec3& centroid) {
    // Build covariance matrix
    float cov[3][3] = {{0}};
    
    for (const auto& p : points) {
        glm::vec3 d = p - centroid;
        cov[0][0] += d.x * d.x;
        cov[0][1] += d.x * d.y;
        cov[0][2] += d.x * d.z;
        cov[1][1] += d.y * d.y;
        cov[1][2] += d.y * d.z;
        cov[2][2] += d.z * d.z;
    }
    
    // Symmetric matrix
    cov[1][0] = cov[0][1];
    cov[2][0] = cov[0][2];
    cov[2][1] = cov[1][2];
    
    // Use power iteration to find eigenvector with smallest eigenvalue
    // (Normal is perpendicular to the plane of most variance)
    glm::vec3 normal(1.0f, 0.0f, 0.0f);
    
    // Simple power iteration for smallest eigenvector
    // We find the largest eigenvector first, then deflate
    glm::vec3 eigenvecs[3];
    float eigenvals[3];
    
    // Power iteration for dominant eigenvector
    glm::vec3 v(1.0f, 1.0f, 1.0f);
    for (int iter = 0; iter < 50; ++iter) {
        glm::vec3 Av(
            cov[0][0] * v.x + cov[0][1] * v.y + cov[0][2] * v.z,
            cov[1][0] * v.x + cov[1][1] * v.y + cov[1][2] * v.z,
            cov[2][0] * v.x + cov[2][1] * v.y + cov[2][2] * v.z
        );
        float len = glm::length(Av);
        if (len > 1e-10f) {
            v = Av / len;
        }
    }
    eigenvecs[0] = v;
    eigenvals[0] = glm::dot(v, glm::vec3(
        cov[0][0] * v.x + cov[0][1] * v.y + cov[0][2] * v.z,
        cov[1][0] * v.x + cov[1][1] * v.y + cov[1][2] * v.z,
        cov[2][0] * v.x + cov[2][1] * v.y + cov[2][2] * v.z
    ));
    
    // Deflate matrix and find second eigenvector
    float deflated[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            float vi = (i == 0) ? v.x : (i == 1) ? v.y : v.z;
            float vj = (j == 0) ? v.x : (j == 1) ? v.y : v.z;
            deflated[i][j] = cov[i][j] - eigenvals[0] * vi * vj;
        }
    }
    
    v = glm::vec3(1.0f, 1.0f, 1.0f);
    for (int iter = 0; iter < 50; ++iter) {
        glm::vec3 Av(
            deflated[0][0] * v.x + deflated[0][1] * v.y + deflated[0][2] * v.z,
            deflated[1][0] * v.x + deflated[1][1] * v.y + deflated[1][2] * v.z,
            deflated[2][0] * v.x + deflated[2][1] * v.y + deflated[2][2] * v.z
        );
        float len = glm::length(Av);
        if (len > 1e-10f) {
            v = Av / len;
        }
    }
    eigenvecs[1] = v;
    
    // Normal is cross of two largest eigenvectors (smallest eigenvalue direction)
    normal = glm::normalize(glm::cross(eigenvecs[0], eigenvecs[1]));
    
    m_normal = normal;
    m_distance = -glm::dot(m_normal, centroid);
}

} // namespace geometry
} // namespace dc3d
