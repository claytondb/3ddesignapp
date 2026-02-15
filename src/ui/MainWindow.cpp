#include "MainWindow.h"
#include "MenuBar.h"
#include "Toolbar.h"
#include "ObjectBrowser.h"
#include "PropertiesPanel.h"
#include "StatusBar.h"
#include "HelpSystem.h"
#include "dialogs/GettingStartedDialog.h"
#include "dialogs/UndoHistoryDialog.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/PrimitiveCreationDialog.h"
#include "dialogs/MeshRepairWizard.h"
#include "dialogs/PolygonReductionDialog.h"
#include "dialogs/SmoothingDialog.h"
#include "dialogs/HoleFillDialog.h"
#include "dialogs/ClippingBoxDialog.h"
#include "renderer/Viewport.h"
#include "renderer/TransformGizmo.h"
#include "tools/MeasureTool.h"
#include "app/Application.h"

#include <QApplication>
#include <QSettings>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QLocale>
#include <QMessageBox>
#include <QUndoStack>
#include <QTimer>
#include <QWhatsThis>

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
    font-weight: 600;
}

QDockWidget::title {
    background-color: #2a2a2a;
    padding: 8px 12px;
    border-bottom: 1px solid #4a4a4a;
    text-align: left;
}

QDockWidget::close-button, QDockWidget::float-button {
    background-color: transparent;
    border: none;
    border-radius: 3px;
    padding: 4px;
    margin: 2px;
}

QDockWidget::close-button:hover, QDockWidget::float-button:hover {
    background-color: #383838;
}

QDockWidget::close-button:pressed, QDockWidget::float-button:pressed {
    background-color: #404040;
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
}

QTreeWidget::branch:open:has-children:!has-siblings,
QTreeWidget::branch:open:has-children:has-siblings {
    border-image: none;
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
    width: 24px;
}

