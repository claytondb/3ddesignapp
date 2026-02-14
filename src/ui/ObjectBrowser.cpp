#include "ObjectBrowser.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QDrag>
#include <QApplication>
#include <QDebug>

ObjectBrowser::ObjectBrowser(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ObjectBrowser");
    setupUI();
    setupSections();
    setupContextMenu();
    setupShortcuts();
    setupDragDrop();
}

void ObjectBrowser::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);

    // Search/filter box
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText(tr("Search objects..."));
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setObjectName("ObjectBrowserSearch");
    connect(m_searchBox, &QLineEdit::textChanged, 
            this, &ObjectBrowser::onFilterTextChanged);
    m_layout->addWidget(m_searchBox);

    // Create tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("ObjectBrowserTree");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(16);
    m_treeWidget->setAnimated(true);
    m_treeWidget->setExpandsOnDoubleClick(false);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Enable drag and drop
    m_treeWidget->setDragEnabled(true);
    m_treeWidget->setAcceptDrops(true);
    m_treeWidget->setDropIndicatorShown(true);
    m_treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    
    // Setup columns (name, visibility icon, lock icon)
    m_treeWidget->setColumnCount(3);
    m_treeWidget->header()->setStretchLastSection(false);
    m_treeWidget->header()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(COL_VISIBILITY, QHeaderView::Fixed);
    m_treeWidget->header()->setSectionResizeMode(COL_LOCK, QHeaderView::Fixed);
    m_treeWidget->header()->resizeSection(COL_VISIBILITY, 24);
    m_treeWidget->header()->resizeSection(COL_LOCK, 24);

    // Connect signals
    connect(m_treeWidget, &QTreeWidget::itemClicked, 
            this, &ObjectBrowser::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, 
            this, &ObjectBrowser::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::itemChanged,
            this, &ObjectBrowser::onItemChanged);
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, 
            this, &ObjectBrowser::onSelectionChanged);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, 
            this, &ObjectBrowser::showContextMenu);

    m_layout->addWidget(m_treeWidget);
    
    // Install event filter for keyboard handling
    m_treeWidget->installEventFilter(this);
}

void ObjectBrowser::setupSections()
{
    // Create section headers with icons
    auto createSection = [this](const QString& name, const QString& icon) -> QTreeWidgetItem* {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(COL_NAME, name);
        item->setData(COL_NAME, ROLE_TYPE, "section");
        item->setExpanded(true);
        
        // Style section headers
        QFont font = item->font(COL_NAME);
        font.setBold(true);
        item->setFont(COL_NAME, font);
        
        // Section items cannot be selected or dragged
        item->setFlags(item->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled));
        
        return item;
    };

    m_meshesSection = createSection(tr("Meshes"), "📦");
    m_primitivesSection = createSection(tr("Primitives"), "◼");
    m_sketchesSection = createSection(tr("Sketches"), "✎");
    m_surfacesSection = createSection(tr("Surfaces"), "◇");
    m_bodiesSection = createSection(tr("Bodies"), "⬡");
}

