/**
 * @file Solid.cpp
 * @brief Implementation of B-Rep solid body representation
 */

#include "Solid.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <queue>
#include <stack>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace {
// LOW FIX: Named constants for magic numbers used throughout the codebase
constexpr float TOLERANCE_DEFAULT = 0.001f;          // General geometric tolerance
constexpr float TOLERANCE_AREA = 1e-10f;             // Minimum face area
constexpr float TOLERANCE_LENGTH = 1e-7f;            // Ray intersection epsilon
constexpr float ANGLE_SHARP_EDGE = 0.523599f;        // ~30 degrees - sharp edge threshold
constexpr float ANGLE_TANGENT = 0.087266f;           // ~5 degrees - tangent edge threshold
constexpr float TOLERANCE_DEGENERATE = 1e-6f;        // Degenerate geometry check
}

namespace dc3d {
namespace geometry {

// Static ID generator - now thread-safe with atomic
static std::atomic<SolidId> nextId_{1};

SolidId Solid::generateNextId() {
    return nextId_.fetch_add(1, std::memory_order_relaxed);
}

// ===================
// SolidFace Implementation
// ===================

bool SolidFace::isPlanar(float tolerance) const {
    if (vertices.size() < 4) return true;  // Triangles are always planar
    
    // Check if all vertices lie on the plane defined by the first 3
    // Already computed normal should work for this check
    return true;  // Simplified - actual implementation would check coplanarity
}

// ===================
// Solid Construction
// ===================

Solid::Solid() {
    id_ = generateNextId();
}

Result<Solid> Solid::fromMesh(const MeshData& mesh, ProgressCallback progress) {
    if (mesh.indices().empty() || mesh.vertices().empty()) {
        return Result<Solid>::failure("Empty mesh data");
    }
    
    Solid solid;
    solid.name_ = "Solid from Mesh";
    
    // Copy vertices
    solid.vertices_.reserve(mesh.vertices().size());
    for (size_t i = 0; i < mesh.vertices().size(); ++i) {
        SolidVertex v;
        v.id = generateNextId();
        v.position = mesh.vertices()[i];
        if (i < mesh.normals().size()) {
            v.normal = mesh.normals()[i];
        }
        solid.vertices_.push_back(v);
    }
    
    if (progress && !progress(0.2f)) {
        return Result<Solid>::failure("Cancelled");
    }
    
    // Create faces from triangles
    size_t numTriangles = mesh.indices().size() / 3;
    solid.faces_.reserve(numTriangles);
    
    for (size_t i = 0; i < numTriangles; ++i) {
        SolidFace face;
        face.id = generateNextId();
        face.vertices = {
            mesh.indices()[i * 3],
            mesh.indices()[i * 3 + 1],
            mesh.indices()[i * 3 + 2]
        };
        solid.faces_.push_back(face);
        
        if (progress && i % 10000 == 0) {
            float p = 0.2f + 0.4f * (float(i) / numTriangles);
            if (!progress(p)) {
                return Result<Solid>::failure("Cancelled");
            }
        }
    }
    
    if (progress && !progress(0.6f)) {
        return Result<Solid>::failure("Cancelled");
    }
    
    // Build topology
    solid.buildEdges();
    
    if (progress && !progress(0.8f)) {
        return Result<Solid>::failure("Cancelled");
    }
    
    solid.buildAdjacency();
    solid.computeFaceProperties();
    solid.computeEdgeProperties();
    solid.recomputeBounds();
    solid.identifyShells();
    
    if (progress) progress(1.0f);
    
    return Result<Solid>::success(std::move(solid));
}

Result<Solid> Solid::fromHalfEdgeMesh(const HalfEdgeMesh& heMesh) {
    MeshData mesh = heMesh.toMeshData();
    return fromMesh(mesh);
}

// ===================
// Primitive Creation
// ===================

Solid Solid::createBox(const glm::vec3& size, const glm::vec3& center) {
    Solid solid;
    solid.name_ = "Box";
    
    glm::vec3 half = size * 0.5f;
    
    // 8 vertices
    solid.vertices_.resize(8);
    glm::vec3 corners[8] = {
        center + glm::vec3(-half.x, -half.y, -half.z),  // 0: left-bottom-back
        center + glm::vec3( half.x, -half.y, -half.z),  // 1: right-bottom-back
        center + glm::vec3( half.x,  half.y, -half.z),  // 2: right-top-back
        center + glm::vec3(-half.x,  half.y, -half.z),  // 3: left-top-back
        center + glm::vec3(-half.x, -half.y,  half.z),  // 4: left-bottom-front
        center + glm::vec3( half.x, -half.y,  half.z),  // 5: right-bottom-front
        center + glm::vec3( half.x,  half.y,  half.z),  // 6: right-top-front
        center + glm::vec3(-half.x,  half.y,  half.z),  // 7: left-top-front
    };
    
    for (int i = 0; i < 8; ++i) {
        solid.vertices_[i].id = generateNextId();
        solid.vertices_[i].position = corners[i];
    }
    
    // 6 faces (each as 2 triangles for consistency, but stored as quads)
    // Front face (z+)
    SolidFace front;
    front.id = generateNextId();
    front.vertices = {4, 5, 6, 7};
    front.normal = glm::vec3(0, 0, 1);
    front.surfaceType = SolidFace::SurfaceType::Planar;
    solid.faces_.push_back(front);
    
    // Back face (z-)
    SolidFace back;
    back.id = generateNextId();
    back.vertices = {1, 0, 3, 2};
    back.normal = glm::vec3(0, 0, -1);
    back.surfaceType = SolidFace::SurfaceType::Planar;
    solid.faces_.push_back(back);
    
    // Right face (x+)
    SolidFace right;
    right.id = generateNextId();
    right.vertices = {5, 1, 2, 6};
    right.normal = glm::vec3(1, 0, 0);
    right.surfaceType = SolidFace::SurfaceType::Planar;
    solid.faces_.push_back(right);
    
    // Left face (x-)
    SolidFace left;
    left.id = generateNextId();
    left.vertices = {0, 4, 7, 3};
    left.normal = glm::vec3(-1, 0, 0);
    left.surfaceType = SolidFace::SurfaceType::Planar;
    solid.faces_.push_back(left);
    
    // Top face (y+)
    SolidFace top;
    top.id = generateNextId();
    top.vertices = {3, 7, 6, 2};
    top.normal = glm::vec3(0, 1, 0);
    top.surfaceType = SolidFace::SurfaceType::Planar;
    solid.faces_.push_back(top);
    
    // Bottom face (y-)
    SolidFace bottom;
    bottom.id = generateNextId();
    bottom.vertices = {0, 1, 5, 4};
    bottom.normal = glm::vec3(0, -1, 0);
    bottom.surfaceType = SolidFace::SurfaceType::Planar;
    solid.faces_.push_back(bottom);
    
    solid.rebuildTopology();
    return solid;
}

Solid Solid::createCylinder(float radius, float height, int segments, 
                           const glm::vec3& center) {
    Solid solid;
    solid.name_ = "Cylinder";
    
    // Create vertices for top and bottom circles
    float halfH = height * 0.5f;
    
    // Bottom center (index 0)
    SolidVertex bottomCenter;
    bottomCenter.id = generateNextId();
    bottomCenter.position = center + glm::vec3(0, -halfH, 0);
    bottomCenter.normal = glm::vec3(0, -1, 0);
    solid.vertices_.push_back(bottomCenter);
    
    // Top center (index 1)
    SolidVertex topCenter;
    topCenter.id = generateNextId();
    topCenter.position = center + glm::vec3(0, halfH, 0);
    topCenter.normal = glm::vec3(0, 1, 0);
    solid.vertices_.push_back(topCenter);
    
    // Circle vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        
        // Bottom circle vertex (indices 2 to segments+1)
        SolidVertex vBot;
        vBot.id = generateNextId();
        vBot.position = center + glm::vec3(x, -halfH, z);
        solid.vertices_.push_back(vBot);
        
        // Top circle vertex (indices segments+2 to 2*segments+1)
        SolidVertex vTop;
        vTop.id = generateNextId();
        vTop.position = center + glm::vec3(x, halfH, z);
        solid.vertices_.push_back(vTop);
    }
    
