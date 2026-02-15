#include "MenuBar.h"
#include "../core/CommandStack.h"
#include <QUndoStack>
#include "dialogs/MeshRepairWizard.h"
#include "dialogs/PolygonReductionDialog.h"
#include "dialogs/SmoothingDialog.h"
#include "dialogs/HoleFillDialog.h"
#include "dialogs/OutlierRemovalDialog.h"
#include "dialogs/ClippingBoxDialog.h"
#include "dialogs/KeyboardShortcutsDialog.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/GettingStartedDialog.h"
#include "dialogs/ExportPresetsDialog.h"
#include "HelpSystem.h"
#include <QActionGroup>
#include <QApplication>
#include <QWhatsThis>
#include <QDesktopServices>
#include <QUrl>

MenuBar::MenuBar(QWidget *parent)
    : QMenuBar(parent)
    , m_polygonReductionDialog(nullptr)
    , m_smoothingDialog(nullptr)
    , m_holeFillDialog(nullptr)
    , m_outlierRemovalDialog(nullptr)
    , m_clippingBoxDialog(nullptr)
    , m_viewport(nullptr)
{
    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
    setupMeshMenu();
    setupToolsMenu();
    setupCreateMenu();
    setupHelpMenu();
    createMeshDialogs();
}

QAction* MenuBar::createAction(const QString& text, const QString& shortcut, const QString& tooltip)
{
    QAction* action = new QAction(text, this);
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    if (!tooltip.isEmpty()) {
        action->setToolTip(tooltip);
        action->setStatusTip(tooltip);
    }
    return action;
}

void MenuBar::setupFileMenu()
{
    m_fileMenu = addMenu(tr("&File"));

    // New Project
    m_actionNew = createAction(tr("&New Project"), "Ctrl+N", tr("Create a new project"));
    m_actionNew->setWhatsThis(HelpText::newProject());
    connect(m_actionNew, &QAction::triggered, this, &MenuBar::newProjectRequested);
    m_fileMenu->addAction(m_actionNew);

    // Open
    m_actionOpen = createAction(tr("&Open..."), "Ctrl+O", tr("Open an existing project"));
    m_actionOpen->setWhatsThis(HelpText::openProject());
    connect(m_actionOpen, &QAction::triggered, this, &MenuBar::openProjectRequested);
    m_fileMenu->addAction(m_actionOpen);

    // Recent Files submenu
    m_recentFilesMenu = m_fileMenu->addMenu(tr("Open &Recent"));

    m_fileMenu->addSeparator();

    // Save
    m_actionSave = createAction(tr("&Save"), "Ctrl+S", tr("Save the current project"));
    m_actionSave->setWhatsThis(HelpText::saveProject());
    connect(m_actionSave, &QAction::triggered, this, &MenuBar::saveProjectRequested);
    m_fileMenu->addAction(m_actionSave);

    // Save As
    m_actionSaveAs = createAction(tr("Save &As..."), "Ctrl+Shift+S", tr("Save project with a new name"));
    m_actionSaveAs->setWhatsThis(tr("<b>Save As</b><br><br>Save the project with a new filename or location.<br><br>Use this to create a copy of your project or save to a different folder."));
    connect(m_actionSaveAs, &QAction::triggered, this, &MenuBar::saveProjectAsRequested);
    m_fileMenu->addAction(m_actionSaveAs);

    m_fileMenu->addSeparator();

    // Import submenu
    QMenu* importMenu = m_fileMenu->addMenu(tr("&Import"));
    
    m_actionImportMesh = createAction(tr("Mesh (STL, OBJ, PLY)..."), "Ctrl+I", tr("Import mesh from STL, OBJ, or PLY file"));
    m_actionImportMesh->setWhatsThis(HelpText::importMesh());
    connect(m_actionImportMesh, &QAction::triggered, this, &MenuBar::importMeshRequested);
    importMenu->addAction(m_actionImportMesh);
    
    m_actionImportCAD = createAction(tr("CAD (STEP, IGES)..."), "Ctrl+Shift+I", tr("Import CAD geometry from STEP or IGES"));
    m_actionImportCAD->setWhatsThis(tr("<b>Import CAD</b><br><br>Import CAD geometry from STEP or IGES files.<br><br>Use this to bring in reference geometry or existing CAD models for comparison with scan data."));
    connect(m_actionImportCAD, &QAction::triggered, this, &MenuBar::importCADRequested);
    importMenu->addAction(m_actionImportCAD);

    // Export submenu
    QMenu* exportMenu = m_fileMenu->addMenu(tr("&Export"));
    
    m_actionExportMesh = createAction(tr("Mesh (STL)..."), "Ctrl+E", tr("Export selected mesh to STL file"));
    m_actionExportMesh->setWhatsThis(HelpText::exportMesh());
    connect(m_actionExportMesh, &QAction::triggered, this, &MenuBar::exportMeshRequested);
    exportMenu->addAction(m_actionExportMesh);
    
    m_actionExportSTEP = createAction(tr("CAD (STEP)..."), "", tr("Export surfaces to STEP CAD file"));
    m_actionExportSTEP->setWhatsThis(tr("<b>Export STEP</b><br><br>Export surfaces and bodies to STEP format.<br><br>STEP is the most widely supported CAD exchange format, compatible with SolidWorks, CATIA, NX, and most CAD systems."));
    connect(m_actionExportSTEP, &QAction::triggered, this, &MenuBar::exportSTEPRequested);
    exportMenu->addAction(m_actionExportSTEP);
    
    m_actionExportIGES = createAction(tr("CAD (IGES)..."), "", tr("Export surfaces to IGES CAD file"));
    m_actionExportIGES->setWhatsThis(tr("<b>Export IGES</b><br><br>Export surfaces to IGES format.<br><br>IGES is an older CAD format with good compatibility for surface data."));
    connect(m_actionExportIGES, &QAction::triggered, this, &MenuBar::exportIGESRequested);
    exportMenu->addAction(m_actionExportIGES);

    m_fileMenu->addSeparator();

    // Project Settings
    m_actionProjectSettings = createAction(tr("Project Se&ttings..."), "", tr("Configure project settings"));
    m_fileMenu->addAction(m_actionProjectSettings);

    m_fileMenu->addSeparator();

    // Exit
    m_actionExit = createAction(tr("E&xit"), "Alt+F4", tr("Exit the application"));
    connect(m_actionExit, &QAction::triggered, this, &MenuBar::exitRequested);
    connect(m_actionExit, &QAction::triggered, qApp, &QApplication::quit);
    m_fileMenu->addAction(m_actionExit);
}

