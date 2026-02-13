#include "AutoSurface.h"
#include "QuadMesh.h"
#include "SurfaceFit.h"
#include "../mesh/TriangleMesh.h"
#include "../nurbs/NurbsSurface.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <chrono>

#ifdef HAVE_EIGEN
#include <Eigen/Dense>
#include <Eigen/Sparse>
#endif

namespace dc {

AutoSurface::AutoSurface() = default;
AutoSurface::~AutoSurface() = default;

std::unique_ptr<QuadMesh> AutoSurface::generateQuadMesh(const TriangleMesh& input,
                                                         const AutoSurfaceParams& params) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    setInput(input);
    detectFeatures(params);
    generateInitialQuadMesh(params);
    optimizeQuadMesh(params);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_metrics.processingTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return getQuadMesh();
}

std::vector<std::unique_ptr<NurbsSurface>> AutoSurface::generateSurfaces(
    const TriangleMesh& input,
    const AutoSurfaceParams& params) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    setInput(input);
    detectFeatures(params);
    generateInitialQuadMesh(params);
    optimizeQuadMesh(params);
    
    if (params.generateNurbs) {
        fitSurfaces(params);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_metrics.processingTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return getSurfaces();
}

void AutoSurface::setInput(const TriangleMesh& mesh) {
    m_inputMesh = std::make_shared<TriangleMesh>(mesh);
    m_featureEdges.clear();
    m_featurePoints.clear();
    m_quadMesh.reset();
    m_surfaces.clear();
    m_cancelled = false;
    m_metrics = {};
}

void AutoSurface::detectFeatures(const AutoSurfaceParams& params) {
    if (!m_inputMesh || m_cancelled) return;
    
    reportProgress(0.0f, "Detecting features");
    
    if (params.detectCreases) {
        detectSharpEdges(params.featureAngleThreshold);
    }
    
    reportProgress(0.3f, "Detecting corners");
    
    if (params.detectCorners) {
        detectCorners();
    }
    
    reportProgress(0.5f, "Computing curvatures");
    
    computePrincipalCurvatures();
    classifyFeaturePoints();
    
    reportProgress(1.0f, "Feature detection complete");
}

void AutoSurface::detectSharpEdges(float angleThreshold) {
    const auto& vertices = m_inputMesh->vertices();
    const auto& indices = m_inputMesh->indices();
    
    // Build edge-to-face map
    std::unordered_map<uint64_t, std::vector<int>> edgeFaces;
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        int faceIdx = static_cast<int>(i / 3);
        for (int j = 0; j < 3; ++j) {
            int v0 = indices[i + j];
            int v1 = indices[i + (j + 1) % 3];
            uint64_t key = (static_cast<uint64_t>(std::min(v0, v1)) << 32) | 
                           static_cast<uint64_t>(std::max(v0, v1));
            edgeFaces[key].push_back(faceIdx);
        }
    }
    
    float cosThreshold = std::cos(angleThreshold * 3.14159265f / 180.0f);
    
    // Check each edge
    for (const auto& [key, faces] : edgeFaces) {
        if (faces.size() != 2) continue; // Skip boundary edges
        
        int v0 = static_cast<int>(key >> 32);
        int v1 = static_cast<int>(key & 0xFFFFFFFF);
        
        // Compute face normals
        auto computeNormal = [&](int faceIdx) -> glm::vec3 {
            int i0 = indices[faceIdx * 3];
            int i1 = indices[faceIdx * 3 + 1];
            int i2 = indices[faceIdx * 3 + 2];
            glm::vec3 e1 = vertices[i1].position - vertices[i0].position;
            glm::vec3 e2 = vertices[i2].position - vertices[i0].position;
            return glm::normalize(glm::cross(e1, e2));
        };
        
        glm::vec3 n0 = computeNormal(faces[0]);
        glm::vec3 n1 = computeNormal(faces[1]);
        
        float dot = glm::dot(n0, n1);
        if (dot < cosThreshold) {
            FeatureEdge fe;
            fe.vertex0 = v0;
            fe.vertex1 = v1;
            fe.sharpness = 1.0f - (dot + 1.0f) * 0.5f; // Map [-1,1] to [1,0]
            fe.direction = glm::normalize(vertices[v1].position - vertices[v0].position);
            m_featureEdges.push_back(fe);
        }
    }
}

