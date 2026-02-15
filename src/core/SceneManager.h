/**
 * @file SceneManager.h
 * @brief Scene graph management with mesh storage
 * 
 * Manages the hierarchical scene structure containing meshes,
 * CAD shapes, sketches, and annotations.
 */

#ifndef DC3D_CORE_SCENEMANAGER_H
#define DC3D_CORE_SCENEMANAGER_H

#include <QObject>
#include <QString>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

namespace dc3d {
namespace geometry {
class MeshData;
}

namespace core {

/**
 * @class SceneNode
 * @brief Base class for all scene graph nodes
 */
class SceneNode
{
public:
    SceneNode(const std::string& name = "Node");
    virtual ~SceneNode() = default;
    
    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

private:
    std::string m_name;
    bool m_visible = true;
};

/**
 * @class MeshNode
 * @brief Scene node containing mesh data
 */
class MeshNode : public SceneNode
{
public:
    MeshNode(uint64_t id, const QString& name, std::shared_ptr<geometry::MeshData> mesh);
    ~MeshNode() override = default;
    
    uint64_t id() const { return m_id; }
    std::shared_ptr<geometry::MeshData> mesh() const { return m_mesh; }
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString& name) { m_displayName = name; }
    
private:
    uint64_t m_id;
    QString m_displayName;
    std::shared_ptr<geometry::MeshData> m_mesh;
};

/**
 * @class ObjectGroup
 * @brief Groups multiple scene objects together
 */
class ObjectGroup
{
public:
    ObjectGroup(uint64_t id, const QString& name);
    ~ObjectGroup() = default;
    
    uint64_t id() const { return m_id; }
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    const std::vector<uint64_t>& members() const { return m_members; }
    void addMember(uint64_t nodeId);
    void removeMember(uint64_t nodeId);
    bool hasMember(uint64_t nodeId) const;
    bool isEmpty() const { return m_members.empty(); }
    size_t memberCount() const { return m_members.size(); }

private:
    uint64_t m_id;
    QString m_name;
    std::vector<uint64_t> m_members;
};

/**
 * @class SceneManager
 * @brief Manages the scene graph and provides access to scene nodes
 * 
 * Responsibilities:
 * - Owns the scene root node
 * - Manages mesh storage and lifecycle
 * - Provides node lookup by ID/name
 * - Emits signals when scene changes
 */
class SceneManager : public QObject
{
    Q_OBJECT

public:
    explicit SceneManager(QObject* parent = nullptr);
    ~SceneManager() override;
    
    // ---- Scene Operations ----
    
    /**
     * @brief Clear all nodes from the scene
     */
    void clear();
    
    /**
     * @brief Get the number of top-level nodes
     */
    size_t nodeCount() const { return m_nodes.size(); }
    
    /**
     * @brief Add a generic node to the scene
     * @param node The node to add (ownership transferred)
     */
    void addNode(std::unique_ptr<SceneNode> node);
    
    // ---- Mesh Management ----
    
    /**
     * @brief Add a mesh to the scene
     * @param id Unique mesh identifier
     * @param name Display name for the mesh
     * @param mesh The mesh data (shared ownership)
     */
    void addMesh(uint64_t id, const QString& name, std::shared_ptr<geometry::MeshData> mesh);
    
    /**
     * @brief Remove a mesh from the scene
     * @param id Mesh identifier to remove
     */
    void removeMesh(uint64_t id);
    
    /**
     * @brief Get a mesh by ID
     * @param id Mesh identifier
     * @return Mesh data or nullptr if not found
     */
    std::shared_ptr<geometry::MeshData> getMesh(uint64_t id) const;
    
    /**
     * @brief Get a mesh node by ID
     * @param id Mesh identifier
     * @return MeshNode or nullptr if not found
     */
    MeshNode* getMeshNode(uint64_t id) const;
    
    /**
     * @brief Check if a mesh exists
     * @param id Mesh identifier
     */
    bool hasMesh(uint64_t id) const;
    
