/**
 * @file Pipe.cpp
 * @brief Pipe/tube creation implementation
 */

#include "Pipe.h"
#include <cmath>
#include <algorithm>

namespace dc3d {
namespace geometry {

float Pipe::interpolateRadius(const std::vector<float>& radii, float t) {
    if (radii.empty()) return 1.0f;
    if (radii.size() == 1) return radii[0];
    
    t = std::clamp(t, 0.0f, 1.0f);
    
    float fIdx = t * (radii.size() - 1);
    int idx = static_cast<int>(fIdx);
    float frac = fIdx - idx;
    
    if (idx >= static_cast<int>(radii.size()) - 1) {
        return radii.back();
    }
    
    return radii[idx] * (1.0f - frac) + radii[idx + 1] * frac;
}

MeshData Pipe::createCap(const glm::vec3& center,
                          const glm::vec3& normal,
                          float radius,
                          int segments) {
    MeshData mesh;
    
    // Find perpendicular vectors
    glm::vec3 up(0, 1, 0);
    if (std::abs(glm::dot(normal, up)) > 0.9f) {
        up = glm::vec3(1, 0, 0);
    }
    glm::vec3 tangent1 = glm::normalize(glm::cross(normal, up));
    glm::vec3 tangent2 = glm::cross(normal, tangent1);
    
    // Center vertex
    uint32_t centerIdx = mesh.addVertex(center);
    
    // Edge vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        glm::vec3 p = center + radius * (std::cos(angle) * tangent1 + std::sin(angle) * tangent2);
        mesh.addVertex(p);
    }
    
    // Triangles
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        mesh.addFace(centerIdx, i + 1, next + 1);
    }
    
    return mesh;
}

PipeResult Pipe::pipe(const std::vector<glm::vec3>& path,
                      float radius,
                      const PipeOptions& options) {
    PipeResult result;
    
    if (path.size() < 2) {
        result.errorMessage = "Path must have at least 2 points";
        return result;
    }
    
    if (radius <= 0) {
        result.errorMessage = "Radius must be positive";
        return result;
    }
    
    // Use Sweep with circular profile
    SweepProfile profile = SweepProfile::circle(radius, options.circumferentialSegments);
    SweepPath sweepPath(path);
    
    SweepOptions sweepOpts;
    sweepOpts.pathSegments = options.pathSegments;
    sweepOpts.profileSegments = options.circumferentialSegments;
    sweepOpts.capEnds = false;  // We'll add our own caps
    sweepOpts.orientation = SweepOrientation::ParallelTransport;
    
    SweepResult sweepResult = Sweep::sweep(profile, sweepPath, sweepOpts);
    
    if (!sweepResult.success) {
        result.errorMessage = sweepResult.errorMessage;
        return result;
    }
    
    result.mesh = sweepResult.mesh;
    
    // Add caps if requested
    if (options.capEnds) {
        SweepPath sp(path);
        
        // Start cap
        glm::vec3 startCenter = sp.evaluate(0.0f);
        glm::vec3 startNormal = -sp.tangent(0.0f);  // Point outward
        MeshData startCap = createCap(startCenter, startNormal, radius, options.circumferentialSegments);
        
        uint32_t startBase = static_cast<uint32_t>(result.mesh.vertexCount());
        for (size_t i = 0; i < startCap.vertexCount(); ++i) {
            result.mesh.addVertex(startCap.vertices()[i]);
        }
        for (size_t i = 0; i < startCap.faceCount(); ++i) {
            uint32_t i0 = startCap.indices()[i * 3];
            uint32_t i1 = startCap.indices()[i * 3 + 1];
            uint32_t i2 = startCap.indices()[i * 3 + 2];
            result.mesh.addFace(startBase + i0, startBase + i2, startBase + i1);  // Flip winding
            result.capStartFaces.push_back(static_cast<uint32_t>(result.mesh.faceCount() - 1));
        }
        
        // End cap
        glm::vec3 endCenter = sp.evaluate(1.0f);
        glm::vec3 endNormal = sp.tangent(1.0f);  // Point outward
        MeshData endCap = createCap(endCenter, endNormal, radius, options.circumferentialSegments);
        
        uint32_t endBase = static_cast<uint32_t>(result.mesh.vertexCount());
        for (size_t i = 0; i < endCap.vertexCount(); ++i) {
            result.mesh.addVertex(endCap.vertices()[i]);
        }
        for (size_t i = 0; i < endCap.faceCount(); ++i) {
            uint32_t i0 = endCap.indices()[i * 3];
            uint32_t i1 = endCap.indices()[i * 3 + 1];
            uint32_t i2 = endCap.indices()[i * 3 + 2];
            result.mesh.addFace(endBase + i0, endBase + i1, endBase + i2);
            result.capEndFaces.push_back(static_cast<uint32_t>(result.mesh.faceCount() - 1));
        }
    }
    
    result.mesh.computeNormals();
    result.success = true;
    return result;
}

