/**
 * @file NURBSSurface.cpp
 * @brief NURBS surface implementation
 */

#include "NURBSSurface.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace dc3d {
namespace geometry {

bool NURBSSurface::create(const std::vector<ControlPoint>& controlPoints,
                          int numU, int numV,
                          const std::vector<float>& knotsU,
                          const std::vector<float>& knotsV,
                          int degreeU, int degreeV) {
    // Validate input
    if (numU < 2 || numV < 2) {
        return false;
    }
    
    if (static_cast<int>(controlPoints.size()) != numU * numV) {
        return false;
    }
    
    // Validate knot vector sizes
    int expectedKnotsU = numU + degreeU + 1;
    int expectedKnotsV = numV + degreeV + 1;
    
    if (static_cast<int>(knotsU.size()) != expectedKnotsU ||
        static_cast<int>(knotsV.size()) != expectedKnotsV) {
        return false;
    }
    
    // Validate knot vectors are non-decreasing
    for (size_t i = 1; i < knotsU.size(); ++i) {
        if (knotsU[i] < knotsU[i-1]) return false;
    }
    for (size_t i = 1; i < knotsV.size(); ++i) {
        if (knotsV[i] < knotsV[i-1]) return false;
    }
    
    m_controlPoints = controlPoints;
    m_numU = numU;
    m_numV = numV;
    m_knotsU = knotsU;
    m_knotsV = knotsV;
    m_degreeU = degreeU;
    m_degreeV = degreeV;
    
    return true;
}

bool NURBSSurface::createBezier(const std::vector<ControlPoint>& controlPoints,
                                 int numU, int numV) {
    // Bezier surface uses clamped knot vectors with all 0s at start and 1s at end
    int degreeU = numU - 1;
    int degreeV = numV - 1;
    
    std::vector<float> knotsU(numU + degreeU + 1);
    std::vector<float> knotsV(numV + degreeV + 1);
    
    // Clamped knot vectors
    for (int i = 0; i <= degreeU; ++i) knotsU[i] = 0.0f;
    for (int i = degreeU + 1; i < static_cast<int>(knotsU.size()); ++i) knotsU[i] = 1.0f;
    
    for (int i = 0; i <= degreeV; ++i) knotsV[i] = 0.0f;
    for (int i = degreeV + 1; i < static_cast<int>(knotsV.size()); ++i) knotsV[i] = 1.0f;
    
    return create(controlPoints, numU, numV, knotsU, knotsV, degreeU, degreeV);
}

NURBSSurface NURBSSurface::createBilinear(const glm::vec3& p00, const glm::vec3& p10,
                                           const glm::vec3& p01, const glm::vec3& p11) {
    NURBSSurface surface;
    
    std::vector<ControlPoint> cps = {
        ControlPoint(p00), ControlPoint(p10),
        ControlPoint(p01), ControlPoint(p11)
    };
    
    // Degree 1 in both directions
    std::vector<float> knots = {0.0f, 0.0f, 1.0f, 1.0f};
    
    surface.create(cps, 2, 2, knots, knots, 1, 1);
    return surface;
}

NURBSSurface NURBSSurface::createPlanar(const std::vector<glm::vec3>& boundary) {
    NURBSSurface surface;
    
    if (boundary.size() < 3) {
        return surface;
    }
    
    // Calculate centroid and plane normal
    glm::vec3 centroid(0.0f);
    for (const auto& p : boundary) {
        centroid += p;
    }
    centroid /= static_cast<float>(boundary.size());
    
    // Compute normal using Newell's method
    glm::vec3 normal(0.0f);
    for (size_t i = 0; i < boundary.size(); ++i) {
        const glm::vec3& curr = boundary[i];
        const glm::vec3& next = boundary[(i + 1) % boundary.size()];
        normal.x += (curr.y - next.y) * (curr.z + next.z);
        normal.y += (curr.z - next.z) * (curr.x + next.x);
        normal.z += (curr.x - next.x) * (curr.y + next.y);
    }
    normal = glm::normalize(normal);
    
    // Create a simple bilinear patch from bounding box
    BoundingBox bbox;
    for (const auto& p : boundary) {
        bbox.expand(p);
    }
    
    // Project to plane and create bilinear surface
    glm::vec3 tangentU = glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0)));
    if (glm::length(tangentU) < 0.1f) {
        tangentU = glm::normalize(glm::cross(normal, glm::vec3(1, 0, 0)));
    }
    glm::vec3 tangentV = glm::cross(normal, tangentU);
    
    float extent = bbox.diagonal() * 0.5f;
    glm::vec3 p00 = centroid - tangentU * extent - tangentV * extent;
    glm::vec3 p10 = centroid + tangentU * extent - tangentV * extent;
    glm::vec3 p01 = centroid - tangentU * extent + tangentV * extent;
    glm::vec3 p11 = centroid + tangentU * extent + tangentV * extent;
    
    return createBilinear(p00, p10, p01, p11);
}