void ObjectBrowser::setupContextMenu()
{
    // Object context menu
    m_contextMenu = new QMenu(this);

    m_actionShow = m_contextMenu->addAction(tr("Show"));
    connect(m_actionShow, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString type = item->data(COL_NAME, ROLE_TYPE).toString();
            QString id = item->data(COL_NAME, ROLE_ID).toString();
            if (type == "object" && !id.isEmpty()) {
                setItemVisible(id, true);
                emit showItemRequested(id);
            } else if (type == "group" && !id.isEmpty()) {
                setGroupVisible(id, true);
                emit groupVisibilityToggled(id, true);
            }
        }
    });

    m_actionHide = m_contextMenu->addAction(tr("Hide"));
    m_actionHide->setShortcut(QKeySequence("H"));
    connect(m_actionHide, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString type = item->data(COL_NAME, ROLE_TYPE).toString();
            QString id = item->data(COL_NAME, ROLE_ID).toString();
            if (type == "object" && !id.isEmpty()) {
                setItemVisible(id, false);
                emit hideItemRequested(id);
            } else if (type == "group" && !id.isEmpty()) {
                setGroupVisible(id, false);
                emit groupVisibilityToggled(id, false);
            }
        }
    });

    m_contextMenu->addSeparator();

    m_actionLock = m_contextMenu->addAction(tr("Lock"));
    connect(m_actionLock, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString type = item->data(COL_NAME, ROLE_TYPE).toString();
            QString id = item->data(COL_NAME, ROLE_ID).toString();
            if (type == "object" && !id.isEmpty()) {
                setItemLocked(id, true);
                emit lockToggled(id, true);
            } else if (type == "group" && !id.isEmpty()) {
                setGroupLocked(id, true);
                emit groupLockToggled(id, true);
            }
        }
    });

    m_actionUnlock = m_contextMenu->addAction(tr("Unlock"));
    connect(m_actionUnlock, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString type = item->data(COL_NAME, ROLE_TYPE).toString();
            QString id = item->data(COL_NAME, ROLE_ID).toString();
            if (type == "object" && !id.isEmpty()) {
                setItemLocked(id, false);
                emit lockToggled(id, false);
            } else if (type == "group" && !id.isEmpty()) {
                setGroupLocked(id, false);
                emit groupLockToggled(id, false);
            }
        }
    });

    m_contextMenu->addSeparator();

    m_actionIsolate = m_contextMenu->addAction(tr("Isolate"));
    connect(m_actionIsolate, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        if (!items.isEmpty()) {
            QString id = items.first()->data(COL_NAME, ROLE_ID).toString();
            if (!id.isEmpty()) {
                emit isolateItemRequested(id);
            }
        }
    });

    m_contextMenu->addSeparator();

    m_actionGroup = m_contextMenu->addAction(tr("Group"));
    m_actionGroup->setShortcut(QKeySequence("Ctrl+G"));
    connect(m_actionGroup, &QAction::triggered, this, &ObjectBrowser::onGroupShortcut);

    m_actionUngroup = m_contextMenu->addAction(tr("Ungroup"));
    connect(m_actionUngroup, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString type = item->data(COL_NAME, ROLE_TYPE).toString();
            if (type == "group") {
                QString id = item->data(COL_NAME, ROLE_ID).toString();
                emit ungroupRequested(id);
            }
        }
    });

    m_contextMenu->addSeparator();

    m_actionRename = m_contextMenu->addAction(tr("Rename"));
    m_actionRename->setShortcut(QKeySequence("F2"));
    connect(m_actionRename, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        if (!items.isEmpty()) {
            m_treeWidget->editItem(items.first(), COL_NAME);
        }
    });

    m_actionExport = m_contextMenu->addAction(tr("Export..."));
    connect(m_actionExport, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        if (!items.isEmpty()) {
            QString id = items.first()->data(COL_NAME, ROLE_ID).toString();
            if (!id.isEmpty()) {
                emit exportItemRequested(id);
            }
        }
    });

    m_contextMenu->addSeparator();

    m_actionDelete = m_contextMenu->addAction(tr("Delete"));
    m_actionDelete->setShortcut(QKeySequence::Delete);
    connect(m_actionDelete, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString id = item->data(COL_NAME, ROLE_ID).toString();
            if (!id.isEmpty()) {
                emit deleteItemRequested(id);
            }
        }
    });
}

void ObjectBrowser::setupShortcuts()
{
    // Ctrl+G - Group selected objects
    m_shortcutGroup = new QShortcut(QKeySequence("Ctrl+G"), this);
    connect(m_shortcutGroup, &QShortcut::activated, 
            this, &ObjectBrowser::onGroupShortcut);
    
    // H - Hide selected
    m_shortcutHide = new QShortcut(QKeySequence("H"), this);
    connect(m_shortcutHide, &QShortcut::activated, 
            this, &ObjectBrowser::onHideShortcut);
    
    // Alt+H - Unhide all
    m_shortcutUnhideAll = new QShortcut(QKeySequence("Alt+H"), this);
    connect(m_shortcutUnhideAll, &QShortcut::activated, 
            this, &ObjectBrowser::onUnhideAllShortcut);
    
    // L - Toggle lock
    m_shortcutToggleLock = new QShortcut(QKeySequence("L"), this);
    connect(m_shortcutToggleLock, &QShortcut::activated, 
            this, &ObjectBrowser::onToggleLockShortcut);
}

