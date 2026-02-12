#include "MenuBar.h"
#include "../core/CommandStack.h"
#include "dialogs/PolygonReductionDialog.h"
#include "dialogs/SmoothingDialog.h"
#include "dialogs/HoleFillDialog.h"
#include "dialogs/OutlierRemovalDialog.h"
#include "dialogs/ClippingBoxDialog.h"
#include <QApplication>

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
    connect(m_actionNew, &QAction::triggered, this, &MenuBar::newProjectRequested);
    m_fileMenu->addAction(m_actionNew);

    // Open
    m_actionOpen = createAction(tr("&Open..."), "Ctrl+O", tr("Open an existing project"));
    connect(m_actionOpen, &QAction::triggered, this, &MenuBar::openProjectRequested);
    m_fileMenu->addAction(m_actionOpen);

    // Recent Files submenu
    m_recentFilesMenu = m_fileMenu->addMenu(tr("Open &Recent"));

    m_fileMenu->addSeparator();

    // Save
    m_actionSave = createAction(tr("&Save"), "Ctrl+S", tr("Save the current project"));
    connect(m_actionSave, &QAction::triggered, this, &MenuBar::saveProjectRequested);
    m_fileMenu->addAction(m_actionSave);

    // Save As
    m_actionSaveAs = createAction(tr("Save &As..."), "Ctrl+Shift+S", tr("Save project with a new name"));
    connect(m_actionSaveAs, &QAction::triggered, this, &MenuBar::saveProjectAsRequested);
    m_fileMenu->addAction(m_actionSaveAs);

    m_fileMenu->addSeparator();

    // Import submenu
    QMenu* importMenu = m_fileMenu->addMenu(tr("&Import"));
    
    m_actionImportMesh = createAction(tr("Mesh (STL, OBJ, PLY)..."), "", tr("Import a mesh file"));
    connect(m_actionImportMesh, &QAction::triggered, this, &MenuBar::importMeshRequested);
    importMenu->addAction(m_actionImportMesh);
    
    m_actionImportCAD = createAction(tr("CAD (STEP, IGES)..."), "", tr("Import a CAD file"));
    connect(m_actionImportCAD, &QAction::triggered, this, &MenuBar::importCADRequested);
    importMenu->addAction(m_actionImportCAD);

    // Export submenu
    QMenu* exportMenu = m_fileMenu->addMenu(tr("&Export"));
    
    m_actionExportMesh = createAction(tr("Mesh (STL)..."), "", tr("Export as STL mesh"));
    connect(m_actionExportMesh, &QAction::triggered, this, &MenuBar::exportMeshRequested);
    exportMenu->addAction(m_actionExportMesh);
    
    m_actionExportSTEP = createAction(tr("CAD (STEP)..."), "", tr("Export as STEP file"));
    connect(m_actionExportSTEP, &QAction::triggered, this, &MenuBar::exportSTEPRequested);
    exportMenu->addAction(m_actionExportSTEP);
    
    m_actionExportIGES = createAction(tr("CAD (IGES)..."), "", tr("Export as IGES file"));
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
    
    m_actionDisplayShaded = createAction(tr("&Shaded"), "1", tr("Shaded display mode"));
    m_actionDisplayShaded->setCheckable(true);
    m_actionDisplayShaded->setChecked(true);
    displayModeGroup->addAction(m_actionDisplayShaded);
    connect(m_actionDisplayShaded, &QAction::triggered, this, &MenuBar::displayModeShadedRequested);
    displayModeMenu->addAction(m_actionDisplayShaded);
    
    m_actionDisplayWireframe = createAction(tr("&Wireframe"), "2", tr("Wireframe display mode"));
    m_actionDisplayWireframe->setCheckable(true);
    displayModeGroup->addAction(m_actionDisplayWireframe);
    connect(m_actionDisplayWireframe, &QAction::triggered, this, &MenuBar::displayModeWireframeRequested);
    displayModeMenu->addAction(m_actionDisplayWireframe);
    
    m_actionDisplayShadedWire = createAction(tr("Shaded + &Wireframe"), "3", tr("Shaded with wireframe overlay"));
    m_actionDisplayShadedWire->setCheckable(true);
    displayModeGroup->addAction(m_actionDisplayShadedWire);
    connect(m_actionDisplayShadedWire, &QAction::triggered, this, &MenuBar::displayModeShadedWireRequested);
    displayModeMenu->addAction(m_actionDisplayShadedWire);
    
    m_actionDisplayXRay = createAction(tr("&X-Ray"), "4", tr("X-Ray transparent display mode"));
    m_actionDisplayXRay->setCheckable(true);
    displayModeGroup->addAction(m_actionDisplayXRay);
    connect(m_actionDisplayXRay, &QAction::triggered, this, &MenuBar::displayModeXRayRequested);
    displayModeMenu->addAction(m_actionDisplayXRay);
    
    m_actionDisplayDeviation = createAction(tr("&Deviation Map"), "5", tr("Show deviation color map"));
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

    // Polygon Reduction
    m_actionPolygonReduction = createAction(tr("&Polygon Reduction..."), "Ctrl+Shift+R", tr("Reduce polygon count"));
    connect(m_actionPolygonReduction, &QAction::triggered, this, &MenuBar::showPolygonReductionDialog);
    connect(m_actionPolygonReduction, &QAction::triggered, this, &MenuBar::polygonReductionRequested);
    m_meshMenu->addAction(m_actionPolygonReduction);

    // Smoothing
    m_actionSmoothing = createAction(tr("&Smoothing..."), "Ctrl+Shift+S", tr("Smooth mesh surface"));
    connect(m_actionSmoothing, &QAction::triggered, this, &MenuBar::showSmoothingDialog);
    connect(m_actionSmoothing, &QAction::triggered, this, &MenuBar::smoothingRequested);
    m_meshMenu->addAction(m_actionSmoothing);

    m_meshMenu->addSeparator();

    // Fill Holes
    m_actionFillHoles = createAction(tr("&Fill Holes..."), "Ctrl+Shift+H", tr("Fill holes in mesh"));
    connect(m_actionFillHoles, &QAction::triggered, this, &MenuBar::showHoleFillDialog);
    connect(m_actionFillHoles, &QAction::triggered, this, &MenuBar::fillHolesRequested);
    m_meshMenu->addAction(m_actionFillHoles);

    // Remove Outliers
    m_actionRemoveOutliers = createAction(tr("&Remove Outliers..."), "", tr("Remove outlier vertices"));
    connect(m_actionRemoveOutliers, &QAction::triggered, this, &MenuBar::showOutlierRemovalDialog);
    connect(m_actionRemoveOutliers, &QAction::triggered, this, &MenuBar::removeOutliersRequested);
    m_meshMenu->addAction(m_actionRemoveOutliers);

    // De-feature
    QAction* deFeature = createAction(tr("&De-feature..."), "", tr("Remove small features"));
    connect(deFeature, &QAction::triggered, this, &MenuBar::deFeatureRequested);
    m_meshMenu->addAction(deFeature);

    m_meshMenu->addSeparator();

    // Clipping Box
    m_actionClippingBox = createAction(tr("&Clipping Box..."), "Ctrl+Shift+B", tr("Enable clipping box"));
    connect(m_actionClippingBox, &QAction::triggered, this, &MenuBar::showClippingBoxDialog);
    connect(m_actionClippingBox, &QAction::triggered, this, &MenuBar::clippingBoxRequested);
    m_meshMenu->addAction(m_actionClippingBox);

    // Split Mesh
    QAction* splitMesh = createAction(tr("Spl&it Mesh"), "", tr("Split mesh into parts"));
    connect(splitMesh, &QAction::triggered, this, &MenuBar::splitMeshRequested);
    m_meshMenu->addAction(splitMesh);

    // Merge Meshes
    QAction* mergeMeshes = createAction(tr("&Merge Meshes"), "", tr("Merge multiple meshes"));
    connect(mergeMeshes, &QAction::triggered, this, &MenuBar::mergeMeshesRequested);
    m_meshMenu->addAction(mergeMeshes);
}

