#ifndef DC_TOOLBAR_H
#define DC_TOOLBAR_H

#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QActionGroup>
#include <QLineEdit>

/**
 * @brief Main toolbar for dc-3ddesignapp
 * 
 * Provides grouped toolbar buttons:
 * - File operations (New, Open, Save, Import)
 * - History (Undo, Redo)
 * - Selection tools (Select, Box, Lasso, Brush)
 * - View modes
 * - Create tools (Primitives, Section, Sketch)
 * - Search placeholder
 */
class Toolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit Toolbar(QWidget *parent = nullptr);
    ~Toolbar() override = default;

    // Access to actions
    QAction* actionNew() const { return m_actionNew; }
    QAction* actionOpen() const { return m_actionOpen; }
    QAction* actionSave() const { return m_actionSave; }
    QAction* actionUndo() const { return m_actionUndo; }
    QAction* actionRedo() const { return m_actionRedo; }
    QAction* actionSelect() const { return m_actionSelect; }

signals:
    // File operations
    void newRequested();
    void openRequested();
    void saveRequested();
    void importRequested();

    // History
    void undoRequested();
    void redoRequested();

    // Selection modes
    void selectModeRequested();
    void boxSelectModeRequested();
    void lassoSelectModeRequested();
    void brushSelectModeRequested();

    // View modes
    void shadedModeRequested();
    void wireframeModeRequested();
    void shadedWireModeRequested();
    void xrayModeRequested();

    // Create tools - primitives
    void createCubeRequested();
    void createSphereRequested();
    void createCylinderRequested();
    void createConeRequested();
    void createPlaneRequested();
    
    // Create tools - other
    void createSectionRequested();
    void createSketchRequested();

    // Mesh tools
    void polygonReductionRequested();
    void smoothingRequested();
    void fillHolesRequested();
    void clippingBoxRequested();
    
    // Measure tools
    void measureDistanceRequested();
    void measureAngleRequested();
    void measureRadiusRequested();
    void clearMeasurementsRequested();
    
    // Transform tools
    void translateModeRequested();
    void rotateModeRequested();
    void scaleModeRequested();

    // Search
    void searchTextChanged(const QString& text);

private:
    void setupFileGroup();
    void setupHistoryGroup();
    void setupSelectionGroup();
    void setupTransformGroup();
    void setupViewGroup();
    void setupCreateGroup();
    void setupMeshToolsGroup();
    void setupMeasureToolsGroup();
    void setupSearchWidget();

    QAction* createAction(const QString& text, const QString& iconName,
                          const QString& tooltip, const QString& shortcut = QString());
    QToolButton* createMenuButton(const QString& text, const QString& iconName,
                                   const QString& tooltip);

    // File actions
    QAction* m_actionNew;
    QAction* m_actionOpen;
    QAction* m_actionSave;
    QAction* m_actionImport;

    // History actions
    QAction* m_actionUndo;
    QAction* m_actionRedo;

    // Selection actions
    QActionGroup* m_selectionGroup;
    QAction* m_actionSelect;
    QAction* m_actionBoxSelect;
    QAction* m_actionLassoSelect;
    QAction* m_actionBrushSelect;
    
    // Transform actions
    QActionGroup* m_transformGroup;
    QAction* m_actionTranslate;
    QAction* m_actionRotate;
    QAction* m_actionScale;

    // View mode actions
    QActionGroup* m_viewModeGroup;
    QAction* m_actionShaded;
    QAction* m_actionWireframe;
    QAction* m_actionShadedWire;
    QAction* m_actionXRay;

    // Create actions - primitives
    QAction* m_actionCreateCube;
    QAction* m_actionCreateSphere;
    QAction* m_actionCreateCylinder;
    QAction* m_actionCreateCone;
    QAction* m_actionCreatePlane;
    
    // Create actions - other
    QAction* m_actionCreateSection;
    QAction* m_actionCreateSketch;

    // Mesh tools actions
    QAction* m_actionPolygonReduction;
    QAction* m_actionSmoothing;
    QAction* m_actionFillHoles;
    QAction* m_actionClippingBox;
    
    // Measure tools actions
    QActionGroup* m_measureGroup;
    QAction* m_actionMeasureDistance;
    QAction* m_actionMeasureAngle;
    QAction* m_actionMeasureRadius;
    QAction* m_actionClearMeasurements;

    // Search widget
    QLineEdit* m_searchEdit;
};

#endif // DC_TOOLBAR_H
