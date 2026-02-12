/**
 * @file SceneManager.cpp
 * @brief Implementation of SceneManager and SceneNode classes
 */

#include "SceneManager.h"
#include "geometry/MeshData.h"

#include <QDebug>

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
// MeshNode
// ============================================================================

MeshNode::MeshNode(uint64_t id, const QString& name, std::shared_ptr<geometry::MeshData> mesh)
    : SceneNode(name.toStdString())
    , m_id(id)
    , m_displayName(name)
    , m_mesh(mesh)
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
    m_meshNodes.clear();
    emit sceneChanged();
}

void SceneManager::addNode(std::unique_ptr<SceneNode> node)
{
    m_nodes.push_back(std::move(node));
    emit sceneChanged();
}

// ---- Mesh Management ----

void SceneManager::addMesh(uint64_t id, const QString& name, std::shared_ptr<geometry::MeshData> mesh)
{
    if (!mesh) {
        qWarning() << "SceneManager::addMesh - null mesh";
        return;
    }
    
    // Check if mesh already exists
    if (m_meshNodes.find(id) != m_meshNodes.end()) {
        qWarning() << "SceneManager::addMesh - mesh already exists with id" << id;
        return;
    }
    
    auto meshNode = std::make_unique<MeshNode>(id, name, mesh);
    m_meshNodes[id] = std::move(meshNode);
    
    qDebug() << "SceneManager::addMesh - Added mesh" << name 
             << "with id" << id
             << "(" << mesh->vertexCount() << "vertices,"
             << mesh->faceCount() << "faces)";
    
    emit meshAdded(id, name);
    emit sceneChanged();
}

void SceneManager::removeMesh(uint64_t id)
{
    auto it = m_meshNodes.find(id);
    if (it == m_meshNodes.end()) {
        qWarning() << "SceneManager::removeMesh - mesh not found with id" << id;
        return;
    }
    
    QString name = it->second->displayName();
    m_meshNodes.erase(it);
    
    qDebug() << "SceneManager::removeMesh - Removed mesh" << name << "with id" << id;
    
    emit meshRemoved(id);
    emit sceneChanged();
}

std::shared_ptr<geometry::MeshData> SceneManager::getMesh(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second->mesh();
    }
    return nullptr;
}

MeshNode* SceneManager::getMeshNode(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool SceneManager::hasMesh(uint64_t id) const
{
    return m_meshNodes.find(id) != m_meshNodes.end();
}

std::vector<uint64_t> SceneManager::meshIds() const
{
    std::vector<uint64_t> ids;
    ids.reserve(m_meshNodes.size());
    for (const auto& pair : m_meshNodes) {
        ids.push_back(pair.first);
    }
    return ids;
}

void SceneManager::setMeshVisible(uint64_t id, bool visible)
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        if (it->second->isVisible() != visible) {
            it->second->setVisible(visible);
            emit meshVisibilityChanged(id, visible);
            emit sceneChanged();
        }
    }
}

bool SceneManager::isMeshVisible(uint64_t id) const
{
    auto it = m_meshNodes.find(id);
    if (it != m_meshNodes.end()) {
        return it->second->isVisible();
    }
    return false;
}

} // namespace core
} // namespace dc3d
