#include "MainWindow.h"
#include "MenuBar.h"
#include "Toolbar.h"
#include "ObjectBrowser.h"
#include "PropertiesPanel.h"
#include "StatusBar.h"
#include "renderer/Viewport.h"
#include "app/Application.h"

#include <QApplication>
#include <QSettings>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QUndoStack>

// Dark theme stylesheet based on STYLE_GUIDE.md
static const char* DARK_THEME_STYLESHEET = R"(
/* Main Window */
QMainWindow {
    background-color: #1a1a1a;
}

/* Menu Bar */
QMenuBar {
    background-color: #2a2a2a;
    color: #b3b3b3;
    border-bottom: 1px solid #4a4a4a;
    padding: 2px;
}

QMenuBar::item {
    background-color: transparent;
    padding: 4px 8px;
    border-radius: 4px;
}

QMenuBar::item:selected {
    background-color: #383838;
    color: #ffffff;
}

QMenuBar::item:pressed {
    background-color: #0078d4;
    color: #ffffff;
}

/* Menus */
QMenu {
    background-color: #2d2d2d;
    color: #b3b3b3;
    border: 1px solid #4a4a4a;
    border-radius: 4px;
    padding: 4px;
}

QMenu::item {
    padding: 6px 24px 6px 12px;
    border-radius: 4px;
}

QMenu::item:selected {
    background-color: #383838;
    color: #ffffff;
}

QMenu::item:disabled {
    color: #5c5c5c;
}

QMenu::separator {
    height: 1px;
    background-color: #4a4a4a;
    margin: 4px 8px;
}

QMenu::indicator {
    width: 16px;
    height: 16px;
    margin-left: 4px;
}

/* Toolbars */
QToolBar {
    background-color: #2a2a2a;
    border: none;
    border-bottom: 1px solid #4a4a4a;
    spacing: 4px;
    padding: 4px;
}

QToolBar::separator {
    width: 1px;
    background-color: #4a4a4a;
    margin: 4px 8px;
}

QToolButton {
    background-color: transparent;
    border: none;
    border-radius: 4px;
    padding: 4px;
    color: #b3b3b3;
}

QToolButton:hover {
    background-color: #383838;
    color: #ffffff;
}

QToolButton:pressed {
    background-color: #404040;
}

QToolButton:checked {
    background-color: #0078d4;
    color: #ffffff;
}

QToolButton:disabled {
    color: #5c5c5c;
}

/* Dock Widgets */
QDockWidget {
    color: #ffffff;
    titlebar-close-icon: url(:/icons/close.svg);
    titlebar-normal-icon: url(:/icons/float.svg);
}

QDockWidget::title {
    background-color: #2a2a2a;
    padding: 6px;
    border-bottom: 1px solid #4a4a4a;
}

QDockWidget::close-button, QDockWidget::float-button {
    background-color: transparent;
    border: none;
    padding: 2px;
}

QDockWidget::close-button:hover, QDockWidget::float-button:hover {
    background-color: #383838;
}

/* Panels */
QWidget#ObjectBrowser, QWidget#PropertiesPanel {
    background-color: #242424;
}

/* Status Bar */
QStatusBar {
    background-color: #2a2a2a;
    color: #b3b3b3;
    border-top: 1px solid #4a4a4a;
}

QStatusBar::item {
    border: none;
}

/* Scroll Bars */
QScrollBar:vertical {
    background-color: #242424;
    width: 12px;
    border: none;
}

QScrollBar::handle:vertical {
    background-color: #4a4a4a;
    border-radius: 4px;
    min-height: 20px;
    margin: 2px;
}

QScrollBar::handle:vertical:hover {
    background-color: #5c5c5c;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar:horizontal {
    background-color: #242424;
    height: 12px;
    border: none;
}

QScrollBar::handle:horizontal {
    background-color: #4a4a4a;
    border-radius: 4px;
    min-width: 20px;
    margin: 2px;
}

