/**
 * @file Pipe.h
 * @brief Pipe/tube creation along a path curve
 * 
 * Specialized sweep operation for creating tubular surfaces.
 * Supports:
 * - Constant or variable radius
 * - Circular or custom cross-sections
 * - Cap ends
 */

#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../NURBSSurface.h"
#include "../MeshData.h"
#include "Sweep.h"

namespace dc3d {
namespace geometry {

/**
 * @brief Options for pipe creation
 */
struct PipeOptions {
    float radius = 1.0f;                 ///< Pipe radius (for constant radius)
    int circumferentialSegments = 32;    ///< Segments around the pipe
    int pathSegments = 32;               ///< Segments along the path
    bool capEnds = true;                 ///< Create end caps
    
    // Variable radius
    bool variableRadius = false;         ///< Use variable radius
    std::vector<float> radii;            ///< Radii at each path point
    
    // Cross-section
    bool customProfile = false;          ///< Use custom profile instead of circle
};

/**
 * @brief Result of pipe creation
 */
struct PipeResult {
    bool success = false;
    std::string errorMessage;
    
    MeshData mesh;                        ///< Tessellated mesh
    std::vector<NURBSSurface> surfaces;   ///< NURBS representation
    
    // Topology
    std::vector<uint32_t> capStartFaces;
    std::vector<uint32_t> capEndFaces;
    std::vector<uint32_t> lateralFaces;
};

/**
 * @brief Pipe/tube creation operations
 */
class Pipe {
public:
    Pipe() = default;
    
    /**
     * @brief Create a pipe along a path with constant radius
     * @param path Path points
     * @param radius Pipe radius
     * @param options Pipe options
     * @return Pipe result with mesh
     */
    static PipeResult pipe(const std::vector<glm::vec3>& path,
                           float radius,
                           const PipeOptions& options = PipeOptions());
    
    /**
     * @brief Create a pipe with variable radius
     * @param path Path points
     * @param radii Radius at each path point (interpolated between)
     * @param options Pipe options
     */
    static PipeResult pipeVariable(const std::vector<glm::vec3>& path,
                                    const std::vector<float>& radii,
                                    const PipeOptions& options = PipeOptions());
    
    /**
     * @brief Create a pipe from SweepPath
     */
    static PipeResult pipe(const SweepPath& path,
                           float radius,
                           const PipeOptions& options = PipeOptions());
    
    /**
     * @brief Create a pipe with custom profile
     * @param path Path points
     * @param profile Custom cross-section profile
     * @param options Pipe options
     */
    static PipeResult pipeWithProfile(const std::vector<glm::vec3>& path,
                                       const SweepProfile& profile,
                                       const PipeOptions& options = PipeOptions());
    
    /**
     * @brief Create a torus (closed pipe)
     * @param center Torus center
     * @param majorRadius Distance from center to tube center
     * @param minorRadius Tube radius
     * @param majorSegments Segments around the major circle
     * @param minorSegments Segments around the tube
     */
    static MeshData torus(const glm::vec3& center,
                          float majorRadius,
                          float minorRadius,
                          int majorSegments = 32,
                          int minorSegments = 16);
    
    /**
     * @brief Create a helix pipe
     * @param center Center of helix
     * @param radius Helix radius
     * @param pitch Distance per revolution
     * @param revolutions Number of revolutions
     * @param tubeRadius Radius of the tube
     * @param segments Segments per revolution
     */
    static MeshData helix(const glm::vec3& center,
                          float radius,
                          float pitch,
                          float revolutions,
                          float tubeRadius,
                          int segments = 32);
    
    /**
     * @brief Create a spring (helix with flat ends)
     */
    static MeshData spring(const glm::vec3& center,
                           float radius,
                           float pitch,
                           int activeCoils,
                           float tubeRadius,
                           int segments = 32);
    
    /**
     * @brief Create NURBS surface representation of pipe
     */
    static std::vector<NURBSSurface> createSurfaces(const std::vector<glm::vec3>& path,
                                                     float radius,
                                                     const PipeOptions& options);

private:
    // Helper to interpolate radius along path
    static float interpolateRadius(const std::vector<float>& radii, float t);
    
    // Create cap mesh
    static MeshData createCap(const glm::vec3& center,
                               const glm::vec3& normal,
                               float radius,
                               int segments);
};

} // namespace geometry
} // namespace dc3d