    /**
     * @brief Get number of meshes in the scene
     */
    size_t meshCount() const { return m_meshNodes.size(); }
    
    /**
     * @brief Get all mesh IDs
     */
    std::vector<uint64_t> meshIds() const;
    
    /**
     * @brief Set mesh visibility
     * @param id Mesh identifier
     * @param visible Visibility state
     */
    void setMeshVisible(uint64_t id, bool visible);
    
    /**
     * @brief Get mesh visibility
     * @param id Mesh identifier
     * @return Visibility state (false if mesh not found)
     */
    bool isMeshVisible(uint64_t id) const;
    
    // ---- Node Management (for undo/redo support) ----
    
    /**
     * @brief Add a mesh node to the scene
     * @param node The mesh node to add (ownership transferred)
     */
    void addMeshNode(std::unique_ptr<MeshNode> node);
    
    /**
     * @brief Add a mesh to the scene and create a node for it
     * @param name Display name for the mesh
     * @param meshData The mesh data (ownership transferred)
     * @return The unique ID of the created mesh node, or 0 on failure
     */
    uint64_t addMeshNode(const std::string& name, std::unique_ptr<geometry::MeshData> meshData);
    
    /**
     * @brief Detach a mesh node from the scene without destroying it
     * @param id Mesh node identifier
     * @return The detached mesh node, or nullptr if not found
     */
    std::unique_ptr<MeshNode> detachMeshNode(uint64_t id);
    
    /**
     * @brief Set transformation for a node
     * @param nodeId Node identifier
     * @param transform 4x4 transformation matrix
     */
    void setNodeTransform(uint64_t nodeId, const glm::mat4& transform);
    
    /**
     * @brief Restore a previously detached node
     * @param node The node to restore (ownership transferred)
     * @param parentId Parent node ID (0 for root)
     * @param index Position index in parent's child list
     */
    void restoreNode(std::unique_ptr<SceneNode> node, uint64_t parentId, size_t index);
    
    /**
     * @brief Detach a node from the scene without destroying it
     * @param nodeId Node identifier
     * @return The detached node, or nullptr if not found
     */
    std::unique_ptr<SceneNode> detachNode(uint64_t nodeId);
    
    /**
     * @brief Get the parent ID of a node
     * @param nodeId Node identifier
     * @return Parent node ID, or 0 if node is at root level or not found
     */
    uint64_t getParentId(uint64_t nodeId) const;
    
    /**
     * @brief Get the index of a node within its parent's child list
     * @param nodeId Node identifier
     * @return Index position, or 0 if not found
     */
    size_t getNodeIndex(uint64_t nodeId) const;

signals:
    /**
     * @brief Emitted when the scene structure changes
     */
    void sceneChanged();
    
    /**
     * @brief Emitted when a mesh is added
     * @param id Mesh identifier
     * @param name Mesh display name
     */
    void meshAdded(uint64_t id, const QString& name);
    
    /**
     * @brief Emitted when a mesh is removed
     * @param id Mesh identifier
     */
    void meshRemoved(uint64_t id);
    
    /**
     * @brief Emitted when mesh visibility changes
     * @param id Mesh identifier
     * @param visible New visibility state
     */
    void meshVisibilityChanged(uint64_t id, bool visible);
    
    /**
     * @brief Emitted when a group is created
     * @param id Group identifier
     */
    void groupCreated(uint64_t id);
    
    /**
     * @brief Emitted when a group is deleted
     * @param id Group identifier
     */
    void groupDeleted(uint64_t id);

private:
    std::vector<std::unique_ptr<SceneNode>> m_nodes;
    std::unordered_map<uint64_t, std::unique_ptr<MeshNode>> m_meshNodes;
    std::unordered_map<uint64_t, std::unique_ptr<ObjectGroup>> m_groups;
    uint64_t m_nextMeshId = 1;
    uint64_t m_nextGroupId = 1;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_SCENEMANAGER_H