void MenuBar::setupEditMenu()
{
    m_editMenu = addMenu(tr("&Edit"));

    // Undo
    m_actionUndo = createAction(tr("&Undo"), "Ctrl+Z", tr("Undo the last action"));
    m_actionUndo->setEnabled(false);  // Disabled until commands are executed
    connect(m_actionUndo, &QAction::triggered, this, &MenuBar::undoRequested);
    m_editMenu->addAction(m_actionUndo);

    // Redo
    m_actionRedo = createAction(tr("&Redo"), "Ctrl+Y", tr("Redo the last undone action"));
    m_actionRedo->setEnabled(false);  // Disabled until undo is performed
    connect(m_actionRedo, &QAction::triggered, this, &MenuBar::redoRequested);
    m_editMenu->addAction(m_actionRedo);
    
    // Undo History
    QAction* undoHistoryAction = createAction(tr("Undo &History..."), "Ctrl+Shift+Z", 
        tr("View and navigate undo/redo history"));
    connect(undoHistoryAction, &QAction::triggered, this, &MenuBar::undoHistoryRequested);
    m_editMenu->addAction(undoHistoryAction);

    m_editMenu->addSeparator();

    // Cut
    m_actionCut = createAction(tr("Cu&t"), "Ctrl+X", tr("Cut selected objects"));
    connect(m_actionCut, &QAction::triggered, this, &MenuBar::cutRequested);
    m_editMenu->addAction(m_actionCut);

    // Copy
    m_actionCopy = createAction(tr("&Copy"), "Ctrl+C", tr("Copy selected objects"));
    connect(m_actionCopy, &QAction::triggered, this, &MenuBar::copyRequested);
    m_editMenu->addAction(m_actionCopy);

    // Paste
    m_actionPaste = createAction(tr("&Paste"), "Ctrl+V", tr("Paste objects from clipboard"));
    connect(m_actionPaste, &QAction::triggered, this, &MenuBar::pasteRequested);
    m_editMenu->addAction(m_actionPaste);

    // Duplicate - create a copy in place
    m_actionDuplicate = createAction(tr("D&uplicate"), "Ctrl+D", tr("Create a copy of selected objects"));
    connect(m_actionDuplicate, &QAction::triggered, this, &MenuBar::duplicateRequested);
    m_editMenu->addAction(m_actionDuplicate);

    // Delete
    m_actionDelete = createAction(tr("&Delete"), "Delete", tr("Delete selected objects"));
    connect(m_actionDelete, &QAction::triggered, this, &MenuBar::deleteRequested);
    m_editMenu->addAction(m_actionDelete);

    m_editMenu->addSeparator();

    // Select All
    m_actionSelectAll = createAction(tr("Select &All"), "Ctrl+A", tr("Select all objects"));
    connect(m_actionSelectAll, &QAction::triggered, this, &MenuBar::selectAllRequested);
    m_editMenu->addAction(m_actionSelectAll);

    // Deselect All
    m_actionDeselectAll = createAction(tr("D&eselect All"), "Escape", tr("Deselect all objects"));
    connect(m_actionDeselectAll, &QAction::triggered, this, &MenuBar::deselectAllRequested);
    m_editMenu->addAction(m_actionDeselectAll);

    // Invert Selection
    m_actionInvertSelection = createAction(tr("&Invert Selection"), "Ctrl+I", tr("Invert the current selection"));
    connect(m_actionInvertSelection, &QAction::triggered, this, &MenuBar::invertSelectionRequested);
    m_editMenu->addAction(m_actionInvertSelection);

    m_editMenu->addSeparator();
    
    // Export Presets
    m_actionExportPresets = createAction(tr("Export &Presets..."), "", 
        tr("Manage export presets"));
    m_actionExportPresets->setWhatsThis(tr("<b>Export Presets</b><br><br>Create, edit, and manage export presets for quick access to common export configurations.<br><br>Set a default preset for Quick Export (Ctrl+Shift+E)."));
    connect(m_actionExportPresets, &QAction::triggered, this, [this]() {
        ExportPresetsDialog dialog(window());
        dialog.exec();
    });
    m_editMenu->addAction(m_actionExportPresets);

    m_editMenu->addSeparator();

    // Preferences
    m_actionPreferences = createAction(tr("Pre&ferences..."), "Ctrl+,", tr("Open application preferences"));
    connect(m_actionPreferences, &QAction::triggered, this, &MenuBar::preferencesRequested);
    m_editMenu->addAction(m_actionPreferences);
}

