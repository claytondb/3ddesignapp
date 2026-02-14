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
    setupTransformGroup();
    addSeparator();
    setupViewGroup();
    addSeparator();
    setupCreateGroup();
    addSeparator();
    setupMeshToolsGroup();
    addSeparator();
    setupMeasureToolsGroup();
    
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
            {"transform-move", QStyle::SP_ArrowUp},
            {"transform-rotate", QStyle::SP_BrowserReload},
            {"transform-scale", QStyle::SP_TitleBarMaxButton},
            {"view-shaded", QStyle::SP_DesktopIcon},
            {"view-wireframe", QStyle::SP_TitleBarNormalButton},
            {"view-shaded-wire", QStyle::SP_TitleBarMaxButton},
            {"view-xray", QStyle::SP_TitleBarShadeButton},
            {"primitive-cube", QStyle::SP_ComputerIcon},
            {"primitive-sphere", QStyle::SP_DriveNetIcon},
            {"primitive-cylinder", QStyle::SP_DriveHDIcon},
            {"primitive-cone", QStyle::SP_TitleBarUnshadeButton},
            {"primitive-plane", QStyle::SP_FileDialogNewFolder},
            {"section", QStyle::SP_ToolBarHorizontalExtensionButton},
            {"sketch", QStyle::SP_FileDialogListView},
            {"mesh-reduce", QStyle::SP_BrowserReload},
            {"mesh-smooth", QStyle::SP_MediaVolume},
            {"mesh-fill", QStyle::SP_MediaVolumeMuted},
            {"mesh-clip", QStyle::SP_DialogDiscardButton},
            {"measure-distance", QStyle::SP_DialogHelpButton},
            {"measure-angle", QStyle::SP_DialogHelpButton},
            {"measure-radius", QStyle::SP_DialogHelpButton},
            {"measure-clear", QStyle::SP_DialogResetButton}
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

    // Click Select - standard pointer selection
    m_actionSelect = createAction(tr("Select"), "select-pointer", 
        tr("Click to select individual objects. Hold Shift to add to selection, Ctrl to toggle."), "Q");
    m_actionSelect->setCheckable(true);
    m_actionSelect->setChecked(true);
    m_selectionGroup->addAction(m_actionSelect);
    connect(m_actionSelect, &QAction::triggered, this, &Toolbar::selectModeRequested);
    addAction(m_actionSelect);

    // Box Select - draw rectangle to select
    m_actionBoxSelect = createAction(tr("Box"), "select-box", 
        tr("Draw a rectangle to select multiple objects. Click and drag to define the selection area."), "B");
    m_actionBoxSelect->setCheckable(true);
    m_selectionGroup->addAction(m_actionBoxSelect);
    connect(m_actionBoxSelect, &QAction::triggered, this, &Toolbar::boxSelectModeRequested);
    addAction(m_actionBoxSelect);

    // Lasso Select - freehand selection
    m_actionLassoSelect = createAction(tr("Lasso"), "select-lasso", 
        tr("Draw a freehand shape to select objects inside it. Click and drag to draw."), "L");
    m_actionLassoSelect->setCheckable(true);
    m_selectionGroup->addAction(m_actionLassoSelect);
    connect(m_actionLassoSelect, &QAction::triggered, this, &Toolbar::lassoSelectModeRequested);
    addAction(m_actionLassoSelect);

    // Brush Select - paint selection
    m_actionBrushSelect = createAction(tr("Brush"), "select-brush", 
        tr("Paint to select faces or vertices. Use scroll wheel to change brush size."), "");
    m_actionBrushSelect->setCheckable(true);
    m_selectionGroup->addAction(m_actionBrushSelect);
    connect(m_actionBrushSelect, &QAction::triggered, this, &Toolbar::brushSelectModeRequested);
    addAction(m_actionBrushSelect);
}

void Toolbar::setupTransformGroup()
{
    m_transformGroup = new QActionGroup(this);
    m_transformGroup->setExclusive(true);
    
    // Translate - move objects
    m_actionTranslate = createAction(tr("Move"), "transform-move",
        tr("Move selected objects. Drag gizmo axes or enter values."), "G");
    m_actionTranslate->setCheckable(true);
    m_actionTranslate->setChecked(true);  // Default mode
    m_transformGroup->addAction(m_actionTranslate);
    connect(m_actionTranslate, &QAction::triggered, this, &Toolbar::translateModeRequested);
    addAction(m_actionTranslate);
    
    // Rotate - rotate objects
    m_actionRotate = createAction(tr("Rotate"), "transform-rotate",
        tr("Rotate selected objects. Drag rotation rings or enter angles."), "R");
    m_actionRotate->setCheckable(true);
    m_transformGroup->addAction(m_actionRotate);
    connect(m_actionRotate, &QAction::triggered, this, &Toolbar::rotateModeRequested);
    addAction(m_actionRotate);
    
    // Scale - resize objects
    m_actionScale = createAction(tr("Scale"), "transform-scale",
        tr("Scale selected objects. Drag handles or enter scale factors."), "S");
    m_actionScale->setCheckable(true);
    m_transformGroup->addAction(m_actionScale);
    connect(m_actionScale, &QAction::triggered, this, &Toolbar::scaleModeRequested);
    addAction(m_actionScale);
}

