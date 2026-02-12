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
}

namespace geometry {
class MeshData;
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

signals:
    /**
     * @brief Emitted when a mesh is imported successfully
     */
    void meshImported(const QString& name, uint64_t id);
    
    /**
     * @brief Emitted when import fails
     */
    void importFailed(const QString& error);

private:
    static Application* s_instance;
    
    std::unique_ptr<core::SceneManager> m_sceneManager;
    std::unique_ptr<QUndoStack> m_undoStack;
    MainWindow* m_mainWindow = nullptr;
    bool m_initialized = false;
    uint64_t m_nextMeshId = 1;
};

} // namespace dc3d

#endif // DC3D_APP_APPLICATION_H
