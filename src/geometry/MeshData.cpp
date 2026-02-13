/**
 * @file MeshData.cpp
 * @brief Implementation of MeshData core mesh structure
 */

#include "MeshData.h"

#include <algorithm>
#include <unordered_map>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace dc3d {
namespace geometry {

// FIX Bug 28: Define named constants for magic numbers
namespace {
    // Epsilon values for floating-point comparisons
    constexpr float EPSILON_TINY = 1e-10f;       // For near-zero checks (e.g., normalization)
    constexpr float EPSILON_TOLERANCE = 1e-7f;   // For tolerance clamping
    constexpr float EPSILON_AREA = 1e-10f;       // For degenerate face detection
} // anonymous namespace

namespace {

// Hash function for glm::vec3 for use in unordered_map
struct Vec3Hash {
    size_t cellSize;
    
    // FIX: Clamp to reasonable range to prevent overflow with very small tolerances
    Vec3Hash(float tolerance) {
        float inv = 1.0f / std::max(tolerance, 1e-7f);
        cellSize = static_cast<size_t>(std::min(inv, 1e7f));
    }
    
    size_t operator()(const glm::vec3& v) const {
        auto ix = static_cast<int64_t>(std::floor(v.x * cellSize));
        auto iy = static_cast<int64_t>(std::floor(v.y * cellSize));
        auto iz = static_cast<int64_t>(std::floor(v.z * cellSize));
        
        // Combine hashes using prime numbers
        size_t h = 0;
        h ^= std::hash<int64_t>{}(ix) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int64_t>{}(iy) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int64_t>{}(iz) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

} // anonymous namespace

bool MeshData::isValid() const {
    if (vertices_.empty() || indices_.empty()) {
        return false;
    }
    
    // Check that index count is multiple of 3
    if (indices_.size() % 3 != 0) {
        return false;
    }
    
    // Check that all indices are valid
    size_t vertexCount = vertices_.size();
    for (uint32_t idx : indices_) {
        if (idx >= vertexCount) {
            return false;
        }
    }
    
    // Check normals size if present
    if (!normals_.empty() && normals_.size() != vertices_.size()) {
        return false;
    }
    
    // Check UVs size if present
    if (!uvs_.empty() && uvs_.size() != vertices_.size()) {
        return false;
    }
    
    return true;
}

const BoundingBox& MeshData::boundingBox() const {
    if (boundsDirty_) {
        updateBounds();
    }
    return bounds_;
}

void MeshData::updateBounds() const {
    bounds_.reset();
    for (const auto& v : vertices_) {
        bounds_.expand(v);
    }
    boundsDirty_ = false;
}

MeshStats MeshData::computeStats() const {
    MeshStats stats;
    stats.vertexCount = vertices_.size();
    stats.faceCount = indices_.size() / 3;
    stats.bounds = boundingBox();
    stats.hasNormals = hasNormals();
    stats.hasUVs = hasUVs();
    stats.surfaceArea = surfaceArea();
    
    // Edge count for closed manifold: E = 3F/2
    // This is an approximation
    stats.edgeCount = (indices_.size() / 3) * 3 / 2;
    
    // TODO: Compute watertightness and boundary edges
    // This requires building edge adjacency, which is done in HalfEdgeMesh
    
    return stats;
}

void MeshData::reserveVertices(size_t count) {
    vertices_.reserve(count);
    normals_.reserve(count);
}

void MeshData::reserveFaces(size_t count) {
    indices_.reserve(count * 3);
}

uint32_t MeshData::addVertex(const glm::vec3& position) {
    uint32_t idx = static_cast<uint32_t>(vertices_.size());
    vertices_.push_back(position);
    invalidateBounds();
    return idx;
}

uint32_t MeshData::addVertex(const glm::vec3& position, const glm::vec3& normal) {
    // FIX Bug 23: Document behavior when mixing addVertex calls with/without normals
    // If vertices were added without normals previously, this fills the gaps with zero normals.
    // For consistent behavior, either always use addVertex with normals, or call computeNormals()
    // after adding all vertices to generate proper normals for the entire mesh.
    uint32_t idx = static_cast<uint32_t>(vertices_.size());
    vertices_.push_back(position);
    
    // Ensure normals array is sized correctly - fill gaps with zero normals
    while (normals_.size() < vertices_.size() - 1) {
        normals_.push_back(glm::vec3(0.0f));  // Zero normal indicates uninitialized
    }
    normals_.push_back(normal);
    
    invalidateBounds();
    return idx;
}

void MeshData::addFace(uint32_t v0, uint32_t v1, uint32_t v2) {
    indices_.push_back(v0);
    indices_.push_back(v1);
    indices_.push_back(v2);
}

void MeshData::clear() {
    vertices_.clear();
    indices_.clear();
    normals_.clear();
    uvs_.clear();
    bounds_.reset();
    boundsDirty_ = true;
}

void MeshData::computeNormals() {
    if (vertices_.empty() || indices_.empty()) {
        return;
    }
    
    // Initialize normals to zero
    normals_.assign(vertices_.size(), glm::vec3(0.0f));
    
    // Accumulate face normals for each vertex
    size_t numFaces = indices_.size() / 3;
    for (size_t f = 0; f < numFaces; ++f) {
        uint32_t i0 = indices_[f * 3 + 0];
        uint32_t i1 = indices_[f * 3 + 1];
        uint32_t i2 = indices_[f * 3 + 2];
        
        const glm::vec3& v0 = vertices_[i0];
        const glm::vec3& v1 = vertices_[i1];
        const glm::vec3& v2 = vertices_[i2];
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::cross(edge1, edge2);
        
        // Add to each vertex's normal (will normalize later)
        normals_[i0] += faceNormal;
        normals_[i1] += faceNormal;
        normals_[i2] += faceNormal;
    }
    
    // Normalize all normals
    for (auto& n : normals_) {
        float len = glm::length(n);
        if (len > 1e-10f) {
            n /= len;
        } else {
            n = glm::vec3(0.0f, 0.0f, 1.0f);  // Default up for degenerate cases
        }
    }
}

void MeshData::computeNormalsWeighted() {
    if (vertices_.empty() || indices_.empty()) {
        return;
    }
    
    // Initialize normals to zero
    normals_.assign(vertices_.size(), glm::vec3(0.0f));
    
    // Accumulate angle-weighted face normals for each vertex
    size_t numFaces = indices_.size() / 3;
    for (size_t f = 0; f < numFaces; ++f) {
        uint32_t i0 = indices_[f * 3 + 0];
        uint32_t i1 = indices_[f * 3 + 1];
        uint32_t i2 = indices_[f * 3 + 2];
        
        const glm::vec3& v0 = vertices_[i0];
        const glm::vec3& v1 = vertices_[i1];
        const glm::vec3& v2 = vertices_[i2];
        
        glm::vec3 e01 = v1 - v0;
        glm::vec3 e02 = v2 - v0;
        glm::vec3 e12 = v2 - v1;
        
        glm::vec3 faceNormal = glm::cross(e01, e02);
        float area2 = glm::length(faceNormal);
        
        if (area2 < 1e-10f) continue;
        
        faceNormal /= area2;  // Unit normal
        
        // Compute angles at each vertex
        float len01 = glm::length(e01);
        float len02 = glm::length(e02);
        float len12 = glm::length(e12);
        
        if (len01 < 1e-10f || len02 < 1e-10f || len12 < 1e-10f) continue;
        
        float angle0 = std::acos(glm::clamp(glm::dot(e01, e02) / (len01 * len02), -1.0f, 1.0f));
        float angle1 = std::acos(glm::clamp(glm::dot(-e01, e12) / (len01 * len12), -1.0f, 1.0f));
        float angle2 = std::acos(glm::clamp(glm::dot(-e02, -e12) / (len02 * len12), -1.0f, 1.0f));
        
        // Weight by angle
        normals_[i0] += faceNormal * angle0;
        normals_[i1] += faceNormal * angle1;
        normals_[i2] += faceNormal * angle2;
    }
    
    // Normalize all normals
    for (auto& n : normals_) {
        float len = glm::length(n);
        if (len > 1e-10f) {
            n /= len;
        } else {
            n = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }
}

void MeshData::flipNormals() {
    // Flip vertex normals
    for (auto& n : normals_) {
        n = -n;
    }
    
    // Reverse winding order of all faces
    size_t numFaces = indices_.size() / 3;
    for (size_t f = 0; f < numFaces; ++f) {
        std::swap(indices_[f * 3 + 1], indices_[f * 3 + 2]);
    }
}

void MeshData::clearNormals() {
    normals_.clear();
}

glm::vec3 MeshData::faceNormal(size_t faceIndex) const {
    if (faceIndex * 3 + 2 >= indices_.size()) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
    
    uint32_t i0 = indices_[faceIndex * 3 + 0];
    uint32_t i1 = indices_[faceIndex * 3 + 1];
    uint32_t i2 = indices_[faceIndex * 3 + 2];
    
    const glm::vec3& v0 = vertices_[i0];
    const glm::vec3& v1 = vertices_[i1];
    const glm::vec3& v2 = vertices_[i2];
    
    glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
    float len = glm::length(normal);
    
    return len > 1e-10f ? normal / len : glm::vec3(0.0f, 0.0f, 1.0f);
}

float MeshData::faceArea(size_t faceIndex) const {
    if (faceIndex * 3 + 2 >= indices_.size()) {
        return 0.0f;
    }
    
    uint32_t i0 = indices_[faceIndex * 3 + 0];
    uint32_t i1 = indices_[faceIndex * 3 + 1];
    uint32_t i2 = indices_[faceIndex * 3 + 2];
    
    const glm::vec3& v0 = vertices_[i0];
    const glm::vec3& v1 = vertices_[i1];
    const glm::vec3& v2 = vertices_[i2];
    
    return 0.5f * glm::length(glm::cross(v1 - v0, v2 - v0));
}

float MeshData::surfaceArea() const {
    float area = 0.0f;
    size_t numFaces = indices_.size() / 3;
    
    for (size_t f = 0; f < numFaces; ++f) {
        area += faceArea(f);
    }
    
    return area;
}

float MeshData::volume() const {
    // Compute signed volume using divergence theorem
    // V = (1/6) * sum over faces of (v0 · (v1 × v2))
    float vol = 0.0f;
    size_t numFaces = indices_.size() / 3;
    
    for (size_t f = 0; f < numFaces; ++f) {
        uint32_t i0 = indices_[f * 3 + 0];
        uint32_t i1 = indices_[f * 3 + 1];
        uint32_t i2 = indices_[f * 3 + 2];
        
        const glm::vec3& v0 = vertices_[i0];
        const glm::vec3& v1 = vertices_[i1];
        const glm::vec3& v2 = vertices_[i2];
        
        vol += glm::dot(v0, glm::cross(v1, v2));
    }
    
    return std::abs(vol) / 6.0f;
}

glm::vec3 MeshData::centroid() const {
    if (vertices_.empty()) {
        return glm::vec3(0.0f);
    }
    
    glm::vec3 sum(0.0f);
    for (const auto& v : vertices_) {
        sum += v;
    }
    
    return sum / static_cast<float>(vertices_.size());
}

void MeshData::transform(const glm::mat4& matrix) {
    // Transform positions
    for (auto& v : vertices_) {
        glm::vec4 transformed = matrix * glm::vec4(v, 1.0f);
        v = glm::vec3(transformed) / transformed.w;
    }
    
    // Transform normals using normal matrix (transpose of inverse of upper-left 3x3)
    if (!normals_.empty()) {
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(matrix)));
        for (auto& n : normals_) {
            n = glm::normalize(normalMatrix * n);
        }
    }
    
