/**
 * @file PrimitiveGenerator.cpp
 * @brief Implementation of primitive mesh generation
 */

#include "PrimitiveGenerator.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace dc3d {
namespace geometry {

MeshData PrimitiveGenerator::createSphere(const glm::vec3& center,
                                           float radius,
                                           int latSegments,
                                           int lonSegments) {
    MeshData mesh;
    
    // Reserve space
    int vertexCount = (latSegments + 1) * (lonSegments + 1);
    int faceCount = latSegments * lonSegments * 2;
    mesh.reserveVertices(vertexCount);
    mesh.reserveFaces(faceCount);
    
    // Generate vertices
    for (int lat = 0; lat <= latSegments; ++lat) {
        float theta = lat * glm::pi<float>() / latSegments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        
        for (int lon = 0; lon <= lonSegments; ++lon) {
            float phi = lon * 2.0f * glm::pi<float>() / lonSegments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            
            glm::vec3 normal(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            glm::vec3 position = center + radius * normal;
            
            mesh.addVertex(position, normal);
        }
    }
    
    // Generate faces
    for (int lat = 0; lat < latSegments; ++lat) {
        for (int lon = 0; lon < lonSegments; ++lon) {
            int current = lat * (lonSegments + 1) + lon;
            int next = current + lonSegments + 1;
            
            // Two triangles per quad
            mesh.addFace(current, next, current + 1);
            mesh.addFace(current + 1, next, next + 1);
        }
    }
    
    return mesh;
}

MeshData PrimitiveGenerator::createCube(const glm::vec3& center, float size) {
    MeshData mesh;
    
    float half = size * 0.5f;
    
    // 8 corners of the cube
    glm::vec3 corners[8] = {
        center + glm::vec3(-half, -half, -half),  // 0
        center + glm::vec3( half, -half, -half),  // 1
        center + glm::vec3( half,  half, -half),  // 2
        center + glm::vec3(-half,  half, -half),  // 3
        center + glm::vec3(-half, -half,  half),  // 4
        center + glm::vec3( half, -half,  half),  // 5
        center + glm::vec3( half,  half,  half),  // 6
        center + glm::vec3(-half,  half,  half)   // 7
    };
    
    // 6 face normals
    glm::vec3 normals[6] = {
        glm::vec3( 0,  0, -1),  // back
        glm::vec3( 0,  0,  1),  // front
        glm::vec3(-1,  0,  0),  // left
        glm::vec3( 1,  0,  0),  // right
        glm::vec3( 0, -1,  0),  // bottom
        glm::vec3( 0,  1,  0)   // top
    };
    
    // Face vertex indices (4 vertices per face)
    int faceIndices[6][4] = {
        {0, 3, 2, 1},  // back
        {4, 5, 6, 7},  // front
        {0, 4, 7, 3},  // left
        {1, 2, 6, 5},  // right
        {0, 1, 5, 4},  // bottom
        {3, 7, 6, 2}   // top
    };
    
    // Create 4 vertices per face (24 total) for proper normals
    for (int face = 0; face < 6; ++face) {
        uint32_t v0 = mesh.addVertex(corners[faceIndices[face][0]], normals[face]);
        uint32_t v1 = mesh.addVertex(corners[faceIndices[face][1]], normals[face]);
        uint32_t v2 = mesh.addVertex(corners[faceIndices[face][2]], normals[face]);
        uint32_t v3 = mesh.addVertex(corners[faceIndices[face][3]], normals[face]);
        
        // Two triangles per face
        mesh.addFace(v0, v1, v2);
        mesh.addFace(v0, v2, v3);
    }
    
    return mesh;
}

MeshData PrimitiveGenerator::createCylinder(const glm::vec3& center,
                                             float radius,
                                             float height,
                                             int radialSegments,
                                             int heightSegments,
                                             bool capped) {
    MeshData mesh;
    
    float halfHeight = height * 0.5f;
    
    // Generate side vertices
    for (int h = 0; h <= heightSegments; ++h) {
        float y = -halfHeight + (height * h / heightSegments);
        
        for (int r = 0; r <= radialSegments; ++r) {
            float angle = r * 2.0f * glm::pi<float>() / radialSegments;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            
            glm::vec3 position = center + glm::vec3(x, y, z);
            glm::vec3 normal = glm::normalize(glm::vec3(x, 0, z));
            
            mesh.addVertex(position, normal);
        }
    }
    
    // Generate side faces
    for (int h = 0; h < heightSegments; ++h) {
        for (int r = 0; r < radialSegments; ++r) {
            int current = h * (radialSegments + 1) + r;
            int next = current + radialSegments + 1;
            
            mesh.addFace(current, next, current + 1);
            mesh.addFace(current + 1, next, next + 1);
        }
    }
    
    // Generate caps
    if (capped) {
        // Top cap
        uint32_t topCenter = mesh.addVertex(center + glm::vec3(0, halfHeight, 0), glm::vec3(0, 1, 0));
        uint32_t topRingStart = mesh.vertexCount();
        
        for (int r = 0; r <= radialSegments; ++r) {
            float angle = r * 2.0f * glm::pi<float>() / radialSegments;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            
            mesh.addVertex(center + glm::vec3(x, halfHeight, z), glm::vec3(0, 1, 0));
        }
        
        for (int r = 0; r < radialSegments; ++r) {
            mesh.addFace(topCenter, topRingStart + r, topRingStart + r + 1);
        }
        
        // Bottom cap
        uint32_t bottomCenter = mesh.addVertex(center + glm::vec3(0, -halfHeight, 0), glm::vec3(0, -1, 0));
        uint32_t bottomRingStart = mesh.vertexCount();
        
        for (int r = 0; r <= radialSegments; ++r) {
            float angle = r * 2.0f * glm::pi<float>() / radialSegments;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            
            mesh.addVertex(center + glm::vec3(x, -halfHeight, z), glm::vec3(0, -1, 0));
        }
        
        for (int r = 0; r < radialSegments; ++r) {
            mesh.addFace(bottomCenter, bottomRingStart + r + 1, bottomRingStart + r);
        }
    }
    
    return mesh;
}

MeshData PrimitiveGenerator::createCone(const glm::vec3& center,
                                         float radius,
                                         float height,
                                         int radialSegments,
                                         bool capped) {
    MeshData mesh;
    
    // Apex at top
    glm::vec3 apex = center + glm::vec3(0, height, 0);
    
    // Generate side vertices (ring at base)
    uint32_t apexIdx = mesh.addVertex(apex, glm::vec3(0, 1, 0));  // Normal will be recomputed
    
    std::vector<uint32_t> baseRing;
    for (int r = 0; r <= radialSegments; ++r) {
        float angle = r * 2.0f * glm::pi<float>() / radialSegments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        
        glm::vec3 position = center + glm::vec3(x, 0, z);
        
        // Compute side normal (tangent to cone surface)
        float slopeAngle = std::atan2(radius, height);
        glm::vec3 radialDir = glm::normalize(glm::vec3(x, 0, z));
        glm::vec3 normal = glm::vec3(radialDir.x * std::cos(slopeAngle),
                                      std::sin(slopeAngle),
                                      radialDir.z * std::cos(slopeAngle));
        
        baseRing.push_back(mesh.addVertex(position, normal));
    }
    
    // Generate side faces
    for (int r = 0; r < radialSegments; ++r) {
        mesh.addFace(apexIdx, baseRing[r + 1], baseRing[r]);
    }
    
    // Generate base cap
    if (capped) {
        uint32_t baseCenter = mesh.addVertex(center, glm::vec3(0, -1, 0));
        uint32_t capRingStart = mesh.vertexCount();
        
        for (int r = 0; r <= radialSegments; ++r) {
            float angle = r * 2.0f * glm::pi<float>() / radialSegments;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            
            mesh.addVertex(center + glm::vec3(x, 0, z), glm::vec3(0, -1, 0));
        }
        
        for (int r = 0; r < radialSegments; ++r) {
            mesh.addFace(baseCenter, capRingStart + r, capRingStart + r + 1);
        }
    }
    
    // Recompute normals for better quality
    mesh.computeNormals();
    
    return mesh;
}

MeshData PrimitiveGenerator::createPlane(const glm::vec3& center,
                                          float width,
                                          float height,
                                          int widthSegments,
                                          int heightSegments) {
    MeshData mesh;
    
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    
    // Generate vertices
    for (int h = 0; h <= heightSegments; ++h) {
        float z = -halfHeight + (height * h / heightSegments);
        
        for (int w = 0; w <= widthSegments; ++w) {
            float x = -halfWidth + (width * w / widthSegments);
            
            glm::vec3 position = center + glm::vec3(x, 0, z);
            glm::vec3 normal(0, 1, 0);  // Face up
            
            mesh.addVertex(position, normal);
        }
    }
    
    // Generate faces
    for (int h = 0; h < heightSegments; ++h) {
        for (int w = 0; w < widthSegments; ++w) {
            int current = h * (widthSegments + 1) + w;
            int next = current + widthSegments + 1;
            
            mesh.addFace(current, next, current + 1);
            mesh.addFace(current + 1, next, next + 1);
        }
    }
    
    return mesh;
}

MeshData PrimitiveGenerator::createTorus(const glm::vec3& center,
                                          float majorRadius,
                                          float minorRadius,
                                          int majorSegments,
                                          int minorSegments) {
    MeshData mesh;
    
    // Generate vertices
    for (int major = 0; major <= majorSegments; ++major) {
        float u = major * 2.0f * glm::pi<float>() / majorSegments;
        float cosU = std::cos(u);
        float sinU = std::sin(u);
        
        // Center of tube at this major segment
        glm::vec3 tubeCenter = glm::vec3(cosU * majorRadius, 0, sinU * majorRadius);
        
        for (int minor = 0; minor <= minorSegments; ++minor) {
            float v = minor * 2.0f * glm::pi<float>() / minorSegments;
            float cosV = std::cos(v);
            float sinV = std::sin(v);
            
            // Position on tube surface
            glm::vec3 localNormal = glm::vec3(cosU * cosV, sinV, sinU * cosV);
            glm::vec3 position = center + tubeCenter + minorRadius * localNormal;
            
            mesh.addVertex(position, localNormal);
        }
    }
    
    // Generate faces
    for (int major = 0; major < majorSegments; ++major) {
        for (int minor = 0; minor < minorSegments; ++minor) {
            int current = major * (minorSegments + 1) + minor;
            int next = current + minorSegments + 1;
            
            mesh.addFace(current, next, current + 1);
            mesh.addFace(current + 1, next, next + 1);
        }
    }
    
    return mesh;
}

} // namespace geometry
} // namespace dc3d
