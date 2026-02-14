#ifndef DC_MENUBAR_H
#define DC_MENUBAR_H

#include <QMenuBar>
#include <QMenu>
#include <QAction>

// Forward declarations for dialogs
class PolygonReductionDialog;
class SmoothingDialog;
class HoleFillDialog;
class OutlierRemovalDialog;
class ClippingBoxDialog;

namespace dc3d {
namespace core {
class CommandStack;
}
}

namespace dc {
class Viewport;
}

/**
 * @brief Application menu bar for dc-3ddesignapp
 * 
 * Implements the menu structure from UI_DESIGN.md:
 * - File, Edit, View, Mesh, Create, Help menus
 * - All keyboard shortcuts
 * - Recent files submenu
 */
class MenuBar : public QMenuBar
{
    Q_OBJECT

public:
    explicit MenuBar(QWidget *parent = nullptr);
    ~MenuBar() override = default;

    // Access to actions for toolbar or shortcut binding
    QAction* actionNew() const { return m_actionNew; }
    QAction* actionOpen() const { return m_actionOpen; }
    QAction* actionSave() const { return m_actionSave; }
    QAction* actionSaveAs() const { return m_actionSaveAs; }
    QAction* actionUndo() const { return m_actionUndo; }
    QAction* actionRedo() const { return m_actionRedo; }
    QAction* actionDelete() const { return m_actionDelete; }

    // Update recent files menu
    void updateRecentFiles(const QStringList& files);
    
    /**
     * @brief Connect undo/redo actions to a command stack
     * @param commandStack The command stack to connect to
     * 
     * This sets up automatic enable/disable of undo/redo menu items
     * and updates the menu text to show the command description.
     */
    void connectToCommandStack(dc3d::core::CommandStack* commandStack);
    
    /**
     * @brief Update undo action state
     * @param canUndo Whether undo is available
     * @param text Description text (e.g., "Import Mesh")
     */
    void setUndoEnabled(bool canUndo, const QString& text = QString());
    
    /**
     * @brief Update redo action state
     * @param canRedo Whether redo is available
     * @param text Description text
     */
    void setRedoEnabled(bool canRedo, const QString& text = QString());
    
    /**
     * @brief Set the viewport for dialog previews
     * @param viewport The main viewport widget
     */
    void setViewport(dc::Viewport* viewport);
    
    /**
     * @brief Access mesh dialogs
     */
    PolygonReductionDialog* polygonReductionDialog() const { return m_polygonReductionDialog; }
    SmoothingDialog* smoothingDialog() const { return m_smoothingDialog; }
    HoleFillDialog* holeFillDialog() const { return m_holeFillDialog; }
    OutlierRemovalDialog* outlierRemovalDialog() const { return m_outlierRemovalDialog; }
    ClippingBoxDialog* clippingBoxDialog() const { return m_clippingBoxDialog; }

signals:
    // File menu signals
    void newProjectRequested();
    void openProjectRequested();
    void saveProjectRequested();
    void saveProjectAsRequested();
    void importMeshRequested();
    void importCADRequested();
    void exportMeshRequested();
    void exportSTEPRequested();
    void exportIGESRequested();
    void recentFileRequested(const QString& path);
    void exitRequested();

    // Edit menu signals
    void undoRequested();
    void redoRequested();
    void cutRequested();
    void copyRequested();
    void pasteRequested();
    void duplicateRequested();
    void deleteRequested();
    void selectAllRequested();
    void deselectAllRequested();
    void invertSelectionRequested();
    void preferencesRequested();

    // View menu signals
    void toggleObjectBrowserRequested();
    void togglePropertiesPanelRequested();
    void zoomToFitRequested();
    void zoomToSelectionRequested();
    void viewFrontRequested();
    void viewBackRequested();
    void viewLeftRequested();
    void viewRightRequested();
    void viewTopRequested();
    void viewBottomRequested();
    void viewIsometricRequested();
    void displayModeShadedRequested();
    void displayModeWireframeRequested();
    void displayModeShadedWireRequested();
    void displayModeXRayRequested();
    void displayModeDeviationRequested();
    void toggleGridRequested();
    void toggleAxesRequested();
    void toggleViewCubeRequested();
    void fullScreenRequested();