QComboBox::down-arrow {
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 6px solid #b3b3b3;
    margin-right: 8px;
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
    , m_undoHistoryDialog(nullptr)
{
    setupUI();
    setupConnections();
    loadSettings();
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Install What's This mode shortcut (Shift+F1)
    HelpSystem::instance()->installShortcut(this);
    
    // Show first-run tutorial after window is shown
    QTimer::singleShot(500, this, [this]() {
        GettingStartedDialog::showOnFirstRun(this);
    });
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
    
    // Connect FPS updates to status bar
    connect(m_viewport, &dc::Viewport::fpsUpdated, this, &MainWindow::setFPS);
    
    // Create measure tool
    m_measureTool = new dc::MeasureTool(m_viewport, this);
    
    // Connect measure tool signals to status bar
    connect(m_measureTool, &dc::MeasureTool::statusUpdate, [this](const QString& text) {
        m_statusBar->showTemporaryMessage(text);
    });
    connect(m_measureTool, &dc::MeasureTool::toolHintUpdate, [this](const QString& hint) {
        m_statusBar->setToolHint(hint);
    });
    
    // Connect transform mode changes to toolbar sync
    connect(m_viewport, &dc::Viewport::transformModeChanged, [this](int mode) {
        if (m_toolbar) {
            m_toolbar->setTransformMode(mode);
        }
        // Update status bar with mode info
        QString modeName;
        switch (mode) {
            case 0: modeName = tr("Move"); break;
            case 1: modeName = tr("Rotate"); break;
            case 2: modeName = tr("Scale"); break;
        }
        setStatusMessage(tr("Transform: %1").arg(modeName));
    });
    
    // Connect axis constraint changes to status bar
    connect(m_viewport, &dc::Viewport::axisConstraintChanged, [this](dc::AxisConstraint constraint) {
        QString constraintStr = dc::axisConstraintToString(constraint);
        if (!constraintStr.isEmpty()) {
            setStatusMessage(tr("Constrained to: %1").arg(constraintStr));
        } else {
            setStatusMessage(tr("Free transform"));
        }
    });
    
    // Connect coordinate space changes
    connect(m_viewport, &dc::Viewport::coordinateSpaceChanged, [this](dc::CoordinateSpace space) {
        QString spaceStr = dc::coordinateSpaceToString(space);
        setStatusMessage(tr("Coordinate Space: %1").arg(spaceStr));
    });
    
    // Connect pivot point changes
    connect(m_viewport, &dc::Viewport::pivotPointChanged, [this](dc::PivotPoint pivot) {
        QString pivotStr = dc::pivotPointToString(pivot);
        setStatusMessage(tr("Pivot: %1").arg(pivotStr));
    });
    
    // Connect numeric input signals
    connect(m_viewport, &dc::Viewport::numericInputStarted, [this]() {
        setStatusMessage(tr("Enter value (comma for X,Y,Z)..."));
    });
    
    connect(m_viewport, &dc::Viewport::numericInputChanged, [this](const QString& text) {
        setStatusMessage(tr("Value: %1").arg(text.isEmpty() ? tr("(type number)") : text));
    });
    
    connect(m_viewport, &dc::Viewport::numericInputConfirmed, [this](const QVector3D& value) {
        setStatusMessage(tr("Applied: %.2f, %.2f, %.2f").arg(value.x()).arg(value.y()).arg(value.z()));
    });
    
    connect(m_viewport, &dc::Viewport::numericInputCancelled, [this]() {
        setStatusMessage(tr("Input cancelled"));
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
    // Application import feedback connections
    auto* app = dc3d::Application::instance();
    if (app) {
        connect(app, &dc3d::Application::meshImported, this, 
            [this](const QString& name, uint64_t /*id*/, size_t vertexCount, size_t faceCount, double /*loadTimeMs*/) {
            // Format statistics for user feedback
            QLocale locale;
            QString stats = tr("Imported: %1 (%2 triangles, %3 vertices)")
                .arg(name)
                .arg(locale.toString(static_cast<qulonglong>(faceCount)))
                .arg(locale.toString(static_cast<qulonglong>(vertexCount)));
            
            m_statusBar->showSuccess(stats);
        });
        
        connect(app, &dc3d::Application::importFailed, this, [this](const QString& error) {
            // Show brief error in status bar
            QString briefError = error.section('\n', 0, 0);  // First line only
            m_statusBar->showError(briefError);
            
            // Show full detailed error in dialog for user attention
            QMessageBox::warning(this, tr("Import Failed"), error);
        });
    }
    
    // File menu connections
    connect(m_menuBar, &MenuBar::openProjectRequested, this, &MainWindow::onOpenProjectRequested);
    connect(m_menuBar, &MenuBar::importMeshRequested, this, &MainWindow::onImportMeshRequested);
    connect(m_menuBar, &MenuBar::recentFileRequested, this, &MainWindow::onRecentFileRequested);
    
    // Toolbar connections - connect toolbar signals to the same slots as menu
    connect(m_toolbar, &Toolbar::openRequested, this, &MainWindow::onOpenProjectRequested);
    connect(m_toolbar, &Toolbar::newRequested, m_menuBar, &MenuBar::newProjectRequested);
    connect(m_toolbar, &Toolbar::saveRequested, m_menuBar, &MenuBar::saveProjectRequested);
    connect(m_toolbar, &Toolbar::importRequested, this, &MainWindow::onImportMeshRequested);
    connect(m_toolbar, &Toolbar::undoRequested, this, &MainWindow::onUndoRequested);
    connect(m_toolbar, &Toolbar::redoRequested, this, &MainWindow::onRedoRequested);
    
    // Transform mode connections
    connect(m_toolbar, &Toolbar::translateModeRequested, this, &MainWindow::onTranslateModeRequested);
    connect(m_toolbar, &Toolbar::rotateModeRequested, this, &MainWindow::onRotateModeRequested);
    connect(m_toolbar, &Toolbar::scaleModeRequested, this, &MainWindow::onScaleModeRequested);
    
    // View menu connections
    connect(m_menuBar, &MenuBar::viewFrontRequested, this, &MainWindow::onViewFrontRequested);
    connect(m_menuBar, &MenuBar::viewBackRequested, this, &MainWindow::onViewBackRequested);
    connect(m_menuBar, &MenuBar::viewLeftRequested, this, &MainWindow::onViewLeftRequested);
    connect(m_menuBar, &MenuBar::viewRightRequested, this, &MainWindow::onViewRightRequested);
    connect(m_menuBar, &MenuBar::viewTopRequested, this, &MainWindow::onViewTopRequested);
    connect(m_menuBar, &MenuBar::viewBottomRequested, this, &MainWindow::onViewBottomRequested);
    connect(m_menuBar, &MenuBar::viewIsometricRequested, this, &MainWindow::onViewIsometricRequested);
    connect(m_menuBar, &MenuBar::zoomToFitRequested, this, &MainWindow::onZoomToFitRequested);
    connect(m_menuBar, &MenuBar::zoomToSelectionRequested, this, &MainWindow::onZoomToSelectionRequested);
    connect(m_menuBar, &MenuBar::toggleGridRequested, this, &MainWindow::onToggleGridRequested);
    
    // Display mode connections
    connect(m_menuBar, &MenuBar::displayModeShadedRequested, this, &MainWindow::onDisplayModeShadedRequested);
    connect(m_menuBar, &MenuBar::displayModeWireframeRequested, this, &MainWindow::onDisplayModeWireframeRequested);
    connect(m_menuBar, &MenuBar::displayModeShadedWireRequested, this, &MainWindow::onDisplayModeShadedWireRequested);
    
    // Edit menu connections
    connect(m_menuBar, &MenuBar::undoRequested, this, &MainWindow::onUndoRequested);
    connect(m_menuBar, &MenuBar::redoRequested, this, &MainWindow::onRedoRequested);
    connect(m_menuBar, &MenuBar::undoHistoryRequested, this, &MainWindow::onUndoHistoryRequested);
    connect(m_menuBar, &MenuBar::preferencesRequested, this, [this]() {
        PreferencesDialog* dialog = new PreferencesDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &PreferencesDialog::settingsChanged, this, &MainWindow::onSceneChanged);
        
        // Also reload preferences in Application (for undo limit, etc.)
        auto* app = dc3d::Application::instance();
        if (app) {
            connect(dialog, &PreferencesDialog::settingsChanged, app, &dc3d::Application::reloadPreferences);
        }
        
        dialog->show();
    });
    
    // Primitive creation connections - menu
    connect(m_menuBar, &MenuBar::createCubeRequested, this, &MainWindow::onCreateCubeRequested);
    connect(m_menuBar, &MenuBar::createSphereRequested, this, &MainWindow::onCreateSphereRequested);
    connect(m_menuBar, &MenuBar::createCylinderRequested, this, &MainWindow::onCreateCylinderRequested);
    connect(m_menuBar, &MenuBar::createConeRequested, this, &MainWindow::onCreateConeRequested);
    connect(m_menuBar, &MenuBar::createPlaneRequested, this, &MainWindow::onCreatePlaneRequested);
    connect(m_menuBar, &MenuBar::createTorusRequested, this, &MainWindow::onCreateTorusRequested);
    
    // Primitive creation connections - toolbar
    connect(m_toolbar, &Toolbar::createCubeRequested, this, &MainWindow::onCreateCubeRequested);
    connect(m_toolbar, &Toolbar::createSphereRequested, this, &MainWindow::onCreateSphereRequested);
    connect(m_toolbar, &Toolbar::createCylinderRequested, this, &MainWindow::onCreateCylinderRequested);
    connect(m_toolbar, &Toolbar::createConeRequested, this, &MainWindow::onCreateConeRequested);
    connect(m_toolbar, &Toolbar::createPlaneRequested, this, &MainWindow::onCreatePlaneRequested);
    
    // Measure tool connections - menu
    connect(m_menuBar, &MenuBar::measureDistanceRequested, this, &MainWindow::onMeasureDistanceRequested);
    connect(m_menuBar, &MenuBar::measureAngleRequested, this, &MainWindow::onMeasureAngleRequested);
    connect(m_menuBar, &MenuBar::measureRadiusRequested, this, &MainWindow::onMeasureRadiusRequested);
    connect(m_menuBar, &MenuBar::clearMeasurementsRequested, this, &MainWindow::onClearMeasurementsRequested);
    
    // Measure tool connections - toolbar
    connect(m_toolbar, &Toolbar::measureDistanceRequested, this, &MainWindow::onMeasureDistanceRequested);
    connect(m_toolbar, &Toolbar::measureAngleRequested, this, &MainWindow::onMeasureAngleRequested);
    connect(m_toolbar, &Toolbar::measureRadiusRequested, this, &MainWindow::onMeasureRadiusRequested);
    connect(m_toolbar, &Toolbar::clearMeasurementsRequested, this, &MainWindow::onClearMeasurementsRequested);
    
    // File menu connections - New, Save, Save As, Export
    connect(m_menuBar, &MenuBar::newProjectRequested, this, &MainWindow::onNewProjectRequested);
    connect(m_menuBar, &MenuBar::saveProjectRequested, this, &MainWindow::onSaveProjectRequested);
    connect(m_menuBar, &MenuBar::saveProjectAsRequested, this, &MainWindow::onSaveProjectAsRequested);
    connect(m_menuBar, &MenuBar::exportMeshRequested, this, &MainWindow::onExportMeshRequested);
    
    // Edit menu connections - clipboard and selection
    connect(m_menuBar, &MenuBar::cutRequested, this, &MainWindow::onCutRequested);
    connect(m_menuBar, &MenuBar::copyRequested, this, &MainWindow::onCopyRequested);
    connect(m_menuBar, &MenuBar::pasteRequested, this, &MainWindow::onPasteRequested);
    connect(m_menuBar, &MenuBar::deleteRequested, this, &MainWindow::onDeleteRequested);
    connect(m_menuBar, &MenuBar::duplicateRequested, this, &MainWindow::onDuplicateRequested);
    connect(m_menuBar, &MenuBar::selectAllRequested, this, &MainWindow::onSelectAllRequested);
    connect(m_menuBar, &MenuBar::deselectAllRequested, this, &MainWindow::onDeselectAllRequested);
    connect(m_menuBar, &MenuBar::invertSelectionRequested, this, &MainWindow::onInvertSelectionRequested);
    
    // Selection mode connections - toolbar
    connect(m_toolbar, &Toolbar::selectModeRequested, this, &MainWindow::onSelectModeRequested);
    connect(m_toolbar, &Toolbar::boxSelectModeRequested, this, &MainWindow::onBoxSelectModeRequested);
    connect(m_toolbar, &Toolbar::lassoSelectModeRequested, this, &MainWindow::onLassoSelectModeRequested);
    connect(m_toolbar, &Toolbar::brushSelectModeRequested, this, &MainWindow::onBrushSelectModeRequested);
    
    // View mode connections - toolbar
    connect(m_toolbar, &Toolbar::shadedModeRequested, this, &MainWindow::onDisplayModeShadedRequested);
    connect(m_toolbar, &Toolbar::wireframeModeRequested, this, &MainWindow::onDisplayModeWireframeRequested);
    connect(m_toolbar, &Toolbar::shadedWireModeRequested, this, &MainWindow::onDisplayModeShadedWireRequested);
    connect(m_toolbar, &Toolbar::xrayModeRequested, this, &MainWindow::onDisplayModeXRayRequested);
    
    // Additional view menu connections
    connect(m_menuBar, &MenuBar::displayModeXRayRequested, this, &MainWindow::onDisplayModeXRayRequested);
    connect(m_menuBar, &MenuBar::fullScreenRequested, this, &MainWindow::onToggleFullScreenRequested);
    
    // Mesh tools connections - toolbar
    connect(m_toolbar, &Toolbar::meshRepairWizardRequested, this, &MainWindow::onMeshRepairWizardRequested);
    connect(m_toolbar, &Toolbar::polygonReductionRequested, this, &MainWindow::onPolygonReductionRequested);
    connect(m_toolbar, &Toolbar::smoothingRequested, this, &MainWindow::onSmoothingRequested);
    connect(m_toolbar, &Toolbar::fillHolesRequested, this, &MainWindow::onFillHolesRequested);
    connect(m_toolbar, &Toolbar::clippingBoxRequested, this, &MainWindow::onClippingBoxRequested);
    
    // Create tools connections - toolbar
    connect(m_toolbar, &Toolbar::createSectionRequested, this, &MainWindow::onCreateSectionRequested);
    connect(m_toolbar, &Toolbar::createSketchRequested, this, &MainWindow::onCreateSketchRequested);
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
    
    // Load recent files
    m_recentFiles = settings.value("recentFiles/list").toStringList();
    
    // Remove any files that no longer exist
    QStringList validFiles;
    for (const QString& file : m_recentFiles) {
        if (QFileInfo::exists(file)) {
            validFiles.append(file);
        }
    }
    m_recentFiles = validFiles;
    
    // Update menu
    if (m_menuBar) {
        m_menuBar->updateRecentFiles(m_recentFiles);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    
    // Save window geometry
    settings.setValue("mainwindow/geometry", saveGeometry());
    
    // Save dock/toolbar state
    settings.setValue("mainwindow/state", saveState());
    
    // Save recent files
    settings.setValue("recentFiles/list", m_recentFiles);
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
    // Build comprehensive filter with format hints
    QString filter = tr(
        "All Supported Files (*.dc3d *.stl *.obj *.ply *.step *.stp *.iges *.igs);;"
        "3D Design Project (*.dc3d);;"
        "Mesh Files (*.stl *.obj *.ply);;"
        "STL Files (*.stl);;"
        "OBJ Wavefront (*.obj);;"
        "PLY Point Cloud (*.ply);;"
        "STEP CAD Files (*.step *.stp);;"
        "IGES CAD Files (*.iges *.igs);;"
        "All Files (*)"
    );
    
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Project"), QString(), filter);
    
    if (filePath.isEmpty()) {
        return;
    }
    
    // Check if it's a mesh file and import it
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    if (isFormatSupported(extension)) {
        // Import as mesh
        auto* app = dc3d::Application::instance();
        if (app) {
            if (!app->importMesh(filePath)) {
                QMessageBox::warning(this, tr("Open Error"), 
                    tr("Failed to open file. Check the console for details."));
            } else {
                addRecentFile(filePath);
                setStatusMessage(QString("Opened: %1").arg(fileInfo.fileName()));
            }
        }
    } else if (extension == "dc3d") {
        // TODO: Native project file support
        QMessageBox::information(this, tr("Open Project"), 
            tr("Native project file support coming soon. For now, use File > Import to load mesh files."));
    } else {
        // Show helpful error with supported formats
        QString supportedList = supportedImportFormats().join(", ").toUpper();
        QMessageBox::warning(this, tr("Unsupported Format"), 
            tr("Cannot open file with extension '.%1'\n\n"
               "Supported formats:\n%2\n\n"
               "Tip: You can drag and drop supported files directly onto the window.")
                .arg(extension)
                .arg(supportedList));
    }
}

void MainWindow::onImportMeshRequested()
{
    // Comprehensive filter with format descriptions
    QString filter = tr(
        "All Mesh Files (*.stl *.obj *.ply);;"
        "STL Stereolithography (*.stl);;"
        "OBJ Wavefront (*.obj);;"
        "PLY Polygon File Format (*.ply);;"
        "All Files (*)"
    );
    
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Mesh"), QString(), filter);
    
    if (filePath.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    // Check if format is supported
    if (extension != "stl" && extension != "obj" && extension != "ply") {
        QMessageBox::warning(this, tr("Unsupported Format"),
            tr("Cannot import file with extension '.%1'\n\n"
               "Supported mesh formats: STL, OBJ, PLY\n\n"
               "For CAD files (STEP, IGES), use File → Import → CAD")
                .arg(extension));
        return;
    }
    
    // Show loading indicator
    m_statusBar->setMessage(tr("Importing %1...").arg(fileInfo.fileName()));
    
    auto* app = dc3d::Application::instance();
    if (app) {
        // Import - success/failure will be handled by connected signals (setupConnections)
        // The signals provide detailed error messages from the importers
        app->importMesh(filePath);
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

void MainWindow::onZoomToSelectionRequested()
{
    if (m_viewport) {
        m_viewport->zoomToSelection();
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

void MainWindow::onUndoHistoryRequested()
{
    // Create dialog lazily
    if (!m_undoHistoryDialog) {
        m_undoHistoryDialog = new UndoHistoryDialog(this);
    }
    
    // Connect to undo stack
    auto* app = dc3d::Application::instance();
    if (app && app->undoStack()) {
        m_undoHistoryDialog->setUndoStack(app->undoStack());
    }
    
    m_undoHistoryDialog->show();
    m_undoHistoryDialog->raise();
    m_undoHistoryDialog->activateWindow();
}

void MainWindow::onCommandsDiscarded(int count)
{
    // Show a subtle warning in the status bar when commands are discarded
    // Note: QUndoStack doesn't emit this signal, but we keep the slot for
    // future use if we switch to the custom CommandStack
    if (m_statusBar && count > 0) {
        m_statusBar->showInfo(tr("Undo history limit reached: %1 old command(s) discarded").arg(count));
    }
}

void MainWindow::createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType type)
{
    auto* app = dc3d::Application::instance();
    if (!app) return;
    
    // Get view center (camera target) for positioning
    glm::vec3 viewCenter(0.0f);
    if (m_viewport) {
        QVector3D center = m_viewport->viewCenter();
        viewCenter = glm::vec3(center.x(), center.y(), center.z());
    }
    
    // Show configuration dialog
    PrimitiveCreationDialog::PrimitiveConfig config;
    if (!PrimitiveCreationDialog::getConfig(type, config, this)) {
        return;  // User cancelled
    }
    
    // Set position based on user choice
    if (config.positionAtViewCenter) {
        config.position = viewCenter;
    }
    
    // Create primitive with configured settings
    QString typeName;
    switch (type) {
        case PrimitiveCreationDialog::PrimitiveType::Cube:     typeName = "cube"; break;
        case PrimitiveCreationDialog::PrimitiveType::Sphere:   typeName = "sphere"; break;
        case PrimitiveCreationDialog::PrimitiveType::Cylinder: typeName = "cylinder"; break;
        case PrimitiveCreationDialog::PrimitiveType::Cone:     typeName = "cone"; break;
        case PrimitiveCreationDialog::PrimitiveType::Plane:    typeName = "plane"; break;
        case PrimitiveCreationDialog::PrimitiveType::Torus:    typeName = "torus"; break;
    }
    
    // Use enhanced createPrimitive with config
    if (!app->createPrimitiveWithConfig(typeName, config.position, 
                                         config.width, config.height, config.depth,
                                         config.segments, config.selectAfterCreation)) {
        QMessageBox::warning(this, tr("Create Error"), 
            tr("Failed to create %1 primitive.").arg(typeName));
    } else {
        m_statusBar->showSuccess(tr("Created %1").arg(typeName));
        
        // Show properties panel if requested
        if (config.selectAfterCreation) {
            m_propertiesDock->show();
        }
    }
}

void MainWindow::onCreateCubeRequested()
{
    createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType::Cube);
}

void MainWindow::onCreateSphereRequested()
{
    createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType::Sphere);
}

void MainWindow::onCreateCylinderRequested()
{
    createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType::Cylinder);
}

void MainWindow::onCreateConeRequested()
{
    createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType::Cone);
}

void MainWindow::onCreatePlaneRequested()
{
    createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType::Plane);
}

void MainWindow::onCreateTorusRequested()
{
    createPrimitiveWithDialog(PrimitiveCreationDialog::PrimitiveType::Torus);
}

void MainWindow::onTranslateModeRequested()
{
    if (m_viewport) {
        m_viewport->setGizmoMode(0);  // GizmoMode::Translate
    }
}

void MainWindow::onRotateModeRequested()
{
    if (m_viewport) {
        m_viewport->setGizmoMode(1);  // GizmoMode::Rotate
    }
}

void MainWindow::onScaleModeRequested()
{
    if (m_viewport) {
        m_viewport->setGizmoMode(2);  // GizmoMode::Scale
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Handle Escape key to cancel current operation
    if (event->key() == Qt::Key_Escape) {
        cancelCurrentOperation();
        event->accept();
        return;
    }
    
    QMainWindow::keyPressEvent(event);
}

void MainWindow::cancelCurrentOperation()
{
    // Deselect all objects
    auto* app = dc3d::Application::instance();
    if (app) {
        app->deselectAll();
    }
    
    // Reset tool to default selection mode
    if (m_toolbar && m_toolbar->actionSelect()) {
        m_toolbar->actionSelect()->setChecked(true);
    }
    
    // Clear any active tool hints
    if (m_statusBar) {
        m_statusBar->clearToolHint();
        m_statusBar->showInfo(tr("Operation cancelled - returned to Select mode"));
    }
    
    // Reset to default mode if in special mode
    if (m_currentMode != "Mesh") {
        setMeshMode();
    }
}

// ============================================================================
// Recent Files Management
// ============================================================================

void MainWindow::addRecentFile(const QString& filePath)
{
    // Remove if already in list (will re-add at top)
    m_recentFiles.removeAll(filePath);
    
    // Add to front
    m_recentFiles.prepend(filePath);
    
    // Limit to max recent files
    while (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }
    
    // Update menu
    if (m_menuBar) {
        m_menuBar->updateRecentFiles(m_recentFiles);
    }
    
    // Save immediately
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    settings.setValue("recentFiles/list", m_recentFiles);
    
    emit recentFilesChanged(m_recentFiles);
}

void MainWindow::clearRecentFiles()
{
    m_recentFiles.clear();
    
    if (m_menuBar) {
        m_menuBar->updateRecentFiles(m_recentFiles);
    }
    
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    settings.setValue("recentFiles/list", m_recentFiles);
    
    emit recentFilesChanged(m_recentFiles);
}

void MainWindow::onRecentFileRequested(const QString& path)
{
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("File Not Found"),
            tr("The file no longer exists:\n%1").arg(path));
        
        // Remove from recent files
        m_recentFiles.removeAll(path);
        if (m_menuBar) {
            m_menuBar->updateRecentFiles(m_recentFiles);
        }
        return;
    }
    
    auto* app = dc3d::Application::instance();
    if (app) {
        if (app->importMesh(path)) {
            addRecentFile(path);
            QFileInfo fileInfo(path);
            setStatusMessage(QString("Opened: %1").arg(fileInfo.fileName()));
        } else {
            QMessageBox::warning(this, tr("Open Error"), 
                tr("Failed to open file. Check the console for details."));
        }
    }
}

// ============================================================================
// Drag and Drop Support
// ============================================================================

QStringList MainWindow::supportedImportFormats()
{
    return QStringList() << "stl" << "obj" << "ply" << "step" << "stp" << "iges" << "igs";
}

QStringList MainWindow::supportedExportFormats()
{
    return QStringList() << "stl" << "obj" << "ply" << "step" << "stp" << "iges" << "igs";
}

bool MainWindow::isFormatSupported(const QString& extension)
{
    QString ext = extension.toLower();
    return supportedImportFormats().contains(ext);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        // Check if any URL is a supported file format
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);
                QString ext = fileInfo.suffix().toLower();
                
                if (isFormatSupported(ext)) {
                    event->acceptProposedAction();
                    if (m_statusBar) {
                        m_statusBar->showInfo(tr("Drop to import: %1").arg(fileInfo.fileName()));
                    }
                    return;
                }
            }
        }
    }
    
    // Not a supported file
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    
    QStringList importedFiles;
    QStringList failedFiles;
    QStringList unsupportedFiles;
    
    for (const QUrl& url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }
        
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        QString ext = fileInfo.suffix().toLower();
        
        if (!isFormatSupported(ext)) {
            unsupportedFiles.append(fileInfo.fileName());
            continue;
        }
        
        // Try to import
        auto* app = dc3d::Application::instance();
        if (app && app->importMesh(filePath)) {
            importedFiles.append(fileInfo.fileName());
            addRecentFile(filePath);
        } else {
            failedFiles.append(fileInfo.fileName());
        }
    }
    
    // Show result
    if (!importedFiles.isEmpty()) {
        if (importedFiles.size() == 1) {
            setStatusMessage(tr("Imported: %1").arg(importedFiles.first()));
        } else {
            setStatusMessage(tr("Imported %1 files").arg(importedFiles.size()));
        }
        event->acceptProposedAction();
    }
    
    // Show errors for failed imports
    if (!failedFiles.isEmpty()) {
        QMessageBox::warning(this, tr("Import Failed"),
            tr("Failed to import:\n%1").arg(failedFiles.join("\n")));
    }
    
    // Show warning for unsupported formats
    if (!unsupportedFiles.isEmpty()) {
        QString supportedList = supportedImportFormats().join(", ").toUpper();
        QMessageBox::information(this, tr("Unsupported Format"),
            tr("The following files have unsupported formats:\n%1\n\n"
               "Supported formats: %2")
                .arg(unsupportedFiles.join("\n"))
                .arg(supportedList));
    }
}