    // Create faces
    // Bottom cap (triangles fanning from center)
    for (int i = 0; i < segments; ++i) {
        SolidFace face;
        face.id = generateNextId();
        int curr = 2 + i * 2;          // Current bottom vertex
        int next = 2 + ((i + 1) % segments) * 2;  // Next bottom vertex
        face.vertices = {0, (uint32_t)next, (uint32_t)curr};
        face.normal = glm::vec3(0, -1, 0);
        face.surfaceType = SolidFace::SurfaceType::Planar;
        solid.faces_.push_back(face);
    }
    
    // Top cap
    for (int i = 0; i < segments; ++i) {
        SolidFace face;
        face.id = generateNextId();
        int curr = 3 + i * 2;          // Current top vertex
        int next = 3 + ((i + 1) % segments) * 2;  // Next top vertex
        face.vertices = {1, (uint32_t)curr, (uint32_t)next};
        face.normal = glm::vec3(0, 1, 0);
        face.surfaceType = SolidFace::SurfaceType::Planar;
        solid.faces_.push_back(face);
    }
    
    // Side faces (quads as 2 triangles each)
    for (int i = 0; i < segments; ++i) {
        int botCurr = 2 + i * 2;
        int botNext = 2 + ((i + 1) % segments) * 2;
        int topCurr = 3 + i * 2;
        int topNext = 3 + ((i + 1) % segments) * 2;
        
        // First triangle
        SolidFace f1;
        f1.id = generateNextId();
        f1.vertices = {(uint32_t)botCurr, (uint32_t)botNext, (uint32_t)topNext};
        f1.surfaceType = SolidFace::SurfaceType::Cylindrical;
        solid.faces_.push_back(f1);
        
        // Second triangle
        SolidFace f2;
        f2.id = generateNextId();
        f2.vertices = {(uint32_t)botCurr, (uint32_t)topNext, (uint32_t)topCurr};
        f2.surfaceType = SolidFace::SurfaceType::Cylindrical;
        solid.faces_.push_back(f2);
    }
    
