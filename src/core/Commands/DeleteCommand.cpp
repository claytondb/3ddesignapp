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
    
    // If this is a redo, we need to delete again using stored info
    if (m_executed && !m_deletedNodes.empty()) {
        for (const auto& info : m_deletedNodes) {
            m_sceneManager->removeNode(info.nodeId);
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
    for (auto it = m_deletedNodes.rbegin(); it != m_deletedNodes.rend(); ++it) {
        if (it->node) {
            // Clone the node for restoration (keep original for potential redo)
            auto nodeClone = it->node->clone();
            m_sceneManager->restoreNode(std::move(nodeClone), it->parentId, it->index);
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