// ---- Measurement Tool Handlers ----

void MainWindow::onMeasureDistanceRequested()
{
    if (m_measureTool) {
        m_measureTool->setMode(dc::MeasureMode::Distance);
        m_measureTool->activate();
        setStatusMessage(tr("Click two points to measure distance"));
    }
}

void MainWindow::onMeasureAngleRequested()
{
    if (m_measureTool) {
        m_measureTool->setMode(dc::MeasureMode::Angle);
        m_measureTool->activate();
        setStatusMessage(tr("Click three points to measure angle"));
    }
}

void MainWindow::onMeasureRadiusRequested()
{
    if (m_measureTool) {
        m_measureTool->setMode(dc::MeasureMode::Radius);
        m_measureTool->activate();
        setStatusMessage(tr("Click on a curved surface to measure radius"));
    }
}

void MainWindow::onClearMeasurementsRequested()
{
    if (m_measureTool) {
        m_measureTool->clearAllMeasurements();
        setStatusMessage(tr("Measurements cleared"));
    }
}

// ============================================================================
// File Operations
// ============================================================================

void MainWindow::onNewProjectRequested()
{
    // TODO: Implement full project management with native format
    // For now, clear the scene after confirmation
    auto* app = dc3d::Application::instance();
    if (app && app->sceneManager()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("New Project"),
            tr("This will clear the current scene. Continue?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            // TODO: app->sceneManager()->clear();
            setStatusMessage(tr("New project created"));
            m_statusBar->showInfo(tr("New project - scene cleared"));
        }
    }
}