int NURBSSurface::findKnotSpan(int n, int p, float t, const std::vector<float>& knots) const {
    // Special cases
    if (t >= knots[n + 1]) return n;
    if (t <= knots[p]) return p;
    
    // Binary search
    int low = p;
    int high = n + 1;
    int mid = (low + high) / 2;
    
    while (t < knots[mid] || t >= knots[mid + 1]) {
        if (t < knots[mid]) {
            high = mid;
        } else {
            low = mid;
        }
        mid = (low + high) / 2;
    }
    
    return mid;
}

float NURBSSurface::basisFunction(int i, int p, float t, const std::vector<float>& knots) const {
    // Cox-de Boor recursion
    if (p == 0) {
        return (t >= knots[i] && t < knots[i + 1]) ? 1.0f : 0.0f;
    }
    
    float left = 0.0f;
    float right = 0.0f;
    
    float denom1 = knots[i + p] - knots[i];
    if (std::abs(denom1) > 1e-10f) {
        left = (t - knots[i]) / denom1 * basisFunction(i, p - 1, t, knots);
    }
    
    float denom2 = knots[i + p + 1] - knots[i + 1];
    if (std::abs(denom2) > 1e-10f) {
        right = (knots[i + p + 1] - t) / denom2 * basisFunction(i + 1, p - 1, t, knots);
    }
    
    return left + right;
}

float NURBSSurface::basisFunctionDerivative(int i, int p, float t, const std::vector<float>& knots) const {
    if (p == 0) return 0.0f;
    
    float left = 0.0f;
    float right = 0.0f;
    
    float denom1 = knots[i + p] - knots[i];
    if (std::abs(denom1) > 1e-10f) {
        left = p / denom1 * basisFunction(i, p - 1, t, knots);
    }
    
    float denom2 = knots[i + p + 1] - knots[i + 1];
    if (std::abs(denom2) > 1e-10f) {
        right = p / denom2 * basisFunction(i + 1, p - 1, t, knots);
    }
    
    return left - right;
}

glm::vec3 NURBSSurface::evaluate(float u, float v) const {
    if (!isValid()) return glm::vec3(0.0f);
    
    // Clamp parameters
    u = std::clamp(u, m_knotsU[m_degreeU], m_knotsU[m_numU]);
    v = std::clamp(v, m_knotsV[m_degreeV], m_knotsV[m_numV]);
    
    // Evaluate using basis functions
    glm::vec4 result(0.0f);
    
    for (int j = 0; j < m_numV; ++j) {
        float Nv = basisFunction(j, m_degreeV, v, m_knotsV);
        if (std::abs(Nv) < 1e-10f) continue;
        
        for (int i = 0; i < m_numU; ++i) {
            float Nu = basisFunction(i, m_degreeU, u, m_knotsU);
            if (std::abs(Nu) < 1e-10f) continue;
            
            const ControlPoint& cp = m_controlPoints[index(i, j)];
            result += Nu * Nv * cp.homogeneous();
        }
    }
    
    // Convert from homogeneous coordinates
    if (std::abs(result.w) < 1e-10f) {
        return glm::vec3(result);
    }
    return glm::vec3(result) / result.w;
}

SurfacePoint NURBSSurface::evaluateWithDerivatives(float u, float v) const {
    SurfacePoint sp;
    sp.u = u;
    sp.v = v;
    sp.position = evaluate(u, v);
    sp.tangentU = derivativeU(u, v);
    sp.tangentV = derivativeV(u, v);
    sp.normal = glm::normalize(glm::cross(sp.tangentU, sp.tangentV));
    return sp;
}

glm::vec3 NURBSSurface::normal(float u, float v) const {
    glm::vec3 du = derivativeU(u, v);
    glm::vec3 dv = derivativeV(u, v);
    glm::vec3 n = glm::cross(du, dv);
    float len = glm::length(n);
    if (len < 1e-10f) return glm::vec3(0, 0, 1);
    return n / len;
}

