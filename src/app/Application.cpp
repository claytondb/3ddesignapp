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
#include "io/NativeFormat.h"
#include "renderer/Picking.h"
#include "renderer/Viewport.h"
#include "ui/MainWindow.h"
#include "ui/MenuBar.h"
#include "ui/ObjectBrowser.h"

#include <filesystem>

#include <QUndoStack>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>

#include <sstream>
#include <cstring>
#include <map>

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
    
    // Initialize undo stack with configurable limit
    m_undoStack = std::make_unique<QUndoStack>(this);
    
    // Load undo limit from settings (default: 100 commands)
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    int undoLimit = settings.value("preferences/performance/undoLimit", 100).toInt();
    m_undoStack->setUndoLimit(undoLimit);
    qDebug() << "Undo limit set to:" << undoLimit;
    
    // Initialize scene manager
    m_sceneManager = std::make_unique<core::SceneManager>();
    
    // Initialize selection system
    m_selection = std::make_unique<core::Selection>();
    
    // Initialize picking system
    m_picking = std::make_unique<renderer::Picking>();
    
    // Initialize integration controller
    m_integrationController = std::make_unique<dc3d::IntegrationController>();
    
    // Load backup settings
    loadBackupSettings();
    
    // Load document settings (last directory, etc.)
    loadDocumentSettings();
    
    // Connect scene changes to modified tracking
    connect(m_sceneManager.get(), &core::SceneManager::sceneChanged, this, [this]() {
        setModified(true);
    });
    connect(m_sceneManager.get(), &core::SceneManager::meshAdded, this, [this](uint64_t, const QString&) {
        setModified(true);
    });
    connect(m_sceneManager.get(), &core::SceneManager::meshRemoved, this, [this](uint64_t) {
        setModified(true);
    });
    
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
    
    // Connect undo stack to menu bar for automatic undo/redo text updates
    if (m_mainWindow && m_undoStack) {
        MenuBar* menuBar = m_mainWindow->menuBar();
        if (menuBar) {
            menuBar->connectToUndoStack(m_undoStack.get());
            qDebug() << "Undo stack connected to menu bar";
        }
    }
}

uint64_t Application::generateMeshId()
{
    return m_nextMeshId++;
}

