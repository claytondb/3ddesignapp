#include "QuadMesh.h"
#include "../nurbs/NurbsSurface.h"
#include "../mesh/TriangleMesh.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace dc {

QuadMesh::QuadMesh() = default;
QuadMesh::~QuadMesh() = default;

void QuadMesh::clear() {
    m_vertices.clear();
    m_faces.clear();
    m_halfEdges.clear();
    m_selectedVertices.clear();
    m_selectedFaces.clear();
    m_edgeMap.clear();
    m_creaseWeights.clear();
}

int QuadMesh::addVertex(const glm::vec3& position) {
    QuadVertex v;
    v.position = position;
    v.normal = glm::vec3(0, 1, 0);
    v.uv = glm::vec2(0, 0);
    v.halfEdgeIdx = -1;
    v.isCorner = false;
    v.cornerWeight = 0.0f;
    v.isBoundary = false;
    v.limitPosition = position;
    v.limitNormal = v.normal;
    
    int idx = static_cast<int>(m_vertices.size());
    m_vertices.push_back(v);
    return idx;
}

int QuadMesh::addFace(const std::vector<int>& vertexIndices) {
    if (vertexIndices.size() < 3 || vertexIndices.size() > 4) {
        return -1;
    }
    
    QuadFace face;
    face.vertexCount = static_cast<int>(vertexIndices.size());
    face.isSelected = false;
    
    int faceIdx = static_cast<int>(m_faces.size());
    int firstHalfEdge = static_cast<int>(m_halfEdges.size());
    face.halfEdgeIdx = firstHalfEdge;
    
    // Create half-edges for this face
    for (size_t i = 0; i < vertexIndices.size(); ++i) {
        HalfEdge he;
        he.vertexIdx = vertexIndices[(i + 1) % vertexIndices.size()];
        he.faceIdx = faceIdx;
        he.nextIdx = firstHalfEdge + static_cast<int>((i + 1) % vertexIndices.size());
        he.prevIdx = firstHalfEdge + static_cast<int>((i + vertexIndices.size() - 1) % vertexIndices.size());
        he.twinIdx = -1;
        he.isCrease = false;
        he.creaseWeight = 0.0f;
        
        int heIdx = static_cast<int>(m_halfEdges.size());
        m_halfEdges.push_back(he);
        
        // Update vertex's outgoing half-edge
        int fromVertex = vertexIndices[i];
        if (m_vertices[fromVertex].halfEdgeIdx == -1) {
            m_vertices[fromVertex].halfEdgeIdx = heIdx;
        }
        
        // Store edge for twin lookup
        uint64_t key = edgeKey(vertexIndices[i], vertexIndices[(i + 1) % vertexIndices.size()]);
        m_edgeMap[key] = heIdx;
    }
    
    // Compute face centroid and normal
    glm::vec3 centroid(0);
    for (int idx : vertexIndices) {
        centroid += m_vertices[idx].position;
    }
    face.centroid = centroid / static_cast<float>(vertexIndices.size());
    
    // Compute normal using first three vertices
    glm::vec3 v0 = m_vertices[vertexIndices[0]].position;
    glm::vec3 v1 = m_vertices[vertexIndices[1]].position;
    glm::vec3 v2 = m_vertices[vertexIndices[2]].position;
    face.normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    
    m_faces.push_back(face);
    return faceIdx;
}

void QuadMesh::buildTopology() {
    // Find twin half-edges
    for (size_t i = 0; i < m_halfEdges.size(); ++i) {
        if (m_halfEdges[i].twinIdx != -1) continue;
        
        int toVertex = m_halfEdges[i].vertexIdx;
        int fromVertex = m_halfEdges[m_halfEdges[i].prevIdx].vertexIdx;
        
        uint64_t twinKey = edgeKey(toVertex, fromVertex);
        auto it = m_edgeMap.find(twinKey);
        if (it != m_edgeMap.end()) {
            int twinIdx = it->second;
            // Verify this is actually the twin
            int twinTo = m_halfEdges[twinIdx].vertexIdx;
            int twinFrom = m_halfEdges[m_halfEdges[twinIdx].prevIdx].vertexIdx;
            if (twinFrom == toVertex && twinTo == fromVertex) {
                m_halfEdges[i].twinIdx = twinIdx;
                m_halfEdges[twinIdx].twinIdx = static_cast<int>(i);
            }
        }
    }
    
    // Mark boundary vertices
    for (auto& he : m_halfEdges) {
        if (he.twinIdx == -1) {
            int fromVertex = m_halfEdges[he.prevIdx].vertexIdx;
            m_vertices[fromVertex].isBoundary = true;
            m_vertices[he.vertexIdx].isBoundary = true;
        }
    }
    
    updateNormals();
}