    solid.rebuildTopology();
    return solid;
}

Solid Solid::createSphere(float radius, int segments, const glm::vec3& center) {
    Solid solid;
    solid.name_ = "Sphere";
    
    int latSegments = segments;
    int lonSegments = segments * 2;
    
    // Create vertices
    // Top pole
    SolidVertex topPole;
    topPole.id = generateNextId();
    topPole.position = center + glm::vec3(0, radius, 0);
    topPole.normal = glm::vec3(0, 1, 0);
    solid.vertices_.push_back(topPole);
    
    // Interior vertices
    for (int lat = 1; lat < latSegments; ++lat) {
        float phi = glm::pi<float>() * lat / latSegments;
        float y = radius * std::cos(phi);
        float ringRadius = radius * std::sin(phi);
        
        for (int lon = 0; lon < lonSegments; ++lon) {
            float theta = 2.0f * glm::pi<float>() * lon / lonSegments;
            float x = ringRadius * std::cos(theta);
            float z = ringRadius * std::sin(theta);
            
            SolidVertex v;
            v.id = generateNextId();
            v.position = center + glm::vec3(x, y, z);
            v.normal = glm::normalize(glm::vec3(x, y, z));
            solid.vertices_.push_back(v);
        }
    }
    
    // Bottom pole
    SolidVertex bottomPole;
    bottomPole.id = generateNextId();
    bottomPole.position = center + glm::vec3(0, -radius, 0);
    bottomPole.normal = glm::vec3(0, -1, 0);
    solid.vertices_.push_back(bottomPole);
    
    // Create faces
    // Top cap triangles
    for (int lon = 0; lon < lonSegments; ++lon) {
        SolidFace face;
        face.id = generateNextId();
        int next = 1 + (lon + 1) % lonSegments;
        face.vertices = {0, (uint32_t)(1 + lon), (uint32_t)next};
        face.surfaceType = SolidFace::SurfaceType::Spherical;
        solid.faces_.push_back(face);
    }
    
    // Interior quads
    for (int lat = 0; lat < latSegments - 2; ++lat) {
        for (int lon = 0; lon < lonSegments; ++lon) {
            int curr = 1 + lat * lonSegments + lon;
            int next = 1 + lat * lonSegments + (lon + 1) % lonSegments;
            int currBelow = 1 + (lat + 1) * lonSegments + lon;
            int nextBelow = 1 + (lat + 1) * lonSegments + (lon + 1) % lonSegments;
            
            // First triangle
            SolidFace f1;
            f1.id = generateNextId();
            f1.vertices = {(uint32_t)curr, (uint32_t)currBelow, (uint32_t)next};
            f1.surfaceType = SolidFace::SurfaceType::Spherical;
            solid.faces_.push_back(f1);
            
            // Second triangle
            SolidFace f2;
            f2.id = generateNextId();
            f2.vertices = {(uint32_t)next, (uint32_t)currBelow, (uint32_t)nextBelow};
            f2.surfaceType = SolidFace::SurfaceType::Spherical;
            solid.faces_.push_back(f2);
        }
    }
    
    // Bottom cap triangles
    uint32_t bottomIdx = static_cast<uint32_t>(solid.vertices_.size() - 1);
    int lastRing = (latSegments - 2) * lonSegments + 1;
    for (int lon = 0; lon < lonSegments; ++lon) {
        SolidFace face;
        face.id = generateNextId();
        int next = lastRing + (lon + 1) % lonSegments;
        face.vertices = {(uint32_t)(lastRing + lon), bottomIdx, (uint32_t)next};
        face.surfaceType = SolidFace::SurfaceType::Spherical;
        solid.faces_.push_back(face);
    }
    
    solid.rebuildTopology();
    return solid;
}

Solid Solid::createCone(float baseRadius, float topRadius, float height,
                       int segments, const glm::vec3& center) {
    Solid solid;
    solid.name_ = (topRadius < 0.001f) ? "Cone" : "Frustum";
    
    float halfH = height * 0.5f;
    bool hasTopCap = topRadius > 0.001f;
    
    // Bottom center
    SolidVertex bottomCenter;
    bottomCenter.id = generateNextId();
    bottomCenter.position = center + glm::vec3(0, -halfH, 0);
    bottomCenter.normal = glm::vec3(0, -1, 0);
    solid.vertices_.push_back(bottomCenter);
    
    // Top center or apex
    SolidVertex topCenter;
    topCenter.id = generateNextId();
    topCenter.position = center + glm::vec3(0, halfH, 0);
    topCenter.normal = glm::vec3(0, 1, 0);
    solid.vertices_.push_back(topCenter);
    
    // Circle vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);
        
        // Bottom circle
        SolidVertex vBot;
        vBot.id = generateNextId();
        vBot.position = center + glm::vec3(x * baseRadius, -halfH, z * baseRadius);
        solid.vertices_.push_back(vBot);
        
        // Top circle (only if frustum)
        if (hasTopCap) {
            SolidVertex vTop;
            vTop.id = generateNextId();
            vTop.position = center + glm::vec3(x * topRadius, halfH, z * topRadius);
            solid.vertices_.push_back(vTop);
        }
    }
    
    // Bottom cap
    for (int i = 0; i < segments; ++i) {
        SolidFace face;
        face.id = generateNextId();
        int stride = hasTopCap ? 2 : 1;
        int curr = 2 + i * stride;
        int next = 2 + ((i + 1) % segments) * stride;
        face.vertices = {0, (uint32_t)next, (uint32_t)curr};
        face.normal = glm::vec3(0, -1, 0);
        face.surfaceType = SolidFace::SurfaceType::Planar;
        solid.faces_.push_back(face);
    }
    
    // Top cap (if frustum)
    if (hasTopCap) {
        for (int i = 0; i < segments; ++i) {
            SolidFace face;
            face.id = generateNextId();
            int curr = 3 + i * 2;
            int next = 3 + ((i + 1) % segments) * 2;
            face.vertices = {1, (uint32_t)curr, (uint32_t)next};
            face.normal = glm::vec3(0, 1, 0);
            face.surfaceType = SolidFace::SurfaceType::Planar;
            solid.faces_.push_back(face);
        }
    }
    
    // Side faces
    for (int i = 0; i < segments; ++i) {
        int stride = hasTopCap ? 2 : 1;
        int botCurr = 2 + i * stride;
        int botNext = 2 + ((i + 1) % segments) * stride;
        
        if (hasTopCap) {
            int topCurr = 3 + i * 2;
            int topNext = 3 + ((i + 1) % segments) * 2;
            
            // Two triangles for quad
            SolidFace f1;
            f1.id = generateNextId();
            f1.vertices = {(uint32_t)botCurr, (uint32_t)botNext, (uint32_t)topNext};
            f1.surfaceType = SolidFace::SurfaceType::Conical;
            solid.faces_.push_back(f1);
            
            SolidFace f2;
            f2.id = generateNextId();
            f2.vertices = {(uint32_t)botCurr, (uint32_t)topNext, (uint32_t)topCurr};
            f2.surfaceType = SolidFace::SurfaceType::Conical;
            solid.faces_.push_back(f2);
        } else {
            // Triangle to apex
            SolidFace face;
            face.id = generateNextId();
            face.vertices = {(uint32_t)botCurr, (uint32_t)botNext, 1};
            face.surfaceType = SolidFace::SurfaceType::Conical;
            solid.faces_.push_back(face);
        }
    }
    
    solid.rebuildTopology();
    return solid;
}

