/**
 * @file PrimitiveGenerator.h
 * @brief Generate mesh data for basic primitives (sphere, cube, cylinder, etc.)
 */

#pragma once

#include "MeshData.h"
#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {

/**
 * @brief Static class for generating primitive meshes
 */
class PrimitiveGenerator {
public:
    /**
     * @brief Generate a UV sphere mesh
     * @param center Center position of the sphere
     * @param radius Radius of the sphere
     * @param latSegments Number of latitude segments (rings)
     * @param lonSegments Number of longitude segments (slices)
     * @return MeshData containing the sphere geometry
     */
    static MeshData createSphere(const glm::vec3& center = glm::vec3(0.0f),
                                  float radius = 1.0f,
                                  int latSegments = 24,
                                  int lonSegments = 32);
    
    /**
     * @brief Generate a cube mesh
     * @param center Center position of the cube
     * @param size Edge length of the cube
     * @return MeshData containing the cube geometry
     */
    static MeshData createCube(const glm::vec3& center = glm::vec3(0.0f),
                                float size = 1.0f);
    
    /**
     * @brief Generate a cylinder mesh
     * @param center Center position (middle of height)
     * @param radius Radius of the cylinder
     * @param height Total height of the cylinder
     * @param radialSegments Number of segments around the circumference
     * @param heightSegments Number of segments along the height
     * @param capped Whether to include top and bottom caps
     * @return MeshData containing the cylinder geometry
     */
    static MeshData createCylinder(const glm::vec3& center = glm::vec3(0.0f),
                                    float radius = 0.5f,
                                    float height = 1.0f,
                                    int radialSegments = 32,
                                    int heightSegments = 1,
                                    bool capped = true);
    
    /**
     * @brief Generate a cone mesh
     * @param center Center position (at base)
     * @param radius Radius of the base
     * @param height Height of the cone
     * @param radialSegments Number of segments around the circumference
     * @param capped Whether to include the base cap
     * @return MeshData containing the cone geometry
     */
    static MeshData createCone(const glm::vec3& center = glm::vec3(0.0f),
                                float radius = 0.5f,
                                float height = 1.0f,
                                int radialSegments = 32,
                                bool capped = true);
    
    /**
     * @brief Generate a plane (quad) mesh
     * @param center Center position of the plane
     * @param width Width of the plane (X axis)
     * @param height Height of the plane (Z axis)
     * @param widthSegments Number of segments along width
     * @param heightSegments Number of segments along height
     * @return MeshData containing the plane geometry
     */
    static MeshData createPlane(const glm::vec3& center = glm::vec3(0.0f),
                                 float width = 1.0f,
                                 float height = 1.0f,
                                 int widthSegments = 1,
                                 int heightSegments = 1);
    
    /**
     * @brief Generate a torus mesh
     * @param center Center position of the torus
     * @param majorRadius Distance from center to tube center
     * @param minorRadius Radius of the tube
     * @param majorSegments Number of segments around the torus
     * @param minorSegments Number of segments around the tube
     * @return MeshData containing the torus geometry
     */
    static MeshData createTorus(const glm::vec3& center = glm::vec3(0.0f),
                                 float majorRadius = 0.5f,
                                 float minorRadius = 0.2f,
                                 int majorSegments = 32,
                                 int minorSegments = 16);
};

} // namespace geometry
} // namespace dc3d