void MenuBar::setupViewMenu()
{
    m_viewMenu = addMenu(tr("&View"));

    // Standard Views submenu
    QMenu* standardViewsMenu = m_viewMenu->addMenu(tr("&Standard Views"));
    
    m_actionViewFront = createAction(tr("&Front"), "1", tr("View from front"));
    connect(m_actionViewFront, &QAction::triggered, this, &MenuBar::viewFrontRequested);
    standardViewsMenu->addAction(m_actionViewFront);
    
    m_actionViewBack = createAction(tr("&Back"), "Ctrl+1", tr("View from back"));
    connect(m_actionViewBack, &QAction::triggered, this, &MenuBar::viewBackRequested);
    standardViewsMenu->addAction(m_actionViewBack);
    
    m_actionViewLeft = createAction(tr("&Left"), "3", tr("View from left"));
    connect(m_actionViewLeft, &QAction::triggered, this, &MenuBar::viewLeftRequested);
    standardViewsMenu->addAction(m_actionViewLeft);
    
    m_actionViewRight = createAction(tr("&Right"), "Ctrl+3", tr("View from right"));
    connect(m_actionViewRight, &QAction::triggered, this, &MenuBar::viewRightRequested);
    standardViewsMenu->addAction(m_actionViewRight);
    
    m_actionViewTop = createAction(tr("&Top"), "7", tr("View from top"));
    connect(m_actionViewTop, &QAction::triggered, this, &MenuBar::viewTopRequested);
    standardViewsMenu->addAction(m_actionViewTop);
    
    m_actionViewBottom = createAction(tr("Botto&m"), "Ctrl+7", tr("View from bottom"));
    connect(m_actionViewBottom, &QAction::triggered, this, &MenuBar::viewBottomRequested);
    standardViewsMenu->addAction(m_actionViewBottom);
    
    m_actionViewIsometric = createAction(tr("&Isometric"), "0", tr("Isometric view"));
    connect(m_actionViewIsometric, &QAction::triggered, this, &MenuBar::viewIsometricRequested);
    standardViewsMenu->addAction(m_actionViewIsometric);

    m_viewMenu->addSeparator();

    // Zoom
    m_actionZoomToFit = createAction(tr("Zoom to &Fit"), "F", tr("Fit all objects in view"));
    connect(m_actionZoomToFit, &QAction::triggered, this, &MenuBar::zoomToFitRequested);
    m_viewMenu->addAction(m_actionZoomToFit);
    
    m_actionZoomToSelection = createAction(tr("Zoom to &Selection"), "Z", tr("Fit selected objects in view"));
    connect(m_actionZoomToSelection, &QAction::triggered, this, &MenuBar::zoomToSelectionRequested);
    m_viewMenu->addAction(m_actionZoomToSelection);

    m_viewMenu->addSeparator();

    // Display Mode submenu
    QMenu* displayModeMenu = m_viewMenu->addMenu(tr("&Display Mode"));
    
    QActionGroup* displayModeGroup = new QActionGroup(this);
    
    m_actionDisplayShaded = createAction(tr("&Shaded"), "Alt+1", tr("Solid shaded view with lighting"));
    m_actionDisplayShaded->setCheckable(true);
    m_actionDisplayShaded->setChecked(true);
    displayModeGroup->addAction(m_actionDisplayShaded);
    connect(m_actionDisplayShaded, &QAction::triggered, this, &MenuBar::displayModeShadedRequested);
    displayModeMenu->addAction(m_actionDisplayShaded);
    
    m_actionDisplayWireframe = createAction(tr("&Wireframe"), "Alt+2", tr("Show mesh edges only"));
    m_actionDisplayWireframe->setCheckable(true);
    displayModeGroup->addAction(m_actionDisplayWireframe);
    connect(m_actionDisplayWireframe, &QAction::triggered, this, &MenuBar::displayModeWireframeRequested);
    displayModeMenu->addAction(m_actionDisplayWireframe);
    
    m_actionDisplayShadedWire = createAction(tr("Shaded + &Wireframe"), "Alt+3", tr("Shaded with wireframe overlay"));
    m_actionDisplayShadedWire->setCheckable(true);
    displayModeGroup->addAction(m_actionDisplayShadedWire);
    connect(m_actionDisplayShadedWire, &QAction::triggered, this, &MenuBar::displayModeShadedWireRequested);
    displayModeMenu->addAction(m_actionDisplayShadedWire);
    
    m_actionDisplayXRay = createAction(tr("&X-Ray"), "Alt+4", tr("Transparent view to see through surfaces"));
    m_actionDisplayXRay->setCheckable(true);
    displayModeGroup->addAction(m_actionDisplayXRay);
    connect(m_actionDisplayXRay, &QAction::triggered, this, &MenuBar::displayModeXRayRequested);
    displayModeMenu->addAction(m_actionDisplayXRay);
    
    m_actionDisplayDeviation = createAction(tr("&Deviation Map"), "Alt+5", tr("Color map showing deviation from reference"));
    m_actionDisplayDeviation->setCheckable(true);
    displayModeGroup->addAction(m_actionDisplayDeviation);
    connect(m_actionDisplayDeviation, &QAction::triggered, this, &MenuBar::displayModeDeviationRequested);
    displayModeMenu->addAction(m_actionDisplayDeviation);

    m_viewMenu->addSeparator();

    // Toggle options
    m_actionToggleGrid = createAction(tr("Show &Grid"), "G", tr("Toggle grid display"));
    m_actionToggleGrid->setCheckable(true);
    m_actionToggleGrid->setChecked(true);
    connect(m_actionToggleGrid, &QAction::triggered, this, &MenuBar::toggleGridRequested);
    m_viewMenu->addAction(m_actionToggleGrid);
    
    m_actionToggleAxes = createAction(tr("Show &Axes"), "", tr("Toggle coordinate axes display"));
    m_actionToggleAxes->setCheckable(true);
    m_actionToggleAxes->setChecked(true);
    connect(m_actionToggleAxes, &QAction::triggered, this, &MenuBar::toggleAxesRequested);
    m_viewMenu->addAction(m_actionToggleAxes);
    
    m_actionToggleViewCube = createAction(tr("Show View&Cube"), "", tr("Toggle ViewCube display"));
    m_actionToggleViewCube->setCheckable(true);
    m_actionToggleViewCube->setChecked(true);
    connect(m_actionToggleViewCube, &QAction::triggered, this, &MenuBar::toggleViewCubeRequested);
    m_viewMenu->addAction(m_actionToggleViewCube);

    m_viewMenu->addSeparator();

    // Panel visibility
    m_actionObjectBrowser = createAction(tr("&Object Browser Panel"), "F2", tr("Toggle Object Browser panel"));
    m_actionObjectBrowser->setCheckable(true);
    m_actionObjectBrowser->setChecked(true);
    connect(m_actionObjectBrowser, &QAction::triggered, this, &MenuBar::toggleObjectBrowserRequested);
    m_viewMenu->addAction(m_actionObjectBrowser);
    
    m_actionPropertiesPanel = createAction(tr("&Properties Panel"), "F3", tr("Toggle Properties panel"));
    m_actionPropertiesPanel->setCheckable(true);
    m_actionPropertiesPanel->setChecked(true);
    connect(m_actionPropertiesPanel, &QAction::triggered, this, &MenuBar::togglePropertiesPanelRequested);
    m_viewMenu->addAction(m_actionPropertiesPanel);

    m_viewMenu->addSeparator();

    // Full Screen
    m_actionFullScreen = createAction(tr("&Full Screen"), "F11", tr("Toggle full screen mode"));
    m_actionFullScreen->setCheckable(true);
    connect(m_actionFullScreen, &QAction::triggered, this, &MenuBar::fullScreenRequested);
    m_viewMenu->addAction(m_actionFullScreen);
}

