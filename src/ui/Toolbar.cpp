#include "Toolbar.h"
#include <QMenu>
#include <QWidgetAction>
#include <QMap>
#include <QStyle>

Toolbar::Toolbar(QWidget *parent)
    : QToolBar(tr("Main Toolbar"), parent)
{
    setObjectName("MainToolbar");
    setMovable(false);
    setIconSize(QSize(24, 24));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    setupFileGroup();
    addSeparator();
    setupHistoryGroup();
    addSeparator();
    setupSelectionGroup();
    addSeparator();
    setupViewGroup();
    addSeparator();
    setupCreateGroup();
    addSeparator();
    setupMeshToolsGroup();
    
    // Add expanding spacer
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    addWidget(spacer);
    
    setupSearchWidget();
}

QAction* Toolbar::createAction(const QString& text, const QString& iconName,
                                const QString& tooltip, const QString& shortcut)
{
    QAction* action = new QAction(text, this);
    action->setToolTip(tooltip + (shortcut.isEmpty() ? "" : " (" + shortcut + ")"));
    action->setStatusTip(tooltip);
    
    // Try loading custom icon first, fallback to Qt standard icons
    QIcon icon = QIcon(":/icons/" + iconName + ".svg");
    if (icon.isNull()) {
        // Map icon names to Qt standard icons
        static const QMap<QString, QStyle::StandardPixmap> iconMap = {
            {"file-new", QStyle::SP_FileIcon},
            {"folder-open", QStyle::SP_DirOpenIcon},
            {"save", QStyle::SP_DialogSaveButton},
            {"import", QStyle::SP_ArrowDown},
            {"undo", QStyle::SP_ArrowBack},
            {"redo", QStyle::SP_ArrowForward},
            {"select-pointer", QStyle::SP_ArrowUp},
            {"select-box", QStyle::SP_FileDialogContentsView},
            {"select-lasso", QStyle::SP_FileDialogDetailedView},
            {"select-brush", QStyle::SP_DialogResetButton},
            {"view-shaded", QStyle::SP_DesktopIcon},
            {"view-wireframe", QStyle::SP_TitleBarNormalButton},
            {"view-shaded-wire", QStyle::SP_TitleBarMaxButton},
            {"view-xray", QStyle::SP_TitleBarShadeButton},
            {"primitive-plane", QStyle::SP_FileDialogNewFolder},
            {"primitive-cylinder", QStyle::SP_DriveHDIcon},
            {"section", QStyle::SP_ToolBarHorizontalExtensionButton},
            {"sketch", QStyle::SP_FileDialogListView},
            {"mesh-reduce", QStyle::SP_BrowserReload},
            {"mesh-smooth", QStyle::SP_MediaVolume},
            {"mesh-fill", QStyle::SP_MediaVolumeMuted},
            {"mesh-clip", QStyle::SP_DialogDiscardButton}
        };
        
        if (iconMap.contains(iconName)) {
            icon = style()->standardIcon(iconMap[iconName]);
        }
    }
    
    if (!icon.isNull()) {
        action->setIcon(icon);
    }
    
    // Keep full text for display under icon
    action->setText(text);
    
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    
    return action;
}

QToolButton* Toolbar::createMenuButton(const QString& text, const QString& iconName,
                                        const QString& tooltip)
{
    QToolButton* button = new QToolButton(this);
    button->setToolTip(tooltip);
    button->setPopupMode(QToolButton::MenuButtonPopup);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    // Try loading custom icon first, fallback to Qt standard icons
    QIcon icon = QIcon(":/icons/" + iconName + ".svg");
    if (icon.isNull()) {
        static const QMap<QString, QStyle::StandardPixmap> iconMap = {
            {"folder-open", QStyle::SP_DirOpenIcon},
            {"import", QStyle::SP_ArrowDown},
            {"undo", QStyle::SP_ArrowBack},
            {"redo", QStyle::SP_ArrowForward}
        };
        
        if (iconMap.contains(iconName)) {
            icon = style()->standardIcon(iconMap[iconName]);
        }
    }
    
    if (!icon.isNull()) {
        button->setIcon(icon);
    }
    button->setText(text);
    return button;
}

