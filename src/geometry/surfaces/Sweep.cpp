/**
 * @file Sweep.cpp
 * @brief Sweep operation implementation
 */

#include "Sweep.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace dc3d {
namespace geometry {

// ============================================================================
// SweepPath Implementation
// ============================================================================

SweepPath::SweepPath(const std::vector<glm::vec3>& points)
    : m_points(points) {
}

void SweepPath::setPoints(const std::vector<glm::vec3>& points) {
    m_points = points;
    m_arcLengthsDirty = true;
}

void SweepPath::updateArcLengths() const {
    if (!m_arcLengthsDirty) return;
    
    m_arcLengths.clear();
    m_arcLengths.push_back(0.0f);
    
    for (size_t i = 1; i < m_points.size(); ++i) {
        float segLen = glm::length(m_points[i] - m_points[i-1]);
        m_arcLengths.push_back(m_arcLengths.back() + segLen);
    }
    
    if (m_closed && m_points.size() >= 2) {
        float segLen = glm::length(m_points.front() - m_points.back());
        m_arcLengths.push_back(m_arcLengths.back() + segLen);
    }
    
    m_arcLengthsDirty = false;
}

float SweepPath::length() const {
    updateArcLengths();
    return m_arcLengths.empty() ? 0.0f : m_arcLengths.back();
}

float SweepPath::arcLengthToT(float s) const {
    updateArcLengths();
    
    if (m_arcLengths.empty() || m_arcLengths.back() < 1e-10f) return 0.0f;
    
    s = std::clamp(s, 0.0f, m_arcLengths.back());
    
    // Binary search for segment
    auto it = std::lower_bound(m_arcLengths.begin(), m_arcLengths.end(), s);
    if (it == m_arcLengths.begin()) return 0.0f;
    
    size_t idx = std::distance(m_arcLengths.begin(), it) - 1;
    float localS = s - m_arcLengths[idx];
    float segLen = m_arcLengths[idx + 1] - m_arcLengths[idx];
    float localT = (segLen > 1e-10f) ? localS / segLen : 0.0f;
    
    return (idx + localT) / (m_points.size() - 1);
}

glm::vec3 SweepPath::evaluate(float t) const {
    if (m_points.empty()) return glm::vec3(0.0f);
    if (m_points.size() == 1) return m_points[0];
    
    t = std::clamp(t, 0.0f, 1.0f);
    
    float fIdx = t * (m_points.size() - 1);
    int idx = static_cast<int>(fIdx);
    float frac = fIdx - idx;
    
    if (idx >= static_cast<int>(m_points.size()) - 1) {
        if (m_closed) {
            return m_points[0];
        }
        return m_points.back();
    }
    
    // Catmull-Rom interpolation for smoothness
    int p0 = std::max(0, idx - 1);
    int p1 = idx;
    int p2 = std::min(idx + 1, static_cast<int>(m_points.size()) - 1);
    int p3 = std::min(idx + 2, static_cast<int>(m_points.size()) - 1);
    
    if (m_closed) {
        p0 = (idx - 1 + m_points.size()) % m_points.size();
        p2 = (idx + 1) % m_points.size();
        p3 = (idx + 2) % m_points.size();
    }
    
    float t2 = frac * frac;
    float t3 = t2 * frac;
    
    glm::vec3 result = 0.5f * (
        (2.0f * m_points[p1]) +
        (-m_points[p0] + m_points[p2]) * frac +
        (2.0f * m_points[p0] - 5.0f * m_points[p1] + 4.0f * m_points[p2] - m_points[p3]) * t2 +
        (-m_points[p0] + 3.0f * m_points[p1] - 3.0f * m_points[p2] + m_points[p3]) * t3
    );
    
    return result;
}

glm::vec3 SweepPath::tangent(float t) const {
    if (m_points.size() < 2) return glm::vec3(0, 0, 1);
    
    const float dt = 0.001f;
    glm::vec3 p0 = evaluate(std::max(0.0f, t - dt));
    glm::vec3 p1 = evaluate(std::min(1.0f, t + dt));
    
    glm::vec3 tan = p1 - p0;
    float len = glm::length(tan);
    return (len > 1e-10f) ? tan / len : glm::vec3(0, 0, 1);
}

float SweepPath::curvature(float t) const {
    if (m_points.size() < 3) return 0.0f;
    
    const float dt = 0.001f;
    glm::vec3 t0 = tangent(std::max(0.0f, t - dt));
    glm::vec3 t1 = tangent(std::min(1.0f, t + dt));
    
    glm::vec3 dT = (t1 - t0) / (2.0f * dt);
    return glm::length(dT);
}

std::vector<glm::vec3> SweepPath::resampleByArcLength(int numPoints) const {
    std::vector<glm::vec3> result;
    if (m_points.empty() || numPoints < 2) return result;
    
    float totalLen = length();
    if (totalLen < 1e-10f) {
        result.resize(numPoints, m_points[0]);
        return result;
    }
    
    for (int i = 0; i < numPoints; ++i) {
        float s = i * totalLen / (numPoints - 1);
        float t = arcLengthToT(s);
        result.push_back(evaluate(t));
    }
    
    return result;
}

// ============================================================================
// SweepProfile Implementation
// ============================================================================

SweepProfile::SweepProfile(const std::vector<glm::vec3>& points, bool closed)
    : m_points(points), m_closed(closed) {
}

SweepProfile SweepProfile::circle(float radius, int segments) {
    std::vector<glm::vec3> points;
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        points.push_back(glm::vec3(radius * std::cos(angle), radius * std::sin(angle), 0.0f));
    }
    return SweepProfile(points, true);
}