QScrollBar::handle:horizontal:hover {
    background-color: #5c5c5c;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0px;
}

/* Tree Widget */
QTreeWidget {
    background-color: #242424;
    color: #b3b3b3;
    border: none;
    outline: none;
}

QTreeWidget::item {
    padding: 4px;
    border-radius: 4px;
}

QTreeWidget::item:hover {
    background-color: #383838;
}

QTreeWidget::item:selected {
    background-color: #0078d4;
    color: #ffffff;
}

QTreeWidget::branch:has-children:!has-siblings:closed,
QTreeWidget::branch:closed:has-children:has-siblings {
    border-image: none;
    image: url(:/icons/branch-closed.svg);
}

QTreeWidget::branch:open:has-children:!has-siblings,
QTreeWidget::branch:open:has-children:has-siblings {
    border-image: none;
    image: url(:/icons/branch-open.svg);
}

/* Buttons */
QPushButton {
    background-color: #3d3d3d;
    color: #b3b3b3;
    border: 1px solid #4a4a4a;
    border-radius: 4px;
    padding: 6px 16px;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #383838;
    color: #ffffff;
    border-color: #5c5c5c;
}

QPushButton:pressed {
    background-color: #404040;
}

QPushButton:disabled {
    background-color: #2a2a2a;
    color: #5c5c5c;
    border-color: #333333;
}

QPushButton[primary="true"] {
    background-color: #0078d4;
    color: #ffffff;
    border: none;
}

QPushButton[primary="true"]:hover {
    background-color: #1a88e0;
}

QPushButton[primary="true"]:pressed {
    background-color: #0066b8;
}

/* Labels */
QLabel {
    color: #b3b3b3;
}

QLabel[heading="true"] {
    color: #ffffff;
    font-weight: 600;
}

/* Line Edit */
QLineEdit {
    background-color: #333333;
    color: #ffffff;
    border: 1px solid #4a4a4a;
    border-radius: 4px;
    padding: 4px 8px;
    selection-background-color: #0078d4;
}

QLineEdit:hover {
    border-color: #5c5c5c;
}

QLineEdit:focus {
    border-color: #0078d4;
}

QLineEdit:disabled {
    background-color: #2a2a2a;
    color: #5c5c5c;
    border-color: #333333;
}

/* Combo Box */
QComboBox {
    background-color: #333333;
    color: #b3b3b3;
    border: 1px solid #4a4a4a;
    border-radius: 4px;
    padding: 4px 8px;
    min-height: 20px;
}

QComboBox:hover {
    border-color: #5c5c5c;
}

QComboBox:focus {
    border-color: #0078d4;
}

QComboBox::drop-down {
    border: none;
    width: 20px;
}

QComboBox::down-arrow {
    image: url(:/icons/dropdown-arrow.svg);
}

QComboBox QAbstractItemView {
    background-color: #2d2d2d;
    color: #b3b3b3;
    border: 1px solid #4a4a4a;
    selection-background-color: #383838;
}

/* Spin Box */
QSpinBox, QDoubleSpinBox {
    background-color: #333333;
    color: #ffffff;
    border: 1px solid #4a4a4a;
    border-radius: 4px;
    padding: 4px 8px;
}

QSpinBox:hover, QDoubleSpinBox:hover {
    border-color: #5c5c5c;
}

QSpinBox:focus, QDoubleSpinBox:focus {
    border-color: #0078d4;
}

/* Check Box */
QCheckBox {
    color: #b3b3b3;
    spacing: 8px;
}

QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid #4a4a4a;
    border-radius: 3px;
    background-color: #333333;
}

QCheckBox::indicator:hover {
    border-color: #5c5c5c;
}

QCheckBox::indicator:checked {
    background-color: #0078d4;
    border-color: #0078d4;
}

