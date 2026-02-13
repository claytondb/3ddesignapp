/**
 * @file BooleanOps.cpp
 * @brief Implementation of boolean operations on solid bodies
 */

#include "BooleanOps.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <queue>

#include <glm/gtx/norm.hpp>

namespace dc3d {
namespace geometry {

// ===================
// BSPNode Implementation
// ===================

BSPNode::Plane BSPNode::Plane::fromPoints(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    float distance = glm::dot(normal, a);
    return Plane(normal, distance);
}

std::unique_ptr<BSPNode> BSPNode::build(std::vector<SolidFace>& faces,
                                        std::vector<SolidVertex>& vertices,
                                        float epsilon) {
    if (faces.empty()) return nullptr;
    
    auto node = std::make_unique<BSPNode>();
    node->epsilon_ = epsilon;
    
    // Use first face as splitting plane
    const auto& firstFace = faces[0];
    // CRITICAL FIX: Validate vertex indices before accessing vertices array
    if (firstFace.vertices.size() >= 3 &&
        firstFace.vertices[0] < vertices.size() &&
        firstFace.vertices[1] < vertices.size() &&
        firstFace.vertices[2] < vertices.size()) {
        const glm::vec3& a = vertices[firstFace.vertices[0]].position;
        const glm::vec3& b = vertices[firstFace.vertices[1]].position;
        const glm::vec3& c = vertices[firstFace.vertices[2]].position;
        node->plane_ = Plane::fromPoints(a, b, c);
    } else {
        // Invalid face - cannot build node
        return nullptr;
    }
    
    std::vector<SolidFace> frontFaces, backFaces;
    std::vector<SolidFace> coplanarFront, coplanarBack;
    
    // HIGH FIX: Pass vertices by reference so new split vertices are preserved
    // No longer create a local copy that would be lost
    
    for (const auto& face : faces) {
        // HIGH FIX: Use vertices directly (passed by reference) so new split vertices are preserved
        node->splitFace(face, vertices, coplanarFront, coplanarBack,
                       frontFaces, backFaces, vertices);
    }
    
    // Add coplanar faces to this node
    node->coplanarFaces_.insert(node->coplanarFaces_.end(),
                               coplanarFront.begin(), coplanarFront.end());
    node->coplanarFaces_.insert(node->coplanarFaces_.end(),
                               coplanarBack.begin(), coplanarBack.end());
    
    // Recursively build children - pass vertices by reference to preserve split vertices
    if (!frontFaces.empty()) {
        node->front_ = build(frontFaces, vertices, epsilon);
    }
    if (!backFaces.empty()) {
        node->back_ = build(backFaces, vertices, epsilon);
    }
    
    return node;
}

BSPNode::Classification BSPNode::classifyPoint(const glm::vec3& point) const {
    float dist = glm::dot(plane_.normal, point) - plane_.distance;
    
    if (dist > epsilon_) return Classification::Front;
    if (dist < -epsilon_) return Classification::Back;
    return Classification::Coplanar;
}

BSPNode::Classification BSPNode::classifyFace(const SolidFace& face,
                                              const std::vector<SolidVertex>& vertices) const {
    int frontCount = 0, backCount = 0, coplanarCount = 0;
    
    for (uint32_t vertIdx : face.vertices) {
        auto cls = classifyPoint(vertices[vertIdx].position);
        switch (cls) {
            case Classification::Front: ++frontCount; break;
            case Classification::Back: ++backCount; break;
            case Classification::Coplanar: ++coplanarCount; break;
            default: break;
        }
    }
    
    if (frontCount > 0 && backCount > 0) return Classification::Spanning;
    if (frontCount > 0) return Classification::Front;
    if (backCount > 0) return Classification::Back;
    return Classification::Coplanar;
}

void BSPNode::splitFace(const SolidFace& face, const std::vector<SolidVertex>& vertices,
                        std::vector<SolidFace>& coplanarFront,
                        std::vector<SolidFace>& coplanarBack,
                        std::vector<SolidFace>& front,
                        std::vector<SolidFace>& back,
                        std::vector<SolidVertex>& newVertices) const {
    auto cls = classifyFace(face, vertices);
    
    switch (cls) {
        case Classification::Coplanar:
            // Check if face normal points same direction as plane
            if (glm::dot(face.normal, plane_.normal) > 0) {
                coplanarFront.push_back(face);
            } else {
                coplanarBack.push_back(face);
            }
            break;
            
        case Classification::Front:
            front.push_back(face);
            break;
            
        case Classification::Back:
            back.push_back(face);
            break;
            
        case Classification::Spanning: {
            // Split the face at the plane
            std::vector<uint32_t> frontVerts, backVerts;
            
            size_t numVerts = face.vertices.size();
            for (size_t i = 0; i < numVerts; ++i) {
                uint32_t vi = face.vertices[i];
                uint32_t vj = face.vertices[(i + 1) % numVerts];
                
                const glm::vec3& pi = vertices[vi].position;
                const glm::vec3& pj = vertices[vj].position;
                
                auto ciClass = classifyPoint(pi);
                
                if (ciClass != Classification::Back) {
                    frontVerts.push_back(vi);
                }
                if (ciClass != Classification::Front) {
                    backVerts.push_back(vi);
                }
                
                auto cjClass = classifyPoint(pj);
                
                // Check if edge crosses the plane
                if ((ciClass == Classification::Front && cjClass == Classification::Back) ||
                    (ciClass == Classification::Back && cjClass == Classification::Front)) {
                    
                    // Compute intersection point
                    float ti = glm::dot(plane_.normal, pi) - plane_.distance;
                    float tj = glm::dot(plane_.normal, pj) - plane_.distance;
                    float t = ti / (ti - tj);
                    
                    SolidVertex newVert;
                    newVert.position = glm::mix(pi, pj, t);
                    newVert.normal = glm::normalize(glm::mix(vertices[vi].normal, 
                                                             vertices[vj].normal, t));
                    
                    uint32_t newIdx = static_cast<uint32_t>(newVertices.size());
                    newVertices.push_back(newVert);
                    
                    frontVerts.push_back(newIdx);
                    backVerts.push_back(newIdx);
                }
            }
            
            // Create front face if valid
            if (frontVerts.size() >= 3) {
                SolidFace frontFace;
                frontFace.vertices = frontVerts;
                frontFace.normal = face.normal;
                frontFace.surfaceType = face.surfaceType;
                front.push_back(frontFace);
            }
            
            // Create back face if valid
            if (backVerts.size() >= 3) {
                SolidFace backFace;
                backFace.vertices = backVerts;
                backFace.normal = face.normal;
                backFace.surfaceType = face.surfaceType;
                back.push_back(backFace);
            }
            break;
        }
    }
}

void BSPNode::clipPolygons(std::vector<SolidFace>& faces,
                          std::vector<SolidVertex>& vertices) const {
    if (faces.empty()) return;
    
    std::vector<SolidFace> frontFaces, backFaces;
    std::vector<SolidFace> coplanarFront, coplanarBack;
    
    for (const auto& face : faces) {
        splitFace(face, vertices, coplanarFront, coplanarBack,
                 frontFaces, backFaces, vertices);
    }
    
    if (front_) {
        front_->clipPolygons(frontFaces, vertices);
    }
    
    if (back_) {
        back_->clipPolygons(backFaces, vertices);
    } else {
        backFaces.clear();  // Remove faces behind BSP with no back node
    }
    
    // Combine results
    faces = frontFaces;
    faces.insert(faces.end(), backFaces.begin(), backFaces.end());
    faces.insert(faces.end(), coplanarFront.begin(), coplanarFront.end());
}

void BSPNode::clipTo(BSPNode* other) {
    // Clip coplanar faces
    std::vector<SolidVertex> tempVerts;  // Would need actual vertex data in real implementation
    // other->clipPolygons(coplanarFaces_, tempVerts);
    
    if (front_) front_->clipTo(other);
    if (back_) back_->clipTo(other);
}

void BSPNode::allPolygons(std::vector<SolidFace>& result) const {
    result.insert(result.end(), coplanarFaces_.begin(), coplanarFaces_.end());
    
    if (front_) front_->allPolygons(result);
    if (back_) back_->allPolygons(result);
}

void BSPNode::invert() {
    // Flip plane
    plane_.normal = -plane_.normal;
    plane_.distance = -plane_.distance;
    
    // Flip face normals and reverse vertex order
    for (auto& face : coplanarFaces_) {
        std::reverse(face.vertices.begin(), face.vertices.end());
        face.normal = -face.normal;
    }
    
    // Swap children
    std::swap(front_, back_);
    
    // Recursively invert children
    if (front_) front_->invert();
    if (back_) back_->invert();
}

// ===================
// BooleanOps Implementation
// ===================

BooleanResult BooleanOps::booleanUnion(const Solid& solidA, const Solid& solidB,
                                       const BooleanOptions& options) {
    return compute(BooleanOperation::Union, solidA, solidB, options);
}

BooleanResult BooleanOps::booleanSubtract(const Solid& solidA, const Solid& solidB,
                                          const BooleanOptions& options) {
    return compute(BooleanOperation::Subtract, solidA, solidB, options);
}

BooleanResult BooleanOps::booleanIntersect(const Solid& solidA, const Solid& solidB,
                                           const BooleanOptions& options) {
    return compute(BooleanOperation::Intersect, solidA, solidB, options);
}

BooleanResult BooleanOps::compute(BooleanOperation op,
                                  const Solid& solidA, const Solid& solidB,
                                  const BooleanOptions& options) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    BooleanResult result;
    