SweepProfile SweepProfile::rectangle(float width, float height) {
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    std::vector<glm::vec3> points = {
        glm::vec3(-hw, -hh, 0),
        glm::vec3( hw, -hh, 0),
        glm::vec3( hw,  hh, 0),
        glm::vec3(-hw,  hh, 0)
    };
    return SweepProfile(points, true);
}

SweepProfile SweepProfile::ellipse(float radiusX, float radiusY, int segments) {
    std::vector<glm::vec3> points;
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        points.push_back(glm::vec3(radiusX * std::cos(angle), radiusY * std::sin(angle), 0.0f));
    }
    return SweepProfile(points, true);
}

void SweepProfile::setPoints(const std::vector<glm::vec3>& points) {
    m_points = points;
}

glm::vec3 SweepProfile::centroid() const {
    if (m_points.empty()) return glm::vec3(0.0f);
    
    glm::vec3 c(0.0f);
    for (const auto& p : m_points) {
        c += p;
    }
    return c / static_cast<float>(m_points.size());
}

SweepProfile SweepProfile::transformed(const glm::mat4& matrix) const {
    std::vector<glm::vec3> newPoints;
    for (const auto& p : m_points) {
        glm::vec4 tp = matrix * glm::vec4(p, 1.0f);
        newPoints.push_back(glm::vec3(tp));
    }
    return SweepProfile(newPoints, m_closed);
}

// ============================================================================
// Sweep Implementation
// ============================================================================

Sweep::Frame Sweep::computeFrenetFrame(const SweepPath& path, float t) {
    Frame frame;
    frame.position = path.evaluate(t);
    frame.tangent = path.tangent(t);
    
    // Get second derivative for normal
    const float dt = 0.001f;
    glm::vec3 t0 = path.tangent(std::max(0.0f, t - dt));
    glm::vec3 t1 = path.tangent(std::min(1.0f, t + dt));
    glm::vec3 dT = t1 - t0;
    
    float dTLen = glm::length(dT);
    if (dTLen > 1e-10f) {
        frame.normal = dT / dTLen;
    } else {
        // Curve is straight, pick arbitrary normal
        glm::vec3 up(0, 1, 0);
        if (std::abs(glm::dot(frame.tangent, up)) > 0.9f) {
            up = glm::vec3(1, 0, 0);
        }
        frame.normal = glm::normalize(glm::cross(frame.tangent, up));
    }
    
    frame.binormal = glm::cross(frame.tangent, frame.normal);
    
    return frame;
}

