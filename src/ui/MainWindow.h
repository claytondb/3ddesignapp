#ifndef DC_MAINWINDOW_H
#define DC_MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QStringList>
#include "dialogs/PrimitiveCreationDialog.h"

class MenuBar;
class Toolbar;
class ObjectBrowser;
class PropertiesPanel;
class StatusBar;
class UndoHistoryDialog;
class QDragEnterEvent;
class QDropEvent;

namespace dc {
class Viewport;
class MeasureTool;
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
 * - Drag and drop file import
 * - Recent files tracking
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
    
    // Recent files management
    void addRecentFile(const QString& filePath);
    QStringList recentFiles() const { return m_recentFiles; }
    void clearRecentFiles();
    
    // Supported file formats
    static QStringList supportedImportFormats();
    static QStringList supportedExportFormats();
    static bool isFormatSupported(const QString& extension);

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
    void recentFilesChanged(const QStringList& files);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onRecentFileRequested(const QString& path);
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
    void onUndoHistoryRequested();
    void onCommandsDiscarded(int count);
    
    // Primitive creation
    void onCreateCubeRequested();
    void onCreateSphereRequested();
    void onCreateCylinderRequested();
    void onCreateConeRequested();
    void onCreatePlaneRequested();
    void onCreateTorusRequested();
    
    // Transform modes
    void onTranslateModeRequested();
    void onRotateModeRequested();
    void onScaleModeRequested();
    
    // Measure tool
    void onMeasureDistanceRequested();
    void onMeasureAngleRequested();
    void onMeasureRadiusRequested();
    void onClearMeasurementsRequested();
    
    // File operations
    void onNewProjectRequested();
    void onSaveProjectRequested();
    void onSaveProjectAsRequested();
    void onExportMeshRequested();
    
    // Edit operations
    void onCutRequested();
    void onCopyRequested();
    void onPasteRequested();
    void onDeleteRequested();
    void onDuplicateRequested();
    void onSelectAllRequested();
    void onDeselectAllRequested();
    void onInvertSelectionRequested();
    
    // Selection modes
    void onSelectModeRequested();
    void onBoxSelectModeRequested();
    void onLassoSelectModeRequested();
    void onBrushSelectModeRequested();
    
    // Additional view modes
    void onDisplayModeXRayRequested();
    void onToggleFullScreenRequested();
    
    // Mesh tools (toolbar)
    void onMeshRepairWizardRequested();
    void onPolygonReductionRequested();
    void onSmoothingRequested();
    void onFillHolesRequested();
    void onClippingBoxRequested();
    
    // Create tools (toolbar)
    void onCreateSectionRequested();
    void onCreateSketchRequested();

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
    
    // Helper for primitive creation with dialog
    void createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType type);

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
    
    // Measure tool
    dc::MeasureTool* m_measureTool;
    
    // Current mode
    QString m_currentMode;
    
    // Recent files (max 10)
    QStringList m_recentFiles;
    static const int MAX_RECENT_FILES = 10;
    
    // Undo history dialog (lazy-created)
    UndoHistoryDialog* m_undoHistoryDialog;
};

#endif // DC_MAINWINDOW_H