void AutoSurface::detectCorners() {
    // Count feature edges per vertex
    std::unordered_map<int, int> vertexFeatureCount;
    
    for (const auto& edge : m_featureEdges) {
        vertexFeatureCount[edge.vertex0]++;
        vertexFeatureCount[edge.vertex1]++;
    }
    
    const auto& vertices = m_inputMesh->vertices();
    
    for (const auto& [vertexIdx, count] : vertexFeatureCount) {
        if (count >= 3) { // Corner where 3+ feature edges meet
            FeaturePoint fp;
            fp.vertexIdx = vertexIdx;
            fp.position = vertices[vertexIdx].position;
            fp.importance = static_cast<float>(count) / 3.0f;
            fp.targetValence = count; // Match feature edge count
            m_featurePoints.push_back(fp);
        }
    }
}

void AutoSurface::computePrincipalCurvatures() {
    // Compute principal curvature directions at each vertex
    // using the shape operator (Weingarten map)
    
    const auto& vertices = m_inputMesh->vertices();
    m_orientationField.resize(vertices.size());
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        auto [dir1, dir2] = computePrincipalDirections(static_cast<int>(i));
        m_orientationField[i] = dir1; // Primary direction
    }
}

std::pair<glm::vec3, glm::vec3> AutoSurface::computePrincipalDirections(int vertexIdx) const {
    // Simplified principal direction estimation using neighboring vertices
    const auto& vertices = m_inputMesh->vertices();
    const auto& indices = m_inputMesh->indices();
    
    // Find one-ring neighbors
    std::vector<int> neighbors;
    for (size_t i = 0; i < indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            if (indices[i + j] == vertexIdx) {
                neighbors.push_back(indices[i + (j + 1) % 3]);
                neighbors.push_back(indices[i + (j + 2) % 3]);
            }
        }
    }
    
    // Remove duplicates
    std::sort(neighbors.begin(), neighbors.end());
    neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    
    if (neighbors.size() < 3) {
        return {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)};
    }
    
    // Compute covariance matrix in tangent plane
    glm::vec3 center = vertices[vertexIdx].position;
    glm::vec3 normal = vertices[vertexIdx].normal;
    
#ifdef HAVE_EIGEN
    Eigen::Matrix3f cov = Eigen::Matrix3f::Zero();
    for (int n : neighbors) {
        glm::vec3 diff = vertices[n].position - center;
        // Project to tangent plane
        diff = diff - glm::dot(diff, normal) * normal;
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                cov(i, j) += diff[i] * diff[j];
            }
        }
    }
    cov /= static_cast<float>(neighbors.size());
    
    // Eigendecomposition
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(cov);
    Eigen::Vector3f ev1 = solver.eigenvectors().col(2);
    Eigen::Vector3f ev2 = solver.eigenvectors().col(1);
    
    return {glm::vec3(ev1.x(), ev1.y(), ev1.z()),
            glm::vec3(ev2.x(), ev2.y(), ev2.z())};
#else
    // Fallback without Eigen: use simple tangent frame from normal
    glm::vec3 up(0, 1, 0);
    if (std::abs(glm::dot(normal, up)) > 0.99f) {
        up = glm::vec3(1, 0, 0);
    }
    glm::vec3 tangent1 = glm::normalize(glm::cross(normal, up));
    glm::vec3 tangent2 = glm::cross(normal, tangent1);
    return {tangent1, tangent2};
#endif
}

void AutoSurface::classifyFeaturePoints() {
    // Add high-curvature points as feature points
    const auto& vertices = m_inputMesh->vertices();
    
    // Compute Gaussian curvature at each vertex
    std::vector<float> curvatures(vertices.size());
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        // Simplified: use angle defect for Gaussian curvature
        // Full implementation would use proper discrete differential geometry
        curvatures[i] = 0.0f; // Placeholder
    }
    
    // Find local maxima
    float maxCurv = *std::max_element(curvatures.begin(), curvatures.end());
    float threshold = maxCurv * 0.5f;
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (curvatures[i] > threshold) {
            // Check if already a feature point
            bool exists = false;
            for (const auto& fp : m_featurePoints) {
                if (fp.vertexIdx == static_cast<int>(i)) {
                    exists = true;
                    break;
                }
            }
            
            if (!exists) {
                FeaturePoint fp;
                fp.vertexIdx = static_cast<int>(i);
                fp.position = vertices[i].position;
                // Fix: Guard against division by zero when maxCurv is zero
                if (maxCurv > 1e-6f) {
                    fp.importance = curvatures[i] / maxCurv;
                } else {
                    fp.importance = 0.0f;
                }
                fp.targetValence = 4; // Regular for curvature features
                m_featurePoints.push_back(fp);
            }
        }
    }
}

