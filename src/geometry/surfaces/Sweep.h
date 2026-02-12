/**
 * @file Sweep.h
 * @brief Sweep operation - profile along a path curve
 * 
 * Supports:
 * - Profile swept along arbitrary path
 * - Twist angle along path
 * - Scale variation along path
 * - Banking/rotation control
 */

#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../NURBSSurface.h"
#include "../MeshData.h"

namespace dc3d {
namespace geometry {

/**
 * @brief How the profile orientation changes along the path
 */
enum class SweepOrientation {
    FrenetSerret,       ///< Use Frenet-Serret frame (natural curve frame)
    ParallelTransport,  ///< Minimize rotation using parallel transport
    Fixed,              ///< Keep profile orientation fixed
    FollowSurface       ///< Orient profile to follow a guide surface
};

/**
 * @brief Options for sweep operation
 */
struct SweepOptions {
    SweepOrientation orientation = SweepOrientation::ParallelTransport;
    
    // Twist
    float twistAngle = 0.0f;            ///< Total twist angle in degrees
    bool twistContinuous = true;         ///< Linear twist vs discrete steps
    
    // Scale
    float startScale = 1.0f;             ///< Scale at start of path
    float endScale = 1.0f;               ///< Scale at end of path
    bool scaleUniform = true;            ///< Uniform vs non-uniform scaling
    
    // Banking
    float banking = 0.0f;                ///< Banking angle at curves (degrees)
    
    // Tessellation
    int pathSegments = 32;               ///< Segments along the path
    int profileSegments = 32;            ///< Segments around the profile
    
    // Caps
    bool capEnds = false;                ///< Cap start and end
    
    // Merge tolerance
    float mergeTolerance = 1e-6f;        ///< Tolerance for merging duplicate vertices
};

/**
 * @brief Result of sweep operation
 */
struct SweepResult {
    bool success = false;
    std::string errorMessage;
    
    std::vector<NURBSSurface> surfaces;  ///< NURBS surfaces
    MeshData mesh;                        ///< Tessellated mesh
    
    // Topology
    std::vector<uint32_t> capStartFaces;
    std::vector<uint32_t> capEndFaces;
};

/**
 * @brief Path curve for sweep
 */
class SweepPath {
public:
    SweepPath() = default;
    
    /**
     * @brief Create path from points
     */
    explicit SweepPath(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Set path points
     */
    void setPoints(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Get path points
     */
    const std::vector<glm::vec3>& points() const { return m_points; }
    
    /**
     * @brief Check if path is closed
     */
    bool isClosed() const { return m_closed; }
    
    /**
     * @brief Set whether path is closed
     */
    void setClosed(bool closed) { m_closed = closed; }
    
    /**
     * @brief Evaluate position on path at parameter t [0,1]
     */
    glm::vec3 evaluate(float t) const;
    
    /**
     * @brief Get tangent at parameter t
     */
    glm::vec3 tangent(float t) const;
    
    /**
     * @brief Get curvature at parameter t
     */
    float curvature(float t) const;
    
    /**
     * @brief Get path length
     */
    float length() const;
    
    /**
     * @brief Check if path is valid
     */
    bool isValid() const { return m_points.size() >= 2; }
    
    /**
     * @brief Get arc-length parameterized points
     */
    std::vector<glm::vec3> resampleByArcLength(int numPoints) const;

private:
    std::vector<glm::vec3> m_points;
    bool m_closed = false;
    
    // Cached arc lengths for parameterization
    mutable std::vector<float> m_arcLengths;
    mutable bool m_arcLengthsDirty = true;
    
    void updateArcLengths() const;
    float arcLengthToT(float s) const;
};

/**
 * @brief Profile for sweep (cross-section)
 */
class SweepProfile {
public:
    SweepProfile() = default;
    
    /**
     * @brief Create profile from points
     * @param points Profile points (assumed to be in XY plane centered at origin)
     * @param closed Whether profile is closed
     */
    explicit SweepProfile(const std::vector<glm::vec3>& points, bool closed = true);
    
    /**
     * @brief Create circular profile
     */
    static SweepProfile circle(float radius, int segments = 32);
    
    /**
     * @brief Create rectangular profile
     */
    static SweepProfile rectangle(float width, float height);
    
    /**
     * @brief Create elliptical profile
     */
    static SweepProfile ellipse(float radiusX, float radiusY, int segments = 32);
    
    /**
     * @brief Set profile points
     */
    void setPoints(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Get profile points
     */
    const std::vector<glm::vec3>& points() const { return m_points; }
    
    /**
     * @brief Check if profile is closed
     */
    bool isClosed() const { return m_closed; }
    
    /**
     * @brief Set closed
     */
    void setClosed(bool closed) { m_closed = closed; }
    
    /**
     * @brief Check if valid
     */
    bool isValid() const { return m_points.size() >= 2; }
    
    /**
     * @brief Get centroid
     */
    glm::vec3 centroid() const;
    
    /**
     * @brief Transform profile
     */
    SweepProfile transformed(const glm::mat4& matrix) const;

private:
    std::vector<glm::vec3> m_points;
    bool m_closed = true;
};

/**
 * @brief Sweep operations
 */
class Sweep {
public:
    Sweep() = default;
    
    /**
     * @brief Sweep a profile along a path
     * @param profile The cross-section profile
     * @param path The path to sweep along
     * @param options Sweep options
     * @return Sweep result with mesh
     */
    static SweepResult sweep(const SweepProfile& profile,
                              const SweepPath& path,
                              const SweepOptions& options = SweepOptions());
    
    /**
     * @brief Sweep with twist
     */
    static SweepResult sweepWithTwist(const SweepProfile& profile,
                                       const SweepPath& path,
                                       float twistAngle,
                                       const SweepOptions& options = SweepOptions());
    
    /**
     * @brief Sweep with scale variation
     */
    static SweepResult sweepWithScale(const SweepProfile& profile,
                                       const SweepPath& path,
                                       float startScale,
                                       float endScale,
                                       const SweepOptions& options = SweepOptions());
    
    /**
     * @brief Sweep from point arrays
     */
    static SweepResult sweep(const std::vector<glm::vec3>& profilePoints,
                              const std::vector<glm::vec3>& pathPoints,
                              bool closedProfile = true,
                              const SweepOptions& options = SweepOptions());
    
    /**
     * @brief Create NURBS surfaces for the sweep
     */
    static std::vector<NURBSSurface> createSurfaces(const SweepProfile& profile,
                                                     const SweepPath& path,
                                                     const SweepOptions& options);

private:
    // Frame computation methods
    struct Frame {
        glm::vec3 position;
        glm::vec3 tangent;
        glm::vec3 normal;
        glm::vec3 binormal;
    };
    
    static Frame computeFrenetFrame(const SweepPath& path, float t);
    static std::vector<Frame> computeParallelTransportFrames(const SweepPath& path, int numFrames);
    static std::vector<Frame> computeFixedFrames(const SweepPath& path, int numFrames, 
                                                   const glm::vec3& upVector = glm::vec3(0, 1, 0));
    
    // Transform profile using frame
    static std::vector<glm::vec3> transformProfile(const SweepProfile& profile,
                                                    const Frame& frame,
                                                    float scale = 1.0f,
                                                    float twist = 0.0f);
    
    // Create mesh from frames
    static MeshData createMesh(const SweepProfile& profile,
                                const std::vector<Frame>& frames,
                                const SweepOptions& options);
};

} // namespace geometry
} // namespace dc3d