void MenuBar::setupMeshMenu()
{
    m_meshMenu = addMenu(tr("&Mesh"));

    // Repair Wizard - one-click mesh repair for beginners
    m_actionMeshRepairWizard = createAction(tr("&Repair Wizard..."), "Ctrl+Shift+W", tr("One-click mesh repair wizard"));
    m_actionMeshRepairWizard->setWhatsThis(
        tr("<b>Mesh Repair Wizard</b><br><br>"
           "Automatically detect and fix common mesh problems with one click.<br><br>"
           "Fixes holes, non-manifold geometry, degenerate faces, and more. "
           "Perfect for cleaning up scanned meshes."));
    connect(m_actionMeshRepairWizard, &QAction::triggered, this, &MenuBar::showMeshRepairWizard);
    m_meshMenu->addAction(m_actionMeshRepairWizard);

    m_meshMenu->addSeparator();

    // Polygon Reduction
    m_actionPolygonReduction = createAction(tr("&Polygon Reduction..."), "Ctrl+Shift+R", tr("Reduce polygon count"));
    m_actionPolygonReduction->setWhatsThis(HelpText::polygonReduction());
    connect(m_actionPolygonReduction, &QAction::triggered, this, &MenuBar::showPolygonReductionDialog);
    m_meshMenu->addAction(m_actionPolygonReduction);

    // Smoothing - changed from Ctrl+Shift+S (conflicts with Save As) to Ctrl+Shift+M
    m_actionSmoothing = createAction(tr("&Smoothing..."), "Ctrl+Shift+M", tr("Smooth mesh to reduce noise and bumps"));
    m_actionSmoothing->setWhatsThis(HelpText::smoothing());
    connect(m_actionSmoothing, &QAction::triggered, this, &MenuBar::showSmoothingDialog);
    m_meshMenu->addAction(m_actionSmoothing);

    m_meshMenu->addSeparator();

    // Fill Holes
    m_actionFillHoles = createAction(tr("&Fill Holes..."), "Ctrl+Shift+H", tr("Fill holes in mesh"));
    m_actionFillHoles->setWhatsThis(HelpText::fillHoles());
    connect(m_actionFillHoles, &QAction::triggered, this, &MenuBar::showHoleFillDialog);
    m_meshMenu->addAction(m_actionFillHoles);

    // Remove Outliers
    m_actionRemoveOutliers = createAction(tr("&Remove Outliers..."), "", tr("Remove outlier vertices"));
    m_actionRemoveOutliers->setWhatsThis(HelpText::removeOutliers());
    connect(m_actionRemoveOutliers, &QAction::triggered, this, &MenuBar::showOutlierRemovalDialog);
    m_meshMenu->addAction(m_actionRemoveOutliers);

    // De-feature
    QAction* deFeature = createAction(tr("&De-feature..."), "", tr("Remove small features"));
    deFeature->setWhatsThis(tr("<b>De-feature</b><br><br>Removes small features from the mesh such as small bumps, indentations, or noise.<br><br>Useful for simplifying scan data before surface fitting."));
    connect(deFeature, &QAction::triggered, this, &MenuBar::deFeatureRequested);
    m_meshMenu->addAction(deFeature);

    m_meshMenu->addSeparator();

    // Clipping Box
    m_actionClippingBox = createAction(tr("&Clipping Box..."), "Ctrl+Shift+B", tr("Enable clipping box"));
    m_actionClippingBox->setWhatsThis(HelpText::clippingBox());
    connect(m_actionClippingBox, &QAction::triggered, this, &MenuBar::showClippingBoxDialog);
    m_meshMenu->addAction(m_actionClippingBox);

    // Split Mesh
    QAction* splitMesh = createAction(tr("Spl&it Mesh"), "", tr("Split mesh into parts"));
    splitMesh->setWhatsThis(tr("<b>Split Mesh</b><br><br>Separates a mesh into multiple parts based on connectivity.<br><br>Each disconnected region becomes a separate mesh object."));
    connect(splitMesh, &QAction::triggered, this, &MenuBar::splitMeshRequested);
    m_meshMenu->addAction(splitMesh);

    // Merge Meshes
    QAction* mergeMeshes = createAction(tr("&Merge Meshes"), "", tr("Merge multiple meshes"));
    mergeMeshes->setWhatsThis(tr("<b>Merge Meshes</b><br><br>Combines multiple selected meshes into a single mesh object.<br><br>Select two or more meshes in the Object Browser, then use this command."));
    connect(mergeMeshes, &QAction::triggered, this, &MenuBar::mergeMeshesRequested);
    m_meshMenu->addAction(mergeMeshes);
}