void MainWindow::onSaveProjectRequested()
{
    // TODO: Implement native .dc3d project file format
    QMessageBox::information(this, tr("Save Project"),
        tr("Native project save (.dc3d) is not yet implemented.\n\n"
           "Use File → Export → Mesh (STL) to export your mesh data."));
}

void MainWindow::onSaveProjectAsRequested()
{
    // TODO: Implement native .dc3d project file format
    QMessageBox::information(this, tr("Save Project As"),
        tr("Native project save (.dc3d) is not yet implemented.\n\n"
           "Use File → Export → Mesh (STL) to export your mesh data."));
}

void MainWindow::onExportMeshRequested()
{
    // Build export filter
    QString filter = tr(
        "STL Stereolithography (*.stl);;"
        "OBJ Wavefront (*.obj);;"
        "PLY Polygon File Format (*.ply);;"
        "All Files (*)"
    );
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Mesh"), QString(), filter);
    
    if (filePath.isEmpty()) {
        return;
    }
    
    // TODO: Implement actual export using SceneManager
    // For now, show stub message
    QMessageBox::information(this, tr("Export Mesh"),
        tr("Mesh export to '%1' is not yet fully implemented.\n\n"
           "This feature will be available in a future update.")
            .arg(QFileInfo(filePath).fileName()));
}

