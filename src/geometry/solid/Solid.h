/**
 * @file Solid.h
 * @brief B-Rep solid body representation for CAD operations
 * 
 * Provides a boundary representation (B-Rep) solid model consisting of
 * faces, edges, and vertices with full topological information.
 * Supports watertight validation, volume/area calculations, and face adjacency.
 */

#pragma once

#include "../MeshData.h"
#include "../HalfEdgeMesh.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <mutex>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace dc3d {
namespace geometry {

// Forward declarations
class Solid;
class SolidFace;
class SolidEdge;
class SolidVertex;

/// Unique identifier for solid components
using SolidId = uint64_t;

/// Invalid solid ID constant
constexpr SolidId INVALID_SOLID_ID = 0;

/**
 * @brief Face loop type for classification
 */
enum class LoopType {
    Outer,      ///< Outer boundary loop
    Inner,      ///< Inner hole loop
    Unknown     ///< Classification unknown
};

/**
 * @brief Face orientation relative to solid volume
 */
enum class FaceOrientation {
    Outward,    ///< Normal points outward (standard)
    Inward,     ///< Normal points inward (inverted)
    Unknown     ///< Orientation not determined
};

/**
 * @brief Solid validation result
 */
struct SolidValidation {
    bool isValid = false;
    bool isWatertight = false;
    bool hasConsistentNormals = false;
    bool isSelfIntersecting = false;
    bool hasNonManifoldEdges = false;
    bool hasNonManifoldVertices = false;
    bool hasZeroAreaFaces = false;
    
    size_t openEdgeCount = 0;
    size_t nonManifoldEdgeCount = 0;
    size_t nonManifoldVertexCount = 0;
    size_t degenerateFaceCount = 0;
    
    std::vector<uint32_t> openEdges;
    std::vector<uint32_t> nonManifoldEdges;
    std::vector<uint32_t> nonManifoldVertices;
    std::vector<uint32_t> degenerateFaces;
    
    std::string errorMessage;
    
    bool ok() const { return isValid && isWatertight && !isSelfIntersecting; }
};

/**
 * @brief Vertex in a solid body
 */
struct SolidVertex {
    SolidId id = INVALID_SOLID_ID;
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};             ///< Average vertex normal
    
    std::vector<uint32_t> edges;        ///< Incident edge indices
    std::vector<uint32_t> faces;        ///< Adjacent face indices
    
    // Attributes
    float curvature = 0.0f;             ///< Mean curvature at vertex
    bool isSharp = false;               ///< Sharp corner flag
    bool isOnBoundary = false;          ///< True if on open boundary
    
    /// Get valence (number of incident edges)
    size_t valence() const { return edges.size(); }
    
    /// Check if vertex is manifold (edge valence equals face valence)
    bool isManifold() const { return edges.size() == faces.size() || isOnBoundary; }
};

/**
 * @brief Edge in a solid body
 */
struct SolidEdge {
    SolidId id = INVALID_SOLID_ID;
    uint32_t vertex0 = 0;               ///< Start vertex index
    uint32_t vertex1 = 0;               ///< End vertex index
    
    std::vector<uint32_t> faces;        ///< Adjacent face indices (usually 2 for manifold)
    
    // Edge classification
    bool isSharp = false;               ///< Hard edge for rendering
    bool isSeam = false;                ///< UV seam edge
    bool isBoundary = false;            ///< Open boundary edge
    float dihedralAngle = 0.0f;         ///< Angle between adjacent faces
    
    // Smooth edge data for fillets/chamfers
    float length = 0.0f;                ///< Edge length
    glm::vec3 direction{0.0f};          ///< Normalized direction v0->v1
    glm::vec3 midpoint{0.0f};           ///< Edge midpoint
    
    /// Check if edge is manifold (exactly 2 adjacent faces)
    bool isManifold() const { return faces.size() == 2; }
    
    /// Check if edge is non-manifold (more than 2 adjacent faces)
    bool isNonManifold() const { return faces.size() > 2; }
    
    /// Get the other vertex given one vertex index
    uint32_t otherVertex(uint32_t v) const {
        return (v == vertex0) ? vertex1 : vertex0;
    }
};

/**
 * @brief Face in a solid body
 */