void AutoSurface::generateInitialQuadMesh(const AutoSurfaceParams& params) {
    if (!m_inputMesh || m_cancelled) return;
    
    reportProgress(0.0f, "Computing orientation field");
    computeOrientationField();
    
    if (m_cancelled) return;
    
    reportProgress(0.3f, "Computing position field");
    computePositionField();
    
    if (m_cancelled) return;
    
    reportProgress(0.6f, "Extracting quad mesh");
    extractQuadMesh(params.targetPatchCount);
    
    if (m_cancelled) return;
    
    reportProgress(0.8f, "Aligning to features");
    alignToFeatures();
    
    reportProgress(1.0f, "Initial quad mesh complete");
}

void AutoSurface::computeOrientationField() {
    // Compute a smooth 4-rotationally symmetric field on the mesh
    // This guides the quad edge directions
    
    const auto& vertices = m_inputMesh->vertices();
    const auto& indices = m_inputMesh->indices();
    int numFaces = static_cast<int>(indices.size() / 3);
    
    // Initialize from principal curvature directions (already computed)
    // Now smooth the field across the mesh
    
    // Use a simple diffusion approach
    for (int iter = 0; iter < 10; ++iter) {
        if (m_cancelled) return;
        
        std::vector<glm::vec3> newField = m_orientationField;
        
        // For each vertex, average with neighbors (in 4-symmetric sense)
        for (size_t i = 0; i < vertices.size(); ++i) {
            // Find neighbors
            std::vector<int> neighbors;
            for (size_t j = 0; j < indices.size(); j += 3) {
                for (int k = 0; k < 3; ++k) {
                    if (indices[j + k] == static_cast<int>(i)) {
                        neighbors.push_back(indices[j + (k + 1) % 3]);
                        neighbors.push_back(indices[j + (k + 2) % 3]);
                    }
                }
            }
            
            if (neighbors.empty()) continue;
            
            // Average directions (need to handle 4-symmetry)
            glm::vec3 sum = m_orientationField[i] * 2.0f;
            for (int n : neighbors) {
                // Find best matching rotation
                glm::vec3 dir = m_orientationField[n];
                glm::vec3 ref = m_orientationField[i];
                
                // Check 4 rotations (0, 90, 180, 270 degrees around normal)
                float bestDot = glm::dot(dir, ref);
                glm::vec3 bestDir = dir;
                
                glm::vec3 normal = vertices[i].normal;
                glm::vec3 rotated = glm::cross(normal, dir);
                if (glm::dot(rotated, ref) > bestDot) {
                    bestDot = glm::dot(rotated, ref);
                    bestDir = rotated;
                }
                rotated = -dir;
                if (glm::dot(rotated, ref) > bestDot) {
                    bestDot = glm::dot(rotated, ref);
                    bestDir = rotated;
                }
                rotated = -glm::cross(normal, dir);
                if (glm::dot(rotated, ref) > bestDot) {
                    bestDir = rotated;
                }
                
                sum += bestDir;
            }
            
            float len = glm::length(sum);
            if (len > 1e-6f) {
                newField[i] = sum / len;
            }
        }
        
        m_orientationField = newField;
    }
}

