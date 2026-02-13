#include "ObjectBrowser.h"
#include <QHeaderView>

ObjectBrowser::ObjectBrowser(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ObjectBrowser");
    setupUI();
    setupSections();
    setupContextMenu();
}

void ObjectBrowser::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Create tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setObjectName("ObjectBrowserTree");
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(16);
    m_treeWidget->setAnimated(true);
    m_treeWidget->setExpandsOnDoubleClick(false);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Setup columns (name, visibility icon)
    m_treeWidget->setColumnCount(2);
    m_treeWidget->header()->setStretchLastSection(false);
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_treeWidget->header()->resizeSection(1, 24);

    // Connect signals
    connect(m_treeWidget, &QTreeWidget::itemClicked, 
            this, &ObjectBrowser::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, 
            this, &ObjectBrowser::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, 
            this, &ObjectBrowser::onSelectionChanged);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, 
            this, &ObjectBrowser::showContextMenu);

    m_layout->addWidget(m_treeWidget);
}

void ObjectBrowser::setupSections()
{
    // Create section headers with icons
    auto createSection = [this](const QString& name, const QString& icon) -> QTreeWidgetItem* {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, name);
        item->setData(0, Qt::UserRole, "section"); // Mark as section
        item->setExpanded(true);
        
        // Style section headers
        QFont font = item->font(0);
        font.setBold(true);
        item->setFont(0, font);
        
        // Section items cannot be selected
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        
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
    m_contextMenu = new QMenu(this);

    m_actionShow = m_contextMenu->addAction(tr("Show"));
    connect(m_actionShow, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString id = item->data(0, Qt::UserRole + 1).toString();
            if (!id.isEmpty()) {
                emit showItemRequested(id);
            }
        }
    });

    m_actionHide = m_contextMenu->addAction(tr("Hide"));
    connect(m_actionHide, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        for (auto* item : items) {
            QString id = item->data(0, Qt::UserRole + 1).toString();
            if (!id.isEmpty()) {
                emit hideItemRequested(id);
            }
        }
    });

    m_contextMenu->addSeparator();

    m_actionIsolate = m_contextMenu->addAction(tr("Isolate"));
    connect(m_actionIsolate, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        if (!items.isEmpty()) {
            QString id = items.first()->data(0, Qt::UserRole + 1).toString();
            if (!id.isEmpty()) {
                emit isolateItemRequested(id);
            }
        }
    });

    m_contextMenu->addSeparator();

    m_actionRename = m_contextMenu->addAction(tr("Rename"));
    m_actionRename->setShortcut(QKeySequence("F2"));
    connect(m_actionRename, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        if (!items.isEmpty()) {
            QString id = items.first()->data(0, Qt::UserRole + 1).toString();
            if (!id.isEmpty()) {
                emit renameItemRequested(id);
            }
        }
    });

    m_actionExport = m_contextMenu->addAction(tr("Export..."));
    connect(m_actionExport, &QAction::triggered, this, [this]() {
        auto items = m_treeWidget->selectedItems();
        if (!items.isEmpty()) {
            QString id = items.first()->data(0, Qt::UserRole + 1).toString();
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
            QString id = item->data(0, Qt::UserRole + 1).toString();
            if (!id.isEmpty()) {
                emit deleteItemRequested(id);
            }
        }
    });
}

QTreeWidgetItem* ObjectBrowser::createObjectItem(const QString& name, const QString& id,
                                                   const QString& icon)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, icon + " " + name);
    item->setData(0, Qt::UserRole, "object"); // Mark as object
    item->setData(0, Qt::UserRole + 1, id);   // Store ID
    item->setData(0, Qt::UserRole + 2, true); // Visibility state
    
    // Visibility toggle icon in column 1
    item->setText(1, "👁");
    item->setToolTip(1, tr("Toggle visibility"));
    
    // Allow editing for rename
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    
    return item;
}

void ObjectBrowser::addMesh(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "📦");
    m_meshesSection->addChild(item);
    m_itemMap[id] = item;
    m_meshesSection->setExpanded(true);
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
}

void ObjectBrowser::addSketch(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "✎");
    m_sketchesSection->addChild(item);
    m_itemMap[id] = item;
    m_sketchesSection->setExpanded(true);
}

