/**
 * @file Application.cpp
 * @brief Implementation of the Application class
 */

#include "Application.h"
#include "core/SceneManager.h"

namespace dc3d {

Application::Application() = default;

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    // Initialize scene manager
    m_sceneManager = std::make_unique<core::SceneManager>();
    
    m_initialized = true;
    return true;
}

void Application::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    // Cleanup in reverse order of initialization
    m_sceneManager.reset();
    
    m_initialized = false;
}

} // namespace dc3d