// ============================================================================
// Edit Operations - Clipboard & Selection
// ============================================================================

void MainWindow::onCutRequested()
{
    // TODO: Implement clipboard support for mesh objects
    // Cut = Copy + Delete
    auto* app = dc3d::Application::instance();
    if (app && app->selection() && !app->selection()->isEmpty()) {
        // TODO: Serialize selected objects to clipboard
        // TODO: Delete selected objects
        m_statusBar->showInfo(tr("Cut: clipboard not yet implemented"));
    } else {
        m_statusBar->showInfo(tr("Nothing selected to cut"));
    }
}

void MainWindow::onCopyRequested()
{
    // TODO: Implement clipboard support for mesh objects
    auto* app = dc3d::Application::instance();
    if (app && app->selection() && !app->selection()->isEmpty()) {
        // TODO: Serialize selected objects to clipboard
        m_statusBar->showInfo(tr("Copy: clipboard not yet implemented"));
    } else {
        m_statusBar->showInfo(tr("Nothing selected to copy"));
    }
}

void MainWindow::onPasteRequested()
{
    // TODO: Implement clipboard support for mesh objects
    // TODO: Deserialize objects from clipboard and add to scene
    m_statusBar->showInfo(tr("Paste: clipboard not yet implemented"));
}

void MainWindow::onDeleteRequested()
{
    auto* app = dc3d::Application::instance();
    if (app && app->selection() && !app->selection()->isEmpty()) {
        // TODO: Use DeleteCommand for proper undo support
        // For now, show confirmation
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Delete"),
            tr("Delete selected objects?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            // TODO: Execute delete through undo stack
            // app->undoStack()->push(new DeleteCommand(app->selection()->selectedIds()));
            m_statusBar->showInfo(tr("Delete: not yet implemented with undo support"));
        }
    } else {
        m_statusBar->showInfo(tr("Nothing selected to delete"));
    }
}

