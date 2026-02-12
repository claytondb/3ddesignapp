/**
 * @file ImportCommand.h
 * @brief Undo/Redo command for mesh import operations
 */

#ifndef DC3D_APP_IMPORTCOMMAND_H
#define DC3D_APP_IMPORTCOMMAND_H

#include <QUndoCommand>
#include <QString>
#include <memory>
#include <cstdint>

namespace dc3d {

class Application;

namespace geometry {
class MeshData;
}

/**
 * @class ImportCommand
 * @brief QUndoCommand for importing meshes with undo/redo support
 * 
 * Stores the imported mesh data to allow undoing the import
 * (removing from scene) and redoing (re-adding to scene).
 */
class ImportCommand : public QUndoCommand
{
public:
    /**
     * @brief Construct an import command
     * @param meshId Unique ID for the mesh
     * @param meshName Display name for the mesh
     * @param mesh The mesh data (shared ownership)
     * @param app Application instance for scene access
     */
    ImportCommand(uint64_t meshId,
                  const QString& meshName,
                  std::shared_ptr<geometry::MeshData> mesh,
                  Application* app,
                  QUndoCommand* parent = nullptr);
    
    ~ImportCommand() override;
    
    /**
     * @brief Execute the import (add mesh to scene)
     */
    void redo() override;
    
    /**
     * @brief Undo the import (remove mesh from scene)
     */
    void undo() override;
    
    /**
     * @brief Get the mesh ID
     */
    uint64_t meshId() const { return m_meshId; }
    
    /**
     * @brief Get the mesh name
     */
    QString meshName() const { return m_meshName; }
    
    /**
     * @brief Get the mesh data
     */
    std::shared_ptr<geometry::MeshData> mesh() const { return m_mesh; }

private:
    uint64_t m_meshId;
    QString m_meshName;
    std::shared_ptr<geometry::MeshData> m_mesh;
    Application* m_app;
    bool m_firstRedo = true;
};

} // namespace dc3d

#endif // DC3D_APP_IMPORTCOMMAND_H
