#include "SurfaceFit.h"
#include "QuadMesh.h"
#include "../nurbs/NurbsSurface.h"
#include "../nurbs/NurbsCurve.h"
#include "../mesh/TriangleMesh.h"
#include <algorithm>
#include <cmath>
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace dc {

SurfaceFitter::SurfaceFitter() = default;
SurfaceFitter::~SurfaceFitter() = default;

SurfaceFitResult SurfaceFitter::fitToPoints(const std::vector<glm::vec3>& points,
                                             const SurfaceFitParams& params) {
    SurfaceFitResult result;
    result.converged = false;
    m_cancelled = false;
    
    if (points.size() < 4) {
        result.message = "Need at least 4 points for surface fitting";
        return result;
    }
    
    // Compute bounding box and principal directions
    glm::vec3 minBound(std::numeric_limits<float>::max());
    glm::vec3 maxBound(std::numeric_limits<float>::lowest());
    glm::vec3 centroid(0);
    
    for (const auto& p : points) {
        minBound = glm::min(minBound, p);
        maxBound = glm::max(maxBound, p);
        centroid += p;
    }
    centroid /= static_cast<float>(points.size());
    
    // Use PCA for principal directions
    Eigen::Matrix3f covariance = Eigen::Matrix3f::Zero();
    for (const auto& p : points) {
        Eigen::Vector3f diff(p.x - centroid.x, p.y - centroid.y, p.z - centroid.z);
        covariance += diff * diff.transpose();
    }
    covariance /= static_cast<float>(points.size());
    
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(covariance);
    Eigen::Vector3f ev0 = solver.eigenvectors().col(2); // Largest eigenvalue
    Eigen::Vector3f ev1 = solver.eigenvectors().col(1);
    
    glm::vec3 uDir(ev0.x(), ev0.y(), ev0.z());
    glm::vec3 vDir(ev1.x(), ev1.y(), ev1.z());
    
    // Parameterize points
    std::vector<glm::vec2> uvParams = parameterizePoints(points, uDir, vDir, centroid);
    
    // Initial fit
    result.surface = solveLinearFit(points, uvParams, params);
    if (!result.surface) {
        result.message = "Linear fit failed";
        return result;
    }
    
    // Iterative refinement
    float prevDeviation = std::numeric_limits<float>::max();
    result.iterations = 0;
    
    for (int iter = 0; iter < params.maxIterations; ++iter) {
        if (m_cancelled) {
            result.message = "Cancelled";
            return result;
        }
        
        // Compute deviation
        result.maxDeviation = computeDeviation(*result.surface, points, uvParams);
        result.iterations = iter + 1;
        
        reportProgress(static_cast<float>(iter) / params.maxIterations, result.maxDeviation);
        
        // Check convergence
        if (result.maxDeviation < params.deviationTolerance) {
            result.converged = true;
            result.message = "Converged within tolerance";
            break;
        }
        
        if (std::abs(prevDeviation - result.maxDeviation) < params.convergenceThreshold) {
            result.converged = true;
            result.message = "Converged (deviation stable)";
            break;
        }
        
        prevDeviation = result.maxDeviation;
        
        // Adaptive refinement if enabled
        if (params.adaptiveRefinement && iter < params.maxRefinementLevel) {
            auto highDevRegions = findHighDeviationRegions(*result.surface, points, uvParams,
                                                           params.deviationTolerance);
            if (!highDevRegions.empty()) {
                result.surface = refineInRegions(*result.surface, highDevRegions);
            }
        }
    }
    
    // Compute final statistics
    float totalDev = 0;
    float sumSqDev = 0;
    for (size_t i = 0; i < points.size(); ++i) {
        glm::vec3 surfPoint = result.surface->evaluate(uvParams[i].x, uvParams[i].y);
        float dev = glm::length(surfPoint - points[i]);
        totalDev += dev;
        sumSqDev += dev * dev;
        result.maxDeviation = std::max(result.maxDeviation, dev);
    }
    result.averageDeviation = totalDev / points.size();
    result.rmsDeviation = std::sqrt(sumSqDev / points.size());
    
    return result;
}

SurfaceFitResult SurfaceFitter::fitToPointsWithNormals(const std::vector<glm::vec3>& points,
                                                        const std::vector<glm::vec3>& normals,
                                                        const SurfaceFitParams& params) {
    // Add normal constraints
    clearConstraints();
    
    // Sample subset of normals as constraints
    int stride = std::max(1, static_cast<int>(points.size() / 20));
    for (size_t i = 0; i < points.size(); i += stride) {
        FitConstraint c;
        c.position = points[i];
        c.normal = normals[i];
        c.type = BoundaryCondition::Tangent;
        c.weight = 0.5f;
        m_constraints.push_back(c);
    }
    
    return fitWithConstraints(points, params);
}

SurfaceFitResult SurfaceFitter::fitToMeshRegion(const TriangleMesh& mesh,
                                                 const std::vector<int>& faceIndices,
                                                 const SurfaceFitParams& params) {
    // Extract points from mesh region
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    
    std::unordered_set<int> vertexSet;
    for (int faceIdx : faceIndices) {
        if (faceIdx * 3 + 2 < static_cast<int>(indices.size())) {
            vertexSet.insert(indices[faceIdx * 3]);
            vertexSet.insert(indices[faceIdx * 3 + 1]);
            vertexSet.insert(indices[faceIdx * 3 + 2]);
        }
    }
    
    for (int vIdx : vertexSet) {
        points.push_back(vertices[vIdx].position);
        normals.push_back(vertices[vIdx].normal);
    }
    
    // Use parameterization based on mesh connectivity
    std::vector<glm::vec2> uvParams = parameterizeFromMesh(points, mesh, faceIndices);
    
    // Set boundary constraints from mesh edges
    // ... (detect boundary edges and add constraints)
    
    return fitToPointsWithNormals(points, normals, params);
}

SurfaceFitResult SurfaceFitter::fitToQuadMesh(const QuadMesh& mesh,
                                               const SurfaceFitParams& params) {
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    
    const auto& vertices = mesh.vertices();
    for (const auto& v : vertices) {
        points.push_back(v.position);
        normals.push_back(v.normal);
    }
    
    // Quad mesh provides better parameterization
    // Use the mesh's implicit UV structure
    
    return fitToPointsWithNormals(points, normals, params);
}

SurfaceFitResult SurfaceFitter::fitToCurveNetwork(
    const std::vector<std::shared_ptr<NurbsCurve>>& curves,
    const SurfaceFitParams& params) {
    
    SurfaceFitResult result;
    
    if (curves.size() < 2) {
        result.message = "Need at least 2 curves for network fitting";
        return result;
    }
    
    // Sample points from curves
    std::vector<glm::vec3> points;
    std::vector<glm::vec2> uvParams;
    
    // Assume curves form a grid pattern
    // First curves are in U direction, rest in V direction
    int uCurves = static_cast<int>(curves.size() / 2);
    int vCurves = static_cast<int>(curves.size()) - uCurves;
    
    for (int i = 0; i < uCurves; ++i) {
        float v = static_cast<float>(i) / (uCurves - 1);
        for (int j = 0; j <= 20; ++j) {
            float u = static_cast<float>(j) / 20.0f;
            glm::vec3 pt = curves[i]->evaluate(u);
            points.push_back(pt);
            uvParams.push_back(glm::vec2(u, v));
        }
    }
    
    // Add constraints at curve intersections
    // ... (find intersections and add position constraints)
    
    SurfaceFitParams adjustedParams = params;
    adjustedParams.uControlPoints = std::max(params.uControlPoints, uCurves + 2);
    adjustedParams.vControlPoints = std::max(params.vControlPoints, vCurves + 2);
    
    result.surface = solveLinearFit(points, uvParams, adjustedParams);
    
    if (result.surface) {
        result.converged = true;
        result.maxDeviation = computeDeviation(*result.surface, points, uvParams);
    }
    
    return result;
}

SurfaceFitResult SurfaceFitter::fitWithBoundaryCurves(
    const std::vector<glm::vec3>& points,
    const std::vector<std::shared_ptr<NurbsCurve>>& boundaries,
    const SurfaceFitParams& params) {
    
    // Add boundary curve constraints
    clearConstraints();
    
    if (boundaries.size() >= 4) {
        // Assume order: uMin, uMax, vMin, vMax
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j <= 10; ++j) {
                float t = static_cast<float>(j) / 10.0f;
                FitConstraint c;
                c.position = boundaries[i]->evaluate(t);
                c.type = BoundaryCondition::Position;
                c.weight = 1.0f;
                
                switch (i) {
                    case 0: c.uv = glm::vec2(t, 0); break;      // uMin (v=0)
                    case 1: c.uv = glm::vec2(t, 1); break;      // uMax (v=1)
                    case 2: c.uv = glm::vec2(0, t); break;      // vMin (u=0)
                    case 3: c.uv = glm::vec2(1, t); break;      // vMax (u=1)
                }
                
                // Also get tangent from curve
                glm::vec3 tangent = boundaries[i]->evaluateDerivative(t, 1);
                if (i < 2) {
                    c.tangentU = tangent;
                } else {
                    c.tangentV = tangent;
                }
                
                m_constraints.push_back(c);
            }
        }
    }
    
    return fitWithConstraints(points, params);
}