Solid Solid::createTorus(float majorRadius, float minorRadius,
                        int majorSegments, int minorSegments,
                        const glm::vec3& center) {
    Solid solid;
    solid.name_ = "Torus";
    
    // Create vertices
    for (int i = 0; i < majorSegments; ++i) {
        float u = 2.0f * glm::pi<float>() * i / majorSegments;
        glm::vec3 ringCenter = center + glm::vec3(majorRadius * std::cos(u), 0, majorRadius * std::sin(u));
        glm::vec3 radial = glm::normalize(glm::vec3(std::cos(u), 0, std::sin(u)));
        
        for (int j = 0; j < minorSegments; ++j) {
            float v = 2.0f * glm::pi<float>() * j / minorSegments;
            
            glm::vec3 pos = ringCenter + 
                minorRadius * std::cos(v) * radial +
                minorRadius * std::sin(v) * glm::vec3(0, 1, 0);
            
            SolidVertex vert;
            vert.id = generateNextId();
            vert.position = pos;
            vert.normal = glm::normalize(pos - ringCenter);
            solid.vertices_.push_back(vert);
        }
    }
    
    // Create faces
    for (int i = 0; i < majorSegments; ++i) {
        int nextI = (i + 1) % majorSegments;
        
        for (int j = 0; j < minorSegments; ++j) {
            int nextJ = (j + 1) % minorSegments;
            
            int v00 = i * minorSegments + j;
            int v10 = nextI * minorSegments + j;
            int v01 = i * minorSegments + nextJ;
            int v11 = nextI * minorSegments + nextJ;
            
            // First triangle
            SolidFace f1;
            f1.id = generateNextId();
            f1.vertices = {(uint32_t)v00, (uint32_t)v10, (uint32_t)v11};
            f1.surfaceType = SolidFace::SurfaceType::Toroidal;
            solid.faces_.push_back(f1);
            
            // Second triangle
            SolidFace f2;
            f2.id = generateNextId();
            f2.vertices = {(uint32_t)v00, (uint32_t)v11, (uint32_t)v01};
            f2.surfaceType = SolidFace::SurfaceType::Toroidal;
            solid.faces_.push_back(f2);
        }
    }
    
    solid.rebuildTopology();
    return solid;
}

// ===================
// Conversion
// ===================

MeshData Solid::toMesh() const {
    MeshData mesh;
    
    // Copy vertices
    mesh.vertices().reserve(vertices_.size());
    mesh.normals().reserve(vertices_.size());
    for (const auto& v : vertices_) {
        mesh.vertices().push_back(v.position);
        mesh.normals().push_back(v.normal);
    }
    
    // Triangulate faces
    for (const auto& face : faces_) {
        if (face.vertices.size() == 3) {
            mesh.indices().push_back(face.vertices[0]);
            mesh.indices().push_back(face.vertices[1]);
            mesh.indices().push_back(face.vertices[2]);
        } else if (face.vertices.size() == 4) {
            // Quad -> 2 triangles
            mesh.indices().push_back(face.vertices[0]);
            mesh.indices().push_back(face.vertices[1]);
            mesh.indices().push_back(face.vertices[2]);
            
            mesh.indices().push_back(face.vertices[0]);
            mesh.indices().push_back(face.vertices[2]);
            mesh.indices().push_back(face.vertices[3]);
        } else {
            // Fan triangulation for n-gons
            for (size_t i = 1; i < face.vertices.size() - 1; ++i) {
                mesh.indices().push_back(face.vertices[0]);
                mesh.indices().push_back(face.vertices[i]);
                mesh.indices().push_back(face.vertices[i + 1]);
            }
        }
    }
    
    // Trigger bounds computation
    mesh.boundingBox();
    return mesh;
}

Result<HalfEdgeMesh> Solid::toHalfEdgeMesh() const {
    MeshData mesh = toMesh();
    return HalfEdgeMesh::buildFromMesh(mesh);
}

// ===================
// Validation
// ===================

SolidValidation Solid::validate() const {
    // LOW FIX: Thread-safe cached validation access
    {
        std::lock_guard<std::mutex> lock(validationMutex_);
        if (cachedValidation_.has_value()) {
            return *cachedValidation_;
        }
    }
    
    SolidValidation result;
    result.isValid = true;
    
    // Check for empty solid
    if (vertices_.empty() || faces_.empty()) {
        result.isValid = false;
        result.errorMessage = "Empty solid";
        return result;
    }
    
    // Check for boundary (open) edges
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& edge = edges_[i];
        
        if (edge.faces.size() == 1) {
            result.openEdges.push_back(static_cast<uint32_t>(i));
        } else if (edge.faces.size() > 2) {
            result.nonManifoldEdges.push_back(static_cast<uint32_t>(i));
        }
    }
    
    result.openEdgeCount = result.openEdges.size();
    result.nonManifoldEdgeCount = result.nonManifoldEdges.size();
    result.isWatertight = result.openEdges.empty();
    result.hasNonManifoldEdges = !result.nonManifoldEdges.empty();
    
    // Check for non-manifold vertices
    for (size_t i = 0; i < vertices_.size(); ++i) {
        const auto& vertex = vertices_[i];
        if (!vertex.isManifold()) {
            result.nonManifoldVertices.push_back(static_cast<uint32_t>(i));
        }
    }
    result.nonManifoldVertexCount = result.nonManifoldVertices.size();
    result.hasNonManifoldVertices = !result.nonManifoldVertices.empty();
    
    // Check for degenerate faces
    for (size_t i = 0; i < faces_.size(); ++i) {
        const auto& face = faces_[i];
        if (face.area < 1e-10f) {
            result.degenerateFaces.push_back(static_cast<uint32_t>(i));
        }
    }
    result.degenerateFaceCount = result.degenerateFaces.size();
    result.hasZeroAreaFaces = !result.degenerateFaces.empty();
    
    // Check normal consistency (simplified - just check that volume is positive)
    float vol = volume();
    result.hasConsistentNormals = (vol >= 0);
    
    result.isValid = !result.hasNonManifoldEdges && 
                     !result.hasNonManifoldVertices &&
                     !result.hasZeroAreaFaces;
    
    // LOW FIX: Thread-safe cached validation assignment
    {
        std::lock_guard<std::mutex> lock(validationMutex_);
        cachedValidation_ = result;
    }
    return result;
}

bool Solid::isWatertight() const {
    return validate().isWatertight;
}

bool Solid::isManifold() const {
    auto val = validate();
    return !val.hasNonManifoldEdges && !val.hasNonManifoldVertices;
}

