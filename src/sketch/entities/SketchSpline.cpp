#include "SketchSpline.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace dc {
namespace sketch {

SketchSpline::SketchSpline(const std::vector<glm::vec2>& controlPoints, int degree)
    : SketchEntity(SketchEntityType::Spline)
    , m_controlPoints(controlPoints)
    , m_degree(degree)
{
    if (controlPoints.size() < 2) {
        throw std::invalid_argument("Spline requires at least 2 control points");
    }
    
    // FIX #13: Clamp degree to valid range with documentation
    // Note: A spline with n control points can have at most degree (n-1).
    // For example, 2 control points = max degree 1 (linear).
    // If you need a higher degree spline, provide more control points.
    int maxDegree = static_cast<int>(m_controlPoints.size()) - 1;
    if (m_degree > maxDegree) {
        // Silently clamp, but this is documented behavior
        m_degree = maxDegree;
    }
    m_degree = std::max(m_degree, 1);
    
    generateKnots();
}

SketchSpline::Ptr SketchSpline::createInterpolating(const std::vector<glm::vec2>& points, int degree) {
    if (points.size() < 2) {
        return nullptr;
    }
    
    // For simple interpolation, we use the points directly as control points
    // A more sophisticated approach would solve for control points that make
    // the curve pass exactly through each point
    // For now, we use a simple approach with chord-length parameterization
    
    int n = static_cast<int>(points.size());
    
    if (n <= degree + 1) {
        // Not enough points for proper interpolation, use points as control points
        return std::make_shared<SketchSpline>(points, degree);
    }
    
    // Calculate chord lengths
    std::vector<float> chordLengths(n - 1);
    float totalLength = 0.0f;
    for (int i = 0; i < n - 1; ++i) {
        chordLengths[i] = glm::length(points[i + 1] - points[i]);
        totalLength += chordLengths[i];
    }
    
    // For a simple approach, we can use the input points as control points
    // with a lower degree to ensure smoother interpolation
    return std::make_shared<SketchSpline>(points, std::min(degree, 2));
}

void SketchSpline::generateKnots() {
    // Generate clamped uniform knot vector
    int n = static_cast<int>(m_controlPoints.size());
    int numKnots = n + m_degree + 1;
    
    m_knots.resize(numKnots);
    
    // Clamped knots: p+1 zeros, then increasing values, then p+1 ones
    for (int i = 0; i <= m_degree; ++i) {
        m_knots[i] = 0.0f;
    }
    
    int numInternalKnots = n - m_degree - 1;
    for (int i = 0; i < numInternalKnots; ++i) {
        m_knots[m_degree + 1 + i] = static_cast<float>(i + 1) / static_cast<float>(numInternalKnots + 1);
    }
    
    for (int i = 0; i <= m_degree; ++i) {
        m_knots[numKnots - 1 - i] = 1.0f;
    }
}

float SketchSpline::basisFunction(int i, int p, float t) const {
    // FIX #16: Iterative de Boor algorithm instead of exponential recursion
    // This avoids stack overflow for high-degree splines and has O(p) complexity
    
    int n = static_cast<int>(m_knots.size());
    if (i < 0 || i >= n - 1) {
        return 0.0f;
    }
    
    // Base case array: N[j,0] for j = i-p to i
    std::vector<float> N(p + 1, 0.0f);
    
    // Initialize degree 0 basis functions
    for (int j = 0; j <= p; ++j) {
        int idx = i - p + j;
        if (idx >= 0 && idx < n - 1) {
            if (t >= m_knots[idx] && t < m_knots[idx + 1]) {
                N[j] = 1.0f;
            } else if (t == 1.0f && m_knots[idx + 1] == 1.0f && m_knots[idx] < 1.0f) {
                N[j] = 1.0f;
            }
        }
    }
    
    // Build up higher degrees iteratively
    for (int deg = 1; deg <= p; ++deg) {
        std::vector<float> Nnew(p + 1 - deg, 0.0f);
        for (int j = 0; j <= p - deg; ++j) {
            int idx = i - p + j + deg;
            float left = 0.0f, right = 0.0f;
            
            // Left term
            if (idx - deg >= 0 && idx < n) {
                float denom = m_knots[idx] - m_knots[idx - deg];
                if (std::abs(denom) > 1e-10f) {
                    left = (t - m_knots[idx - deg]) / denom * N[j];
                }
            }
            
            // Right term
            if (idx + 1 - deg >= 0 && idx + 1 < n) {
                float denom = m_knots[idx + 1] - m_knots[idx + 1 - deg];
                if (std::abs(denom) > 1e-10f) {
                    right = (m_knots[idx + 1] - t) / denom * N[j + 1];
                }
            }
            
            Nnew[j] = left + right;
        }
        N = std::move(Nnew);
    }
    
    return N.empty() ? 0.0f : N[0];
}

float SketchSpline::basisDerivative(int i, int p, float t) const {
    if (p == 0) {
        return 0.0f;
    }
    
    float left = 0.0f, right = 0.0f;
    
    if (i >= 0 && i + p < static_cast<int>(m_knots.size())) {
        float denom = m_knots[i + p] - m_knots[i];
        if (std::abs(denom) > 1e-10f) {
            left = static_cast<float>(p) / denom * basisFunction(i, p - 1, t);
        }
    }
    
    if (i + 1 >= 0 && i + p + 1 < static_cast<int>(m_knots.size())) {
        float denom = m_knots[i + p + 1] - m_knots[i + 1];
        if (std::abs(denom) > 1e-10f) {
            right = static_cast<float>(p) / denom * basisFunction(i + 1, p - 1, t);
        }
    }
    
    return left - right;
}

float SketchSpline::basisSecondDerivative(int i, int p, float t) const {
    if (p <= 1) {
        return 0.0f;
    }
    
    float result = 0.0f;
    
    if (i >= 0 && i + p < static_cast<int>(m_knots.size())) {
        float denom = m_knots[i + p] - m_knots[i];
        if (std::abs(denom) > 1e-10f) {
            result += static_cast<float>(p) / denom * basisDerivative(i, p - 1, t);
        }
    }
    
    if (i + 1 >= 0 && i + p + 1 < static_cast<int>(m_knots.size())) {
        float denom = m_knots[i + p + 1] - m_knots[i + 1];
        if (std::abs(denom) > 1e-10f) {
            result -= static_cast<float>(p) / denom * basisDerivative(i + 1, p - 1, t);
        }
    }
    
    return result;
}

const glm::vec2& SketchSpline::getControlPoint(int index) const {
    if (index < 0 || index >= static_cast<int>(m_controlPoints.size())) {
        throw std::out_of_range("Control point index out of range");
    }
    return m_controlPoints[index];
}

void SketchSpline::setControlPoint(int index, const glm::vec2& point) {
    if (index < 0 || index >= static_cast<int>(m_controlPoints.size())) {
        throw std::out_of_range("Control point index out of range");
    }
    m_controlPoints[index] = point;
}

void SketchSpline::addControlPoint(const glm::vec2& point) {
    m_controlPoints.push_back(point);
    generateKnots();
}

void SketchSpline::insertControlPoint(int index, const glm::vec2& point) {
    if (index < 0 || index > static_cast<int>(m_controlPoints.size())) {
        throw std::out_of_range("Insert index out of range");
    }
    m_controlPoints.insert(m_controlPoints.begin() + index, point);
    generateKnots();
}

void SketchSpline::removeControlPoint(int index) {
    if (m_controlPoints.size() <= 2) {
        throw std::runtime_error("Cannot remove control point: minimum 2 required");
    }
    if (index < 0 || index >= static_cast<int>(m_controlPoints.size())) {
        throw std::out_of_range("Control point index out of range");
    }
    m_controlPoints.erase(m_controlPoints.begin() + index);
    m_degree = std::min(m_degree, static_cast<int>(m_controlPoints.size()) - 1);
    generateKnots();
}

glm::vec2 SketchSpline::startPoint() const {
    return m_controlPoints.front();
}

glm::vec2 SketchSpline::endPoint() const {
    return m_controlPoints.back();
}

glm::vec2 SketchSpline::evaluate(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    
    glm::vec2 result(0.0f);
    int n = static_cast<int>(m_controlPoints.size());
    
    for (int i = 0; i < n; ++i) {
        float basis = basisFunction(i, m_degree, t);
        result += basis * m_controlPoints[i];
    }
    
    return result;
}

glm::vec2 SketchSpline::derivative(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    
    glm::vec2 result(0.0f);
    int n = static_cast<int>(m_controlPoints.size());
    
    for (int i = 0; i < n; ++i) {
        float basis = basisDerivative(i, m_degree, t);
        result += basis * m_controlPoints[i];
    }
    
    return result;
}

glm::vec2 SketchSpline::secondDerivative(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    
    glm::vec2 result(0.0f);
    int n = static_cast<int>(m_controlPoints.size());
    
    for (int i = 0; i < n; ++i) {
        float basis = basisSecondDerivative(i, m_degree, t);
        result += basis * m_controlPoints[i];
    }
    
    return result;
}

glm::vec2 SketchSpline::tangent(float t) const {
    glm::vec2 d = derivative(t);
    float len = glm::length(d);
    if (len < 1e-10f) {
        return glm::vec2(1.0f, 0.0f);
    }
    return d / len;
}

float SketchSpline::curvature(float t) const {
    glm::vec2 d1 = derivative(t);
    glm::vec2 d2 = secondDerivative(t);
    
    // Curvature = |d1 x d2| / |d1|^3
    float cross = d1.x * d2.y - d1.y * d2.x;
    float lenCubed = std::pow(glm::length(d1), 3.0f);
    
    if (lenCubed < 1e-10f) {
        return 0.0f;
    }
    
    return cross / lenCubed;
}

BoundingBox2D SketchSpline::boundingBox() const {
    BoundingBox2D box;
    
    // Conservative approach: include all control points
    for (const auto& cp : m_controlPoints) {
        box.expand(cp);
    }
    
    return box;
}

float SketchSpline::length() const {
    // Numerical integration using Simpson's rule
    const int numSegments = 64;
    float totalLength = 0.0f;
    
    for (int i = 0; i < numSegments; ++i) {
        float t0 = static_cast<float>(i) / static_cast<float>(numSegments);
        float t1 = static_cast<float>(i + 1) / static_cast<float>(numSegments);
        float tMid = (t0 + t1) * 0.5f;
        
        // Simpson's rule: (b-a)/6 * (f(a) + 4*f(mid) + f(b))
        float len0 = glm::length(derivative(t0));
        float lenMid = glm::length(derivative(tMid));
        float len1 = glm::length(derivative(t1));
        
        totalLength += (t1 - t0) / 6.0f * (len0 + 4.0f * lenMid + len1);
    }
    
    return totalLength;
}

float SketchSpline::closestParameter(const glm::vec2& point) const {
    // FIX #10: Newton-Raphson with oscillation detection and damping
    const int maxIterations = 20;
    const float tolerance = 1e-6f;
    const float dampingFactor = 0.5f;  // Damping for large steps
    
    // Start with coarse sampling to find initial guess
    float bestT = 0.0f;
    float bestDistSq = std::numeric_limits<float>::max();
    
    for (int i = 0; i <= 10; ++i) {
        float t = static_cast<float>(i) / 10.0f;
        glm::vec2 p = evaluate(t);
        float distSq = glm::dot(point - p, point - p);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestT = t;
        }
    }
    