uint64_t QuadMesh::edgeKey(int v0, int v1) const {
    return (static_cast<uint64_t>(v0) << 32) | static_cast<uint64_t>(v1);
}

int QuadMesh::findHalfEdge(int v0, int v1) const {
    uint64_t key = edgeKey(v0, v1);
    auto it = m_edgeMap.find(key);
    return (it != m_edgeMap.end()) ? it->second : -1;
}

std::vector<int> QuadMesh::getVertexFaces(int vertexIdx) const {
    std::vector<int> faces;
    int startHe = m_vertices[vertexIdx].halfEdgeIdx;
    if (startHe == -1) return faces;
    
    int he = startHe;
    do {
        faces.push_back(m_halfEdges[he].faceIdx);
        int twin = m_halfEdges[he].twinIdx;
        if (twin == -1) break;
        he = m_halfEdges[twin].nextIdx;
    } while (he != startHe);
    
    return faces;
}

std::vector<int> QuadMesh::getVertexNeighbors(int vertexIdx) const {
    std::vector<int> neighbors;
    int startHe = m_vertices[vertexIdx].halfEdgeIdx;
    if (startHe == -1) return neighbors;
    
    int he = startHe;
    do {
        neighbors.push_back(m_halfEdges[he].vertexIdx);
        int twin = m_halfEdges[he].twinIdx;
        if (twin == -1) break;
        he = m_halfEdges[twin].nextIdx;
    } while (he != startHe);
    
    return neighbors;
}

std::vector<int> QuadMesh::getFaceVertices(int faceIdx) const {
    std::vector<int> vertices;
    int startHe = m_faces[faceIdx].halfEdgeIdx;
    int he = startHe;
    do {
        vertices.push_back(m_halfEdges[he].vertexIdx);
        he = m_halfEdges[he].nextIdx;
    } while (he != startHe);
    
    return vertices;
}

int QuadMesh::getVertexValence(int vertexIdx) const {
    return static_cast<int>(getVertexNeighbors(vertexIdx).size());
}