std::vector<Sweep::Frame> Sweep::computeParallelTransportFrames(const SweepPath& path, int numFrames) {
    std::vector<Frame> frames;
    if (numFrames < 2) return frames;
    
    // Initialize first frame
    Frame first;
    first.position = path.evaluate(0.0f);
    first.tangent = path.tangent(0.0f);
    
    // Choose initial normal perpendicular to tangent
    glm::vec3 up(0, 1, 0);
    if (std::abs(glm::dot(first.tangent, up)) > 0.9f) {
        up = glm::vec3(1, 0, 0);
    }
    first.normal = glm::normalize(glm::cross(first.tangent, up));
    first.binormal = glm::cross(first.tangent, first.normal);
    frames.push_back(first);
    
    // Propagate frame using parallel transport
    for (int i = 1; i < numFrames; ++i) {
        float t = static_cast<float>(i) / (numFrames - 1);
        
        Frame frame;
        frame.position = path.evaluate(t);
        frame.tangent = path.tangent(t);
        
        // Rotate previous normal to be perpendicular to new tangent
        const Frame& prev = frames.back();
        
        // Rotation axis from previous tangent to current tangent
        glm::vec3 axis = glm::cross(prev.tangent, frame.tangent);
        float axisLen = glm::length(axis);
        
        if (axisLen > 1e-10f) {
            axis /= axisLen;
            float angle = std::acos(std::clamp(glm::dot(prev.tangent, frame.tangent), -1.0f, 1.0f));
            
            // Rodrigues' rotation formula
            frame.normal = prev.normal * std::cos(angle) +
                          glm::cross(axis, prev.normal) * std::sin(angle) +
                          axis * glm::dot(axis, prev.normal) * (1.0f - std::cos(angle));
        } else {
            frame.normal = prev.normal;
        }
        
        // Ensure orthonormality
        frame.normal = glm::normalize(frame.normal - glm::dot(frame.normal, frame.tangent) * frame.tangent);
        frame.binormal = glm::cross(frame.tangent, frame.normal);
        
        frames.push_back(frame);
    }
    
    return frames;
}

std::vector<Sweep::Frame> Sweep::computeFixedFrames(const SweepPath& path, int numFrames,
                                                      const glm::vec3& upVector) {
    std::vector<Frame> frames;
    
    for (int i = 0; i < numFrames; ++i) {
        float t = static_cast<float>(i) / (numFrames - 1);
        
        Frame frame;
        frame.position = path.evaluate(t);
        frame.tangent = path.tangent(t);
        
        // Use fixed up vector
        glm::vec3 right = glm::cross(upVector, frame.tangent);
        float rightLen = glm::length(right);
        
        if (rightLen > 1e-10f) {
            frame.binormal = right / rightLen;
            frame.normal = glm::cross(frame.tangent, frame.binormal);
        } else {
            // Tangent is parallel to up, use fallback
            glm::vec3 altUp = glm::vec3(1, 0, 0);
            frame.binormal = glm::normalize(glm::cross(altUp, frame.tangent));
            frame.normal = glm::cross(frame.tangent, frame.binormal);
        }
        
        frames.push_back(frame);
    }
    
    return frames;
}

std::vector<glm::vec3> Sweep::transformProfile(const SweepProfile& profile,
                                                 const Frame& frame,
                                                 float scale,
                                                 float twist) {
    std::vector<glm::vec3> result;
    
    // Build transformation matrix
    glm::mat4 transform(1.0f);
    
    // Translation to frame position
    transform[3] = glm::vec4(frame.position, 1.0f);
    
    // Rotation to align with frame
    transform[0] = glm::vec4(frame.normal * scale, 0.0f);
    transform[1] = glm::vec4(frame.binormal * scale, 0.0f);
    transform[2] = glm::vec4(frame.tangent, 0.0f);
    
    // Apply twist around tangent
    if (std::abs(twist) > 1e-6f) {
        float twistRad = glm::radians(twist);
        glm::mat4 twistMat = glm::rotate(glm::mat4(1.0f), twistRad, glm::vec3(0, 0, 1));
        transform = transform * twistMat;
    }
    
    for (const auto& p : profile.points()) {
        glm::vec4 tp = transform * glm::vec4(p, 1.0f);
        result.push_back(glm::vec3(tp));
    }
    
    return result;
}

MeshData Sweep::createMesh(const SweepProfile& profile,
                            const std::vector<Frame>& frames,
                            const SweepOptions& options) {
    MeshData mesh;
    
    if (frames.empty() || !profile.isValid()) return mesh;
    
    int numFrames = static_cast<int>(frames.size());
    int numPoints = static_cast<int>(profile.points().size());
    bool closedProfile = profile.isClosed();
    
    // Generate vertices
    for (int i = 0; i < numFrames; ++i) {
        float t = static_cast<float>(i) / (numFrames - 1);
        
        // Calculate scale and twist at this point
        float scale = options.startScale + t * (options.endScale - options.startScale);
        float twist = t * options.twistAngle;
        
        auto transformed = transformProfile(profile, frames[i], scale, twist);
        for (const auto& p : transformed) {
            mesh.addVertex(p);
        }
    }
    
    // Generate faces
    for (int i = 0; i < numFrames - 1; ++i) {
        int effectiveCols = closedProfile ? numPoints : numPoints - 1;
        
        for (int j = 0; j < effectiveCols; ++j) {
            int nextJ = (j + 1) % numPoints;
            
            uint32_t v00 = i * numPoints + j;
            uint32_t v10 = i * numPoints + nextJ;
            uint32_t v01 = (i + 1) * numPoints + j;
            uint32_t v11 = (i + 1) * numPoints + nextJ;
            
            mesh.addFace(v00, v10, v11);
            mesh.addFace(v00, v11, v01);
        }
    }
    
    mesh.computeNormals();
    return mesh;
}

