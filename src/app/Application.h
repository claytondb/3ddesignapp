/**
 * @file Application.h
 * @brief Main application class for dc-3ddesignapp
 * 
 * Manages application lifecycle, global services, and configuration.
 */

#ifndef DC3D_APP_APPLICATION_H
#define DC3D_APP_APPLICATION_H

#include <QObject>
#include <memory>
#include <cstdint>

class MainWindow;
class QUndoStack;

namespace dc3d {

// Forward declarations
namespace core {
class SceneManager;
class Selection;
}

class IntegrationController;

namespace geometry {
class MeshData;
}

namespace renderer {
class Picking;
}

/**
 * @class Application
 * @brief Singleton application manager
 * 
 * Owns and initializes all core services:
 * - SceneManager
 * - UndoStack
 * - DocumentManager (future)
 * - SettingsManager (future)
 */
class Application : public QObject
{
    Q_OBJECT

public:
    Application(QObject* parent = nullptr);
    ~Application();
    
    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    /**
     * @brief Get singleton instance
     */
    static Application* instance();
    
    /**
     * @brief Initialize all application services
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown and cleanup all services
     */
    void shutdown();
    
    /**
     * @brief Get the scene manager instance
     */
    core::SceneManager* sceneManager() const { return m_sceneManager.get(); }
    
    /**
     * @brief Get the undo stack
     */
    QUndoStack* undoStack() const { return m_undoStack.get(); }
    
    /**
     * @brief Get the selection manager
     */
    core::Selection* selection() const { return m_selection.get(); }
    
    /**
     * @brief Get the picking system
     */
    renderer::Picking* picking() const { return m_picking.get(); }
    
    /**
     * @brief Get the integration controller
     */
    IntegrationController* integrationController() const { return m_integrationController.get(); }
    
    /**
     * @brief Get the main window
     */
    MainWindow* mainWindow() const { return m_mainWindow; }
    
    /**
     * @brief Set the main window (called by main())
     */
    void setMainWindow(MainWindow* window);
    
    /**
     * @brief Import a mesh file and add to scene
     * @param filePath Path to the mesh file
     * @return true on success
     */
    bool importMesh(const QString& filePath);
    
    /**
     * @brief Generate unique mesh ID
     */
    uint64_t generateMeshId();
    
    /**
     * @brief Create a primitive and add to scene (default dimensions)
     * @param type Type of primitive: "sphere", "cube", "cylinder", "cone", "plane", "torus"
     * @return true on success
     */
    bool createPrimitive(const QString& type);
    
    /**
     * @brief Create a primitive with custom dimensions and position
     * @param type Type of primitive
     * @param position Center position of the primitive
     * @param width Width/radius depending on type
     * @param height Height depending on type
     * @param depth Depth depending on type (cube only)
     * @param segments Number of segments for curved primitives
     * @param selectAfterCreation Whether to select the primitive after creation
     * @return true on success
     */
    bool createPrimitiveWithConfig(const QString& type, 
                                    const glm::vec3& position,
                                    float width, float height, float depth,
                                    int segments = 32,
                                    bool selectAfterCreation = true);
    
    /**
     * @brief Deselect all objects in the scene
     */
    void deselectAll();
    
    // =========================================================================
    // Auto-backup functionality
    // =========================================================================
    
    /**
     * @brief Enable or disable auto-backup before destructive operations
     */
    void setAutoBackupEnabled(bool enabled);
    bool isAutoBackupEnabled() const { return m_autoBackupEnabled; }
    
    /**
     * @brief Set the backup directory
     */
    void setBackupDirectory(const QString& dir);
    QString backupDirectory() const { return m_backupDirectory; }
    
    /**
     * @brief Create a backup of the current scene
     * @param reason Description of why backup was created
     * @return Path to backup file, or empty if failed
     */
    QString createBackup(const QString& reason = QString());
    
    /**
     * @brief Get list of recent backups
     */
    QStringList recentBackups() const;
    
    /**
     * @brief Restore from a backup file
     */
    bool restoreFromBackup(const QString& backupPath);

signals:
    /**
     * @brief Emitted when a mesh is imported successfully
     * @param name Display name of the mesh
     * @param id Unique mesh ID
     * @param vertexCount Number of vertices
     * @param faceCount Number of triangles
     * @param loadTimeMs Time taken to load in milliseconds
     */
    void meshImported(const QString& name, uint64_t id, 
                     size_t vertexCount, size_t faceCount, double loadTimeMs);
    
    /**
     * @brief Emitted when import fails
     * @param error Human-readable error message explaining what went wrong
     */
    void importFailed(const QString& error);

private:
    static Application* s_instance;
    
    std::unique_ptr<core::SceneManager> m_sceneManager;
    std::unique_ptr<core::Selection> m_selection;
    std::unique_ptr<renderer::Picking> m_picking;
    std::unique_ptr<IntegrationController> m_integrationController;
    std::unique_ptr<QUndoStack> m_undoStack;
    MainWindow* m_mainWindow = nullptr;
    bool m_initialized = false;
    uint64_t m_nextMeshId = 1;
    
    // Auto-backup settings
    bool m_autoBackupEnabled = true;
    QString m_backupDirectory;
    static const int MAX_BACKUPS = 10;
    
    void loadBackupSettings();
    void saveBackupSettings();
    void cleanupOldBackups();
};

} // namespace dc3d

#endif // DC3D_APP_APPLICATION_H