void SurfaceFitter::addConstraint(const FitConstraint& constraint) {
    m_constraints.push_back(constraint);
}

void SurfaceFitter::clearConstraints() {
    m_constraints.clear();
}

SurfaceFitResult SurfaceFitter::fitWithConstraints(const std::vector<glm::vec3>& points,
                                                    const SurfaceFitParams& params) {
    // Parameterize points first
    glm::vec3 centroid(0);
    for (const auto& p : points) centroid += p;
    centroid /= static_cast<float>(points.size());
    
    // Simple planar parameterization for now
    std::vector<glm::vec2> uvParams = parameterizePoints(points, 
                                                          glm::vec3(1, 0, 0),
                                                          glm::vec3(0, 0, 1),
                                                          centroid);
    
    // Build constraint system
    int numPoints = static_cast<int>(points.size());
    int numConstraints = static_cast<int>(m_constraints.size());
    int numCPs = params.uControlPoints * params.vControlPoints;
    
    // Set up linear system with constraints
    // A * x = b where x are control point positions
    
    int rows = numPoints + numConstraints;
    Eigen::MatrixXf A = Eigen::MatrixXf::Zero(rows, numCPs);
    Eigen::MatrixXf b = Eigen::MatrixXf::Zero(rows, 3);
    
    // Generate knot vectors
    std::vector<float> uKnots = NurbsSurface::generateUniformKnots(params.uControlPoints, params.uDegree);
    std::vector<float> vKnots = NurbsSurface::generateUniformKnots(params.vControlPoints, params.vDegree);
    
    // Fill in point equations
    for (int i = 0; i < numPoints; ++i) {
        float u = uvParams[i].x;
        float v = uvParams[i].y;
        
        // Compute basis function values
        for (int j = 0; j < params.uControlPoints; ++j) {
            float Nu = NurbsSurface::basisFunction(j, params.uDegree, u, uKnots);
            for (int k = 0; k < params.vControlPoints; ++k) {
                float Nv = NurbsSurface::basisFunction(k, params.vDegree, v, vKnots);
                A(i, j * params.vControlPoints + k) = Nu * Nv;
            }
        }
        
        b(i, 0) = points[i].x;
        b(i, 1) = points[i].y;
        b(i, 2) = points[i].z;
    }
    
    // Fill in constraint equations
    for (int c = 0; c < numConstraints; ++c) {
        const FitConstraint& con = m_constraints[c];
        int row = numPoints + c;
        
        float u = con.uv.x;
        float v = con.uv.y;
        
        for (int j = 0; j < params.uControlPoints; ++j) {
            float Nu = NurbsSurface::basisFunction(j, params.uDegree, u, uKnots);
            for (int k = 0; k < params.vControlPoints; ++k) {
                float Nv = NurbsSurface::basisFunction(k, params.vDegree, v, vKnots);
                A(row, j * params.vControlPoints + k) = Nu * Nv * con.weight;
            }
        }
        
        b(row, 0) = con.position.x * con.weight;
        b(row, 1) = con.position.y * con.weight;
        b(row, 2) = con.position.z * con.weight;
    }
    
    // Solve using least squares
    Eigen::MatrixXf x = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    
    // Build surface from solution
    std::vector<std::vector<glm::vec3>> controlPoints(params.uControlPoints,
                                                       std::vector<glm::vec3>(params.vControlPoints));
    
    for (int i = 0; i < params.uControlPoints; ++i) {
        for (int j = 0; j < params.vControlPoints; ++j) {
            int idx = i * params.vControlPoints + j;
            controlPoints[i][j] = glm::vec3(x(idx, 0), x(idx, 1), x(idx, 2));
        }
    }
    
    SurfaceFitResult result;
    result.surface = std::make_unique<NurbsSurface>(controlPoints, uKnots, vKnots,
                                                     params.uDegree, params.vDegree);
    result.maxDeviation = computeDeviation(*result.surface, points, uvParams);
    result.converged = true;
    
    return result;
}

