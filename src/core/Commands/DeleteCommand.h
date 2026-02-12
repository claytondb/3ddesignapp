/**
 * @file DeleteCommand.h
 * @brief Command for deleting scene objects with undo support
 */

#ifndef DC3D_CORE_COMMANDS_DELETECOMMAND_H
#define DC3D_CORE_COMMANDS_DELETECOMMAND_H

#include "../Command.h"

#include <QString>
#include <QList>
#include <memory>
#include <vector>
#include <cstdint>

namespace dc3d {
namespace core {

class SceneManager;
class SceneNode;

/**
 * @struct DeletedNodeInfo
 * @brief Stores information about a deleted node for restoration
 */
struct DeletedNodeInfo
{
    uint64_t nodeId;
    std::unique_ptr<SceneNode> node;
    uint64_t parentId;  // 0 if root-level node
    int index;          // Original position in parent's children
};

/**
 * @class DeleteCommand
 * @brief Deletes selected objects from the scene
 * 
 * On execute: Removes objects from scene, stores them for undo
 * On undo: Restores objects to their original positions
 * On redo: Removes the same objects again
 */
class DeleteCommand : public Command
{
public:
    /**
     * @brief Construct a delete command for a single node
     * @param sceneManager The scene manager
     * @param nodeId ID of the node to delete
     */
    DeleteCommand(SceneManager* sceneManager, uint64_t nodeId);
    
    /**
     * @brief Construct a delete command for multiple nodes
     * @param sceneManager The scene manager
     * @param nodeIds IDs of the nodes to delete
     */
    DeleteCommand(SceneManager* sceneManager, const std::vector<uint64_t>& nodeIds);
    
    ~DeleteCommand() override;
    
    void execute() override;
    void undo() override;
    QString description() const override;
    
    /**
     * @brief Get the number of deleted nodes
     */
    size_t deletedCount() const { return m_deletedNodes.size(); }

private:
    SceneManager* m_sceneManager;
    std::vector<uint64_t> m_nodeIds;
    std::vector<DeletedNodeInfo> m_deletedNodes;
    bool m_executed;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_COMMANDS_DELETECOMMAND_H