void Toolbar::setupViewGroup()
{
    m_viewModeGroup = new QActionGroup(this);
    m_viewModeGroup->setExclusive(true);

    // Shaded - standard solid view
    m_actionShaded = createAction(tr("Shaded"), "view-shaded", 
        tr("Solid shaded view - shows surfaces with lighting and materials."), "Alt+1");
    m_actionShaded->setCheckable(true);
    m_actionShaded->setChecked(true);
    m_viewModeGroup->addAction(m_actionShaded);
    connect(m_actionShaded, &QAction::triggered, this, &Toolbar::shadedModeRequested);
    addAction(m_actionShaded);

    // Wireframe - show edges only
    m_actionWireframe = createAction(tr("Wire"), "view-wireframe", 
        tr("Wireframe view - shows mesh edges only. Useful for seeing internal structure."), "Alt+2");
    m_actionWireframe->setCheckable(true);
    m_viewModeGroup->addAction(m_actionWireframe);
    connect(m_actionWireframe, &QAction::triggered, this, &Toolbar::wireframeModeRequested);
    addAction(m_actionWireframe);

    // Shaded + Wireframe - combined view
    m_actionShadedWire = createAction(tr("S+W"), "view-shaded-wire", 
        tr("Shaded with wireframe overlay - shows surfaces and mesh edges together."), "Alt+3");
    m_actionShadedWire->setCheckable(true);
    m_viewModeGroup->addAction(m_actionShadedWire);
    connect(m_actionShadedWire, &QAction::triggered, this, &Toolbar::shadedWireModeRequested);
    addAction(m_actionShadedWire);

    // X-Ray - transparent view
    m_actionXRay = createAction(tr("X-Ray"), "view-xray", 
        tr("X-Ray transparent view - see through surfaces to internal geometry."), "Alt+4");
    m_actionXRay->setCheckable(true);
    m_viewModeGroup->addAction(m_actionXRay);
    connect(m_actionXRay, &QAction::triggered, this, &Toolbar::xrayModeRequested);
    addAction(m_actionXRay);
}

void Toolbar::setupCreateGroup()
{
    // Cube primitive - most common
    m_actionCreateCube = createAction(tr("Cube"), "primitive-cube", 
        tr("Create a cube (box). Opens dialog for size presets or custom dimensions."), "B");
    connect(m_actionCreateCube, &QAction::triggered, this, &Toolbar::createCubeRequested);
    addAction(m_actionCreateCube);
    
    // Sphere primitive
    m_actionCreateSphere = createAction(tr("Sphere"), "primitive-sphere", 
        tr("Create a sphere. Opens dialog for radius and resolution settings."), "");
    connect(m_actionCreateSphere, &QAction::triggered, this, &Toolbar::createSphereRequested);
    addAction(m_actionCreateSphere);

    // Cylinder primitive
    m_actionCreateCylinder = createAction(tr("Cyl"), "primitive-cylinder", 
        tr("Create a cylinder. Opens dialog for radius and height settings."), "C");
    connect(m_actionCreateCylinder, &QAction::triggered, this, &Toolbar::createCylinderRequested);
    addAction(m_actionCreateCylinder);
    
    // Cone primitive
    m_actionCreateCone = createAction(tr("Cone"), "primitive-cone", 
        tr("Create a cone. Opens dialog for base radius and height settings."), "");
    connect(m_actionCreateCone, &QAction::triggered, this, &Toolbar::createConeRequested);
    addAction(m_actionCreateCone);

    // Plane primitive - reference plane for sketching
    m_actionCreatePlane = createAction(tr("Plane"), "primitive-plane", 
        tr("Create a reference plane for sketching or alignment."), "P");
    connect(m_actionCreatePlane, &QAction::triggered, this, &Toolbar::createPlaneRequested);
    addAction(m_actionCreatePlane);
    
    addSeparator();

    // Section plane - cross-section view
    m_actionCreateSection = createAction(tr("Sect"), "section", 
        tr("Create a section plane to see inside the model. Drag the plane to move the cut location."), "S");
    connect(m_actionCreateSection, &QAction::triggered, this, &Toolbar::createSectionRequested);
    addAction(m_actionCreateSection);

    // 2D Sketch - start sketching
    m_actionCreateSketch = createAction(tr("Sketch"), "sketch", 
        tr("Start a 2D sketch on a plane or face. Use sketch tools to draw shapes, then extrude."), "K");
    connect(m_actionCreateSketch, &QAction::triggered, this, &Toolbar::createSketchRequested);
    addAction(m_actionCreateSketch);
}

