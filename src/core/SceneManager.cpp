/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager, SceneNode, and ObjectGroup classes
 * 
 * Thread Safety Note: SceneManager is designed to be accessed from the main
 * (UI) thread only. All signal/slot connections should use Qt::AutoConnection
 * to ensure thread safety when signals cross thread boundaries.
 */

#include "SceneManager.h"
#include "geometry/MeshData.h"

#include <QDebug>
#include <algorithm>

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
// ObjectGroup
// ============================================================================

ObjectGroup::ObjectGroup(uint64_t id, const QString& name)
    : m_id(id)
    , m_name(name)
{
}

void ObjectGroup::addMember(uint64_t nodeId)
{
    if (!hasMember(nodeId)) {
        m_members.push_back(nodeId);
    }
}

void ObjectGroup::removeMember(uint64_t nodeId)
{
    auto it = std::find(m_members.begin(), m_members.end(), nodeId);
    if (it != m_members.end()) {
        m_members.erase(it);
    }
}

bool ObjectGroup::hasMember(uint64_t nodeId) const
{
    return std::find(m_members.begin(), m_members.end(), nodeId) != m_members.end();
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
    
    // Emit groupDeleted signals
    for (const auto& pair : m_groups) {
        emit groupDeleted(pair.first);
    }
    
    m_nodes.clear();
    m_meshNodes.clear();
    m_groups.clear();
    emit sceneChanged();
}

void SceneManager::addNode(std::unique_ptr<SceneNode> node)
{
    node->setSortOrder(m_nextSortOrder++);
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
    meshNode->setSortOrder(m_nextSortOrder++);
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
    
    // Remove from any group
    uint64_t groupId = it->second->groupId();
    if (groupId != 0) {
        auto groupIt = m_groups.find(groupId);
        if (groupIt != m_groups.end()) {
            groupIt->second->removeMember(id);
        }
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

void SceneManager::setMeshLocked(uint64_t id, bool locked)
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        if (it->second->isLocked() != locked) {
            it->second->setLocked(locked);
            emit meshLockedChanged(id, locked);
            emit sceneChanged();
        }
    }
}

bool SceneManager::isMeshLocked(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second->isLocked();
    }
    return false;
}

void SceneManager::renameMesh(uint64_t id, const QString& name)
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        it->second->setDisplayName(name);
        emit meshRenamed(id, name);
        emit sceneChanged();
    }
}

QString SceneManager::getMeshName(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second->displayName();
    }
    return QString();
}

// ---- Group Management ----

uint64_t SceneManager::createGroup(const std::vector<uint64_t>& memberIds, const QString& name)
{
    if (memberIds.empty()) {
        qWarning() << "SceneManager::createGroup - no members specified";
        return 0;
    }
    
    // Verify all members exist
    for (uint64_t id : memberIds) {
        if (m_meshNodes.find(id) == m_meshNodes.end()) {
            qWarning() << "SceneManager::createGroup - member not found:" << id;
            return 0;
        }
    }
    
    uint64_t groupId = m_nextGroupId++;
    QString groupName = name.isEmpty() ? QString("Group %1").arg(groupId) : name;
    
    auto group = std::make_unique<ObjectGroup>(groupId, groupName);
    group->setSortOrder(m_nextSortOrder++);
    
    // Add members
    for (uint64_t id : memberIds) {
        // Remove from existing group first
        auto meshIt = m_meshNodes.find(id);
        if (meshIt != m_meshNodes.end()) {
            uint64_t oldGroupId = meshIt->second->groupId();
            if (oldGroupId != 0) {
                auto oldGroupIt = m_groups.find(oldGroupId);
                if (oldGroupIt != m_groups.end()) {
                    oldGroupIt->second->removeMember(id);
                }
            }
            meshIt->second->setGroupId(groupId);
        }
        group->addMember(id);
    }
    
    m_groups[groupId] = std::move(group);
    
    qDebug() << "SceneManager::createGroup - Created group" << groupName 
             << "with id" << groupId << "and" << memberIds.size() << "members";
    
    emit groupCreated(groupId, groupName);
    emit sceneChanged();
    
    return groupId;
}