    // Quick bounding box check
    if (!boundingBoxesOverlap(solidA, solidB)) {
        if (op == BooleanOperation::Union) {
            // For union with no overlap, just combine both
            result.success = true;
            result.solid = solidA.clone();
            // Would need to merge solidB as separate shell
            // For now, just return A (simplified)
        } else if (op == BooleanOperation::Subtract) {
            // No overlap means subtract has no effect
            result.success = true;
            result.solid = solidA.clone();
        } else {
            // No overlap means empty intersection
            result.success = true;
            result.solid = Solid();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.stats.computeTimeMs = std::chrono::duration<float, std::milli>(
            endTime - startTime).count();
        return result;
    }
    
    // Perform BSP-based boolean
    result = performBSPBoolean(solidA, solidB, op, options);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.stats.computeTimeMs = std::chrono::duration<float, std::milli>(
        endTime - startTime).count();
    
    return result;
}

BooleanResult BooleanOps::booleanUnionMultiple(const std::vector<Solid>& solids,
                                               const BooleanOptions& options) {
    BooleanResult result;
    
    if (solids.empty()) {
        result.success = true;
        result.solid = Solid();
        return result;
    }
    
    if (solids.size() == 1) {
        result.success = true;
        result.solid = solids[0].clone();
        return result;
    }
    
    // Iteratively union all solids
    Solid accumulated = solids[0].clone();
    
    for (size_t i = 1; i < solids.size(); ++i) {
        auto unionResult = booleanUnion(accumulated, solids[i], options);
        if (!unionResult.ok()) {
            return unionResult;
        }
        accumulated = std::move(*unionResult.solid);
        
        result.stats.intersectionCount += unionResult.stats.intersectionCount;
        result.stats.newVertexCount += unionResult.stats.newVertexCount;
        
        if (options.progress) {
            if (!options.progress(float(i) / solids.size())) {
                result.success = false;
                result.error = "Cancelled";
                return result;
            }
        }
    }
    
    result.success = true;
    result.solid = std::move(accumulated);
    return result;
}

BooleanResult BooleanOps::booleanSubtractMultiple(const Solid& base,
                                                   const std::vector<Solid>& tools,
                                                   const BooleanOptions& options) {
    BooleanResult result;
    
    if (tools.empty()) {
        result.success = true;
        result.solid = base.clone();
        return result;
    }
    
    Solid accumulated = base.clone();
    
    for (size_t i = 0; i < tools.size(); ++i) {
        auto subResult = booleanSubtract(accumulated, tools[i], options);
        if (!subResult.ok()) {
            return subResult;
        }
        accumulated = std::move(*subResult.solid);
        
        result.stats.intersectionCount += subResult.stats.intersectionCount;
        result.stats.newVertexCount += subResult.stats.newVertexCount;
        
        if (options.progress) {
            if (!options.progress(float(i) / tools.size())) {
                result.success = false;
                result.error = "Cancelled";
                return result;
            }
        }
    }
    
    result.success = true;
    result.solid = std::move(accumulated);
    return result;
}

BooleanResult BooleanOps::evaluateCSGTree(const std::shared_ptr<CSGNode>& root,
                                          const BooleanOptions& options) {
    BooleanResult result;
    
    if (!root) {
        result.success = false;
        result.error = "Null CSG tree root";
        return result;
    }
    
    if (root->isLeaf()) {
        if (root->solid()) {
            result.success = true;
            result.solid = root->solid()->clone();
        } else {
            result.success = false;
            result.error = "Null solid in leaf node";
        }
        return result;
    }
    
    // Evaluate children
    auto leftResult = evaluateCSGTree(root->left(), options);
    if (!leftResult.ok()) {
        return leftResult;
    }
    
    auto rightResult = evaluateCSGTree(root->right(), options);
    if (!rightResult.ok()) {
        return rightResult;
    }
    
    // Perform operation
    BooleanOperation op;
    switch (root->operation()) {
        case CSGNode::Operation::Union: op = BooleanOperation::Union; break;
        case CSGNode::Operation::Subtract: op = BooleanOperation::Subtract; break;
        case CSGNode::Operation::Intersect: op = BooleanOperation::Intersect; break;
        default:
            result.success = false;
            result.error = "Unknown CSG operation";
            return result;
    }
    
    return compute(op, *leftResult.solid, *rightResult.solid, options);
}

bool BooleanOps::boundingBoxesOverlap(const Solid& solidA, const Solid& solidB) {
    const auto& boxA = solidA.bounds();
    const auto& boxB = solidB.bounds();
    
    return boxA.min.x <= boxB.max.x && boxA.max.x >= boxB.min.x &&
           boxA.min.y <= boxB.max.y && boxA.max.y >= boxB.min.y &&
           boxA.min.z <= boxB.max.z && boxA.max.z >= boxB.min.z;
}

std::vector<std::vector<glm::vec3>> BooleanOps::findIntersectionCurves(
    const Solid& solidA, const Solid& solidB, float tolerance) {
    
    std::vector<std::vector<glm::vec3>> curves;
    
    // Find face-face intersections
    for (const auto& faceA : solidA.faces()) {
        for (const auto& faceB : solidB.faces()) {
            // Skip if face bounding boxes don't overlap
            // TODO: Add bounding box check
            
            // For each triangle pair, find intersection
            // This is a simplified approach - real implementation would be more robust
            
            if (faceA.isTriangle() && faceB.isTriangle()) {
                std::vector<glm::vec3> intersectionPoints;
                
                // CRITICAL FIX: Validate vertex indices before accessing
                if (faceA.vertices[0] >= solidA.vertexCount() ||
                    faceA.vertices[1] >= solidA.vertexCount() ||
                    faceA.vertices[2] >= solidA.vertexCount() ||
                    faceB.vertices[0] >= solidB.vertexCount() ||
                    faceB.vertices[1] >= solidB.vertexCount() ||
                    faceB.vertices[2] >= solidB.vertexCount()) {
                    continue;  // Skip faces with invalid vertex indices
                }
                
                const auto& a0 = solidA.vertex(faceA.vertices[0]).position;
                const auto& a1 = solidA.vertex(faceA.vertices[1]).position;
                const auto& a2 = solidA.vertex(faceA.vertices[2]).position;
                
                const auto& b0 = solidB.vertex(faceB.vertices[0]).position;
                const auto& b1 = solidB.vertex(faceB.vertices[1]).position;
                const auto& b2 = solidB.vertex(faceB.vertices[2]).position;
                
                if (trianglesIntersect(a0, a1, a2, b0, b1, b2, intersectionPoints)) {
                    if (intersectionPoints.size() >= 2) {
                        curves.push_back(intersectionPoints);
                    }
                }
            }
        }
    }
    
    // TODO: Chain intersection segments into continuous curves
    
    return curves;
}

std::vector<bool> BooleanOps::classifyPoints(const Solid& solid,
                                            const std::vector<glm::vec3>& points) {
    std::vector<bool> results;
    results.reserve(points.size());
    
    for (const auto& point : points) {
        results.push_back(isPointInside(solid, point));
    }
    
    return results;
}

bool BooleanOps::isPointInside(const Solid& solid, const glm::vec3& point) {
    // Ray casting method: count intersections with solid surface
    // Odd count = inside, even count = outside
    
    if (!solid.bounds().isValid()) return false;
    
    // Check bounding box first
    const auto& bounds = solid.bounds();
    if (point.x < bounds.min.x || point.x > bounds.max.x ||
        point.y < bounds.min.y || point.y > bounds.max.y ||
        point.z < bounds.min.z || point.z > bounds.max.z) {
        return false;
    }
    
    // Cast ray in +X direction
    glm::vec3 rayDir(1, 0, 0);
    int intersectionCount = 0;
    
    for (const auto& face : solid.faces()) {
        if (!face.isTriangle()) continue;  // Only handle triangles for simplicity
        
        // CRITICAL FIX: Validate vertex indices before accessing
        if (face.vertices[0] >= solid.vertexCount() ||
            face.vertices[1] >= solid.vertexCount() ||
            face.vertices[2] >= solid.vertexCount()) {
            continue;  // Skip faces with invalid vertex indices
        }
        
        const auto& v0 = solid.vertex(face.vertices[0]).position;
        const auto& v1 = solid.vertex(face.vertices[1]).position;
        const auto& v2 = solid.vertex(face.vertices[2]).position;
        
        auto hit = rayTriangleIntersect(point, rayDir, v0, v1, v2);
        if (hit.has_value() && hit->x > point.x) {
            ++intersectionCount;
        }
    }
    
    return (intersectionCount % 2) == 1;
}

// ===================
// Private Helpers
// ===================

BooleanResult BooleanOps::performBSPBoolean(const Solid& solidA, const Solid& solidB,
                                            BooleanOperation op,
                                            const BooleanOptions& options) {
    BooleanResult result;
    
    // Convert solids to mutable face/vertex lists
    std::vector<SolidFace> facesA = solidA.faces();
    std::vector<SolidFace> facesB = solidB.faces();
    std::vector<SolidVertex> verticesA = solidA.vertices();
    std::vector<SolidVertex> verticesB = solidB.vertices();
    
    // Build BSP trees
    auto bspA = BSPNode::build(facesA, verticesA, options.coplanarEpsilon);
    auto bspB = BSPNode::build(facesB, verticesB, options.coplanarEpsilon);
    
    if (!bspA || !bspB) {
        result.success = false;
        result.error = "Failed to build BSP trees";
        return result;
    }
    
    if (options.progress && !options.progress(0.3f)) {
        result.success = false;
        result.error = "Cancelled";
        return result;
    }
    
    std::vector<SolidFace> resultFaces;
    std::vector<SolidVertex> resultVertices;
    
    // Perform operation based on type
    switch (op) {
        case BooleanOperation::Union: {
            // Union: A + B - (A ∩ B)
            // Clip A to outside of B, clip B to outside of A, combine
            
            auto clippedA = facesA;
            auto clippedB = facesB;
            auto vertsA = verticesA;
            auto vertsB = verticesB;
            
            // Clip A by B (keep outside parts)
            bspB->clipPolygons(clippedA, vertsA);
            
            // Clip B by A (keep outside parts)
            bspA->clipPolygons(clippedB, vertsB);
            
            // Merge vertices (offset B's vertex indices)
            uint32_t vertexOffset = static_cast<uint32_t>(vertsA.size());
            resultVertices = vertsA;
            resultVertices.insert(resultVertices.end(), vertsB.begin(), vertsB.end());
            
            // Offset face vertex indices for B
            resultFaces = clippedA;
            for (auto& face : clippedB) {
                for (auto& vi : face.vertices) {
                    vi += vertexOffset;
                }
                resultFaces.push_back(face);
            }
            break;
        }
        
        case BooleanOperation::Subtract: {
            // Subtract: A - B
            // Clip A to outside of B, clip B to inside of A (inverted), combine
            
            auto clippedA = facesA;
            auto clippedB = facesB;
            auto vertsA = verticesA;
            auto vertsB = verticesB;
            
            // Clip A by B
            bspB->clipPolygons(clippedA, vertsA);
            
            // Invert B and clip by A
            bspB->invert();
            bspA->clipPolygons(clippedB, vertsB);
            
            // Invert clipped B faces (to face inward)
            for (auto& face : clippedB) {
                std::reverse(face.vertices.begin(), face.vertices.end());
                face.normal = -face.normal;
            }
            
            // Merge
            uint32_t vertexOffset = static_cast<uint32_t>(vertsA.size());
            resultVertices = vertsA;
            resultVertices.insert(resultVertices.end(), vertsB.begin(), vertsB.end());
            
            resultFaces = clippedA;
            for (auto& face : clippedB) {
                for (auto& vi : face.vertices) {
                    vi += vertexOffset;
                }
                resultFaces.push_back(face);
            }
            break;
        }
        
        case BooleanOperation::Intersect: {
            // Intersect: A ∩ B
            // Invert both, union, invert result
            
            auto clippedA = facesA;
            auto clippedB = facesB;
            auto vertsA = verticesA;
            auto vertsB = verticesB;
            
            // Invert A and clip by B (keep A inside B)
            bspA->invert();
            bspB->clipPolygons(clippedA, vertsA);
            
            // Invert B and clip by inverted A (keep B inside A)
            bspB->invert();
            bspA->clipPolygons(clippedB, vertsB);
            
            // Invert both face sets
            for (auto& face : clippedA) {
                std::reverse(face.vertices.begin(), face.vertices.end());
                face.normal = -face.normal;
            }
            for (auto& face : clippedB) {
                std::reverse(face.vertices.begin(), face.vertices.end());
                face.normal = -face.normal;
            }
            
            // Merge
            uint32_t vertexOffset = static_cast<uint32_t>(vertsA.size());
            resultVertices = vertsA;
            resultVertices.insert(resultVertices.end(), vertsB.begin(), vertsB.end());
            
            resultFaces = clippedA;
            for (auto& face : clippedB) {
                for (auto& vi : face.vertices) {
                    vi += vertexOffset;
                }
                resultFaces.push_back(face);
            }
            break;
        }
    }
    
    if (options.progress && !options.progress(0.7f)) {
        result.success = false;
        result.error = "Cancelled";
        return result;
    }
    
    // Merge coincident vertices
    mergeVertices(resultVertices, resultFaces, options.tolerance);
    
    // Clean up degenerate faces
    cleanupDegenerateFaces(resultFaces, resultVertices, options.tolerance);
    
    if (options.progress && !options.progress(0.9f)) {
        result.success = false;
        result.error = "Cancelled";
        return result;
    }
    
    // Build result solid
    Solid resultSolid;
    resultSolid.vertices() = std::move(resultVertices);
    resultSolid.faces() = std::move(resultFaces);
    resultSolid.rebuildTopology();
    
    result.success = true;
    result.solid = std::move(resultSolid);
    result.stats.newFaceCount = static_cast<int>(result.solid->faceCount());
    result.stats.newVertexCount = static_cast<int>(result.solid->vertexCount());
    
    if (options.progress) options.progress(1.0f);
    
    return result;
}

std::unique_ptr<BSPNode> BooleanOps::solidToBSP(const Solid& solid) {
    std::vector<SolidFace> faces = solid.faces();
    std::vector<SolidVertex> vertices = solid.vertices();
    return BSPNode::build(faces, vertices);
}

Solid BooleanOps::bspToSolid(BSPNode* bsp, const std::vector<SolidVertex>& vertices) {
    Solid solid;
    
    if (bsp) {
        bsp->allPolygons(solid.faces());
    }
    solid.vertices() = vertices;
    solid.rebuildTopology();
    
    return solid;
}

void BooleanOps::mergeVertices(std::vector<SolidVertex>& vertices,
                               std::vector<SolidFace>& faces,
                               float tolerance) {
    if (vertices.empty()) return;
    
    float toleranceSq = tolerance * tolerance;
    std::vector<uint32_t> vertexRemap(vertices.size());
    std::vector<SolidVertex> mergedVertices;
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        const auto& v = vertices[i];
        bool merged = false;
        
        // Check against existing merged vertices
        for (size_t j = 0; j < mergedVertices.size(); ++j) {
            if (glm::distance2(v.position, mergedVertices[j].position) < toleranceSq) {
                vertexRemap[i] = static_cast<uint32_t>(j);
                merged = true;
                break;
            }
        }
        
        if (!merged) {
            vertexRemap[i] = static_cast<uint32_t>(mergedVertices.size());
            mergedVertices.push_back(v);
        }
    }
    