/* Group Box */
QGroupBox {
    color: #ffffff;
    border: 1px solid #4a4a4a;
    border-radius: 4px;
    margin-top: 12px;
    padding-top: 12px;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 8px;
    color: #ffffff;
}

/* Tab Widget */
QTabWidget::pane {
    background-color: #242424;
    border: 1px solid #4a4a4a;
    border-top: none;
}

QTabBar::tab {
    background-color: #2a2a2a;
    color: #808080;
    padding: 8px 16px;
    border: 1px solid #4a4a4a;
    border-bottom: none;
    margin-right: 2px;
}

QTabBar::tab:hover {
    color: #b3b3b3;
}

QTabBar::tab:selected {
    background-color: #242424;
    color: #ffffff;
    border-bottom: 2px solid #0078d4;
}

/* Splitter */
QSplitter::handle {
    background-color: #4a4a4a;
}

QSplitter::handle:horizontal {
    width: 1px;
}

QSplitter::handle:vertical {
    height: 1px;
}

/* Progress Bar */
QProgressBar {
    background-color: #333333;
    border: none;
    border-radius: 4px;
    text-align: center;
    color: #ffffff;
}

QProgressBar::chunk {
    background-color: #0078d4;
    border-radius: 4px;
}

/* Slider */
QSlider::groove:horizontal {
    background-color: #4a4a4a;
    height: 4px;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background-color: #ffffff;
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}

QSlider::handle:horizontal:hover {
    background-color: #e0e0e0;
}

QSlider::sub-page:horizontal {
    background-color: #0078d4;
    border-radius: 2px;
}

/* Tooltip */
QToolTip {
    background-color: #1a1a1a;
    color: #ffffff;
    border: 1px solid #4a4a4a;
    border-radius: 4px;
    padding: 8px 12px;
}

/* Stacked Widget */
QStackedWidget {
    background-color: #242424;
}

/* File Dialog */
QFileDialog {
    background-color: #1a1a1a;
}
)";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_menuBar(nullptr)
    , m_toolbar(nullptr)
    , m_objectBrowser(nullptr)
    , m_propertiesPanel(nullptr)
    , m_statusBar(nullptr)
    , m_objectBrowserDock(nullptr)
    , m_propertiesDock(nullptr)
    , m_viewport(nullptr)
    , m_currentMode("Mesh")
{
    setupUI();
    setupConnections();
    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    // Set window properties
    setWindowTitle("dc-3ddesignapp - Scan-to-CAD Application");
    setMinimumSize(1024, 768);
    
    // Apply dark theme
    applyStylesheet();
    
    // Setup components in order
    setupCentralWidget();
    setupMenuBar();
    setupToolbar();
    setupDockWidgets();
    setupStatusBar();
    
    // Allow docking and tabbing
    setDockOptions(QMainWindow::AllowNestedDocks | 
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::AnimatedDocks);
}

void MainWindow::setupCentralWidget()
{
    // Create OpenGL viewport
    m_viewport = new dc::Viewport(this);
    m_viewport->setObjectName("Viewport");
    setCentralWidget(m_viewport);
    
    // Connect viewport signals
    connect(m_viewport, &dc::Viewport::cursorMoved, [this](const QVector3D& pos) {
        setCursorPosition(pos.x(), pos.y(), pos.z());
    });
}

void MainWindow::setupMenuBar()
{
    m_menuBar = new MenuBar(this);
    setMenuBar(m_menuBar);
    
    // Connect menu actions to slots
    connect(m_menuBar, &MenuBar::toggleObjectBrowserRequested, this, &MainWindow::toggleObjectBrowser);
    connect(m_menuBar, &MenuBar::togglePropertiesPanelRequested, this, &MainWindow::togglePropertiesPanel);
}

void MainWindow::setupToolbar()
{
    m_toolbar = new Toolbar(this);
    addToolBar(Qt::TopToolBarArea, m_toolbar);
}

