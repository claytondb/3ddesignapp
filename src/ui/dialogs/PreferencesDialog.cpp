#include "PreferencesDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QColorDialog>
#include <QMessageBox>
#include <QApplication>

// Default values
static const QString DEFAULT_THEME = "dark";
static const QString DEFAULT_LANGUAGE = "en";
static const int DEFAULT_RECENT_FILES_COUNT = 10;
static const int DEFAULT_AUTO_SAVE_INTERVAL = 5;  // minutes

static const QColor DEFAULT_VIEWPORT_BACKGROUND(30, 30, 30);  // #1e1e1e
static const bool DEFAULT_GRID_VISIBLE = true;
static const double DEFAULT_GRID_SPACING = 10.0;
static const double DEFAULT_CAMERA_FOV = 45.0;

static const QString DEFAULT_DISPLAY_UNITS = "mm";
static const int DEFAULT_DECIMAL_PRECISION = 3;

static const int DEFAULT_UNDO_LIMIT = 100;
static const int DEFAULT_LARGE_FILE_THRESHOLD = 100;  // MB

static const bool DEFAULT_INVERT_ZOOM = false;
static const double DEFAULT_ROTATION_SENSITIVITY = 1.0;
static const double DEFAULT_PAN_SENSITIVITY = 1.0;

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_settingsModified(false)
{
    setWindowTitle(tr("Preferences"));
    setMinimumSize(500, 450);
    setModal(true);
    
    setupUI();
    setupConnections();
    applyStylesheet();
    loadSettings();
}

// Static accessor methods
QString PreferencesDialog::theme()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/general/theme", DEFAULT_THEME).toString();
}

QString PreferencesDialog::language()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/general/language", DEFAULT_LANGUAGE).toString();
}

int PreferencesDialog::recentFilesCount()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/general/recentFilesCount", DEFAULT_RECENT_FILES_COUNT).toInt();
}

int PreferencesDialog::autoSaveInterval()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/general/autoSaveInterval", DEFAULT_AUTO_SAVE_INTERVAL).toInt();
}

QColor PreferencesDialog::viewportBackgroundColor()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/viewport/backgroundColor", DEFAULT_VIEWPORT_BACKGROUND).value<QColor>();
}

bool PreferencesDialog::gridVisibleByDefault()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/viewport/gridVisible", DEFAULT_GRID_VISIBLE).toBool();
}

double PreferencesDialog::gridSpacing()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/viewport/gridSpacing", DEFAULT_GRID_SPACING).toDouble();
}

double PreferencesDialog::defaultCameraFOV()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/viewport/cameraFOV", DEFAULT_CAMERA_FOV).toDouble();
}

QString PreferencesDialog::displayUnits()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/units/displayUnits", DEFAULT_DISPLAY_UNITS).toString();
}

int PreferencesDialog::decimalPrecision()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/units/precision", DEFAULT_DECIMAL_PRECISION).toInt();
}

int PreferencesDialog::undoHistoryLimit()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/performance/undoLimit", DEFAULT_UNDO_LIMIT).toInt();
}

qint64 PreferencesDialog::largeFileWarningThreshold()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    int mb = settings.value("preferences/performance/largeFileThreshold", DEFAULT_LARGE_FILE_THRESHOLD).toInt();
    return static_cast<qint64>(mb) * 1024 * 1024;  // Convert MB to bytes
}

bool PreferencesDialog::invertZoomDirection()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/mouse/invertZoom", DEFAULT_INVERT_ZOOM).toBool();
}

double PreferencesDialog::rotationSensitivity()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/mouse/rotationSensitivity", DEFAULT_ROTATION_SENSITIVITY).toDouble();
}

double PreferencesDialog::panSensitivity()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    return settings.value("preferences/mouse/panSensitivity", DEFAULT_PAN_SENSITIVITY).toDouble();
}

void PreferencesDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Tab widget
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    // Create tabs
    setupGeneralTab();
    setupViewportTab();
    setupUnitsTab();
    setupPerformanceTab();
    setupMouseTab();
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    
    m_restoreDefaultsButton = new QPushButton(tr("Restore Defaults"));
    m_restoreDefaultsButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_restoreDefaultsButton);
    
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_cancelButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_cancelButton);
    
    m_applyButton = new QPushButton(tr("Apply"));
    m_applyButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_applyButton);
    
    m_okButton = new QPushButton(tr("OK"));
    m_okButton->setObjectName("primaryButton");
    m_okButton->setDefault(true);
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);
}

void PreferencesDialog::setupGeneralTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);
    
    // Appearance group
    QGroupBox* appearanceGroup = new QGroupBox(tr("Appearance"));
    QFormLayout* appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->setSpacing(12);
    
    m_themeCombo = new QComboBox();
    m_themeCombo->addItem(tr("Dark"), "dark");
    m_themeCombo->addItem(tr("Light"), "light");
    appearanceLayout->addRow(tr("Theme:"), m_themeCombo);
    
    m_languageCombo = new QComboBox();
    m_languageCombo->addItem(tr("English"), "en");
    // Placeholder for future languages
    m_languageCombo->setToolTip(tr("Additional languages coming in future updates"));
    appearanceLayout->addRow(tr("Language:"), m_languageCombo);
    
    layout->addWidget(appearanceGroup);
    
    // Files group
    QGroupBox* filesGroup = new QGroupBox(tr("Files"));
    QFormLayout* filesLayout = new QFormLayout(filesGroup);
    filesLayout->setSpacing(12);
    
    m_recentFilesSpinbox = new QSpinBox();
    m_recentFilesSpinbox->setRange(5, 20);
    m_recentFilesSpinbox->setSuffix(tr(" files"));
    m_recentFilesSpinbox->setToolTip(tr("Number of recent files to remember (5-20)"));
    filesLayout->addRow(tr("Recent files count:"), m_recentFilesSpinbox);
    
    m_autoSaveSpinbox = new QSpinBox();
    m_autoSaveSpinbox->setRange(0, 30);
    m_autoSaveSpinbox->setSuffix(tr(" min"));
    m_autoSaveSpinbox->setSpecialValueText(tr("Disabled"));
    m_autoSaveSpinbox->setToolTip(tr("Auto-save interval in minutes (0 = disabled)"));
    filesLayout->addRow(tr("Auto-save interval:"), m_autoSaveSpinbox);
    
    layout->addWidget(filesGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(tab, tr("General"));
}

void PreferencesDialog::setupViewportTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);
    
    // Display group
    QGroupBox* displayGroup = new QGroupBox(tr("Display"));
    QFormLayout* displayLayout = new QFormLayout(displayGroup);
    displayLayout->setSpacing(12);
    
    // Background color with button
    QHBoxLayout* colorLayout = new QHBoxLayout();
    m_backgroundColorButton = new QPushButton();
    m_backgroundColorButton->setFixedSize(60, 24);
    m_backgroundColorButton->setToolTip(tr("Click to choose viewport background color"));
    colorLayout->addWidget(m_backgroundColorButton);
    colorLayout->addStretch();
    displayLayout->addRow(tr("Background color:"), colorLayout);
    
    layout->addWidget(displayGroup);
    
    // Grid group
    QGroupBox* gridGroup = new QGroupBox(tr("Grid"));
    QFormLayout* gridLayout = new QFormLayout(gridGroup);
    gridLayout->setSpacing(12);
    
    m_gridVisibleCheck = new QCheckBox(tr("Show grid by default"));
    gridLayout->addRow(m_gridVisibleCheck);
    
    m_gridSpacingSpinbox = new QDoubleSpinBox();
    m_gridSpacingSpinbox->setRange(0.1, 1000.0);
    m_gridSpacingSpinbox->setDecimals(2);
    m_gridSpacingSpinbox->setSuffix(tr(" units"));
    m_gridSpacingSpinbox->setToolTip(tr("Spacing between grid lines"));
    gridLayout->addRow(tr("Grid spacing:"), m_gridSpacingSpinbox);
    
    layout->addWidget(gridGroup);
    
    // Camera group
    QGroupBox* cameraGroup = new QGroupBox(tr("Camera"));
    QFormLayout* cameraLayout = new QFormLayout(cameraGroup);
    cameraLayout->setSpacing(12);
    
    m_cameraFOVSpinbox = new QDoubleSpinBox();
    m_cameraFOVSpinbox->setRange(10.0, 120.0);
    m_cameraFOVSpinbox->setDecimals(1);
    m_cameraFOVSpinbox->setSuffix(tr("°"));
    m_cameraFOVSpinbox->setToolTip(tr("Default camera field of view (10-120 degrees)"));
    cameraLayout->addRow(tr("Default FOV:"), m_cameraFOVSpinbox);
    
    layout->addWidget(cameraGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(tab, tr("Viewport"));
}