void MenuBar::setupCreateMenu()
{
    m_createMenu = addMenu(tr("&Create"));

    // Primitives submenu
    QMenu* primitivesMenu = m_createMenu->addMenu(tr("&Primitives"));
    
    // Cube - most common primitive first
    QAction* cube = createAction(tr("Cu&be"), "B", tr("Create a cube"));
    cube->setWhatsThis(tr("<b>Create Cube</b><br><br>Creates a cube (box) primitive.<br><br>Use size presets or specify exact dimensions."));
    connect(cube, &QAction::triggered, this, &MenuBar::createCubeRequested);
    primitivesMenu->addAction(cube);
    
    QAction* sphere = createAction(tr("&Sphere"), "", tr("Create a sphere"));
    sphere->setWhatsThis(tr("<b>Create Sphere</b><br><br>Creates a sphere primitive.<br><br>Use size presets or specify radius and resolution."));
    connect(sphere, &QAction::triggered, this, &MenuBar::createSphereRequested);
    primitivesMenu->addAction(sphere);
    
    m_actionCreateCylinder = createAction(tr("&Cylinder"), "C", tr("Create a cylinder"));
    m_actionCreateCylinder->setWhatsThis(HelpText::createCylinder());
    connect(m_actionCreateCylinder, &QAction::triggered, this, &MenuBar::createCylinderRequested);
    primitivesMenu->addAction(m_actionCreateCylinder);
    
    QAction* cone = createAction(tr("C&one"), "", tr("Create a cone"));
    cone->setWhatsThis(tr("<b>Create Cone</b><br><br>Creates a cone primitive.<br><br>Specify base radius and height."));
    connect(cone, &QAction::triggered, this, &MenuBar::createConeRequested);
    primitivesMenu->addAction(cone);
    
    m_actionCreatePlane = createAction(tr("&Plane"), "P", tr("Create a reference plane"));
    m_actionCreatePlane->setWhatsThis(HelpText::createPlane());
    connect(m_actionCreatePlane, &QAction::triggered, this, &MenuBar::createPlaneRequested);
    primitivesMenu->addAction(m_actionCreatePlane);
    
    primitivesMenu->addSeparator();
    
    QAction* torus = createAction(tr("&Torus"), "", tr("Create a torus (donut shape)"));
    torus->setWhatsThis(tr("<b>Create Torus</b><br><br>Creates a torus (donut) primitive.<br><br>Specify major radius (ring) and minor radius (tube)."));
    connect(torus, &QAction::triggered, this, &MenuBar::createTorusRequested);
    primitivesMenu->addAction(torus);

    m_createMenu->addSeparator();

    // Section
    m_actionSection2D = createAction(tr("&Section Plane..."), "S", tr("Create a section plane"));
    m_actionSection2D->setWhatsThis(HelpText::sectionPlane());
    connect(m_actionSection2D, &QAction::triggered, this, &MenuBar::sectionPlaneRequested);
    m_createMenu->addAction(m_actionSection2D);
    
    QAction* multipleSections = createAction(tr("&Multiple Sections..."), "", tr("Create multiple section planes"));
    multipleSections->setWhatsThis(tr("<b>Multiple Sections</b><br><br>Creates a series of parallel section planes at regular intervals.<br><br>Great for creating multiple cross-section profiles at once."));
    connect(multipleSections, &QAction::triggered, this, &MenuBar::multipleSectionsRequested);
    m_createMenu->addAction(multipleSections);

    m_createMenu->addSeparator();

    // Sketch submenu
    QMenu* sketchMenu = m_createMenu->addMenu(tr("S&ketch"));
    
    m_actionSketch2D = createAction(tr("&2D Sketch"), "K", tr("Create a 2D sketch"));
    m_actionSketch2D->setWhatsThis(HelpText::sketch2D());
    connect(m_actionSketch2D, &QAction::triggered, this, &MenuBar::sketch2DRequested);
    sketchMenu->addAction(m_actionSketch2D);
    
    QAction* sketch3D = createAction(tr("&3D Sketch"), "", tr("Create a 3D sketch"));
    sketch3D->setWhatsThis(tr("<b>3D Sketch</b><br><br>Create a sketch directly in 3D space, not constrained to a plane.<br><br>Useful for 3D paths, sweep trajectories, and space curves."));
    connect(sketch3D, &QAction::triggered, this, &MenuBar::sketch3DRequested);
    sketchMenu->addAction(sketch3D);

    m_createMenu->addSeparator();

    // Surface submenu
    QMenu* surfaceMenu = m_createMenu->addMenu(tr("S&urface"));
    
    QAction* fitSurface = createAction(tr("&Fit Surface..."), "", tr("Fit surface to selection"));
    fitSurface->setWhatsThis(tr("<b>Fit Surface</b><br><br>Fits an analytical surface (plane, cylinder, sphere, cone) to the selected mesh region.<br><br>The algorithm automatically determines the best-fit surface type and parameters."));
    connect(fitSurface, &QAction::triggered, this, &MenuBar::fitSurfaceRequested);
    surfaceMenu->addAction(fitSurface);
    
    QAction* autoSurface = createAction(tr("&Auto Surface..."), "", tr("Automatically create surfaces"));
    autoSurface->setWhatsThis(tr("<b>Auto Surface</b><br><br>Automatically segments the mesh into regions and fits surfaces to each region.<br><br>A fast way to convert mesh data to CAD surfaces."));
    connect(autoSurface, &QAction::triggered, this, &MenuBar::autoSurfaceRequested);
    surfaceMenu->addAction(autoSurface);
    
    surfaceMenu->addSeparator();
    
    m_actionExtrude = createAction(tr("&Extrude..."), "E", tr("Extrude sketch or face"));
    m_actionExtrude->setWhatsThis(HelpText::extrude());
    connect(m_actionExtrude, &QAction::triggered, this, &MenuBar::extrudeRequested);
    surfaceMenu->addAction(m_actionExtrude);
    
    m_actionRevolve = createAction(tr("&Revolve..."), "R", tr("Revolve sketch around axis"));
    m_actionRevolve->setWhatsThis(HelpText::revolve());
    connect(m_actionRevolve, &QAction::triggered, this, &MenuBar::revolveRequested);
    surfaceMenu->addAction(m_actionRevolve);
    
    QAction* loft = createAction(tr("&Loft..."), "", tr("Create lofted surface"));
    loft->setWhatsThis(tr("<b>Loft</b><br><br>Creates a smooth surface connecting multiple profile sketches.<br><br>Select two or more sketches, and Loft will create a surface that transitions between them."));
    connect(loft, &QAction::triggered, this, &MenuBar::loftRequested);
    surfaceMenu->addAction(loft);
    
    QAction* sweep = createAction(tr("&Sweep..."), "", tr("Create swept surface"));
    sweep->setWhatsThis(tr("<b>Sweep</b><br><br>Creates a surface by sweeping a profile sketch along a path.<br><br>Select a profile sketch and a path curve to create the sweep."));
    connect(sweep, &QAction::triggered, this, &MenuBar::sweepRequested);
    surfaceMenu->addAction(sweep);
    
    surfaceMenu->addSeparator();
    
    QAction* freeformSurface = createAction(tr("Free-&form Surface..."), "", tr("Create free-form surface"));
    freeformSurface->setWhatsThis(tr("<b>Free-form Surface</b><br><br>Creates a NURBS surface with control points for direct manipulation.<br><br>Great for organic shapes that can't be created with analytical surfaces."));
    connect(freeformSurface, &QAction::triggered, this, &MenuBar::freeformSurfaceRequested);
    surfaceMenu->addAction(freeformSurface);
}

