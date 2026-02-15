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
#include <glm/glm.hpp>

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
    
    // =========================================================================
    // Document Management (Save/Load)
    // =========================================================================
    
    /**
     * @brief Get the current project file path
     * @return Path to the current .dc3d file, or empty if untitled
     */
    QString currentFilePath() const { return m_currentFilePath; }
    
    /**
     * @brief Check if the document has unsaved changes
     */
    bool isModified() const { return m_isModified; }
    
    /**
     * @brief Set the modified state
     */
    void setModified(bool modified);
    
    /**
     * @brief Create a new empty project
     * @param askToSave If true, prompt to save current project if modified
     * @return true if new project was created (user didn't cancel)
     */
    bool newProject(bool askToSave = true);
    
    /**
     * @brief Save the current project
     * @return true on success, false if cancelled or failed
     * 
     * If the project has no file path, this calls saveProjectAs().
     */
    bool saveProject();
    
    /**
     * @brief Save the project to a new file
     * @param filePath Optional path. If empty, shows file dialog.
     * @return true on success
     */
    bool saveProjectAs(const QString& filePath = QString());
    
    /**
     * @brief Open a project file
     * @param filePath Path to the .dc3d file
     * @param askToSave If true, prompt to save current project if modified
     * @return true on success
     */
    bool openProject(const QString& filePath, bool askToSave = true);
    
    /**
     * @brief Get the last used directory for save/open dialogs
     */
    QString lastUsedDirectory() const { return m_lastUsedDirectory; }
    
    /**
     * @brief Set the last used directory
     */
    void setLastUsedDirectory(const QString& dir);

public slots:
    /**
     * @brief Reload preferences from settings
     * 
     * Called when PreferencesDialog is applied. Updates undo limit, etc.
     */
    void reloadPreferences();

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
    
    /**
     * @brief Emitted when modified state changes
     * @param modified New modified state
     */
    void modifiedChanged(bool modified);
    
    /**
     * @brief Emitted when current file path changes
     * @param filePath New file path (empty for untitled)
     */
    void filePathChanged(const QString& filePath);
    
    /**
     * @brief Emitted when a project is saved successfully
     * @param filePath Path to the saved file
     */
    void projectSaved(const QString& filePath);
    
    /**
     * @brief Emitted when a project is loaded successfully
     * @param filePath Path to the loaded file
     */
    void projectLoaded(const QString& filePath);

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
    
    // Document state
    QString m_currentFilePath;
    QString m_lastUsedDirectory;
    bool m_isModified = false;
    
    void loadBackupSettings();
    void saveBackupSettings();
    void cleanupOldBackups();
    
    // Document management helpers
    void loadDocumentSettings();
    void saveDocumentSettings();
    bool doSaveProject(const QString& filePath);
    bool doLoadProject(const QString& filePath);
};

} // namespace dc3d

#endif // DC3D_APP_APPLICATION_H