void MainWindow::setupDockWidgets()
{
    // Object Browser (left panel, 200px width)
    m_objectBrowser = new ObjectBrowser(this);
    m_objectBrowserDock = new QDockWidget(tr("Object Browser"), this);
    m_objectBrowserDock->setObjectName("ObjectBrowserDock");
    m_objectBrowserDock->setWidget(m_objectBrowser);
    m_objectBrowserDock->setFeatures(QDockWidget::DockWidgetClosable | 
                                      QDockWidget::DockWidgetMovable |
                                      QDockWidget::DockWidgetFloatable);
    m_objectBrowserDock->setMinimumWidth(180);
    m_objectBrowserDock->setMaximumWidth(400);
    addDockWidget(Qt::LeftDockWidgetArea, m_objectBrowserDock);
    
    // Properties Panel (right panel, 280px width)
    m_propertiesPanel = new PropertiesPanel(this);
    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->setObjectName("PropertiesDock");
    m_propertiesDock->setWidget(m_propertiesPanel);
    m_propertiesDock->setFeatures(QDockWidget::DockWidgetClosable | 
                                   QDockWidget::DockWidgetMovable |
                                   QDockWidget::DockWidgetFloatable);
    m_propertiesDock->setMinimumWidth(250);
    m_propertiesDock->setMaximumWidth(450);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);
    
    // Set initial widths
    resizeDocks({m_objectBrowserDock}, {200}, Qt::Horizontal);
    resizeDocks({m_propertiesDock}, {280}, Qt::Horizontal);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new StatusBar(this);
    setStatusBar(m_statusBar);
}

void MainWindow::setupConnections()
{
    // File menu connections
    connect(m_menuBar, &MenuBar::openProjectRequested, this, &MainWindow::onOpenProjectRequested);
    connect(m_menuBar, &MenuBar::importMeshRequested, this, &MainWindow::onImportMeshRequested);
    
    // View menu connections
    connect(m_menuBar, &MenuBar::viewFrontRequested, this, &MainWindow::onViewFrontRequested);
    connect(m_menuBar, &MenuBar::viewBackRequested, this, &MainWindow::onViewBackRequested);
    connect(m_menuBar, &MenuBar::viewLeftRequested, this, &MainWindow::onViewLeftRequested);
    connect(m_menuBar, &MenuBar::viewRightRequested, this, &MainWindow::onViewRightRequested);
    connect(m_menuBar, &MenuBar::viewTopRequested, this, &MainWindow::onViewTopRequested);
    connect(m_menuBar, &MenuBar::viewBottomRequested, this, &MainWindow::onViewBottomRequested);
    connect(m_menuBar, &MenuBar::viewIsometricRequested, this, &MainWindow::onViewIsometricRequested);
    connect(m_menuBar, &MenuBar::zoomToFitRequested, this, &MainWindow::onZoomToFitRequested);
    connect(m_menuBar, &MenuBar::toggleGridRequested, this, &MainWindow::onToggleGridRequested);
    
    // Display mode connections
    connect(m_menuBar, &MenuBar::displayModeShadedRequested, this, &MainWindow::onDisplayModeShadedRequested);
    connect(m_menuBar, &MenuBar::displayModeWireframeRequested, this, &MainWindow::onDisplayModeWireframeRequested);
    connect(m_menuBar, &MenuBar::displayModeShadedWireRequested, this, &MainWindow::onDisplayModeShadedWireRequested);
    
    // Edit menu connections
    connect(m_menuBar, &MenuBar::undoRequested, this, &MainWindow::onUndoRequested);
    connect(m_menuBar, &MenuBar::redoRequested, this, &MainWindow::onRedoRequested);
    
    // Primitive creation connections
    connect(m_menuBar, &MenuBar::createSphereRequested, this, &MainWindow::onCreateSphereRequested);
    connect(m_menuBar, &MenuBar::createCylinderRequested, this, &MainWindow::onCreateCylinderRequested);
    connect(m_menuBar, &MenuBar::createConeRequested, this, &MainWindow::onCreateConeRequested);
    connect(m_menuBar, &MenuBar::createPlaneRequested, this, &MainWindow::onCreatePlaneRequested);
}

