/**
 * @file ICP.cpp
 * @brief Implementation of Iterative Closest Point algorithm
 */

#include "ICP.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace dc3d {
namespace geometry {

// ============================================================================
// KDTree Implementation
// ============================================================================

void KDTree::build(const std::vector<glm::vec3>& points,
                   const std::vector<glm::vec3>& normals) {
    points_ = points;
    normals_ = normals;
    
    if (points_.empty()) {
        root_ = nullptr;
        return;
    }
    
    std::vector<int> indices(points_.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    root_ = buildRecursive(indices, 0, points_, normals_);
}

std::unique_ptr<KDNode> KDTree::buildRecursive(
    std::vector<int>& indices, int depth,
    const std::vector<glm::vec3>& points,
    const std::vector<glm::vec3>& normals)
{
    if (indices.empty()) {
        return nullptr;
    }
    
    int axis = depth % 3;
    
    // Sort by current axis
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return points[a][axis] < points[b][axis];
    });
    
    size_t mid = indices.size() / 2;
    
    auto node = std::make_unique<KDNode>();
    node->index = indices[mid];
    node->point = points[indices[mid]];
    node->normal = normals.empty() ? glm::vec3(0, 0, 1) : normals[indices[mid]];
    node->splitAxis = axis;
    
    // Build subtrees
    std::vector<int> leftIndices(indices.begin(), indices.begin() + mid);
    std::vector<int> rightIndices(indices.begin() + mid + 1, indices.end());
    
    node->left = buildRecursive(leftIndices, depth + 1, points, normals);
    node->right = buildRecursive(rightIndices, depth + 1, points, normals);
    
    return node;
}

int KDTree::findNearest(const glm::vec3& query, float maxDistance) const {
    float dist;
    glm::vec3 normal;
    return findNearest(query, dist, normal, maxDistance);
}

int KDTree::findNearest(const glm::vec3& query, float& outDistance, glm::vec3& outNormal,
                        float maxDistance) const {
    if (!root_) {
        return -1;
    }
    
    int bestIndex = -1;
    float bestDist = maxDistance;
    glm::vec3 bestNormal(0, 0, 1);
    
    findNearestRecursive(root_.get(), query, bestIndex, bestDist, bestNormal, maxDistance);
    
    outDistance = bestDist;
    outNormal = bestNormal;
    return bestIndex;
}

void KDTree::findNearestRecursive(const KDNode* node, const glm::vec3& query,
                                   int& bestIndex, float& bestDist, glm::vec3& bestNormal,
                                   float maxDistance) const {
    if (!node) {
        return;
    }
    
    // Check current node
    float dist = glm::length(node->point - query);
    if (dist < bestDist) {
        bestDist = dist;
        bestIndex = node->index;
        bestNormal = node->normal;
    }
    
    // Determine which subtree to search first
    int axis = node->splitAxis;
    float diff = query[axis] - node->point[axis];
    
    KDNode* first = diff < 0 ? node->left.get() : node->right.get();
    KDNode* second = diff < 0 ? node->right.get() : node->left.get();
    
    // Search nearer subtree
    findNearestRecursive(first, query, bestIndex, bestDist, bestNormal, maxDistance);
    
    // Check if we need to search farther subtree
    if (std::abs(diff) < bestDist) {
        findNearestRecursive(second, query, bestIndex, bestDist, bestNormal, maxDistance);
    }
}

// ============================================================================
// ICP Implementation
// ============================================================================

ICPResult ICP::align(MeshData& source, const MeshData& target,
                     const ICPOptions& options, ProgressCallback progress) {
    // Extract points and normals from meshes
    std::vector<glm::vec3> sourcePoints = source.vertices();
    std::vector<glm::vec3> sourceNormals = source.normals();
    
    // Use iteration callback to report progress
    ICPIterationCallback iterCallback = nullptr;
    if (progress) {
        iterCallback = [&](const ICPIterationStats& stats) {
            float p = static_cast<float>(stats.iteration) / options.maxIterations;
            return progress(p);
        };
    }
    
    ICPResult result = alignPoints(sourcePoints, sourceNormals, target, options, iterCallback);
    
    // Apply final transformation to mesh
    if (result.converged) {
        source.transform(result.transform);
    }
    
    return result;
}

