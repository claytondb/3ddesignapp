#include "KeyboardShortcutsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Keyboard Shortcuts"));
    setMinimumSize(500, 600);
    setModal(false);  // Allow user to keep it open while working
    
    setupUI();
    populateShortcuts();
    applyStylesheet();
}

void KeyboardShortcutsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header
    QLabel* header = new QLabel(tr("Keyboard Shortcuts"));
    header->setObjectName("dialogHeader");
    mainLayout->addWidget(header);

    // Search box
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel(tr("Search:"));
    searchLayout->addWidget(searchLabel);
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Type to filter shortcuts..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &KeyboardShortcutsDialog::onSearchTextChanged);
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);

    // Tree widget for shortcuts
    m_treeWidget = new QTreeWidget();
    m_treeWidget->setHeaderLabels({tr("Action"), tr("Shortcut")});
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->header()->setStretchLastSection(false);
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->setIndentation(20);
    mainLayout->addWidget(m_treeWidget);

    // Tip
    QLabel* tip = new QLabel(tr("💡 Tip: Press Shift+F1 then click any button to see its help."));
    tip->setObjectName("tipLabel");
    tip->setWordWrap(true);
    mainLayout->addWidget(tip);

    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setObjectName("primaryButton");
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void KeyboardShortcutsDialog::populateShortcuts()
{
    // File operations
    addCategory(tr("File"), {
        {tr("New Project"), "Ctrl+N"},
        {tr("Open Project"), "Ctrl+O"},
        {tr("Save Project"), "Ctrl+S"},
        {tr("Save As"), "Ctrl+Shift+S"},
        {tr("Import Mesh"), "Ctrl+I"},
        {tr("Import CAD"), "Ctrl+Shift+I"},
        {tr("Export Mesh"), "Ctrl+E"},
        {tr("Exit"), "Alt+F4"}
    });

    // Edit operations
    addCategory(tr("Edit"), {
        {tr("Undo"), "Ctrl+Z"},
        {tr("Redo"), "Ctrl+Y"},
        {tr("Cut"), "Ctrl+X"},
        {tr("Copy"), "Ctrl+C"},
        {tr("Paste"), "Ctrl+V"},
        {tr("Duplicate"), "Ctrl+D"},
        {tr("Delete"), "Delete"},
        {tr("Select All"), "Ctrl+A"},
        {tr("Deselect All"), "Escape"},
        {tr("Preferences"), "Ctrl+,"}
    });

    // View operations
    addCategory(tr("View"), {
        {tr("Front View"), "1"},
        {tr("Back View"), "Ctrl+1"},
        {tr("Left View"), "3"},
        {tr("Right View"), "Ctrl+3"},
        {tr("Top View"), "7"},
        {tr("Bottom View"), "Ctrl+7"},
        {tr("Isometric View"), "0"},
        {tr("Zoom to Fit"), "F"},
        {tr("Zoom to Selection"), "Z"},
        {tr("Toggle Grid"), "G"},
        {tr("Shaded Mode"), "Alt+1"},
        {tr("Wireframe Mode"), "Alt+2"},
        {tr("Shaded + Wireframe"), "Alt+3"},
        {tr("X-Ray Mode"), "Alt+4"},
        {tr("Deviation Map"), "Alt+5"},
        {tr("Object Browser"), "F2"},
        {tr("Properties Panel"), "F3"},
        {tr("Full Screen"), "F11"}
    });

    // Selection modes
    addCategory(tr("Selection"), {
        {tr("Select Mode"), "Q"},
        {tr("Box Select"), "B"},
        {tr("Lasso Select"), "L"}
    });

    // Create operations
    addCategory(tr("Create"), {
        {tr("Create Plane"), "P"},
        {tr("Create Cylinder"), "C"},
        {tr("Section Plane"), "S"},
        {tr("2D Sketch"), "K"},
        {tr("Extrude"), "E"},
        {tr("Revolve"), "R"}
    });

    // Mesh operations
    addCategory(tr("Mesh Tools"), {
        {tr("Polygon Reduction"), "Ctrl+Shift+R"},
        {tr("Smoothing"), "Ctrl+Shift+M"},
        {tr("Fill Holes"), "Ctrl+Shift+H"},
        {tr("Clipping Box"), "Ctrl+Shift+B"}
    });

    // Navigation
    addCategory(tr("Navigation"), {
        {tr("Orbit (rotate view)"), tr("Middle Mouse Drag")},
        {tr("Pan (move view)"), tr("Shift + Middle Mouse")},
        {tr("Zoom"), tr("Scroll Wheel")},
        {tr("Zoom to cursor"), tr("Ctrl + Scroll")},
        {tr("Focus on selection"), "F"}
    });

    // Help
    addCategory(tr("Help"), {
        {tr("What's This? Mode"), "Shift+F1"},
        {tr("Context Help"), "F1"}
    });

    // Expand all categories
    m_treeWidget->expandAll();
}