    invalidateBounds();
}

void MeshData::translate(const glm::vec3& offset) {
    for (auto& v : vertices_) {
        v += offset;
    }
    invalidateBounds();
}

void MeshData::scale(float factor) {
    for (auto& v : vertices_) {
        v *= factor;
    }
    invalidateBounds();
}

void MeshData::scale(const glm::vec3& factors) {
    for (auto& v : vertices_) {
        v *= factors;
    }
    invalidateBounds();
}

void MeshData::centerAtOrigin() {
    glm::vec3 center = centroid();
    translate(-center);
}

void MeshData::normalizeToUnitCube() {
    const BoundingBox& box = boundingBox();
    
    if (!box.isValid()) return;
    
    glm::vec3 center = box.center();
    float maxDim = std::max({box.dimensions().x, box.dimensions().y, box.dimensions().z});
    
    if (maxDim < 1e-10f) return;
    
    translate(-center);
    scale(1.0f / maxDim);
}

size_t MeshData::countDegenerateFaces(float areaThreshold) const {
    size_t count = 0;
    size_t numFaces = indices_.size() / 3;
    
    for (size_t f = 0; f < numFaces; ++f) {
        if (faceArea(f) < areaThreshold) {
            ++count;
        }
    }
    
    return count;
}

size_t MeshData::removeDegenerateFaces(float areaThreshold) {
    std::vector<uint32_t> newIndices;
    newIndices.reserve(indices_.size());
    
    size_t numFaces = indices_.size() / 3;
    size_t removed = 0;
    
    for (size_t f = 0; f < numFaces; ++f) {
        if (faceArea(f) >= areaThreshold) {
            newIndices.push_back(indices_[f * 3 + 0]);
            newIndices.push_back(indices_[f * 3 + 1]);
            newIndices.push_back(indices_[f * 3 + 2]);
        } else {
            ++removed;
        }
    }
    
    indices_ = std::move(newIndices);
    return removed;
}

size_t MeshData::countDuplicateVertices(float tolerance) const {
    if (vertices_.empty()) return 0;
    
    // FIX Bug 24: Check neighbor cells to catch duplicates near cell boundaries
    // Use spatial hashing for efficient duplicate detection
    Vec3Hash hasher(tolerance);
    std::unordered_multimap<size_t, size_t> spatialHash;
    
    for (size_t i = 0; i < vertices_.size(); ++i) {
        spatialHash.emplace(hasher(vertices_[i]), i);
    }
    
    size_t duplicates = 0;
    std::vector<bool> counted(vertices_.size(), false);
    
    float tolSq = tolerance * tolerance;
    float cellSize = std::max(tolerance, 1e-7f);
    
    for (size_t i = 0; i < vertices_.size(); ++i) {
        if (counted[i]) continue;
        
        const glm::vec3& vi = vertices_[i];
        
        // Check 3x3x3 neighborhood of cells to catch boundary cases
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    glm::vec3 offset(dx * cellSize, dy * cellSize, dz * cellSize);
                    size_t h = hasher(vi + offset);
                    
                    auto range = spatialHash.equal_range(h);
                    for (auto it = range.first; it != range.second; ++it) {
                        size_t j = it->second;
                        if (j <= i) continue;
                        
                        if (glm::length2(vertices_[j] - vi) < tolSq) {
                            if (!counted[j]) {
                                counted[j] = true;
                                ++duplicates;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return duplicates;
}

size_t MeshData::mergeDuplicateVertices(float tolerance, ProgressCallback progress) {
    if (vertices_.empty()) return 0;
    
    const size_t totalVertices = vertices_.size();
    const bool reportProgress = progress && totalVertices > 1000000;
    
    // Use spatial hashing for efficient duplicate detection
    Vec3Hash hasher(tolerance);
    std::unordered_map<size_t, std::vector<size_t>> spatialHash;
    
    // Build spatial hash
    for (size_t i = 0; i < totalVertices; ++i) {
        spatialHash[hasher(vertices_[i])].push_back(i);
        
        if (reportProgress && (i % 100000 == 0)) {
            if (!progress(static_cast<float>(i) / (2.0f * totalVertices))) {
                return 0;  // Cancelled
            }
        }
    }
    
    // Map old indices to new indices
    std::vector<uint32_t> indexMap(totalVertices);
    std::vector<glm::vec3> newVertices;
    std::vector<glm::vec3> newNormals;
    std::vector<glm::vec2> newUVs;
    
    newVertices.reserve(totalVertices);
    if (hasNormals()) newNormals.reserve(totalVertices);
    if (hasUVs()) newUVs.reserve(totalVertices);
    
    float tolSq = tolerance * tolerance;
    size_t mergedCount = 0;
    
    for (size_t i = 0; i < totalVertices; ++i) {
        const glm::vec3& vi = vertices_[i];
        size_t h = hasher(vi);
        
        // Check if we've already processed a duplicate of this vertex
        bool foundDuplicate = false;
        const auto& bucket = spatialHash[h];
        
        for (size_t j : bucket) {
            if (j >= i) break;  // Only check previously processed vertices
            
            if (glm::length2(vertices_[j] - vi) < tolSq) {
                // Found a duplicate - use its new index
                indexMap[i] = indexMap[j];
                foundDuplicate = true;
                ++mergedCount;
                break;
            }
        }
        
        if (!foundDuplicate) {
            // New unique vertex
            indexMap[i] = static_cast<uint32_t>(newVertices.size());
            newVertices.push_back(vi);
            
            if (hasNormals()) {
                newNormals.push_back(normals_[i]);
            }
            if (hasUVs()) {
                newUVs.push_back(uvs_[i]);
            }
        }
        
        if (reportProgress && (i % 100000 == 0)) {
            if (!progress(0.5f + static_cast<float>(i) / (2.0f * totalVertices))) {
                return mergedCount;  // Cancelled
            }
        }
    }
    
    // Update indices to use new vertex indices
    for (auto& idx : indices_) {
        idx = indexMap[idx];
    }
    
    // Replace vertex arrays
    vertices_ = std::move(newVertices);
    if (hasNormals()) {
        normals_ = std::move(newNormals);
    }
    if (hasUVs()) {
        uvs_ = std::move(newUVs);
    }
    
    invalidateBounds();
    
    if (progress) {
        progress(1.0f);
    }
    
    return mergedCount;
}

size_t MeshData::memoryUsage() const {
    size_t bytes = 0;
    bytes += vertices_.capacity() * sizeof(glm::vec3);
    bytes += indices_.capacity() * sizeof(uint32_t);
    bytes += normals_.capacity() * sizeof(glm::vec3);
    bytes += uvs_.capacity() * sizeof(glm::vec2);
    bytes += sizeof(BoundingBox);
    bytes += sizeof(bool);
    return bytes;
}

void MeshData::shrinkToFit() {
    vertices_.shrink_to_fit();
    indices_.shrink_to_fit();
    normals_.shrink_to_fit();
    uvs_.shrink_to_fit();
}

} // namespace geometry
} // namespace dc3d