struct SolidFace {
    SolidId id = INVALID_SOLID_ID;
    std::vector<uint32_t> vertices;     ///< Vertex indices (CCW order)
    std::vector<uint32_t> edges;        ///< Edge indices around face
    
    glm::vec3 normal{0.0f, 0.0f, 1.0f}; ///< Face normal
    glm::vec3 centroid{0.0f};           ///< Face centroid
    float area = 0.0f;                  ///< Face area
    
    // Face classification
    FaceOrientation orientation = FaceOrientation::Outward;
    LoopType loopType = LoopType::Outer;
    int materialId = -1;                 ///< Material assignment
    int groupId = -1;                    ///< Grouping for operations
    
    // Surface type for parametric faces
    enum class SurfaceType {
        Planar,
        Cylindrical,
        Conical,
        Spherical,
        Toroidal,
        NURBS,
        Freeform
    } surfaceType = SurfaceType::Freeform;
    
    /// Check if face is triangular
    bool isTriangle() const { return vertices.size() == 3; }
    
    /// Check if face is quadrilateral
    bool isQuad() const { return vertices.size() == 4; }
    
    /// Check if face is planar within tolerance
    bool isPlanar(float tolerance = 1e-6f) const;
    
    /// Get vertex count
    size_t vertexCount() const { return vertices.size(); }
};

/**
 * @brief Shell represents a connected set of faces (could be open or closed)
 */
struct SolidShell {
    std::vector<uint32_t> faces;        ///< Face indices in this shell
    bool isClosed = false;              ///< True if shell is watertight
    bool isOuterShell = true;           ///< True for outer shell, false for voids
    float volume = 0.0f;                ///< Shell volume (if closed)
    float surfaceArea = 0.0f;           ///< Total surface area
    BoundingBox bounds;                 ///< Shell bounding box
};

/**
 * @brief B-Rep solid body representation
 * 
 * A solid consists of one or more shells. The outer shell defines the
 * exterior boundary, while inner shells define voids (cavities).
 * 
 * Key features:
 * - Full topological connectivity (vertex-edge-face)
 * - Watertight validation
 * - Volume and surface area calculations
 * - Support for multiple shells (solids with holes)
 * - Face adjacency queries
 */
class Solid {
public:
    Solid();
    ~Solid() = default;
    
    // Move operations - enabled by using unique_ptr for mutex
    Solid(Solid&& other) noexcept;
    Solid& operator=(Solid&& other) noexcept;
    
    // Copy operations - deleted (mutex cannot be copied)
    Solid(const Solid& other) = delete;
    Solid& operator=(const Solid& other) = delete;
    
    // ===================
    // Construction
    // ===================
    
    /**
     * @brief Create solid from mesh data
     * @param mesh Source triangle mesh
     * @param progress Optional progress callback
     * @return Result with Solid or error message
     */
    static Result<Solid> fromMesh(const MeshData& mesh, 
                                  ProgressCallback progress = nullptr);
    
    /**
     * @brief Create solid from half-edge mesh
     * @param heMesh Source half-edge mesh
     * @return Result with Solid or error message
     */
    static Result<Solid> fromHalfEdgeMesh(const HalfEdgeMesh& heMesh);
    
    /**
     * @brief Create a box primitive
     * @param size Box dimensions (width, height, depth)
     * @param center Box center position
     * @return Box solid
     */
    static Solid createBox(const glm::vec3& size, 
                          const glm::vec3& center = glm::vec3(0.0f));
    
    /**
     * @brief Create a cylinder primitive
     * @param radius Cylinder radius
     * @param height Cylinder height
     * @param segments Number of radial segments
     * @param center Base center position
     * @return Cylinder solid
     */
    static Solid createCylinder(float radius, float height, int segments = 32,
                               const glm::vec3& center = glm::vec3(0.0f));
    
    /**
     * @brief Create a sphere primitive
     * @param radius Sphere radius
     * @param segments Latitude/longitude segments
     * @param center Sphere center
     * @return Sphere solid
     */
    static Solid createSphere(float radius, int segments = 32,
                             const glm::vec3& center = glm::vec3(0.0f));
    