// Catmull-Clark subdivision
std::unique_ptr<QuadMesh> QuadMesh::subdivide(int levels) const {
    if (levels <= 0) {
        auto copy = std::make_unique<QuadMesh>();
        copy->m_vertices = m_vertices;
        copy->m_faces = m_faces;
        copy->m_halfEdges = m_halfEdges;
        copy->m_edgeMap = m_edgeMap;
        copy->m_creaseWeights = m_creaseWeights;
        return copy;
    }
    
    auto result = std::make_unique<QuadMesh>();
    
    // Step 1: Compute face points
    std::vector<glm::vec3> facePoints(m_faces.size());
    for (size_t i = 0; i < m_faces.size(); ++i) {
        facePoints[i] = computeFacePoint(static_cast<int>(i));
    }
    
    // Step 2: Compute edge points
    std::vector<glm::vec3> edgePoints(m_halfEdges.size());
    std::unordered_set<uint64_t> processedEdges;
    for (size_t i = 0; i < m_halfEdges.size(); ++i) {
        int twin = m_halfEdges[i].twinIdx;
        if (twin != -1 && static_cast<size_t>(twin) < i) continue;
        edgePoints[i] = computeEdgePoint(static_cast<int>(i));
        if (twin != -1) {
            edgePoints[twin] = edgePoints[i];
        }
    }
    
    // Step 3: Compute new vertex positions
    std::vector<glm::vec3> newVertexPositions(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        // Check for crease vertex
        std::vector<int> creaseEdges;
        int startHe = m_vertices[i].halfEdgeIdx;
        if (startHe != -1) {
            int he = startHe;
            do {
                if (m_halfEdges[he].isCrease || m_halfEdges[he].twinIdx == -1) {
                    creaseEdges.push_back(he);
                }
                int twin = m_halfEdges[he].twinIdx;
                if (twin == -1) break;
                he = m_halfEdges[twin].nextIdx;
            } while (he != startHe);
        }
        
        if (m_vertices[i].isCorner || creaseEdges.size() > 2) {
            // Corner vertex: keep position
            newVertexPositions[i] = m_vertices[i].position;
        } else if (creaseEdges.size() == 2) {
            // Crease vertex: average of edge midpoints
            newVertexPositions[i] = computeCreaseVertexPoint(static_cast<int>(i), edgePoints);
        } else {
            // Regular vertex
            newVertexPositions[i] = computeVertexPoint(static_cast<int>(i), facePoints, edgePoints);
        }
    }
    
    // Step 4: Create new topology
    // Add original vertices (with new positions)
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        result->addVertex(newVertexPositions[i]);
    }
    
    // Add face points as vertices
    int facePointStart = result->vertexCount();
    for (const auto& fp : facePoints) {
        result->addVertex(fp);
    }
    
    // Add edge points as vertices (one per unique edge)
    int edgePointStart = result->vertexCount();
    std::unordered_map<uint64_t, int> edgeToVertex;
    for (size_t i = 0; i < m_halfEdges.size(); ++i) {
        int fromV = m_halfEdges[m_halfEdges[i].prevIdx].vertexIdx;
        int toV = m_halfEdges[i].vertexIdx;
        int minV = std::min(fromV, toV);
        int maxV = std::max(fromV, toV);
        uint64_t key = edgeKey(minV, maxV);
        
        if (edgeToVertex.find(key) == edgeToVertex.end()) {
            int newIdx = result->addVertex(edgePoints[i]);
            edgeToVertex[key] = newIdx;
        }
    }
    
    // Create new faces (subdivide each original face into quads)
    for (size_t f = 0; f < m_faces.size(); ++f) {
        int faceVertex = facePointStart + static_cast<int>(f);
        std::vector<int> faceVerts = getFaceVertices(static_cast<int>(f));
        
        for (size_t i = 0; i < faceVerts.size(); ++i) {
            int v0 = faceVerts[i];
            int v1 = faceVerts[(i + 1) % faceVerts.size()];
            int vPrev = faceVerts[(i + faceVerts.size() - 1) % faceVerts.size()];
            
            // Edge vertices
            uint64_t edgeKey0 = edgeKey(std::min(vPrev, v0), std::max(vPrev, v0));
            uint64_t edgeKey1 = edgeKey(std::min(v0, v1), std::max(v0, v1));
            
            int edgeV0 = edgeToVertex[edgeKey0];
            int edgeV1 = edgeToVertex[edgeKey1];
            
            // Create quad: corner, edge, face, edge
            result->addFace({v0, edgeV1, faceVertex, edgeV0});
        }
    }
    
    result->buildTopology();
    
    // Propagate crease weights (reduced by subdivision)
    for (const auto& [key, weight] : m_creaseWeights) {
        if (weight > 0.01f) {
            // Find corresponding edge in subdivided mesh and apply reduced weight
            float newWeight = std::max(0.0f, weight - 1.0f / static_cast<float>(levels));
            // Note: Edge mapping would need proper implementation
        }
    }
    
    // Recurse for additional levels
    if (levels > 1) {
        return result->subdivide(levels - 1);
    }
    
    return result;
}

glm::vec3 QuadMesh::computeFacePoint(int faceIdx) const {
    glm::vec3 sum(0);
    int count = 0;
    
    int startHe = m_faces[faceIdx].halfEdgeIdx;
    int he = startHe;
    do {
        sum += m_vertices[m_halfEdges[he].vertexIdx].position;
        count++;
        he = m_halfEdges[he].nextIdx;
    } while (he != startHe);
    
    return sum / static_cast<float>(count);
}