PipeResult Pipe::pipeVariable(const std::vector<glm::vec3>& path,
                               const std::vector<float>& radii,
                               const PipeOptions& options) {
    PipeResult result;
    
    if (path.size() < 2) {
        result.errorMessage = "Path must have at least 2 points";
        return result;
    }
    
    if (radii.empty()) {
        result.errorMessage = "Radii array must not be empty";
        return result;
    }
    
    // Create path with proper parameterization
    SweepPath sweepPath(path);
    
    // Create mesh manually with variable radius
    MeshData& mesh = result.mesh;
    
    int numPathSegs = options.pathSegments;
    int numCircSegs = options.circumferentialSegments;
    
    // Compute frames using parallel transport
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> binormals;
    
    // First frame
    glm::vec3 t0 = sweepPath.tangent(0.0f);
    glm::vec3 up(0, 1, 0);
    if (std::abs(glm::dot(t0, up)) > 0.9f) up = glm::vec3(1, 0, 0);
    glm::vec3 n0 = glm::normalize(glm::cross(t0, up));
    glm::vec3 b0 = glm::cross(t0, n0);
    
    positions.push_back(sweepPath.evaluate(0.0f));
    tangents.push_back(t0);
    normals.push_back(n0);
    binormals.push_back(b0);
    
    // Propagate frames
    for (int i = 1; i <= numPathSegs; ++i) {
        float t = static_cast<float>(i) / numPathSegs;
        
        positions.push_back(sweepPath.evaluate(t));
        tangents.push_back(sweepPath.tangent(t));
        
        // Parallel transport
        glm::vec3 axis = glm::cross(tangents[i-1], tangents[i]);
        float axisLen = glm::length(axis);
        
        if (axisLen > 1e-10f) {
            axis /= axisLen;
            float angle = std::acos(std::clamp(glm::dot(tangents[i-1], tangents[i]), -1.0f, 1.0f));
            
            // Rodrigues rotation
            glm::vec3 n = normals[i-1] * std::cos(angle) +
                         glm::cross(axis, normals[i-1]) * std::sin(angle) +
                         axis * glm::dot(axis, normals[i-1]) * (1.0f - std::cos(angle));
            normals.push_back(glm::normalize(n));
        } else {
            normals.push_back(normals[i-1]);
        }
        
        binormals.push_back(glm::cross(tangents[i], normals[i]));
    }
    
    // Generate vertices
    for (int i = 0; i <= numPathSegs; ++i) {
        float pathT = static_cast<float>(i) / numPathSegs;
        float r = interpolateRadius(radii, pathT);
        
        for (int j = 0; j < numCircSegs; ++j) {
            float circAngle = 2.0f * 3.14159265359f * j / numCircSegs;
            
            glm::vec3 offset = r * (std::cos(circAngle) * normals[i] + 
                                    std::sin(circAngle) * binormals[i]);
            mesh.addVertex(positions[i] + offset);
        }
    }
    
    // Generate faces
    for (int i = 0; i < numPathSegs; ++i) {
        for (int j = 0; j < numCircSegs; ++j) {
            int nextJ = (j + 1) % numCircSegs;
            
            uint32_t v00 = i * numCircSegs + j;
            uint32_t v10 = i * numCircSegs + nextJ;
            uint32_t v01 = (i + 1) * numCircSegs + j;
            uint32_t v11 = (i + 1) * numCircSegs + nextJ;
            
            mesh.addFace(v00, v10, v11);
            mesh.addFace(v00, v11, v01);
        }
    }
    
    // Add caps if requested
    if (options.capEnds) {
        float startRadius = interpolateRadius(radii, 0.0f);
        float endRadius = interpolateRadius(radii, 1.0f);
        
        MeshData startCap = createCap(positions.front(), -tangents.front(), 
                                       startRadius, numCircSegs);
        
        uint32_t startBase = static_cast<uint32_t>(mesh.vertexCount());
        for (size_t i = 0; i < startCap.vertexCount(); ++i) {
            mesh.addVertex(startCap.vertices()[i]);
        }
        for (size_t i = 0; i < startCap.faceCount(); ++i) {
            mesh.addFace(startBase + startCap.indices()[i * 3],
                        startBase + startCap.indices()[i * 3 + 2],
                        startBase + startCap.indices()[i * 3 + 1]);
            result.capStartFaces.push_back(static_cast<uint32_t>(mesh.faceCount() - 1));
        }
        
        MeshData endCap = createCap(positions.back(), tangents.back(), 
                                     endRadius, numCircSegs);
        
        uint32_t endBase = static_cast<uint32_t>(mesh.vertexCount());
        for (size_t i = 0; i < endCap.vertexCount(); ++i) {
            mesh.addVertex(endCap.vertices()[i]);
        }
        for (size_t i = 0; i < endCap.faceCount(); ++i) {
            mesh.addFace(endBase + endCap.indices()[i * 3],
                        endBase + endCap.indices()[i * 3 + 1],
                        endBase + endCap.indices()[i * 3 + 2]);
            result.capEndFaces.push_back(static_cast<uint32_t>(mesh.faceCount() - 1));
        }
    }
    
    mesh.computeNormals();
    result.success = true;
    return result;
}