void Toolbar::setupFileGroup()
{
    // New
    m_actionNew = createAction(tr("New"), "file-new", tr("New Project (Ctrl+N)"), "Ctrl+N");
    connect(m_actionNew, &QAction::triggered, this, &Toolbar::newRequested);
    addAction(m_actionNew);

    // Open with recent files menu
    QToolButton* openButton = createMenuButton(tr("Open"), "folder-open", tr("Open Project"));
    m_actionOpen = createAction(tr("Open"), "folder-open", tr("Open Project (Ctrl+O)"), "Ctrl+O");
    openButton->setDefaultAction(m_actionOpen);
    
    QMenu* openMenu = new QMenu(this);
    openMenu->addAction(m_actionOpen);
    openMenu->addSeparator();
    QAction* recentAction = openMenu->addAction(tr("Recent Files..."));
    recentAction->setEnabled(false);
    openButton->setMenu(openMenu);
    
    connect(m_actionOpen, &QAction::triggered, this, &Toolbar::openRequested);
    addWidget(openButton);

    // Save
    m_actionSave = createAction(tr("Save"), "save", tr("Save Project (Ctrl+S)"), "Ctrl+S");
    connect(m_actionSave, &QAction::triggered, this, &Toolbar::saveRequested);
    addAction(m_actionSave);

    // Import with submenu
    QToolButton* importButton = createMenuButton(tr("Import"), "import", tr("Import File"));
    m_actionImport = createAction(tr("Import"), "import", tr("Import Mesh or CAD file"));
    importButton->setDefaultAction(m_actionImport);
    
    QMenu* importMenu = new QMenu(this);
    importMenu->addAction(tr("Mesh (STL, OBJ, PLY)..."));
    importMenu->addAction(tr("CAD (STEP, IGES)..."));
    importButton->setMenu(importMenu);
    
    connect(m_actionImport, &QAction::triggered, this, &Toolbar::importRequested);
    addWidget(importButton);
}

void Toolbar::setupHistoryGroup()
{
    // Undo with history menu
    QToolButton* undoButton = createMenuButton(tr("Undo"), "undo", tr("Undo"));
    m_actionUndo = createAction(tr("Undo"), "undo", tr("Undo (Ctrl+Z)"), "Ctrl+Z");
    undoButton->setDefaultAction(m_actionUndo);
    
    QMenu* undoMenu = new QMenu(this);
    undoMenu->addAction(tr("(No undo history)"))->setEnabled(false);
    undoButton->setMenu(undoMenu);
    
    connect(m_actionUndo, &QAction::triggered, this, &Toolbar::undoRequested);
    addWidget(undoButton);

    // Redo with history menu
    QToolButton* redoButton = createMenuButton(tr("Redo"), "redo", tr("Redo"));
    m_actionRedo = createAction(tr("Redo"), "redo", tr("Redo (Ctrl+Y)"), "Ctrl+Y");
    redoButton->setDefaultAction(m_actionRedo);
    
    QMenu* redoMenu = new QMenu(this);
    redoMenu->addAction(tr("(No redo history)"))->setEnabled(false);
    redoButton->setMenu(redoMenu);
    
    connect(m_actionRedo, &QAction::triggered, this, &Toolbar::redoRequested);
    addWidget(redoButton);
}

void Toolbar::setupSelectionGroup()
{
    m_selectionGroup = new QActionGroup(this);
    m_selectionGroup->setExclusive(true);

    // Click Select
    m_actionSelect = createAction(tr("Select"), "select-pointer", tr("Click Select (Q)"), "Q");
    m_actionSelect->setCheckable(true);
    m_actionSelect->setChecked(true);
    m_selectionGroup->addAction(m_actionSelect);
    connect(m_actionSelect, &QAction::triggered, this, &Toolbar::selectModeRequested);
    addAction(m_actionSelect);

    // Box Select
    m_actionBoxSelect = createAction(tr("Box"), "select-box", tr("Box Select"));
    m_actionBoxSelect->setCheckable(true);
    m_selectionGroup->addAction(m_actionBoxSelect);
    connect(m_actionBoxSelect, &QAction::triggered, this, &Toolbar::boxSelectModeRequested);
    addAction(m_actionBoxSelect);

    // Lasso Select
    m_actionLassoSelect = createAction(tr("Lasso"), "select-lasso", tr("Lasso Select"));
    m_actionLassoSelect->setCheckable(true);
    m_selectionGroup->addAction(m_actionLassoSelect);
    connect(m_actionLassoSelect, &QAction::triggered, this, &Toolbar::lassoSelectModeRequested);
    addAction(m_actionLassoSelect);

    // Brush Select
    m_actionBrushSelect = createAction(tr("Brush"), "select-brush", tr("Brush Select"));
    m_actionBrushSelect->setCheckable(true);
    m_selectionGroup->addAction(m_actionBrushSelect);
    connect(m_actionBrushSelect, &QAction::triggered, this, &Toolbar::brushSelectModeRequested);
    addAction(m_actionBrushSelect);
}

