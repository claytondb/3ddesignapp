#ifndef DC_OBJECTBROWSER_H
#define DC_OBJECTBROWSER_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QShortcut>
#include <QMimeData>

/**
 * @brief Object Browser panel showing scene hierarchy
 * 
 * Displays the scene contents organized by type:
 * - Meshes (imported scans, reduced versions)
 * - Primitives (planes, cylinders, spheres)
 * - Sketches (2D/3D sketches)
 * - Surfaces (fitted surfaces, extruded, etc.)
 * - Bodies (solid bodies)
 * 
 * Features:
 * - Object groups with expand/collapse
 * - Visibility and lock toggles
 * - Double-click to rename
 * - Drag and drop reordering
 * - Search/filter bar
 * - Keyboard shortcuts (H, Alt+H, L, Ctrl+G)
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
    void setItemName(const QString& id, const QString& name);
    
    // Batch selection
    void setSelectedItems(const QStringList& ids);
    
    // Get selected items
    QStringList selectedItemIds() const;
    
    // Group management
    void addGroup(const QString& name, const QString& groupId);
    void removeGroup(const QString& groupId);
    void addItemToGroup(const QString& itemId, const QString& groupId);
    void removeItemFromGroup(const QString& itemId);
    void setGroupExpanded(const QString& groupId, bool expanded);
    void setGroupVisible(const QString& groupId, bool visible);
    void setGroupLocked(const QString& groupId, bool locked);
    void setGroupName(const QString& groupId, const QString& name);
    
    // Filter/search
    void setFilterText(const QString& text);
    QString filterText() const;

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
    
    // Lock toggle
    void lockToggled(const QString& id, bool locked);
    
    // Group operations
    void groupRequested(const QStringList& ids);
    void ungroupRequested(const QString& groupId);
    void groupRenamed(const QString& groupId, const QString& newName);
    void groupVisibilityToggled(const QString& groupId, bool visible);
    void groupLockToggled(const QString& groupId, bool locked);
    void groupExpandedChanged(const QString& groupId, bool expanded);
    
    // Reorder
    void itemMovedBefore(const QString& itemId, const QString& beforeId);
    
    // Hide/show shortcuts
    void hideSelectedRequested();
    void unhideAllRequested();
    void toggleLockSelectedRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onSelectionChanged();
    void showContextMenu(const QPoint& pos);
    void onFilterTextChanged(const QString& text);
    void onGroupShortcut();
    void onHideShortcut();
    void onUnhideAllShortcut();
    void onToggleLockShortcut();

private:
    void setupUI();
    void setupSections();
    void setupContextMenu();
    void setupShortcuts();
    void setupDragDrop();

    QTreeWidgetItem* findItemById(const QString& id) const;
    QTreeWidgetItem* findGroupById(const QString& groupId) const;
    QTreeWidgetItem* createObjectItem(const QString& name, const QString& id, 
                                       const QString& icon);
    QTreeWidgetItem* createGroupItem(const QString& name, const QString& id);
    
    void applyFilter();
    bool itemMatchesFilter(QTreeWidgetItem* item) const;
    void updateItemVisuals(QTreeWidgetItem* item);
    
    // Get icon based on visibility/lock state
    QString getVisibilityIcon(bool visible) const;
    QString getLockIcon(bool locked) const;

    // UI Components
    QVBoxLayout* m_layout;
    QLineEdit* m_searchBox;
    QTreeWidget* m_treeWidget;
    QMenu* m_contextMenu;
    QMenu* m_groupContextMenu;

    // Section items (categories)
    QTreeWidgetItem* m_meshesSection;
    QTreeWidgetItem* m_primitivesSection;
    QTreeWidgetItem* m_sketchesSection;
    QTreeWidgetItem* m_surfacesSection;
    QTreeWidgetItem* m_bodiesSection;

    // ID to item mapping
    QMap<QString, QTreeWidgetItem*> m_itemMap;
    QMap<QString, QTreeWidgetItem*> m_groupMap;
    
    // Current filter text
    QString m_filterText;
    
    // Editing state
    bool m_isEditing = false;

    // Context menu actions
    QAction* m_actionHide;
    QAction* m_actionShow;
    QAction* m_actionDelete;
    QAction* m_actionIsolate;
    QAction* m_actionExport;
    QAction* m_actionRename;
    QAction* m_actionGroup;
    QAction* m_actionUngroup;
    QAction* m_actionLock;
    QAction* m_actionUnlock;
    
    // Shortcuts
    QShortcut* m_shortcutGroup;
    QShortcut* m_shortcutHide;
    QShortcut* m_shortcutUnhideAll;
    QShortcut* m_shortcutToggleLock;
    
    // Column indices
    static constexpr int COL_NAME = 0;
    static constexpr int COL_VISIBILITY = 1;
    static constexpr int COL_LOCK = 2;
    
    // Data roles
    static constexpr int ROLE_TYPE = Qt::UserRole;          // "section", "object", "group"
    static constexpr int ROLE_ID = Qt::UserRole + 1;        // Object/group ID
    static constexpr int ROLE_VISIBLE = Qt::UserRole + 2;   // Visibility state
    static constexpr int ROLE_LOCKED = Qt::UserRole + 3;    // Locked state
    static constexpr int ROLE_GROUP_ID = Qt::UserRole + 4;  // Parent group ID (for objects)
    static constexpr int ROLE_OLD_NAME = Qt::UserRole + 10; // For rename tracking
};

#endif // DC_OBJECTBROWSER_H
