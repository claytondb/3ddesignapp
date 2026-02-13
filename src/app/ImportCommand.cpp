/**
 * @file ImportCommand.cpp
 * @brief Implementation of ImportCommand for undo/redo mesh import
 */

#include "ImportCommand.h"
#include "Application.h"
#include "core/SceneManager.h"
#include "core/IntegrationController.h"
#include "geometry/MeshData.h"
#include "ui/MainWindow.h"
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
    
    // Use IntegrationController to add mesh - this properly connects to:
    // SceneManager, Viewport, Picking, and ObjectBrowser via signals
    auto* integrationController = m_app->integrationController();
    if (integrationController) {
        integrationController->addMesh(m_meshId, m_meshName, m_mesh);
    } else {
        // Fallback: add directly to scene manager (not recommended)
        auto* sceneManager = m_app->sceneManager();
        if (sceneManager) {
            sceneManager->addMesh(m_meshId, m_meshName, m_mesh);
        }
    }
    
    // Fit view to show the new mesh
    auto* mainWindow = m_app->mainWindow();
    if (mainWindow) {
        auto* viewport = mainWindow->viewport();
        if (viewport) {
            viewport->fitView();
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
    
    // Use IntegrationController to remove mesh - this properly removes from:
    // SceneManager, Viewport, Picking, ObjectBrowser, and Selection
    auto* integrationController = m_app->integrationController();
    if (integrationController) {
        integrationController->removeMesh(m_meshId);
    } else {
        // Fallback: remove directly from scene manager (not recommended)
        auto* sceneManager = m_app->sceneManager();
        if (sceneManager) {
            sceneManager->removeMesh(m_meshId);
        }
    }
    
    auto* mainWindow = m_app->mainWindow();
    if (mainWindow) {
        mainWindow->setStatusMessage(QString("Undone: Import %1").arg(m_meshName));
    }
}

} // namespace dc3d