void MainWindow::onDuplicateRequested()
{
    auto* app = dc3d::Application::instance();
    if (app && app->selection() && !app->selection()->isEmpty()) {
        // TODO: Implement duplicate (copy in place with offset)
        m_statusBar->showInfo(tr("Duplicate: not yet implemented"));
    } else {
        m_statusBar->showInfo(tr("Nothing selected to duplicate"));
    }
}

void MainWindow::onSelectAllRequested()
{
    auto* app = dc3d::Application::instance();
    if (app && app->selection()) {
        // TODO: Select all objects in scene
        // app->selection()->selectAll();
        m_statusBar->showInfo(tr("Select All: not yet implemented"));
    }
}

void MainWindow::onDeselectAllRequested()
{
    auto* app = dc3d::Application::instance();
    if (app) {
        app->deselectAll();
        m_statusBar->showInfo(tr("Selection cleared"));
    }
}

void MainWindow::onInvertSelectionRequested()
{
    auto* app = dc3d::Application::instance();
    if (app && app->selection()) {
        // TODO: Invert selection
        // app->selection()->invertSelection();
        m_statusBar->showInfo(tr("Invert Selection: not yet implemented"));
    }
}

// ============================================================================
// Selection Modes
// ============================================================================

void MainWindow::onSelectModeRequested()
{
    // Default click selection is always active
    m_statusBar->showInfo(tr("Click Select mode - click to select, Shift+click to add"));
}