void MainWindow::applyStylesheet()
{
    setStyleSheet(DARK_THEME_STYLESHEET);
}

void MainWindow::loadSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    
    // Restore window geometry
    if (settings.contains("mainwindow/geometry")) {
        restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    }
    
    // Restore dock/toolbar state
    if (settings.contains("mainwindow/state")) {
        restoreState(settings.value("mainwindow/state").toByteArray());
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    
    // Save window geometry
    settings.setValue("mainwindow/geometry", saveGeometry());
    
    // Save dock/toolbar state
    settings.setValue("mainwindow/state", saveState());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

void MainWindow::toggleObjectBrowser()
{
    m_objectBrowserDock->setVisible(!m_objectBrowserDock->isVisible());
}

void MainWindow::togglePropertiesPanel()
{
    m_propertiesDock->setVisible(!m_propertiesDock->isVisible());
}

void MainWindow::setMeshMode()
{
    m_currentMode = "Mesh";
    m_statusBar->setModeIndicator("Mesh");
    emit modeChanged(m_currentMode);
}

void MainWindow::setSketchMode()
{
    m_currentMode = "Sketch";
    m_statusBar->setModeIndicator("Sketch");
    emit modeChanged(m_currentMode);
}

void MainWindow::setSurfaceMode()
{
    m_currentMode = "Surface";
    m_statusBar->setModeIndicator("Surface");
    emit modeChanged(m_currentMode);
}

void MainWindow::setAnalysisMode()
{
    m_currentMode = "Analysis";
    m_statusBar->setModeIndicator("Analysis");
    emit modeChanged(m_currentMode);
}

void MainWindow::setStatusMessage(const QString& message)
{
    m_statusBar->setMessage(message);
}

void MainWindow::setSelectionInfo(const QString& info)
{
    m_statusBar->setSelectionInfo(info);
}

void MainWindow::setCursorPosition(double x, double y, double z)
{
    m_statusBar->setCursorPosition(x, y, z);
}

void MainWindow::setFPS(int fps)
{
    m_statusBar->setFPS(fps);
}

void MainWindow::importMesh()
{
    onImportMeshRequested();
}

void MainWindow::onSceneChanged()
{
    // Update viewport when scene changes
    if (m_viewport) {
        m_viewport->update();
    }
}

void MainWindow::onOpenProjectRequested()
{
    // Open project file dialog
    QString filter = "3D Design Project (*.dc3d);;Mesh Files (*.stl *.obj *.ply);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Project"), QString(), filter);
    
    if (filePath.isEmpty()) {
        return;
    }
    
    // Check if it's a mesh file and import it
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    if (extension == "stl" || extension == "obj" || extension == "ply") {
        // Import as mesh
        auto* app = dc3d::Application::instance();
        if (app) {
            if (!app->importMesh(filePath)) {
                QMessageBox::warning(this, tr("Open Error"), 
                    tr("Failed to open file. Check the console for details."));
            } else {
                setStatusMessage(QString("Opened: %1").arg(fileInfo.fileName()));
            }
        }
    } else if (extension == "dc3d") {
        // TODO: Native project file support
        QMessageBox::information(this, tr("Open Project"), 
            tr("Native project file support coming soon. For now, use File > Import to load mesh files."));
    } else {
        QMessageBox::warning(this, tr("Open Error"), 
            tr("Unsupported file format: %1").arg(extension));
    }
}