void PreferencesDialog::setupUnitsTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);
    
    // Units group
    QGroupBox* unitsGroup = new QGroupBox(tr("Display Units"));
    QFormLayout* unitsLayout = new QFormLayout(unitsGroup);
    unitsLayout->setSpacing(12);
    
    m_unitsCombo = new QComboBox();
    m_unitsCombo->addItem(tr("Millimeters (mm)"), "mm");
    m_unitsCombo->addItem(tr("Centimeters (cm)"), "cm");
    m_unitsCombo->addItem(tr("Inches (in)"), "in");
    m_unitsCombo->setToolTip(tr("Units for displaying measurements"));
    unitsLayout->addRow(tr("Display units:"), m_unitsCombo);
    
    m_precisionSpinbox = new QSpinBox();
    m_precisionSpinbox->setRange(0, 8);
    m_precisionSpinbox->setSuffix(tr(" digits"));
    m_precisionSpinbox->setToolTip(tr("Number of decimal places to display (0-8)"));
    unitsLayout->addRow(tr("Decimal precision:"), m_precisionSpinbox);
    
    layout->addWidget(unitsGroup);
    
    // Info label
    QLabel* infoLabel = new QLabel(tr(
        "<i>Note: Units affect display only. Internal calculations use millimeters.</i>"
    ));
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #808080; font-size: 11px;");
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    m_tabWidget->addTab(tab, tr("Units"));
}

void PreferencesDialog::setupPerformanceTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);
    
    // History group
    QGroupBox* historyGroup = new QGroupBox(tr("History"));
    QFormLayout* historyLayout = new QFormLayout(historyGroup);
    historyLayout->setSpacing(12);
    
    m_undoLimitSpinbox = new QSpinBox();
    m_undoLimitSpinbox->setRange(10, 500);
    m_undoLimitSpinbox->setSuffix(tr(" actions"));
    m_undoLimitSpinbox->setToolTip(tr("Maximum number of undo steps (10-500)"));
    historyLayout->addRow(tr("Undo history limit:"), m_undoLimitSpinbox);
    
    layout->addWidget(historyGroup);
    
    // Files group
    QGroupBox* filesGroup = new QGroupBox(tr("Files"));
    QFormLayout* filesLayout = new QFormLayout(filesGroup);
    filesLayout->setSpacing(12);
    
    m_largeFileThresholdSpinbox = new QSpinBox();
    m_largeFileThresholdSpinbox->setRange(10, 2000);
    m_largeFileThresholdSpinbox->setSuffix(tr(" MB"));
    m_largeFileThresholdSpinbox->setToolTip(tr("Show warning when importing files larger than this size"));
    filesLayout->addRow(tr("Large file warning:"), m_largeFileThresholdSpinbox);
    
    layout->addWidget(filesGroup);
    
    // Info label
    QLabel* infoLabel = new QLabel(tr(
        "<i>Higher undo limits use more memory. Reduce if experiencing performance issues.</i>"
    ));
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #808080; font-size: 11px;");
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    m_tabWidget->addTab(tab, tr("Performance"));
}