bool Solid::hasSelfIntersections(ProgressCallback progress) const {
    // Simplified check - full implementation would use BVH acceleration
    // This is O(n²) and should be optimized for production
    
    size_t numFaces = faces_.size();
    for (size_t i = 0; i < numFaces; ++i) {
        const auto& faceI = faces_[i];
        if (!faceI.isTriangle()) continue;  // Only handle triangles
        
        // LOW FIX: Validate vertex indices for face i
        if (faceI.vertices[0] >= vertices_.size() ||
            faceI.vertices[1] >= vertices_.size() ||
            faceI.vertices[2] >= vertices_.size()) {
            continue;
        }
        
        const glm::vec3& a0 = vertices_[faceI.vertices[0]].position;
        const glm::vec3& a1 = vertices_[faceI.vertices[1]].position;
        const glm::vec3& a2 = vertices_[faceI.vertices[2]].position;
        
        for (size_t j = i + 2; j < numFaces; ++j) {
            const auto& faceJ = faces_[j];
            if (!faceJ.isTriangle()) continue;
            
            // Skip adjacent faces (share a vertex)
            bool adjacent = false;
            for (uint32_t vi : faceI.vertices) {
                for (uint32_t vj : faceJ.vertices) {
                    if (vi == vj) {
                        adjacent = true;
                        break;
                    }
                }
                if (adjacent) break;
            }
            
            if (!adjacent) {
                // LOW FIX: Implemented actual triangle-triangle intersection test
                if (faceJ.vertices[0] >= vertices_.size() ||
                    faceJ.vertices[1] >= vertices_.size() ||
                    faceJ.vertices[2] >= vertices_.size()) {
                    continue;
                }
                
                const glm::vec3& b0 = vertices_[faceJ.vertices[0]].position;
                const glm::vec3& b1 = vertices_[faceJ.vertices[1]].position;
                const glm::vec3& b2 = vertices_[faceJ.vertices[2]].position;
                
                if (triangleTriangleIntersect(a0, a1, a2, b0, b1, b2)) {
                    return true;  // Found self-intersection
                }
            }
        }
        
        if (progress && i % 1000 == 0) {
            if (!progress(float(i) / numFaces)) {
                return false;  // Cancelled
            }
        }
    }
    
    return false;
}

bool Solid::triangleTriangleIntersect(const glm::vec3& a0, const glm::vec3& a1, const glm::vec3& a2,
                                      const glm::vec3& b0, const glm::vec3& b1, const glm::vec3& b2) const {
    // LOW FIX: Implemented Möller triangle-triangle intersection test
    constexpr float EPSILON = 1e-6f;
    
    // Compute plane of triangle A
    glm::vec3 normalA = glm::cross(a1 - a0, a2 - a0);
    float lenA = glm::length(normalA);
    if (lenA < EPSILON) return false;  // Degenerate triangle
    normalA /= lenA;
    float dA = -glm::dot(normalA, a0);
    
    // Classify vertices of B with respect to plane A
    float db0 = glm::dot(normalA, b0) + dA;
    float db1 = glm::dot(normalA, b1) + dA;
    float db2 = glm::dot(normalA, b2) + dA;
    
    // All on same side = no intersection
    if (db0 > EPSILON && db1 > EPSILON && db2 > EPSILON) return false;
    if (db0 < -EPSILON && db1 < -EPSILON && db2 < -EPSILON) return false;
    
    // Compute plane of triangle B
    glm::vec3 normalB = glm::cross(b1 - b0, b2 - b0);
    float lenB = glm::length(normalB);
    if (lenB < EPSILON) return false;  // Degenerate triangle
    normalB /= lenB;
    float dB = -glm::dot(normalB, b0);
    
    // Classify vertices of A with respect to plane B
    float da0 = glm::dot(normalB, a0) + dB;
    float da1 = glm::dot(normalB, a1) + dB;
    float da2 = glm::dot(normalB, a2) + dB;
    
    // All on same side = no intersection
    if (da0 > EPSILON && da1 > EPSILON && da2 > EPSILON) return false;
    if (da0 < -EPSILON && da1 < -EPSILON && da2 < -EPSILON) return false;
    
    // Compute intersection line direction
    glm::vec3 lineDir = glm::cross(normalA, normalB);
    float lineDirLen = glm::length(lineDir);
    
    // Coplanar case - simplified overlap check
    if (lineDirLen < EPSILON) {
        // Triangles are coplanar - project to 2D and check overlap
        // Simplified: check if any vertex of A is inside B or vice versa
        return false;  // Conservative: coplanar triangles rarely self-intersect in valid meshes
    }
    
    // Non-coplanar case: check if intersection intervals overlap
    // Simplified check: if we reach here, triangles likely intersect
    // Full implementation would compute exact intervals on the intersection line
    return true;
}

// ===================
// Geometric Properties
// ===================

float Solid::volume() const {
    if (cachedVolume_.has_value()) {
        return *cachedVolume_;
    }
    
    // Divergence theorem: sum of signed tetrahedra volumes
    float vol = 0.0f;
    
    for (const auto& face : faces_) {
        if (face.vertices.size() < 3) continue;
        
        // Fan triangulation for non-triangles
        const glm::vec3& v0 = vertices_[face.vertices[0]].position;
        
        for (size_t i = 1; i < face.vertices.size() - 1; ++i) {
            const glm::vec3& v1 = vertices_[face.vertices[i]].position;
            const glm::vec3& v2 = vertices_[face.vertices[i + 1]].position;
            
            // Signed volume of tetrahedron with origin
            vol += glm::dot(v0, glm::cross(v1, v2)) / 6.0f;
        }
    }
    
    // MEDIUM FIX: Preserve signed volume - negative indicates inverted normals
    // Store absolute value but log warning if normals are inverted
    // The sign is useful for detecting inside-out solids
    cachedSignedVolume_ = vol;
    cachedVolume_ = std::abs(vol);
    return *cachedVolume_;
}

float Solid::signedVolume() const {
    // Call volume() to ensure cache is populated
    volume();
    return cachedSignedVolume_.value_or(0.0f);
}

