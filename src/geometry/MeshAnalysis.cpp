/**
 * @file MeshAnalysis.cpp
 * @brief Implementation of mesh analysis utilities
 */

#include "MeshAnalysis.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>

namespace dc3d {
namespace geometry {

MeshAnalysisStats MeshAnalysis::analyze(const MeshData& mesh, ProgressCallback progress) {
    MeshAnalysisStats stats;
    
    if (mesh.isEmpty()) {
        return stats;
    }
    
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    // Basic counts
    stats.vertexCount = vertices.size();
    stats.faceCount = indices.size() / 3;
    
    // Data flags
    stats.hasNormals = mesh.hasNormals();
    stats.hasUVs = mesh.hasUVs();
    
    if (progress && !progress(0.1f)) return stats;
    
    // Build edge-face adjacency
    auto edgeMap = buildEdgeFaceAdjacency(mesh);
    stats.edgeCount = edgeMap.size();
    
    if (progress && !progress(0.2f)) return stats;
    
    // Compute bounding box and centroid
    stats.bounds = mesh.boundingBox();
    stats.centroid = mesh.centroid();
    
    // Surface area
    stats.surfaceArea = mesh.surfaceArea();
    
    if (progress && !progress(0.3f)) return stats;
    
    // Edge statistics
    computeEdgeStatistics(mesh, edgeMap, stats);
    
    if (progress && !progress(0.5f)) return stats;
    
    // Face statistics
    computeFaceStatistics(mesh, stats);
    
    if (progress && !progress(0.6f)) return stats;
    
    // Topology statistics
    computeTopologyStatistics(mesh, edgeMap, stats);
    
    if (progress && !progress(0.7f)) return stats;
    
    // Check winding consistency
    stats.hasConsistentWinding = checkConsistentWinding(mesh, edgeMap);
    
    // Manifold check
    stats.isManifold = (stats.nonManifoldEdgeCount == 0 && stats.nonManifoldVertexCount == 0);
    
    // Watertight check
    stats.isWatertight = stats.isManifold && 
                          stats.boundaryEdgeCount == 0 && 
                          stats.hasConsistentWinding;
    
    if (progress && !progress(0.8f)) return stats;
    
    // Volume (only valid for watertight meshes)
    if (stats.isWatertight) {
        stats.volume = mesh.volume();
        stats.volumeValid = true;
    }
    
    // Find holes
    stats.holes = findHoles(mesh);
    stats.holeCount = stats.holes.size();
    
    // Count degenerate faces
    stats.degenerateFaceCount = mesh.countDegenerateFaces();
    
    // Count isolated vertices
    std::vector<bool> vertexUsed(vertices.size(), false);
    for (uint32_t idx : indices) {
        vertexUsed[idx] = true;
    }
    stats.isolatedVertexCount = std::count(vertexUsed.begin(), vertexUsed.end(), false);
    
    if (progress) progress(1.0f);
    
    return stats;
}

bool MeshAnalysis::isWatertight(const MeshData& mesh) {
    if (mesh.isEmpty()) return false;
    
    auto edgeMap = buildEdgeFaceAdjacency(mesh);
    
    // Check that every edge has exactly 2 adjacent faces
    for (const auto& [edge, faces] : edgeMap) {
        if (faces.size() != 2) {
            return false;
        }
    }
    
    // Check consistent winding
    return checkConsistentWinding(mesh, edgeMap);
}

bool MeshAnalysis::isManifold(const MeshData& mesh) {
    if (mesh.isEmpty()) return false;
    
    auto edgeMap = buildEdgeFaceAdjacency(mesh);
    
    // Check that every edge has at most 2 adjacent faces
    for (const auto& [edge, faces] : edgeMap) {
        if (faces.size() > 2) {
            return false;
        }
    }
    
    // TODO: Add vertex manifold check (single fan of triangles)
    return true;
}

std::vector<float> MeshAnalysis::computeCurvature(const MeshData& mesh, CurvatureType type) {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    std::vector<float> curvatures(vertices.size(), 0.0f);
    
    if (mesh.isEmpty()) {
        return curvatures;
    }
    
    // Build vertex-face adjacency
    auto vertexFaces = buildVertexFaceAdjacency(mesh);
    
    // Compute mean curvature using discrete Laplace-Beltrami operator
    for (size_t vi = 0; vi < vertices.size(); ++vi) {
        const glm::vec3& v = vertices[vi];
        const auto& adjacentFaces = vertexFaces[vi];
        
        if (adjacentFaces.empty()) {
            curvatures[vi] = 0.0f;
            continue;
        }
        
        // Collect one-ring neighbors
        std::unordered_map<uint32_t, float> neighborWeights;
        float totalWeight = 0.0f;
        float mixedArea = 0.0f;
        
        for (uint32_t fi : adjacentFaces) {
            uint32_t i0 = indices[fi * 3];
            uint32_t i1 = indices[fi * 3 + 1];
            uint32_t i2 = indices[fi * 3 + 2];
            
            // Find current vertex position in triangle
            uint32_t self, other1, other2;
            if (i0 == vi) {
                self = i0; other1 = i1; other2 = i2;
            } else if (i1 == vi) {
                self = i1; other1 = i2; other2 = i0;
            } else {
                self = i2; other1 = i0; other2 = i1;
            }
            
            const glm::vec3& vSelf = vertices[self];
            const glm::vec3& vOther1 = vertices[other1];
            const glm::vec3& vOther2 = vertices[other2];
            
            // Compute cotangent weights
            glm::vec3 e1 = vOther1 - vSelf;
            glm::vec3 e2 = vOther2 - vSelf;
            glm::vec3 e12 = vOther2 - vOther1;
            
            // Cotangent at other1
            float dot1 = glm::dot(vSelf - vOther1, vOther2 - vOther1);
            float cross1 = glm::length(glm::cross(vSelf - vOther1, vOther2 - vOther1));
            float cot1 = (cross1 > 1e-10f) ? dot1 / cross1 : 0.0f;
            
            // Cotangent at other2
            float dot2 = glm::dot(vSelf - vOther2, vOther1 - vOther2);
            float cross2 = glm::length(glm::cross(vSelf - vOther2, vOther1 - vOther2));
            float cot2 = (cross2 > 1e-10f) ? dot2 / cross2 : 0.0f;
            
            // Add weights for neighbors
            neighborWeights[other1] += cot2 * 0.5f;
            neighborWeights[other2] += cot1 * 0.5f;
            
            // Compute mixed area contribution
            float angle = computeVertexAngle(vSelf, vOther1, vOther2);
            float faceArea = mesh.faceArea(fi);
            
            // Check if triangle is obtuse
            float angle1 = computeVertexAngle(vOther1, vSelf, vOther2);
            float angle2 = computeVertexAngle(vOther2, vSelf, vOther1);
            
            if (angle > M_PI / 2) {
                mixedArea += faceArea / 2.0f;
            } else if (angle1 > M_PI / 2 || angle2 > M_PI / 2) {
                mixedArea += faceArea / 4.0f;
            } else {
                // Voronoi area
                float len1Sq = glm::dot(e1, e1);
                float len2Sq = glm::dot(e2, e2);
                mixedArea += (cot1 * len2Sq + cot2 * len1Sq) / 8.0f;
            }
        }
        
        // Compute mean curvature normal
        glm::vec3 laplacian(0.0f);
        for (const auto& [ni, weight] : neighborWeights) {
            laplacian += weight * (vertices[ni] - v);
        }
        
        // Mean curvature magnitude
        float H = 0.0f;
        if (mixedArea > 1e-10f) {
            H = glm::length(laplacian) / (2.0f * mixedArea);
        }
        
        // Determine sign using normal direction
        if (mesh.hasNormals() && glm::dot(laplacian, mesh.normals()[vi]) < 0.0f) {
            H = -H;
        }
        
        // Store based on curvature type
        switch (type) {
            case CurvatureType::Mean:
                curvatures[vi] = H;
                break;
            case CurvatureType::Maximum:
                curvatures[vi] = std::abs(H);
                break;
            default:
                curvatures[vi] = H;
                break;
        }
    }
    
    return curvatures;
}

CurvatureStats MeshAnalysis::computeCurvatureStats(const std::vector<float>& curvatures) {
    CurvatureStats stats;
    
    if (curvatures.empty()) {
        return stats;
    }
    
    // Min/max
    auto [minIt, maxIt] = std::minmax_element(curvatures.begin(), curvatures.end());
    stats.minCurvature = *minIt;
    stats.maxCurvature = *maxIt;
    
    // Average
    double sum = std::accumulate(curvatures.begin(), curvatures.end(), 0.0);
    stats.avgCurvature = static_cast<float>(sum / curvatures.size());
    
    // Standard deviation
    double sqSum = 0.0;
    for (float c : curvatures) {
        double diff = c - stats.avgCurvature;
        sqSum += diff * diff;
    }
    stats.stddevCurvature = static_cast<float>(std::sqrt(sqSum / curvatures.size()));
    
    return stats;
}

std::vector<Edge> MeshAnalysis::findBoundaryEdges(const MeshData& mesh) {
    std::vector<Edge> boundaryEdges;
    
    auto edgeMap = buildEdgeFaceAdjacency(mesh);
    
    for (const auto& [edge, faces] : edgeMap) {
        if (faces.size() == 1) {
            boundaryEdges.push_back(edge);
        }
    }
    
    return boundaryEdges;
}

std::vector<Edge> MeshAnalysis::findNonManifoldEdges(const MeshData& mesh) {
    std::vector<Edge> nonManifoldEdges;
    
    auto edgeMap = buildEdgeFaceAdjacency(mesh);
    
    for (const auto& [edge, faces] : edgeMap) {
        if (faces.size() > 2) {
            nonManifoldEdges.push_back(edge);
        }
    }
    
    return nonManifoldEdges;
}

std::vector<HoleInfo> MeshAnalysis::findHoles(const MeshData& mesh) {
    std::vector<HoleInfo> holes;
    
    auto edgeMap = buildEdgeFaceAdjacency(mesh);
    std::unordered_set<Edge, EdgeHash> visitedEdges;
    
    // Find boundary edges and trace holes
    for (const auto& [edge, faces] : edgeMap) {
        if (faces.size() == 1 && visitedEdges.find(edge) == visitedEdges.end()) {
            HoleInfo hole = traceHole(mesh, edge, visitedEdges, edgeMap);
            if (!hole.boundaryVertices.empty()) {
                holes.push_back(std::move(hole));
            }
        }
    }
    
    return holes;
}

float MeshAnalysis::computeTriangleAspectRatio(
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2
) {
    // Compute edge lengths
    float e0 = glm::length(v1 - v0);
    float e1 = glm::length(v2 - v1);
    float e2 = glm::length(v0 - v2);
    
    // Find longest edge
    float maxEdge = std::max({e0, e1, e2});
    
    // Compute area
    glm::vec3 cross = glm::cross(v1 - v0, v2 - v0);
    float area = 0.5f * glm::length(cross);
    
    // Shortest altitude = 2 * area / longest edge
    if (maxEdge < 1e-10f) return std::numeric_limits<float>::max();
    
    float shortestAltitude = 2.0f * area / maxEdge;
    
    if (shortestAltitude < 1e-10f) return std::numeric_limits<float>::max();
    
    // Aspect ratio = longest edge / shortest altitude
    return maxEdge / shortestAltitude;
}

std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>
MeshAnalysis::buildEdgeFaceAdjacency(const MeshData& mesh) {
    std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash> edgeMap;
    
    const auto& indices = mesh.indices();
    size_t faceCount = indices.size() / 3;
    
    for (size_t fi = 0; fi < faceCount; ++fi) {
        uint32_t i0 = indices[fi * 3];
        uint32_t i1 = indices[fi * 3 + 1];
        uint32_t i2 = indices[fi * 3 + 2];
        
        edgeMap[Edge(i0, i1)].push_back(static_cast<uint32_t>(fi));
        edgeMap[Edge(i1, i2)].push_back(static_cast<uint32_t>(fi));
        edgeMap[Edge(i2, i0)].push_back(static_cast<uint32_t>(fi));
    }
    
    return edgeMap;
}

std::vector<std::vector<uint32_t>>
MeshAnalysis::buildVertexFaceAdjacency(const MeshData& mesh) {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    std::vector<std::vector<uint32_t>> vertexFaces(vertices.size());
    
    size_t faceCount = indices.size() / 3;
    for (size_t fi = 0; fi < faceCount; ++fi) {
        uint32_t i0 = indices[fi * 3];
        uint32_t i1 = indices[fi * 3 + 1];
        uint32_t i2 = indices[fi * 3 + 2];
        
        vertexFaces[i0].push_back(static_cast<uint32_t>(fi));
        vertexFaces[i1].push_back(static_cast<uint32_t>(fi));
        vertexFaces[i2].push_back(static_cast<uint32_t>(fi));
    }
    
    return vertexFaces;
}

float MeshAnalysis::computeVertexAngle(
    const glm::vec3& v,
    const glm::vec3& v1,
    const glm::vec3& v2
) {
    glm::vec3 e1 = glm::normalize(v1 - v);
    glm::vec3 e2 = glm::normalize(v2 - v);
    
    float dot = glm::clamp(glm::dot(e1, e2), -1.0f, 1.0f);
    return std::acos(dot);
}

void MeshAnalysis::computeEdgeStatistics(
    const MeshData& mesh,
    const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap,
    MeshAnalysisStats& stats
) {
    const auto& vertices = mesh.vertices();
    
    if (edgeMap.empty()) return;
    
    std::vector<float> edgeLengths;
    edgeLengths.reserve(edgeMap.size());
    
    for (const auto& [edge, faces] : edgeMap) {
        float length = glm::length(vertices[edge.v1] - vertices[edge.v0]);
        edgeLengths.push_back(length);
    }
    
    // Min/max
    auto [minIt, maxIt] = std::minmax_element(edgeLengths.begin(), edgeLengths.end());
    stats.minEdgeLength = *minIt;
    stats.maxEdgeLength = *maxIt;
    
    // Average
    double sum = std::accumulate(edgeLengths.begin(), edgeLengths.end(), 0.0);
    stats.avgEdgeLength = static_cast<float>(sum / edgeLengths.size());
    
    // Standard deviation
    double sqSum = 0.0;
    for (float len : edgeLengths) {
        double diff = len - stats.avgEdgeLength;
        sqSum += diff * diff;
    }
    stats.stddevEdgeLength = static_cast<float>(std::sqrt(sqSum / edgeLengths.size()));
}

void MeshAnalysis::computeFaceStatistics(const MeshData& mesh, MeshAnalysisStats& stats) {
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    size_t faceCount = indices.size() / 3;
    if (faceCount == 0) return;
    
    stats.minFaceArea = std::numeric_limits<float>::max();
    stats.maxFaceArea = 0.0f;
    double areaSum = 0.0;
    
    for (size_t fi = 0; fi < faceCount; ++fi) {
        const glm::vec3& v0 = vertices[indices[fi * 3]];
        const glm::vec3& v1 = vertices[indices[fi * 3 + 1]];
        const glm::vec3& v2 = vertices[indices[fi * 3 + 2]];
        
        // Face area
        float area = mesh.faceArea(fi);
        stats.minFaceArea = std::min(stats.minFaceArea, area);
        stats.maxFaceArea = std::max(stats.maxFaceArea, area);
        areaSum += area;
        
        // Aspect ratio
        float aspectRatio = computeTriangleAspectRatio(v0, v1, v2);
        
        if (aspectRatio < 1.5f) {
            stats.aspectRatios.excellent++;
        } else if (aspectRatio < 3.0f) {
            stats.aspectRatios.good++;
        } else if (aspectRatio < 6.0f) {
            stats.aspectRatios.fair++;
        } else if (aspectRatio < 10.0f) {
            stats.aspectRatios.poor++;
        } else {
            stats.aspectRatios.terrible++;
        }
    }
    
    stats.avgFaceArea = static_cast<float>(areaSum / faceCount);
}

void MeshAnalysis::computeTopologyStatistics(
    const MeshData& mesh,
    const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap,
    MeshAnalysisStats& stats
) {
    stats.boundaryEdgeCount = 0;
    stats.nonManifoldEdgeCount = 0;
    
    for (const auto& [edge, faces] : edgeMap) {
        if (faces.size() == 1) {
            stats.boundaryEdgeCount++;
        } else if (faces.size() > 2) {
            stats.nonManifoldEdgeCount++;
        }
    }
    
    // TODO: Detect non-manifold vertices (vertices with multiple fans)
    stats.nonManifoldVertexCount = 0;
}

bool MeshAnalysis::checkConsistentWinding(
    const MeshData& mesh,
    const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap
) {
    const auto& indices = mesh.indices();
    
    // For each edge shared by exactly 2 faces, check opposite winding
    for (const auto& [edge, faces] : edgeMap) {
        if (faces.size() != 2) continue;
        
        uint32_t f0 = faces[0];
        uint32_t f1 = faces[1];
        
        // Get edge direction in each face
        auto getEdgeDirection = [&](uint32_t faceIdx) -> int {
            uint32_t i0 = indices[faceIdx * 3];
            uint32_t i1 = indices[faceIdx * 3 + 1];
            uint32_t i2 = indices[faceIdx * 3 + 2];
            
            if ((i0 == edge.v0 && i1 == edge.v1) ||
                (i1 == edge.v0 && i2 == edge.v1) ||
                (i2 == edge.v0 && i0 == edge.v1)) {
                return 1;  // v0 -> v1
            }
            return -1;  // v1 -> v0
        };
        
        int dir0 = getEdgeDirection(f0);
        int dir1 = getEdgeDirection(f1);
        
        // Adjacent faces should have opposite edge directions
        if (dir0 == dir1) {
            return false;
        }
    }
    
    return true;
}

HoleInfo MeshAnalysis::traceHole(
    const MeshData& mesh,
    const Edge& startEdge,
    std::unordered_set<Edge, EdgeHash>& visitedEdges,
    const std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash>& edgeMap
) {
    HoleInfo hole;
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    // Find the boundary direction from the face
    uint32_t faceIdx = edgeMap.at(startEdge)[0];
    uint32_t i0 = indices[faceIdx * 3];
    uint32_t i1 = indices[faceIdx * 3 + 1];
    uint32_t i2 = indices[faceIdx * 3 + 2];
    
    // Determine start vertex and direction
    uint32_t currentVertex, nextVertex;
    
    if ((i0 == startEdge.v0 && i1 == startEdge.v1) ||
        (i1 == startEdge.v0 && i2 == startEdge.v1) ||
        (i2 == startEdge.v0 && i0 == startEdge.v1)) {
        // Edge goes v0 -> v1 in face, so boundary goes v1 -> v0
        currentVertex = startEdge.v1;
        nextVertex = startEdge.v0;
    } else {
        currentVertex = startEdge.v0;
        nextVertex = startEdge.v1;
    }
    
    uint32_t startVertex = currentVertex;
    visitedEdges.insert(startEdge);
    hole.boundaryVertices.push_back(currentVertex);
    hole.perimeter = glm::length(vertices[nextVertex] - vertices[currentVertex]);
    
    currentVertex = nextVertex;
    
    // Trace the boundary
    int maxIterations = static_cast<int>(vertices.size()) + 1;
    int iterations = 0;
    
    while (currentVertex != startVertex && iterations < maxIterations) {
        hole.boundaryVertices.push_back(currentVertex);
        
        // Find next boundary edge
        bool found = false;
        for (const auto& [edge, faces] : edgeMap) {
            if (faces.size() != 1) continue;
            if (visitedEdges.count(edge)) continue;
            
            if (edge.v0 == currentVertex || edge.v1 == currentVertex) {
                visitedEdges.insert(edge);
                uint32_t next = (edge.v0 == currentVertex) ? edge.v1 : edge.v0;
                hole.perimeter += glm::length(vertices[next] - vertices[currentVertex]);
                currentVertex = next;
                found = true;
                break;
            }
        }
        
        if (!found) break;
        iterations++;
    }
    
    // Compute centroid and estimated area
    if (hole.boundaryVertices.size() >= 3) {
        glm::vec3 sum(0.0f);
        for (uint32_t vi : hole.boundaryVertices) {
            sum += vertices[vi];
        }
        hole.centroid = sum / static_cast<float>(hole.boundaryVertices.size());
        
        // Estimate area using polygon area formula
        float area = 0.0f;
        size_t n = hole.boundaryVertices.size();
        for (size_t i = 0; i < n; ++i) {
            const glm::vec3& p0 = vertices[hole.boundaryVertices[i]];
            const glm::vec3& p1 = vertices[hole.boundaryVertices[(i + 1) % n]];
            area += glm::length(glm::cross(p0 - hole.centroid, p1 - hole.centroid));
        }
        hole.estimatedArea = area * 0.5f;
    }
    
    return hole;
}

} // namespace geometry
} // namespace dc3d