void ObjectBrowser::setupDragDrop()
{
    // Drag and drop is already enabled in setupUI
    // Additional configuration can be done here if needed
}

QString ObjectBrowser::getVisibilityIcon(bool visible) const
{
    return visible ? "👁" : "👁‍🗨";
}

QString ObjectBrowser::getLockIcon(bool locked) const
{
    return locked ? "🔒" : "";
}

QTreeWidgetItem* ObjectBrowser::createObjectItem(const QString& name, const QString& id,
                                                   const QString& icon)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(COL_NAME, icon + " " + name);
    item->setData(COL_NAME, ROLE_TYPE, "object");
    item->setData(COL_NAME, ROLE_ID, id);
    item->setData(COL_NAME, ROLE_VISIBLE, true);
    item->setData(COL_NAME, ROLE_LOCKED, false);
    item->setData(COL_NAME, ROLE_GROUP_ID, QString());
    
    // Visibility toggle icon
    item->setText(COL_VISIBILITY, getVisibilityIcon(true));
    item->setToolTip(COL_VISIBILITY, tr("Toggle visibility (H to hide, Alt+H to unhide all)"));
    
    // Lock icon (empty when unlocked)
    item->setText(COL_LOCK, "");
    item->setToolTip(COL_LOCK, tr("Toggle lock (L)"));
    
    // Allow editing and dragging
    item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
    
    return item;
}

QTreeWidgetItem* ObjectBrowser::createGroupItem(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(COL_NAME, "📁 " + name);
    item->setData(COL_NAME, ROLE_TYPE, "group");
    item->setData(COL_NAME, ROLE_ID, id);
    item->setData(COL_NAME, ROLE_VISIBLE, true);
    item->setData(COL_NAME, ROLE_LOCKED, false);
    
    // Visibility toggle icon
    item->setText(COL_VISIBILITY, getVisibilityIcon(true));
    item->setToolTip(COL_VISIBILITY, tr("Toggle group visibility"));
    
    // Lock icon
    item->setText(COL_LOCK, "");
    item->setToolTip(COL_LOCK, tr("Toggle group lock"));
    
    // Style group items
    QFont font = item->font(COL_NAME);
    font.setBold(true);
    item->setFont(COL_NAME, font);
    
    // Allow editing, dragging, and dropping
    item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    item->setExpanded(true);
    
    return item;
}

void ObjectBrowser::addMesh(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "📦");
    m_meshesSection->addChild(item);
    m_itemMap[id] = item;
    m_meshesSection->setExpanded(true);
    applyFilter();
}

void ObjectBrowser::addPrimitive(const QString& name, const QString& id, const QString& type)
{
    QString icon = "◼"; // Default
    if (type == "cylinder") icon = "⬤";
    else if (type == "sphere") icon = "●";
    else if (type == "cone") icon = "🔺";
    
    QTreeWidgetItem* item = createObjectItem(name, id, icon);
    m_primitivesSection->addChild(item);
    m_itemMap[id] = item;
    m_primitivesSection->setExpanded(true);
    applyFilter();
}

void ObjectBrowser::addSketch(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "✎");
    m_sketchesSection->addChild(item);
    m_itemMap[id] = item;
    m_sketchesSection->setExpanded(true);
    applyFilter();
}

void ObjectBrowser::addSurface(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "◇");
    m_surfacesSection->addChild(item);
    m_itemMap[id] = item;
    m_surfacesSection->setExpanded(true);
    applyFilter();
}

void ObjectBrowser::addBody(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "⬡");
    m_bodiesSection->addChild(item);
    m_itemMap[id] = item;
    m_bodiesSection->setExpanded(true);
    applyFilter();
}

void ObjectBrowser::addGroup(const QString& name, const QString& groupId)
{
    QTreeWidgetItem* item = createGroupItem(name, groupId);
    
    // Add groups to the meshes section for now
    // In a more sophisticated implementation, groups could be top-level
    m_meshesSection->addChild(item);
    m_groupMap[groupId] = item;
    m_meshesSection->setExpanded(true);
    
    qDebug() << "ObjectBrowser: Added group" << name << "with id" << groupId;
}