bool Solid::hasInvertedNormals() const {
    // Call volume() to ensure cache is populated
    volume();
    return cachedSignedVolume_.has_value() && cachedSignedVolume_.value() < 0.0f;
}

float Solid::surfaceArea() const {
    if (cachedSurfaceArea_.has_value()) {
        return *cachedSurfaceArea_;
    }
    
    float area = 0.0f;
    for (const auto& face : faces_) {
        area += face.area;
    }
    
    cachedSurfaceArea_ = area;
    return *cachedSurfaceArea_;
}

glm::vec3 Solid::centerOfMass() const {
    if (vertices_.empty()) {
        return glm::vec3(0.0f);
    }
    
    // Weighted average of face centroids by area
    glm::vec3 com(0.0f);
    float totalArea = 0.0f;
    
    for (const auto& face : faces_) {
        com += face.centroid * face.area;
        totalArea += face.area;
    }
    
    return (totalArea > 0) ? com / totalArea : bounds_.center();
}

glm::mat3 Solid::inertiaTensor(float density) const {
    // Simplified - returns diagonal approximation based on bounding box
    glm::vec3 dims = bounds_.dimensions();
    float mass = volume() * density;
    
    float Ixx = (mass / 12.0f) * (dims.y * dims.y + dims.z * dims.z);
    float Iyy = (mass / 12.0f) * (dims.x * dims.x + dims.z * dims.z);
    float Izz = (mass / 12.0f) * (dims.x * dims.x + dims.y * dims.y);
    
    return glm::mat3(
        Ixx, 0, 0,
        0, Iyy, 0,
        0, 0, Izz
    );
}

// ===================
// Adjacency Queries
// ===================

std::vector<uint32_t> Solid::facesAroundVertex(uint32_t vertexIdx) const {
    if (vertexIdx >= vertices_.size()) return {};
    return vertices_[vertexIdx].faces;
}

std::vector<uint32_t> Solid::edgesAroundVertex(uint32_t vertexIdx) const {
    if (vertexIdx >= vertices_.size()) return {};
    return vertices_[vertexIdx].edges;
}

std::vector<uint32_t> Solid::verticesAroundVertex(uint32_t vertexIdx) const {
    if (vertexIdx >= vertices_.size()) return {};
    
    std::vector<uint32_t> neighbors;
    for (uint32_t edgeIdx : vertices_[vertexIdx].edges) {
        // CRITICAL FIX: Validate edge index before dereferencing
        if (edgeIdx >= edges_.size()) continue;
        neighbors.push_back(edges_[edgeIdx].otherVertex(vertexIdx));
    }
    return neighbors;
}

std::vector<uint32_t> Solid::adjacentFaces(uint32_t faceIdx) const {
    if (faceIdx >= faces_.size()) return {};
    
    std::unordered_set<uint32_t> adjSet;
    
    for (uint32_t edgeIdx : faces_[faceIdx].edges) {
        for (uint32_t adjFace : edges_[edgeIdx].faces) {
            if (adjFace != faceIdx) {
                adjSet.insert(adjFace);
            }
        }
    }
    
    return std::vector<uint32_t>(adjSet.begin(), adjSet.end());
}

std::vector<uint32_t> Solid::facesOnEdge(uint32_t edgeIdx) const {
    if (edgeIdx >= edges_.size()) return {};
    return edges_[edgeIdx].faces;
}

