/**
 * @file PrimitiveFitter.cpp
 * @brief Implementation of unified primitive fitting
 */

#include "PrimitiveFitter.h"
#include <algorithm>
#include <cmath>

namespace dc3d {
namespace geometry {

PrimitiveType DetectionScores::best() const {
    float maxScore = plane;
    PrimitiveType bestType = PrimitiveType::Plane;
    
    if (cylinder > maxScore) { maxScore = cylinder; bestType = PrimitiveType::Cylinder; }
    if (cone > maxScore) { maxScore = cone; bestType = PrimitiveType::Cone; }
    if (sphere > maxScore) { maxScore = sphere; bestType = PrimitiveType::Sphere; }
    
    return bestType;
}

float DetectionScores::bestScore() const {
    return std::max({plane, cylinder, cone, sphere});
}

DetectionScores PrimitiveFitter::detectPrimitiveType(const MeshData& mesh, 
                                                      const std::vector<uint32_t>& selectedFaces) {
    std::vector<glm::vec3> points, normals;
    extractFromSelection(mesh, selectedFaces, points, normals);
    return detectPrimitiveType(points, normals);
}

DetectionScores PrimitiveFitter::detectPrimitiveType(const std::vector<glm::vec3>& points,
                                                      const std::vector<glm::vec3>& normals) {
    DetectionScores scores;
    
    if (points.size() < 4) {
        return scores;
    }
    
    float threshold = computeThreshold(points);
    
    // Try fitting each primitive type and score by inlier ratio and error
    
    // Plane
    {
        Plane plane;
        auto result = plane.fitToPoints(points);
        if (result.success) {
            size_t inliers = 0;
            for (const auto& p : points) {
                if (plane.absoluteDistanceToPoint(p) <= threshold) ++inliers;
            }
            float inlierRatio = static_cast<float>(inliers) / points.size();
            float errorScore = 1.0f / (1.0f + result.rmsError / threshold);
            scores.plane = inlierRatio * errorScore;
        }
    }
    
    // Sphere - also good for detecting if uniform radial distance from center
    {
        Sphere sphere;
        SphereFitOptions opts;
        opts.inlierThreshold = threshold;
        auto result = sphere.fitToPoints(points, opts);
        if (result.success) {
            size_t inliers = 0;
            for (const auto& p : points) {
                if (sphere.absoluteDistanceToPoint(p) <= threshold) ++inliers;
            }
            float inlierRatio = static_cast<float>(inliers) / points.size();
            float errorScore = 1.0f / (1.0f + result.rmsError / threshold);
            scores.sphere = inlierRatio * errorScore;
        }
    }
    
    // Cylinder - check if normals are perpendicular to a common axis
    {
        Cylinder cylinder;
        CylinderFitOptions opts;
        opts.inlierThreshold = threshold;
        opts.useNormals = !normals.empty();
        
        CylinderFitResult result;
        if (!normals.empty()) {
            result = cylinder.fitToPointsWithNormals(points, normals, opts);
        } else {
            result = cylinder.fitToPoints(points, opts);
        }
        
        if (result.success) {
            size_t inliers = 0;
            for (const auto& p : points) {
                if (cylinder.absoluteDistanceToPoint(p) <= threshold) ++inliers;
            }
            float inlierRatio = static_cast<float>(inliers) / points.size();
            float errorScore = 1.0f / (1.0f + result.rmsError / threshold);
            scores.cylinder = inlierRatio * errorScore;
        }
    }
    
    // Cone
    {
        Cone cone;
        ConeFitOptions opts;
        opts.inlierThreshold = threshold;
        opts.useNormals = !normals.empty();
        
        ConeFitResult result;
        if (!normals.empty()) {
            result = cone.fitToPointsWithNormals(points, normals, opts);
        } else {
            result = cone.fitToPoints(points, opts);
        }
        
        if (result.success) {
            size_t inliers = 0;
            for (const auto& p : points) {
                if (cone.absoluteDistanceToPoint(p) <= threshold) ++inliers;
            }
            float inlierRatio = static_cast<float>(inliers) / points.size();
            float errorScore = 1.0f / (1.0f + result.rmsError / threshold);
            scores.cone = inlierRatio * errorScore;
        }
    }
    
    // Use normals for additional discrimination
    if (!normals.empty()) {
        // Plane: all normals should be parallel
        glm::vec3 avgNormal(0.0f);
        for (const auto& n : normals) avgNormal += n;
        avgNormal = glm::normalize(avgNormal);
        
        float normalVariance = 0.0f;
        for (const auto& n : normals) {
            float dot = std::abs(glm::dot(n, avgNormal));
            normalVariance += 1.0f - dot;
        }
        normalVariance /= normals.size();
        
        // Low variance = probably plane
        if (normalVariance < 0.1f) {
            scores.plane *= 1.2f;  // Boost plane score
        }
        
        // Sphere: normals point radially outward
        // Compute centroid and check if normals point away from it
        glm::vec3 centroid(0.0f);
        for (const auto& p : points) centroid += p;
        centroid /= static_cast<float>(points.size());
        
        float radialScore = 0.0f;
        for (size_t i = 0; i < points.size(); ++i) {
            glm::vec3 radial = glm::normalize(points[i] - centroid);
            radialScore += std::abs(glm::dot(normals[i], radial));
        }
        radialScore /= normals.size();
        
        if (radialScore > 0.9f) {
            scores.sphere *= 1.2f;  // Boost sphere score
        }
    }
    
    // Normalize scores
    float maxScore = std::max({scores.plane, scores.cylinder, scores.cone, scores.sphere, 0.001f});
    scores.plane /= maxScore;
    scores.cylinder /= maxScore;
    scores.cone /= maxScore;
    scores.sphere /= maxScore;
    
    return scores;
}

FitResult PrimitiveFitter::fitAuto(const MeshData& mesh, 
                                   const std::vector<uint32_t>& selectedFaces) {
    auto scores = detectPrimitiveType(mesh, selectedFaces);
    
    if (scores.bestScore() < m_options.detectionThreshold) {
        // Low confidence, try all and pick best
        return fitBest(mesh, selectedFaces);
    }
    
    return fitPrimitive(mesh, selectedFaces, scores.best());
}

FitResult PrimitiveFitter::fitPrimitive(const MeshData& mesh, 
                                         const std::vector<uint32_t>& selectedFaces,
                                         PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Plane: return fitPlane(mesh, selectedFaces);
        case PrimitiveType::Cylinder: return fitCylinder(mesh, selectedFaces);
        case PrimitiveType::Cone: return fitCone(mesh, selectedFaces);
        case PrimitiveType::Sphere: return fitSphere(mesh, selectedFaces);
        default: {
            FitResult result;
            result.errorMessage = "Unknown primitive type";
            return result;
        }
    }
}

FitResult PrimitiveFitter::fitBest(const MeshData& mesh, 
                                    const std::vector<uint32_t>& selectedFaces) {
    std::vector<FitResult> results = {
        fitPlane(mesh, selectedFaces),
        fitCylinder(mesh, selectedFaces),
        fitCone(mesh, selectedFaces),
        fitSphere(mesh, selectedFaces)
    };
    
    FitResult best;
    float bestScore = -1.0f;
    
    for (auto& r : results) {
        if (r.success) {
            float score = r.confidence;
            if (score > bestScore) {
                bestScore = score;
                best = std::move(r);
            }
        }
    }
    
    return best;
}

FitResult PrimitiveFitter::fitPlane(const MeshData& mesh, 
                                     const std::vector<uint32_t>& selectedFaces) {
    std::vector<glm::vec3> points, normals;
    extractFromSelection(mesh, selectedFaces, points, normals);
    return fitPlane(points);
}

FitResult PrimitiveFitter::fitCylinder(const MeshData& mesh, 
                                        const std::vector<uint32_t>& selectedFaces) {
    std::vector<glm::vec3> points, normals;
    extractFromSelection(mesh, selectedFaces, points, normals);
    return fitCylinder(points, normals);
}

FitResult PrimitiveFitter::fitCone(const MeshData& mesh, 
                                    const std::vector<uint32_t>& selectedFaces) {
    std::vector<glm::vec3> points, normals;
    extractFromSelection(mesh, selectedFaces, points, normals);
    return fitCone(points, normals);
}

FitResult PrimitiveFitter::fitSphere(const MeshData& mesh, 
                                      const std::vector<uint32_t>& selectedFaces) {
    std::vector<glm::vec3> points, normals;
    extractFromSelection(mesh, selectedFaces, points, normals);
    return fitSphere(points);
}

FitResult PrimitiveFitter::fitAuto(const std::vector<glm::vec3>& points,
                                    const std::vector<glm::vec3>& normals) {
    auto scores = detectPrimitiveType(points, normals);
    PrimitiveType type = scores.best();
    
    switch (type) {
        case PrimitiveType::Plane: return fitPlane(points);
        case PrimitiveType::Cylinder: return fitCylinder(points, normals);
        case PrimitiveType::Cone: return fitCone(points, normals);
        case PrimitiveType::Sphere: return fitSphere(points);
        default: {
            FitResult result;
            result.errorMessage = "Detection failed";
            return result;
        }
    }
}

FitResult PrimitiveFitter::fitPlane(const std::vector<glm::vec3>& points) {
    FitResult result;
    result.type = PrimitiveType::Plane;
    result.totalPoints = points.size();
    
    Plane plane;
    auto fitResult = plane.fitToPoints(points);
    
    if (!fitResult.success) {
        result.errorMessage = fitResult.errorMessage;
        return result;
    }
    
    float threshold = computeThreshold(points);
    
    // Count inliers
    size_t inliers = 0;
    for (const auto& p : points) {
        if (plane.absoluteDistanceToPoint(p) <= threshold) ++inliers;
    }
    
    result.success = true;
    result.primitive = plane;
    result.rmsError = fitResult.rmsError;
    result.maxError = fitResult.maxError;
    result.inlierCount = inliers;
    result.inlierRatio = static_cast<float>(inliers) / points.size();
    result.confidence = computeConfidence(result.rmsError, threshold, inliers, points.size());
    
    return result;
}

FitResult PrimitiveFitter::fitCylinder(const std::vector<glm::vec3>& points,
                                        const std::vector<glm::vec3>& normals) {
    FitResult result;
    result.type = PrimitiveType::Cylinder;
    result.totalPoints = points.size();
    
    Cylinder cylinder;
    CylinderFitOptions opts;
    opts.ransacIterations = m_options.ransacIterations;
    opts.inlierThreshold = computeThreshold(points);
    opts.refinementIterations = m_options.refinementIterations;
    opts.useNormals = m_options.useNormals && !normals.empty();
    
    CylinderFitResult fitResult;
    if (opts.useNormals) {
        fitResult = cylinder.fitToPointsWithNormals(points, normals, opts);
    } else {
        fitResult = cylinder.fitToPoints(points, opts);
    }
    
    if (!fitResult.success) {
        result.errorMessage = fitResult.errorMessage;
        return result;
    }
    
    result.success = true;
    result.primitive = cylinder;
    result.rmsError = fitResult.rmsError;
    result.maxError = fitResult.maxError;
    result.inlierCount = fitResult.inlierCount;
    result.inlierRatio = static_cast<float>(fitResult.inlierCount) / points.size();
    result.confidence = computeConfidence(result.rmsError, opts.inlierThreshold, 
                                          result.inlierCount, points.size());
    
    return result;
}

FitResult PrimitiveFitter::fitCone(const std::vector<glm::vec3>& points,
                                    const std::vector<glm::vec3>& normals) {
    FitResult result;
    result.type = PrimitiveType::Cone;
    result.totalPoints = points.size();
    
    Cone cone;
    ConeFitOptions opts;
    opts.ransacIterations = m_options.ransacIterations;
    opts.inlierThreshold = computeThreshold(points);
    opts.refinementIterations = m_options.refinementIterations;
    opts.useNormals = m_options.useNormals && !normals.empty();
    
    ConeFitResult fitResult;
    if (opts.useNormals) {
        fitResult = cone.fitToPointsWithNormals(points, normals, opts);
    } else {
        fitResult = cone.fitToPoints(points, opts);
    }
    
    if (!fitResult.success) {
        result.errorMessage = fitResult.errorMessage;
        return result;
    }
    
    result.success = true;
    result.primitive = cone;
    result.rmsError = fitResult.rmsError;
    result.maxError = fitResult.maxError;
    result.inlierCount = fitResult.inlierCount;
    result.inlierRatio = static_cast<float>(fitResult.inlierCount) / points.size();
    result.confidence = computeConfidence(result.rmsError, opts.inlierThreshold, 
                                          result.inlierCount, points.size());
    
    return result;
}

FitResult PrimitiveFitter::fitSphere(const std::vector<glm::vec3>& points) {
    FitResult result;
    result.type = PrimitiveType::Sphere;
    result.totalPoints = points.size();
    
    Sphere sphere;
    SphereFitOptions opts;
    opts.ransacIterations = m_options.ransacIterations;
    opts.inlierThreshold = computeThreshold(points);
    
    auto fitResult = sphere.fitToPoints(points, opts);
    
    if (!fitResult.success) {
        result.errorMessage = fitResult.errorMessage;
        return result;
    }
    
    result.success = true;
    result.primitive = sphere;
    result.rmsError = fitResult.rmsError;
    result.maxError = fitResult.maxError;
    result.inlierCount = fitResult.inlierCount;
    result.inlierRatio = static_cast<float>(fitResult.inlierCount) / points.size();
    result.confidence = computeConfidence(result.rmsError, opts.inlierThreshold, 
                                          result.inlierCount, points.size());
    
    return result;
}

void PrimitiveFitter::extractFromSelection(const MeshData& mesh, 
                                           const std::vector<uint32_t>& selectedFaces,
                                           std::vector<glm::vec3>& points,
                                           std::vector<glm::vec3>& normals) {
    points.clear();
    normals.clear();
    
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
}

float PrimitiveFitter::computeThreshold(const std::vector<glm::vec3>& points) {
    if (!m_options.useRelativeThreshold) {
        return m_options.inlierThreshold;
    }
    
    if (points.empty()) return m_options.inlierThreshold;
    
    // Compute bounding box diagonal
    glm::vec3 minP = points[0], maxP = points[0];
    for (const auto& p : points) {
        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
    }
    
    float diagonal = glm::length(maxP - minP);
    return m_options.inlierThresholdRel * diagonal;
}

float PrimitiveFitter::computeConfidence(float rmsError, float threshold, 
                                         size_t inliers, size_t total) {
    if (total == 0) return 0.0f;
    
    float inlierRatio = static_cast<float>(inliers) / total;
    float errorRatio = threshold / (threshold + rmsError);
    
    // Combine factors
    float confidence = inlierRatio * errorRatio;
    
    // Bonus for high inlier count
    if (inlierRatio > 0.9f) confidence *= 1.1f;
    
    return std::min(1.0f, confidence);
}

} // namespace geometry
} // namespace dc3d
