/**
 * @file Chamfer.cpp
 * @brief Implementation of chamfer operations for solid bodies
 */

#include "Chamfer.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <unordered_set>

#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>

namespace dc3d {
namespace geometry {

// ===================
// ChamferSurface Implementation
// ===================

std::vector<SolidFace> ChamferSurface::generateFaces(std::vector<SolidVertex>& vertices) const {
    std::vector<SolidFace> faces;
    
    if (boundary0.size() < 2 || boundary1.size() < 2) return faces;
    if (boundary0.size() != boundary1.size()) return faces;
    
    uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
    
    // Add boundary vertices
    for (size_t i = 0; i < boundary0.size(); ++i) {
        SolidVertex v0;
        v0.position = boundary0[i];
        vertices.push_back(v0);
        
        SolidVertex v1;
        v1.position = boundary1[i];
        vertices.push_back(v1);
    }
    
    // Compute chamfer normal and assign to vertices
    if (boundary0.size() >= 2 && boundary1.size() >= 2) {
        glm::vec3 edge01 = boundary0[1] - boundary0[0];
        glm::vec3 edge02 = boundary1[0] - boundary0[0];
        glm::vec3 chamferNormal = glm::normalize(glm::cross(edge01, edge02));
        
        for (size_t i = 0; i < boundary0.size() * 2; ++i) {
            vertices[baseIdx + i].normal = chamferNormal;
        }
    }
    
    // Create quad faces between boundaries
    for (size_t i = 0; i < boundary0.size() - 1; ++i) {
        uint32_t v00 = baseIdx + static_cast<uint32_t>(i * 2);      // boundary0[i]
        uint32_t v01 = baseIdx + static_cast<uint32_t>(i * 2 + 1);  // boundary1[i]
        uint32_t v10 = baseIdx + static_cast<uint32_t>((i + 1) * 2);     // boundary0[i+1]
        uint32_t v11 = baseIdx + static_cast<uint32_t>((i + 1) * 2 + 1); // boundary1[i+1]
        
        // Two triangles per quad
        SolidFace f1;
        f1.vertices = {v00, v10, v01};
        faces.push_back(f1);
        
        SolidFace f2;
        f2.vertices = {v01, v10, v11};
        faces.push_back(f2);
    }
    
    return faces;
}

// ===================
// Chamfer Implementation
// ===================

ChamferResult Chamfer::chamferEdges(const Solid& solid,
                                    const std::vector<uint32_t>& edgeIndices,
                                    float distance,
                                    const ChamferOptions& options) {
    ChamferOptions opts = options;
    opts.type = ChamferType::Symmetric;
    opts.distance = distance;
    opts.distance2 = distance;
    
    return chamferEdgesAsymmetric(solid, edgeIndices, distance, distance, opts);
}

ChamferResult Chamfer::chamferEdgesAsymmetric(const Solid& solid,
                                              const std::vector<uint32_t>& edgeIndices,
                                              float distance1, float distance2,
                                              const ChamferOptions& options) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ChamferResult result;
    
    if (edgeIndices.empty()) {
        result.success = true;
        result.solid = std::make_optional(solid.clone());
        return result;
    }
    
    // Validate edges
    for (uint32_t edgeIdx : edgeIndices) {
        if (edgeIdx >= solid.edgeCount()) {
            result.error = "Invalid edge index: " + std::to_string(edgeIdx);
            return result;
        }
        
        if (!isValidChamferDistance(solid, edgeIdx, distance1, distance2)) {
            result.error = "Chamfer distance too large for edge " + std::to_string(edgeIdx);
            return result;
        }
    }
    