void ObjectBrowser::addSurface(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "◇");
    m_surfacesSection->addChild(item);
    m_itemMap[id] = item;
    m_surfacesSection->setExpanded(true);
}

void ObjectBrowser::addBody(const QString& name, const QString& id)
{
    QTreeWidgetItem* item = createObjectItem(name, id, "⬡");
    m_bodiesSection->addChild(item);
    m_itemMap[id] = item;
    m_bodiesSection->setExpanded(true);
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
    for (int i = m_meshesSection->childCount() - 1; i >= 0; --i) {
        delete m_meshesSection->takeChild(i);
    }
    for (int i = m_primitivesSection->childCount() - 1; i >= 0; --i) {
        delete m_primitivesSection->takeChild(i);
    }
    for (int i = m_sketchesSection->childCount() - 1; i >= 0; --i) {
        delete m_sketchesSection->takeChild(i);
    }
    for (int i = m_surfacesSection->childCount() - 1; i >= 0; --i) {
        delete m_surfacesSection->takeChild(i);
    }
    for (int i = m_bodiesSection->childCount() - 1; i >= 0; --i) {
        delete m_bodiesSection->takeChild(i);
    }
    m_itemMap.clear();
}

void ObjectBrowser::setItemVisible(const QString& id, bool visible)
{
    auto it = m_itemMap.find(id);
    if (it != m_itemMap.end()) {
        QTreeWidgetItem* item = it.value();
        item->setData(0, Qt::UserRole + 2, visible);
        item->setText(1, visible ? "👁" : "👁‍🗨");
        
        // Gray out hidden items
        QColor textColor = visible ? QColor("#b3b3b3") : QColor("#5c5c5c");
        item->setForeground(0, textColor);
    }
}

void ObjectBrowser::setItemLocked(const QString& id, bool locked)
{
    auto it = m_itemMap.find(id);
    if (it != m_itemMap.end()) {
        QTreeWidgetItem* item = it.value();
        item->setData(0, Qt::UserRole + 3, locked);
        
        // Update flags - locked items can't be edited
        Qt::ItemFlags flags = item->flags();
        if (locked) {
            flags &= ~Qt::ItemIsEditable;
        } else {
            flags |= Qt::ItemIsEditable;
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
        QString id = item->data(0, Qt::UserRole + 1).toString();
        if (!id.isEmpty()) {
            ids.append(id);
        }
    }
    return ids;
}

QTreeWidgetItem* ObjectBrowser::findItemById(const QString& id) const
{
    auto it = m_itemMap.find(id);
    return (it != m_itemMap.end()) ? it.value() : nullptr;
}

void ObjectBrowser::onItemClicked(QTreeWidgetItem* item, int column)
{
    if (column == 1) {
        // Visibility toggle clicked
        QString id = item->data(0, Qt::UserRole + 1).toString();
        if (!id.isEmpty()) {
            bool visible = item->data(0, Qt::UserRole + 2).toBool();
            setItemVisible(id, !visible);
            emit visibilityToggled(id, !visible);
        }
    } else {
        // Normal selection
        QString id = item->data(0, Qt::UserRole + 1).toString();
        if (!id.isEmpty()) {
            emit itemSelected(id);
        }
    }
}

void ObjectBrowser::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    
    QString type = item->data(0, Qt::UserRole).toString();
    if (type == "section") {
        // Toggle section expansion
        item->setExpanded(!item->isExpanded());
    } else {
        // Object double-clicked (for rename or zoom-to)
        QString id = item->data(0, Qt::UserRole + 1).toString();
        if (!id.isEmpty()) {
            emit itemDoubleClicked(id);
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
    
    QString type = item->data(0, Qt::UserRole).toString();
    if (type == "section") return; // No context menu for sections
    
    // Update action states based on selection
    auto selected = m_treeWidget->selectedItems();
    bool hasSelection = !selected.isEmpty();
    bool singleSelection = selected.count() == 1;
    
    m_actionHide->setEnabled(hasSelection);
    m_actionShow->setEnabled(hasSelection);
    m_actionDelete->setEnabled(hasSelection);
    m_actionIsolate->setEnabled(singleSelection);
    m_actionExport->setEnabled(singleSelection);
    m_actionRename->setEnabled(singleSelection);
    
    // Show at cursor position
    m_contextMenu->popup(m_treeWidget->viewport()->mapToGlobal(pos));
}