void KeyboardShortcutsDialog::addCategory(const QString& category, 
                                          const QList<QPair<QString, QString>>& shortcuts)
{
    QTreeWidgetItem* categoryItem = new QTreeWidgetItem(m_treeWidget);
    categoryItem->setText(0, category);
    categoryItem->setExpanded(true);
    
    // Make category bold
    QFont font = categoryItem->font(0);
    font.setBold(true);
    categoryItem->setFont(0, font);
    
    for (const auto& shortcut : shortcuts) {
        QTreeWidgetItem* item = new QTreeWidgetItem(categoryItem);
        item->setText(0, shortcut.first);
        item->setText(1, shortcut.second);
    }
}

void KeyboardShortcutsDialog::onSearchTextChanged(const QString& text)
{
    QString searchText = text.toLower();
    
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* category = m_treeWidget->topLevelItem(i);
        bool categoryVisible = false;
        
        for (int j = 0; j < category->childCount(); ++j) {
            QTreeWidgetItem* item = category->child(j);
            bool matches = searchText.isEmpty() ||
                          item->text(0).toLower().contains(searchText) ||
                          item->text(1).toLower().contains(searchText);
            item->setHidden(!matches);
            if (matches) categoryVisible = true;
        }
        
        // Show category if any child matches, or if search is empty
        category->setHidden(!categoryVisible && !searchText.isEmpty());
    }
}

void KeyboardShortcutsDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QLabel#dialogHeader {
            color: #ffffff;
            font-size: 18px;
            font-weight: bold;
            padding-bottom: 8px;
        }
        
        QLabel {
            color: #b3b3b3;
        }
        
        QLabel#tipLabel {
            color: #808080;
            font-size: 11px;
            padding: 8px;
            background-color: #242424;
            border-radius: 4px;
        }
        
        QLineEdit {
            background-color: #333333;
            color: #ffffff;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px 10px;
            font-size: 13px;
        }
        
        QLineEdit:focus {
            border-color: #0078d4;
        }
        
        QTreeWidget {
            background-color: #242424;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            outline: none;
        }
        
        QTreeWidget::item {
            padding: 6px 4px;
        }
        
        QTreeWidget::item:hover {
            background-color: #383838;
        }
        
        QTreeWidget::item:selected {
            background-color: #0078d4;
            color: #ffffff;
        }
        
        QTreeWidget::branch:has-children:!has-siblings:closed,
        QTreeWidget::branch:closed:has-children:has-siblings {
            border-image: none;
            image: url(:/icons/branch-closed.png);
        }
        
        QTreeWidget::branch:open:has-children:!has-siblings,
        QTreeWidget::branch:open:has-children:has-siblings {
            border-image: none;
            image: url(:/icons/branch-open.png);
        }
        
        QHeaderView::section {
            background-color: #333333;
            color: #ffffff;
            padding: 8px;
            border: none;
            border-bottom: 1px solid #4a4a4a;
            font-weight: bold;
        }
        
        QPushButton#primaryButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 24px;
            font-size: 13px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton#primaryButton:hover {
            background-color: #1a88e0;
        }
        
        QPushButton#primaryButton:pressed {
            background-color: #0066b8;
        }
    )");
}