void ObjectBrowser::removeGroup(const QString& groupId)
{
    auto it = m_groupMap.find(groupId);
    if (it != m_groupMap.end()) {
        QTreeWidgetItem* groupItem = it.value();
        
        // Move children back to parent section
        while (groupItem->childCount() > 0) {
            QTreeWidgetItem* child = groupItem->takeChild(0);
            child->setData(COL_NAME, ROLE_GROUP_ID, QString());
            
            // Move to appropriate section (for now, meshes section)
            m_meshesSection->addChild(child);
        }
        
        QTreeWidgetItem* parent = groupItem->parent();
        if (parent) {
            parent->removeChild(groupItem);
        }
        delete groupItem;
        m_groupMap.erase(it);
    }
}

void ObjectBrowser::addItemToGroup(const QString& itemId, const QString& groupId)
{
    auto itemIt = m_itemMap.find(itemId);
    auto groupIt = m_groupMap.find(groupId);
    
    if (itemIt == m_itemMap.end() || groupIt == m_groupMap.end()) {
        return;
    }
    
    QTreeWidgetItem* item = itemIt.value();
    QTreeWidgetItem* group = groupIt.value();
    
    // Remove from current parent
    QTreeWidgetItem* parent = item->parent();
    if (parent) {
        parent->removeChild(item);
    }
    
    // Add to group
    group->addChild(item);
    item->setData(COL_NAME, ROLE_GROUP_ID, groupId);
    group->setExpanded(true);
}

void ObjectBrowser::removeItemFromGroup(const QString& itemId)
{
    auto itemIt = m_itemMap.find(itemId);
    if (itemIt == m_itemMap.end()) {
        return;
    }
    
    QTreeWidgetItem* item = itemIt.value();
    QString groupId = item->data(COL_NAME, ROLE_GROUP_ID).toString();
    
    if (groupId.isEmpty()) {
        return;
    }
    
    // Remove from group
    QTreeWidgetItem* parent = item->parent();
    if (parent) {
        parent->removeChild(item);
    }
    
    // Add back to meshes section
    item->setData(COL_NAME, ROLE_GROUP_ID, QString());
    m_meshesSection->addChild(item);
}

void ObjectBrowser::setGroupExpanded(const QString& groupId, bool expanded)
{
    auto it = m_groupMap.find(groupId);
    if (it != m_groupMap.end()) {
        it.value()->setExpanded(expanded);
    }
}

void ObjectBrowser::setGroupVisible(const QString& groupId, bool visible)
{
    auto it = m_groupMap.find(groupId);
    if (it != m_groupMap.end()) {
        QTreeWidgetItem* item = it.value();
        item->setData(COL_NAME, ROLE_VISIBLE, visible);
        item->setText(COL_VISIBILITY, getVisibilityIcon(visible));
        
        // Update text color
        QColor textColor = visible ? QColor("#b3b3b3") : QColor("#5c5c5c");
        item->setForeground(COL_NAME, textColor);
        
        // Update children
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            QString childId = child->data(COL_NAME, ROLE_ID).toString();
            if (!childId.isEmpty()) {
                setItemVisible(childId, visible);
            }
        }
    }
}

void ObjectBrowser::setGroupLocked(const QString& groupId, bool locked)
{
    auto it = m_groupMap.find(groupId);
    if (it != m_groupMap.end()) {
        QTreeWidgetItem* item = it.value();
        item->setData(COL_NAME, ROLE_LOCKED, locked);
        item->setText(COL_LOCK, getLockIcon(locked));
        
        // Update children
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            QString childId = child->data(COL_NAME, ROLE_ID).toString();
            if (!childId.isEmpty()) {
                setItemLocked(childId, locked);
            }
        }
    }
}

void ObjectBrowser::setGroupName(const QString& groupId, const QString& name)
{
    auto it = m_groupMap.find(groupId);
    if (it != m_groupMap.end()) {
        it.value()->setText(COL_NAME, "📁 " + name);
    }
}