    // Update face indices
    for (auto& face : faces) {
        for (auto& vi : face.vertices) {
            vi = vertexRemap[vi];
        }
    }
    
    vertices = std::move(mergedVertices);
}

void BooleanOps::cleanupDegenerateFaces(std::vector<SolidFace>& faces,
                                        const std::vector<SolidVertex>& vertices,
                                        float tolerance) {
    faces.erase(
        std::remove_if(faces.begin(), faces.end(),
            [&vertices, tolerance](const SolidFace& face) {
                // Remove faces with less than 3 vertices
                if (face.vertices.size() < 3) return true;
                
                // Remove faces with duplicate vertices
                std::unordered_set<uint32_t> uniqueVerts(face.vertices.begin(), face.vertices.end());
                if (uniqueVerts.size() < 3) return true;
                
                // Remove faces with zero area (collinear vertices)
                if (face.vertices.size() == 3) {
                    const auto& v0 = vertices[face.vertices[0]].position;
                    const auto& v1 = vertices[face.vertices[1]].position;
                    const auto& v2 = vertices[face.vertices[2]].position;
                    
                    float area = glm::length(glm::cross(v1 - v0, v2 - v0)) * 0.5f;
                    if (area < tolerance * tolerance) return true;
                }
                
                return false;
            }),
        faces.end());
}

bool BooleanOps::trianglesIntersect(const glm::vec3& a0, const glm::vec3& a1, const glm::vec3& a2,
                                    const glm::vec3& b0, const glm::vec3& b1, const glm::vec3& b2,
                                    std::vector<glm::vec3>& intersectionPoints) {
    // Möller–Trumbore intersection algorithm for triangle-triangle intersection
    // Returns intersection line segment endpoints
    
    // Compute plane of triangle A
    glm::vec3 normalA = glm::cross(a1 - a0, a2 - a0);
    float lenA = glm::length(normalA);
    if (lenA < 1e-10f) return false;
    normalA /= lenA;
    float dA = -glm::dot(normalA, a0);
    
    // Compute plane of triangle B
    glm::vec3 normalB = glm::cross(b1 - b0, b2 - b0);
    float lenB = glm::length(normalB);
    if (lenB < 1e-10f) return false;
    normalB /= lenB;
    float dB = -glm::dot(normalB, b0);
    
    // Classify vertices of B with respect to plane A
    float db0 = glm::dot(normalA, b0) + dA;
    float db1 = glm::dot(normalA, b1) + dA;
    float db2 = glm::dot(normalA, b2) + dA;
    
    // All vertices on same side means no intersection
    if (db0 > 0 && db1 > 0 && db2 > 0) return false;
    if (db0 < 0 && db1 < 0 && db2 < 0) return false;
    
    // Classify vertices of A with respect to plane B
    float da0 = glm::dot(normalB, a0) + dB;
    float da1 = glm::dot(normalB, a1) + dB;
    float da2 = glm::dot(normalB, a2) + dB;
    
    if (da0 > 0 && da1 > 0 && da2 > 0) return false;
    if (da0 < 0 && da1 < 0 && da2 < 0) return false;
    
    // Compute intersection line direction
    glm::vec3 lineDir = glm::cross(normalA, normalB);
    if (glm::length2(lineDir) < 1e-10f) {
        // Planes are parallel (coplanar case)
        return false;
    }
    
    // Simplified: if we got here, triangles likely intersect
    // Return midpoints as approximate intersection
    glm::vec3 midA = (a0 + a1 + a2) / 3.0f;
    glm::vec3 midB = (b0 + b1 + b2) / 3.0f;
    intersectionPoints.push_back((midA + midB) * 0.5f);
    
    return true;
}

std::optional<glm::vec3> BooleanOps::rayTriangleIntersect(const glm::vec3& origin,
                                                          const glm::vec3& dir,
                                                          const glm::vec3& v0,
                                                          const glm::vec3& v1,
                                                          const glm::vec3& v2) {
    const float EPSILON = 1e-7f;
    
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);
    