    // Collect edges including tangent propagation
    std::vector<uint32_t> allEdges = edgeIndices;
    if (options.tangentPropagation) {
        std::unordered_set<uint32_t> edgeSet(edgeIndices.begin(), edgeIndices.end());
        
        for (uint32_t edgeIdx : edgeIndices) {
            auto tangentEdges = solid.findTangentEdges(edgeIdx, options.tangentAngleThreshold);
            for (uint32_t tangent : tangentEdges) {
                if (edgeSet.find(tangent) == edgeSet.end()) {
                    edgeSet.insert(tangent);
                    allEdges.push_back(tangent);
                }
            }
        }
    }
    
    if (options.progress && !options.progress(0.1f)) {
        result.error = "Cancelled";
        return result;
    }
    
    // Compute chamfer surfaces
    std::unordered_map<uint32_t, ChamferSurface> chamferSurfaces;
    for (size_t i = 0; i < allEdges.size(); ++i) {
        uint32_t edgeIdx = allEdges[i];
        chamferSurfaces[edgeIdx] = computeChamferSurface(solid, edgeIdx, distance1, distance2, options);
        
        if (options.progress) {
            float progress = 0.1f + 0.4f * (float(i) / allEdges.size());
            if (!options.progress(progress)) {
                result.error = "Cancelled";
                return result;
            }
        }
    }
    
    // Build result solid
    Solid resultSolid = solid.clone();
    std::vector<SolidVertex>& vertices = resultSolid.vertices();
    std::vector<SolidFace>& faces = resultSolid.faces();
    
    // Generate chamfer faces
    for (const auto& [edgeIdx, chamferSurf] : chamferSurfaces) {
        auto newFaces = chamferSurf.generateFaces(vertices);
        
        for (auto& face : newFaces) {
            faces.push_back(face);
            result.chamferFaces.push_back(static_cast<uint32_t>(faces.size() - 1));
        }
        
        result.stats.chamferFacesCreated += static_cast<int>(newFaces.size());
    }
    
    if (options.progress && !options.progress(0.7f)) {
        result.error = "Cancelled";
        return result;
    }
    
    // Handle corners
    if (options.handleCorners) {
        std::unordered_set<uint32_t> processedVertices;
        
        for (uint32_t edgeIdx : allEdges) {
            const auto& edge = solid.edge(edgeIdx);
            
            for (uint32_t vertIdx : {edge.vertex0, edge.vertex1}) {
                if (processedVertices.count(vertIdx)) continue;
                
                std::vector<uint32_t> meetingEdges;
                for (uint32_t adjEdge : solid.vertex(vertIdx).edges) {
                    if (chamferSurfaces.count(adjEdge)) {
                        meetingEdges.push_back(adjEdge);
                    }
                }
                
                if (meetingEdges.size() >= 2) {
                    auto cornerFaces = computeCornerChamfer(
                        solid, vertIdx, meetingEdges, chamferSurfaces, vertices, options);
                    
                    for (auto& face : cornerFaces) {
                        faces.push_back(face);
                        result.chamferFaces.push_back(static_cast<uint32_t>(faces.size() - 1));
                    }
                    
                    ++result.stats.cornersProcessed;
                }
                
                processedVertices.insert(vertIdx);
            }
        }
    }
    
    if (options.progress && !options.progress(0.9f)) {
        result.error = "Cancelled";
        return result;
    }
    
    // Mark modified faces
    std::unordered_set<uint32_t> modifiedFaceSet;
    for (uint32_t edgeIdx : allEdges) {
        for (uint32_t faceIdx : solid.edge(edgeIdx).faces) {
            modifiedFaceSet.insert(faceIdx);
        }
    }
    result.modifiedFaces = std::vector<uint32_t>(modifiedFaceSet.begin(), modifiedFaceSet.end());
    
    // Rebuild topology
    resultSolid.rebuildTopology();
    
    result.success = true;
    result.solid = std::make_optional(std::move(resultSolid));
    result.stats.edgesProcessed = static_cast<int>(allEdges.size());
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.stats.computeTimeMs = std::chrono::duration<float, std::milli>(
        endTime - startTime).count();
    
