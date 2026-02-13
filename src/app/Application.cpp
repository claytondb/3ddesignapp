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
#include "geometry/PrimitiveGenerator.h"
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
    
    try {
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
        
        try {
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
        } catch (const std::exception& e) {
            QString error = QString("Import exception: %1").arg(e.what());
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
        
        if (!result.value) {
            QString error = "Import returned null mesh data";
            qWarning() << error;
            emit importFailed(error);
            return false;
        }
        
        // Generate mesh ID and name
        uint64_t meshId = generateMeshId();
        QString meshName = fileInfo.baseName();
        
        // Create mesh data
        auto mesh = std::make_shared<geometry::MeshData>(std::move(*result.value));
        
        if (!mesh || mesh->isEmpty()) {
            QString error = "Imported mesh is empty or invalid";
            qWarning() << error;
            emit importFailed(error);
            return false;
        }
        
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
        
    } catch (const std::bad_alloc& e) {
        QString error = QString("Out of memory while importing mesh: %1").arg(e.what());
        qWarning() << error;
        emit importFailed(error);
        return false;
    } catch (const std::exception& e) {
        QString error = QString("Unexpected error importing mesh: %1").arg(e.what());
        qWarning() << error;
        emit importFailed(error);
        return false;
    } catch (...) {
        QString error = "Unknown error occurred while importing mesh";
        qWarning() << error;
        emit importFailed(error);
        return false;
    }
}

bool Application::createPrimitive(const QString& type)
{
    qDebug() << "Creating primitive:" << type;
    
    try {
        geometry::MeshData meshData;
        QString primitiveName;
        
        // Generate mesh based on primitive type
        if (type == "sphere") {
            meshData = geometry::PrimitiveGenerator::createSphere(glm::vec3(0.0f), 1.0f);
            primitiveName = "Sphere";
        } else if (type == "cube") {
            meshData = geometry::PrimitiveGenerator::createCube(glm::vec3(0.0f), 2.0f);
            primitiveName = "Cube";
        } else if (type == "cylinder") {
            meshData = geometry::PrimitiveGenerator::createCylinder(glm::vec3(0.0f), 0.5f, 2.0f);
            primitiveName = "Cylinder";
        } else if (type == "cone") {
            meshData = geometry::PrimitiveGenerator::createCone(glm::vec3(0.0f), 0.5f, 2.0f);
            primitiveName = "Cone";
        } else if (type == "plane") {
            meshData = geometry::PrimitiveGenerator::createPlane(glm::vec3(0.0f), 2.0f, 2.0f);
            primitiveName = "Plane";
        } else if (type == "torus") {
            meshData = geometry::PrimitiveGenerator::createTorus(glm::vec3(0.0f), 1.0f, 0.3f);
            primitiveName = "Torus";
        } else {
            qWarning() << "Unknown primitive type:" << type;
            return false;
        }
        
        if (meshData.isEmpty()) {
            qWarning() << "Generated primitive mesh is empty";
            return false;
        }
        
        // Generate unique mesh ID and name
        uint64_t meshId = generateMeshId();
        QString meshName = QString("%1_%2").arg(primitiveName).arg(meshId);
        
        // Create shared pointer from mesh data
        auto mesh = std::make_shared<geometry::MeshData>(std::move(meshData));
        
        // Compute normals if not already present
        if (!mesh->hasNormals()) {
            mesh->computeNormals();
        }
        
        // Create and execute import command (reuse for undo support)
        ImportCommand* cmd = new ImportCommand(meshId, meshName, mesh, this);
        m_undoStack->push(cmd);
        
        qDebug() << "Primitive created successfully:" << meshName
                 << "Vertices:" << mesh->vertexCount()
                 << "Faces:" << mesh->faceCount();
        
        emit meshImported(meshName, meshId);
        
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "Error creating primitive:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "Unknown error creating primitive";
        return false;
    }
}

} // namespace dc3d