glm::vec3 QuadMesh::computeEdgePoint(int halfEdgeIdx) const {
    const HalfEdge& he = m_halfEdges[halfEdgeIdx];
    int v0 = m_halfEdges[he.prevIdx].vertexIdx;
    int v1 = he.vertexIdx;
    
    glm::vec3 edgeMid = (m_vertices[v0].position + m_vertices[v1].position) * 0.5f;
    
    // Boundary or crease edge: just midpoint
    if (he.twinIdx == -1 || he.isCrease) {
        return edgeMid;
    }
    
    // Interior edge: average of endpoints and adjacent face centers
    glm::vec3 f0 = computeFacePoint(he.faceIdx);
    glm::vec3 f1 = computeFacePoint(m_halfEdges[he.twinIdx].faceIdx);
    
    return (m_vertices[v0].position + m_vertices[v1].position + f0 + f1) * 0.25f;
}

glm::vec3 QuadMesh::computeVertexPoint(int vertexIdx,
                                        const std::vector<glm::vec3>& facePoints,
                                        const std::vector<glm::vec3>& edgePoints) const {
    std::vector<int> neighbors = getVertexNeighbors(vertexIdx);
    std::vector<int> faces = getVertexFaces(vertexIdx);
    int n = static_cast<int>(neighbors.size());
    
    if (m_vertices[vertexIdx].isBoundary) {
        // Boundary vertex: average of adjacent boundary edge midpoints
        glm::vec3 sum = m_vertices[vertexIdx].position * 6.0f;
        int boundaryCount = 0;
        int startHe = m_vertices[vertexIdx].halfEdgeIdx;
        int he = startHe;
        do {
            if (m_halfEdges[he].twinIdx == -1) {
                sum += m_vertices[m_halfEdges[he].vertexIdx].position;
                boundaryCount++;
            }
            int prevTwin = m_halfEdges[m_halfEdges[he].prevIdx].twinIdx;
            if (prevTwin == -1) {
                sum += m_vertices[m_halfEdges[m_halfEdges[he].prevIdx].vertexIdx].position;
                boundaryCount++;
                break;
            }
            he = m_halfEdges[prevTwin].nextIdx;
        } while (he != startHe);
        
        return sum / 8.0f;
    }
    
    // Interior vertex: Catmull-Clark formula
    // V' = (F + 2E + (n-3)V) / n
    // where F = average of face points, E = average of edge midpoints
    
    glm::vec3 F(0);
    for (int faceIdx : faces) {
        F += facePoints[faceIdx];
    }
    F /= static_cast<float>(faces.size());
    
    glm::vec3 E(0);
    for (int neighbor : neighbors) {
        E += (m_vertices[vertexIdx].position + m_vertices[neighbor].position) * 0.5f;
    }
    E /= static_cast<float>(n);
    
    glm::vec3 V = m_vertices[vertexIdx].position;
    
    return (F + 2.0f * E + static_cast<float>(n - 3) * V) / static_cast<float>(n);
}

glm::vec3 QuadMesh::computeCreaseVertexPoint(int vertexIdx,
                                              const std::vector<glm::vec3>& edgePoints) const {
    // For crease vertices: use edge rule
    // V' = (E0 + 6V + E1) / 8
    // where E0 and E1 are the crease edge midpoints
    
    glm::vec3 sum(0);
    int creaseCount = 0;
    
    int startHe = m_vertices[vertexIdx].halfEdgeIdx;
    int he = startHe;
    do {
        if (m_halfEdges[he].isCrease || m_halfEdges[he].twinIdx == -1) {
            sum += (m_vertices[vertexIdx].position + 
                   m_vertices[m_halfEdges[he].vertexIdx].position) * 0.5f;
            creaseCount++;
        }
        int twin = m_halfEdges[he].twinIdx;
        if (twin == -1) break;
        he = m_halfEdges[twin].nextIdx;
    } while (he != startHe);
    
    if (creaseCount == 2) {
        return (sum + 6.0f * m_vertices[vertexIdx].position) / 8.0f;
    }
    
    return m_vertices[vertexIdx].position;
}

