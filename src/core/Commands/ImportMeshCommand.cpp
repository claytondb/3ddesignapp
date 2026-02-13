/**
 * @file ImportMeshCommand.cpp
 * @brief Implementation of ImportMeshCommand
 */

#include "ImportMeshCommand.h"
#include "../SceneManager.h"
#include "../../io/MeshImporter.h"

#include <QFileInfo>

namespace dc3d {
namespace core {

ImportMeshCommand::ImportMeshCommand(SceneManager* sceneManager, const QString& filePath)
    : m_sceneManager(sceneManager)
    , m_filePath(filePath)
    , m_meshData(nullptr)
    , m_nodeId(0)
    , m_success(false)
    , m_isInScene(false)
{
}

ImportMeshCommand::ImportMeshCommand(SceneManager* sceneManager, const QString& filePath,
                                     std::unique_ptr<geometry::MeshData> meshData)
    : m_sceneManager(sceneManager)
    , m_filePath(filePath)
    , m_meshData(std::move(meshData))
    , m_nodeId(0)
    , m_success(m_meshData != nullptr)
    , m_isInScene(false)
{
}

ImportMeshCommand::~ImportMeshCommand() = default;

void ImportMeshCommand::execute()
{
    if (!m_sceneManager) {
        m_success = false;
        m_errorMessage = QStringLiteral("No scene manager");
        return;
    }
    
    // If already in scene, nothing to do
    if (m_isInScene) {
        return;
    }
    
    // First execution - need to load the mesh
    if (!m_meshData) {
        if (!loadMeshFromFile()) {
            return;  // Error already set
        }
    }
    
    // Add mesh to scene (works for both first execution and redo)
    QFileInfo fileInfo(m_filePath);
    QString nodeName = fileInfo.baseName();
    
    // For redo case, m_meshData was restored in undo(), so we can reuse it
    m_nodeId = m_sceneManager->addMeshNode(nodeName.toStdString(), std::move(m_meshData));
    
    if (m_nodeId != 0) {
        m_success = true;
        m_isInScene = true;
    } else {
        m_success = false;
        m_errorMessage = QStringLiteral("Failed to add mesh to scene");
    }
}

void ImportMeshCommand::undo()
{
    if (!m_sceneManager || m_nodeId == 0) {
        return;
    }
    
    // Remove the mesh from scene but keep the data for redo
    m_meshData = m_sceneManager->detachMeshNode(m_nodeId);
    m_isInScene = false;
}

QString ImportMeshCommand::description() const
{
    QFileInfo fileInfo(m_filePath);
    return QStringLiteral("Import Mesh \"%1\"").arg(fileInfo.fileName());
}

bool ImportMeshCommand::loadMeshFromFile()
{
    auto result = io::MeshImporter::import(m_filePath.toStdString());
    
    if (result.ok()) {
        m_meshData = std::move(result.mesh);
        m_success = true;
        return true;
    } else {
        m_success = false;
        m_errorMessage = QString::fromStdString(result.error);
        return false;
    }
}

} // namespace core
} // namespace dc3d