SurfaceFitResult SurfaceFitter::refine(const NurbsSurface& surface,
                                        const std::vector<glm::vec3>& targetPoints,
                                        const SurfaceFitParams& params) {
    // Insert knots where deviation is high
    std::vector<glm::vec2> uvParams;
    
    // Find closest surface parameters for each target point
    for (const auto& p : targetPoints) {
        glm::vec2 uv = surface.findClosestParameter(p);
        uvParams.push_back(uv);
    }
    
    auto highDevRegions = findHighDeviationRegions(surface, targetPoints, uvParams,
                                                    params.deviationTolerance);
    
    SurfaceFitResult result;
    if (highDevRegions.empty()) {
        result.surface = std::make_unique<NurbsSurface>(surface);
        result.converged = true;
        result.maxDeviation = computeDeviation(*result.surface, targetPoints, uvParams);
    } else {
        result.surface = refineInRegions(surface, highDevRegions);
        result.maxDeviation = computeDeviation(*result.surface, targetPoints, uvParams);
        result.converged = result.maxDeviation < params.deviationTolerance;
    }
    
    return result;
}

SurfaceFitResult SurfaceFitter::refineAdaptive(const NurbsSurface& surface,
                                                const std::vector<glm::vec3>& targetPoints,
                                                float tolerance) {
    SurfaceFitParams params;
    params.deviationTolerance = tolerance;
    params.adaptiveRefinement = true;
    params.maxRefinementLevel = 5;
    
    auto currentSurface = std::make_unique<NurbsSurface>(surface);
    SurfaceFitResult result;
    
    for (int level = 0; level < params.maxRefinementLevel; ++level) {
        result = refine(*currentSurface, targetPoints, params);
        
        if (result.converged) break;
        
        currentSurface = std::move(result.surface);
    }
    
    return result;
}