void QuadMesh::computeLimitPositions() {
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        if (m_vertices[i].isBoundary || m_vertices[i].isCorner) {
            m_vertices[i].limitPosition = m_vertices[i].position;
            continue;
        }
        
        std::vector<int> neighbors = getVertexNeighbors(static_cast<int>(i));
        std::vector<int> faces = getVertexFaces(static_cast<int>(i));
        int n = static_cast<int>(neighbors.size());
        
        if (n == 0) {
            m_vertices[i].limitPosition = m_vertices[i].position;
            continue;
        }
        
        // Limit position formula for Catmull-Clark
        float n2 = static_cast<float>(n * n);
        float weight = n2 / (n2 + 5.0f * n);
        
        glm::vec3 Q(0); // Average of face points
        for (int faceIdx : faces) {
            Q += computeFacePoint(faceIdx);
        }
        Q /= static_cast<float>(faces.size());
        
        glm::vec3 R(0); // Average of edge midpoints
        for (int neighbor : neighbors) {
            R += (m_vertices[i].position + m_vertices[neighbor].position) * 0.5f;
        }
        R /= static_cast<float>(n);
        
        m_vertices[i].limitPosition = weight * m_vertices[i].position + 
                                      (1.0f - weight) * (Q + 2.0f * R) / 3.0f;
    }
}

void QuadMesh::computeLimitNormals() {
    // Compute limit normals using limit tangent vectors
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        std::vector<int> neighbors = getVertexNeighbors(static_cast<int>(i));
        int n = static_cast<int>(neighbors.size());
        
        if (n < 2) {
            m_vertices[i].limitNormal = m_vertices[i].normal;
            continue;
        }
        
        // Compute tangent vectors using Loop's formula
        glm::vec3 tu(0), tv(0);
        float pi2n = 2.0f * 3.14159265f / static_cast<float>(n);
        
        for (int j = 0; j < n; ++j) {
            float angle = static_cast<float>(j) * pi2n;
            glm::vec3 p = m_vertices[neighbors[j]].position;
            tu += std::cos(angle) * p;
            tv += std::sin(angle) * p;
        }
        
        glm::vec3 normal = glm::cross(tu, tv);
        float len = glm::length(normal);
        m_vertices[i].limitNormal = (len > 1e-6f) ? normal / len : m_vertices[i].normal;
    }
}

// Crease operations
void QuadMesh::setCreaseEdge(int v0, int v1, float weight) {
    uint64_t key = edgeKey(std::min(v0, v1), std::max(v0, v1));
    m_creaseWeights[key] = glm::clamp(weight, 0.0f, 1.0f);
    
    int he = findHalfEdge(v0, v1);
    if (he != -1) {
        m_halfEdges[he].isCrease = (weight > 0.01f);
        m_halfEdges[he].creaseWeight = weight;
        int twin = m_halfEdges[he].twinIdx;
        if (twin != -1) {
            m_halfEdges[twin].isCrease = m_halfEdges[he].isCrease;
            m_halfEdges[twin].creaseWeight = weight;
        }
    }
}

void QuadMesh::removeCreaseEdge(int v0, int v1) {
    setCreaseEdge(v0, v1, 0.0f);
}

void QuadMesh::setCornerVertex(int vertexIdx, float weight) {
    if (vertexIdx >= 0 && vertexIdx < static_cast<int>(m_vertices.size())) {
        m_vertices[vertexIdx].isCorner = (weight > 0.01f);
        m_vertices[vertexIdx].cornerWeight = glm::clamp(weight, 0.0f, 1.0f);
    }
}

std::vector<CreaseEdge> QuadMesh::getCreaseEdges() const {
    std::vector<CreaseEdge> edges;
    for (const auto& [key, weight] : m_creaseWeights) {
        if (weight > 0.01f) {
            CreaseEdge ce;
            ce.vertex0 = static_cast<int>(key >> 32);
            ce.vertex1 = static_cast<int>(key & 0xFFFFFFFF);
            ce.weight = weight;
            edges.push_back(ce);
        }
    }
    return edges;
}