glm::vec3 NURBSSurface::derivativeU(float u, float v) const {
    if (!isValid()) return glm::vec3(0.0f);
    
    u = std::clamp(u, m_knotsU[m_degreeU], m_knotsU[m_numU]);
    v = std::clamp(v, m_knotsV[m_degreeV], m_knotsV[m_numV]);
    
    glm::vec4 numerator(0.0f);
    glm::vec4 numeratorDeriv(0.0f);
    float denominator = 0.0f;
    float denominatorDeriv = 0.0f;
    
    for (int j = 0; j < m_numV; ++j) {
        float Nv = basisFunction(j, m_degreeV, v, m_knotsV);
        if (std::abs(Nv) < 1e-10f) continue;
        
        for (int i = 0; i < m_numU; ++i) {
            float Nu = basisFunction(i, m_degreeU, u, m_knotsU);
            float dNu = basisFunctionDerivative(i, m_degreeU, u, m_knotsU);
            
            const ControlPoint& cp = m_controlPoints[index(i, j)];
            glm::vec4 h = cp.homogeneous();
            
            numerator += Nu * Nv * h;
            numeratorDeriv += dNu * Nv * h;
            denominator += Nu * Nv * cp.weight;
            denominatorDeriv += dNu * Nv * cp.weight;
        }
    }
    
    // Quotient rule for rational surface
    if (std::abs(denominator) < 1e-10f) return glm::vec3(0.0f);
    
    glm::vec3 S = glm::vec3(numerator) / denominator;
    glm::vec3 dNum = glm::vec3(numeratorDeriv);
    
    return (dNum - S * denominatorDeriv) / denominator;
}

glm::vec3 NURBSSurface::derivativeV(float u, float v) const {
    if (!isValid()) return glm::vec3(0.0f);
    
    u = std::clamp(u, m_knotsU[m_degreeU], m_knotsU[m_numU]);
    v = std::clamp(v, m_knotsV[m_degreeV], m_knotsV[m_numV]);
    
    glm::vec4 numerator(0.0f);
    glm::vec4 numeratorDeriv(0.0f);
    float denominator = 0.0f;
    float denominatorDeriv = 0.0f;
    
    for (int j = 0; j < m_numV; ++j) {
        float Nv = basisFunction(j, m_degreeV, v, m_knotsV);
        float dNv = basisFunctionDerivative(j, m_degreeV, v, m_knotsV);
        
        for (int i = 0; i < m_numU; ++i) {
            float Nu = basisFunction(i, m_degreeU, u, m_knotsU);
            if (std::abs(Nu) < 1e-10f) continue;
            
            const ControlPoint& cp = m_controlPoints[index(i, j)];
            glm::vec4 h = cp.homogeneous();
            
            numerator += Nu * Nv * h;
            numeratorDeriv += Nu * dNv * h;
            denominator += Nu * Nv * cp.weight;
            denominatorDeriv += Nu * dNv * cp.weight;
        }
    }
    
    if (std::abs(denominator) < 1e-10f) return glm::vec3(0.0f);
    
    glm::vec3 S = glm::vec3(numerator) / denominator;
    glm::vec3 dNum = glm::vec3(numeratorDeriv);
    
    return (dNum - S * denominatorDeriv) / denominator;
}

MeshData NURBSSurface::tessellate(const TessellationOptions& options) const {
    return tessellate(options.uDivisions, options.vDivisions);
}

MeshData NURBSSurface::tessellate(int uDivs, int vDivs) const {
    MeshData mesh;
    
    if (!isValid()) return mesh;
    
    uDivs = std::max(2, uDivs);
    vDivs = std::max(2, vDivs);
    
    // Get parameter domain
    float uMin = m_knotsU[m_degreeU];
    float uMax = m_knotsU[m_numU];
    float vMin = m_knotsV[m_degreeV];
    float vMax = m_knotsV[m_numV];
    
    float du = (uMax - uMin) / uDivs;
    float dv = (vMax - vMin) / vDivs;
    
    // Generate vertices
    mesh.reserveVertices((uDivs + 1) * (vDivs + 1));
    mesh.reserveFaces(uDivs * vDivs * 2);
    
    for (int j = 0; j <= vDivs; ++j) {
        float v = vMin + j * dv;
        for (int i = 0; i <= uDivs; ++i) {
            float u = uMin + i * du;
            SurfacePoint sp = evaluateWithDerivatives(u, v);
            mesh.addVertex(sp.position, sp.normal);
        }
    }
    
    // Set up UVs
    mesh.uvs().resize(mesh.vertexCount());
    int idx = 0;
    for (int j = 0; j <= vDivs; ++j) {
        float v = static_cast<float>(j) / vDivs;
        for (int i = 0; i <= uDivs; ++i) {
            float u = static_cast<float>(i) / uDivs;
            mesh.uvs()[idx++] = glm::vec2(u, v);
        }
    }
    
    // Generate faces (two triangles per quad)
    for (int j = 0; j < vDivs; ++j) {
        for (int i = 0; i < uDivs; ++i) {
            uint32_t v00 = j * (uDivs + 1) + i;
            uint32_t v10 = v00 + 1;
            uint32_t v01 = v00 + (uDivs + 1);
            uint32_t v11 = v01 + 1;
            
            mesh.addFace(v00, v10, v11);
            mesh.addFace(v00, v11, v01);
        }
    }
    
    return mesh;
}

