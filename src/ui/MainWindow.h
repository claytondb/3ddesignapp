#ifndef DC_MAINWINDOW_H
#define DC_MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>

class MenuBar;
class Toolbar;
class ObjectBrowser;
class PropertiesPanel;
class StatusBar;

namespace dc {
class Viewport;
}

/**
 * @brief Main application window for dc-3ddesignapp
 * 
 * Provides the main window framework with:
 * - Central viewport (OpenGL 3D view)
 * - Left dock: Object Browser panel
 * - Right dock: Properties panel
 * - Menu bar and toolbar
 * - Status bar
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Accessors for child components
    MenuBar* menuBar() const { return m_menuBar; }
    Toolbar* toolbar() const { return m_toolbar; }
    ObjectBrowser* objectBrowser() const { return m_objectBrowser; }
    PropertiesPanel* propertiesPanel() const { return m_propertiesPanel; }
    StatusBar* statusBar() const { return m_statusBar; }

    // Viewport access (real OpenGL viewport)
    dc::Viewport* viewport() const { return m_viewport; }

public slots:
    // Panel visibility
    void toggleObjectBrowser();
    void togglePropertiesPanel();
    
    // Mode switching
    void setMeshMode();
    void setSketchMode();
    void setSurfaceMode();
    void setAnalysisMode();

    // Status updates
    void setStatusMessage(const QString& message);
    void setSelectionInfo(const QString& info);
    void setCursorPosition(double x, double y, double z);
    void setFPS(int fps);
    
    // Import operations
    void importMesh();
    
    // Scene updates
    void onSceneChanged();
    
    // Tool/operation cancellation
    void cancelCurrentOperation();

signals:
    void modeChanged(const QString& mode);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onOpenProjectRequested();
    void onImportMeshRequested();
    void onViewFrontRequested();
    void onViewBackRequested();
    void onViewLeftRequested();
    void onViewRightRequested();
    void onViewTopRequested();
    void onViewBottomRequested();
    void onViewIsometricRequested();
    void onZoomToFitRequested();
    void onZoomToSelectionRequested();
    void onToggleGridRequested();
    void onDisplayModeShadedRequested();
    void onDisplayModeWireframeRequested();
    void onDisplayModeShadedWireRequested();
    void onUndoRequested();
    void onRedoRequested();
    
    // Primitive creation
    void onCreateSphereRequested();
    void onCreateCubeRequested();
    void onCreateCylinderRequested();
    void onCreateConeRequested();
    void onCreatePlaneRequested();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolbar();
    void setupDockWidgets();
    void setupStatusBar();
    void setupCentralWidget();
    void setupConnections();
    void applyStylesheet();
    void loadSettings();
    void saveSettings();

    // UI Components
    MenuBar* m_menuBar;
    Toolbar* m_toolbar;
    ObjectBrowser* m_objectBrowser;
    PropertiesPanel* m_propertiesPanel;
    StatusBar* m_statusBar;
    
    // Dock widgets
    QDockWidget* m_objectBrowserDock;
    QDockWidget* m_propertiesDock;
    
    // Central viewport (OpenGL)
    dc::Viewport* m_viewport;
    
    // Current mode
    QString m_currentMode;
};

#endif // DC_MAINWINDOW_H