// Control point editing
void QuadMesh::moveVertex(int vertexIdx, const glm::vec3& newPosition) {
    if (vertexIdx >= 0 && vertexIdx < static_cast<int>(m_vertices.size())) {
        m_vertices[vertexIdx].position = newPosition;
    }
}

void QuadMesh::moveVertices(const std::vector<int>& indices, const glm::vec3& delta) {
    for (int idx : indices) {
        if (idx >= 0 && idx < static_cast<int>(m_vertices.size())) {
            m_vertices[idx].position += delta;
        }
    }
}

void QuadMesh::smoothVertex(int vertexIdx, float factor) {
    std::vector<int> neighbors = getVertexNeighbors(vertexIdx);
    if (neighbors.empty()) return;
    
    glm::vec3 avg(0);
    for (int n : neighbors) {
        avg += m_vertices[n].position;
    }
    avg /= static_cast<float>(neighbors.size());
    
    m_vertices[vertexIdx].position = glm::mix(m_vertices[vertexIdx].position, avg, factor);
}

void QuadMesh::relaxVertices(const std::vector<int>& indices, int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<glm::vec3> newPositions(indices.size());
        
        for (size_t i = 0; i < indices.size(); ++i) {
            int idx = indices[i];
            std::vector<int> neighbors = getVertexNeighbors(idx);
            
            if (neighbors.empty() || m_vertices[idx].isBoundary) {
                newPositions[i] = m_vertices[idx].position;
                continue;
            }
            
            glm::vec3 avg(0);
            for (int n : neighbors) {
                avg += m_vertices[n].position;
            }
            avg /= static_cast<float>(neighbors.size());
            
            newPositions[i] = glm::mix(m_vertices[idx].position, avg, 0.5f);
        }
        
        for (size_t i = 0; i < indices.size(); ++i) {
            m_vertices[indices[i]].position = newPositions[i];
        }
    }
}

// Selection
void QuadMesh::selectVertex(int idx, bool addToSelection) {
    if (!addToSelection) m_selectedVertices.clear();
    if (idx >= 0 && idx < static_cast<int>(m_vertices.size())) {
        m_selectedVertices.insert(idx);
    }
}

void QuadMesh::selectFace(int idx, bool addToSelection) {
    if (!addToSelection) m_selectedFaces.clear();
    if (idx >= 0 && idx < static_cast<int>(m_faces.size())) {
        m_selectedFaces.insert(idx);
        m_faces[idx].isSelected = true;
    }
}

void QuadMesh::selectEdgeLoop(int halfEdgeIdx) {
    if (halfEdgeIdx < 0 || halfEdgeIdx >= static_cast<int>(m_halfEdges.size())) return;
    
    // Walk around edge loop selecting edges
    std::unordered_set<int> visited;
    std::queue<int> toProcess;
    toProcess.push(halfEdgeIdx);
    
    while (!toProcess.empty()) {
        int he = toProcess.front();
        toProcess.pop();
        
        if (visited.count(he)) continue;
        visited.insert(he);
        
        // Select vertices of this edge
        m_selectedVertices.insert(m_halfEdges[he].vertexIdx);
        m_selectedVertices.insert(m_halfEdges[m_halfEdges[he].prevIdx].vertexIdx);
        
        // Continue to opposite edge in quad
        if (m_faces[m_halfEdges[he].faceIdx].vertexCount == 4) {
            int next = m_halfEdges[m_halfEdges[he].nextIdx].nextIdx;
            if (!visited.count(next)) {
                toProcess.push(next);
            }
        }
        
        // Go through twin
        int twin = m_halfEdges[he].twinIdx;
        if (twin != -1 && m_faces[m_halfEdges[twin].faceIdx].vertexCount == 4) {
            int twinOpposite = m_halfEdges[m_halfEdges[twin].nextIdx].nextIdx;
            if (!visited.count(twinOpposite)) {
                toProcess.push(twinOpposite);
            }
        }
    }
}