void MenuBar::setupHelpMenu()
{
    m_helpMenu = addMenu(tr("&Help"));

    // What's This mode
    QAction* whatsThis = createAction(tr("&What's This?"), "Shift+F1", 
        tr("Click on any button or control to see help about it"));
    whatsThis->setWhatsThis(tr("Enter What's This mode. Click on any UI element to see detailed help about what it does."));
    connect(whatsThis, &QAction::triggered, this, []() {
        QWhatsThis::enterWhatsThisMode();
    });
    m_helpMenu->addAction(whatsThis);

    m_helpMenu->addSeparator();

    // Getting Started
    QAction* gettingStarted = createAction(tr("&Getting Started..."), "", 
        tr("Quick tutorial to learn the basics"));
    gettingStarted->setWhatsThis(HelpText::newProject());
    connect(gettingStarted, &QAction::triggered, this, [this]() {
        GettingStartedDialog dialog(window());
        dialog.exec();
    });
    m_helpMenu->addAction(gettingStarted);

    // Keyboard Shortcuts
    QAction* shortcuts = createAction(tr("&Keyboard Shortcuts..."), "F1", 
        tr("View all keyboard shortcuts"));
    shortcuts->setWhatsThis(tr("Opens a searchable list of all keyboard shortcuts in the application."));
    connect(shortcuts, &QAction::triggered, this, [this]() {
        KeyboardShortcutsDialog* dialog = new KeyboardShortcutsDialog(window());
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
    m_helpMenu->addAction(shortcuts);

    m_helpMenu->addSeparator();

    // Documentation (opens web/local docs)
    QAction* documentation = createAction(tr("&Documentation"), "", 
        tr("Open online documentation"));
    connect(documentation, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/dc-3ddesignapp/docs"));
    });
    m_helpMenu->addAction(documentation);

    // Release Notes
    QAction* releaseNotes = createAction(tr("&Release Notes"), "", 
        tr("View what's new in this version"));
    connect(releaseNotes, &QAction::triggered, this, &MenuBar::releaseNotesRequested);
    m_helpMenu->addAction(releaseNotes);

    m_helpMenu->addSeparator();

    // Check for Updates
    QAction* checkUpdates = createAction(tr("Check for &Updates..."), "", 
        tr("Check if a newer version is available"));
    connect(checkUpdates, &QAction::triggered, this, &MenuBar::checkForUpdatesRequested);
    m_helpMenu->addAction(checkUpdates);

    m_helpMenu->addSeparator();

    // About
    QAction* about = createAction(tr("&About..."), "", 
        tr("About dc-3ddesignapp - version and credits"));
    connect(about, &QAction::triggered, this, [this]() {
        AboutDialog dialog(window());
        dialog.exec();
    });
    m_helpMenu->addAction(about);
}

void MenuBar::updateRecentFiles(const QStringList& files)
{
    m_recentFilesMenu->clear();
    
    if (files.isEmpty()) {
        QAction* noRecent = m_recentFilesMenu->addAction(tr("No Recent Files"));
        noRecent->setEnabled(false);
        return;
    }
    
    for (const QString& file : files) {
        QAction* action = m_recentFilesMenu->addAction(file);
        connect(action, &QAction::triggered, this, [this, file]() {
            emit recentFileRequested(file);
        });
    }
    
    m_recentFilesMenu->addSeparator();
    QAction* clearRecent = m_recentFilesMenu->addAction(tr("Clear Recent Files"));
    connect(clearRecent, &QAction::triggered, this, [this]() {
        m_recentFilesMenu->clear();
        QAction* noRecent = m_recentFilesMenu->addAction(tr("No Recent Files"));
        noRecent->setEnabled(false);
    });
}

void MenuBar::connectToCommandStack(dc3d::core::CommandStack* commandStack)
{
    if (!commandStack) {
        return;
    }
    
    // Connect undo action to command stack
    connect(m_actionUndo, &QAction::triggered, commandStack, &dc3d::core::CommandStack::undo);
    connect(m_actionRedo, &QAction::triggered, commandStack, &dc3d::core::CommandStack::redo);
    
    // Update enabled state when stack changes
    connect(commandStack, &dc3d::core::CommandStack::canUndoChanged, this, [this](bool canUndo) {
        m_actionUndo->setEnabled(canUndo);
    });
    
    connect(commandStack, &dc3d::core::CommandStack::canRedoChanged, this, [this](bool canRedo) {
        m_actionRedo->setEnabled(canRedo);
    });
    
    // Update menu text with command description
    connect(commandStack, &dc3d::core::CommandStack::undoTextChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            m_actionUndo->setText(tr("&Undo"));
        } else {
            m_actionUndo->setText(tr("&Undo %1").arg(text));
        }
    });
    
    connect(commandStack, &dc3d::core::CommandStack::redoTextChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            m_actionRedo->setText(tr("&Redo"));
        } else {
            m_actionRedo->setText(tr("&Redo %1").arg(text));
        }
    });
    
    // Set initial state
    m_actionUndo->setEnabled(commandStack->canUndo());
    m_actionRedo->setEnabled(commandStack->canRedo());
    
    // Set initial text
    QString undoText = commandStack->undoText();
    if (!undoText.isEmpty()) {
        m_actionUndo->setText(tr("&Undo %1").arg(undoText));
    }
    
    QString redoText = commandStack->redoText();
    if (!redoText.isEmpty()) {
        m_actionRedo->setText(tr("&Redo %1").arg(redoText));
    }
}