std::vector<glm::vec3> NURBSSurface::sampleGrid(int uSamples, int vSamples) const {
    std::vector<glm::vec3> points;
    
    if (!isValid()) return points;
    
    float uMin, uMax, vMin, vMax;
    getDomain(uMin, uMax, vMin, vMax);
    
    float du = (uMax - uMin) / (uSamples - 1);
    float dv = (vMax - vMin) / (vSamples - 1);
    
    points.reserve(uSamples * vSamples);
    
    for (int j = 0; j < vSamples; ++j) {
        float v = vMin + j * dv;
        for (int i = 0; i < uSamples; ++i) {
            float u = uMin + i * du;
            points.push_back(evaluate(u, v));
        }
    }
    
    return points;
}

bool NURBSSurface::isValid() const {
    if (m_numU < 2 || m_numV < 2) return false;
    if (m_degreeU < 1 || m_degreeV < 1) return false;
    if (static_cast<int>(m_controlPoints.size()) != m_numU * m_numV) return false;
    if (static_cast<int>(m_knotsU.size()) != m_numU + m_degreeU + 1) return false;
    if (static_cast<int>(m_knotsV.size()) != m_numV + m_degreeV + 1) return false;
    return true;
}

void NURBSSurface::getDomain(float& uMin, float& uMax, float& vMin, float& vMax) const {
    if (!isValid()) {
        uMin = uMax = vMin = vMax = 0.0f;
        return;
    }
    uMin = m_knotsU[m_degreeU];
    uMax = m_knotsU[m_numU];
    vMin = m_knotsV[m_degreeV];
    vMax = m_knotsV[m_numV];
}

float NURBSSurface::surfaceArea(int samples) const {
    MeshData mesh = tessellate(samples, samples);
    return mesh.surfaceArea();
}

BoundingBox NURBSSurface::boundingBox() const {
    BoundingBox bbox;
    for (const auto& cp : m_controlPoints) {
        bbox.expand(cp.position);
    }
    return bbox;
}

const ControlPoint& NURBSSurface::controlPoint(int i, int j) const {
    return m_controlPoints[index(i, j)];
}

ControlPoint& NURBSSurface::controlPoint(int i, int j) {
    return m_controlPoints[index(i, j)];
}

void NURBSSurface::setControlPoint(int i, int j, const glm::vec3& pos) {
    m_controlPoints[index(i, j)].position = pos;
}

void NURBSSurface::setWeight(int i, int j, float weight) {
    m_controlPoints[index(i, j)].weight = weight;
}

void NURBSSurface::transform(const glm::mat4& matrix) {
    for (auto& cp : m_controlPoints) {
        glm::vec4 p = matrix * glm::vec4(cp.position, 1.0f);
        cp.position = glm::vec3(p) / p.w;
    }
}

void NURBSSurface::reverseU() {
    // Reverse control points in U direction
    for (int j = 0; j < m_numV; ++j) {
        for (int i = 0; i < m_numU / 2; ++i) {
            std::swap(m_controlPoints[index(i, j)], 
                     m_controlPoints[index(m_numU - 1 - i, j)]);
        }
    }
    
    // Reverse and negate knot vector
    std::reverse(m_knotsU.begin(), m_knotsU.end());
    float knotMax = m_knotsU.front();
    for (auto& k : m_knotsU) {
        k = knotMax - k;
    }
}

void NURBSSurface::reverseV() {
    // Reverse control points in V direction
    for (int i = 0; i < m_numU; ++i) {
        for (int j = 0; j < m_numV / 2; ++j) {
            std::swap(m_controlPoints[index(i, j)],
                     m_controlPoints[index(i, m_numV - 1 - j)]);
        }
    }
    
    std::reverse(m_knotsV.begin(), m_knotsV.end());
    float knotMax = m_knotsV.front();
    for (auto& k : m_knotsV) {
        k = knotMax - k;
    }
}