void ObjectBrowser::removeItem(const QString& id)
{
    auto it = m_itemMap.find(id);
    if (it != m_itemMap.end()) {
        QTreeWidgetItem* item = it.value();
        QTreeWidgetItem* parent = item->parent();
        if (parent) {
            parent->removeChild(item);
        }
        delete item;
        m_itemMap.erase(it);
    }
}

void ObjectBrowser::clear()
{
    // Remove all items from sections but keep sections
    auto clearSection = [](QTreeWidgetItem* section) {
        while (section->childCount() > 0) {
            delete section->takeChild(0);
        }
    };
    
    clearSection(m_meshesSection);
    clearSection(m_primitivesSection);
    clearSection(m_sketchesSection);
    clearSection(m_surfacesSection);
    clearSection(m_bodiesSection);
    
    m_itemMap.clear();
    m_groupMap.clear();
}

void ObjectBrowser::setItemVisible(const QString& id, bool visible)
{
    auto it = m_itemMap.find(id);
    if (it != m_itemMap.end()) {
        QTreeWidgetItem* item = it.value();
        item->setData(COL_NAME, ROLE_VISIBLE, visible);
        item->setText(COL_VISIBILITY, getVisibilityIcon(visible));
        
        // Gray out hidden items
        QColor textColor = visible ? QColor("#b3b3b3") : QColor("#5c5c5c");
        item->setForeground(COL_NAME, textColor);
    }
}

void ObjectBrowser::setItemLocked(const QString& id, bool locked)
{
    auto it = m_itemMap.find(id);
    if (it != m_itemMap.end()) {
        QTreeWidgetItem* item = it.value();
        item->setData(COL_NAME, ROLE_LOCKED, locked);
        item->setText(COL_LOCK, getLockIcon(locked));
        
        // Update flags - locked items can't be edited or dragged
        Qt::ItemFlags flags = item->flags();
        if (locked) {
            flags &= ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
        } else {
            flags |= Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
        }
        item->setFlags(flags);
    }
}

void ObjectBrowser::setItemSelected(const QString& id, bool selected)
{
    auto it = m_itemMap.find(id);
    if (it != m_itemMap.end()) {
        it.value()->setSelected(selected);
    }
}

void ObjectBrowser::setItemName(const QString& id, const QString& name)
{
    auto it = m_itemMap.find(id);
    if (it != m_itemMap.end()) {
        QTreeWidgetItem* item = it.value();
        // Preserve the icon prefix
        QString currentText = item->text(COL_NAME);
        int spacePos = currentText.indexOf(' ');
        QString icon = (spacePos > 0) ? currentText.left(spacePos + 1) : "";
        item->setText(COL_NAME, icon + name);
    }
}

void ObjectBrowser::setSelectedItems(const QStringList& ids)
{
    // Block signals during batch update
    m_treeWidget->blockSignals(true);
    
    // Clear existing selection
    m_treeWidget->clearSelection();
    
    // Select items by ID
    for (const QString& id : ids) {
        auto it = m_itemMap.find(id);
        if (it != m_itemMap.end()) {
            it.value()->setSelected(true);
        }
    }
    
    m_treeWidget->blockSignals(false);
}

QStringList ObjectBrowser::selectedItemIds() const
{
    QStringList ids;
    for (auto* item : m_treeWidget->selectedItems()) {
        QString type = item->data(COL_NAME, ROLE_TYPE).toString();
        if (type == "object" || type == "group") {
            QString id = item->data(COL_NAME, ROLE_ID).toString();
            if (!id.isEmpty()) {
                ids.append(id);
            }
        }
    }
    return ids;
}

QTreeWidgetItem* ObjectBrowser::findItemById(const QString& id) const
{
    auto it = m_itemMap.find(id);
    return (it != m_itemMap.end()) ? it.value() : nullptr;
}

QTreeWidgetItem* ObjectBrowser::findGroupById(const QString& groupId) const
{
    auto it = m_groupMap.find(groupId);
    return (it != m_groupMap.end()) ? it.value() : nullptr;
}

void ObjectBrowser::setFilterText(const QString& text)
{
    m_searchBox->setText(text);
}

QString ObjectBrowser::filterText() const
{
    return m_filterText;
}

void ObjectBrowser::onFilterTextChanged(const QString& text)
{
    m_filterText = text;
    applyFilter();
}

