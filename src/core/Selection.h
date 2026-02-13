/**
 * @file Selection.h
 * @brief Selection manager for tracking selected objects, faces, vertices, and edges
 * 
 * Provides:
 * - Multiple selection modes (Object, Face, Vertex, Edge)
 * - Add/remove/toggle/clear selection operations
 * - Selection changed signals for UI updates
 */

#pragma once

#include <QObject>
#include <set>
#include <vector>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

namespace dc3d {
namespace core {

/**
 * @brief Selection mode determines what type of elements can be selected
 */
enum class SelectionMode {
    Object,     ///< Select entire meshes/objects
    Face,       ///< Select individual triangular faces
    Vertex,     ///< Select individual vertices
    Edge        ///< Select edges between vertices
};

/**
 * @brief Selection operation modifier
 */
enum class SelectionOp {
    Replace,    ///< Clear existing selection and select new item(s)
    Add,        ///< Add to existing selection (Shift+Click)
    Toggle,     ///< Toggle selection state (Ctrl+Click)
    Remove      ///< Remove from selection
};

/**
 * @brief Identifies a selectable element with mesh ID and element index
 */
struct SelectionElement {
    uint32_t meshId = 0;        ///< ID of the mesh this element belongs to
    uint64_t elementIndex = 0;  ///< Index within the mesh (face/vertex/edge index)
                                ///< For edges: upper 32 bits = v2, lower 32 bits = v1
    SelectionMode mode = SelectionMode::Object;  ///< What type of element this is
    
    bool operator<(const SelectionElement& other) const {
        if (meshId != other.meshId) return meshId < other.meshId;
        if (mode != other.mode) return static_cast<int>(mode) < static_cast<int>(other.mode);
        return elementIndex < other.elementIndex;
    }
    
    bool operator==(const SelectionElement& other) const {
        return meshId == other.meshId && 
               elementIndex == other.elementIndex && 
               mode == other.mode;
    }
};

/**
 * @brief Hit information from a picking operation
 */
struct HitInfo {
    bool hit = false;                       ///< Whether something was hit
    uint32_t meshId = 0;                    ///< ID of the hit mesh
    uint32_t faceIndex = 0;                 ///< Index of the hit face
    glm::vec3 hitPoint{0.0f};               ///< World-space hit point
    glm::vec3 hitNormal{0.0f};              ///< Surface normal at hit point
    glm::vec3 barycentricCoords{0.0f};      ///< Barycentric coordinates within triangle
    float distance = std::numeric_limits<float>::max();  ///< Distance from ray origin
    
    // Vertex indices of the hit face
    uint32_t vertexIndices[3] = {0, 0, 0};
    
    // Closest edge to hit point (for edge selection mode)
    int closestEdge = -1;                   ///< 0, 1, or 2 for the three edges
    uint32_t closestVertex = 0;             ///< Closest vertex index
};

/**
 * @brief Selection manager class
 * 
 * Manages the current selection state including:
 * - Which elements are selected
 * - Current selection mode
 * - Selection history for undo/redo
 */
class Selection : public QObject {
    Q_OBJECT

public:
    explicit Selection(QObject* parent = nullptr);
    ~Selection() override = default;
    
    // ---- Selection Mode ----
    
    /**
     * @brief Get the current selection mode
     */
    SelectionMode mode() const { return m_mode; }
    
    /**
     * @brief Set the selection mode
     * 
     * Changing modes clears the current selection.
     */
    void setMode(SelectionMode mode);
    
    // ---- Selection Operations ----
    
    /**
     * @brief Select an element
     * @param element The element to select
     * @param op Selection operation (Replace, Add, Toggle, Remove)
     */
    void select(const SelectionElement& element, SelectionOp op = SelectionOp::Replace);
    
    /**
     * @brief Select multiple elements
     * @param elements Elements to select
     * @param op Selection operation
     */
    void select(const std::vector<SelectionElement>& elements, SelectionOp op = SelectionOp::Replace);
    
    /**
     * @brief Select from a hit result
     * @param hit Hit information from picking
     * @param op Selection operation
     */
    void selectFromHit(const HitInfo& hit, SelectionOp op = SelectionOp::Replace);
    
    /**
     * @brief Deselect a specific element
     */
    void deselect(const SelectionElement& element);
    
    /**
     * @brief Clear all selection
     */
    void clear();
    
    /**
     * @brief Invert selection within a mesh
     * @param meshId Mesh to invert selection in
     * @param totalElements Total number of elements in the mesh
     */
    void invertSelection(uint32_t meshId, uint32_t totalElements);
    
    /**
     * @brief Select all elements of a mesh
     * @param meshId Mesh to select all in
     * @param totalElements Total number of elements
     */
    void selectAll(uint32_t meshId, uint32_t totalElements);
    
    // ---- Query ----
    
    /**
     * @brief Check if an element is selected
     */
    bool isSelected(const SelectionElement& element) const;
    
    /**
     * @brief Check if a mesh has any selected elements
     */
    bool hasSelection(uint32_t meshId) const;
    
    /**
     * @brief Check if selection is empty
     */
    bool isEmpty() const { return m_selectedElements.empty(); }
    
    /**
     * @brief Get count of selected elements
     */
    size_t count() const { return m_selectedElements.size(); }
    
    /**
     * @brief Get all selected elements
     */
    const std::set<SelectionElement>& selectedElements() const { return m_selectedElements; }
    
    /**
     * @brief Get selected elements for a specific mesh
     * @param meshId Mesh to query
     * @return Vector of element indices that are selected
     */
    std::vector<uint32_t> selectedIndices(uint32_t meshId) const;
    
    /**
     * @brief Get selected mesh IDs (for Object mode)
     */
    std::vector<uint32_t> selectedMeshIds() const;
    
    // ---- Object Selection Helpers ----
    
    /**
     * @brief Select an entire object/mesh
     */
    void selectObject(uint32_t meshId, SelectionOp op = SelectionOp::Replace);
    
    /**
     * @brief Deselect an object
     */
    void deselectObject(uint32_t meshId);
    
    /**
     * @brief Check if an object is selected
     */
    bool isObjectSelected(uint32_t meshId) const;
    
    // ---- Face/Vertex/Edge Selection Helpers ----
    
    /**
     * @brief Select a face
     */
    void selectFace(uint32_t meshId, uint32_t faceIndex, SelectionOp op = SelectionOp::Replace);
    
    /**
     * @brief Select a vertex
     */
    void selectVertex(uint32_t meshId, uint32_t vertexIndex, SelectionOp op = SelectionOp::Replace);
    
    /**
     * @brief Select an edge (stored as vertex pair)
     * Edge index encoding: lower 32 bits = vertex1, upper 32 bits = vertex2 (supports large meshes)
     */
    void selectEdge(uint32_t meshId, uint32_t v1, uint32_t v2, SelectionOp op = SelectionOp::Replace);
    
    // ---- Selection Bounds ----
    
    /**
     * @brief Get the bounding box of the current selection
     * Requires mesh vertex data to compute
     */
    // void getSelectionBounds(glm::vec3& min, glm::vec3& max) const;

signals:
    /**
     * @brief Emitted when selection changes
     */
    void selectionChanged();
    
    /**
     * @brief Emitted when selection mode changes
     */
    void modeChanged(SelectionMode newMode);

private:
    SelectionMode m_mode = SelectionMode::Object;
    std::set<SelectionElement> m_selectedElements;
    
    // Helper to create element based on current mode
    SelectionElement createElementFromHit(const HitInfo& hit) const;
};

} // namespace core
} // namespace dc3d