void Toolbar::setupViewGroup()
{
    m_viewModeGroup = new QActionGroup(this);
    m_viewModeGroup->setExclusive(true);

    // Shaded
    m_actionShaded = createAction(tr("Shaded"), "view-shaded", tr("Shaded Mode (1)"), "1");
    m_actionShaded->setCheckable(true);
    m_actionShaded->setChecked(true);
    m_viewModeGroup->addAction(m_actionShaded);
    connect(m_actionShaded, &QAction::triggered, this, &Toolbar::shadedModeRequested);
    addAction(m_actionShaded);

    // Wireframe
    m_actionWireframe = createAction(tr("Wire"), "view-wireframe", tr("Wireframe Mode (2)"), "2");
    m_actionWireframe->setCheckable(true);
    m_viewModeGroup->addAction(m_actionWireframe);
    connect(m_actionWireframe, &QAction::triggered, this, &Toolbar::wireframeModeRequested);
    addAction(m_actionWireframe);

    // Shaded + Wireframe
    m_actionShadedWire = createAction(tr("S+W"), "view-shaded-wire", tr("Shaded + Wireframe Mode (3)"), "3");
    m_actionShadedWire->setCheckable(true);
    m_viewModeGroup->addAction(m_actionShadedWire);
    connect(m_actionShadedWire, &QAction::triggered, this, &Toolbar::shadedWireModeRequested);
    addAction(m_actionShadedWire);

    // X-Ray
    m_actionXRay = createAction(tr("X-Ray"), "view-xray", tr("X-Ray Mode (4)"), "4");
    m_actionXRay->setCheckable(true);
    m_viewModeGroup->addAction(m_actionXRay);
    connect(m_actionXRay, &QAction::triggered, this, &Toolbar::xrayModeRequested);
    addAction(m_actionXRay);
}

void Toolbar::setupCreateGroup()
{
    // Plane primitive
    m_actionCreatePlane = createAction(tr("Plane"), "primitive-plane", tr("Create Plane (P)"), "P");
    connect(m_actionCreatePlane, &QAction::triggered, this, &Toolbar::createPlaneRequested);
    addAction(m_actionCreatePlane);

    // Cylinder primitive
    m_actionCreateCylinder = createAction(tr("Cyl"), "primitive-cylinder", tr("Create Cylinder (C)"), "C");
    connect(m_actionCreateCylinder, &QAction::triggered, this, &Toolbar::createCylinderRequested);
    addAction(m_actionCreateCylinder);

    // Section plane
    m_actionCreateSection = createAction(tr("Sect"), "section", tr("Section Plane (S)"), "S");
    connect(m_actionCreateSection, &QAction::triggered, this, &Toolbar::createSectionRequested);
    addAction(m_actionCreateSection);

    // 2D Sketch
    m_actionCreateSketch = createAction(tr("Sketch"), "sketch", tr("2D Sketch (K)"), "K");
    connect(m_actionCreateSketch, &QAction::triggered, this, &Toolbar::createSketchRequested);
    addAction(m_actionCreateSketch);
}

void Toolbar::setupMeshToolsGroup()
{
    // Polygon Reduction
    m_actionPolygonReduction = createAction(tr("Reduce"), "mesh-reduce", 
        tr("Polygon Reduction (Ctrl+Shift+R)"), "Ctrl+Shift+R");
    connect(m_actionPolygonReduction, &QAction::triggered, this, &Toolbar::polygonReductionRequested);
    addAction(m_actionPolygonReduction);

    // Smoothing
    m_actionSmoothing = createAction(tr("Smooth"), "mesh-smooth", 
        tr("Mesh Smoothing (Ctrl+Shift+S)"), "Ctrl+Shift+S");
    connect(m_actionSmoothing, &QAction::triggered, this, &Toolbar::smoothingRequested);
    addAction(m_actionSmoothing);

    // Fill Holes
    m_actionFillHoles = createAction(tr("Fill"), "mesh-fill", 
        tr("Fill Holes (Ctrl+Shift+H)"), "Ctrl+Shift+H");
    connect(m_actionFillHoles, &QAction::triggered, this, &Toolbar::fillHolesRequested);
    addAction(m_actionFillHoles);

    // Clipping Box
    m_actionClippingBox = createAction(tr("Clip"), "mesh-clip", 
        tr("Clipping Box (Ctrl+Shift+B)"), "Ctrl+Shift+B");
    connect(m_actionClippingBox, &QAction::triggered, this, &Toolbar::clippingBoxRequested);
    addAction(m_actionClippingBox);
}

void Toolbar::setupSearchWidget()
{
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search..."));
    m_searchEdit->setFixedWidth(150);
    m_searchEdit->setClearButtonEnabled(true);
    
    // Style the search box
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #333333;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QLineEdit:focus {
            border-color: #0078d4;
        }
    )");
    
    connect(m_searchEdit, &QLineEdit::textChanged, this, &Toolbar::searchTextChanged);
    addWidget(m_searchEdit);
}
