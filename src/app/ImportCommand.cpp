/**
 * @file ImportCommand.cpp
 * @brief Implementation of ImportCommand for undo/redo mesh import
 */

#include "ImportCommand.h"
#include "Application.h"
#include "core/SceneManager.h"
#include "geometry/MeshData.h"
#include "ui/MainWindow.h"
#include "ui/ObjectBrowser.h"
#include "renderer/Viewport.h"

#include <QDebug>

namespace dc3d {

ImportCommand::ImportCommand(uint64_t meshId,
                             const QString& meshName,
                             std::shared_ptr<geometry::MeshData> mesh,
                             Application* app,
                             QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_meshId(meshId)
    , m_meshName(meshName)
    , m_mesh(mesh)
    , m_app(app)
{
    setText(QString("Import %1").arg(meshName));
}

ImportCommand::~ImportCommand() = default;

void ImportCommand::redo()
{
    if (!m_app || !m_mesh) {
        qWarning() << "ImportCommand::redo - Invalid state";
        return;
    }
    
    qDebug() << "ImportCommand::redo - Adding mesh:" << m_meshName;
    
    // Add mesh to scene manager
    auto* sceneManager = m_app->sceneManager();
    if (sceneManager) {
        sceneManager->addMesh(m_meshId, m_meshName, m_mesh);
    }
    
    // Update viewport to render the mesh
    auto* mainWindow = m_app->mainWindow();
    if (mainWindow) {
        auto* viewport = mainWindow->viewport();
        if (viewport) {
            viewport->addMesh(m_meshId, m_mesh);
            viewport->fitView();  // Fit view to show the new mesh
        }
        
        // Add to object browser (only on first redo, otherwise scene signals handle it)
        if (m_firstRedo) {
            emit m_app->meshImported(m_meshName, m_meshId);
            m_firstRedo = false;
        } else {
            // Re-add to object browser on redo
            auto* objectBrowser = mainWindow->objectBrowser();
            if (objectBrowser) {
                objectBrowser->addMesh(m_meshName, QString::number(m_meshId));
            }
        }
        
        // Update status
        mainWindow->setStatusMessage(QString("Imported: %1 (%2 vertices, %3 faces)")
            .arg(m_meshName)
            .arg(m_mesh->vertexCount())
            .arg(m_mesh->faceCount()));
    }
}

void ImportCommand::undo()
{
    if (!m_app) {
        qWarning() << "ImportCommand::undo - Invalid state";
        return;
    }
    
    qDebug() << "ImportCommand::undo - Removing mesh:" << m_meshName;
    
    // Remove mesh from scene manager
    auto* sceneManager = m_app->sceneManager();
    if (sceneManager) {
        sceneManager->removeMesh(m_meshId);
    }
    
    // Remove from viewport
    auto* mainWindow = m_app->mainWindow();
    if (mainWindow) {
        auto* viewport = mainWindow->viewport();
        if (viewport) {
            viewport->removeMesh(m_meshId);
        }
        
        // Remove from object browser
        auto* objectBrowser = mainWindow->objectBrowser();
        if (objectBrowser) {
            objectBrowser->removeItem(QString::number(m_meshId));
        }
        
        mainWindow->setStatusMessage(QString("Undone: Import %1").arg(m_meshName));
    }
}

} // namespace dc3d