void PreferencesDialog::setupMouseTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);
    
    // Zoom group
    QGroupBox* zoomGroup = new QGroupBox(tr("Zoom"));
    QFormLayout* zoomLayout = new QFormLayout(zoomGroup);
    zoomLayout->setSpacing(12);
    
    m_invertZoomCheck = new QCheckBox(tr("Invert zoom direction"));
    m_invertZoomCheck->setToolTip(tr("Swap the direction of mouse wheel zoom"));
    zoomLayout->addRow(m_invertZoomCheck);
    
    layout->addWidget(zoomGroup);
    
    // Sensitivity group
    QGroupBox* sensitivityGroup = new QGroupBox(tr("Sensitivity"));
    QFormLayout* sensitivityLayout = new QFormLayout(sensitivityGroup);
    sensitivityLayout->setSpacing(12);
    
    m_rotationSensitivitySpinbox = new QDoubleSpinBox();
    m_rotationSensitivitySpinbox->setRange(0.1, 5.0);
    m_rotationSensitivitySpinbox->setDecimals(2);
    m_rotationSensitivitySpinbox->setSingleStep(0.1);
    m_rotationSensitivitySpinbox->setToolTip(tr("Camera rotation sensitivity (0.1-5.0, default: 1.0)"));
    sensitivityLayout->addRow(tr("Rotation sensitivity:"), m_rotationSensitivitySpinbox);
    
    m_panSensitivitySpinbox = new QDoubleSpinBox();
    m_panSensitivitySpinbox->setRange(0.1, 5.0);
    m_panSensitivitySpinbox->setDecimals(2);
    m_panSensitivitySpinbox->setSingleStep(0.1);
    m_panSensitivitySpinbox->setToolTip(tr("Camera pan sensitivity (0.1-5.0, default: 1.0)"));
    sensitivityLayout->addRow(tr("Pan sensitivity:"), m_panSensitivitySpinbox);
    
    layout->addWidget(sensitivityGroup);
    
    // Info label
    QLabel* infoLabel = new QLabel(tr(
        "<i>Sensitivity of 1.0 is the default. Lower values = slower movement.</i>"
    ));
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #808080; font-size: 11px;");
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    m_tabWidget->addTab(tab, tr("Mouse"));
}

void PreferencesDialog::setupConnections()
{
    // Button connections
    connect(m_okButton, &QPushButton::clicked, this, &PreferencesDialog::onOkClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &PreferencesDialog::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &PreferencesDialog::onCancelClicked);
    connect(m_restoreDefaultsButton, &QPushButton::clicked, this, &PreferencesDialog::onRestoreDefaultsClicked);
    
    // Background color button
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &PreferencesDialog::onBackgroundColorClicked);
    
    // Track modifications (could be expanded to enable/disable Apply button)
    auto markModified = [this]() { m_settingsModified = true; };
    
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), markModified);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), markModified);
    connect(m_recentFilesSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), markModified);
    connect(m_autoSaveSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), markModified);
    connect(m_gridVisibleCheck, &QCheckBox::toggled, markModified);
    connect(m_gridSpacingSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), markModified);
    connect(m_cameraFOVSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), markModified);
    connect(m_unitsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), markModified);
    connect(m_precisionSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), markModified);
    connect(m_undoLimitSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), markModified);
    connect(m_largeFileThresholdSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), markModified);
    connect(m_invertZoomCheck, &QCheckBox::toggled, markModified);
    connect(m_rotationSensitivitySpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), markModified);
    connect(m_panSensitivitySpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), markModified);
}

void PreferencesDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QTabWidget::pane {
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            background-color: #2d2d2d;
        }
        
        QTabBar::tab {
            background-color: #242424;
            color: #b3b3b3;
            padding: 8px 16px;
            border: 1px solid #4a4a4a;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            margin-right: 2px;
        }
        
        QTabBar::tab:selected {
            background-color: #2d2d2d;
            color: #ffffff;
            border-bottom: 1px solid #2d2d2d;
        }
        
        QTabBar::tab:hover:!selected {
            background-color: #383838;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 8px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 4px;
            color: #0078d4;
        }
        
        QLabel {
            color: #b3b3b3;
        }
        
        QComboBox, QSpinBox, QDoubleSpinBox {
            background-color: #242424;
            color: #ffffff;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 4px 8px;
            min-width: 120px;
            min-height: 24px;
        }
        
        QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover {
            border-color: #0078d4;
        }
        
        QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: #0078d4;
            outline: none;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        
        QComboBox QAbstractItemView {
            background-color: #242424;
            color: #ffffff;
            border: 1px solid #4a4a4a;
            selection-background-color: #0078d4;
        }
        
        QCheckBox {
            color: #b3b3b3;
            spacing: 8px;
        }
        
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #4a4a4a;
            border-radius: 3px;
            background-color: #242424;
        }
        
        QCheckBox::indicator:checked {
            background-color: #0078d4;
            border-color: #0078d4;
        }
        
        QCheckBox::indicator:hover {
            border-color: #0078d4;
        }
        
        QPushButton#primaryButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 24px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton#primaryButton:hover {
            background-color: #1a88e0;
        }
        
        QPushButton#primaryButton:pressed {
            background-color: #0066b8;
        }
        
        QPushButton#secondaryButton {
            background-color: #383838;
            color: #ffffff;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 8px 24px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton#secondaryButton:hover {
            background-color: #404040;
            border-color: #5a5a5a;
        }
        
        QPushButton#secondaryButton:pressed {
            background-color: #303030;
        }
    )");
}

void PreferencesDialog::loadSettings()
{
    // General
    int themeIndex = m_themeCombo->findData(theme());
    if (themeIndex >= 0) m_themeCombo->setCurrentIndex(themeIndex);
    
    int langIndex = m_languageCombo->findData(language());
    if (langIndex >= 0) m_languageCombo->setCurrentIndex(langIndex);
    
    m_recentFilesSpinbox->setValue(recentFilesCount());
    m_autoSaveSpinbox->setValue(autoSaveInterval());
    
    // Viewport
    m_backgroundColor = viewportBackgroundColor();
    updateBackgroundColorButton();
    m_gridVisibleCheck->setChecked(gridVisibleByDefault());
    m_gridSpacingSpinbox->setValue(gridSpacing());
    m_cameraFOVSpinbox->setValue(defaultCameraFOV());
    
    // Units
    int unitsIndex = m_unitsCombo->findData(displayUnits());
    if (unitsIndex >= 0) m_unitsCombo->setCurrentIndex(unitsIndex);
    m_precisionSpinbox->setValue(decimalPrecision());
    
    // Performance
    m_undoLimitSpinbox->setValue(undoHistoryLimit());
    m_largeFileThresholdSpinbox->setValue(static_cast<int>(largeFileWarningThreshold() / (1024 * 1024)));
    
    // Mouse
    m_invertZoomCheck->setChecked(invertZoomDirection());
    m_rotationSensitivitySpinbox->setValue(rotationSensitivity());
    m_panSensitivitySpinbox->setValue(panSensitivity());
    
    m_settingsModified = false;
}

void PreferencesDialog::saveSettings()
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    
    // General
    settings.setValue("preferences/general/theme", m_themeCombo->currentData().toString());
    settings.setValue("preferences/general/language", m_languageCombo->currentData().toString());
    settings.setValue("preferences/general/recentFilesCount", m_recentFilesSpinbox->value());
    settings.setValue("preferences/general/autoSaveInterval", m_autoSaveSpinbox->value());
    
    // Viewport
    settings.setValue("preferences/viewport/backgroundColor", m_backgroundColor);
    settings.setValue("preferences/viewport/gridVisible", m_gridVisibleCheck->isChecked());
    settings.setValue("preferences/viewport/gridSpacing", m_gridSpacingSpinbox->value());
    settings.setValue("preferences/viewport/cameraFOV", m_cameraFOVSpinbox->value());
    
    // Units
    settings.setValue("preferences/units/displayUnits", m_unitsCombo->currentData().toString());
    settings.setValue("preferences/units/precision", m_precisionSpinbox->value());
    
    // Performance
    settings.setValue("preferences/performance/undoLimit", m_undoLimitSpinbox->value());
    settings.setValue("preferences/performance/largeFileThreshold", m_largeFileThresholdSpinbox->value());
    
    // Mouse
    settings.setValue("preferences/mouse/invertZoom", m_invertZoomCheck->isChecked());
    settings.setValue("preferences/mouse/rotationSensitivity", m_rotationSensitivitySpinbox->value());
    settings.setValue("preferences/mouse/panSensitivity", m_panSensitivitySpinbox->value());
    
    settings.sync();
}

