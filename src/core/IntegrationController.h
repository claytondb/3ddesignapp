/**
 * @file IntegrationController.h
 * @brief Central controller that wires together all application components
 * 
 * Connects:
 * - SceneManager → Viewport (mesh rendering)
 * - SceneManager → Picking (mesh selection)
 * - SceneManager → ObjectBrowser (UI tree)
 * - Viewport mouse events → Picking → Selection
 * - Selection → PropertiesPanel
 * - Selection → SelectionRenderer
 */

#pragma once

#include <QObject>
#include <memory>
#include <cstdint>

#include "Selection.h"  // Required for SelectionMode enum

class MainWindow;
class ObjectBrowser;
class PropertiesPanel;

namespace dc {
class Viewport;
}

namespace dc3d {
namespace core {
class SceneManager;
class Selection;
}
namespace renderer {
class Picking;
class SelectionRenderer;
}
namespace geometry {
class MeshData;
}

/**
 * @brief Central integration controller for the application
 * 
 * This class acts as the glue between all major components,
 * connecting signals and managing data flow between:
 * - Scene management
 * - 3D rendering
 * - Selection system
 * - UI panels
 */
class IntegrationController : public QObject {
    Q_OBJECT

public:
    explicit IntegrationController(QObject* parent = nullptr);
    ~IntegrationController() override;
    
    /**
     * @brief Initialize with application components
     * @param sceneManager Scene graph manager
     * @param viewport 3D viewport widget
     * @param selection Selection manager
     * @param picking Picking system
     * @param mainWindow Main application window
     */
    void initialize(
        core::SceneManager* sceneManager,
        dc::Viewport* viewport,
        core::Selection* selection,
        renderer::Picking* picking,
        MainWindow* mainWindow
    );
    
    /**
     * @brief Check if controller is initialized
     */
    bool isInitialized() const { return m_initialized; }
    
    // ---- Component Access ----
    
    core::SceneManager* sceneManager() const { return m_sceneManager; }
    dc::Viewport* viewport() const { return m_viewport; }
    core::Selection* selection() const { return m_selection; }
    renderer::Picking* picking() const { return m_picking; }
    
    // ---- Operations ----
    
    /**
     * @brief Add mesh to scene with full integration
     * 
     * This adds the mesh to:
     * - SceneManager (data ownership)
     * - Viewport (rendering)
     * - Picking system (selection)
     * - ObjectBrowser (UI)
     */
    void addMesh(uint64_t id, const QString& name, 
                 std::shared_ptr<geometry::MeshData> mesh);
    
    /**
     * @brief Remove mesh from all systems
     */
    void removeMesh(uint64_t id);
    
    /**
     * @brief Clear all meshes from scene
     */
    void clearScene();
    
    /**
     * @brief Delete selected objects
     */
    void deleteSelected();
    
    /**
     * @brief Select mesh by ID
     */
    void selectMesh(uint64_t id);
    
    /**
     * @brief Deselect all
     */
    void deselectAll();
    
    /**
     * @brief Focus view on selection
     */
    void focusOnSelection();
    
    /**
     * @brief Focus view on mesh
     */
    void focusOnMesh(uint64_t id);
    
    // ---- Selection Mode ----
    
    /**
     * @brief Set selection mode (object/face/vertex/edge)
     */
    void setSelectionMode(core::SelectionMode mode);
    
signals:
    /**
     * @brief Emitted when integration is ready
     */
    void ready();
    
    /**
     * @brief Emitted when scene changes
     */
    void sceneChanged();
    
    /**
     * @brief Emitted when selection changes
     */
    void selectionChanged();

private slots:
    // Scene manager signals
    void onMeshAdded(uint64_t id, const QString& name);
    void onMeshRemoved(uint64_t id);
    void onMeshVisibilityChanged(uint64_t id, bool visible);
    
    // Selection signals
    void onSelectionChanged();
    void onSelectionModeChanged(core::SelectionMode mode);
    
    // Object browser signals
    void onObjectBrowserItemSelected(const QString& id);
    void onObjectBrowserItemDoubleClicked(const QString& id);
    void onObjectBrowserVisibilityToggled(const QString& id, bool visible);
    
    // Viewport signals
    void onViewportSelectionClick(const QPoint& pos, bool addToSelection, bool toggleSelection);
    void onViewportBoxSelection(const QRect& rect, bool addToSelection);
    
private:
    void connectSceneManager();
    void connectSelection();
    void connectObjectBrowser();
    void connectPropertiesPanel();
    void connectViewport();
    
    void updatePropertiesForSelection();
    void updateStatusBarForSelection();
    
    // Components (non-owning pointers)
    core::SceneManager* m_sceneManager = nullptr;
    dc::Viewport* m_viewport = nullptr;
    core::Selection* m_selection = nullptr;
    renderer::Picking* m_picking = nullptr;
    MainWindow* m_mainWindow = nullptr;
    ObjectBrowser* m_objectBrowser = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    
    bool m_initialized = false;
};

} // namespace dc3d
