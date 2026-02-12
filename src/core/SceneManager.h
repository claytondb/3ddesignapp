/**
 * @file SceneManager.h
 * @brief Scene graph management
 * 
 * Manages the hierarchical scene structure containing meshes,
 * CAD shapes, sketches, and annotations.
 */

#ifndef DC3D_CORE_SCENEMANAGER_H
#define DC3D_CORE_SCENEMANAGER_H

#include <QObject>
#include <memory>
#include <vector>
#include <string>

namespace dc3d {
namespace core {

/**
 * @class SceneNode
 * @brief Base class for all scene graph nodes
 */
class SceneNode
{
public:
    SceneNode(const std::string& name = "Node");
    virtual ~SceneNode() = default;
    
    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

private:
    std::string m_name;
    bool m_visible = true;
};

/**
 * @class SceneManager
 * @brief Manages the scene graph and provides access to scene nodes
 * 
 * Responsibilities:
 * - Owns the scene root node
 * - Provides node lookup by ID/name
 * - Emits signals when scene changes
 */
class SceneManager : public QObject
{
    Q_OBJECT

public:
    explicit SceneManager(QObject* parent = nullptr);
    ~SceneManager() override;
    
    /**
     * @brief Clear all nodes from the scene
     */
    void clear();
    
    /**
     * @brief Get the number of top-level nodes
     */
    size_t nodeCount() const { return m_nodes.size(); }
    
    /**
     * @brief Add a node to the scene
     * @param node The node to add (ownership transferred)
     */
    void addNode(std::unique_ptr<SceneNode> node);

signals:
    /**
     * @brief Emitted when the scene structure changes
     */
    void sceneChanged();

private:
    std::vector<std::unique_ptr<SceneNode>> m_nodes;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_SCENEMANAGER_H
