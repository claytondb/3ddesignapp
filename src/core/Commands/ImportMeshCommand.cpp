/**
 * @file ImportMeshCommand.cpp
 * @brief Implementation of ImportMeshCommand
 */

#include "ImportMeshCommand.h"
#include "../SceneManager.h"
#include "../../io/MeshImporter.h"

#include <QFileInfo>
#include <QLocale>

namespace dc3d {
namespace core {

ImportMeshCommand::ImportMeshCommand(SceneManager* sceneManager, const QString& filePath)
    : Command(QStringLiteral("Import Mesh"))
    , m_sceneManager(sceneManager)
    , m_filePath(filePath)
    , m_meshData(nullptr)
    , m_nodeId(0)
    , m_success(false)
    , m_isInScene(false)
{
}

ImportMeshCommand::ImportMeshCommand(SceneManager* sceneManager, const QString& filePath,
                                     std::unique_ptr<geometry::MeshData> meshData)
    : Command(QStringLiteral("Import Mesh"))
    , m_sceneManager(sceneManager)
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
    
    // Redo case: we have a detached node to re-add
    if (m_detachedNode) {
        m_sceneManager->addMeshNode(std::move(m_detachedNode));
        m_isInScene = true;
        m_success = true;
        return;
    }
    
    // First execution - need to load the mesh
    if (!m_meshData) {
        if (!loadMeshFromFile()) {
            return;  // Error already set
        }
    }
    
    // Add mesh to scene
    QFileInfo fileInfo(m_filePath);
    QString nodeName = fileInfo.baseName();
    
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
    
    // Remove the mesh from scene but keep the node for redo
    m_detachedNode = m_sceneManager->detachMeshNode(m_nodeId);
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
        
        // Store import statistics for user feedback
        m_vertexCount = result.vertexCount;
        m_faceCount = result.faceCount;
        m_loadTimeMs = result.loadTimeMs;
        
        return true;
    } else {
        m_success = false;
        m_errorMessage = QString::fromStdString(result.error);
        return false;
    }
}

QString ImportMeshCommand::successMessage() const
{
    if (!m_success) {
        return QString();
    }
    
    QFileInfo info(m_filePath);
    qint64 fileSize = info.size();
    double fileSizeMB = static_cast<double>(fileSize) / (1024.0 * 1024.0);
    
    QString stats = QString("Imported \"%1\" - %2 triangles, %3 vertices")
        .arg(info.fileName())
        .arg(QLocale().toString(static_cast<qulonglong>(m_faceCount)))
        .arg(QLocale().toString(static_cast<qulonglong>(m_vertexCount)));
    
    if (fileSizeMB >= 0.1) {
        stats += QString(" (%1 MB)").arg(QString::number(fileSizeMB, 'f', 1));
    }
    
    if (m_loadTimeMs > 100) {
        if (m_loadTimeMs < 1000) {
            stats += QString(" in %1 ms").arg(static_cast<int>(m_loadTimeMs));
        } else {
            stats += QString(" in %1 s").arg(m_loadTimeMs / 1000.0, 0, 'f', 1);
        }
    }
    
    return stats;
}

} // namespace core
} // namespace dc3d