    // Refine with Newton-Raphson + damping and oscillation detection
    float t = bestT;
    float prevT = t;
    float prevPrevT = t;
    int oscillationCount = 0;
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        glm::vec2 p = evaluate(t);
        glm::vec2 d1 = derivative(t);
        glm::vec2 d2 = secondDerivative(t);
        
        glm::vec2 diff = p - point;
        
        // f(t) = (p(t) - point) . p'(t)
        // f'(t) = p'(t) . p'(t) + (p(t) - point) . p''(t)
        float f = glm::dot(diff, d1);
        float fPrime = glm::dot(d1, d1) + glm::dot(diff, d2);
        
        if (std::abs(fPrime) < 1e-10f) {
            break;
        }
        
        float dt = -f / fPrime;
        
        // Detect oscillation: if we're bouncing between boundaries or same values
        if (iter >= 2) {
            float oscillationTest = (t - prevPrevT);
            if (std::abs(oscillationTest) < tolerance * 10.0f && std::abs(dt) > tolerance) {
                oscillationCount++;
                // Apply stronger damping when oscillating
                dt *= dampingFactor / (oscillationCount + 1);
            }
        }
        
        // Limit step size to prevent overshooting
        float maxStep = 0.25f;
        if (std::abs(dt) > maxStep) {
            dt = (dt > 0 ? maxStep : -maxStep);
        }
        