void QuadMesh::selectFaceLoop(int halfEdgeIdx) {
    if (halfEdgeIdx < 0 || halfEdgeIdx >= static_cast<int>(m_halfEdges.size())) return;
    
    std::unordered_set<int> visited;
    std::queue<int> toProcess;
    toProcess.push(m_halfEdges[halfEdgeIdx].faceIdx);
    
    while (!toProcess.empty()) {
        int faceIdx = toProcess.front();
        toProcess.pop();
        
        if (visited.count(faceIdx)) continue;
        visited.insert(faceIdx);
        
        selectFace(faceIdx, true);
        
        // Add adjacent faces along the loop direction
        // This would need proper implementation based on edge direction
    }
}

void QuadMesh::clearSelection() {
    for (int idx : m_selectedFaces) {
        m_faces[idx].isSelected = false;
    }
    m_selectedVertices.clear();
    m_selectedFaces.clear();
}

std::vector<int> QuadMesh::getSelectedVertices() const {
    return std::vector<int>(m_selectedVertices.begin(), m_selectedVertices.end());
}

std::vector<int> QuadMesh::getSelectedFaces() const {
    return std::vector<int>(m_selectedFaces.begin(), m_selectedFaces.end());
}

// Quality analysis
QuadMeshQuality QuadMesh::computeQuality() const {
    QuadMeshQuality quality{};
    quality.minAngle = 180.0f;
    quality.maxAngle = 0.0f;
    
    int quadCount = 0;
    float angleSum = 0.0f;
    int angleCount = 0;
    
    for (const auto& face : m_faces) {
        if (face.vertexCount == 4) quadCount++;
        
        std::vector<int> verts = getFaceVertices(&face - &m_faces[0]);
        for (size_t i = 0; i < verts.size(); ++i) {
            glm::vec3 v0 = m_vertices[verts[i]].position;
            glm::vec3 v1 = m_vertices[verts[(i + 1) % verts.size()]].position;
            glm::vec3 v2 = m_vertices[verts[(i + verts.size() - 1) % verts.size()]].position;
            
            glm::vec3 e1 = glm::normalize(v1 - v0);
            glm::vec3 e2 = glm::normalize(v2 - v0);
            float angle = std::acos(glm::clamp(glm::dot(e1, e2), -1.0f, 1.0f)) * 180.0f / 3.14159265f;
            
            quality.minAngle = std::min(quality.minAngle, angle);
            quality.maxAngle = std::max(quality.maxAngle, angle);
            angleSum += angle;
            angleCount++;
        }
    }
    
    quality.averageAngle = angleCount > 0 ? angleSum / angleCount : 0;
    quality.quadPercentage = m_faces.empty() ? 0 : 
                             100.0f * quadCount / static_cast<float>(m_faces.size());
    quality.irregularVertices = static_cast<int>(findIrregularVertices().size());
    
    // Compute aspect ratio
    float aspectSum = 0.0f;
    for (size_t f = 0; f < m_faces.size(); ++f) {
        if (m_faces[f].vertexCount != 4) continue;
        
        std::vector<int> verts = getFaceVertices(static_cast<int>(f));
        float e0 = glm::length(m_vertices[verts[1]].position - m_vertices[verts[0]].position);
        float e1 = glm::length(m_vertices[verts[2]].position - m_vertices[verts[1]].position);
        float e2 = glm::length(m_vertices[verts[3]].position - m_vertices[verts[2]].position);
        float e3 = glm::length(m_vertices[verts[0]].position - m_vertices[verts[3]].position);
        
        float avgWidth = (e0 + e2) * 0.5f;
        float avgHeight = (e1 + e3) * 0.5f;
        float aspect = (avgWidth > avgHeight) ? avgWidth / avgHeight : avgHeight / avgWidth;
        aspectSum += aspect;
    }
    quality.aspectRatio = quadCount > 0 ? aspectSum / quadCount : 1.0f;
    
    return quality;
}

std::vector<int> QuadMesh::findIrregularVertices() const {
    std::vector<int> irregular;
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        int valence = getVertexValence(static_cast<int>(i));
        // Regular valence is 4 for interior, 2 for boundary
        int expectedValence = m_vertices[i].isBoundary ? 2 : 4;
        if (valence != expectedValence && valence > 0) {
            irregular.push_back(static_cast<int>(i));
        }
    }
    return irregular;
}

