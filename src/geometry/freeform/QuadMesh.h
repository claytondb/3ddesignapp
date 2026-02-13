#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <glm/glm.hpp>

namespace dc {

// Forward declarations
class NurbsSurface;
class TriangleMesh;

/**
 * @brief Half-edge data structure for quad-dominant mesh
 */
struct HalfEdge {
    int vertexIdx;          // Vertex this half-edge points to
    int faceIdx;            // Face this half-edge belongs to
    int nextIdx;            // Next half-edge in face loop
    int prevIdx;            // Previous half-edge in face loop
    int twinIdx;            // Opposite half-edge (-1 if boundary)
    bool isCrease;          // Sharp crease edge
    float creaseWeight;     // Crease sharpness (0-1)
};

/**
 * @brief Vertex in quad mesh with subdivision support
 */
struct QuadVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    int halfEdgeIdx;        // One outgoing half-edge
    bool isCorner;          // Sharp corner vertex
    float cornerWeight;     // Corner sharpness (0-1)
    bool isBoundary;        // On mesh boundary
    
    // For subdivision
    glm::vec3 limitPosition;
    glm::vec3 limitNormal;
};

/**
 * @brief Face in quad mesh (supports quads and triangles)
 */
struct QuadFace {
    int halfEdgeIdx;        // First half-edge of face
    int vertexCount;        // 3 for triangle, 4 for quad
    glm::vec3 normal;
    glm::vec3 centroid;
    bool isSelected;
};

/**
 * @brief Crease edge definition
 */
struct CreaseEdge {
    int vertex0;
    int vertex1;
    float weight;           // 0 = smooth, 1 = fully sharp
};

/**
 * @brief Quality metrics for quad mesh
 */
struct QuadMeshQuality {
    float minAngle;
    float maxAngle;
    float averageAngle;
    float aspectRatio;
    int irregularVertices;  // Vertices with valence != 4
    float quadPercentage;   // Percentage of quad faces
};

/**
 * @brief Quad-dominant mesh with Catmull-Clark subdivision support
 * 
 * Provides a complete implementation of subdivision surfaces suitable
 * for organic/freeform modeling workflows.
 */
class QuadMesh {
public:
    QuadMesh();
    ~QuadMesh();
    
    // Construction
    void clear();
    int addVertex(const glm::vec3& position);
    int addFace(const std::vector<int>& vertexIndices);
    void buildTopology();
    
    // Import/Export
    static std::unique_ptr<QuadMesh> fromTriangleMesh(const TriangleMesh& mesh);
    std::unique_ptr<TriangleMesh> toTriangleMesh() const;
    
    // Catmull-Clark Subdivision
    std::unique_ptr<QuadMesh> subdivide(int levels = 1) const;
    void computeLimitPositions();
    void computeLimitNormals();
    
    // Crease support
    void setCreaseEdge(int v0, int v1, float weight);
    void removeCreaseEdge(int v0, int v1);
    void setCornerVertex(int vertexIdx, float weight);
    std::vector<CreaseEdge> getCreaseEdges() const;
    
    // Control point editing
    void moveVertex(int vertexIdx, const glm::vec3& newPosition);
    void moveVertices(const std::vector<int>& indices, const glm::vec3& delta);
    void smoothVertex(int vertexIdx, float factor = 0.5f);
    void relaxVertices(const std::vector<int>& indices, int iterations = 1);
    
    // Topology operations
    void splitEdge(int halfEdgeIdx);
    void collapseEdge(int halfEdgeIdx);
    void insertEdgeLoop(int halfEdgeIdx);
    void deleteEdgeLoop(int halfEdgeIdx);
    void extrudeFace(int faceIdx, float distance);
    void bevelEdge(int halfEdgeIdx, float offset);
    
    // NURBS conversion
    std::unique_ptr<NurbsSurface> toNurbs(int uDegree = 3, int vDegree = 3) const;
    std::vector<std::unique_ptr<NurbsSurface>> toNurbsPatches() const;
    
    // Selection
    void selectVertex(int idx, bool addToSelection = false);
    void selectFace(int idx, bool addToSelection = false);
    void selectEdgeLoop(int halfEdgeIdx);
    void selectFaceLoop(int halfEdgeIdx);
    void clearSelection();
    std::vector<int> getSelectedVertices() const;
    std::vector<int> getSelectedFaces() const;
    
    // Quality analysis
    QuadMeshQuality computeQuality() const;
    std::vector<int> findIrregularVertices() const;
    int getVertexValence(int vertexIdx) const;
    
    // Accessors
    const std::vector<QuadVertex>& vertices() const { return m_vertices; }
    const std::vector<QuadFace>& faces() const { return m_faces; }
    const std::vector<HalfEdge>& halfEdges() const { return m_halfEdges; }
    
    int vertexCount() const { return static_cast<int>(m_vertices.size()); }
    int faceCount() const { return static_cast<int>(m_faces.size()); }
    int edgeCount() const { return static_cast<int>(m_halfEdges.size()) / 2; }
    
    // Rendering data
    void updateNormals();
    std::vector<float> getVertexBuffer() const;
    std::vector<unsigned int> getIndexBuffer() const;
    std::vector<float> getWireframeBuffer() const;
    
private:
    std::vector<QuadVertex> m_vertices;
    std::vector<QuadFace> m_faces;
    std::vector<HalfEdge> m_halfEdges;
    
    std::unordered_set<int> m_selectedVertices;
    std::unordered_set<int> m_selectedFaces;
    
    // Edge lookup: hash(v0, v1) -> half-edge index
    std::unordered_map<uint64_t, int> m_edgeMap;
    
    // Crease data
    std::unordered_map<uint64_t, float> m_creaseWeights;
    
    // Helper functions
    uint64_t edgeKey(int v0, int v1) const;
    int findHalfEdge(int v0, int v1) const;
    std::vector<int> getVertexFaces(int vertexIdx) const;
    std::vector<int> getVertexNeighbors(int vertexIdx) const;
    std::vector<int> getFaceVertices(int faceIdx) const;
    
    // Subdivision helpers
    glm::vec3 computeFacePoint(int faceIdx) const;
    glm::vec3 computeEdgePoint(int halfEdgeIdx) const;
    glm::vec3 computeVertexPoint(int vertexIdx, 
                                  const std::vector<glm::vec3>& facePoints,
                                  const std::vector<glm::vec3>& edgePoints) const;
    glm::vec3 computeCreaseVertexPoint(int vertexIdx,
                                        const std::vector<glm::vec3>& edgePoints) const;
};

} // namespace dc