void ObjectBrowser::applyFilter()
{
    if (m_filterText.isEmpty()) {
        // Show all items
        for (auto it = m_itemMap.begin(); it != m_itemMap.end(); ++it) {
            it.value()->setHidden(false);
        }
        for (auto it = m_groupMap.begin(); it != m_groupMap.end(); ++it) {
            it.value()->setHidden(false);
        }
        return;
    }
    
    QString filter = m_filterText.toLower();
    
    // Filter items
    for (auto it = m_itemMap.begin(); it != m_itemMap.end(); ++it) {
        QTreeWidgetItem* item = it.value();
        QString name = item->text(COL_NAME).toLower();
        item->setHidden(!name.contains(filter));
    }
    
    // Filter groups - show if name matches or any child matches
    for (auto it = m_groupMap.begin(); it != m_groupMap.end(); ++it) {
        QTreeWidgetItem* group = it.value();
        QString name = group->text(COL_NAME).toLower();
        bool hasVisibleChild = false;
        
        for (int i = 0; i < group->childCount(); ++i) {
            if (!group->child(i)->isHidden()) {
                hasVisibleChild = true;
                break;
            }
        }
        
        group->setHidden(!name.contains(filter) && !hasVisibleChild);
    }
}

bool ObjectBrowser::itemMatchesFilter(QTreeWidgetItem* item) const
{
    if (m_filterText.isEmpty()) {
        return true;
    }
    return item->text(COL_NAME).toLower().contains(m_filterText.toLower());
}

void ObjectBrowser::onItemClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    QString type = item->data(COL_NAME, ROLE_TYPE).toString();
    QString id = item->data(COL_NAME, ROLE_ID).toString();
    
    if (column == COL_VISIBILITY) {
        // Visibility toggle clicked
        if (type == "object" && !id.isEmpty()) {
            bool visible = item->data(COL_NAME, ROLE_VISIBLE).toBool();
            setItemVisible(id, !visible);
            emit visibilityToggled(id, !visible);
        } else if (type == "group" && !id.isEmpty()) {
            bool visible = item->data(COL_NAME, ROLE_VISIBLE).toBool();
            setGroupVisible(id, !visible);
            emit groupVisibilityToggled(id, !visible);
        }
    } else if (column == COL_LOCK) {
        // Lock toggle clicked
        if (type == "object" && !id.isEmpty()) {
            bool locked = item->data(COL_NAME, ROLE_LOCKED).toBool();
            setItemLocked(id, !locked);
            emit lockToggled(id, !locked);
        } else if (type == "group" && !id.isEmpty()) {
            bool locked = item->data(COL_NAME, ROLE_LOCKED).toBool();
            setGroupLocked(id, !locked);
            emit groupLockToggled(id, !locked);
        }
    } else if (column == COL_NAME) {
        // Name clicked - check if locked before allowing selection
        if (type == "object" && !id.isEmpty()) {
            bool locked = item->data(COL_NAME, ROLE_LOCKED).toBool();
            if (!locked) {
                emit itemSelected(id);
            }
        } else if (type == "group") {
            // Groups can always be selected
            emit itemSelected(id);
        }
    }
}

void ObjectBrowser::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    
    QString type = item->data(COL_NAME, ROLE_TYPE).toString();
    
    if (type == "section") {
        // Toggle section expansion
        item->setExpanded(!item->isExpanded());
    } else if (type == "group") {
        // Toggle group expansion or start rename
        if (column == COL_NAME) {
            // Store old name for comparison
            item->setData(COL_NAME, ROLE_OLD_NAME, item->text(COL_NAME));
            m_treeWidget->editItem(item, COL_NAME);
        } else {
            item->setExpanded(!item->isExpanded());
            emit groupExpandedChanged(item->data(COL_NAME, ROLE_ID).toString(), item->isExpanded());
        }
    } else if (type == "object") {
        // Object double-clicked - start rename if not locked
        QString id = item->data(COL_NAME, ROLE_ID).toString();
        bool locked = item->data(COL_NAME, ROLE_LOCKED).toBool();
        
        if (!id.isEmpty() && !locked) {
            // Store old name for comparison
            item->setData(COL_NAME, ROLE_OLD_NAME, item->text(COL_NAME));
            m_treeWidget->editItem(item, COL_NAME);
        }
    }
}