void SceneManager::deleteGroup(uint64_t groupId)
{
    auto it = m_groups.find(groupId);
    if (it == m_groups.end()) {
        qWarning() << "SceneManager::deleteGroup - group not found:" << groupId;
        return;
    }
    
    // Remove group reference from all members
    for (uint64_t memberId : it->second->members()) {
        auto meshIt = m_meshNodes.find(memberId);
        if (meshIt != m_meshNodes.end()) {
            meshIt->second->setGroupId(0);
        }
    }
    
    QString groupName = it->second->name();
    m_groups.erase(it);
    
    qDebug() << "SceneManager::deleteGroup - Deleted group" << groupName;
    
    emit groupDeleted(groupId);
    emit sceneChanged();
}

ObjectGroup* SceneManager::getGroup(uint64_t groupId) const
{
    auto it = m_groups.find(groupId);
    if (it != m_groups.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<uint64_t> SceneManager::groupIds() const
{
    std::vector<uint64_t> ids;
    ids.reserve(m_groups.size());
    for (const auto& pair : m_groups) {
        ids.push_back(pair.first);
    }
    return ids;
}

void SceneManager::addToGroup(uint64_t nodeId, uint64_t groupId)
{
    auto meshIt = m_meshNodes.find(nodeId);
    if (meshIt == m_meshNodes.end()) {
        qWarning() << "SceneManager::addToGroup - node not found:" << nodeId;
        return;
    }
    
    auto groupIt = m_groups.find(groupId);
    if (groupIt == m_groups.end()) {
        qWarning() << "SceneManager::addToGroup - group not found:" << groupId;
        return;
    }
    
    // Remove from existing group first
    uint64_t oldGroupId = meshIt->second->groupId();
    if (oldGroupId != 0 && oldGroupId != groupId) {
        auto oldGroupIt = m_groups.find(oldGroupId);
        if (oldGroupIt != m_groups.end()) {
            oldGroupIt->second->removeMember(nodeId);
            emit groupMembershipChanged(oldGroupId);
        }
    }
    
    meshIt->second->setGroupId(groupId);
    groupIt->second->addMember(nodeId);
    
    emit groupMembershipChanged(groupId);
    emit sceneChanged();
}

void SceneManager::removeFromGroup(uint64_t nodeId)
{
    auto meshIt = m_meshNodes.find(nodeId);
    if (meshIt == m_meshNodes.end()) {
        return;
    }
    
    uint64_t groupId = meshIt->second->groupId();
    if (groupId == 0) {
        return;
    }
    
    meshIt->second->setGroupId(0);
    
    auto groupIt = m_groups.find(groupId);
    if (groupIt != m_groups.end()) {
        groupIt->second->removeMember(nodeId);
        emit groupMembershipChanged(groupId);
    }
    
    emit sceneChanged();
}

uint64_t SceneManager::getObjectGroup(uint64_t nodeId) const
{
    auto meshIt = m_meshNodes.find(nodeId);
    if (meshIt != m_meshNodes.end()) {
        return meshIt->second->groupId();
    }
    return 0;
}

void SceneManager::setGroupVisible(uint64_t groupId, bool visible)
{
    auto it = m_groups.find(groupId);
    if (it == m_groups.end()) {
        return;
    }
    
    it->second->setVisible(visible);
    
    // Apply to all members
    for (uint64_t memberId : it->second->members()) {
        auto meshIt = m_meshNodes.find(memberId);
        if (meshIt != m_meshNodes.end()) {
            meshIt->second->setVisible(visible);
            emit meshVisibilityChanged(memberId, visible);
        }
    }
    
    emit groupChanged(groupId);
    emit sceneChanged();
}

void SceneManager::setGroupLocked(uint64_t groupId, bool locked)
{
    auto it = m_groups.find(groupId);
    if (it == m_groups.end()) {
        return;
    }
    
    it->second->setLocked(locked);
    
    // Apply to all members
    for (uint64_t memberId : it->second->members()) {
        auto meshIt = m_meshNodes.find(memberId);
        if (meshIt != m_meshNodes.end()) {
            meshIt->second->setLocked(locked);
            emit meshLockedChanged(memberId, locked);
        }
    }
    
    emit groupChanged(groupId);
    emit sceneChanged();
}

void SceneManager::setGroupExpanded(uint64_t groupId, bool expanded)
{
    auto it = m_groups.find(groupId);
    if (it != m_groups.end()) {
        it->second->setExpanded(expanded);
        emit groupChanged(groupId);
    }
}

void SceneManager::renameGroup(uint64_t groupId, const QString& name)
{
    auto it = m_groups.find(groupId);
    if (it != m_groups.end()) {
        it->second->setName(name);
        emit groupChanged(groupId);
        emit sceneChanged();
    }
}

void SceneManager::setGroupTransform(uint64_t groupId, const glm::mat4& transform)
{
    auto it = m_groups.find(groupId);
    if (it == m_groups.end()) {
        return;
    }
    
    // Calculate the delta transform
    glm::mat4 oldTransform = it->second->transform();
    glm::mat4 deltaTransform = transform * glm::inverse(oldTransform);
    
    it->second->setTransform(transform);
    
    // Apply delta to all members
    for (uint64_t memberId : it->second->members()) {
        auto meshIt = m_meshNodes.find(memberId);
        if (meshIt != m_meshNodes.end()) {
            glm::mat4 memberTransform = meshIt->second->transform();
            meshIt->second->setTransform(deltaTransform * memberTransform);
        }
    }
    
    emit groupChanged(groupId);
    emit sceneChanged();
}

// ---- Sort Order Management ----

void SceneManager::moveNodeBefore(uint64_t nodeId, uint64_t beforeNodeId)
{
    auto meshIt = m_meshNodes.find(nodeId);
    if (meshIt == m_meshNodes.end()) {
        return;
    }
    
    if (beforeNodeId == 0) {
        // Move to end
        meshIt->second->setSortOrder(m_nextSortOrder++);
    } else {
        auto beforeIt = m_meshNodes.find(beforeNodeId);
        if (beforeIt != m_meshNodes.end()) {
            int beforeOrder = beforeIt->second->sortOrder();
            meshIt->second->setSortOrder(beforeOrder - 1);
        }
    }
    
    emit sceneChanged();
}

void SceneManager::moveGroupBefore(uint64_t groupId, uint64_t beforeId)
{
    auto groupIt = m_groups.find(groupId);
    if (groupIt == m_groups.end()) {
        return;
    }
    
    if (beforeId == 0) {
        groupIt->second->setSortOrder(m_nextSortOrder++);
    } else {
        // Check if beforeId is a group or mesh
        auto beforeGroupIt = m_groups.find(beforeId);
        if (beforeGroupIt != m_groups.end()) {
            int beforeOrder = beforeGroupIt->second->sortOrder();
            groupIt->second->setSortOrder(beforeOrder - 1);
        } else {
            auto beforeMeshIt = m_meshNodes.find(beforeId);
            if (beforeMeshIt != m_meshNodes.end()) {
                int beforeOrder = beforeMeshIt->second->sortOrder();
                groupIt->second->setSortOrder(beforeOrder - 1);
            }
        }
    }
    
    emit sceneChanged();
}

// ---- Visibility Operations ----

void SceneManager::hideObjects(const std::vector<uint64_t>& nodeIds)
{
    for (uint64_t id : nodeIds) {
        setMeshVisible(id, false);
    }
}

void SceneManager::unhideAll()
{
    for (auto& pair : m_meshNodes) {
        if (!pair.second->isVisible()) {
            pair.second->setVisible(true);
            emit meshVisibilityChanged(pair.first, true);
        }
    }
    
    for (auto& pair : m_groups) {
        if (!pair.second->isVisible()) {
            pair.second->setVisible(true);
            emit groupChanged(pair.first);
        }
    }
    
    emit sceneChanged();
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
    
    node->setSortOrder(m_nextSortOrder++);
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
    node->setSortOrder(m_nextSortOrder++);
    
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
    
    // Remove from group
    uint64_t groupId = it->second->groupId();
    if (groupId != 0) {
        auto groupIt = m_groups.find(groupId);
        if (groupIt != m_groups.end()) {
            groupIt->second->removeMember(id);
            emit groupMembershipChanged(groupId);
        }
    }
    
    auto node = std::move(it->second);
    m_meshNodes.erase(it);
    
    emit meshRemoved(id);
    emit sceneChanged();
    
    return node;
}

void SceneManager::setNodeTransform(uint64_t nodeId, const glm::mat4& transform)
{
    auto it = m_meshNodes.find(nodeId);
    if (it == m_meshNodes.end()) {
        qWarning() << "SceneManager::setNodeTransform - node not found with id" << nodeId;
        return;
    }
    
    it->second->setTransform(transform);
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
        // Remove from group
        uint64_t groupId = meshIt->second->groupId();
        if (groupId != 0) {
            auto groupIt = m_groups.find(groupId);
            if (groupIt != m_groups.end()) {
                groupIt->second->removeMember(nodeId);
                emit groupMembershipChanged(groupId);
            }
        }
        
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