std::vector<SurfaceFitResult> SurfaceFitter::fitMultiPatch(
    const TriangleMesh& mesh,
    const std::vector<std::vector<int>>& patches,
    int continuity,
    const SurfaceFitParams& params) {
    
    std::vector<SurfaceFitResult> results;
    
    // First pass: fit each patch independently
    for (const auto& patch : patches) {
        results.push_back(fitToMeshRegion(mesh, patch, params));
    }
    
    // Second pass: enforce continuity between adjacent patches
    if (continuity > 0) {
        // Find adjacent patches and adjust control points
        for (size_t i = 0; i < results.size(); ++i) {
            for (size_t j = i + 1; j < results.size(); ++j) {
                // Check if patches share an edge
                // Adjust control points for G1/G2 continuity
                // ... (implementation depends on patch adjacency detection)
            }
        }
    }
    
    return results;
}

void SurfaceFitter::setProgressCallback(FitProgressCallback callback) {
    m_progressCallback = callback;
}

void SurfaceFitter::cancel() {
    m_cancelled = true;
}

void SurfaceFitter::reportProgress(float progress, float deviation) {
    if (m_progressCallback) {
        m_progressCallback(progress, deviation);
    }
}

std::vector<glm::vec2> SurfaceFitter::parameterizePoints(const std::vector<glm::vec3>& points,
                                                          const glm::vec3& uDir,
                                                          const glm::vec3& vDir,
                                                          const glm::vec3& origin) {
    std::vector<glm::vec2> params;
    params.reserve(points.size());
    
    float minU = std::numeric_limits<float>::max();
    float maxU = std::numeric_limits<float>::lowest();
    float minV = std::numeric_limits<float>::max();
    float maxV = std::numeric_limits<float>::lowest();
    
    // Project points onto UV plane
    for (const auto& p : points) {
        glm::vec3 diff = p - origin;
        float u = glm::dot(diff, uDir);
        float v = glm::dot(diff, vDir);
        params.push_back(glm::vec2(u, v));
        
        minU = std::min(minU, u);
        maxU = std::max(maxU, u);
        minV = std::min(minV, v);
        maxV = std::max(maxV, v);
    }
    
    // Normalize to [0, 1]
    float rangeU = maxU - minU;
    float rangeV = maxV - minV;
    
    if (rangeU < 1e-6f) rangeU = 1.0f;
    if (rangeV < 1e-6f) rangeV = 1.0f;
    
    for (auto& p : params) {
        p.x = (p.x - minU) / rangeU;
        p.y = (p.y - minV) / rangeV;
    }
    
    return params;
}

