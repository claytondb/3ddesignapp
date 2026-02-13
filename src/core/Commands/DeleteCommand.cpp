/**
 * @file DeleteCommand.cpp
 * @brief Implementation of DeleteCommand
 */

#include "DeleteCommand.h"
#include "../SceneManager.h"

namespace dc3d {
namespace core {

DeleteCommand::DeleteCommand(SceneManager* sceneManager, uint64_t nodeId)
    : m_sceneManager(sceneManager)
    , m_nodeIds{nodeId}
    , m_executed(false)
{
}

DeleteCommand::DeleteCommand(SceneManager* sceneManager, const std::vector<uint64_t>& nodeIds)
    : m_sceneManager(sceneManager)
    , m_nodeIds(nodeIds)
    , m_executed(false)
{
}

DeleteCommand::~DeleteCommand() = default;

void DeleteCommand::execute()
{
    if (!m_sceneManager) {
        return;
    }
    
    // If this is a redo, the nodes are already stored from first execution
    // We need to remove them from the scene again
    if (m_executed && !m_deletedNodes.empty()) {
        for (auto& info : m_deletedNodes) {
            // Move the node from our storage back to scene, then detach it again
            // This ensures we have the same node instance throughout
            if (info.node) {
                m_sceneManager->restoreNode(std::move(info.node), info.parentId, info.index);
                info.node = m_sceneManager->detachNode(info.nodeId);
            }
        }
        return;
    }
    
    // First execution - store node info before deleting
    m_deletedNodes.clear();
    
    for (uint64_t nodeId : m_nodeIds) {
        DeletedNodeInfo info;
        info.nodeId = nodeId;
        info.parentId = m_sceneManager->getParentId(nodeId);
        info.index = m_sceneManager->getNodeIndex(nodeId);
        info.node = m_sceneManager->detachNode(nodeId);
        
        if (info.node) {
            m_deletedNodes.push_back(std::move(info));
        }
    }
    
    m_executed = true;
}

void DeleteCommand::undo()
{
    if (!m_sceneManager || m_deletedNodes.empty()) {
        return;
    }
    
    // Restore nodes in reverse order to maintain correct indices
    // Move nodes from command storage to scene (no cloning needed)
    for (auto it = m_deletedNodes.rbegin(); it != m_deletedNodes.rend(); ++it) {
        if (it->node) {
            // Move the node to the scene - we keep a detached reference via nodeId
            m_sceneManager->restoreNode(std::move(it->node), it->parentId, it->index);
            // Node is now in the scene, set our pointer to null
            // (it will be re-captured on redo via execute())
        }
    }
}

QString DeleteCommand::description() const
{
    if (m_nodeIds.size() == 1) {
        return QStringLiteral("Delete Object");
    }
    return QStringLiteral("Delete %1 Objects").arg(m_nodeIds.size());
}

} // namespace core
} // namespace dc3d