bool Application::importMesh(const QString& filePath)
{
    qDebug() << "Importing mesh:" << filePath;
    
    // SAFETY: Ensure application is properly initialized
    if (!m_initialized) {
        QString error = "Application not initialized";
        qWarning() << error;
        emit importFailed(error);
        return false;
    }
    
    if (!m_undoStack) {
        QString error = "Undo stack not initialized";
        qWarning() << error;
        emit importFailed(error);
        return false;
    }
    
    if (!m_integrationController) {
        QString error = "Integration controller not initialized";
        qWarning() << error;
        emit importFailed(error);
        return false;
    }
    
    try {
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            QString error = QString("File not found: %1").arg(filePath);
            qWarning() << error;
            emit importFailed(error);
            return false;
        }
        
        // File size protection for large files
        constexpr qint64 LARGE_FILE_WARNING_BYTES = 50 * 1024 * 1024;  // 50 MB
        constexpr qint64 MAX_FILE_SIZE_BYTES = 500 * 1024 * 1024;      // 500 MB hard limit
        constexpr qint64 MEMORY_MULTIPLIER = 10;  // Estimated memory overhead for mesh data
        
        qint64 fileSize = fileInfo.size();
        qint64 estimatedMemory = fileSize * MEMORY_MULTIPLIER;
        
        // Hard limit check
        if (fileSize > MAX_FILE_SIZE_BYTES) {
            QString error = QString("File too large (%1 MB). Maximum supported size is %2 MB.")
                .arg(fileSize / (1024 * 1024))
                .arg(MAX_FILE_SIZE_BYTES / (1024 * 1024));
            qWarning() << error;
            emit importFailed(error);
            return false;
        }
        
        // Warning for large files
        if (fileSize > LARGE_FILE_WARNING_BYTES) {
            QString warning = QString(
                "This file is large (%1 MB) and may require approximately %2 MB of memory.\n\n"
                "Large files can take a long time to load and may cause the application to become unresponsive.\n\n"
                "Do you want to continue?")
                .arg(fileSize / (1024 * 1024))
                .arg(estimatedMemory / (1024 * 1024));
            
            QMessageBox::StandardButton reply = QMessageBox::warning(
                m_mainWindow,
                "Large File Warning",
                warning,
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            
            if (reply != QMessageBox::Yes) {
                qDebug() << "User cancelled import of large file";
                return false;
            }
        }
        
        QString extension = fileInfo.suffix().toLower();
        std::filesystem::path path = filePath.toStdString();
        
        // Import based on file extension
        geometry::Result<geometry::MeshData> result;
        
        // Show progress dialog for larger files (> 10MB)
        constexpr qint64 PROGRESS_DIALOG_THRESHOLD = 10 * 1024 * 1024;
        std::unique_ptr<QProgressDialog> progressDialog;
        
        if (fileSize > PROGRESS_DIALOG_THRESHOLD && m_mainWindow) {
            progressDialog = std::make_unique<QProgressDialog>(
                QString("Importing %1...").arg(fileInfo.fileName()),
                "Cancel",
                0, 100,
                m_mainWindow);
            progressDialog->setWindowModality(Qt::WindowModal);
            progressDialog->setMinimumDuration(0);
            progressDialog->setValue(0);
            QApplication::processEvents();
        }
        
        // Progress callback for importers
        auto progressCallback = [&progressDialog](float progress) -> bool {
            if (progressDialog) {
                if (progressDialog->wasCanceled()) {
                    return false;  // Cancel import
                }
                progressDialog->setValue(static_cast<int>(progress * 100));
                QApplication::processEvents();
            }
            return true;  // Continue import
        };
        
        try {
            if (extension == "stl") {
                io::STLImportOptions options;
                options.computeNormals = true;
                options.mergeVertexTolerance = 1e-6f;
                result = io::STLImporter::import(path, options, progressCallback);
            } 
            else if (extension == "obj") {
                io::OBJImportOptions options;
                options.computeNormalsIfMissing = true;
                options.triangulate = true;
                result = io::OBJImporter::import(path, options, progressCallback);
            }
            else if (extension == "ply") {
                io::PLYImportOptions options;
                options.computeNormalsIfMissing = true;
                result = io::PLYImporter::import(path, options, progressCallback);
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
        
        // Close progress dialog
        if (progressDialog) {
            progressDialog->close();
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
        
        // CRITICAL FIX: Use isValid() instead of isEmpty() to catch corrupted mesh data
        if (!mesh || mesh->isEmpty() || !mesh->isValid()) {
            QString error = "Imported mesh is empty or invalid - data may be corrupted";
            qWarning() << error;
            emit importFailed(error);
            return false;
        }
        
        // Compute normals if not present
        if (!mesh->hasNormals()) {
            mesh->computeNormals();
        }
        
        // Track statistics before move
        size_t vertexCount = mesh->vertexCount();
        size_t faceCount = mesh->faceCount();
        
        // Create and execute import command (for undo support)
        // Note: push() calls redo() which adds mesh via IntegrationController
        ImportCommand* cmd = new ImportCommand(meshId, meshName, mesh, this);
        m_undoStack->push(cmd);
        
        // Calculate load time (rough estimate - time from start of try block)
        // In a real implementation, we'd track this more precisely
        double loadTimeMs = 0.0;  // TODO: Add actual timing
        
        qDebug() << "Mesh imported successfully:" << meshName 
                 << "Vertices:" << vertexCount
                 << "Faces:" << faceCount;
        
        emit meshImported(meshName, meshId, vertexCount, faceCount, loadTimeMs);
        
        return true;
        
    } catch (const std::bad_alloc& e) {
        QString error = QString("Out of memory while importing mesh: %1").arg(e.what());
        qWarning() << error;
        
        // Show user-friendly error dialog
        if (m_mainWindow) {
            QMessageBox::critical(
                m_mainWindow,
                "Import Failed - Out of Memory",
                QString("The file is too large to import. Your system ran out of memory.\n\n"
                        "Suggestions:\n"
                        "• Close other applications to free memory\n"
                        "• Try a smaller file or reduce the mesh in another application\n"
                        "• Restart the application and try again\n\n"
                        "Technical details: %1").arg(e.what()));
        }
        
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
    
    // SAFETY: Ensure application is properly initialized
    if (!m_initialized) {
        qWarning() << "Application::createPrimitive called before initialization";
        return false;
    }
    
    if (!m_undoStack) {
        qWarning() << "Application::createPrimitive - undo stack not initialized";
        return false;
    }
    
    if (!m_integrationController) {
        qWarning() << "Application::createPrimitive - integration controller not initialized";
        return false;
    }
    
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
        
        // Validate mesh data
        if (meshData.vertexCount() == 0 || meshData.faceCount() == 0) {
            qWarning() << "Generated primitive has no geometry";
            return false;
        }
        
        // Generate unique mesh ID and name
        uint64_t meshId = generateMeshId();
        QString meshName = QString("%1_%2").arg(primitiveName).arg(meshId);
        
        // Create shared pointer from mesh data
        auto mesh = std::make_shared<geometry::MeshData>(std::move(meshData));
        
        if (!mesh) {
            qWarning() << "Failed to create mesh shared pointer";
            return false;
        }
        
        // Compute normals if not already present
        if (!mesh->hasNormals()) {
            mesh->computeNormals();
        }
        
        // Create and execute import command (reuse for undo support)
        ImportCommand* cmd = new ImportCommand(meshId, meshName, mesh, this);
        if (!cmd) {
            qWarning() << "Failed to create import command";
            return false;
        }
        
        m_undoStack->push(cmd);
        
        qDebug() << "Primitive created successfully:" << meshName
                 << "Vertices:" << mesh->vertexCount()
                 << "Faces:" << mesh->faceCount();
        
        emit meshImported(meshName, meshId, mesh->vertexCount(), mesh->faceCount(), 0.0);
        
        return true;
        
    } catch (const std::bad_alloc& e) {
        qWarning() << "Out of memory creating primitive:" << e.what();
        return false;
    } catch (const std::exception& e) {
        qWarning() << "Error creating primitive:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "Unknown error creating primitive";
        return false;
    }
}

bool Application::createPrimitiveWithConfig(const QString& type, 
                                             const glm::vec3& position,
                                             float width, float height, float depth,
                                             int segments,
                                             bool selectAfterCreation)
{
    qDebug() << "Creating primitive with config:" << type 
             << "at" << position.x << position.y << position.z;
    
    // SAFETY: Ensure application is properly initialized
    if (!m_initialized || !m_undoStack || !m_integrationController) {
        qWarning() << "Application::createPrimitiveWithConfig - not properly initialized";
        return false;
    }
    
    try {
        geometry::MeshData meshData;
        QString primitiveName;
        
        // Generate mesh based on primitive type with custom dimensions
        if (type == "sphere") {
            meshData = geometry::PrimitiveGenerator::createSphere(position, width, segments, segments);
            primitiveName = "Sphere";
        } else if (type == "cube") {
            // For cube, create a box with dimensions width x height x depth
            meshData = geometry::PrimitiveGenerator::createCube(position, width);
            // Scale to get non-uniform dimensions if needed
            if (height != width || depth != width) {
                glm::vec3 scale(width, height, depth);
                meshData.scale(scale / width);  // Normalize then scale
            }
            primitiveName = "Cube";
        } else if (type == "cylinder") {
            meshData = geometry::PrimitiveGenerator::createCylinder(position, width, height, segments, 1, true);
            primitiveName = "Cylinder";
        } else if (type == "cone") {
            meshData = geometry::PrimitiveGenerator::createCone(position, width, height, segments, true);
            primitiveName = "Cone";
        } else if (type == "plane") {
            meshData = geometry::PrimitiveGenerator::createPlane(position, width, height);
            primitiveName = "Plane";
        } else if (type == "torus") {
            meshData = geometry::PrimitiveGenerator::createTorus(position, width, height, segments, segments / 2);
            primitiveName = "Torus";
        } else {
            qWarning() << "Unknown primitive type:" << type;
            return false;
        }
        
        if (meshData.isEmpty() || meshData.vertexCount() == 0) {
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
        
        // Select the new primitive if requested
        if (selectAfterCreation && m_selection) {
            m_selection->selectObject(static_cast<uint32_t>(meshId));
        }
        
        emit meshImported(meshName, meshId, mesh->vertexCount(), mesh->faceCount(), 0.0);
        
        return true;
        
    } catch (const std::bad_alloc& e) {
        qWarning() << "Out of memory creating primitive:" << e.what();
        return false;
    } catch (const std::exception& e) {
        qWarning() << "Error creating primitive:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "Unknown error creating primitive";
        return false;
    }
}

// ============================================================================
// Selection Management
// ============================================================================

void Application::deselectAll()
{
    if (m_selection) {
        m_selection->clear();
    }
}

// ============================================================================
// Auto-Backup Functionality
// ============================================================================

void Application::loadBackupSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    m_autoBackupEnabled = settings.value("backup/enabled", true).toBool();
    m_backupDirectory = settings.value("backup/directory", 
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups").toString();
    
    // Create backup directory if it doesn't exist
    QDir dir(m_backupDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

void Application::saveBackupSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    settings.setValue("backup/enabled", m_autoBackupEnabled);
    settings.setValue("backup/directory", m_backupDirectory);
}

void Application::setAutoBackupEnabled(bool enabled)
{
    m_autoBackupEnabled = enabled;
    saveBackupSettings();
    qDebug() << "Auto-backup" << (enabled ? "enabled" : "disabled");
}

void Application::setBackupDirectory(const QString& dir)
{
    m_backupDirectory = dir;
    
    // Create directory if needed
    QDir backupDir(dir);
    if (!backupDir.exists()) {
        backupDir.mkpath(".");
    }
    
    saveBackupSettings();
    qDebug() << "Backup directory set to:" << dir;
}

QString Application::createBackup(const QString& reason)
{
    if (!m_autoBackupEnabled) {
        qDebug() << "Auto-backup is disabled, skipping backup creation";
        return QString();
    }
    
    if (!m_sceneManager || m_sceneManager->meshCount() == 0) {
        qDebug() << "No meshes in scene, skipping backup";
        return QString();
    }
    
    // Create backup directory if needed
    QDir backupDir(m_backupDirectory);
    if (!backupDir.exists()) {
        if (!backupDir.mkpath(".")) {
            qWarning() << "Failed to create backup directory:" << m_backupDirectory;
            return QString();
        }
    }
    
    // Generate backup filename with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString reasonSuffix = reason.isEmpty() ? "" : "_" + reason.simplified().replace(" ", "-").left(30);
    QString backupName = QString("backup_%1%2.dc3d").arg(timestamp).arg(reasonSuffix);
    QString backupPath = backupDir.filePath(backupName);
    
    // TODO: Actually save the scene to backup file
    // For now, we create a placeholder that demonstrates the structure
    // In a full implementation, this would serialize the scene using NativeFormat
    
    QFile backupFile(backupPath);
    if (backupFile.open(QIODevice::WriteOnly)) {
        // Placeholder - in real implementation, use NativeFormat::save()
        QTextStream stream(&backupFile);
        stream << "# DC-3DDesignApp Backup\n";
        stream << "# Created: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        stream << "# Reason: " << reason << "\n";
        stream << "# Mesh count: " << m_sceneManager->meshCount() << "\n";
        backupFile.close();
        
        qDebug() << "Backup created:" << backupPath;
        
        // Clean up old backups
        cleanupOldBackups();
        
        return backupPath;
    } else {
        qWarning() << "Failed to create backup file:" << backupPath;
        return QString();
    }
}

QStringList Application::recentBackups() const
{
    QDir backupDir(m_backupDirectory);
    if (!backupDir.exists()) {
        return QStringList();
    }
    
    QStringList filters;
    filters << "backup_*.dc3d";
    
    QFileInfoList files = backupDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    QStringList result;
    for (const QFileInfo& info : files) {
        result.append(info.absoluteFilePath());
    }
    
    return result;
}

bool Application::restoreFromBackup(const QString& backupPath)
{
    if (!QFile::exists(backupPath)) {
        qWarning() << "Backup file not found:" << backupPath;
        return false;
    }
    
    // TODO: Implement actual restore using NativeFormat::load()
    // For now, just log the intent
    qDebug() << "Restoring from backup:" << backupPath;
    
    // In full implementation:
    // 1. Clear current scene
    // 2. Load backup file
    // 3. Rebuild viewport
    
    return true;
}

void Application::cleanupOldBackups()
{
    QDir backupDir(m_backupDirectory);
    if (!backupDir.exists()) {
        return;
    }
    
    QStringList filters;
    filters << "backup_*.dc3d";
    
    QFileInfoList files = backupDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    // Keep only MAX_BACKUPS most recent
    while (files.size() > MAX_BACKUPS) {
        QFileInfo oldest = files.takeLast();
        if (QFile::remove(oldest.absoluteFilePath())) {
            qDebug() << "Removed old backup:" << oldest.fileName();
        }
    }
}

void Application::reloadPreferences()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    
    // Update undo limit
    if (m_undoStack) {
        int undoLimit = settings.value("preferences/performance/undoLimit", 100).toInt();
        m_undoStack->setUndoLimit(undoLimit);
        qDebug() << "Undo limit updated to:" << undoLimit;
    }
    
    // Future: add other preference reloading here
    // - Auto-backup settings
    // - Viewport settings
    // - etc.
    
    qDebug() << "Preferences reloaded";
}

// ============================================================================
// Document Management Implementation
// ============================================================================

void Application::loadDocumentSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    m_lastUsedDirectory = settings.value("document/lastDirectory",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
}

void Application::saveDocumentSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    settings.setValue("document/lastDirectory", m_lastUsedDirectory);
}

void Application::setLastUsedDirectory(const QString& dir)
{
    m_lastUsedDirectory = dir;
    saveDocumentSettings();
}

void Application::setModified(bool modified)
{
    if (m_isModified != modified) {
        m_isModified = modified;
        emit modifiedChanged(modified);
    }
}

bool Application::newProject(bool askToSave)
{
    // Check if we need to save the current project
    if (askToSave && m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_mainWindow,
            tr("Save Changes?"),
            tr("The current project has unsaved changes.\n\nDo you want to save before creating a new project?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        
        if (reply == QMessageBox::Cancel) {
            return false;
        }
        
        if (reply == QMessageBox::Save) {
            if (!saveProject()) {
                return false;  // Save was cancelled or failed
            }
        }
    }
    
    // Clear the scene via IntegrationController (clears viewport, picking, etc.)
    if (m_integrationController) {
        m_integrationController->clearScene();
    } else if (m_sceneManager) {
        m_sceneManager->clear();
    }
    
    // Clear undo stack
    if (m_undoStack) {
        m_undoStack->clear();
    }
    
    // Reset mesh ID counter
    m_nextMeshId = 1;
    
    // Reset document state
    m_currentFilePath.clear();
    setModified(false);
    
    emit filePathChanged(m_currentFilePath);
    
    qDebug() << "New project created";
    return true;
}

bool Application::saveProject()
{
    if (m_currentFilePath.isEmpty()) {
        // No file path, use Save As
        return saveProjectAs();
    }
    
    return doSaveProject(m_currentFilePath);
}

bool Application::saveProjectAs(const QString& filePath)
{
    QString path = filePath;
    
    if (path.isEmpty()) {
        // Show save dialog
        path = QFileDialog::getSaveFileName(
            m_mainWindow,
            tr("Save Project As"),
            m_lastUsedDirectory + "/Untitled.dc3d",
            tr("DC-3D Project (*.dc3d);;All Files (*)"));
        
        if (path.isEmpty()) {
            return false;  // User cancelled
        }
        
        // Ensure .dc3d extension
        if (!path.endsWith(".dc3d", Qt::CaseInsensitive)) {
            path += ".dc3d";
        }
    }
    
    // Update last used directory
    QFileInfo fileInfo(path);
    setLastUsedDirectory(fileInfo.absolutePath());
    
    return doSaveProject(path);
}

bool Application::doSaveProject(const QString& filePath)
{
    if (!m_sceneManager) {
        qWarning() << "Cannot save: SceneManager not initialized";
        return false;
    }
    
    qDebug() << "Saving project to:" << filePath;
    
    try {
        // Create simple archive format to save meshes
        dc::SimpleArchive archive;
        std::vector<dc::SimpleArchive::Entry> entries;
        
        // Create manifest JSON with project info
        std::ostringstream manifest;
        manifest << "{\n";
        manifest << "  \"version\": 1,\n";
        manifest << "  \"format\": \"dc3d\",\n";
        manifest << "  \"meshCount\": " << m_sceneManager->meshCount() << ",\n";
        manifest << "  \"meshes\": [\n";
        
        // Get all mesh IDs
        std::vector<uint64_t> meshIds = m_sceneManager->meshIds();
        
        for (size_t i = 0; i < meshIds.size(); i++) {
            uint64_t id = meshIds[i];
            auto* meshNode = m_sceneManager->getMeshNode(id);
            if (!meshNode) continue;
            
            manifest << "    {\n";
            manifest << "      \"id\": " << id << ",\n";
            manifest << "      \"name\": \"" << meshNode->displayName().toStdString() << "\",\n";
            manifest << "      \"visible\": " << (meshNode->isVisible() ? "true" : "false") << "\n";
            manifest << "    }" << (i < meshIds.size() - 1 ? "," : "") << "\n";
            
            // Serialize mesh data
            auto mesh = meshNode->mesh();
            if (mesh) {
                std::vector<uint8_t> meshData;
                
                // Write vertex count
                uint32_t vertexCount = static_cast<uint32_t>(mesh->vertexCount());
                meshData.push_back(vertexCount & 0xFF);
                meshData.push_back((vertexCount >> 8) & 0xFF);
                meshData.push_back((vertexCount >> 16) & 0xFF);
                meshData.push_back((vertexCount >> 24) & 0xFF);
                
                // Write vertices
                const auto& vertices = mesh->vertices();
                for (const auto& v : vertices) {
                    // Write as floats (4 bytes each)
                    auto writeFloat = [&meshData](float f) {
                        uint32_t bits;
                        std::memcpy(&bits, &f, sizeof(float));
                        meshData.push_back(bits & 0xFF);
                        meshData.push_back((bits >> 8) & 0xFF);
                        meshData.push_back((bits >> 16) & 0xFF);
                        meshData.push_back((bits >> 24) & 0xFF);
                    };
                    writeFloat(v.x);
                    writeFloat(v.y);
                    writeFloat(v.z);
                }
                
                // Write index count
                uint32_t indexCount = static_cast<uint32_t>(mesh->indexCount());
                meshData.push_back(indexCount & 0xFF);
                meshData.push_back((indexCount >> 8) & 0xFF);
                meshData.push_back((indexCount >> 16) & 0xFF);
                meshData.push_back((indexCount >> 24) & 0xFF);
                
                // Write indices
                const auto& indices = mesh->indices();
                for (uint32_t idx : indices) {
                    meshData.push_back(idx & 0xFF);
                    meshData.push_back((idx >> 8) & 0xFF);
                    meshData.push_back((idx >> 16) & 0xFF);
                    meshData.push_back((idx >> 24) & 0xFF);
                }
                
                // Write normals flag and data
                bool hasNormals = mesh->hasNormals();
                meshData.push_back(hasNormals ? 1 : 0);
                if (hasNormals) {
                    const auto& normals = mesh->normals();
                    for (const auto& n : normals) {
                        auto writeFloat = [&meshData](float f) {
                            uint32_t bits;
                            std::memcpy(&bits, &f, sizeof(float));
                            meshData.push_back(bits & 0xFF);
                            meshData.push_back((bits >> 8) & 0xFF);
                            meshData.push_back((bits >> 16) & 0xFF);
                            meshData.push_back((bits >> 24) & 0xFF);
                        };
                        writeFloat(n.x);
                        writeFloat(n.y);
                        writeFloat(n.z);
                    }
                }
                
                dc::SimpleArchive::Entry meshEntry;
                meshEntry.name = "mesh_" + std::to_string(id) + ".bin";
                meshEntry.data = std::move(meshData);
                entries.push_back(std::move(meshEntry));
            }
        }
        
        manifest << "  ]\n";
        manifest << "}\n";
        
        // Add manifest entry
        dc::SimpleArchive::Entry manifestEntry;
        manifestEntry.name = "manifest.json";
        std::string manifestStr = manifest.str();
        manifestEntry.data = std::vector<uint8_t>(manifestStr.begin(), manifestStr.end());
        entries.insert(entries.begin(), std::move(manifestEntry));
        
        // Write archive
        if (!archive.write(filePath.toStdString(), entries)) {
            QMessageBox::critical(m_mainWindow, tr("Save Failed"),
                tr("Failed to write project file."));
            return false;
        }
        
        // Update document state
        m_currentFilePath = filePath;
        setModified(false);
        emit filePathChanged(m_currentFilePath);
        emit projectSaved(filePath);
        
        qDebug() << "Project saved successfully:" << filePath;
        return true;
        
    } catch (const std::exception& e) {
        QMessageBox::critical(m_mainWindow, tr("Save Failed"),
            tr("Failed to save project: %1").arg(e.what()));
        return false;
    }
}

bool Application::openProject(const QString& filePath, bool askToSave)
{
    // Check if we need to save the current project
    if (askToSave && m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_mainWindow,
            tr("Save Changes?"),
            tr("The current project has unsaved changes.\n\nDo you want to save before opening another project?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        
        if (reply == QMessageBox::Cancel) {
            return false;
        }
        
        if (reply == QMessageBox::Save) {
            if (!saveProject()) {
                return false;  // Save was cancelled or failed
            }
        }
    }
    
    return doLoadProject(filePath);
}

bool Application::doLoadProject(const QString& filePath)
{
    if (!m_sceneManager) {
        qWarning() << "Cannot load: SceneManager not initialized";
        return false;
    }
    
    if (!QFileInfo::exists(filePath)) {
        QMessageBox::critical(m_mainWindow, tr("Open Failed"),
            tr("File not found: %1").arg(filePath));
        return false;
    }
    
    qDebug() << "Loading project from:" << filePath;
    
    try {
        // Read archive
        dc::SimpleArchive archive;
        std::vector<dc::SimpleArchive::Entry> entries;
        
        if (!archive.read(filePath.toStdString(), entries)) {
            QMessageBox::critical(m_mainWindow, tr("Open Failed"),
                tr("Failed to read project file. The file may be corrupted."));
            return false;
        }
        
        // Find manifest
        std::string manifestJson;
        std::map<std::string, std::vector<uint8_t>> meshDataMap;
        
        for (const auto& entry : entries) {
            if (entry.name == "manifest.json") {
                manifestJson = std::string(entry.data.begin(), entry.data.end());
            } else if (entry.name.find("mesh_") == 0 && entry.name.find(".bin") != std::string::npos) {
                meshDataMap[entry.name] = entry.data;
            }
        }
        
        if (manifestJson.empty()) {
            QMessageBox::critical(m_mainWindow, tr("Open Failed"),
                tr("Invalid project file: missing manifest."));
            return false;
        }
        
        // Clear current scene via IntegrationController (clears viewport, picking, etc.)
        if (m_integrationController) {
            m_integrationController->clearScene();
        } else {
            m_sceneManager->clear();
        }
        if (m_undoStack) {
            m_undoStack->clear();
        }
        
        // Simple JSON parsing for mesh entries
        // Find meshes array
        size_t meshesStart = manifestJson.find("\"meshes\"");
        if (meshesStart == std::string::npos) {
            // No meshes in file, that's OK
            m_currentFilePath = filePath;
            setModified(false);
            emit filePathChanged(m_currentFilePath);
            emit projectLoaded(filePath);
            return true;
        }
        
        // Parse mesh entries (simple parsing)
        size_t pos = meshesStart;
        while (true) {
            size_t idPos = manifestJson.find("\"id\":", pos);
            if (idPos == std::string::npos || idPos > manifestJson.find("]", meshesStart)) break;
            
            // Extract ID
            size_t idStart = idPos + 5;
            while (idStart < manifestJson.size() && !std::isdigit(manifestJson[idStart])) idStart++;
            size_t idEnd = idStart;
            while (idEnd < manifestJson.size() && std::isdigit(manifestJson[idEnd])) idEnd++;
            uint64_t meshId = std::stoull(manifestJson.substr(idStart, idEnd - idStart));
            
            // Extract name
            size_t namePos = manifestJson.find("\"name\":", idPos);
            std::string meshName = "Mesh";
            if (namePos != std::string::npos && namePos < manifestJson.find("}", idPos)) {
                size_t nameStart = manifestJson.find("\"", namePos + 7);
                if (nameStart != std::string::npos) {
                    nameStart++;
                    size_t nameEnd = manifestJson.find("\"", nameStart);
                    if (nameEnd != std::string::npos) {
                        meshName = manifestJson.substr(nameStart, nameEnd - nameStart);
                    }
                }
            }
            
            // Load mesh data
            std::string meshFileName = "mesh_" + std::to_string(meshId) + ".bin";
            auto it = meshDataMap.find(meshFileName);
            if (it != meshDataMap.end()) {
                const auto& data = it->second;
                size_t offset = 0;
                
                auto readUint32 = [&data, &offset]() -> uint32_t {
                    if (offset + 4 > data.size()) return 0;
                    uint32_t val = data[offset] | (data[offset+1] << 8) | 
                                   (data[offset+2] << 16) | (data[offset+3] << 24);
                    offset += 4;
                    return val;
                };
                
                auto readFloat = [&readUint32]() -> float {
                    uint32_t bits = readUint32();
                    float val;
                    std::memcpy(&val, &bits, sizeof(float));
                    return val;
                };
                
                // Create mesh
                auto mesh = std::make_shared<geometry::MeshData>();
                
                // Read vertices
                uint32_t vertexCount = readUint32();
                mesh->reserveVertices(vertexCount);
                for (uint32_t i = 0; i < vertexCount; i++) {
                    float x = readFloat();
                    float y = readFloat();
                    float z = readFloat();
                    mesh->addVertex(glm::vec3(x, y, z));
                }
                
                // Read indices
                uint32_t indexCount = readUint32();
                mesh->reserveFaces(indexCount / 3);
                for (uint32_t i = 0; i + 2 < indexCount; i += 3) {
                    uint32_t i0 = readUint32();
                    uint32_t i1 = readUint32();
                    uint32_t i2 = readUint32();
                    mesh->addFace(i0, i1, i2);
                }
                
                // Read normals if present
                if (offset < data.size()) {
                    bool hasNormals = data[offset++] != 0;
                    if (hasNormals && mesh->vertexCount() > 0) {
                        auto& normals = mesh->normals();
                        normals.reserve(mesh->vertexCount());
                        for (size_t i = 0; i < mesh->vertexCount() && offset + 12 <= data.size(); i++) {
                            float x = readFloat();
                            float y = readFloat();
                            float z = readFloat();
                            normals.push_back(glm::vec3(x, y, z));
                        }
                    }
                }
                
                // Compute normals if not present
                if (!mesh->hasNormals()) {
                    mesh->computeNormals();
                }
                
                // Add to scene via IntegrationController (handles scene, viewport, picking, etc.)
                if (meshId >= m_nextMeshId) {
                    m_nextMeshId = meshId + 1;
                }
                if (m_integrationController) {
                    m_integrationController->addMesh(meshId, QString::fromStdString(meshName), mesh);
                } else {
                    // Fallback: just add to scene manager directly
                    m_sceneManager->addMesh(meshId, QString::fromStdString(meshName), mesh);
                }
            }
            
            pos = idEnd;
        }
        
        // Update document state
        QFileInfo fileInfo(filePath);
        setLastUsedDirectory(fileInfo.absolutePath());
        m_currentFilePath = filePath;
        setModified(false);
        emit filePathChanged(m_currentFilePath);
        emit projectLoaded(filePath);
        
        qDebug() << "Project loaded successfully:" << filePath;
        return true;
        
    } catch (const std::exception& e) {
        QMessageBox::critical(m_mainWindow, tr("Open Failed"),
            tr("Failed to load project: %1").arg(e.what()));
        return false;
    }
}

} // namespace dc3d
