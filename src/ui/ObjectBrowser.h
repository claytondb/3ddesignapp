#ifndef DC_OBJECTBROWSER_H
#define DC_OBJECTBROWSER_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QMenu>

/**
 * @brief Object Browser panel showing scene hierarchy
 * 
 * Displays the scene contents organized by type:
 * - Meshes (imported scans, reduced versions)
 * - Primitives (planes, cylinders, spheres)
 * - Sketches (2D/3D sketches)
 * - Surfaces (fitted surfaces, extruded, etc.)
 * - Bodies (solid bodies)
 */
class ObjectBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectBrowser(QWidget *parent = nullptr);
    ~ObjectBrowser() override = default;

    // Add items to sections
    void addMesh(const QString& name, const QString& id);
    void addPrimitive(const QString& name, const QString& id, const QString& type);
    void addSketch(const QString& name, const QString& id);
    void addSurface(const QString& name, const QString& id);
    void addBody(const QString& name, const QString& id);

    // Remove items
    void removeItem(const QString& id);
    void removeMesh(const QString& id) { removeItem(id); }
    void clear();

    // Item state
    void setItemVisible(const QString& id, bool visible);
    void setMeshVisible(const QString& id, bool visible) { setItemVisible(id, visible); }
    void setItemLocked(const QString& id, bool locked);
    void setItemSelected(const QString& id, bool selected);
    
    // Batch selection
    void setSelectedItems(const QStringList& ids);
    
    // Get selected items
    QStringList selectedItemIds() const;

signals:
    // Selection changed
    void itemSelected(const QString& id);
    void itemDoubleClicked(const QString& id);
    void selectionChanged(const QStringList& ids);

    // Context menu actions
    void hideItemRequested(const QString& id);
    void showItemRequested(const QString& id);
    void deleteItemRequested(const QString& id);
    void isolateItemRequested(const QString& id);
    void exportItemRequested(const QString& id);
    void renameItemRequested(const QString& id);
    
    // Rename completed
    void itemRenamed(const QString& id, const QString& newName);

    // Visibility toggle
    void visibilityToggled(const QString& id, bool visible);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onSelectionChanged();
    void showContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupSections();
    void setupContextMenu();

    QTreeWidgetItem* findItemById(const QString& id) const;
    QTreeWidgetItem* createObjectItem(const QString& name, const QString& id, 
                                       const QString& icon);

    // UI Components
    QVBoxLayout* m_layout;
    QTreeWidget* m_treeWidget;
    QMenu* m_contextMenu;

    // Section items (categories)
    QTreeWidgetItem* m_meshesSection;
    QTreeWidgetItem* m_primitivesSection;
    QTreeWidgetItem* m_sketchesSection;
    QTreeWidgetItem* m_surfacesSection;
    QTreeWidgetItem* m_bodiesSection;

    // ID to item mapping
    QMap<QString, QTreeWidgetItem*> m_itemMap;

    // Context menu actions
    QAction* m_actionHide;
    QAction* m_actionShow;
    QAction* m_actionDelete;
    QAction* m_actionIsolate;
    QAction* m_actionExport;
    QAction* m_actionRename;
};

#endif // DC_OBJECTBROWSER_H
