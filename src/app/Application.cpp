/**
 * @file Application.cpp
 * @brief Implementation of the Application class
 */

#include "Application.h"
#include "ImportCommand.h"
#include "core/SceneManager.h"
#include "core/Selection.h"
#include "core/IntegrationController.h"
#include "geometry/MeshData.h"
#include "io/MeshImporter.h"
#include "io/STLImporter.h"
#include "io/OBJImporter.h"
#include "io/PLYImporter.h"
#include "renderer/Picking.h"
#include "renderer/Viewport.h"
#include "ui/MainWindow.h"
#include "ui/ObjectBrowser.h"

#include <filesystem>

#include <QUndoStack>
#include <QFileInfo>
#include <QDebug>

namespace dc3d {

Application* Application::s_instance = nullptr;

Application::Application(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
}

Application::~Application()
{
    shutdown();
    s_instance = nullptr;
}

Application* Application::instance()
{
    return s_instance;
}

bool Application::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "Initializing Application...";
    
    // Initialize undo stack
    m_undoStack = std::make_unique<QUndoStack>(this);
    
    // Initialize scene manager
    m_sceneManager = std::make_unique<core::SceneManager>();
    
    // Initialize selection system
    m_selection = std::make_unique<core::Selection>();
    
    // Initialize picking system
    m_picking = std::make_unique<renderer::Picking>();
    
    // Initialize integration controller
    m_integrationController = std::make_unique<dc3d::IntegrationController>();
    
    m_initialized = true;
    qDebug() << "Application initialized successfully";
    return true;
}

void Application::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "Shutting down Application...";
    
    // Cleanup in reverse order of initialization
    m_integrationController.reset();
    m_picking.reset();
    m_selection.reset();
    m_undoStack.reset();
    m_sceneManager.reset();
    
    m_initialized = false;
}

void Application::setMainWindow(MainWindow* window)
{
    m_mainWindow = window;
    
    if (m_mainWindow && m_integrationController) {
        // Initialize the integration controller with all components
        m_integrationController->initialize(
            m_sceneManager.get(),
            m_mainWindow->viewport(),
            m_selection.get(),
            m_picking.get(),
            m_mainWindow
        );
        
        qDebug() << "Integration controller connected to main window";
    }
}

uint64_t Application::generateMeshId()
{
    return m_nextMeshId++;
}

bool Application::importMesh(const QString& filePath)
{
    qDebug() << "Importing mesh:" << filePath;
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QString error = QString("File not found: %1").arg(filePath);
        qWarning() << error;
        emit importFailed(error);
        return false;
    }
    
    QString extension = fileInfo.suffix().toLower();
    std::filesystem::path path = filePath.toStdString();
    
    // Import based on file extension
    geometry::Result<geometry::MeshData> result;
    
    if (extension == "stl") {
        io::STLImportOptions options;
        options.computeNormals = true;
        options.mergeVertexTolerance = 1e-6f;
        result = io::STLImporter::import(path, options);
    } 
    else if (extension == "obj") {
        io::OBJImportOptions options;
        options.computeNormalsIfMissing = true;
        options.triangulate = true;
        result = io::OBJImporter::import(path, options);
    }
    else if (extension == "ply") {
        io::PLYImportOptions options;
        options.computeNormalsIfMissing = true;
        result = io::PLYImporter::import(path, options);
    }
    else {
        QString error = QString("Unsupported file format: %1").arg(extension);
        qWarning() << error;
        emit importFailed(error);
        return false;
    }
    
    if (!result.ok()) {
        QString error = QString::fromStdString(result.error);
        qWarning() << "Import failed:" << error;
        emit importFailed(error);
        return false;
    }
    
    // Generate mesh ID and name
    uint64_t meshId = generateMeshId();
    QString meshName = fileInfo.baseName();
    
    // Create mesh data
    auto mesh = std::make_shared<geometry::MeshData>(std::move(*result.value));
    
    // Compute normals if not present
    if (!mesh->hasNormals()) {
        mesh->computeNormals();
    }
    
    // Create and execute import command (for undo support)
    // Note: push() calls redo() which adds mesh via IntegrationController
    ImportCommand* cmd = new ImportCommand(meshId, meshName, mesh, this);
    m_undoStack->push(cmd);
    
    qDebug() << "Mesh imported successfully:" << meshName 
             << "Vertices:" << mesh->vertexCount()
             << "Faces:" << mesh->faceCount();
    
    emit meshImported(meshName, meshId);
    
    return true;
}

} // namespace dc3d