std::vector<glm::vec2> SurfaceFitter::parameterizeFromMesh(const std::vector<glm::vec3>& points,
                                                            const TriangleMesh& mesh,
                                                            const std::vector<int>& faceIndices) {
    // Use Floater's mean value coordinates or similar
    // This is a simplified planar projection
    
    glm::vec3 centroid(0);
    glm::vec3 normal(0);
    
    for (const auto& p : points) {
        centroid += p;
    }
    centroid /= static_cast<float>(points.size());
    
    // Compute average normal
    const auto& indices = mesh.indices();
    const auto& vertices = mesh.vertices();
    
    for (int faceIdx : faceIndices) {
        if (faceIdx * 3 + 2 < static_cast<int>(indices.size())) {
            int i0 = indices[faceIdx * 3];
            int i1 = indices[faceIdx * 3 + 1];
            int i2 = indices[faceIdx * 3 + 2];
            
            glm::vec3 e1 = vertices[i1].position - vertices[i0].position;
            glm::vec3 e2 = vertices[i2].position - vertices[i0].position;
            normal += glm::cross(e1, e2);
        }
    }
    normal = glm::normalize(normal);
    
    // Build orthonormal basis
    glm::vec3 uDir, vDir;
    if (std::abs(normal.x) < 0.9f) {
        uDir = glm::normalize(glm::cross(normal, glm::vec3(1, 0, 0)));
    } else {
        uDir = glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0)));
    }
    vDir = glm::cross(normal, uDir);
    
    return parameterizePoints(points, uDir, vDir, centroid);
}

