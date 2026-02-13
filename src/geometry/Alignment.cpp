/**
 * @file Alignment.cpp
 * @brief Implementation of mesh alignment algorithms
 */

#include "Alignment.h"
#include "ICP.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace dc3d {
namespace geometry {

// ============================================================================
// AlignmentResult Implementation
// ============================================================================

AlignmentResult AlignmentResult::createSuccess(const glm::mat4& mat) {
    AlignmentResult result;
    result.success = true;
    result.transform = mat;
    result.decomposeTransform();
    return result;
}

AlignmentResult AlignmentResult::createFailure(const std::string& error) {
    AlignmentResult result;
    result.success = false;
    result.errorMessage = error;
    return result;
}

void AlignmentResult::decomposeTransform() {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(transform, scale, rotation, translation, skew, perspective);
}

void AlignmentResult::composeTransform() {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 R = glm::toMat4(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    transform = T * R * S;
}

// ============================================================================
// Alignment Class Implementation
// ============================================================================

glm::vec3 Alignment::getAxisDirection(WCSAxis axis) {
    switch (axis) {
        case WCSAxis::PositiveX: return glm::vec3(1.0f, 0.0f, 0.0f);
        case WCSAxis::NegativeX: return glm::vec3(-1.0f, 0.0f, 0.0f);
        case WCSAxis::PositiveY: return glm::vec3(0.0f, 1.0f, 0.0f);
        case WCSAxis::NegativeY: return glm::vec3(0.0f, -1.0f, 0.0f);
        case WCSAxis::PositiveZ: return glm::vec3(0.0f, 0.0f, 1.0f);
        case WCSAxis::NegativeZ: return glm::vec3(0.0f, 0.0f, -1.0f);
        default: return glm::vec3(0.0f, 0.0f, 1.0f);
    }
}

glm::mat3 Alignment::buildRotationFromAxes(
    const glm::vec3& fromPrimary,
    const glm::vec3& fromSecondary,
    const glm::vec3& toPrimary,
    const glm::vec3& toSecondary)
{
    // Orthogonalize secondary to primary for both systems
    glm::vec3 fromSecondaryOrtho = fromSecondary - 
        glm::dot(fromSecondary, fromPrimary) * fromPrimary;
    if (glm::length(fromSecondaryOrtho) < 1e-6f) {
        // Degenerate case: find any perpendicular vector
        if (std::abs(fromPrimary.x) < 0.9f) {
            fromSecondaryOrtho = glm::cross(fromPrimary, glm::vec3(1, 0, 0));
        } else {
            fromSecondaryOrtho = glm::cross(fromPrimary, glm::vec3(0, 1, 0));
        }
    }
    fromSecondaryOrtho = glm::normalize(fromSecondaryOrtho);
    
    glm::vec3 toSecondaryOrtho = toSecondary - 
        glm::dot(toSecondary, toPrimary) * toPrimary;
    if (glm::length(toSecondaryOrtho) < 1e-6f) {
        if (std::abs(toPrimary.x) < 0.9f) {
            toSecondaryOrtho = glm::cross(toPrimary, glm::vec3(1, 0, 0));
        } else {
            toSecondaryOrtho = glm::cross(toPrimary, glm::vec3(0, 1, 0));
        }
    }
    toSecondaryOrtho = glm::normalize(toSecondaryOrtho);
    
    // Compute tertiary axes
    glm::vec3 fromTertiary = glm::cross(fromPrimary, fromSecondaryOrtho);
    glm::vec3 toTertiary = glm::cross(toPrimary, toSecondaryOrtho);
    
    // Build rotation matrices
    // From: columns are fromPrimary, fromSecondaryOrtho, fromTertiary
    glm::mat3 fromMat(fromPrimary, fromSecondaryOrtho, fromTertiary);
    // To: columns are toPrimary, toSecondaryOrtho, toTertiary
    glm::mat3 toMat(toPrimary, toSecondaryOrtho, toTertiary);
    
    // Rotation = To * From^T
    return toMat * glm::transpose(fromMat);
}

AlignmentResult Alignment::alignToWCS(
    MeshData& mesh,
    const AlignmentFeatureData& primary,
    WCSAxis primaryAxis,
    const AlignmentFeatureData& secondary,
    WCSAxis secondaryAxis,
    const std::optional<glm::vec3>& origin,
    const AlignmentOptions& options)
{
    // Get direction vectors from features
    glm::vec3 fromPrimaryDir;
    glm::vec3 fromSecondaryDir;
    
    switch (primary.type) {
        case AlignmentFeature::Plane:
            fromPrimaryDir = primary.direction;
            break;
        case AlignmentFeature::Line:
        case AlignmentFeature::CylinderAxis:
            fromPrimaryDir = primary.direction;
            break;
        default:
            return AlignmentResult::createFailure("Primary feature must be a plane, line, or cylinder");
    }
    
    switch (secondary.type) {
        case AlignmentFeature::Plane:
            fromSecondaryDir = secondary.direction;
            break;
        case AlignmentFeature::Line:
        case AlignmentFeature::CylinderAxis:
            fromSecondaryDir = secondary.direction;
            break;
        default:
            return AlignmentResult::createFailure("Secondary feature must be a plane, line, or cylinder");
    }
    
    // Get target WCS directions
    glm::vec3 toPrimaryDir = getAxisDirection(primaryAxis);
    glm::vec3 toSecondaryDir = getAxisDirection(secondaryAxis);
    
    // Build rotation
    glm::mat3 rotation = buildRotationFromAxes(
        fromPrimaryDir, fromSecondaryDir,
        toPrimaryDir, toSecondaryDir
    );
    
    // Determine origin point
    glm::vec3 originPoint = origin.value_or(primary.point);
    
    // Build full transformation: translate to origin, rotate, then set at WCS origin
    // T = R * (p - originPoint)
    // Final transform: first translate by -originPoint, then rotate
    glm::mat4 transform(1.0f);
    transform = glm::mat4(rotation) * glm::translate(glm::mat4(1.0f), -originPoint);
    
    // Apply transformation
    if (!options.preview) {
        mesh.transform(transform);
    }
    
    return AlignmentResult::createSuccess(transform);
}

AlignmentResult Alignment::alignInteractive(
    MeshData& mesh,
    const glm::mat4& transform,
    const AlignmentOptions& options)
{
    if (!options.preview) {
        mesh.transform(transform);
    }
    return AlignmentResult::createSuccess(transform);
}

AlignmentResult Alignment::alignByNPoints(
    MeshData& meshB,
    const MeshData& meshA,
    const std::vector<PointPair>& pointPairs,
    const AlignmentOptions& options)
{
    if (pointPairs.size() < 3) {
        return AlignmentResult::createFailure("At least 3 point pairs are required for alignment");
    }
    
    // Extract source and target points
    std::vector<glm::vec3> sourcePoints, targetPoints;
    std::vector<float> weights;
    sourcePoints.reserve(pointPairs.size());
    targetPoints.reserve(pointPairs.size());
    weights.reserve(pointPairs.size());
    
    for (const auto& pair : pointPairs) {
        sourcePoints.push_back(pair.source);
        targetPoints.push_back(pair.target);
        weights.push_back(pair.weight);
    }
    
    // Check for collinearity
    if (pointPairs.size() == 3) {
        glm::vec3 v1 = targetPoints[1] - targetPoints[0];
        glm::vec3 v2 = targetPoints[2] - targetPoints[0];
        glm::vec3 cross = glm::cross(v1, v2);
        if (glm::length(cross) < 1e-6f) {
            return AlignmentResult::createFailure("Points are collinear - cannot determine unique alignment");
        }
    }
    
    // Compute rigid transformation using SVD
    glm::mat4 transform = computeRigidTransform(sourcePoints, targetPoints, weights);
    
    // Compute error
    AlignmentResult result = AlignmentResult::createSuccess(transform);
    if (options.computeError) {
        std::vector<glm::vec3> transformedPoints;
        transformedPoints.reserve(sourcePoints.size());
        for (const auto& p : sourcePoints) {
            glm::vec4 tp = transform * glm::vec4(p, 1.0f);
            transformedPoints.push_back(glm::vec3(tp));
        }
        auto [rms, maxErr] = computeError(transformedPoints, targetPoints);
        result.rmsError = rms;
        result.maxError = maxErr;
    }
    
    // Apply transformation
    if (!options.preview) {
        meshB.transform(transform);
    }
    
    return result;
}

AlignmentResult Alignment::fineAlign(
    MeshData& meshB,
    const MeshData& meshA,
    int maxIterations,
    float convergenceThreshold,
    ProgressCallback progress)
{
    ICPOptions icpOptions;
    icpOptions.maxIterations = maxIterations;
    icpOptions.convergenceThreshold = convergenceThreshold;
    icpOptions.algorithm = ICPAlgorithm::PointToPlane;
    icpOptions.outlierRejection = true;
    icpOptions.outlierThreshold = 3.0f;
    
    ICP icp;
    ICPResult icpResult = icp.align(meshB, meshA, icpOptions, progress);
    
    AlignmentResult result;
    result.success = icpResult.converged;
    result.transform = icpResult.transform;
    result.rmsError = icpResult.finalRMSError;
    result.iterationsUsed = icpResult.iterationsUsed;
    
    if (!icpResult.converged) {
        result.errorMessage = "ICP did not converge";
    }
    
    result.decomposeTransform();
    return result;
}

std::pair<float, float> Alignment::computeError(
    const std::vector<glm::vec3>& sourcePoints,
    const std::vector<glm::vec3>& targetPoints)
{
    if (sourcePoints.size() != targetPoints.size() || sourcePoints.empty()) {
        return {0.0f, 0.0f};
    }
    
    float sumSquared = 0.0f;
    float maxError = 0.0f;
    
    for (size_t i = 0; i < sourcePoints.size(); ++i) {
        float dist = glm::length(sourcePoints[i] - targetPoints[i]);
        sumSquared += dist * dist;
        maxError = std::max(maxError, dist);
    }
    
    float rms = std::sqrt(sumSquared / static_cast<float>(sourcePoints.size()));
    return {rms, maxError};
}

glm::vec3 Alignment::computeCentroid(const std::vector<glm::vec3>& points) {
    if (points.empty()) {
        return glm::vec3(0.0f);
    }
    
    glm::vec3 sum(0.0f);
    for (const auto& p : points) {
        sum += p;
    }
    return sum / static_cast<float>(points.size());
}

glm::mat4 Alignment::computeRigidTransform(
    const std::vector<glm::vec3>& sourcePoints,
    const std::vector<glm::vec3>& targetPoints,
    const std::vector<float>& weights)
{
    if (sourcePoints.size() != targetPoints.size() || sourcePoints.empty()) {
        return glm::mat4(1.0f);
    }
    
    const size_t n = sourcePoints.size();
    bool useWeights = !weights.empty() && weights.size() == n;
    
    // Compute weighted centroids
    glm::vec3 srcCentroid(0.0f), tgtCentroid(0.0f);
    float totalWeight = 0.0f;
    
    for (size_t i = 0; i < n; ++i) {
        float w = useWeights ? weights[i] : 1.0f;
        srcCentroid += w * sourcePoints[i];
        tgtCentroid += w * targetPoints[i];
        totalWeight += w;
    }
    srcCentroid /= totalWeight;
    tgtCentroid /= totalWeight;
    
    // Compute centered points and covariance matrix H
    // H = sum(w * (src - srcCentroid) * (tgt - tgtCentroid)^T)
    glm::mat3 H(0.0f);
    
    for (size_t i = 0; i < n; ++i) {
        float w = useWeights ? weights[i] : 1.0f;
        glm::vec3 srcCentered = sourcePoints[i] - srcCentroid;
        glm::vec3 tgtCentered = targetPoints[i] - tgtCentroid;
        
        // Outer product: srcCentered * tgtCentered^T
        H[0][0] += w * srcCentered.x * tgtCentered.x;
        H[0][1] += w * srcCentered.x * tgtCentered.y;
        H[0][2] += w * srcCentered.x * tgtCentered.z;
        H[1][0] += w * srcCentered.y * tgtCentered.x;
        H[1][1] += w * srcCentered.y * tgtCentered.y;
        H[1][2] += w * srcCentered.y * tgtCentered.z;
        H[2][0] += w * srcCentered.z * tgtCentered.x;
        H[2][1] += w * srcCentered.z * tgtCentered.y;
        H[2][2] += w * srcCentered.z * tgtCentered.z;
    }
    
    // SVD of H: H = U * S * V^T
    // Optimal rotation R = V * U^T
    // Using power iteration for SVD (simplified implementation)
    // For production, use Eigen or similar library
    
    // Compute H^T * H
    glm::mat3 HtH = glm::transpose(H) * H;
    
    // Power iteration to find eigenvectors
    // FIX: Added convergence checking and better handling of edge cases
    glm::vec3 v1(1.0f, 0.0f, 0.0f);
    glm::vec3 v2(0.0f, 1.0f, 0.0f);
    glm::vec3 v3(0.0f, 0.0f, 1.0f);
    
    const float convergenceThreshold = 1e-8f;
    const int maxIterations = 50;
    
    // Power iteration with convergence check
    for (int iter = 0; iter < maxIterations; ++iter) {
        glm::vec3 v1_new = HtH * v1;
        float len = glm::length(v1_new);
        if (len < 1e-10f) break;
        v1_new = v1_new / len;
        
        float change = glm::length(v1_new - v1);
        v1 = v1_new;
        if (change < convergenceThreshold) break;
    }
    
    // Gram-Schmidt for v2 with convergence check
    v2 = v2 - glm::dot(v2, v1) * v1;
    if (glm::length(v2) < 1e-6f) {
        // v2 was parallel to v1, pick perpendicular vector
        v2 = (std::abs(v1.x) < 0.9f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        v2 = v2 - glm::dot(v2, v1) * v1;
    }
    v2 = glm::normalize(v2);
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        glm::vec3 v2_new = HtH * v2;
        v2_new = v2_new - glm::dot(v2_new, v1) * v1;
        float len = glm::length(v2_new);
        if (len < 1e-10f) break;
        v2_new = v2_new / len;
        
        float change = glm::length(v2_new - v2);
        v2 = v2_new;
        if (change < convergenceThreshold) break;
    }
    
    // v3 is cross product
    v3 = glm::cross(v1, v2);
    
    // V matrix (columns are v1, v2, v3)
    glm::mat3 V(v1, v2, v3);
    
    // U = H * V * S^-1, but for orthogonal approximation:
    // R = H * (H^T * H)^(-1/2)
    // Simplified: use polar decomposition
    
    glm::mat3 U = H * V;
    U[0] = glm::normalize(U[0]);
    U[1] = glm::normalize(U[1]);
    U[2] = glm::normalize(U[2]);
    
    // Ensure right-handed coordinate system
    if (glm::determinant(U * glm::transpose(V)) < 0) {
        V[2] = -V[2];
    }
    
    glm::mat3 R = U * glm::transpose(V);
    
    // Orthonormalize R using Gram-Schmidt
    R[0] = glm::normalize(R[0]);
    R[1] = R[1] - glm::dot(R[1], R[0]) * R[0];
    R[1] = glm::normalize(R[1]);
    R[2] = glm::cross(R[0], R[1]);
    
    // Compute translation: t = tgtCentroid - R * srcCentroid
    glm::vec3 t = tgtCentroid - R * srcCentroid;
    
    // Build 4x4 transformation matrix
    glm::mat4 transform(1.0f);
    transform[0] = glm::vec4(R[0], 0.0f);
    transform[1] = glm::vec4(R[1], 0.0f);
    transform[2] = glm::vec4(R[2], 0.0f);
    transform[3] = glm::vec4(t, 1.0f);
    
    return transform;
}

} // namespace geometry
} // namespace dc3d