    /**
     * @brief Create a cone primitive
     * @param baseRadius Radius at base
     * @param topRadius Radius at top (0 for point)
     * @param height Cone height
     * @param segments Number of radial segments
     * @param center Base center position
     * @return Cone/frustum solid
     */
    static Solid createCone(float baseRadius, float topRadius, float height,
                           int segments = 32, const glm::vec3& center = glm::vec3(0.0f));
    
    /**
     * @brief Create a torus primitive
     * @param majorRadius Distance from center to tube center
     * @param minorRadius Tube radius
     * @param majorSegments Segments around the ring
     * @param minorSegments Segments around the tube
     * @param center Torus center
     * @return Torus solid
     */
    static Solid createTorus(float majorRadius, float minorRadius,
                            int majorSegments = 32, int minorSegments = 16,
                            const glm::vec3& center = glm::vec3(0.0f));
    
    // ===================
    // Conversion
    // ===================
    
    /**
     * @brief Convert to triangle mesh for rendering
     * @return Triangulated mesh data
     */
    MeshData toMesh() const;
    
    /**
     * @brief Convert to half-edge mesh
     * @return Half-edge mesh representation
     */
    Result<HalfEdgeMesh> toHalfEdgeMesh() const;
    
    // ===================
    // Basic Queries
    // ===================
    
    /// Get unique solid ID
    SolidId id() const { return id_; }
    
    /// Set solid ID
    void setId(SolidId newId) { id_ = newId; }
    
    /// Get solid name
    const std::string& name() const { return name_; }
    
    /// Set solid name
    void setName(const std::string& newName) { name_ = newName; }
    
    /// Check if solid is empty
    bool isEmpty() const { return vertices_.empty(); }
    
    /// Get vertex count
    size_t vertexCount() const { return vertices_.size(); }
    
    /// Get edge count
    size_t edgeCount() const { return edges_.size(); }
    
    /// Get face count
    size_t faceCount() const { return faces_.size(); }
    
    /// Get shell count
    size_t shellCount() const { return shells_.size(); }
    
    /// Get bounding box
    const BoundingBox& bounds() const { return bounds_; }
    
    // ===================
    // Element Access
    // ===================
    
    const SolidVertex& vertex(uint32_t idx) const { return vertices_[idx]; }
    const SolidEdge& edge(uint32_t idx) const { return edges_[idx]; }
    const SolidFace& face(uint32_t idx) const { return faces_[idx]; }
    const SolidShell& shell(uint32_t idx) const { return shells_[idx]; }
    
    SolidVertex& vertex(uint32_t idx) { return vertices_[idx]; }
    SolidEdge& edge(uint32_t idx) { return edges_[idx]; }
    SolidFace& face(uint32_t idx) { return faces_[idx]; }
    SolidShell& shell(uint32_t idx) { return shells_[idx]; }
    
    const std::vector<SolidVertex>& vertices() const { return vertices_; }
    const std::vector<SolidEdge>& edges() const { return edges_; }
    const std::vector<SolidFace>& faces() const { return faces_; }
    const std::vector<SolidShell>& shells() const { return shells_; }
    
    std::vector<SolidVertex>& vertices() { return vertices_; }
    std::vector<SolidEdge>& edges() { return edges_; }
    std::vector<SolidFace>& faces() { return faces_; }
    std::vector<SolidShell>& shells() { return shells_; }
    
    // ===================
    // Validation
    // ===================
    
    /**
     * @brief Validate solid topology and geometry
     * @return Validation result with detailed information
     */
    SolidValidation validate() const;
    
    /**
     * @brief Check if solid is watertight (closed manifold)
     * @return true if solid has no holes or non-manifold geometry
     */
    bool isWatertight() const;
    
    /**
     * @brief Check if solid is manifold
     * @return true if all edges have exactly 2 adjacent faces
     */
    bool isManifold() const;
    
    /**
     * @brief Check for self-intersections
     * @param progress Optional progress callback
     * @return true if solid has self-intersections
     */
    bool hasSelfIntersections(ProgressCallback progress = nullptr) const;
    
    // ===================
    // Geometric Properties
    // ===================
    