void AutoSurface::computePositionField() {
    // Compute a position field (UV coordinates) that aligns with orientation
    // This gives us the quad grid positions
    
    const auto& vertices = m_inputMesh->vertices();
    m_positionField.resize(vertices.size(), glm::vec2(0));
    
    // Simple parameterization based on geodesic distances
    // Full implementation would use the method from "Instant Meshes"
    
    // Initialize with a Laplacian-based parameterization
    int n = static_cast<int>(vertices.size());
    
#ifdef HAVE_EIGEN
    // Build graph Laplacian
    Eigen::SparseMatrix<float> L(n, n);
    std::vector<Eigen::Triplet<float>> triplets;
    
    const auto& indices = m_inputMesh->indices();
    std::vector<std::vector<int>> adjacency(n);
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            int v0 = indices[i + j];
            int v1 = indices[i + (j + 1) % 3];
            adjacency[v0].push_back(v1);
            adjacency[v1].push_back(v0);
        }
    }
    
    for (int i = 0; i < n; ++i) {
        // Remove duplicates
        std::sort(adjacency[i].begin(), adjacency[i].end());
        adjacency[i].erase(std::unique(adjacency[i].begin(), adjacency[i].end()), adjacency[i].end());
        
        int degree = static_cast<int>(adjacency[i].size());
        triplets.emplace_back(i, i, static_cast<float>(degree));
        
        for (int j : adjacency[i]) {
            triplets.emplace_back(i, j, -1.0f);
        }
    }
    
    L.setFromTriplets(triplets.begin(), triplets.end());
    
    // Solve for UV using sparse solver
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<float>> solver(L);
    
    // Check solver status before using
    if (solver.info() != Eigen::Success) {
        // Handle degenerate case - fall back to simple parameterization
        for (int i = 0; i < n; ++i) {
            m_positionField[i] = glm::vec2(
                static_cast<float>(i % 10) / 10.0f,
                static_cast<float>(i / 10) / 10.0f
            );
        }
        return;
    }
    
    // Create RHS based on orientation field
    Eigen::VectorXf rhsU = Eigen::VectorXf::Zero(n);
    Eigen::VectorXf rhsV = Eigen::VectorXf::Zero(n);
    
    // Simple initialization based on orientation
    for (int i = 0; i < n; ++i) {
        glm::vec3 dir = m_orientationField[i];
        rhsU(i) = dir.x;
        rhsV(i) = dir.z;
    }
    
    // Solve (this is simplified - real implementation needs constraints)
    Eigen::VectorXf u = solver.solve(rhsU);
    Eigen::VectorXf v = solver.solve(rhsV);
    
    for (int i = 0; i < n; ++i) {
        m_positionField[i] = glm::vec2(u(i), v(i));
    }
#else
    // Fallback without Eigen: simple planar projection
    for (int i = 0; i < n; ++i) {
        const auto& pos = vertices[i].position;
        // Simple XZ projection as UV
        m_positionField[i] = glm::vec2(pos.x, pos.z);
    }
#endif
}

void AutoSurface::extractQuadMesh(int targetPatchCount) {
    // Extract quad mesh by tracing iso-lines of the position field
    
    m_quadMesh = std::make_unique<QuadMesh>();
    
    const auto& vertices = m_inputMesh->vertices();
    
    // Fix: Use targetPatchCount parameter instead of uninitialized m_metrics.patchCount
    // m_metrics.patchCount is only set AFTER optimization, so use the target value here
    float targetPatches = static_cast<float>(targetPatchCount > 0 ? targetPatchCount : 100);
    float scale = std::sqrt(static_cast<float>(vertices.size()) / targetPatches);
    
    // Round position field values to grid
    std::vector<glm::ivec2> gridCoords(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        gridCoords[i].x = static_cast<int>(std::round(m_positionField[i].x * scale));
        gridCoords[i].y = static_cast<int>(std::round(m_positionField[i].y * scale));
    }
    
    // Group vertices by grid cell
    std::unordered_map<uint64_t, std::vector<int>> cellVertices;
    for (size_t i = 0; i < vertices.size(); ++i) {
        uint64_t key = (static_cast<uint64_t>(gridCoords[i].x + 10000) << 32) | 
                       static_cast<uint64_t>(gridCoords[i].y + 10000);
        cellVertices[key].push_back(static_cast<int>(i));
    }
    
    // Create quad mesh vertices (average position per cell)
    std::unordered_map<uint64_t, int> cellToQuadVertex;
    for (const auto& [key, verts] : cellVertices) {
        glm::vec3 avgPos(0);
        for (int v : verts) {
            avgPos += vertices[v].position;
        }
        avgPos /= static_cast<float>(verts.size());
        
        int idx = m_quadMesh->addVertex(avgPos);
        cellToQuadVertex[key] = idx;
    }
    
    // Create quad faces by connecting adjacent cells
    std::unordered_set<uint64_t> addedFaces;
    for (const auto& [key, _] : cellVertices) {
        int x = static_cast<int>((key >> 32) - 10000);
        int y = static_cast<int>((key & 0xFFFFFFFF) - 10000);
        
        // Check for quad (x,y), (x+1,y), (x+1,y+1), (x,y+1)
        uint64_t k00 = key;
        uint64_t k10 = (static_cast<uint64_t>(x + 1 + 10000) << 32) | static_cast<uint64_t>(y + 10000);
        uint64_t k11 = (static_cast<uint64_t>(x + 1 + 10000) << 32) | static_cast<uint64_t>(y + 1 + 10000);
        uint64_t k01 = (static_cast<uint64_t>(x + 10000) << 32) | static_cast<uint64_t>(y + 1 + 10000);
        
        if (cellToQuadVertex.count(k00) && cellToQuadVertex.count(k10) &&
            cellToQuadVertex.count(k11) && cellToQuadVertex.count(k01)) {
            
            uint64_t faceKey = std::min({k00, k10, k11, k01});
            if (!addedFaces.count(faceKey)) {
                m_quadMesh->addFace({
                    cellToQuadVertex[k00],
                    cellToQuadVertex[k10],
                    cellToQuadVertex[k11],
                    cellToQuadVertex[k01]
                });
                addedFaces.insert(faceKey);
            }
        }
    }
    
    m_quadMesh->buildTopology();
}