void MainWindow::onBoxSelectModeRequested()
{
    // Box selection is triggered by dragging in the viewport
    m_statusBar->showInfo(tr("Box Select - drag in viewport to select multiple objects"));
}

void MainWindow::onLassoSelectModeRequested()
{
    // TODO: Implement lasso selection in Viewport
    m_statusBar->showInfo(tr("Lasso Select: not yet implemented"));
}

void MainWindow::onBrushSelectModeRequested()
{
    // TODO: Implement brush selection in Viewport
    m_statusBar->showInfo(tr("Brush Select: not yet implemented"));
}

// ============================================================================
// Additional View Modes
// ============================================================================

void MainWindow::onDisplayModeXRayRequested()
{
    if (m_viewport) {
        m_viewport->setDisplayMode(dc::DisplayMode::XRay);
        m_statusBar->showInfo(tr("X-Ray display mode"));
    }
}

void MainWindow::onToggleFullScreenRequested()
{
    if (isFullScreen()) {
        showNormal();
        m_statusBar->showInfo(tr("Windowed mode"));
    } else {
        showFullScreen();
        m_statusBar->showInfo(tr("Full screen mode (F11 to exit)"));
    }
}

// ============================================================================
// Mesh Tools (from Toolbar)
// ============================================================================

void MainWindow::onMeshRepairWizardRequested()
{
    if (m_menuBar && m_menuBar->meshRepairWizard()) {
        m_menuBar->meshRepairWizard()->show();
        m_menuBar->meshRepairWizard()->raise();
        m_menuBar->meshRepairWizard()->activateWindow();
    }
}