ICPResult ICP::alignPoints(std::vector<glm::vec3>& sourcePoints,
                           std::vector<glm::vec3>& sourceNormals,
                           const MeshData& target,
                           const ICPOptions& options,
                           ICPIterationCallback iterationCallback) {
    ICPResult result;
    result.transform = glm::mat4(1.0f);
    
    if (sourcePoints.empty() || target.isEmpty()) {
        return result;
    }
    
    // Build KD-Tree for target mesh
    KDTree targetTree;
    targetTree.build(target.vertices(), target.normals());
    
    glm::mat4 cumulativeTransform(1.0f);
    glm::mat4 prevTransform(1.0f);
    
    // Working copy of source points
    std::vector<glm::vec3> workingPoints = sourcePoints;
    
    // Initial error
    auto initialCorr = findCorrespondences(workingPoints, targetTree, options);
    result.initialRMSError = computeRMSError(initialCorr);
    
    for (int iter = 0; iter < options.maxIterations; ++iter) {
        // Find correspondences
        auto correspondences = findCorrespondences(workingPoints, targetTree, options);
        
        if (correspondences.empty()) {
            break;
        }
        
        // Reject outliers
        if (options.outlierRejection) {
            rejectOutliers(correspondences, options);
        }
        
        if (correspondences.size() < 3) {
            break;
        }
        
        // Compute transformation for this iteration
        glm::mat4 iterTransform = computeIterationTransform(correspondences, options.algorithm);
        
        // Apply transformation to working points
        for (auto& p : workingPoints) {
            glm::vec4 tp = iterTransform * glm::vec4(p, 1.0f);
            p = glm::vec3(tp);
        }
        
        // Update cumulative transform
        cumulativeTransform = iterTransform * cumulativeTransform;
        
        // Compute error
        float rmsError = computeRMSError(correspondences);
        result.errorHistory.push_back(rmsError);
        
        // Check convergence
        float transformChange = computeTransformChange(prevTransform, cumulativeTransform);
        
        ICPIterationStats stats;
        stats.iteration = iter;
        stats.rmsError = rmsError;
        stats.correspondenceCount = static_cast<int>(correspondences.size());
        stats.transformChange = transformChange;
        
        if (iterationCallback && !iterationCallback(stats)) {
            // Cancelled
            break;
        }
        
        if (transformChange < options.convergenceThreshold) {
            result.converged = true;
            result.iterationsUsed = iter + 1;
            break;
        }
        
        prevTransform = cumulativeTransform;
        result.iterationsUsed = iter + 1;
    }
    
    // Final correspondences for error
    auto finalCorr = findCorrespondences(workingPoints, targetTree, options);
    result.finalRMSError = computeRMSError(finalCorr);
    result.correspondenceCount = static_cast<int>(finalCorr.size());
    result.transform = cumulativeTransform;
    
    // If we completed all iterations without early termination, consider it converged
    if (result.iterationsUsed == options.maxIterations) {
        result.converged = true;
    }
    
    // Update source points
    sourcePoints = workingPoints;
    
    return result;
}

std::vector<Correspondence> ICP::findCorrespondences(
    const std::vector<glm::vec3>& sourcePoints,
    const KDTree& targetTree,
    const ICPOptions& options)
{
    std::vector<Correspondence> correspondences;
    correspondences.reserve(sourcePoints.size() / options.correspondenceSampling);
    
    for (size_t i = 0; i < sourcePoints.size(); i += options.correspondenceSampling) {
        float distance;
        glm::vec3 normal;
        int targetIdx = targetTree.findNearest(sourcePoints[i], distance, normal,
                                               options.maxCorrespondenceDistance);
        
        if (targetIdx >= 0) {
            Correspondence corr;
            corr.sourceIndex = static_cast<int>(i);
            corr.targetIndex = targetIdx;
            corr.sourcePoint = sourcePoints[i];
            corr.targetPoint = sourcePoints[i]; // Will be updated from tree
            corr.targetNormal = normal;
            corr.distance = distance;
            correspondences.push_back(corr);
        }
    }
    
    return correspondences;
}