SweepResult Sweep::sweep(const SweepProfile& profile,
                          const SweepPath& path,
                          const SweepOptions& options) {
    SweepResult result;
    
    if (!profile.isValid()) {
        result.errorMessage = "Invalid profile";
        return result;
    }
    
    if (!path.isValid()) {
        result.errorMessage = "Invalid path";
        return result;
    }
    
    // Compute frames along path
    std::vector<Frame> frames;
    
    switch (options.orientation) {
        case SweepOrientation::FrenetSerret:
            for (int i = 0; i <= options.pathSegments; ++i) {
                float t = static_cast<float>(i) / options.pathSegments;
                frames.push_back(computeFrenetFrame(path, t));
            }
            break;
            
        case SweepOrientation::ParallelTransport:
            frames = computeParallelTransportFrames(path, options.pathSegments + 1);
            break;
            
        case SweepOrientation::Fixed:
            frames = computeFixedFrames(path, options.pathSegments + 1);
            break;
            
        default:
            frames = computeParallelTransportFrames(path, options.pathSegments + 1);
            break;
    }
    
    // Create mesh
    result.mesh = createMesh(profile, frames, options);
    result.success = true;
    
    return result;
}

SweepResult Sweep::sweepWithTwist(const SweepProfile& profile,
                                   const SweepPath& path,
                                   float twistAngle,
                                   const SweepOptions& options) {
    SweepOptions opts = options;
    opts.twistAngle = twistAngle;
    return sweep(profile, path, opts);
}

SweepResult Sweep::sweepWithScale(const SweepProfile& profile,
                                   const SweepPath& path,
                                   float startScale,
                                   float endScale,
                                   const SweepOptions& options) {
    SweepOptions opts = options;
    opts.startScale = startScale;
    opts.endScale = endScale;
    return sweep(profile, path, opts);
}

SweepResult Sweep::sweep(const std::vector<glm::vec3>& profilePoints,
                          const std::vector<glm::vec3>& pathPoints,
                          bool closedProfile,
                          const SweepOptions& options) {
    SweepProfile profile(profilePoints, closedProfile);
    SweepPath path(pathPoints);
    return sweep(profile, path, options);
}

std::vector<NURBSSurface> Sweep::createSurfaces(const SweepProfile& profile,
                                                 const SweepPath& path,
                                                 const SweepOptions& options) {
    std::vector<NURBSSurface> surfaces;
    
    // Compute frames
    auto frames = computeParallelTransportFrames(path, options.pathSegments + 1);
    
    int numU = static_cast<int>(profile.points().size());
    int numV = static_cast<int>(frames.size());
    
    std::vector<ControlPoint> cps(numU * numV);
    
    for (int j = 0; j < numV; ++j) {
        float t = static_cast<float>(j) / (numV - 1);
        float scale = options.startScale + t * (options.endScale - options.startScale);
        float twist = t * options.twistAngle;
        
        auto transformed = transformProfile(profile, frames[j], scale, twist);
        
        for (int i = 0; i < numU; ++i) {
            cps[j * numU + i] = ControlPoint(transformed[i]);
        }
    }
    
    int degreeU = std::min(3, numU - 1);
    int degreeV = std::min(3, numV - 1);
    
    std::vector<float> knotsU(numU + degreeU + 1);
    std::vector<float> knotsV(numV + degreeV + 1);
    
    // Clamped uniform
    for (int i = 0; i <= degreeU; ++i) knotsU[i] = 0.0f;
    for (int i = degreeU + 1; i < numU; ++i) {
        knotsU[i] = static_cast<float>(i - degreeU) / (numU - degreeU);
    }
    for (int i = numU; i < static_cast<int>(knotsU.size()); ++i) knotsU[i] = 1.0f;
    
    for (int i = 0; i <= degreeV; ++i) knotsV[i] = 0.0f;
    for (int i = degreeV + 1; i < numV; ++i) {
        knotsV[i] = static_cast<float>(i - degreeV) / (numV - degreeV);
    }
    for (int i = numV; i < static_cast<int>(knotsV.size()); ++i) knotsV[i] = 1.0f;
    
    NURBSSurface surface;
    surface.create(cps, numU, numV, knotsU, knotsV, degreeU, degreeV);
    surfaces.push_back(surface);
    
    return surfaces;
}

} // namespace geometry
} // namespace dc3d