std::unique_ptr<NurbsSurface> SurfaceFitter::solveLinearFit(
    const std::vector<glm::vec3>& points,
    const std::vector<glm::vec2>& params,
    const SurfaceFitParams& fitParams) {
    
    int numPoints = static_cast<int>(points.size());
    int numCPs = fitParams.uControlPoints * fitParams.vControlPoints;
    
    // Generate knot vectors
    std::vector<float> uKnots = NurbsSurface::generateUniformKnots(fitParams.uControlPoints, fitParams.uDegree);
    std::vector<float> vKnots = NurbsSurface::generateUniformKnots(fitParams.vControlPoints, fitParams.vDegree);
    
    // Build matrix A where A(i, j) = N_j(u_i) for all basis functions
    Eigen::MatrixXf A(numPoints, numCPs);
    Eigen::MatrixXf b(numPoints, 3);
    
    for (int i = 0; i < numPoints; ++i) {
        float u = params[i].x;
        float v = params[i].y;
        
        for (int j = 0; j < fitParams.uControlPoints; ++j) {
            float Nu = NurbsSurface::basisFunction(j, fitParams.uDegree, u, uKnots);
            for (int k = 0; k < fitParams.vControlPoints; ++k) {
                float Nv = NurbsSurface::basisFunction(k, fitParams.vDegree, v, vKnots);
                A(i, j * fitParams.vControlPoints + k) = Nu * Nv;
            }
        }
        
        b(i, 0) = points[i].x;
        b(i, 1) = points[i].y;
        b(i, 2) = points[i].z;
    }
    
    // Add smoothing regularization
    if (fitParams.smoothingWeight > 0) {
        // Add second derivative smoothing terms
        // This is simplified - full implementation would add proper Laplacian terms
    }
    
    // Solve least squares
    Eigen::MatrixXf x = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    
    // Build control point grid
    std::vector<std::vector<glm::vec3>> controlPoints(fitParams.uControlPoints,
                                                       std::vector<glm::vec3>(fitParams.vControlPoints));
    
    for (int i = 0; i < fitParams.uControlPoints; ++i) {
        for (int j = 0; j < fitParams.vControlPoints; ++j) {
            int idx = i * fitParams.vControlPoints + j;
            controlPoints[i][j] = glm::vec3(x(idx, 0), x(idx, 1), x(idx, 2));
        }
    }
    
    // Apply boundary conditions
    applyBoundaryConditions(controlPoints, fitParams);
    
    return std::make_unique<NurbsSurface>(controlPoints, uKnots, vKnots,
                                           fitParams.uDegree, fitParams.vDegree);
}

void SurfaceFitter::applyBoundaryConditions(std::vector<std::vector<glm::vec3>>& controlPoints,
                                             const SurfaceFitParams& params) {
    // Apply various boundary conditions
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    // Implementation would enforce tangent/curvature conditions at boundaries
}

void SurfaceFitter::applySmoothingRegularization(std::vector<std::vector<glm::vec3>>& controlPoints,
                                                  float weight) {
    if (weight <= 0) return;
    
    int nu = static_cast<int>(controlPoints.size());
    int nv = static_cast<int>(controlPoints[0].size());
    
    // Simple Laplacian smoothing
    std::vector<std::vector<glm::vec3>> smoothed = controlPoints;
    
    for (int i = 1; i < nu - 1; ++i) {
        for (int j = 1; j < nv - 1; ++j) {
            glm::vec3 laplacian = controlPoints[i-1][j] + controlPoints[i+1][j] +
                                  controlPoints[i][j-1] + controlPoints[i][j+1] -
                                  4.0f * controlPoints[i][j];
            smoothed[i][j] = controlPoints[i][j] + weight * laplacian;
        }
    }
    
    controlPoints = smoothed;
}

float SurfaceFitter::computeDeviation(const NurbsSurface& surface,
                                       const std::vector<glm::vec3>& points,
                                       const std::vector<glm::vec2>& params) {
    float maxDev = 0;
    
    for (size_t i = 0; i < points.size(); ++i) {
        glm::vec3 surfPoint = surface.evaluate(params[i].x, params[i].y);
        float dev = glm::length(surfPoint - points[i]);
        maxDev = std::max(maxDev, dev);
    }
    
    return maxDev;
}

