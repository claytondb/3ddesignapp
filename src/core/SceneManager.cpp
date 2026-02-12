/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager and SceneNode classes
 */

#include "SceneManager.h"

namespace dc3d {
namespace core {

// ============================================================================
// SceneNode
// ============================================================================

SceneNode::SceneNode(const std::string& name)
    : m_name(name)
{
}

// ============================================================================
// SceneManager
// ============================================================================

SceneManager::SceneManager(QObject* parent)
    : QObject(parent)
{
}

SceneManager::~SceneManager() = default;

void SceneManager::clear()
{
    m_nodes.clear();
    emit sceneChanged();
}

void SceneManager::addNode(std::unique_ptr<SceneNode> node)
{
    m_nodes.push_back(std::move(node));
    emit sceneChanged();
}

} // namespace core
} // namespace dc3d
