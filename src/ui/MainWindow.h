#ifndef DC_MAINWINDOW_H
#define DC_MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>

class MenuBar;
class Toolbar;
class ObjectBrowser;
class PropertiesPanel;
class StatusBar;

/**
 * @brief Main application window for dc-3ddesignapp
 * 
 * Provides the main window framework with:
 * - Central viewport placeholder
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

    // Viewport access (placeholder for now)
    QWidget* viewport() const { return m_viewport; }

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

signals:
    void modeChanged(const QString& mode);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void setupMenuBar();
    void setupToolbar();
    void setupDockWidgets();
    void setupStatusBar();
    void setupCentralWidget();
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
    
    // Central viewport placeholder
    QWidget* m_viewport;
    
    // Current mode
    QString m_currentMode;
};

#endif // DC_MAINWINDOW_H