// Rendering
void QuadMesh::updateNormals() {
    // Reset vertex normals
    for (auto& v : m_vertices) {
        v.normal = glm::vec3(0);
    }
    
    // Accumulate face normals
    for (auto& face : m_faces) {
        std::vector<int> verts = getFaceVertices(&face - &m_faces[0]);
        if (verts.size() < 3) continue;
        
        glm::vec3 v0 = m_vertices[verts[0]].position;
        glm::vec3 v1 = m_vertices[verts[1]].position;
        glm::vec3 v2 = m_vertices[verts[2]].position;
        
        face.normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        
        // Compute centroid
        glm::vec3 centroid(0);
        for (int idx : verts) {
            centroid += m_vertices[idx].position;
        }
        face.centroid = centroid / static_cast<float>(verts.size());
        
        // Add to vertex normals (weighted by angle)
        for (size_t i = 0; i < verts.size(); ++i) {
            glm::vec3 e1 = m_vertices[verts[(i + 1) % verts.size()]].position - 
                           m_vertices[verts[i]].position;
            glm::vec3 e2 = m_vertices[verts[(i + verts.size() - 1) % verts.size()]].position - 
                           m_vertices[verts[i]].position;
            float angle = std::acos(glm::clamp(glm::dot(glm::normalize(e1), glm::normalize(e2)), -1.0f, 1.0f));
            m_vertices[verts[i]].normal += face.normal * angle;
        }
    }
    
    // Normalize vertex normals
    for (auto& v : m_vertices) {
        float len = glm::length(v.normal);
        if (len > 1e-6f) {
            v.normal /= len;
        } else {
            v.normal = glm::vec3(0, 1, 0);
        }
    }
}

std::vector<float> QuadMesh::getVertexBuffer() const {
    // Format: position (3) + normal (3) + uv (2) = 8 floats per vertex
    std::vector<float> buffer;
    buffer.reserve(m_vertices.size() * 8);
    
    for (const auto& v : m_vertices) {
        buffer.push_back(v.position.x);
        buffer.push_back(v.position.y);
        buffer.push_back(v.position.z);
        buffer.push_back(v.normal.x);
        buffer.push_back(v.normal.y);
        buffer.push_back(v.normal.z);
        buffer.push_back(v.uv.x);
        buffer.push_back(v.uv.y);
    }
    
    return buffer;
}

std::vector<unsigned int> QuadMesh::getIndexBuffer() const {
    std::vector<unsigned int> indices;
    
    for (const auto& face : m_faces) {
        std::vector<int> verts = getFaceVertices(&face - &m_faces[0]);
        
        if (verts.size() == 3) {
            indices.push_back(verts[0]);
            indices.push_back(verts[1]);
            indices.push_back(verts[2]);
        } else if (verts.size() == 4) {
            // Split quad into two triangles
            indices.push_back(verts[0]);
            indices.push_back(verts[1]);
            indices.push_back(verts[2]);
            
            indices.push_back(verts[0]);
            indices.push_back(verts[2]);
            indices.push_back(verts[3]);
        }
    }
    
    return indices;
}

std::vector<float> QuadMesh::getWireframeBuffer() const {
    std::vector<float> buffer;
    
    for (size_t i = 0; i < m_halfEdges.size(); ++i) {
        // Only process each edge once
        if (m_halfEdges[i].twinIdx != -1 && static_cast<size_t>(m_halfEdges[i].twinIdx) < i) {
            continue;
        }
        
        int v0 = m_halfEdges[m_halfEdges[i].prevIdx].vertexIdx;
        int v1 = m_halfEdges[i].vertexIdx;
        
        const glm::vec3& p0 = m_vertices[v0].position;
        const glm::vec3& p1 = m_vertices[v1].position;
        
        buffer.push_back(p0.x);
        buffer.push_back(p0.y);
        buffer.push_back(p0.z);
        
        buffer.push_back(p1.x);
        buffer.push_back(p1.y);
        buffer.push_back(p1.z);
    }
    
    return buffer;
}

} // namespace dc
