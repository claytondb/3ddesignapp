/**
 * @file IntegrationController.cpp
 * @brief Implementation of the central integration controller
 * 
 * Thread Safety Note: All IntegrationController methods must be called from
 * the main (UI) thread. Qt signal/slot connections ensure this when using
 * Qt::AutoConnection (the default) for cross-thread signals.
 */

#include "IntegrationController.h"
#include "SceneManager.h"
#include "Selection.h"
#include "../renderer/Picking.h"
#include "../renderer/Viewport.h"
#include "../geometry/MeshData.h"
#include "../ui/MainWindow.h"
#include "../ui/ObjectBrowser.h"
#include "../ui/PropertiesPanel.h"
#include "../ui/StatusBar.h"

#include <QDebug>

namespace dc3d {

IntegrationController::IntegrationController(QObject* parent)
    : QObject(parent)
{
}

IntegrationController::~IntegrationController() = default;

void IntegrationController::initialize(
    core::SceneManager* sceneManager,
    dc::Viewport* viewport,
    core::Selection* selection,
    renderer::Picking* picking,
    MainWindow* mainWindow)
{
    if (m_initialized) {
        qWarning() << "IntegrationController already initialized";
        return;
    }
    
    m_sceneManager = sceneManager;
    m_viewport = viewport;
    m_selection = selection;
    m_picking = picking;
    m_mainWindow = mainWindow;
    
    if (mainWindow) {
        m_objectBrowser = mainWindow->objectBrowser();
        m_propertiesPanel = mainWindow->propertiesPanel();
    }
    
    // Connect all components
    connectSceneManager();
    connectSelection();
    connectObjectBrowser();
    connectPropertiesPanel();
    connectViewport();
    
    m_initialized = true;
    qDebug() << "IntegrationController initialized";
    
    emit ready();
}

void IntegrationController::connectSceneManager()
{
    if (!m_sceneManager) return;
    
    connect(m_sceneManager, &core::SceneManager::meshAdded,
            this, &IntegrationController::onMeshAdded);
    connect(m_sceneManager, &core::SceneManager::meshRemoved,
            this, &IntegrationController::onMeshRemoved);
    connect(m_sceneManager, &core::SceneManager::meshVisibilityChanged,
            this, &IntegrationController::onMeshVisibilityChanged);
    connect(m_sceneManager, &core::SceneManager::sceneChanged,
            this, &IntegrationController::sceneChanged);
}

void IntegrationController::connectSelection()
{
    if (!m_selection) return;
    
    connect(m_selection, &core::Selection::selectionChanged,
            this, &IntegrationController::onSelectionChanged);
    connect(m_selection, &core::Selection::modeChanged,
            this, &IntegrationController::onSelectionModeChanged);
}

void IntegrationController::connectObjectBrowser()
{
    if (!m_objectBrowser) return;
    
    connect(m_objectBrowser, &ObjectBrowser::itemSelected,
            this, &IntegrationController::onObjectBrowserItemSelected);
    connect(m_objectBrowser, &ObjectBrowser::itemDoubleClicked,
            this, &IntegrationController::onObjectBrowserItemDoubleClicked);
    connect(m_objectBrowser, &ObjectBrowser::visibilityToggled,
            this, &IntegrationController::onObjectBrowserVisibilityToggled);
}

void IntegrationController::connectPropertiesPanel()
{
    // Properties panel is mostly a display - no outgoing connections needed
}

void IntegrationController::connectViewport()
{
    if (!m_viewport) return;
    
    // Set selection manager on viewport for rendering highlights
    m_viewport->setSelection(m_selection);
    
    connect(m_viewport, &dc::Viewport::selectionClick,
            this, &IntegrationController::onViewportSelectionClick);
    connect(m_viewport, &dc::Viewport::boxSelectionComplete,
            this, &IntegrationController::onViewportBoxSelection);
}

// ===================
// Slot Implementations
// ===================

void IntegrationController::onMeshAdded(uint64_t id, const QString& name)
{
    // Guard against partially initialized state
    if (!m_initialized) return;
    
    qDebug() << "IntegrationController: mesh added" << name << "id:" << id;
    
    // Get mesh data from scene manager
    if (!m_sceneManager) return;
    
    auto mesh = m_sceneManager->getMesh(id);
    if (!mesh) {
        qWarning() << "Mesh not found in scene manager:" << id;
        return;
    }
    
    // Add to viewport for rendering
    if (m_viewport) {
        m_viewport->addMesh(id, mesh);
    }
    
    // Add to picking system
    if (m_picking) {
        m_picking->addMesh(static_cast<uint32_t>(id), mesh.get());
    }
    
    // Add to object browser
    if (m_objectBrowser) {
        m_objectBrowser->addMesh(name, QString::number(id));
    }
    
    // Update viewport
    if (m_viewport) {
        m_viewport->update();
    }
}

void IntegrationController::onMeshRemoved(uint64_t id)
{
    // Guard against partially initialized state
    if (!m_initialized) return;
    
    qDebug() << "IntegrationController: mesh removed id:" << id;
    
    // Remove from viewport
    if (m_viewport) {
        m_viewport->removeMesh(id);
    }
    
    // Remove from picking system
    if (m_picking) {
        m_picking->removeMesh(static_cast<uint32_t>(id));
    }
    
    // Remove from object browser
    if (m_objectBrowser) {
        m_objectBrowser->removeMesh(QString::number(id));
    }
    
    // Clear from selection
    if (m_selection) {
        m_selection->deselectObject(static_cast<uint32_t>(id));
    }
    
    // Update viewport
    if (m_viewport) {
        m_viewport->update();
    }
}

void IntegrationController::onMeshVisibilityChanged(uint64_t id, bool visible)
{
    // Guard against partially initialized state
    if (!m_initialized) return;
    
    qDebug() << "IntegrationController: visibility changed" << id << visible;
    
    // Update picking system
    if (m_picking) {
        m_picking->setMeshVisible(static_cast<uint32_t>(id), visible);
    }
    
    // Update object browser
    if (m_objectBrowser) {
        m_objectBrowser->setMeshVisible(QString::number(id), visible);
    }
    
    // Update viewport
    if (m_viewport) {
        m_viewport->update();
    }
}

void IntegrationController::onSelectionChanged()
{
    // Guard against partially initialized state
    if (!m_initialized) return;
    
    qDebug() << "IntegrationController: selection changed";
    
    // Update properties panel
    updatePropertiesForSelection();
    
    // Update status bar
    updateStatusBarForSelection();
    
    // Update object browser selection highlighting
    if (m_objectBrowser && m_selection) {
        auto meshIds = m_selection->selectedMeshIds();
        QStringList idStrings;
        for (uint32_t id : meshIds) {
            idStrings << QString::number(id);
        }
        m_objectBrowser->setSelectedItems(idStrings);
    }
    
    // Request viewport repaint for selection highlighting
    if (m_viewport) {
        m_viewport->update();
    }
    
    emit selectionChanged();
}

void IntegrationController::onSelectionModeChanged(core::SelectionMode mode)
{
    // Guard against partially initialized state
    if (!m_initialized) return;
    
    qDebug() << "IntegrationController: selection mode changed to" << static_cast<int>(mode);
    
    // Update status bar mode indicator
    if (m_mainWindow) {
        QString modeStr;
        switch (mode) {
            case core::SelectionMode::Object: modeStr = "Object"; break;
            case core::SelectionMode::Face: modeStr = "Face"; break;
            case core::SelectionMode::Vertex: modeStr = "Vertex"; break;
            case core::SelectionMode::Edge: modeStr = "Edge"; break;
        }
        m_mainWindow->setStatusMessage("Selection mode: " + modeStr);
    }
}

void IntegrationController::onObjectBrowserItemSelected(const QString& id)
{
    if (!m_initialized || !m_selection) return;
    
    bool ok;
    uint64_t meshId = id.toULongLong(&ok);
    if (!ok) return;
    
    // Select the mesh
    m_selection->selectObject(static_cast<uint32_t>(meshId), core::SelectionOp::Replace);
}

void IntegrationController::onObjectBrowserItemDoubleClicked(const QString& id)
{
    if (!m_initialized) return;
    
    bool ok;
    uint64_t meshId = id.toULongLong(&ok);
    if (!ok) return;
    
    // Focus view on the mesh
    focusOnMesh(meshId);
}

void IntegrationController::onObjectBrowserVisibilityToggled(const QString& id, bool visible)
{
    if (!m_initialized || !m_sceneManager) return;
    
    bool ok;
    uint64_t meshId = id.toULongLong(&ok);
    if (!ok) return;
    
    m_sceneManager->setMeshVisible(meshId, visible);
}

// ===================
// Operations
// ===================

void IntegrationController::addMesh(uint64_t id, const QString& name,
                                    std::shared_ptr<geometry::MeshData> mesh)
{
    if (!m_sceneManager) {
        qWarning() << "Cannot add mesh: SceneManager not initialized";
        return;
    }
    
    // Add to scene manager - this will trigger onMeshAdded
    m_sceneManager->addMesh(id, name, mesh);
}

void IntegrationController::removeMesh(uint64_t id)
{
    if (!m_sceneManager) return;
    
    // Remove from scene manager - this will trigger onMeshRemoved
    m_sceneManager->removeMesh(id);
}

void IntegrationController::clearScene()
{
    if (!m_sceneManager) return;
    
    // Clear scene manager (will emit meshRemoved signals)
    m_sceneManager->clear();
    
    // Clear viewport
    if (m_viewport) {
        m_viewport->clearMeshes();
    }
    
    // Clear picking
    if (m_picking) {
        m_picking->clear();
    }
    
    // Clear selection
    if (m_selection) {
        m_selection->clear();
    }
    
    // Clear object browser
    if (m_objectBrowser) {
        m_objectBrowser->clear();
    }
    
    emit sceneChanged();
}

void IntegrationController::deleteSelected()
{
    if (!m_selection || !m_sceneManager) return;
    
    auto meshIds = m_selection->selectedMeshIds();
    
    // Clear selection first
    m_selection->clear();
    
    // Remove each selected mesh
    for (uint32_t id : meshIds) {
        m_sceneManager->removeMesh(id);
    }
}

void IntegrationController::selectMesh(uint64_t id)
{
    if (!m_selection) return;
    m_selection->selectObject(static_cast<uint32_t>(id), core::SelectionOp::Replace);
}

void IntegrationController::deselectAll()
{
    if (!m_selection) return;
    m_selection->clear();
}

void IntegrationController::focusOnSelection()
{
    if (!m_selection || !m_viewport) return;
    
    auto meshIds = m_selection->selectedMeshIds();
    if (meshIds.empty()) {
        // Fit to all
        m_viewport->fitView();
        return;
    }
    
    // TODO: Compute bounding box of selected meshes and fit to that
    // For now, fit to first selected mesh
    focusOnMesh(meshIds[0]);
}

void IntegrationController::focusOnMesh(uint64_t id)
{
    if (!m_sceneManager || !m_viewport) return;
    
    auto mesh = m_sceneManager->getMesh(id);
    if (!mesh) return;
    
    // Fit view to mesh bounds
    // The viewport should compute bounds from mesh data
    m_viewport->fitView();
}

void IntegrationController::setSelectionMode(core::SelectionMode mode)
{
    if (!m_selection) return;
    m_selection->setMode(mode);
}

// ===================
// Private Helpers
// ===================

void IntegrationController::updatePropertiesForSelection()
{
    if (!m_propertiesPanel || !m_selection || !m_sceneManager) return;
    
    auto meshIds = m_selection->selectedMeshIds();
    
    if (meshIds.empty()) {
        m_propertiesPanel->clearProperties();
        return;
    }
    
    if (meshIds.size() == 1) {
        // Single selection - show mesh properties
        uint64_t id = meshIds[0];
        auto mesh = m_sceneManager->getMesh(id);
        auto node = m_sceneManager->getMeshNode(id);
        
        if (mesh && node) {
            QVariantMap props;
            props["Name"] = node->displayName();
            props["Vertices"] = QString::number(mesh->vertexCount());
            props["Triangles"] = QString::number(mesh->faceCount());
            props["Has Normals"] = mesh->hasNormals() ? "Yes" : "No";
            
            auto bounds = mesh->bounds();
            props["Bounds Min"] = QString("(%1, %2, %3)")
                .arg(bounds.min.x, 0, 'f', 3)
                .arg(bounds.min.y, 0, 'f', 3)
                .arg(bounds.min.z, 0, 'f', 3);
            props["Bounds Max"] = QString("(%1, %2, %3)")
                .arg(bounds.max.x, 0, 'f', 3)
                .arg(bounds.max.y, 0, 'f', 3)
                .arg(bounds.max.z, 0, 'f', 3);
            
            m_propertiesPanel->setProperties(props);
        }
    } else {
        // Multiple selection
        QVariantMap props;
        props["Selected Objects"] = QString::number(meshIds.size());
        
        size_t totalVerts = 0;
        size_t totalFaces = 0;
        for (uint32_t id : meshIds) {
            auto mesh = m_sceneManager->getMesh(id);
            if (mesh) {
                totalVerts += mesh->vertexCount();
                totalFaces += mesh->faceCount();
            }
        }
        props["Total Vertices"] = QString::number(totalVerts);
        props["Total Triangles"] = QString::number(totalFaces);
        
        m_propertiesPanel->setProperties(props);
    }
}

void IntegrationController::updateStatusBarForSelection()
{
    if (!m_mainWindow || !m_selection) return;
    
    auto meshIds = m_selection->selectedMeshIds();
    
    if (meshIds.empty()) {
        m_mainWindow->setSelectionInfo("Nothing selected");
    } else if (meshIds.size() == 1) {
        if (m_sceneManager) {
            auto node = m_sceneManager->getMeshNode(meshIds[0]);
            if (node) {
                m_mainWindow->setSelectionInfo("Selected: " + node->displayName());
            }
        }
    } else {
        m_mainWindow->setSelectionInfo(QString("Selected: %1 objects").arg(meshIds.size()));
    }
}

void IntegrationController::onViewportSelectionClick(const QPoint& pos, bool addToSelection, bool toggleSelection)
{
    // Guard against partially initialized state
    if (!m_initialized) return;
    if (!m_picking || !m_selection || !m_viewport) return;
    
    // Perform pick at click position
    core::HitInfo hit = m_picking->pick(pos, m_viewport->size(), m_viewport->camera());
    
    if (!hit.hit) {
        // Clicked on nothing - deselect unless adding to selection
        if (!addToSelection && !toggleSelection) {
            m_selection->clear();
        }
        return;
    }
    
    // Determine selection operation
    core::SelectionOp op = core::SelectionOp::Replace;
    if (addToSelection) {
        op = core::SelectionOp::Add;
    } else if (toggleSelection) {
        op = core::SelectionOp::Toggle;
    }
    
    // Select based on current mode
    m_selection->selectFromHit(hit, op);
    
    qDebug() << "Pick hit mesh" << hit.meshId << "face" << hit.faceIndex;
}

void IntegrationController::onViewportBoxSelection(const QRect& rect, bool addToSelection)
{
    // Guard against partially initialized state
    if (!m_initialized) return;
    if (!m_picking || !m_selection || !m_viewport) return;
    
    // Perform box selection
    auto elements = m_picking->boxSelect(rect, m_viewport->size(), 
                                         m_viewport->camera(), m_selection->mode());
    
    if (elements.empty()) {
        if (!addToSelection) {
            m_selection->clear();
        }
        return;
    }
    
    // Select all elements in the box
    core::SelectionOp op = addToSelection ? core::SelectionOp::Add : core::SelectionOp::Replace;
    m_selection->select(elements, op);
    
    qDebug() << "Box selection found" << elements.size() << "elements";
}

} // namespace dc3d