void ICP::rejectOutliers(std::vector<Correspondence>& correspondences,
                         const ICPOptions& options) {
    if (correspondences.empty()) {
        return;
    }
    
    // Compute mean and stddev of distances
    float sum = 0.0f;
    for (const auto& c : correspondences) {
        sum += c.distance;
    }
    float mean = sum / correspondences.size();
    
    float sumSq = 0.0f;
    for (const auto& c : correspondences) {
        float diff = c.distance - mean;
        sumSq += diff * diff;
    }
    float stddev = std::sqrt(sumSq / correspondences.size());
    
    // Reject outliers beyond threshold * stddev
    float threshold = mean + options.outlierThreshold * stddev;
    
    correspondences.erase(
        std::remove_if(correspondences.begin(), correspondences.end(),
            [threshold](const Correspondence& c) {
                return c.distance > threshold;
            }),
        correspondences.end()
    );
    
    // Trim highest percentage if requested
    if (options.trimPercentage > 0.0f && !correspondences.empty()) {
        std::sort(correspondences.begin(), correspondences.end(),
            [](const Correspondence& a, const Correspondence& b) {
                return a.distance < b.distance;
            });
        
        size_t keepCount = static_cast<size_t>(
            correspondences.size() * (1.0f - options.trimPercentage));
        keepCount = std::max(keepCount, size_t(3));
        correspondences.resize(keepCount);
    }
}

glm::mat4 ICP::computeIterationTransform(
    const std::vector<Correspondence>& correspondences,
    ICPAlgorithm algorithm)
{
    if (algorithm == ICPAlgorithm::PointToPlane) {
        return computePointToPlaneTransform(correspondences);
    } else {
        return computePointToPointTransform(correspondences);
    }
}

glm::mat4 ICP::computePointToPointTransform(
    const std::vector<Correspondence>& correspondences)
{
    if (correspondences.size() < 3) {
        return glm::mat4(1.0f);
    }
    
    // Extract points
    std::vector<glm::vec3> srcPts, tgtPts;
    srcPts.reserve(correspondences.size());
    tgtPts.reserve(correspondences.size());
    
    for (const auto& c : correspondences) {
        srcPts.push_back(c.sourcePoint);
        tgtPts.push_back(c.targetPoint);
    }
    
    // Compute centroids
    glm::vec3 srcCentroid(0), tgtCentroid(0);
    for (size_t i = 0; i < srcPts.size(); ++i) {
        srcCentroid += srcPts[i];
        tgtCentroid += tgtPts[i];
    }
    srcCentroid /= static_cast<float>(srcPts.size());
    tgtCentroid /= static_cast<float>(tgtPts.size());
    
    // Compute covariance matrix H
    glm::mat3 H(0.0f);
    for (size_t i = 0; i < srcPts.size(); ++i) {
        glm::vec3 s = srcPts[i] - srcCentroid;
        glm::vec3 t = tgtPts[i] - tgtCentroid;
        
        H[0][0] += s.x * t.x; H[0][1] += s.x * t.y; H[0][2] += s.x * t.z;
        H[1][0] += s.y * t.x; H[1][1] += s.y * t.y; H[1][2] += s.y * t.z;
        H[2][0] += s.z * t.x; H[2][1] += s.z * t.y; H[2][2] += s.z * t.z;
    }
    
    // SVD using power iteration (simplified)
    glm::mat3 HtH = glm::transpose(H) * H;
    
    glm::vec3 v1(1, 0, 0), v2(0, 1, 0), v3(0, 0, 1);
    for (int i = 0; i < 30; ++i) {
        v1 = glm::normalize(HtH * v1);
    }
    v2 = v2 - glm::dot(v2, v1) * v1;
    for (int i = 0; i < 30; ++i) {
        v2 = HtH * v2;
        v2 = v2 - glm::dot(v2, v1) * v1;
        if (glm::length(v2) > 1e-6f) v2 = glm::normalize(v2);
    }
    v3 = glm::cross(v1, v2);
    
    glm::mat3 V(v1, v2, v3);
    glm::mat3 U = H * V;
    if (glm::length(U[0]) > 1e-6f) U[0] = glm::normalize(U[0]);
    if (glm::length(U[1]) > 1e-6f) U[1] = glm::normalize(U[1]);
    if (glm::length(U[2]) > 1e-6f) U[2] = glm::normalize(U[2]);
    
    glm::mat3 R = U * glm::transpose(V);
    
    // Orthonormalize
    R[0] = glm::normalize(R[0]);
    R[1] = R[1] - glm::dot(R[1], R[0]) * R[0];
    R[1] = glm::normalize(R[1]);
    R[2] = glm::cross(R[0], R[1]);
    
    // Translation
    glm::vec3 t = tgtCentroid - R * srcCentroid;
    
    // Build 4x4 matrix
    glm::mat4 transform(1.0f);
    transform[0] = glm::vec4(R[0], 0.0f);
    transform[1] = glm::vec4(R[1], 0.0f);
    transform[2] = glm::vec4(R[2], 0.0f);
    transform[3] = glm::vec4(t, 1.0f);
    
    return transform;
}

