/**
 * @file ImportMeshCommand.h
 * @brief Command for importing mesh files with undo support
 */

#ifndef DC3D_CORE_COMMANDS_IMPORTMESHCOMMAND_H
#define DC3D_CORE_COMMANDS_IMPORTMESHCOMMAND_H

#include "../Command.h"
#include "../../geometry/MeshData.h"

#include <QString>
#include <memory>
#include <cstdint>

namespace dc3d {
namespace core {

class SceneManager;
class MeshNode;

/**
 * @class ImportMeshCommand
 * @brief Imports a mesh file and adds it to the scene
 * 
 * On execute: Loads mesh from file and adds to scene
 * On undo: Removes the mesh from the scene
 * On redo: Re-adds the stored mesh to the scene
 */
class ImportMeshCommand : public Command
{
public:
    /**
     * @brief Construct an import mesh command
     * @param sceneManager The scene manager to add mesh to
     * @param filePath Path to the mesh file to import
     */
    ImportMeshCommand(SceneManager* sceneManager, const QString& filePath);
    
    /**
     * @brief Construct with pre-loaded mesh data
     * @param sceneManager The scene manager to add mesh to
     * @param filePath Path (for description only)
     * @param meshData Already loaded mesh data
     */
    ImportMeshCommand(SceneManager* sceneManager, const QString& filePath,
                      std::unique_ptr<geometry::MeshData> meshData);
    
    ~ImportMeshCommand() override;
    
    void execute() override;
    void undo() override;
    QString description() const override;
    
    /**
     * @brief Get the ID assigned to the imported mesh
     * @return Node ID, or 0 if not yet executed
     */
    uint64_t meshNodeId() const { return m_nodeId; }
    
    /**
     * @brief Check if the import was successful
     */
    bool wasSuccessful() const { return m_success; }
    
    /**
     * @brief Get error message if import failed
     */
    const QString& errorMessage() const { return m_errorMessage; }

private:
    bool loadMeshFromFile();
    
    SceneManager* m_sceneManager;
    QString m_filePath;
    std::unique_ptr<geometry::MeshData> m_meshData;     // For initial load
    std::unique_ptr<MeshNode> m_detachedNode;           // For undo/redo cycle
    uint64_t m_nodeId;
    bool m_success;
    QString m_errorMessage;
    bool m_isInScene;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_COMMANDS_IMPORTMESHCOMMAND_H