    if (options.progress) options.progress(1.0f);
    
    return result;
}

ChamferResult Chamfer::chamferEdgesAngle(const Solid& solid,
                                         const std::vector<uint32_t>& edgeIndices,
                                         float distance, float angle,
                                         const ChamferOptions& options) {
    ChamferResult result;
    
    if (edgeIndices.empty()) {
        result.success = true;
        result.solid = std::make_optional(solid.clone());
        return result;
    }
    
    // Convert angle to distances for each edge
    std::unordered_map<uint32_t, std::pair<float, float>> edgeParams;
    
    for (uint32_t edgeIdx : edgeIndices) {
        if (edgeIdx >= solid.edgeCount()) {
            result.error = "Invalid edge index: " + std::to_string(edgeIdx);
            return result;
        }
        
        float dihedralAngle = solid.edge(edgeIdx).dihedralAngle;
        auto [d1, d2] = angleToDistances(distance, angle, dihedralAngle);
        edgeParams[edgeIdx] = {d1, d2};
    }
    
    return chamferEdgesWithParams(solid, edgeParams, options);
}

ChamferResult Chamfer::chamferEdgesWithParams(
    const Solid& solid,
    const std::unordered_map<uint32_t, std::pair<float, float>>& edgeParams,
    const ChamferOptions& options) {
    
    ChamferResult result;
    
    if (edgeParams.empty()) {
        result.success = true;
        result.solid = std::make_optional(solid.clone());
        return result;
    }
    
    Solid current = solid.clone();
    
    for (const auto& [edgeIdx, params] : edgeParams) {
        ChamferOptions edgeOptions = options;
        edgeOptions.tangentPropagation = false;
        
        auto edgeResult = chamferEdgesAsymmetric(current, {edgeIdx}, params.first, params.second, edgeOptions);
        if (!edgeResult.ok()) {
            return edgeResult;
        }
        
        current = std::move(*edgeResult.solid);
        result.stats.edgesProcessed++;
        result.stats.chamferFacesCreated += edgeResult.stats.chamferFacesCreated;
    }
    
    result.success = true;
    result.solid = std::make_optional(std::move(current));
    return result;
}

ChamferResult Chamfer::chamferEdgeVariable(
    const Solid& solid,
    uint32_t edgeIndex,
    const std::vector<ChamferPoint>& chamferPoints,
    const ChamferOptions& options) {
    
    ChamferResult result;
    
    if (edgeIndex >= solid.edgeCount()) {
        result.error = "Invalid edge index";
        return result;
    }
    
    if (chamferPoints.size() < 2) {
        result.error = "Variable chamfer requires at least 2 control points";
        return result;
    }
    
    // Compute variable chamfer surface
    ChamferSurface chamferSurf = computeVariableChamferSurface(solid, edgeIndex, chamferPoints, options);
    
    // Build result
    Solid resultSolid = solid.clone();
    auto newFaces = chamferSurf.generateFaces(resultSolid.vertices());
    
    for (auto& face : newFaces) {
        resultSolid.faces().push_back(face);
        result.chamferFaces.push_back(static_cast<uint32_t>(resultSolid.faceCount() - 1));
    }
    
    resultSolid.rebuildTopology();
    
    result.success = true;
    result.solid = std::make_optional(std::move(resultSolid));
    result.stats.edgesProcessed = 1;
    result.stats.chamferFacesCreated = static_cast<int>(newFaces.size());
    
    return result;
}

ChamferResult Chamfer::chamferFaces(const Solid& solid,
                                    uint32_t face0Index, uint32_t face1Index,
                                    const ChamferOptions& options) {
    ChamferResult result;
    
    if (face0Index >= solid.faceCount() || face1Index >= solid.faceCount()) {
        result.error = "Invalid face index";
        return result;
    }
    
    // Find shared edge
    const auto& face0 = solid.face(face0Index);
    const auto& face1 = solid.face(face1Index);
    
    std::optional<uint32_t> sharedEdge;
    for (uint32_t edge0 : face0.edges) {
        for (uint32_t edge1 : face1.edges) {
            if (edge0 == edge1) {
                sharedEdge = edge0;
                break;
            }
        }
        if (sharedEdge) break;
    }
    
    if (!sharedEdge) {
        result.error = "Faces do not share an edge";
        return result;
    }
    
    return chamferEdges(solid, {*sharedEdge}, options.distance, options);
}

ChamferResult Chamfer::chamferFaceEdges(const Solid& solid,
                                        uint32_t faceIndex,
                                        const ChamferOptions& options) {
    ChamferResult result;
    
    if (faceIndex >= solid.faceCount()) {
        result.error = "Invalid face index";
        return result;
    }
    
    return chamferEdges(solid, solid.face(faceIndex).edges, options.distance, options);
}

std::vector<uint32_t> Chamfer::findChamferableEdges(const Solid& solid, float distance) {
    std::vector<uint32_t> result;
    
    for (size_t i = 0; i < solid.edgeCount(); ++i) {
        const auto& edge = solid.edge(i);
        
        if (edge.isBoundary) continue;
        
        if (isValidChamferDistance(solid, static_cast<uint32_t>(i), distance, distance)) {
            result.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return result;
}

float Chamfer::maxChamferDistance(const Solid& solid, uint32_t edgeIndex) {
    if (edgeIndex >= solid.edgeCount()) return 0.0f;
    
    const auto& edge = solid.edge(edgeIndex);
    
    // Max distance is limited by edge length and adjacent face sizes
    float maxByLength = edge.length * 0.5f;
    
    // Also check distances to adjacent edges on each face
    float minAdjacentDist = std::numeric_limits<float>::max();
    
    for (uint32_t faceIdx : edge.faces) {
        const auto& face = solid.face(faceIdx);
        
        for (uint32_t adjEdgeIdx : face.edges) {
            if (adjEdgeIdx == edgeIndex) continue;
            
            const auto& adjEdge = solid.edge(adjEdgeIdx);
            minAdjacentDist = std::min(minAdjacentDist, adjEdge.length * 0.5f);
        }
    }
    
    return std::min(maxByLength, minAdjacentDist);
}

std::pair<float, float> Chamfer::angleToDistances(float distance, float angle,
                                                  float dihedralAngle) {
    // Given distance d on reference face and angle a from that face,
    // compute distance on second face
    
    float d1 = distance;
    
    // Angle between chamfer plane and second face
    float complementAngle = dihedralAngle - angle;
    
    // Using sine rule
    float d2 = d1 * std::sin(angle) / std::sin(complementAngle);
    
    return {d1, std::max(0.001f, d2)};
}

MeshData Chamfer::generatePreview(const Solid& solid,
                                 const std::vector<uint32_t>& edgeIndices,
                                 float distance) {
    return generatePreviewAsymmetric(solid, edgeIndices, distance, distance);
}

MeshData Chamfer::generatePreviewAsymmetric(const Solid& solid,
                                           const std::vector<uint32_t>& edgeIndices,
                                           float distance1, float distance2) {
    MeshData preview;
    ChamferOptions options;
    
    std::vector<SolidVertex> vertices;
    
    for (uint32_t edgeIdx : edgeIndices) {
        if (edgeIdx >= solid.edgeCount()) continue;
        
        ChamferSurface chamferSurf = computeChamferSurface(solid, edgeIdx, distance1, distance2, options);
        auto faces = chamferSurf.generateFaces(vertices);
        
        // Convert to MeshData
        uint32_t baseIdx = static_cast<uint32_t>(preview.vertices().size());
        
        for (const auto& v : vertices) {
            preview.vertices().push_back(v.position);
            preview.normals().push_back(v.normal);
        }
        vertices.clear();
        
        for (const auto& face : faces) {
            for (uint32_t vi : face.vertices) {
                preview.indices().push_back(baseIdx + vi);
            }
        }
    }
    
    // Trigger bounds computation
    preview.boundingBox();
    return preview;
}

// ===================
// Private Methods
// ===================

ChamferSurface Chamfer::computeChamferSurface(const Solid& solid,
                                              uint32_t edgeIndex,
                                              float distance1, float distance2,
                                              const ChamferOptions& options) {
    ChamferSurface result;
    result.edgeIndex = edgeIndex;
    
    const auto& edge = solid.edge(edgeIndex);
    
    if (edge.faces.size() < 2) {
        return result;
    }
    
    result.face0Index = edge.faces[0];
    result.face1Index = edge.faces[1];
    
    const glm::vec3& normal0 = solid.face(result.face0Index).normal;
    const glm::vec3& normal1 = solid.face(result.face1Index).normal;
    
    const glm::vec3& p0 = solid.vertex(edge.vertex0).position;
    const glm::vec3& p1 = solid.vertex(edge.vertex1).position;
    glm::vec3 edgeDir = glm::normalize(p1 - p0);
    
    // Sample points along edge
    int numSamples = std::max(2, static_cast<int>(edge.length / (std::max(distance1, distance2) * 0.5f)));
    numSamples = std::min(numSamples, 50);
    
    for (int i = 0; i <= numSamples; ++i) {
        float t = float(i) / numSamples;
        glm::vec3 edgePoint = glm::mix(p0, p1, t);
        
        // Compute offset points on each face
        glm::vec3 offset0 = computeOffsetPoint(edgePoint, edgeDir, normal0, distance1);
        glm::vec3 offset1 = computeOffsetPoint(edgePoint, edgeDir, normal1, distance2);
        
        result.boundary0.push_back(offset0);
        result.boundary1.push_back(offset1);
    }
    
    return result;
}

ChamferSurface Chamfer::computeVariableChamferSurface(
    const Solid& solid,
    uint32_t edgeIndex,
    const std::vector<ChamferPoint>& chamferPoints,
    const ChamferOptions& options) {
    
    ChamferSurface result;
    result.edgeIndex = edgeIndex;
    
    const auto& edge = solid.edge(edgeIndex);
    
    if (edge.faces.size() < 2) {
        return result;
    }
    
    result.face0Index = edge.faces[0];
    result.face1Index = edge.faces[1];
    
    const glm::vec3& normal0 = solid.face(result.face0Index).normal;
    const glm::vec3& normal1 = solid.face(result.face1Index).normal;
    
    const glm::vec3& p0 = solid.vertex(edge.vertex0).position;
    const glm::vec3& p1 = solid.vertex(edge.vertex1).position;
    glm::vec3 edgeDir = glm::normalize(p1 - p0);
    
    int numSamples = 20;
    
    for (int i = 0; i <= numSamples; ++i) {
        float t = float(i) / numSamples;
        glm::vec3 edgePoint = glm::mix(p0, p1, t);
        
        auto [d1, d2] = interpolateChamfer(t, chamferPoints);
        
        glm::vec3 offset0 = computeOffsetPoint(edgePoint, edgeDir, normal0, d1);
        glm::vec3 offset1 = computeOffsetPoint(edgePoint, edgeDir, normal1, d2);
        
        result.boundary0.push_back(offset0);
        result.boundary1.push_back(offset1);
    }
    
    return result;
}

glm::vec3 Chamfer::computeOffsetPoint(const glm::vec3& edgePoint,
                                      const glm::vec3& edgeDir,
                                      const glm::vec3& faceNormal,
                                      float distance) {
    // Direction away from edge, within the face plane
    glm::vec3 inPlaneDir = glm::normalize(glm::cross(edgeDir, faceNormal));
    
    // Ensure direction points away from edge (into the face)
    // This depends on face orientation, may need adjustment
    
    return edgePoint + inPlaneDir * distance;
}

std::vector<SolidFace> Chamfer::computeCornerChamfer(
    const Solid& solid,
    uint32_t vertexIndex,
    const std::vector<uint32_t>& chamferEdges,
    const std::unordered_map<uint32_t, ChamferSurface>& chamferSurfaces,
    std::vector<SolidVertex>& vertices,
    const ChamferOptions& options) {
    
    std::vector<SolidFace> cornerFaces;
    
    if (chamferEdges.size() < 2) return cornerFaces;
    
    const glm::vec3& cornerPos = solid.vertex(vertexIndex).position;
    
    // Collect chamfer boundary endpoints at this vertex
    std::vector<glm::vec3> boundaryPoints;
    
    for (uint32_t edgeIdx : chamferEdges) {
        const auto& edge = solid.edge(edgeIdx);
        auto it = chamferSurfaces.find(edgeIdx);
        if (it == chamferSurfaces.end()) continue;
        
        const auto& surface = it->second;
        
        // Get the boundary point at this vertex end
        if (edge.vertex0 == vertexIndex) {
            if (!surface.boundary0.empty()) boundaryPoints.push_back(surface.boundary0.front());
            if (!surface.boundary1.empty()) boundaryPoints.push_back(surface.boundary1.front());
        } else {
            if (!surface.boundary0.empty()) boundaryPoints.push_back(surface.boundary0.back());
            if (!surface.boundary1.empty()) boundaryPoints.push_back(surface.boundary1.back());
        }
    }
    
    if (boundaryPoints.size() < 3) return cornerFaces;
    
    // Create triangular corner face connecting boundary points
    // Sort points around the corner center
    glm::vec3 center(0.0f);
    for (const auto& p : boundaryPoints) {
        center += p;
    }
    center /= float(boundaryPoints.size());
    
    // Add corner vertices
    uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
    
    for (const auto& p : boundaryPoints) {
        SolidVertex v;
        v.position = p;
        v.normal = glm::normalize(p - cornerPos);
        vertices.push_back(v);
    }
    
    // Create fan triangulation
    for (size_t i = 1; i < boundaryPoints.size() - 1; ++i) {
        SolidFace face;
        face.vertices = {
            baseIdx,
            baseIdx + static_cast<uint32_t>(i),
            baseIdx + static_cast<uint32_t>(i + 1)
        };
        cornerFaces.push_back(face);
    }
    
    return cornerFaces;
}

bool Chamfer::isValidChamferDistance(const Solid& solid,
                                     uint32_t edgeIndex,
                                     float distance1, float distance2) {
    float maxDist = maxChamferDistance(solid, edgeIndex);
    return distance1 > 0 && distance1 <= maxDist &&
           distance2 > 0 && distance2 <= maxDist;
}

std::pair<float, float> Chamfer::interpolateChamfer(float t,
                                                    const std::vector<ChamferPoint>& points) {
    if (points.empty()) return {1.0f, 1.0f};
    if (points.size() == 1) return {points[0].distance1, points[0].distance2};
    
    // Find surrounding control points
    size_t i = 0;
    while (i < points.size() - 1 && points[i + 1].parameter < t) {
        ++i;
    }
    
    if (i >= points.size() - 1) {
        return {points.back().distance1, points.back().distance2};
    }
    
    float t0 = points[i].parameter;
    float t1 = points[i + 1].parameter;
    
    if (std::abs(t1 - t0) < 1e-6f) {
        return {points[i].distance1, points[i].distance2};
    }
    
    float localT = (t - t0) / (t1 - t0);
    
    float d1 = glm::mix(points[i].distance1, points[i + 1].distance1, localT);
    float d2 = glm::mix(points[i].distance2, points[i + 1].distance2, localT);
    
    return {d1, d2};
}

} // namespace geometry
} // namespace dc3d
