/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager and SceneNode classes
 * 
 * Thread Safety Note: SceneManager is designed to be accessed from the main
 * (UI) thread only. All signal/slot connections should use Qt::AutoConnection
 * to ensure thread safety when signals cross thread boundaries.
 */

#include "SceneManager.h"
#include "geometry/MeshData.h"

#include <QDebug>

namespace dc3d {
namespace core {

// ============================================================================
// SceneNode
// ============================================================================

SceneNode::SceneNode(const std::string& name)
    : m_name(name)
{
}

// ============================================================================
// MeshNode
// ============================================================================

MeshNode::MeshNode(uint64_t id, const QString& name, std::shared_ptr<geometry::MeshData> mesh)
    : SceneNode(name.toStdString())
    , m_id(id)
    , m_displayName(name)
    , m_mesh(mesh)
{
}

// ============================================================================
// SceneManager
// ============================================================================

SceneManager::SceneManager(QObject* parent)
    : QObject(parent)
{
}

SceneManager::~SceneManager() = default;

void SceneManager::clear()
{
    // Emit meshRemoved signals for each mesh before clearing
    // This allows connected components to properly clean up their state
    for (const auto& pair : m_meshNodes) {
        emit meshRemoved(pair.first);
    }
    
    m_nodes.clear();
    m_meshNodes.clear();
    emit sceneChanged();
}

void SceneManager::addNode(std::unique_ptr<SceneNode> node)
{
    m_nodes.push_back(std::move(node));
    emit sceneChanged();
}

// ---- Mesh Management ----

void SceneManager::addMesh(uint64_t id, const QString& name, std::shared_ptr<geometry::MeshData> mesh)
{
    if (!mesh) {
        qWarning() << "SceneManager::addMesh - null mesh";
        return;
    }
    
    // Check if mesh already exists
    if (m_meshNodes.find(id) != m_meshNodes.end()) {
        qWarning() << "SceneManager::addMesh - mesh already exists with id" << id;
        return;
    }
    
    auto meshNode = std::make_unique<MeshNode>(id, name, mesh);
    m_meshNodes[id] = std::move(meshNode);
    
    qDebug() << "SceneManager::addMesh - Added mesh" << name 
             << "with id" << id
             << "(" << mesh->vertexCount() << "vertices,"
             << mesh->faceCount() << "faces)";
    
    emit meshAdded(id, name);
    emit sceneChanged();
}

void SceneManager::removeMesh(uint64_t id)
{
    auto it = m_meshNodes.find(id);
    if (it == m_meshNodes.end()) {
        qWarning() << "SceneManager::removeMesh - mesh not found with id" << id;
        return;
    }
    
    QString name = it->second->displayName();
    m_meshNodes.erase(it);
    
    qDebug() << "SceneManager::removeMesh - Removed mesh" << name << "with id" << id;
    
    emit meshRemoved(id);
    emit sceneChanged();
}

std::shared_ptr<geometry::MeshData> SceneManager::getMesh(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second->mesh();
    }
    return nullptr;
}

MeshNode* SceneManager::getMeshNode(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool SceneManager::hasMesh(uint64_t id) const
{
    return m_meshNodes.find(id) != m_meshNodes.end();
}

std::vector<uint64_t> SceneManager::meshIds() const
{
    std::vector<uint64_t> ids;
    ids.reserve(m_meshNodes.size());
    for (const auto& pair : m_meshNodes) {
        ids.push_back(pair.first);
    }
    return ids;
}

void SceneManager::setMeshVisible(uint64_t id, bool visible)
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        if (it->second->isVisible() != visible) {
            it->second->setVisible(visible);
            emit meshVisibilityChanged(id, visible);
            emit sceneChanged();
        }
    }
}

bool SceneManager::isMeshVisible(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second->isVisible();
    }
    return false;
}

// ---- Node Management (for undo/redo support) ----

void SceneManager::addMeshNode(std::unique_ptr<MeshNode> node)
{
    if (!node) {
        qWarning() << "SceneManager::addMeshNode - null node";
        return;
    }
    
    uint64_t id = node->id();
    QString name = node->displayName();
    
    // Check if mesh already exists
    if (m_meshNodes.find(id) != m_meshNodes.end()) {
        qWarning() << "SceneManager::addMeshNode - mesh already exists with id" << id;
        return;
    }
    
    m_meshNodes[id] = std::move(node);
    
    emit meshAdded(id, name);
    emit sceneChanged();
}