    /**
     * @brief Calculate solid volume
     * @return Volume (positive for outward-facing normals)
     * 
     * Uses the divergence theorem (sum of signed tetrahedron volumes).
     * Requires watertight solid for accurate results.
     */
    float volume() const;
    
    /**
     * @brief Get signed volume (negative if normals are inverted)
     * @return Signed volume
     */
    float signedVolume() const;
    
    /**
     * @brief Check if normals are inverted (inside-out solid)
     * @return true if signed volume is negative
     */
    bool hasInvertedNormals() const;
    
    /**
     * @brief Calculate total surface area
     * @return Sum of all face areas
     */
    float surfaceArea() const;
    
    /**
     * @brief Calculate center of mass (assuming uniform density)
     * @return Centroid position
     */
    glm::vec3 centerOfMass() const;
    
    /**
     * @brief Calculate moment of inertia tensor
     * @param density Material density (default 1.0)
     * @return 3x3 inertia tensor matrix
     */
    glm::mat3 inertiaTensor(float density = 1.0f) const;
    
    // ===================
    // Adjacency Queries
    // ===================
    
    /**
     * @brief Get faces adjacent to a vertex
     * @param vertexIdx Vertex index
     * @return Vector of face indices
     */
    std::vector<uint32_t> facesAroundVertex(uint32_t vertexIdx) const;
    
    /**
     * @brief Get edges adjacent to a vertex
     * @param vertexIdx Vertex index
     * @return Vector of edge indices
     */
    std::vector<uint32_t> edgesAroundVertex(uint32_t vertexIdx) const;
    
    /**
     * @brief Get vertices adjacent to a vertex (1-ring)
     * @param vertexIdx Vertex index
     * @return Vector of vertex indices
     */
    std::vector<uint32_t> verticesAroundVertex(uint32_t vertexIdx) const;
    
    /**
     * @brief Get faces adjacent to a face
     * @param faceIdx Face index
     * @return Vector of adjacent face indices
     */
    std::vector<uint32_t> adjacentFaces(uint32_t faceIdx) const;
    
    /**
     * @brief Get faces sharing an edge
     * @param edgeIdx Edge index
     * @return Vector of face indices (usually 2)
     */
    std::vector<uint32_t> facesOnEdge(uint32_t edgeIdx) const;
    
    /**
     * @brief Find edge between two vertices
     * @param v0 First vertex index
     * @param v1 Second vertex index
     * @return Edge index or std::nullopt if not found
     */
    std::optional<uint32_t> findEdge(uint32_t v0, uint32_t v1) const;
    
    /**
     * @brief Get edges forming the boundary of a face
     * @param faceIdx Face index
     * @return Vector of edge indices in order
     */
    std::vector<uint32_t> edgesOfFace(uint32_t faceIdx) const;
    
    // ===================
    // Selection Helpers
    // ===================
    
    /**
     * @brief Find sharp edges (dihedral angle above threshold)
     * @param angleThreshold Minimum angle in radians (default 30°)
     * @return Vector of sharp edge indices
     */
    std::vector<uint32_t> findSharpEdges(float angleThreshold = 0.523599f) const;
    
    /**
     * @brief Find boundary edges (open edges)
     * @return Vector of boundary edge indices
     */
    std::vector<uint32_t> findBoundaryEdges() const;
    
    /**
     * @brief Find edges connected by tangent continuity
     * @param startEdge Starting edge index
     * @param angleThreshold Maximum angle deviation in radians
     * @return Vector of tangent-connected edge indices
     */
    std::vector<uint32_t> findTangentEdges(uint32_t startEdge, 
                                           float angleThreshold = 0.087266f) const;
    
    /**
     * @brief Find concave edges (opening inward)
     * @return Vector of concave edge indices
     */
    std::vector<uint32_t> findConcaveEdges() const;
    
    /**
     * @brief Find convex edges (opening outward)
     * @return Vector of convex edge indices
     */
    std::vector<uint32_t> findConvexEdges() const;
    
    // ===================
    // Transformations
    // ===================
    
    /**
     * @brief Apply transformation matrix
     * @param transform 4x4 transformation matrix
     */
    void transform(const glm::mat4& transform);
    
    /**
     * @brief Translate solid
     * @param offset Translation vector
     */
    void translate(const glm::vec3& offset);
    