void MenuBar::setupCreateMenu()
{
    m_createMenu = addMenu(tr("&Create"));

    // Primitives submenu
    QMenu* primitivesMenu = m_createMenu->addMenu(tr("&Primitives"));
    
    m_actionCreatePlane = createAction(tr("&Plane"), "P", tr("Create a reference plane"));
    connect(m_actionCreatePlane, &QAction::triggered, this, &MenuBar::createPlaneRequested);
    primitivesMenu->addAction(m_actionCreatePlane);
    
    m_actionCreateCylinder = createAction(tr("&Cylinder"), "C", tr("Create a cylinder"));
    connect(m_actionCreateCylinder, &QAction::triggered, this, &MenuBar::createCylinderRequested);
    primitivesMenu->addAction(m_actionCreateCylinder);
    
    QAction* cone = createAction(tr("C&one"), "", tr("Create a cone"));
    connect(cone, &QAction::triggered, this, &MenuBar::createConeRequested);
    primitivesMenu->addAction(cone);
    
    QAction* sphere = createAction(tr("&Sphere"), "", tr("Create a sphere"));
    connect(sphere, &QAction::triggered, this, &MenuBar::createSphereRequested);
    primitivesMenu->addAction(sphere);

    m_createMenu->addSeparator();

    // Section
    m_actionSection2D = createAction(tr("&Section Plane..."), "S", tr("Create a section plane"));
    connect(m_actionSection2D, &QAction::triggered, this, &MenuBar::sectionPlaneRequested);
    m_createMenu->addAction(m_actionSection2D);
    
    QAction* multipleSections = createAction(tr("&Multiple Sections..."), "", tr("Create multiple section planes"));
    connect(multipleSections, &QAction::triggered, this, &MenuBar::multipleSectionsRequested);
    m_createMenu->addAction(multipleSections);

    m_createMenu->addSeparator();

    // Sketch submenu
    QMenu* sketchMenu = m_createMenu->addMenu(tr("S&ketch"));
    
    m_actionSketch2D = createAction(tr("&2D Sketch"), "K", tr("Create a 2D sketch"));
    connect(m_actionSketch2D, &QAction::triggered, this, &MenuBar::sketch2DRequested);
    sketchMenu->addAction(m_actionSketch2D);
    
    QAction* sketch3D = createAction(tr("&3D Sketch"), "", tr("Create a 3D sketch"));
    connect(sketch3D, &QAction::triggered, this, &MenuBar::sketch3DRequested);
    sketchMenu->addAction(sketch3D);

    m_createMenu->addSeparator();

    // Surface submenu
    QMenu* surfaceMenu = m_createMenu->addMenu(tr("S&urface"));
    
    QAction* fitSurface = createAction(tr("&Fit Surface..."), "", tr("Fit surface to selection"));
    connect(fitSurface, &QAction::triggered, this, &MenuBar::fitSurfaceRequested);
    surfaceMenu->addAction(fitSurface);
    
    QAction* autoSurface = createAction(tr("&Auto Surface..."), "", tr("Automatically create surfaces"));
    connect(autoSurface, &QAction::triggered, this, &MenuBar::autoSurfaceRequested);
    surfaceMenu->addAction(autoSurface);
    
    surfaceMenu->addSeparator();
    
    m_actionExtrude = createAction(tr("&Extrude..."), "E", tr("Extrude sketch or face"));
    connect(m_actionExtrude, &QAction::triggered, this, &MenuBar::extrudeRequested);
    surfaceMenu->addAction(m_actionExtrude);
    
    m_actionRevolve = createAction(tr("&Revolve..."), "R", tr("Revolve sketch around axis"));
    connect(m_actionRevolve, &QAction::triggered, this, &MenuBar::revolveRequested);
    surfaceMenu->addAction(m_actionRevolve);
    
    QAction* loft = createAction(tr("&Loft..."), "", tr("Create lofted surface"));
    connect(loft, &QAction::triggered, this, &MenuBar::loftRequested);
    surfaceMenu->addAction(loft);
    
    QAction* sweep = createAction(tr("&Sweep..."), "", tr("Create swept surface"));
    connect(sweep, &QAction::triggered, this, &MenuBar::sweepRequested);
    surfaceMenu->addAction(sweep);
    
    surfaceMenu->addSeparator();
    
    QAction* freeformSurface = createAction(tr("Free-&form Surface..."), "", tr("Create free-form surface"));
    connect(freeformSurface, &QAction::triggered, this, &MenuBar::freeformSurfaceRequested);
    surfaceMenu->addAction(freeformSurface);
}

