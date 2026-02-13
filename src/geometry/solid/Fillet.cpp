/**
 * @file Fillet.cpp
 * @brief Implementation of fillet operations for solid bodies
 */

#include "Fillet.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <queue>

#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>

namespace {
// LOW FIX: Named constants for magic numbers
constexpr float TOLERANCE_DEFAULT = 0.001f;
constexpr float TOLERANCE_ZERO_RADIUS = 1e-6f;
}

namespace dc3d {
namespace geometry {

// ===================
// FilletSurface Implementation
// ===================

std::vector<SolidFace> FilletSurface::generateFaces(std::vector<SolidVertex>& vertices,
                                                    int segments) const {
    std::vector<SolidFace> faces;
    
    if (spinePoints.size() < 2) return faces;
    
    size_t numSpinePoints = spinePoints.size();
    uint32_t baseVertexIdx = static_cast<uint32_t>(vertices.size());
    
    // Generate surface grid
    for (size_t i = 0; i < numSpinePoints; ++i) {
        float radius = (i < radii.size()) ? radii[i] : radii.back();
        
        // MEDIUM FIX: Handle zero or near-zero radius
        if (radius < TOLERANCE_ZERO_RADIUS) {
            // Degenerate case - just add single vertex at spine
            for (int j = 0; j <= segments; ++j) {
                SolidVertex v;
                v.position = spinePoints[i];
                v.normal = glm::vec3(0, 1, 0);  // Default up normal
                vertices.push_back(v);
            }
            continue;
        }
        
        // Get local coordinate system at this spine point
        glm::vec3 spine = spinePoints[i];
        glm::vec3 tangent = (i < numSpinePoints - 1) 
            ? glm::normalize(spinePoints[i + 1] - spine)
            : glm::normalize(spine - spinePoints[i - 1]);
        
        // MEDIUM FIX: Properly compute refDir from control points or derive from tangent
        glm::vec3 refDir;
        bool hasValidRefDir = false;
        
        if (controlPoints.size() > i * 2) {
            glm::vec3 candidateRefDir = controlPoints[i * 2] - spine;
            float refLen = glm::length(candidateRefDir);
            if (refLen > TOLERANCE_ZERO_RADIUS) {
                refDir = candidateRefDir / refLen;
                hasValidRefDir = true;
            }
        }
        
        // Fallback: compute refDir perpendicular to tangent
        if (!hasValidRefDir) {
            // Find a vector not parallel to tangent
            glm::vec3 up(0, 1, 0);
            if (std::abs(glm::dot(tangent, up)) > 0.99f) {
                up = glm::vec3(1, 0, 0);
            }
            refDir = glm::normalize(glm::cross(tangent, up));
        }
        
        // Second reference direction perpendicular to both tangent and refDir
        glm::vec3 biNormal = glm::normalize(glm::cross(tangent, refDir));
        
        // Generate arc points
        for (int j = 0; j <= segments; ++j) {
            float angle = glm::half_pi<float>() * j / segments;  // 0 to 90 degrees
            
            SolidVertex v;
            // Generate quarter-circle arc in the plane defined by refDir and biNormal
            v.position = spine + radius * (std::cos(angle) * refDir + std::sin(angle) * biNormal);
            
            // MEDIUM FIX: Compute proper normal - direction from spine to surface point
            glm::vec3 toSurface = v.position - spine;
            float dist = glm::length(toSurface);
            if (dist > TOLERANCE_ZERO_RADIUS) {
                v.normal = toSurface / dist;
            } else {
                v.normal = refDir;  // Fallback
            }
            vertices.push_back(v);
        }
    }
    
    // Generate quad faces
    int vertsPerRing = segments + 1;
    for (size_t i = 0; i < numSpinePoints - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            uint32_t v00 = baseVertexIdx + static_cast<uint32_t>(i * vertsPerRing + j);
            uint32_t v01 = v00 + 1;
            uint32_t v10 = v00 + vertsPerRing;
            uint32_t v11 = v10 + 1;
            
            // Two triangles per quad
            SolidFace f1;
            f1.vertices = {v00, v10, v11};
            faces.push_back(f1);
            
            SolidFace f2;
            f2.vertices = {v00, v11, v01};
            faces.push_back(f2);
        }
    }
    