void MenuBar::connectToUndoStack(QUndoStack* undoStack)
{
    if (!undoStack) {
        return;
    }
    
    // Connect undo action to undo stack
    connect(m_actionUndo, &QAction::triggered, undoStack, &QUndoStack::undo);
    connect(m_actionRedo, &QAction::triggered, undoStack, &QUndoStack::redo);
    
    // Update enabled state when stack changes
    connect(undoStack, &QUndoStack::canUndoChanged, this, [this](bool canUndo) {
        m_actionUndo->setEnabled(canUndo);
    });
    
    connect(undoStack, &QUndoStack::canRedoChanged, this, [this](bool canRedo) {
        m_actionRedo->setEnabled(canRedo);
    });
    
    // Update menu text with command description
    connect(undoStack, &QUndoStack::undoTextChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            m_actionUndo->setText(tr("&Undo"));
        } else {
            m_actionUndo->setText(tr("&Undo %1").arg(text));
        }
    });
    
    connect(undoStack, &QUndoStack::redoTextChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            m_actionRedo->setText(tr("&Redo"));
        } else {
            m_actionRedo->setText(tr("&Redo %1").arg(text));
        }
    });
    
    // Set initial state
    m_actionUndo->setEnabled(undoStack->canUndo());
    m_actionRedo->setEnabled(undoStack->canRedo());
    
    // Set initial text
    QString undoText = undoStack->undoText();
    if (!undoText.isEmpty()) {
        m_actionUndo->setText(tr("&Undo %1").arg(undoText));
    }
    
    QString redoText = undoStack->redoText();
    if (!redoText.isEmpty()) {
        m_actionRedo->setText(tr("&Redo %1").arg(redoText));
    }
}