std::vector<glm::vec2> SurfaceFitter::findHighDeviationRegions(
    const NurbsSurface& surface,
    const std::vector<glm::vec3>& points,
    const std::vector<glm::vec2>& params,
    float threshold) {
    
    std::vector<glm::vec2> regions;
    
    for (size_t i = 0; i < points.size(); ++i) {
        glm::vec3 surfPoint = surface.evaluate(params[i].x, params[i].y);
        float dev = glm::length(surfPoint - points[i]);
        
        if (dev > threshold) {
            regions.push_back(params[i]);
        }
    }
    
    return regions;
}

std::unique_ptr<NurbsSurface> SurfaceFitter::refineInRegions(
    const NurbsSurface& surface,
    const std::vector<glm::vec2>& regions) {
    
    // Insert knots at high-deviation parameters
    auto refined = std::make_unique<NurbsSurface>(surface);
    
    std::unordered_set<float> uKnotsToInsert;
    std::unordered_set<float> vKnotsToInsert;
    
    for (const auto& uv : regions) {
        uKnotsToInsert.insert(uv.x);
        vKnotsToInsert.insert(uv.y);
    }
    
    // Insert knots
    for (float u : uKnotsToInsert) {
        refined->insertKnotU(u);
    }
    for (float v : vKnotsToInsert) {
        refined->insertKnotV(v);
    }
    
    return refined;
}

// Utility functions
namespace SurfaceFitUtils {

std::pair<int, int> estimateControlPointCount(const std::vector<glm::vec3>& points,
                                               int degree, float tolerance) {
    // Heuristic based on point count and tolerance
    int n = static_cast<int>(std::sqrt(points.size()));
    int cpCount = std::max(degree + 1, n / 3);
    cpCount = std::min(cpCount, 50); // Cap at reasonable number
    return {cpCount, cpCount};
}

std::vector<glm::vec2> computeChordLengthParameterization(const std::vector<glm::vec3>& points) {
    std::vector<glm::vec2> params;
    // Implementation for chord-length parameterization
    return params;
}

std::vector<glm::vec2> computeCentripetalParameterization(const std::vector<glm::vec3>& points) {
    std::vector<glm::vec2> params;
    // Implementation for centripetal parameterization
    return params;
}

float computeRMSDeviation(const NurbsSurface& surface,
                           const std::vector<glm::vec3>& points) {
    float sumSq = 0;
    for (const auto& p : points) {
        glm::vec2 uv = surface.findClosestParameter(p);
        glm::vec3 sp = surface.evaluate(uv.x, uv.y);
        sumSq += glm::dot(sp - p, sp - p);
    }
    return std::sqrt(sumSq / points.size());
}

float computeMaxDeviation(const NurbsSurface& surface,
                           const std::vector<glm::vec3>& points) {
    float maxDev = 0;
    for (const auto& p : points) {
        glm::vec2 uv = surface.findClosestParameter(p);
        glm::vec3 sp = surface.evaluate(uv.x, uv.y);
        maxDev = std::max(maxDev, glm::length(sp - p));
    }
    return maxDev;
}

float computeSurfaceEnergy(const NurbsSurface& surface) {
    // Compute strain energy (integral of squared second derivatives)
    float energy = 0;
    
    const int samples = 20;
    for (int i = 0; i <= samples; ++i) {
        for (int j = 0; j <= samples; ++j) {
            float u = static_cast<float>(i) / samples;
            float v = static_cast<float>(j) / samples;
            
            // Get second derivatives
            glm::vec3 duu = surface.evaluateDerivative(u, v, 2, 0);
            glm::vec3 dvv = surface.evaluateDerivative(u, v, 0, 2);
            glm::vec3 duv = surface.evaluateDerivative(u, v, 1, 1);
            
            energy += glm::dot(duu, duu) + 2.0f * glm::dot(duv, duv) + glm::dot(dvv, dvv);
        }
    }
    
    return energy / ((samples + 1) * (samples + 1));
}

float computeFairnessMetric(const NurbsSurface& surface) {
    // Compute fairness based on curvature variation
    return computeSurfaceEnergy(surface);
}

} // namespace SurfaceFitUtils

} // namespace dc