    return faces;
}

// ===================
// Fillet Implementation
// ===================

FilletResult Fillet::filletEdges(const Solid& solid,
                                 const std::vector<uint32_t>& edgeIndices,
                                 const FilletOptions& options) {
    auto startTime = std::chrono::high_resolution_clock::now();
    FilletResult result;
    
    if (edgeIndices.empty()) {
        result.success = true;
        result.solid = solid.clone();
        return result;
    }
    
    // Validate edges
    for (uint32_t edgeIdx : edgeIndices) {
        if (edgeIdx >= solid.edgeCount()) {
            result.error = "Invalid edge index: " + std::to_string(edgeIdx);
            return result;
        }
        
        if (!isValidFilletRadius(solid, edgeIdx, options.radius)) {
            result.error = "Radius " + std::to_string(options.radius) + 
                          " too large for edge " + std::to_string(edgeIdx);
            return result;
        }
    }
    
    // Collect edges including tangent propagation
    std::vector<uint32_t> allEdges = edgeIndices;
    if (options.tangentPropagation) {
        std::unordered_set<uint32_t> edgeSet(edgeIndices.begin(), edgeIndices.end());
        
        for (uint32_t edgeIdx : edgeIndices) {
            auto tangentEdges = findTangentChain(solid, edgeIdx, options.tangentAngleThreshold);
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
    
    // Compute fillet surfaces for all edges
    std::unordered_map<uint32_t, FilletSurface> filletSurfaces;
    for (size_t i = 0; i < allEdges.size(); ++i) {
        uint32_t edgeIdx = allEdges[i];
        filletSurfaces[edgeIdx] = computeFilletSurface(solid, edgeIdx, options);
        
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
    
    // Mark original face count
    size_t originalFaceCount = faces.size();
    
    // Generate fillet faces
    for (const auto& [edgeIdx, filletSurf] : filletSurfaces) {
        auto newFaces = filletSurf.generateFaces(vertices, options.segments);
        
        uint32_t firstNewFace = static_cast<uint32_t>(faces.size());
        for (auto& face : newFaces) {
            faces.push_back(face);
            result.filletFaces.push_back(static_cast<uint32_t>(faces.size() - 1));
        }
        
        result.stats.filletFacesCreated += static_cast<int>(newFaces.size());
    }
    
    if (options.progress && !options.progress(0.7f)) {
        result.error = "Cancelled";
        return result;
    }
    
    // Handle corners where multiple filleted edges meet
    if (options.handleCorners) {
        std::unordered_set<uint32_t> processedVertices;
        
        for (uint32_t edgeIdx : allEdges) {
            const auto& edge = solid.edge(edgeIdx);
            
            for (uint32_t vertIdx : {edge.vertex0, edge.vertex1}) {
                if (processedVertices.count(vertIdx)) continue;
                
                // Count how many filleted edges meet at this vertex
                std::vector<uint32_t> meetingEdges;
                for (uint32_t adjEdge : solid.vertex(vertIdx).edges) {
                    if (filletSurfaces.count(adjEdge)) {
                        meetingEdges.push_back(adjEdge);
                    }
                }
                
                if (meetingEdges.size() >= 2) {
                    // Create corner blend
                    auto cornerFaces = computeCornerBlend(
                        solid, vertIdx, meetingEdges, filletSurfaces, vertices, options);
                    
                    for (auto& face : cornerFaces) {
                        faces.push_back(face);
                        result.filletFaces.push_back(static_cast<uint32_t>(faces.size() - 1));
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
    
    // Trim original faces (simplified - would need proper implementation)
    if (options.trimFaces) {
        // In a real implementation, we would:
        // 1. Compute intersection curves between original faces and fillet surfaces
        // 2. Trim original faces at these curves
        // 3. Stitch fillet boundaries to trimmed face boundaries
        
        // For now, mark faces adjacent to filleted edges as modified
        std::unordered_set<uint32_t> modifiedFaceSet;
        for (uint32_t edgeIdx : allEdges) {
            for (uint32_t faceIdx : solid.edge(edgeIdx).faces) {
                modifiedFaceSet.insert(faceIdx);
            }
        }
        result.modifiedFaces = std::vector<uint32_t>(modifiedFaceSet.begin(), modifiedFaceSet.end());
    }
    
    // Rebuild topology
    resultSolid.rebuildTopology();
    
    result.success = true;
    result.solid = std::move(resultSolid);
    result.stats.edgesProcessed = static_cast<int>(allEdges.size());
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.stats.computeTimeMs = std::chrono::duration<float, std::milli>(
        endTime - startTime).count();
    
    if (options.progress) options.progress(1.0f);
    
    return result;
}

FilletResult Fillet::filletEdgesWithRadii(
    const Solid& solid,
    const std::unordered_map<uint32_t, float>& edgeRadii,
    const FilletOptions& options) {
    
    FilletResult result;
    
    if (edgeRadii.empty()) {
        result.success = true;
        result.solid = solid.clone();
        return result;
    }
    
    // Process each edge with its specific radius
    Solid current = solid.clone();
    
    for (const auto& [edgeIdx, radius] : edgeRadii) {
        FilletOptions edgeOptions = options;
        edgeOptions.radius = radius;
        edgeOptions.tangentPropagation = false;  // Don't propagate when individual radii
        
        auto edgeResult = filletEdges(current, {edgeIdx}, edgeOptions);
        if (!edgeResult.ok()) {
            return edgeResult;
        }
        
        current = std::move(*edgeResult.solid);
        result.stats.edgesProcessed++;
        result.stats.filletFacesCreated += edgeResult.stats.filletFacesCreated;
    }
    
    result.success = true;
    result.solid = std::move(current);
    return result;
}

FilletResult Fillet::filletEdgeVariable(
    const Solid& solid,
    uint32_t edgeIndex,
    const std::vector<FilletRadiusPoint>& radiusPoints,
    const FilletOptions& options) {
    
    FilletResult result;
    
    if (edgeIndex >= solid.edgeCount()) {
        result.error = "Invalid edge index";
        return result;
    }
    
    if (radiusPoints.size() < 2) {
        result.error = "Variable radius fillet requires at least 2 control points";
        return result;
    }
    
    // Compute variable radius fillet surface
    FilletSurface filletSurf = computeVariableFilletSurface(solid, edgeIndex, radiusPoints, options);
    
    // Build result
    Solid resultSolid = solid.clone();
    auto newFaces = filletSurf.generateFaces(resultSolid.vertices(), options.segments);
    
    for (auto& face : newFaces) {
        resultSolid.faces().push_back(face);
        result.filletFaces.push_back(static_cast<uint32_t>(resultSolid.faceCount() - 1));
    }
    
    resultSolid.rebuildTopology();
    
    result.success = true;
    result.solid = std::move(resultSolid);
    result.stats.edgesProcessed = 1;
    result.stats.filletFacesCreated = static_cast<int>(newFaces.size());
    
    return result;
}

FilletResult Fillet::filletFaces(const Solid& solid,
                                 uint32_t face0Index, uint32_t face1Index,
                                 const FilletOptions& options) {
    FilletResult result;
    
    if (face0Index >= solid.faceCount() || face1Index >= solid.faceCount()) {
        result.error = "Invalid face index";
        return result;
    }
    
    // Find shared edge between faces
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
    
    return filletEdges(solid, {*sharedEdge}, options);
}

FilletResult Fillet::filletFaceEdges(const Solid& solid,
                                     uint32_t faceIndex,
                                     const FilletOptions& options) {
    FilletResult result;
    
    if (faceIndex >= solid.faceCount()) {
        result.error = "Invalid face index";
        return result;
    }
    
    return filletEdges(solid, solid.face(faceIndex).edges, options);
}

std::vector<uint32_t> Fillet::findFilletableEdges(const Solid& solid, float radius) {
    std::vector<uint32_t> result;
    
    for (size_t i = 0; i < solid.edgeCount(); ++i) {
        const auto& edge = solid.edge(i);
        
        // Skip boundary edges
        if (edge.isBoundary) continue;
        
        // Check if radius is valid
        if (isValidFilletRadius(solid, static_cast<uint32_t>(i), radius)) {
            result.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return result;
}

float Fillet::maxFilletRadius(const Solid& solid, uint32_t edgeIndex) {
    if (edgeIndex >= solid.edgeCount()) return 0.0f;
    
    const auto& edge = solid.edge(edgeIndex);
    
    // Maximum radius is limited by:
    // 1. Edge length
    // 2. Adjacent face sizes
    // 3. Dihedral angle
    
    float maxByLength = edge.length * 0.5f;
    
    // Limit by dihedral angle (smaller angles allow smaller radii)
    float halfAngle = edge.dihedralAngle * 0.5f;
    if (halfAngle < 0.01f) return maxByLength;  // Nearly flat, no practical limit
    
    float maxByAngle = maxByLength / std::tan(halfAngle);
    
    return std::min(maxByLength, maxByAngle);
}

std::vector<uint32_t> Fillet::findTangentChain(const Solid& solid,
                                               uint32_t startEdge,
                                               float angleThreshold) {
    return solid.findTangentEdges(startEdge, angleThreshold);
}

MeshData Fillet::generatePreview(const Solid& solid,
                                const std::vector<uint32_t>& edgeIndices,
                                float radius, int segments) {
    MeshData preview;
    
    FilletOptions options;
    options.radius = radius;
    options.segments = segments;
    options.tangentPropagation = false;
    
    std::vector<SolidVertex> vertices;
    
    for (uint32_t edgeIdx : edgeIndices) {
        if (edgeIdx >= solid.edgeCount()) continue;
        
        FilletSurface filletSurf = computeFilletSurface(solid, edgeIdx, options);
        auto faces = filletSurf.generateFaces(vertices, segments);
        
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

glm::vec3 Fillet::computeRollingBallCenter(const glm::vec3& edgePoint,
                                           const glm::vec3& normal0,
                                           const glm::vec3& normal1,
                                           float radius) {
    // Bisector direction
    glm::vec3 bisectorSum = normal0 + normal1;
    float bisectorLen = glm::length(bisectorSum);
    
    // HIGH FIX: Handle degenerate case where normals are opposite (180° apart)
    if (bisectorLen < 0.001f) {
        // Normals are nearly opposite - use edge point directly
        // This is a degenerate case (nearly flat or 180° angle)
        return edgePoint;
    }
    
    glm::vec3 bisector = bisectorSum / bisectorLen;
    
    // Half angle between normals
    float cosHalfAngle = glm::dot(normal0, bisector);
    
    // HIGH FIX: Handle case where cosHalfAngle is negative or too small
    // This can happen when normals point away from each other (> 90° apart)
    if (std::abs(cosHalfAngle) < 0.001f) {
        // Degenerate case: nearly parallel faces (close to 180°)
        return edgePoint;
    }
    
    // Use absolute value to handle both convex and concave edges properly
    float distance = radius / std::abs(cosHalfAngle);
    
    return edgePoint + bisector * distance;
}

FilletSurface Fillet::computeFilletSurface(const Solid& solid,
                                           uint32_t edgeIndex,
                                           const FilletOptions& options) {
    FilletSurface result;
    result.edgeIndex = edgeIndex;
    
    const auto& edge = solid.edge(edgeIndex);
    
    if (edge.faces.size() < 2) {
        return result;  // Boundary edge, no fillet
    }
    
    result.face0Index = edge.faces[0];
    result.face1Index = edge.faces[1];
    
    const glm::vec3& normal0 = solid.face(result.face0Index).normal;
    const glm::vec3& normal1 = solid.face(result.face1Index).normal;
    
    const glm::vec3& p0 = solid.vertex(edge.vertex0).position;
    const glm::vec3& p1 = solid.vertex(edge.vertex1).position;
    
    // Sample points along the edge
    int numSamples = std::max(2, static_cast<int>(edge.length / (options.radius * 0.5f)));
    numSamples = std::min(numSamples, 50);  // Limit
    
    for (int i = 0; i <= numSamples; ++i) {
        float t = float(i) / numSamples;
        glm::vec3 edgePoint = glm::mix(p0, p1, t);
        
        // Compute rolling ball center
        glm::vec3 center = computeRollingBallCenter(edgePoint, normal0, normal1, options.radius);
        result.spinePoints.push_back(center);
        result.radii.push_back(options.radius);
        
        // Store control points (contact points on faces)
        glm::vec3 contact0 = edgePoint + normal0 * options.radius;
        glm::vec3 contact1 = edgePoint + normal1 * options.radius;
        result.controlPoints.push_back(contact0);
        result.controlPoints.push_back(contact1);
    }
    
    return result;
}

FilletSurface Fillet::computeVariableFilletSurface(
    const Solid& solid,
    uint32_t edgeIndex,
    const std::vector<FilletRadiusPoint>& radiusPoints,
    const FilletOptions& options) {
    
    FilletSurface result;
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
    
    // Sample with variable radius
    int numSamples = 20;
    
    for (int i = 0; i <= numSamples; ++i) {
        float t = float(i) / numSamples;
        glm::vec3 edgePoint = glm::mix(p0, p1, t);
        
        // Interpolate radius
        float radius = interpolateRadius(t, radiusPoints);
        
        glm::vec3 center = computeRollingBallCenter(edgePoint, normal0, normal1, radius);
        result.spinePoints.push_back(center);
        result.radii.push_back(radius);
    }
    
    return result;
}

std::vector<glm::vec3> Fillet::generateFilletProfile(
    const glm::vec3& center,
    const glm::vec3& start,
    const glm::vec3& end,
    float radius,
    int segments,
    FilletProfile profile,
    float rho) {
    
    std::vector<glm::vec3> points;
    
    glm::vec3 startDir = glm::normalize(start - center);
    glm::vec3 endDir = glm::normalize(end - center);
    
    for (int i = 0; i <= segments; ++i) {
        float t = float(i) / segments;
        glm::vec3 point;
        
        switch (profile) {
            case FilletProfile::Circular: {
                // Spherical interpolation
                float angle = std::acos(glm::clamp(glm::dot(startDir, endDir), -1.0f, 1.0f));
                float theta = t * angle;
                
                glm::vec3 axis = glm::cross(startDir, endDir);
                if (glm::length2(axis) < 1e-10f) {
                    axis = glm::vec3(0, 1, 0);
                }
                axis = glm::normalize(axis);
                
                // Rodrigues' rotation formula
                glm::vec3 rotated = startDir * std::cos(theta) +
                                   glm::cross(axis, startDir) * std::sin(theta) +
                                   axis * glm::dot(axis, startDir) * (1 - std::cos(theta));
                
                point = center + rotated * radius;
                break;
            }
            
            case FilletProfile::Conic: {
                // Rational Bezier curve with conic parameter
                float w1 = rho;  // Weight of middle control point
                glm::vec3 p0 = start;
                glm::vec3 p2 = end;
                glm::vec3 p1 = center + (startDir + endDir) * 0.5f * radius / rho;
                
                // Rational quadratic Bezier
                float u = t;
                float denom = (1-u)*(1-u) + 2*w1*u*(1-u) + u*u;
                point = ((1-u)*(1-u)*p0 + 2*w1*u*(1-u)*p1 + u*u*p2) / denom;
                break;
            }
            
            case FilletProfile::Chamfer: {
                // Linear interpolation (not actually a fillet)
                point = glm::mix(start, end, t);
                break;
            }
            
            default:
                point = glm::mix(start, end, t);
                break;
        }
        
        points.push_back(point);
    }
    
    return points;
}

SolidFace Fillet::trimFaceByFillet(const SolidFace& face,
                                   const std::vector<SolidVertex>& vertices,
                                   const FilletSurface& fillet,
                                   std::vector<SolidVertex>& newVertices) {
    // Simplified - actual implementation would compute intersection and trim
    SolidFace trimmed = face;
    return trimmed;
}

std::vector<SolidFace> Fillet::computeCornerBlend(
    const Solid& solid,
    uint32_t vertexIndex,
    const std::vector<uint32_t>& filletEdges,
    const std::unordered_map<uint32_t, FilletSurface>& filletSurfaces,
    std::vector<SolidVertex>& vertices,
    const FilletOptions& options) {
    
    std::vector<SolidFace> cornerFaces;
    
    if (filletEdges.size() < 2) return cornerFaces;
    
    const glm::vec3& cornerPos = solid.vertex(vertexIndex).position;
    
    // Simple corner blend: create a small spherical patch
    // Real implementation would properly blend the fillet surfaces
    
    // Find average radius
    float avgRadius = 0.0f;
    for (uint32_t edgeIdx : filletEdges) {
        auto it = filletSurfaces.find(edgeIdx);
        if (it != filletSurfaces.end() && !it->second.radii.empty()) {
            avgRadius += it->second.radii[0];
        } else {
            avgRadius += options.radius;
        }
    }
    avgRadius /= filletEdges.size();
    
    // Create corner patch vertices
    uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
    int rings = 3;
    int segments = 6;
    
    // Center vertex
    SolidVertex centerVert;
    centerVert.position = cornerPos + glm::normalize(solid.vertex(vertexIndex).normal) * avgRadius;
    centerVert.normal = solid.vertex(vertexIndex).normal;
    vertices.push_back(centerVert);
    
    // Ring vertices
    for (int r = 1; r <= rings; ++r) {
        float phi = glm::half_pi<float>() * r / rings;
        
        for (int s = 0; s < segments; ++s) {
            float theta = glm::two_pi<float>() * s / segments;
            
            SolidVertex v;
            v.normal = glm::vec3(
                std::sin(phi) * std::cos(theta),
                std::sin(phi) * std::sin(theta),
                std::cos(phi)
            );
            v.position = cornerPos + v.normal * avgRadius;
            vertices.push_back(v);
        }
    }
    
    // Create triangles from center to first ring
    for (int s = 0; s < segments; ++s) {
        int next = (s + 1) % segments;
        
        SolidFace face;
        face.vertices = {
            baseIdx,
            baseIdx + 1 + static_cast<uint32_t>(s),
            baseIdx + 1 + static_cast<uint32_t>(next)
        };
        cornerFaces.push_back(face);
    }
    
    // Create quads between rings
    for (int r = 0; r < rings - 1; ++r) {
        uint32_t ringStart = baseIdx + 1 + r * segments;
        uint32_t nextRingStart = ringStart + segments;
        
        for (int s = 0; s < segments; ++s) {
            int next = (s + 1) % segments;
            
            // Two triangles per quad
            SolidFace f1;
            f1.vertices = {
                ringStart + s,
                nextRingStart + s,
                ringStart + next
            };
            cornerFaces.push_back(f1);
            
            SolidFace f2;
            f2.vertices = {
                ringStart + next,
                nextRingStart + s,
                nextRingStart + next
            };
            cornerFaces.push_back(f2);
        }
    }
    
    return cornerFaces;
}

bool Fillet::isValidFilletRadius(const Solid& solid,
                                 uint32_t edgeIndex,
                                 float radius) {
    float maxR = maxFilletRadius(solid, edgeIndex);
    return radius > 0 && radius <= maxR;
}

float Fillet::interpolateRadius(float t, const std::vector<FilletRadiusPoint>& points) {
    if (points.empty()) return 1.0f;
    if (points.size() == 1) return points[0].radius;
    
    // Find surrounding control points
    size_t i = 0;
    while (i < points.size() - 1 && points[i + 1].parameter < t) {
        ++i;
    }
    
    if (i >= points.size() - 1) {
        return points.back().radius;
    }
    
    // Linear interpolation between control points
    float t0 = points[i].parameter;
    float t1 = points[i + 1].parameter;
    float r0 = points[i].radius;
    float r1 = points[i + 1].radius;
    
    if (std::abs(t1 - t0) < 1e-6f) return r0;
    
    float localT = (t - t0) / (t1 - t0);
    return glm::mix(r0, r1, localT);
}

// ===================
// RollingBallFillet Implementation
// ===================

RollingBallFillet::RollingBallFillet(const Solid& solid, uint32_t edgeIndex, float radius)
    : solid_(solid), edgeIndex_(edgeIndex), radius_(radius) {
    computeGeometry();
}

void RollingBallFillet::computeGeometry() {
    if (edgeIndex_ >= solid_.edgeCount()) {
        isValid_ = false;
        return;
    }
    
    const auto& edge = solid_.edge(edgeIndex_);
    
    if (edge.faces.size() < 2) {
        isValid_ = false;
        return;
    }
    
    // Get edge geometry
    edgeStart_ = solid_.vertex(edge.vertex0).position;
    edgeEnd_ = solid_.vertex(edge.vertex1).position;
    edgeDir_ = edgeEnd_ - edgeStart_;
    edgeLength_ = glm::length(edgeDir_);
    
    if (edgeLength_ < 1e-10f) {
        isValid_ = false;
        return;
    }
    edgeDir_ /= edgeLength_;
    
    // Get face normals
    normal0_ = solid_.face(edge.faces[0]).normal;
    normal1_ = solid_.face(edge.faces[1]).normal;
    
    // Compute bisector and half angle
    glm::vec3 bisector = normal0_ + normal1_;
    float bisectorLen = glm::length(bisector);
    
    if (bisectorLen < 1e-6f) {
        // Normals are opposite (180° angle)
        halfAngle_ = glm::half_pi<float>();
        biNormal_ = glm::normalize(glm::cross(edgeDir_, normal0_));
    } else {
        bisector /= bisectorLen;
        halfAngle_ = std::acos(glm::clamp(glm::dot(normal0_, bisector), -1.0f, 1.0f));
        biNormal_ = bisector;
    }
    
    // Compute max radius
    maxRadius_ = Fillet::maxFilletRadius(solid_, edgeIndex_);
    
    isValid_ = radius_ <= maxRadius_ && radius_ > 0;
}

glm::vec3 RollingBallFillet::ballCenter(float t) const {
    t = glm::clamp(t, 0.0f, 1.0f);
    glm::vec3 edgePoint = glm::mix(edgeStart_, edgeEnd_, t);
    
    float distance = radius_ / std::max(0.001f, std::cos(halfAngle_));
    return edgePoint + biNormal_ * distance;
}

glm::vec3 RollingBallFillet::contactPoint0(float t) const {
    glm::vec3 center = ballCenter(t);
    return center - normal0_ * radius_;
}

glm::vec3 RollingBallFillet::contactPoint1(float t) const {
    glm::vec3 center = ballCenter(t);
    return center - normal1_ * radius_;
}

std::vector<std::vector<glm::vec3>> RollingBallFillet::generateSurface(
    int uSamples, int vSamples) const {
    
    std::vector<std::vector<glm::vec3>> surface(uSamples + 1);
    
    for (int i = 0; i <= uSamples; ++i) {
        float u = float(i) / uSamples;
        surface[i].resize(vSamples + 1);
        
        glm::vec3 center = ballCenter(u);
        glm::vec3 p0 = contactPoint0(u);
        glm::vec3 p1 = contactPoint1(u);
        
        // Generate arc from p0 to p1 around center
        glm::vec3 dir0 = glm::normalize(p0 - center);
        glm::vec3 dir1 = glm::normalize(p1 - center);
        
        for (int j = 0; j <= vSamples; ++j) {
            float v = float(j) / vSamples;
            
            // Spherical interpolation
            float angle = std::acos(glm::clamp(glm::dot(dir0, dir1), -1.0f, 1.0f));
            float theta = v * angle;
            
            glm::vec3 axis = glm::cross(dir0, dir1);
            if (glm::length2(axis) < 1e-10f) {
                axis = edgeDir_;
            }
            axis = glm::normalize(axis);
            
            // Rodrigues' rotation
            glm::vec3 rotated = dir0 * std::cos(theta) +
                               glm::cross(axis, dir0) * std::sin(theta) +
                               axis * glm::dot(axis, dir0) * (1 - std::cos(theta));
            
            surface[i][j] = center + rotated * radius_;
        }
    }
    
    return surface;
}

} // namespace geometry
} // namespace dc3d