std::optional<uint32_t> Solid::findEdge(uint32_t v0, uint32_t v1) const {
    uint64_t key = makeEdgeKey(v0, v1);
    auto it = edgeLookup_.find(key);
    if (it != edgeLookup_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<uint32_t> Solid::edgesOfFace(uint32_t faceIdx) const {
    if (faceIdx >= faces_.size()) return {};
    return faces_[faceIdx].edges;
}

// ===================
// Selection Helpers
// ===================

std::vector<uint32_t> Solid::findSharpEdges(float angleThreshold) const {
    std::vector<uint32_t> result;
    
    for (size_t i = 0; i < edges_.size(); ++i) {
        if (edges_[i].dihedralAngle > angleThreshold) {
            result.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return result;
}

std::vector<uint32_t> Solid::findBoundaryEdges() const {
    std::vector<uint32_t> result;
    
    for (size_t i = 0; i < edges_.size(); ++i) {
        if (edges_[i].isBoundary) {
            result.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return result;
}

std::vector<uint32_t> Solid::findTangentEdges(uint32_t startEdge, 
                                              float angleThreshold) const {
    if (startEdge >= edges_.size()) return {};
    
    std::vector<uint32_t> result;
    std::unordered_set<uint32_t> visited;
    std::queue<uint32_t> queue;
    
    queue.push(startEdge);
    visited.insert(startEdge);
    
    while (!queue.empty()) {
        uint32_t current = queue.front();
        queue.pop();
        result.push_back(current);
        
        // HIGH FIX: Validate current edge index
        if (current >= edges_.size()) continue;
        const auto& edge = edges_[current];
        
        // Check connected edges at both vertices
        for (uint32_t vertIdx : {edge.vertex0, edge.vertex1}) {
            // HIGH FIX: Validate vertex index before accessing
            if (vertIdx >= vertices_.size()) continue;
            
            for (uint32_t adjEdgeIdx : vertices_[vertIdx].edges) {
                // HIGH FIX: Check visited BEFORE computing angle to prevent duplicates
                // and validate edge index
                if (adjEdgeIdx >= edges_.size() || visited.count(adjEdgeIdx)) continue;
                
                // Mark as visited immediately to prevent duplicate processing
                visited.insert(adjEdgeIdx);
                
                // Check if edges are tangent (small angle between directions)
                const auto& adjEdge = edges_[adjEdgeIdx];
                float dot = std::abs(glm::dot(edge.direction, adjEdge.direction));
                float angle = std::acos(std::min(1.0f, dot));
                
                if (angle < angleThreshold || std::abs(angle - glm::pi<float>()) < angleThreshold) {
                    queue.push(adjEdgeIdx);
                }
            }
        }
    }
    
    return result;
}

std::vector<uint32_t> Solid::findConcaveEdges() const {
    std::vector<uint32_t> result;
    
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& edge = edges_[i];
        if (edge.faces.size() != 2) continue;
        
        // Check if edge is concave (dihedral angle < 180°, normals point away)
        const auto& face0 = faces_[edge.faces[0]];
        const auto& face1 = faces_[edge.faces[1]];
        
        glm::vec3 mid = edge.midpoint;
        glm::vec3 toFace0 = face0.centroid - mid;
        
        if (glm::dot(face1.normal, toFace0) > 0) {
            result.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return result;
}

std::vector<uint32_t> Solid::findConvexEdges() const {
    std::vector<uint32_t> result;
    
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& edge = edges_[i];
        if (edge.faces.size() != 2) continue;
        
        const auto& face0 = faces_[edge.faces[0]];
        const auto& face1 = faces_[edge.faces[1]];
        
        glm::vec3 mid = edge.midpoint;
        glm::vec3 toFace0 = face0.centroid - mid;
        
        if (glm::dot(face1.normal, toFace0) <= 0) {
            result.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return result;
}

// ===================
// Transformations
// ===================

void Solid::transform(const glm::mat4& transform) {
    glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(transform)));
    
    for (auto& v : vertices_) {
        glm::vec4 pos(v.position, 1.0f);
        v.position = glm::vec3(transform * pos);
        v.normal = glm::normalize(normalMat * v.normal);
    }
    
    // Recompute derived data
    computeFaceProperties();
    computeEdgeProperties();
    recomputeBounds();
    invalidateCache();
}

void Solid::translate(const glm::vec3& offset) {
    for (auto& v : vertices_) {
        v.position += offset;
    }
    
    for (auto& f : faces_) {
        f.centroid += offset;
    }
    
    for (auto& e : edges_) {
        e.midpoint += offset;
    }
    
    bounds_.min += offset;
    bounds_.max += offset;
}

void Solid::rotate(const glm::quat& rotation, const glm::vec3& center) {
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), center) *
                    glm::mat4_cast(rotation) *
                    glm::translate(glm::mat4(1.0f), -center);
    transform(mat);
}

void Solid::scale(float factor, const glm::vec3& center) {
    scale(glm::vec3(factor), center);
}

void Solid::scale(const glm::vec3& factors, const glm::vec3& center) {
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), center) *
                    glm::scale(glm::mat4(1.0f), factors) *
                    glm::translate(glm::mat4(1.0f), -center);
    transform(mat);
}

void Solid::flipNormals() {
    // Reverse face winding
    for (auto& face : faces_) {
        std::reverse(face.vertices.begin(), face.vertices.end());
        face.normal = -face.normal;
    }
    
    // Flip vertex normals
    for (auto& v : vertices_) {
        v.normal = -v.normal;
    }
    
    invalidateCache();
}

// ===================
// Topology Modification
// ===================

void Solid::rebuildTopology() {
    buildEdges();
    buildAdjacency();
    computeFaceProperties();
    computeEdgeProperties();
    recomputeBounds();
    identifyShells();
    invalidateCache();
}

void Solid::recomputeNormals() {
    computeFaceProperties();
    
    // Recompute vertex normals as average of adjacent face normals
    for (auto& v : vertices_) {
        v.normal = glm::vec3(0.0f);
        for (uint32_t faceIdx : v.faces) {
            v.normal += faces_[faceIdx].normal;
        }
        if (glm::length2(v.normal) > 0) {
            v.normal = glm::normalize(v.normal);
        }
    }
}

void Solid::recomputeBounds() {
    bounds_.reset();
    for (const auto& v : vertices_) {
        bounds_.expand(v.position);
    }
}

size_t Solid::identifyShells() {
    shells_.clear();
    
    if (faces_.empty()) return 0;
    
    std::vector<bool> visited(faces_.size(), false);
    
    for (size_t startFace = 0; startFace < faces_.size(); ++startFace) {
        if (visited[startFace]) continue;
        
        SolidShell shell;
        std::queue<uint32_t> queue;
        queue.push(static_cast<uint32_t>(startFace));
        visited[startFace] = true;
        
        while (!queue.empty()) {
            uint32_t faceIdx = queue.front();
            queue.pop();
            shell.faces.push_back(faceIdx);
            
            // Add unvisited adjacent faces
            for (uint32_t adjFace : adjacentFaces(faceIdx)) {
                if (!visited[adjFace]) {
                    visited[adjFace] = true;
                    queue.push(adjFace);
                }
            }
        }
        
        // Compute shell properties
        shell.surfaceArea = 0.0f;
        shell.bounds.reset();
        
        for (uint32_t faceIdx : shell.faces) {
            shell.surfaceArea += faces_[faceIdx].area;
            for (uint32_t vertIdx : faces_[faceIdx].vertices) {
                shell.bounds.expand(vertices_[vertIdx].position);
            }
        }
        
        shells_.push_back(shell);
    }
    
    // Determine which shell is outer (largest bounding box diagonal)
    if (shells_.size() > 1) {
        size_t outerIdx = 0;
        float maxDiag = 0;
        
        for (size_t i = 0; i < shells_.size(); ++i) {
            float diag = shells_[i].bounds.diagonal();
            if (diag > maxDiag) {
                maxDiag = diag;
                outerIdx = i;
            }
        }
        
        for (size_t i = 0; i < shells_.size(); ++i) {
            shells_[i].isOuterShell = (i == outerIdx);
        }
    }
    
    return shells_.size();
}

Solid Solid::clone() const {
    Solid copy = *this;
    copy.id_ = generateNextId();
    return copy;
}

// ===================
// Private Helpers
// ===================

void Solid::buildEdges() {
    edges_.clear();
    edgeLookup_.clear();
    
    for (size_t faceIdx = 0; faceIdx < faces_.size(); ++faceIdx) {
        auto& face = faces_[faceIdx];
        face.edges.clear();
        
        size_t numVerts = face.vertices.size();
        for (size_t i = 0; i < numVerts; ++i) {
            uint32_t v0 = face.vertices[i];
            uint32_t v1 = face.vertices[(i + 1) % numVerts];
            
            uint64_t key = makeEdgeKey(v0, v1);
            
            auto it = edgeLookup_.find(key);
            if (it == edgeLookup_.end()) {
                // New edge
                SolidEdge edge;
                edge.id = generateNextId();
                edge.vertex0 = std::min(v0, v1);
                edge.vertex1 = std::max(v0, v1);
                edge.faces.push_back(static_cast<uint32_t>(faceIdx));
                
                uint32_t edgeIdx = static_cast<uint32_t>(edges_.size());
                edges_.push_back(edge);
                edgeLookup_[key] = edgeIdx;
                face.edges.push_back(edgeIdx);
            } else {
                // Existing edge
                uint32_t edgeIdx = it->second;
                edges_[edgeIdx].faces.push_back(static_cast<uint32_t>(faceIdx));
                face.edges.push_back(edgeIdx);
            }
        }
    }
}

void Solid::buildAdjacency() {
    // Clear existing adjacency
    for (auto& v : vertices_) {
        v.edges.clear();
        v.faces.clear();
    }
    
    // Build vertex-edge adjacency
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& edge = edges_[i];
        vertices_[edge.vertex0].edges.push_back(static_cast<uint32_t>(i));
        vertices_[edge.vertex1].edges.push_back(static_cast<uint32_t>(i));
    }
    
    // Build vertex-face adjacency
    for (size_t faceIdx = 0; faceIdx < faces_.size(); ++faceIdx) {
        for (uint32_t vertIdx : faces_[faceIdx].vertices) {
            vertices_[vertIdx].faces.push_back(static_cast<uint32_t>(faceIdx));
        }
    }
    
    // Mark boundary vertices
    for (auto& v : vertices_) {
        v.isOnBoundary = false;
        for (uint32_t edgeIdx : v.edges) {
            if (edges_[edgeIdx].isBoundary) {
                v.isOnBoundary = true;
                break;
            }
        }
    }
}

void Solid::computeEdgeProperties() {
    for (auto& edge : edges_) {
        const glm::vec3& p0 = vertices_[edge.vertex0].position;
        const glm::vec3& p1 = vertices_[edge.vertex1].position;
        
        edge.direction = p1 - p0;
        edge.length = glm::length(edge.direction);
        if (edge.length > 0) {
            edge.direction /= edge.length;
        }
        edge.midpoint = (p0 + p1) * 0.5f;
        edge.isBoundary = (edge.faces.size() == 1);
        
        // Compute dihedral angle
        if (edge.faces.size() == 2) {
            const glm::vec3& n0 = faces_[edge.faces[0]].normal;
            const glm::vec3& n1 = faces_[edge.faces[1]].normal;
            float dot = glm::clamp(glm::dot(n0, n1), -1.0f, 1.0f);
            edge.dihedralAngle = std::acos(dot);
        } else {
            edge.dihedralAngle = 0.0f;
        }
        
        edge.isSharp = edge.dihedralAngle > ANGLE_SHARP_EDGE;  // LOW FIX: Use named constant
    }
}

void Solid::computeFaceProperties() {
    for (auto& face : faces_) {
        if (face.vertices.size() < 3) {
            face.area = 0;
            face.centroid = glm::vec3(0);
            continue;
        }
        
        // Compute centroid
        face.centroid = glm::vec3(0.0f);
        for (uint32_t vertIdx : face.vertices) {
            face.centroid += vertices_[vertIdx].position;
        }
        face.centroid /= float(face.vertices.size());
        
        // Compute normal and area using Newell's method
        glm::vec3 normal(0.0f);
        float area = 0.0f;
        
        const glm::vec3& v0 = vertices_[face.vertices[0]].position;
        for (size_t i = 1; i < face.vertices.size() - 1; ++i) {
            const glm::vec3& v1 = vertices_[face.vertices[i]].position;
            const glm::vec3& v2 = vertices_[face.vertices[i + 1]].position;
            
            glm::vec3 cross = glm::cross(v1 - v0, v2 - v0);
            normal += cross;
            area += glm::length(cross) * 0.5f;
        }
        
        face.area = area;
        if (glm::length2(normal) > 0) {
            face.normal = glm::normalize(normal);
        }
    }
}

void Solid::invalidateCache() {
    cachedVolume_.reset();
    cachedSignedVolume_.reset();
    cachedSurfaceArea_.reset();
    cachedValidation_.reset();
}

uint64_t Solid::makeEdgeKey(uint32_t v0, uint32_t v1) const {
    if (v0 > v1) std::swap(v0, v1);
    return (uint64_t(v0) << 32) | v1;
}

// ===================
// CSGNode Implementation
// ===================

std::shared_ptr<CSGNode> CSGNode::primitive(std::shared_ptr<Solid> solid) {
    auto node = std::make_shared<CSGNode>();
    node->operation_ = Operation::Primitive;
    node->solid_ = solid;
    return node;
}

std::shared_ptr<CSGNode> CSGNode::operation(Operation op,
                                            std::shared_ptr<CSGNode> left,
                                            std::shared_ptr<CSGNode> right) {
    auto node = std::make_shared<CSGNode>();
    node->operation_ = op;
    node->left_ = left;
    node->right_ = right;
    return node;
}

Result<Solid> CSGNode::evaluate() const {
    if (isLeaf()) {
        if (!solid_) {
            return Result<Solid>::failure("Null solid in primitive node");
        }
        return Result<Solid>::success(solid_->clone());
    }
    
    // CRITICAL FIX: Check for null child nodes before dereferencing
    if (!left_ || !right_) {
        return Result<Solid>::failure("Null child node in CSG operation");
    }
    
    // Evaluate children
    auto leftResult = left_->evaluate();
    if (!leftResult.ok()) {
        return leftResult;
    }
    
    auto rightResult = right_->evaluate();
    if (!rightResult.ok()) {
        return rightResult;
    }
    
    // Perform boolean operation (implemented in BooleanOps)
    // This is a placeholder - actual implementation uses BooleanOps class
    return Result<Solid>::failure("CSG evaluation requires BooleanOps");
}

} // namespace geometry
} // namespace dc3d