uint64_t SceneManager::addMeshNode(const std::string& name, std::unique_ptr<geometry::MeshData> meshData)
{
    if (!meshData) {
        qWarning() << "SceneManager::addMeshNode - null mesh data";
        return 0;
    }
    
    uint64_t id = m_nextMeshId++;
    QString qname = QString::fromStdString(name);
    
    auto mesh = std::make_shared<geometry::MeshData>(std::move(*meshData));
    auto node = std::make_unique<MeshNode>(id, qname, mesh);
    
    m_meshNodes[id] = std::move(node);
    
    qDebug() << "SceneManager::addMeshNode - Added mesh" << qname 
             << "with id" << id
             << "(" << mesh->vertexCount() << "vertices,"
             << mesh->faceCount() << "faces)";
    
    emit meshAdded(id, qname);
    emit sceneChanged();
    
    return id;
}

std::unique_ptr<MeshNode> SceneManager::detachMeshNode(uint64_t id)
{
    auto it = m_meshNodes.find(id);
    if (it == m_meshNodes.end()) {
        qWarning() << "SceneManager::detachMeshNode - mesh not found with id" << id;
        return nullptr;
    }
    
    auto node = std::move(it->second);
    m_meshNodes.erase(it);
    
    emit meshRemoved(id);
    emit sceneChanged();
    
    return node;
}

void SceneManager::setNodeTransform(uint64_t nodeId, const glm::mat4& transform)
{
    // Currently MeshNode doesn't store transform - this is a placeholder
    // for future transform support. For now, just verify the node exists.
    auto it = m_meshNodes.find(nodeId);
    if (it == m_meshNodes.end()) {
        qWarning() << "SceneManager::setNodeTransform - node not found with id" << nodeId;
        return;
    }
    
    // TODO: When MeshNode gains transform support, apply it here
    Q_UNUSED(transform);
    
    emit sceneChanged();
}

void SceneManager::restoreNode(std::unique_ptr<SceneNode> node, uint64_t parentId, size_t index)
{
    if (!node) {
        qWarning() << "SceneManager::restoreNode - null node";
        return;
    }
    
    // For now, we only support restoring nodes at root level
    // parentId is reserved for future hierarchical scene support
    Q_UNUSED(parentId);
    Q_UNUSED(index);
    
    // Check if this is a MeshNode and restore to m_meshNodes
    MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());
    if (meshNode) {
        uint64_t id = meshNode->id();
        QString name = meshNode->displayName();
        
        // Transfer ownership to unique_ptr<MeshNode>
        node.release();  // Release from SceneNode unique_ptr
        std::unique_ptr<MeshNode> meshNodePtr(meshNode);
        
        // Restore to mesh nodes
        m_meshNodes[id] = std::move(meshNodePtr);
        
        emit meshAdded(id, name);
        emit sceneChanged();
        return;
    }
    
    // Otherwise add to generic nodes
    if (index < m_nodes.size()) {
        m_nodes.insert(m_nodes.begin() + index, std::move(node));
    } else {
        m_nodes.push_back(std::move(node));
    }
    
    emit sceneChanged();
}

std::unique_ptr<SceneNode> SceneManager::detachNode(uint64_t nodeId)
{
    // First check mesh nodes
    auto meshIt = m_meshNodes.find(nodeId);
    if (meshIt != m_meshNodes.end()) {
        auto node = std::move(meshIt->second);
        m_meshNodes.erase(meshIt);
        emit meshRemoved(nodeId);
        emit sceneChanged();
        return node;
    }
    
    // Check generic nodes (by matching name or other criteria)
    // For now, nodeId doesn't directly map to generic nodes
    // This is a simplified implementation
    qWarning() << "SceneManager::detachNode - node not found with id" << nodeId;
    return nullptr;
}

uint64_t SceneManager::getParentId(uint64_t nodeId) const
{
    // Currently all nodes are at root level, so parent is always 0
    // Check if the node exists first
    if (m_meshNodes.find(nodeId) != m_meshNodes.end()) {
        return 0;  // Root level
    }
    
    qWarning() << "SceneManager::getParentId - node not found with id" << nodeId;
    return 0;
}

size_t SceneManager::getNodeIndex(uint64_t nodeId) const
{
    // For mesh nodes, find position in the ordered iteration
    size_t index = 0;
    for (const auto& pair : m_meshNodes) {
        if (pair.first == nodeId) {
            return index;
        }
        ++index;
    }
    
    qWarning() << "SceneManager::getNodeIndex - node not found with id" << nodeId;
    return 0;
}

} // namespace core
} // namespace dc3d