void ObjectBrowser::onItemChanged(QTreeWidgetItem* item, int column)
{
    if (column != COL_NAME) return;
    
    QString type = item->data(COL_NAME, ROLE_TYPE).toString();
    if (type != "object" && type != "group") return;
    
    QString id = item->data(COL_NAME, ROLE_ID).toString();
    QString oldName = item->data(COL_NAME, ROLE_OLD_NAME).toString();
    QString newName = item->text(COL_NAME);
    
    // Clear the stored old name
    item->setData(COL_NAME, ROLE_OLD_NAME, QVariant());
    
    // Only emit if name actually changed
    if (!id.isEmpty() && !oldName.isEmpty() && oldName != newName) {
        if (type == "object") {
            emit itemRenamed(id, newName);
        } else if (type == "group") {
            emit groupRenamed(id, newName);
        }
    }
}

void ObjectBrowser::onSelectionChanged()
{
    emit selectionChanged(selectedItemIds());
}

void ObjectBrowser::showContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) return;
    
    QString type = item->data(COL_NAME, ROLE_TYPE).toString();
    if (type == "section") return;
    
    // Update action states based on selection
    auto selected = m_treeWidget->selectedItems();
    bool hasSelection = !selected.isEmpty();
    bool singleSelection = selected.count() == 1;
    bool multiSelection = selected.count() > 1;
    bool hasGroups = false;
    bool hasObjects = false;
    bool allVisible = true;
    bool allLocked = true;
    
    for (auto* sel : selected) {
        QString selType = sel->data(COL_NAME, ROLE_TYPE).toString();
        if (selType == "group") hasGroups = true;
        if (selType == "object") hasObjects = true;
        if (!sel->data(COL_NAME, ROLE_VISIBLE).toBool()) allVisible = false;
        if (!sel->data(COL_NAME, ROLE_LOCKED).toBool()) allLocked = false;
    }
    
    m_actionHide->setEnabled(hasSelection && allVisible);
    m_actionShow->setEnabled(hasSelection && !allVisible);
    m_actionLock->setEnabled(hasSelection && !allLocked);
    m_actionUnlock->setEnabled(hasSelection && allLocked);
    m_actionDelete->setEnabled(hasSelection);
    m_actionIsolate->setEnabled(singleSelection);
    m_actionExport->setEnabled(singleSelection && hasObjects);
    m_actionRename->setEnabled(singleSelection);
    m_actionGroup->setEnabled(multiSelection && hasObjects);
    m_actionUngroup->setEnabled(hasGroups);
    
    // Show at cursor position
    m_contextMenu->popup(m_treeWidget->viewport()->mapToGlobal(pos));
}

void ObjectBrowser::onGroupShortcut()
{
    QStringList ids = selectedItemIds();
    if (ids.size() >= 2) {
        emit groupRequested(ids);
    }
}

void ObjectBrowser::onHideShortcut()
{
    emit hideSelectedRequested();
}

void ObjectBrowser::onUnhideAllShortcut()
{
    emit unhideAllRequested();
}

void ObjectBrowser::onToggleLockShortcut()
{
    emit toggleLockSelectedRequested();
}

void ObjectBrowser::keyPressEvent(QKeyEvent* event)
{
    // Let parent handle if not a shortcut we care about
    QWidget::keyPressEvent(event);
}

bool ObjectBrowser::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_treeWidget && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // F2 - Rename
        if (keyEvent->key() == Qt::Key_F2) {
            auto items = m_treeWidget->selectedItems();
            if (!items.isEmpty()) {
                QTreeWidgetItem* item = items.first();
                QString type = item->data(COL_NAME, ROLE_TYPE).toString();
                bool locked = item->data(COL_NAME, ROLE_LOCKED).toBool();
                
                if ((type == "object" || type == "group") && !locked) {
                    item->setData(COL_NAME, ROLE_OLD_NAME, item->text(COL_NAME));
                    m_treeWidget->editItem(item, COL_NAME);
                    return true;
                }
            }
        }
    }
    
    return QWidget::eventFilter(watched, event);
}