void MainWindow::onImportMeshRequested()
{
    QString filter = "Mesh Files (*.stl *.obj *.ply);;STL Files (*.stl);;OBJ Files (*.obj);;PLY Files (*.ply);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Mesh"), QString(), filter);
    
    if (filePath.isEmpty()) {
        return;
    }
    
    auto* app = dc3d::Application::instance();
    if (app) {
        if (!app->importMesh(filePath)) {
            QMessageBox::warning(this, tr("Import Error"), 
                tr("Failed to import mesh file. Check the console for details."));
        }
    }
}

void MainWindow::onViewFrontRequested()
{
    if (m_viewport) {
        m_viewport->setStandardView("front");
    }
}

void MainWindow::onViewBackRequested()
{
    if (m_viewport) {
        m_viewport->setStandardView("back");
    }
}

void MainWindow::onViewLeftRequested()
{
    if (m_viewport) {
        m_viewport->setStandardView("left");
    }
}

void MainWindow::onViewRightRequested()
{
    if (m_viewport) {
        m_viewport->setStandardView("right");
    }
}

void MainWindow::onViewTopRequested()
{
    if (m_viewport) {
        m_viewport->setStandardView("top");
    }
}

void MainWindow::onViewBottomRequested()
{
    if (m_viewport) {
        m_viewport->setStandardView("bottom");
    }
}

void MainWindow::onViewIsometricRequested()
{
    if (m_viewport) {
        m_viewport->setStandardView("isometric");
    }
}

void MainWindow::onZoomToFitRequested()
{
    if (m_viewport) {
        m_viewport->fitView();
    }
}

void MainWindow::onToggleGridRequested()
{
    if (m_viewport) {
        m_viewport->setGridVisible(!m_viewport->isGridVisible());
    }
}

void MainWindow::onDisplayModeShadedRequested()
{
    if (m_viewport) {
        m_viewport->setDisplayMode(dc::DisplayMode::Shaded);
    }
}

void MainWindow::onDisplayModeWireframeRequested()
{
    if (m_viewport) {
        m_viewport->setDisplayMode(dc::DisplayMode::Wireframe);
    }
}

void MainWindow::onDisplayModeShadedWireRequested()
{
    if (m_viewport) {
        m_viewport->setDisplayMode(dc::DisplayMode::ShadedWireframe);
    }
}

void MainWindow::onUndoRequested()
{
    auto* app = dc3d::Application::instance();
    if (app && app->undoStack()) {
        app->undoStack()->undo();
    }
}

void MainWindow::onRedoRequested()
{
    auto* app = dc3d::Application::instance();
    if (app && app->undoStack()) {
        app->undoStack()->redo();
    }
}

void MainWindow::onCreateSphereRequested()
{
    auto* app = dc3d::Application::instance();
    if (app) {
        if (!app->createPrimitive("sphere")) {
            QMessageBox::warning(this, tr("Create Error"), 
                tr("Failed to create sphere primitive."));
        }
    }
}

void MainWindow::onCreateCubeRequested()
{
    auto* app = dc3d::Application::instance();
    if (app) {
        if (!app->createPrimitive("cube")) {
            QMessageBox::warning(this, tr("Create Error"), 
                tr("Failed to create cube primitive."));
        }
    }
}

void MainWindow::onCreateCylinderRequested()
{
    auto* app = dc3d::Application::instance();
    if (app) {
        if (!app->createPrimitive("cylinder")) {
            QMessageBox::warning(this, tr("Create Error"), 
                tr("Failed to create cylinder primitive."));
        }
    }
}

void MainWindow::onCreateConeRequested()
{
    auto* app = dc3d::Application::instance();
    if (app) {
        if (!app->createPrimitive("cone")) {
            QMessageBox::warning(this, tr("Create Error"), 
                tr("Failed to create cone primitive."));
        }
    }
}

void MainWindow::onCreatePlaneRequested()
{
    auto* app = dc3d::Application::instance();
    if (app) {
        if (!app->createPrimitive("plane")) {
            QMessageBox::warning(this, tr("Create Error"), 
                tr("Failed to create plane primitive."));
        }
    }
}