    // Mesh menu signals
    void polygonReductionRequested();
    void smoothingRequested();
    void fillHolesRequested();
    void removeOutliersRequested();
    void deFeatureRequested();
    void clippingBoxRequested();
    void splitMeshRequested();
    void mergeMeshesRequested();

    // Create menu signals
    void createPlaneRequested();
    void createCylinderRequested();
    void createConeRequested();
    void createSphereRequested();
    void sectionPlaneRequested();
    void multipleSectionsRequested();
    void sketch2DRequested();
    void sketch3DRequested();
    void fitSurfaceRequested();
    void autoSurfaceRequested();
    void extrudeRequested();
    void revolveRequested();
    void loftRequested();
    void sweepRequested();
    void freeformSurfaceRequested();

    // Help menu signals
    void gettingStartedRequested();
    void tutorialsRequested();
    void keyboardShortcutsRequested();
    void documentationRequested();
    void releaseNotesRequested();
    void checkForUpdatesRequested();
    void aboutRequested();

private slots:
    void showPolygonReductionDialog();
    void showSmoothingDialog();
    void showHoleFillDialog();
    void showOutlierRemovalDialog();
    void showClippingBoxDialog();

private:
    void setupFileMenu();
    void setupEditMenu();
    void setupViewMenu();
    void setupMeshMenu();
    void setupCreateMenu();
    void setupHelpMenu();
    void createMeshDialogs();

    QAction* createAction(const QString& text, const QString& shortcut = QString(),
                          const QString& tooltip = QString());

    // Menus
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_meshMenu;
    QMenu* m_createMenu;
    QMenu* m_helpMenu;
    QMenu* m_recentFilesMenu;

    // File actions
    QAction* m_actionNew;
    QAction* m_actionOpen;
    QAction* m_actionSave;
    QAction* m_actionSaveAs;
    QAction* m_actionImportMesh;
    QAction* m_actionImportCAD;
    QAction* m_actionExportMesh;
    QAction* m_actionExportSTEP;
    QAction* m_actionExportIGES;
    QAction* m_actionProjectSettings;
    QAction* m_actionExit;

    // Edit actions
    QAction* m_actionUndo;
    QAction* m_actionRedo;
    QAction* m_actionCut;
    QAction* m_actionCopy;
    QAction* m_actionPaste;
    QAction* m_actionDuplicate;
    QAction* m_actionDelete;
    QAction* m_actionSelectAll;
    QAction* m_actionDeselectAll;
    QAction* m_actionInvertSelection;
    QAction* m_actionPreferences;

    // View actions
    QAction* m_actionObjectBrowser;
    QAction* m_actionPropertiesPanel;
    QAction* m_actionZoomToFit;
    QAction* m_actionZoomToSelection;
    QAction* m_actionViewFront;
    QAction* m_actionViewBack;
    QAction* m_actionViewLeft;
    QAction* m_actionViewRight;
    QAction* m_actionViewTop;
    QAction* m_actionViewBottom;
    QAction* m_actionViewIsometric;
    QAction* m_actionDisplayShaded;
    QAction* m_actionDisplayWireframe;
    QAction* m_actionDisplayShadedWire;
    QAction* m_actionDisplayXRay;
    QAction* m_actionDisplayDeviation;
    QAction* m_actionToggleGrid;
    QAction* m_actionToggleAxes;
    QAction* m_actionToggleViewCube;
    QAction* m_actionFullScreen;

    // Mesh actions
    QAction* m_actionPolygonReduction;
    QAction* m_actionSmoothing;
    QAction* m_actionFillHoles;
    QAction* m_actionRemoveOutliers;
    QAction* m_actionClippingBox;
    
    // Mesh dialogs
    PolygonReductionDialog* m_polygonReductionDialog;
    SmoothingDialog* m_smoothingDialog;
    HoleFillDialog* m_holeFillDialog;
    OutlierRemovalDialog* m_outlierRemovalDialog;
    ClippingBoxDialog* m_clippingBoxDialog;
    
    // Viewport reference
    dc::Viewport* m_viewport;

    // Create actions
    QAction* m_actionCreatePlane;
    QAction* m_actionCreateCylinder;
    QAction* m_actionSection2D;
    QAction* m_actionSketch2D;
    QAction* m_actionExtrude;
    QAction* m_actionRevolve;
};

#endif // DC_MENUBAR_H