    /**
     * @brief Rotate solid
     * @param rotation Quaternion rotation
     * @param center Rotation center (default origin)
     */
    void rotate(const glm::quat& rotation, const glm::vec3& center = glm::vec3(0.0f));
    
    /**
     * @brief Scale solid uniformly
     * @param factor Scale factor
     * @param center Scale center (default origin)
     */
    void scale(float factor, const glm::vec3& center = glm::vec3(0.0f));
    
    /**
     * @brief Scale solid non-uniformly
     * @param factors Scale factors (x, y, z)
     * @param center Scale center (default origin)
     */
    void scale(const glm::vec3& factors, const glm::vec3& center = glm::vec3(0.0f));
    
    /**
     * @brief Flip face normals (inside-out)
     */
    void flipNormals();
    
    // ===================
    // Topology Modification
    // ===================
    
    /**
     * @brief Rebuild topology data (edges, adjacency, etc.)
     * 
     * Call after modifying vertices or faces directly.
     */
    void rebuildTopology();
    
    /**
     * @brief Recompute all normals
     */
    void recomputeNormals();
    
    /**
     * @brief Recompute bounding box
     */
    void recomputeBounds();
    
    /**
     * @brief Identify and separate shells
     * @return Number of shells found
     */
    size_t identifyShells();
    
    /**
     * @brief Make a deep copy of the solid
     * @return New solid with copied data
     */
    Solid clone() const;

private:
    // Core data
    SolidId id_ = INVALID_SOLID_ID;
    std::string name_;
    
    std::vector<SolidVertex> vertices_;
    std::vector<SolidEdge> edges_;
    std::vector<SolidFace> faces_;
    std::vector<SolidShell> shells_;
    
    // Cached data
    BoundingBox bounds_;
    mutable std::optional<float> cachedVolume_;
    mutable std::optional<float> cachedSignedVolume_;
    mutable std::optional<float> cachedSurfaceArea_;
    mutable std::optional<SolidValidation> cachedValidation_;
    mutable std::unique_ptr<std::mutex> validationMutex_;  // unique_ptr for move semantics
    
    // Edge lookup for quick adjacency queries
    std::unordered_map<uint64_t, uint32_t> edgeLookup_;
    
    // Helper methods
    void buildEdges();
    void buildAdjacency();
    void computeEdgeProperties();
    void computeFaceProperties();
    void invalidateCache();
    
    uint64_t makeEdgeKey(uint32_t v0, uint32_t v1) const;
    
    /**
     * @brief Triangle-triangle intersection test
     */
    bool triangleTriangleIntersect(const glm::vec3& a0, const glm::vec3& a1, const glm::vec3& a2,
                                   const glm::vec3& b0, const glm::vec3& b1, const glm::vec3& b2) const;
    
    // ID generator
    static SolidId generateNextId();
};

/**
 * @brief CSG tree node for boolean operation history
 */
class CSGNode {
public:
    enum class Operation {
        Primitive,      ///< Leaf node (original solid)
        Union,          ///< A ∪ B
        Subtract,       ///< A - B
        Intersect       ///< A ∩ B
    };
    
    CSGNode() = default;
    
    /// Create a primitive (leaf) node
    static std::shared_ptr<CSGNode> primitive(std::shared_ptr<Solid> solid);
    
    /// Create a boolean operation node
    static std::shared_ptr<CSGNode> operation(Operation op,
                                              std::shared_ptr<CSGNode> left,
                                              std::shared_ptr<CSGNode> right);
    
    Operation operation() const { return operation_; }
    std::shared_ptr<CSGNode> left() const { return left_; }
    std::shared_ptr<CSGNode> right() const { return right_; }
    std::shared_ptr<Solid> solid() const { return solid_; }
    
    /// Evaluate the CSG tree to produce a solid
    Result<Solid> evaluate() const;
    
    /// Check if this is a leaf node
    bool isLeaf() const { return operation_ == Operation::Primitive; }

private:
    Operation operation_ = Operation::Primitive;
    std::shared_ptr<CSGNode> left_;
    std::shared_ptr<CSGNode> right_;
    std::shared_ptr<Solid> solid_;
};

} // namespace geometry
} // namespace dc3d