void MainWindow::onPolygonReductionRequested()
{
    if (m_menuBar && m_menuBar->polygonReductionDialog()) {
        m_menuBar->polygonReductionDialog()->show();
        m_menuBar->polygonReductionDialog()->raise();
        m_menuBar->polygonReductionDialog()->activateWindow();
    }
}

void MainWindow::onSmoothingRequested()
{
    if (m_menuBar && m_menuBar->smoothingDialog()) {
        m_menuBar->smoothingDialog()->show();
        m_menuBar->smoothingDialog()->raise();
        m_menuBar->smoothingDialog()->activateWindow();
    }
}

void MainWindow::onFillHolesRequested()
{
    if (m_menuBar && m_menuBar->holeFillDialog()) {
        m_menuBar->holeFillDialog()->show();
        m_menuBar->holeFillDialog()->raise();
        m_menuBar->holeFillDialog()->activateWindow();
    }
}

void MainWindow::onClippingBoxRequested()
{
    if (m_menuBar && m_menuBar->clippingBoxDialog()) {
        m_menuBar->clippingBoxDialog()->show();
        m_menuBar->clippingBoxDialog()->raise();
        m_menuBar->clippingBoxDialog()->activateWindow();
    }
}

// ============================================================================
// Create Tools (from Toolbar)
// ============================================================================

void MainWindow::onCreateSectionRequested()
{
    // TODO: Implement section plane creation
    m_statusBar->showInfo(tr("Section Plane: not yet implemented"));
}

void MainWindow::onCreateSketchRequested()
{
    // TODO: Implement 2D sketch mode
    m_statusBar->showInfo(tr("2D Sketch: not yet implemented"));
}
