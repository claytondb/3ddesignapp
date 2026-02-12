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

    // Create tools
    void createPlaneRequested();
    void createCylinderRequested();
    void createSectionRequested();
    void createSketchRequested();

    // Search
    void searchTextChanged(const QString& text);

private:
    void setupFileGroup();
    void setupHistoryGroup();
    void setupSelectionGroup();
    void setupViewGroup();
    void setupCreateGroup();
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

    // View mode actions
    QActionGroup* m_viewModeGroup;
    QAction* m_actionShaded;
    QAction* m_actionWireframe;
    QAction* m_actionShadedWire;
    QAction* m_actionXRay;

    // Create actions
    QAction* m_actionCreatePlane;
    QAction* m_actionCreateCylinder;
    QAction* m_actionCreateSection;
    QAction* m_actionCreateSketch;

    // Search widget
    QLineEdit* m_searchEdit;
};

#endif // DC_TOOLBAR_H