void NURBSSurface::insertKnotU(float u) {
    // Knot insertion algorithm (simplified)
    // Full implementation would use Oslo algorithm
    
    int k = findKnotSpan(m_numU - 1, m_degreeU, u, m_knotsU);
    
    // Insert knot into knot vector
    m_knotsU.insert(m_knotsU.begin() + k + 1, u);
    
    // Add new control points
    std::vector<ControlPoint> newCPs;
    newCPs.reserve((m_numU + 1) * m_numV);
    
    for (int j = 0; j < m_numV; ++j) {
        for (int i = 0; i <= m_numU; ++i) {
            if (i <= k - m_degreeU) {
                newCPs.push_back(m_controlPoints[index(i, j)]);
            } else if (i > k) {
                newCPs.push_back(m_controlPoints[index(i - 1, j)]);
            } else {
                float alpha = (u - m_knotsU[i]) / (m_knotsU[i + m_degreeU] - m_knotsU[i]);
                ControlPoint cp;
                cp.position = (1.0f - alpha) * m_controlPoints[index(i - 1, j)].position +
                              alpha * m_controlPoints[index(i, j)].position;
                cp.weight = (1.0f - alpha) * m_controlPoints[index(i - 1, j)].weight +
                            alpha * m_controlPoints[index(i, j)].weight;
                newCPs.push_back(cp);
            }
        }
    }
    
    m_controlPoints = std::move(newCPs);
    m_numU++;
}

void NURBSSurface::insertKnotV(float v) {
    int k = findKnotSpan(m_numV - 1, m_degreeV, v, m_knotsV);
    
    m_knotsV.insert(m_knotsV.begin() + k + 1, v);
    
    std::vector<ControlPoint> newCPs;
    newCPs.reserve(m_numU * (m_numV + 1));
    
    for (int j = 0; j <= m_numV; ++j) {
        for (int i = 0; i < m_numU; ++i) {
            if (j <= k - m_degreeV) {
                newCPs.push_back(m_controlPoints[index(i, j)]);
            } else if (j > k) {
                newCPs.push_back(m_controlPoints[index(i, j - 1)]);
            } else {
                float alpha = (v - m_knotsV[j]) / (m_knotsV[j + m_degreeV] - m_knotsV[j]);
                ControlPoint cp;
                cp.position = (1.0f - alpha) * m_controlPoints[index(i, j - 1)].position +
                              alpha * m_controlPoints[index(i, j)].position;
                cp.weight = (1.0f - alpha) * m_controlPoints[index(i, j - 1)].weight +
                            alpha * m_controlPoints[index(i, j)].weight;
                newCPs.push_back(cp);
            }
        }
    }
    
    m_controlPoints = std::move(newCPs);
    m_numV++;
}

void NURBSSurface::refine(int uInsertions, int vInsertions) {
    float uMin, uMax, vMin, vMax;
    getDomain(uMin, uMax, vMin, vMax);
    
    for (int i = 0; i < uInsertions; ++i) {
        float u = uMin + (i + 1) * (uMax - uMin) / (uInsertions + 1);
        insertKnotU(u);
    }
    
    for (int i = 0; i < vInsertions; ++i) {
        float v = vMin + (i + 1) * (vMax - vMin) / (vInsertions + 1);
        insertKnotV(v);
    }
}

std::vector<glm::vec3> NURBSSurface::isocurveU(float u, int samples) const {
    std::vector<glm::vec3> curve;
    curve.reserve(samples);
    
    float vMin, vMax, uMin, uMax;
    getDomain(uMin, uMax, vMin, vMax);
    
    float dv = (vMax - vMin) / (samples - 1);
    
    for (int i = 0; i < samples; ++i) {
        float v = vMin + i * dv;
        curve.push_back(evaluate(u, v));
    }
    
    return curve;
}

std::vector<glm::vec3> NURBSSurface::isocurveV(float v, int samples) const {
    std::vector<glm::vec3> curve;
    curve.reserve(samples);
    
    float vMin, vMax, uMin, uMax;
    getDomain(uMin, uMax, vMin, vMax);
    
    float du = (uMax - uMin) / (samples - 1);
    
    for (int i = 0; i < samples; ++i) {
        float u = uMin + i * du;
        curve.push_back(evaluate(u, v));
    }
    
    return curve;
}

void NURBSSurface::getBoundaries(std::vector<glm::vec3>& uMinCurve, 
                                  std::vector<glm::vec3>& uMaxCurve,
                                  std::vector<glm::vec3>& vMinCurve, 
                                  std::vector<glm::vec3>& vMaxCurve,
                                  int samples) const {
    float uMin, uMax, vMin, vMax;
    getDomain(uMin, uMax, vMin, vMax);
    
    uMinCurve = isocurveU(uMin, samples);
    uMaxCurve = isocurveU(uMax, samples);
    vMinCurve = isocurveV(vMin, samples);
    vMaxCurve = isocurveV(vMax, samples);
}

} // namespace geometry
} // namespace dc3d