void PreferencesDialog::applySettings()
{
    QString previousTheme = theme();
    
    saveSettings();
    
    // Emit theme change if needed
    QString newTheme = m_themeCombo->currentData().toString();
    if (newTheme != previousTheme) {
        emit themeChanged(newTheme);
    }
    
    emit settingsChanged();
    m_settingsModified = false;
}

void PreferencesDialog::restoreDefaults()
{
    // General
    m_themeCombo->setCurrentIndex(m_themeCombo->findData(DEFAULT_THEME));
    m_languageCombo->setCurrentIndex(m_languageCombo->findData(DEFAULT_LANGUAGE));
    m_recentFilesSpinbox->setValue(DEFAULT_RECENT_FILES_COUNT);
    m_autoSaveSpinbox->setValue(DEFAULT_AUTO_SAVE_INTERVAL);
    
    // Viewport
    m_backgroundColor = DEFAULT_VIEWPORT_BACKGROUND;
    updateBackgroundColorButton();
    m_gridVisibleCheck->setChecked(DEFAULT_GRID_VISIBLE);
    m_gridSpacingSpinbox->setValue(DEFAULT_GRID_SPACING);
    m_cameraFOVSpinbox->setValue(DEFAULT_CAMERA_FOV);
    
    // Units
    m_unitsCombo->setCurrentIndex(m_unitsCombo->findData(DEFAULT_DISPLAY_UNITS));
    m_precisionSpinbox->setValue(DEFAULT_DECIMAL_PRECISION);
    
    // Performance
    m_undoLimitSpinbox->setValue(DEFAULT_UNDO_LIMIT);
    m_largeFileThresholdSpinbox->setValue(DEFAULT_LARGE_FILE_THRESHOLD);
    
    // Mouse
    m_invertZoomCheck->setChecked(DEFAULT_INVERT_ZOOM);
    m_rotationSensitivitySpinbox->setValue(DEFAULT_ROTATION_SENSITIVITY);
    m_panSensitivitySpinbox->setValue(DEFAULT_PAN_SENSITIVITY);
    
    m_settingsModified = true;
}

void PreferencesDialog::onApplyClicked()
{
    applySettings();
}

void PreferencesDialog::onOkClicked()
{
    applySettings();
    accept();
}

void PreferencesDialog::onCancelClicked()
{
    reject();
}

void PreferencesDialog::onRestoreDefaultsClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Restore Defaults"),
        tr("Are you sure you want to restore all preferences to their default values?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        restoreDefaults();
    }
}

void PreferencesDialog::onBackgroundColorClicked()
{
    QColor color = QColorDialog::getColor(
        m_backgroundColor,
        this,
        tr("Choose Background Color"),
        QColorDialog::DontUseNativeDialog
    );
    
    if (color.isValid()) {
        m_backgroundColor = color;
        updateBackgroundColorButton();
        m_settingsModified = true;
    }
}

void PreferencesDialog::updateBackgroundColorButton()
{
    QString colorStyle = QString(
        "background-color: %1; border: 1px solid #4a4a4a; border-radius: 4px;"
    ).arg(m_backgroundColor.name());
    m_backgroundColorButton->setStyleSheet(colorStyle);
}