void MenuBar::setUndoEnabled(bool canUndo, const QString& text)
{
    m_actionUndo->setEnabled(canUndo);
    if (text.isEmpty()) {
        m_actionUndo->setText(tr("&Undo"));
    } else {
        m_actionUndo->setText(tr("&Undo %1").arg(text));
    }
}

void MenuBar::setRedoEnabled(bool canRedo, const QString& text)
{
    m_actionRedo->setEnabled(canRedo);
    if (text.isEmpty()) {
        m_actionRedo->setText(tr("&Redo"));
    } else {
        m_actionRedo->setText(tr("&Redo %1").arg(text));
    }
}

void MenuBar::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
    
    // Set viewport on all dialogs
    if (m_polygonReductionDialog) {
        m_polygonReductionDialog->setViewport(viewport);
    }
    if (m_smoothingDialog) {
        m_smoothingDialog->setViewport(viewport);
    }
    if (m_holeFillDialog) {
        m_holeFillDialog->setViewport(viewport);
    }
    if (m_outlierRemovalDialog) {
        m_outlierRemovalDialog->setViewport(viewport);
    }
    if (m_clippingBoxDialog) {
        m_clippingBoxDialog->setViewport(viewport);
    }
}

void MenuBar::createMeshDialogs()
{
    // Create all mesh dialogs (lazy initialization would be better for memory,
    // but we create them now for simplicity)
    QWidget* parentWidget = window();
    if (!parentWidget) {
        parentWidget = this;  // Fallback to MenuBar as parent if window() is null
    }
    
    m_polygonReductionDialog = new PolygonReductionDialog(parentWidget);
    m_smoothingDialog = new SmoothingDialog(parentWidget);
    m_holeFillDialog = new HoleFillDialog(parentWidget);
    m_outlierRemovalDialog = new OutlierRemovalDialog(parentWidget);
    m_clippingBoxDialog = new ClippingBoxDialog(parentWidget);
}

void MenuBar::showPolygonReductionDialog()
{
    if (m_polygonReductionDialog) {
        m_polygonReductionDialog->show();
        m_polygonReductionDialog->raise();
        m_polygonReductionDialog->activateWindow();
    }
}

void MenuBar::showSmoothingDialog()
{
    if (m_smoothingDialog) {
        m_smoothingDialog->show();
        m_smoothingDialog->raise();
        m_smoothingDialog->activateWindow();
    }
}

void MenuBar::showHoleFillDialog()
{
    if (m_holeFillDialog) {
        m_holeFillDialog->show();
        m_holeFillDialog->raise();
        m_holeFillDialog->activateWindow();
    }
}

void MenuBar::showOutlierRemovalDialog()
{
    if (m_outlierRemovalDialog) {
        m_outlierRemovalDialog->show();
        m_outlierRemovalDialog->raise();
        m_outlierRemovalDialog->activateWindow();
    }
}

void MenuBar::showClippingBoxDialog()
{
    if (m_clippingBoxDialog) {
        m_clippingBoxDialog->show();
        m_clippingBoxDialog->raise();
        m_clippingBoxDialog->activateWindow();
    }
}