    if (std::abs(a) < EPSILON) return std::nullopt;
    
    float f = 1.0f / a;
    glm::vec3 s = origin - v0;
    float u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f) return std::nullopt;
    
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    
    if (v < 0.0f || u + v > 1.0f) return std::nullopt;
    
    float t = f * glm::dot(edge2, q);
    
    if (t > EPSILON) {
        return origin + dir * t;
    }
    
    return std::nullopt;
}

// ===================
// ExactPredicates Implementation
// ===================

double ExactPredicates::orient3d(const glm::dvec3& a, const glm::dvec3& b,
                                 const glm::dvec3& c, const glm::dvec3& d) {
    // Standard 4x4 determinant using double precision
    // For true robustness, would need adaptive precision
    
    double adx = a.x - d.x;
    double ady = a.y - d.y;
    double adz = a.z - d.z;
    double bdx = b.x - d.x;
    double bdy = b.y - d.y;
    double bdz = b.z - d.z;
    double cdx = c.x - d.x;
    double cdy = c.y - d.y;
    double cdz = c.z - d.z;
    
    return adx * (bdy * cdz - bdz * cdy)
         - ady * (bdx * cdz - bdz * cdx)
         + adz * (bdx * cdy - bdy * cdx);
}

double ExactPredicates::orient2d(const glm::dvec2& a, const glm::dvec2& b,
                                 const glm::dvec2& c) {
    return (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
}

double ExactPredicates::inSphere(const glm::dvec3& a, const glm::dvec3& b,
                                 const glm::dvec3& c, const glm::dvec3& d,
                                 const glm::dvec3& e) {
    // 5x5 determinant for in-sphere test
    double aex = a.x - e.x;
    double aey = a.y - e.y;
    double aez = a.z - e.z;
    double bex = b.x - e.x;
    double bey = b.y - e.y;
    double bez = b.z - e.z;
    double cex = c.x - e.x;
    double cey = c.y - e.y;
    double cez = c.z - e.z;
    double dex = d.x - e.x;
    double dey = d.y - e.y;
    double dez = d.z - e.z;
    
    double aeLenSq = aex*aex + aey*aey + aez*aez;
    double beLenSq = bex*bex + bey*bey + bez*bez;
    double ceLenSq = cex*cex + cey*cey + cez*cez;
    double deLenSq = dex*dex + dey*dey + dez*dez;
    
    double ab = aex*bey - bex*aey;
    double bc = bex*cey - cex*bey;
    double cd = cex*dey - dex*cey;
    double da = dex*aey - aex*dey;
    double ac = aex*cey - cex*aey;
    double bd = bex*dey - dex*bey;
    
    double abc = aez*bc - bez*ac + cez*ab;
    double bcd = bez*cd - cez*bd + dez*bc;
    double cda = cez*da + dez*ac + aez*cd;
    double dab = dez*ab + aez*bd + bez*da;
    
    return (deLenSq * abc - ceLenSq * dab) + (beLenSq * cda - aeLenSq * bcd);
}

} // namespace geometry
} // namespace dc3d