void AutoSurface::alignToFeatures() {
    if (!m_quadMesh) return;
    
    // Snap quad mesh edges to feature edges
    for (const auto& fe : m_featureEdges) {
        // Find closest quad mesh edge and mark as crease
        // Simplified implementation
    }
    
    // Ensure feature points have correct valence
    for (const auto& fp : m_featurePoints) {
        // Find closest quad mesh vertex
        // Adjust topology if needed
    }
}

void AutoSurface::optimizeQuadMesh(const AutoSurfaceParams& params) {
    if (!m_quadMesh || m_cancelled) return;
    
    reportProgress(0.0f, "Optimizing vertex positions");
    optimizeVertexPositions(params.maxIterations);
    
    if (m_cancelled) return;
    
    reportProgress(0.5f, "Optimizing connectivity");
    optimizeConnectivity();
    
    if (m_cancelled) return;
    
    reportProgress(0.7f, "Removing degenerate faces");
    removeDegenerateFaces();
    
    if (m_cancelled) return;
    
    reportProgress(0.9f, "Projecting to original mesh");
    projectToOriginalMesh();
    
    // Compute quality metrics
    auto quality = m_quadMesh->computeQuality();
    m_metrics.quadPercentage = quality.quadPercentage;
    m_metrics.singularityCount = quality.irregularVertices;
    m_metrics.patchCount = m_quadMesh->faceCount();
    
    reportProgress(1.0f, "Optimization complete");
}

void AutoSurface::optimizeVertexPositions(int iterations) {
    // Iteratively move vertices to improve quad quality
    for (int iter = 0; iter < iterations; ++iter) {
        if (m_cancelled) return;
        
        m_quadMesh->relaxVertices(m_quadMesh->getSelectedVertices(), 1);
        projectToOriginalMesh();
        
        // Check convergence
        auto quality = m_quadMesh->computeQuality();
        // Could track improvement and early-exit
    }
}

void AutoSurface::optimizeConnectivity() {
    // Perform edge flips to improve quad quality
    // This is a simplified version - full implementation would use
    // proper quad mesh optimization algorithms
}

void AutoSurface::removeDegenerateFaces() {
    // Remove faces with near-zero area or very bad angles
    auto quality = m_quadMesh->computeQuality();
    // Implementation would iterate through faces and collapse bad ones
}

void AutoSurface::projectToOriginalMesh() {
    if (!m_quadMesh || !m_inputMesh) return;
    
    // Project each quad mesh vertex to closest point on original mesh
    // Simplified: just snap to nearest vertex
    const auto& origVerts = m_inputMesh->vertices();
    
    for (int i = 0; i < m_quadMesh->vertexCount(); ++i) {
        glm::vec3 pos = m_quadMesh->vertices()[i].position;
        
        float minDist = std::numeric_limits<float>::max();
        glm::vec3 closest = pos;
        
        for (const auto& v : origVerts) {
            float dist = glm::length(v.position - pos);
            if (dist < minDist) {
                minDist = dist;
                closest = v.position;
            }
        }
        
        // Move partway to maintain smoothness
        m_quadMesh->moveVertex(i, glm::mix(pos, closest, 0.5f));
    }
}