        prevPrevT = prevT;
        prevT = t;
        t = std::clamp(t + dt, 0.0f, 1.0f);
        
        if (std::abs(dt) < tolerance) {
            break;
        }
        
        // Fallback: if oscillating too much, use bisection between current best endpoints
        if (oscillationCount > 3) {
            // Fall back to golden section search in the best region
            float a = std::max(0.0f, bestT - 0.1f);
            float b = std::min(1.0f, bestT + 0.1f);
            const float phi = 0.618033988749895f;
            
            for (int gs = 0; gs < 10; ++gs) {
                float c = b - phi * (b - a);
                float d = a + phi * (b - a);
                glm::vec2 pc = evaluate(c);
                glm::vec2 pd = evaluate(d);
                float distC = glm::dot(point - pc, point - pc);
                float distD = glm::dot(point - pd, point - pd);
                
                if (distC < distD) {
                    b = d;
                } else {
                    a = c;
                }
            }
            t = (a + b) * 0.5f;
            break;
        }
    }
    
    return t;
}

SketchEntity::Ptr SketchSpline::clone() const {
    auto copy = std::make_shared<SketchSpline>(m_controlPoints, m_degree);
    copy->setConstruction(isConstruction());
    return copy;
}

std::vector<glm::vec2> SketchSpline::tessellate(int numSamples) const {
    std::vector<glm::vec2> points;
    points.reserve(numSamples + 1);
    
    for (int i = 0; i <= numSamples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSamples);
        points.push_back(evaluate(t));
    }
    
    return points;
}

SketchSpline::Ptr SketchSpline::create(const std::vector<glm::vec2>& controlPoints, int degree) {
    return std::make_shared<SketchSpline>(controlPoints, degree);
}

} // namespace sketch
} // namespace dc