void MenuBar::setupHelpMenu()
{
    m_helpMenu = addMenu(tr("&Help"));

    // Getting Started
    QAction* gettingStarted = createAction(tr("&Getting Started"), "", tr("Open getting started guide"));
    connect(gettingStarted, &QAction::triggered, this, &MenuBar::gettingStartedRequested);
    m_helpMenu->addAction(gettingStarted);

    // Tutorials
    QAction* tutorials = createAction(tr("&Tutorials"), "", tr("Open tutorials"));
    connect(tutorials, &QAction::triggered, this, &MenuBar::tutorialsRequested);
    m_helpMenu->addAction(tutorials);

    // Keyboard Shortcuts
    QAction* shortcuts = createAction(tr("&Keyboard Shortcuts..."), "", tr("View keyboard shortcuts"));
    connect(shortcuts, &QAction::triggered, this, &MenuBar::keyboardShortcutsRequested);
    m_helpMenu->addAction(shortcuts);

    m_helpMenu->addSeparator();

    // Documentation
    QAction* documentation = createAction(tr("&Documentation"), "", tr("Open documentation"));
    connect(documentation, &QAction::triggered, this, &MenuBar::documentationRequested);
    m_helpMenu->addAction(documentation);

    // Release Notes
    QAction* releaseNotes = createAction(tr("&Release Notes"), "", tr("View release notes"));
    connect(releaseNotes, &QAction::triggered, this, &MenuBar::releaseNotesRequested);
    m_helpMenu->addAction(releaseNotes);

    m_helpMenu->addSeparator();

    // Check for Updates
    QAction* checkUpdates = createAction(tr("Check for &Updates..."), "", tr("Check for application updates"));
    connect(checkUpdates, &QAction::triggered, this, &MenuBar::checkForUpdatesRequested);
    m_helpMenu->addAction(checkUpdates);

    m_helpMenu->addSeparator();

    // About
    QAction* about = createAction(tr("&About"), "", tr("About dc-3ddesignapp"));
    connect(about, &QAction::triggered, this, &MenuBar::aboutRequested);
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