void AutoSurface::fitSurfaces(const AutoSurfaceParams& params) {
    if (!m_quadMesh || m_cancelled) return;
    
    reportProgress(0.0f, "Segmenting into patches");
    segmentIntoPatches();
    
    if (m_cancelled) return;
    
    reportProgress(0.3f, "Fitting NURBS surfaces");
    fitPatchSurfaces(params.nurbsDegree);
    
    if (m_cancelled) return;
    
    reportProgress(0.7f, "Ensuring continuity");
    ensureContinuity(params.targetContinuity);
    
    // Compute deviation metrics
    float totalDev = 0, maxDev = 0;
    int sampleCount = 0;
    
    // Sample points and compute deviation
    // ... (simplified)
    
    m_metrics.averageDeviation = sampleCount > 0 ? totalDev / sampleCount : 0;
    m_metrics.maxDeviation = maxDev;
    
    reportProgress(1.0f, "Surface fitting complete");
}

void AutoSurface::segmentIntoPatches() {
    // Group quad faces into patches for NURBS fitting
    // Each patch should be a rectangular grid of quads
}

void AutoSurface::fitPatchSurfaces(int degree) {
    // Fit a NURBS surface to each patch
    SurfaceFitter fitter;
    
    // For each patch, extract control points and fit surface
    // ... (implementation would use SurfaceFit class)
}

void AutoSurface::ensureContinuity(int targetContinuity) {
    // Adjust control points to ensure G0/G1/G2 continuity between patches
}

void AutoSurface::setProgressCallback(AutoSurfaceProgressCallback callback) {
    m_progressCallback = callback;
}

void AutoSurface::cancel() {
    m_cancelled = true;
}

void AutoSurface::reportProgress(float progress, const std::string& stage) {
    // Fix: Wrap callback in try-catch for defensive coding - callback could throw
    if (m_progressCallback) {
        try {
            m_progressCallback(progress, stage);
        } catch (...) {
            // Silently ignore callback exceptions to avoid interrupting processing
            // The caller's callback is responsible for its own error handling
        }
    }
}

std::unique_ptr<QuadMesh> AutoSurface::getQuadMesh() {
    return std::move(m_quadMesh);
}

std::vector<std::unique_ptr<NurbsSurface>> AutoSurface::getSurfaces() {
    return std::move(m_surfaces);
}

// Utility functions
namespace AutoSurfaceUtils {

int estimatePatchCount(const TriangleMesh& mesh, float detailLevel) {
    int vertexCount = mesh.vertexCount();
    float factor = glm::clamp(detailLevel, 0.1f, 1.0f);
    return static_cast<int>(std::sqrt(vertexCount) * factor * 10);
}

AutoSurfaceParams suggestParameters(const TriangleMesh& mesh) {
    AutoSurfaceParams params;
    
    // Estimate complexity
    int verts = mesh.vertexCount();
    int tris = mesh.triangleCount();
    
    // Adjust patch count based on complexity
    params.targetPatchCount = std::max(50, std::min(500, static_cast<int>(std::sqrt(verts) * 5)));
    
    // Compute curvature to determine feature preservation
    float minC, maxC, avgC;
    computeCurvatureStats(mesh, minC, maxC, avgC);
    
    if (maxC > avgC * 3.0f) {
        params.featurePreservation = 0.9f;
        params.detectCreases = true;
    } else {
        params.featurePreservation = 0.5f;
    }
    
    return params;
}

bool validateParameters(const AutoSurfaceParams& params, std::string& error) {
    if (params.targetPatchCount < 1) {
        error = "Target patch count must be at least 1";
        return false;
    }
    if (params.deviationTolerance <= 0) {
        error = "Deviation tolerance must be positive";
        return false;
    }
    if (params.featureAngleThreshold < 0 || params.featureAngleThreshold > 180) {
        error = "Feature angle threshold must be between 0 and 180 degrees";
        return false;
    }
    return true;
}

void computeCurvatureStats(const TriangleMesh& mesh,
                           float& minCurvature, float& maxCurvature,
                           float& avgCurvature) {
    minCurvature = std::numeric_limits<float>::max();
    maxCurvature = 0;
    avgCurvature = 0;
    
    const auto& vertices = mesh.vertices();
    int count = 0;
    
    for (const auto& v : vertices) {
        // Use normal variation as curvature proxy
        // Full implementation would compute proper discrete curvature
        float curv = 0.0f; // Placeholder
        
        minCurvature = std::min(minCurvature, curv);
        maxCurvature = std::max(maxCurvature, curv);
        avgCurvature += curv;
        count++;
    }
    
    if (count > 0) {
        avgCurvature /= count;
    }
}

} // namespace AutoSurfaceUtils

} // namespace dc