glm::mat4 ICP::computePointToPlaneTransform(
    const std::vector<Correspondence>& correspondences)
{
    if (correspondences.size() < 6) {
        return computePointToPointTransform(correspondences);
    }
    
    // Point-to-plane uses linearized rotation
    // Minimize sum of (n_i . (R*p_i + t - q_i))^2
    // Linearize R ≈ I + [alpha, beta, gamma]_x
    
    // Build linear system: A^T A x = A^T b
    // x = [alpha, beta, gamma, tx, ty, tz]
    
    const int n = static_cast<int>(correspondences.size());
    
    // 6x6 matrix ATA and 6x1 vector ATb
    float ATA[6][6] = {0};
    float ATb[6] = {0};
    
    for (const auto& c : correspondences) {
        const glm::vec3& p = c.sourcePoint;
        const glm::vec3& q = c.targetPoint;
        const glm::vec3& n = c.targetNormal;
        
        // Cross product p x n contributes to rotation terms
        glm::vec3 cn = glm::cross(p, n);
        
        float a[6] = {cn.x, cn.y, cn.z, n.x, n.y, n.z};
        float b = glm::dot(n, q - p);
        
        // Accumulate ATA and ATb
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 6; ++j) {
                ATA[i][j] += a[i] * a[j];
            }
            ATb[i] += a[i] * b;
        }
    }
    
    // Solve 6x6 system using Gaussian elimination
    float aug[6][7];
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            aug[i][j] = ATA[i][j];
        }
        aug[i][6] = ATb[i];
    }
    
    // Forward elimination
    for (int k = 0; k < 6; ++k) {
        // Find pivot
        int maxRow = k;
        for (int i = k + 1; i < 6; ++i) {
            if (std::abs(aug[i][k]) > std::abs(aug[maxRow][k])) {
                maxRow = i;
            }
        }
        
        // Swap rows
        for (int j = 0; j < 7; ++j) {
            std::swap(aug[k][j], aug[maxRow][j]);
        }
        
        // Check for singularity
        if (std::abs(aug[k][k]) < 1e-10f) {
            return computePointToPointTransform(correspondences);
        }
        
        // Eliminate column
        for (int i = k + 1; i < 6; ++i) {
            float factor = aug[i][k] / aug[k][k];
            for (int j = k; j < 7; ++j) {
                aug[i][j] -= factor * aug[k][j];
            }
        }
    }
    
    // Back substitution
    float x[6];
    for (int i = 5; i >= 0; --i) {
        x[i] = aug[i][6];
        for (int j = i + 1; j < 6; ++j) {
            x[i] -= aug[i][j] * x[j];
        }
        x[i] /= aug[i][i];
    }
    
    // Build rotation matrix from small angles
    float alpha = x[0], beta = x[1], gamma = x[2];
    
    glm::mat3 R(1.0f);
    R[0][1] = -gamma; R[0][2] = beta;
    R[1][0] = gamma;  R[1][2] = -alpha;
    R[2][0] = -beta;  R[2][1] = alpha;
    R = glm::mat3(1.0f) + R;
    
    // Orthonormalize R using Gram-Schmidt
    R[0] = glm::normalize(R[0]);
    R[1] = R[1] - glm::dot(R[1], R[0]) * R[0];
    R[1] = glm::normalize(R[1]);
    R[2] = glm::cross(R[0], R[1]);
    
    glm::vec3 t(x[3], x[4], x[5]);
    
    // Build 4x4 matrix
    glm::mat4 transform(1.0f);
    transform[0] = glm::vec4(R[0], 0.0f);
    transform[1] = glm::vec4(R[1], 0.0f);
    transform[2] = glm::vec4(R[2], 0.0f);
    transform[3] = glm::vec4(t, 1.0f);
    
    return transform;
}

float ICP::computeRMSError(const std::vector<Correspondence>& correspondences) {
    if (correspondences.empty()) {
        return 0.0f;
    }
    
    float sumSq = 0.0f;
    for (const auto& c : correspondences) {
        sumSq += c.distance * c.distance;
    }
    
    return std::sqrt(sumSq / correspondences.size());
}

float ICP::computeTransformChange(const glm::mat4& prev, const glm::mat4& curr) {
    // Compute Frobenius norm of difference
    float sum = 0.0f;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float diff = curr[i][j] - prev[i][j];
            sum += diff * diff;
        }
    }
    return std::sqrt(sum);
}

} // namespace geometry
} // namespace dc3d