PipeResult Pipe::pipe(const SweepPath& path,
                      float radius,
                      const PipeOptions& options) {
    return pipe(path.points(), radius, options);
}

PipeResult Pipe::pipeWithProfile(const std::vector<glm::vec3>& path,
                                  const SweepProfile& profile,
                                  const PipeOptions& options) {
    SweepPath sweepPath(path);
    
    SweepOptions sweepOpts;
    sweepOpts.pathSegments = options.pathSegments;
    sweepOpts.profileSegments = options.circumferentialSegments;
    sweepOpts.capEnds = options.capEnds;
    
    SweepResult sweepResult = Sweep::sweep(profile, sweepPath, sweepOpts);
    
    PipeResult result;
    result.success = sweepResult.success;
    result.errorMessage = sweepResult.errorMessage;
    result.mesh = sweepResult.mesh;
    
    return result;
}

MeshData Pipe::torus(const glm::vec3& center,
                     float majorRadius,
                     float minorRadius,
                     int majorSegments,
                     int minorSegments) {
    MeshData mesh;
    
    // Generate vertices
    for (int i = 0; i < majorSegments; ++i) {
        float majorAngle = 2.0f * 3.14159265359f * i / majorSegments;
        
        // Center of tube at this point
        glm::vec3 tubeCenter = center + majorRadius * glm::vec3(std::cos(majorAngle), 0, std::sin(majorAngle));
        
        // Direction from center to tube center
        glm::vec3 radialDir = glm::normalize(tubeCenter - center);
        
        for (int j = 0; j < minorSegments; ++j) {
            float minorAngle = 2.0f * 3.14159265359f * j / minorSegments;
            
            // Point on tube surface
            glm::vec3 p = tubeCenter + 
                         minorRadius * std::cos(minorAngle) * radialDir +
                         minorRadius * std::sin(minorAngle) * glm::vec3(0, 1, 0);
            
            mesh.addVertex(p);
        }
    }
    
    // Generate faces
    for (int i = 0; i < majorSegments; ++i) {
        int nextI = (i + 1) % majorSegments;
        
        for (int j = 0; j < minorSegments; ++j) {
            int nextJ = (j + 1) % minorSegments;
            
            uint32_t v00 = i * minorSegments + j;
            uint32_t v10 = i * minorSegments + nextJ;
            uint32_t v01 = nextI * minorSegments + j;
            uint32_t v11 = nextI * minorSegments + nextJ;
            
            mesh.addFace(v00, v10, v11);
            mesh.addFace(v00, v11, v01);
        }
    }
    
    mesh.computeNormals();
    return mesh;
}

