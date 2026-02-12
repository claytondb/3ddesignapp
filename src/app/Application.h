/**
 * @file Application.h
 * @brief Main application class for dc-3ddesignapp
 * 
 * Manages application lifecycle, global services, and configuration.
 */

#ifndef DC3D_APP_APPLICATION_H
#define DC3D_APP_APPLICATION_H

#include <memory>

namespace dc3d {

// Forward declarations
namespace core {
class SceneManager;
}

/**
 * @class Application
 * @brief Singleton application manager
 * 
 * Owns and initializes all core services:
 * - SceneManager
 * - DocumentManager (future)
 * - SettingsManager (future)
 */
class Application
{
public:
    Application();
    ~Application();
    
    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
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

private:
    std::unique_ptr<core::SceneManager> m_sceneManager;
    bool m_initialized = false;
};

} // namespace dc3d

#endif // DC3D_APP_APPLICATION_H