void Toolbar::setupMeshToolsGroup()
{
    // Repair Wizard - one-click mesh repair
    m_actionMeshRepairWizard = createAction(tr("Repair"), "mesh-repair", 
        tr("One-click mesh repair wizard. Fixes holes, non-manifold edges, and more."), "Ctrl+Shift+W");
    connect(m_actionMeshRepairWizard, &QAction::triggered, this, &Toolbar::meshRepairWizardRequested);
    addAction(m_actionMeshRepairWizard);

    addSeparator();

    // Polygon Reduction - simplify mesh
    m_actionPolygonReduction = createAction(tr("Reduce"), "mesh-reduce", 
        tr("Reduce polygon count while preserving shape. Great for large scanned meshes."), "Ctrl+Shift+R");
    connect(m_actionPolygonReduction, &QAction::triggered, this, &Toolbar::polygonReductionRequested);
    addAction(m_actionPolygonReduction);

    // Smoothing - remove noise
    m_actionSmoothing = createAction(tr("Smooth"), "mesh-smooth", 
        tr("Smooth mesh to reduce noise and bumps. Use after scanning or for cleaner surfaces."), "Ctrl+Shift+M");
    connect(m_actionSmoothing, &QAction::triggered, this, &Toolbar::smoothingRequested);
    addAction(m_actionSmoothing);

    // Fill Holes - repair mesh
    m_actionFillHoles = createAction(tr("Fill"), "mesh-fill", 
        tr("Detect and fill holes in the mesh. Choose flat, smooth, or curvature-based fill."), "Ctrl+Shift+H");
    connect(m_actionFillHoles, &QAction::triggered, this, &Toolbar::fillHolesRequested);
    addAction(m_actionFillHoles);

    // Clipping Box - crop mesh
    m_actionClippingBox = createAction(tr("Clip"), "mesh-clip", 
        tr("Create a clipping box to hide or remove parts of the mesh outside the box."), "Ctrl+Shift+B");
    connect(m_actionClippingBox, &QAction::triggered, this, &Toolbar::clippingBoxRequested);
    addAction(m_actionClippingBox);
}

void Toolbar::setupMeasureToolsGroup()
{
    m_measureGroup = new QActionGroup(this);
    m_measureGroup->setExclusive(true);
    
    // Distance measurement - ruler icon
    m_actionMeasureDistance = createAction(tr("Dist"), "measure-distance", 
        tr("Measure point-to-point distance. Click two points to measure."), "M");
    m_actionMeasureDistance->setCheckable(true);
    m_measureGroup->addAction(m_actionMeasureDistance);
    connect(m_actionMeasureDistance, &QAction::triggered, this, &Toolbar::measureDistanceRequested);
    addAction(m_actionMeasureDistance);
    
    // Angle measurement
    m_actionMeasureAngle = createAction(tr("Angle"), "measure-angle", 
        tr("Measure angle between three points. Click vertex and two arm endpoints."), "");
    m_actionMeasureAngle->setCheckable(true);
    m_measureGroup->addAction(m_actionMeasureAngle);
    connect(m_actionMeasureAngle, &QAction::triggered, this, &Toolbar::measureAngleRequested);
    addAction(m_actionMeasureAngle);
    
    // Radius measurement
    m_actionMeasureRadius = createAction(tr("Rad"), "measure-radius", 
        tr("Measure radius of curved surface. Click on a curved region."), "");
    m_actionMeasureRadius->setCheckable(true);
    m_measureGroup->addAction(m_actionMeasureRadius);
    connect(m_actionMeasureRadius, &QAction::triggered, this, &Toolbar::measureRadiusRequested);
    addAction(m_actionMeasureRadius);
    
    // Clear measurements
    m_actionClearMeasurements = createAction(tr("Clear"), "measure-clear", 
        tr("Clear all measurements from the viewport."), "");
    connect(m_actionClearMeasurements, &QAction::triggered, this, &Toolbar::clearMeasurementsRequested);
    addAction(m_actionClearMeasurements);
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