MeshData Pipe::helix(const glm::vec3& center,
                     float radius,
                     float pitch,
                     float revolutions,
                     float tubeRadius,
                     int segments) {
    // Create helix path
    std::vector<glm::vec3> path;
    
    int totalSegments = static_cast<int>(segments * revolutions);
    for (int i = 0; i <= totalSegments; ++i) {
        float t = static_cast<float>(i) / segments;  // t in revolutions
        float angle = 2.0f * 3.14159265359f * t;
        float height = pitch * t;
        
        path.push_back(center + glm::vec3(
            radius * std::cos(angle),
            height,
            radius * std::sin(angle)
        ));
    }
    
    PipeOptions options;
    options.pathSegments = totalSegments;
    options.circumferentialSegments = segments / 2;
    options.capEnds = true;
    
    return pipe(path, tubeRadius, options).mesh;
}

MeshData Pipe::spring(const glm::vec3& center,
                      float radius,
                      float pitch,
                      int activeCoils,
                      float tubeRadius,
                      int segments) {
    // Create spring path with flat ends
    std::vector<glm::vec3> path;
    
    // Lead-in (1/2 coil flat)
    float flatAngle = 3.14159265359f;
    int flatSegments = segments / 2;
    
    for (int i = 0; i <= flatSegments; ++i) {
        float t = static_cast<float>(i) / flatSegments;
        float angle = t * flatAngle;
        
        path.push_back(center + glm::vec3(
            radius * std::cos(angle),
            0,
            radius * std::sin(angle)
        ));
    }
    
    // Active coils
    int activeSegments = activeCoils * segments;
    for (int i = 1; i <= activeSegments; ++i) {
        float t = static_cast<float>(i) / segments;
        float angle = flatAngle + 2.0f * 3.14159265359f * t;
        float height = pitch * t;
        
        path.push_back(center + glm::vec3(
            radius * std::cos(angle),
            height,
            radius * std::sin(angle)
        ));
    }
    
    // Lead-out (1/2 coil flat)
    float endHeight = pitch * activeCoils;
    float startAngle = flatAngle + 2.0f * 3.14159265359f * activeCoils;
    
    for (int i = 1; i <= flatSegments; ++i) {
        float t = static_cast<float>(i) / flatSegments;
        float angle = startAngle + t * flatAngle;
        
        path.push_back(center + glm::vec3(
            radius * std::cos(angle),
            endHeight,
            radius * std::sin(angle)
        ));
    }
    
    PipeOptions options;
    options.pathSegments = static_cast<int>(path.size()) - 1;
    options.circumferentialSegments = segments / 2;
    options.capEnds = true;
    
    return pipe(path, tubeRadius, options).mesh;
}

std::vector<NURBSSurface> Pipe::createSurfaces(const std::vector<glm::vec3>& path,
                                                float radius,
                                                const PipeOptions& options) {
    SweepProfile profile = SweepProfile::circle(radius, options.circumferentialSegments);
    SweepPath sweepPath(path);
    
    SweepOptions sweepOpts;
    sweepOpts.pathSegments = options.pathSegments;
    
    return Sweep::createSurfaces(profile, sweepPath, sweepOpts);
}

} // namespace geometry
} // namespace dc3d
