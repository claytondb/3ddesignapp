/**
 * @file SnapSettingsDialog.cpp
 * @brief Implementation of SnapSettingsDialog
 */

#include "SnapSettingsDialog.h"
#include "core/SnapManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>

SnapSettingsDialog::SnapSettingsDialog(dc3d::core::SnapManager* snapManager,
                                       QWidget* parent)
    : QDialog(parent)
    , m_snapManager(snapManager)
{
    setWindowTitle(tr("Snap Settings"));
    setMinimumWidth(350);
    
    setupUI();
    loadSettings();
}

void SnapSettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Grid Snap Group
    m_gridGroup = new QGroupBox(tr("Grid Snapping"), this);
    QGridLayout* gridLayout = new QGridLayout(m_gridGroup);
    
    m_gridSnapEnabled = new QCheckBox(tr("Enable grid snapping"), m_gridGroup);
    gridLayout->addWidget(m_gridSnapEnabled, 0, 0, 1, 2);
    
    gridLayout->addWidget(new QLabel(tr("Grid Size:"), m_gridGroup), 1, 0);
    m_gridSize = new QDoubleSpinBox(m_gridGroup);
    m_gridSize->setRange(0.01, 1000.0);
    m_gridSize->setDecimals(2);
    m_gridSize->setSuffix(tr(" mm"));
    m_gridSize->setValue(1.0);
    gridLayout->addWidget(m_gridSize, 1, 1);
    
    gridLayout->addWidget(new QLabel(tr("Subdivisions:"), m_gridGroup), 2, 0);
    m_gridSubdivisions = new QComboBox(m_gridGroup);
    m_gridSubdivisions->addItem(tr("None"), 1);
    m_gridSubdivisions->addItem(tr("2x"), 2);
    m_gridSubdivisions->addItem(tr("4x"), 4);
    m_gridSubdivisions->addItem(tr("5x"), 5);
    m_gridSubdivisions->addItem(tr("10x"), 10);
    gridLayout->addWidget(m_gridSubdivisions, 2, 1);
    
    mainLayout->addWidget(m_gridGroup);
    
    // Object Snap Group
    m_objectGroup = new QGroupBox(tr("Object Snapping"), this);
    QVBoxLayout* objectLayout = new QVBoxLayout(m_objectGroup);
    
    m_objectSnapEnabled = new QCheckBox(tr("Enable object snapping"), m_objectGroup);
    objectLayout->addWidget(m_objectSnapEnabled);
    
    m_snapToVertices = new QCheckBox(tr("Snap to vertices"), m_objectGroup);
    objectLayout->addWidget(m_snapToVertices);
    
    m_snapToEdges = new QCheckBox(tr("Snap to edges"), m_objectGroup);
    objectLayout->addWidget(m_snapToEdges);
    
    m_snapToEdgeMidpoints = new QCheckBox(tr("Snap to edge midpoints"), m_objectGroup);
    objectLayout->addWidget(m_snapToEdgeMidpoints);
    
    m_snapToFaces = new QCheckBox(tr("Snap to faces"), m_objectGroup);
    objectLayout->addWidget(m_snapToFaces);
    
    m_snapToFaceCenters = new QCheckBox(tr("Snap to face centers"), m_objectGroup);
    objectLayout->addWidget(m_snapToFaceCenters);
    
    m_snapToOrigins = new QCheckBox(tr("Snap to object origins"), m_objectGroup);
    objectLayout->addWidget(m_snapToOrigins);
    
    mainLayout->addWidget(m_objectGroup);
    
    // Tolerance Group
    QGroupBox* toleranceGroup = new QGroupBox(tr("Tolerance"), this);
    QGridLayout* toleranceLayout = new QGridLayout(toleranceGroup);
    
    toleranceLayout->addWidget(new QLabel(tr("Screen tolerance:"), toleranceGroup), 0, 0);
    m_snapTolerance = new QDoubleSpinBox(toleranceGroup);
    m_snapTolerance->setRange(1.0, 50.0);
    m_snapTolerance->setDecimals(0);
    m_snapTolerance->setSuffix(tr(" px"));
    m_snapTolerance->setValue(10.0);
    toleranceLayout->addWidget(m_snapTolerance, 0, 1);
    
    toleranceLayout->addWidget(new QLabel(tr("World tolerance:"), toleranceGroup), 1, 0);
    m_worldTolerance = new QDoubleSpinBox(toleranceGroup);
    m_worldTolerance->setRange(0.01, 100.0);
    m_worldTolerance->setDecimals(2);
    m_worldTolerance->setSuffix(tr(" mm"));
    m_worldTolerance->setValue(0.5);
    toleranceLayout->addWidget(m_worldTolerance, 1, 1);
    
    mainLayout->addWidget(toleranceGroup);
    
    // Visual Group
    QGroupBox* visualGroup = new QGroupBox(tr("Visual Feedback"), this);
    QGridLayout* visualLayout = new QGridLayout(visualGroup);
    
    m_showIndicator = new QCheckBox(tr("Show snap indicator"), visualGroup);
    visualLayout->addWidget(m_showIndicator, 0, 0, 1, 2);
    
    visualLayout->addWidget(new QLabel(tr("Indicator size:"), visualGroup), 1, 0);
    m_indicatorSize = new QDoubleSpinBox(visualGroup);
    m_indicatorSize->setRange(4.0, 32.0);
    m_indicatorSize->setDecimals(0);
    m_indicatorSize->setSuffix(tr(" px"));
    m_indicatorSize->setValue(8.0);
    visualLayout->addWidget(m_indicatorSize, 1, 1);
    
    mainLayout->addWidget(visualGroup);
    
    // Button box
    mainLayout->addStretch();
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* resetButton = new QPushButton(tr("Reset to Defaults"), this);
    connect(resetButton, &QPushButton::clicked, this, &SnapSettingsDialog::resetToDefaults);
    buttonLayout->addWidget(resetButton);
    
    buttonLayout->addStretch();
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SnapSettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SnapSettingsDialog::apply);
    buttonLayout->addWidget(buttonBox);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect enable checkboxes to enable/disable child controls
    connect(m_gridSnapEnabled, &QCheckBox::toggled, m_gridSize, &QWidget::setEnabled);
    connect(m_gridSnapEnabled, &QCheckBox::toggled, m_gridSubdivisions, &QWidget::setEnabled);
    
    connect(m_objectSnapEnabled, &QCheckBox::toggled, [this](bool enabled) {
        m_snapToVertices->setEnabled(enabled);
        m_snapToEdges->setEnabled(enabled);
        m_snapToEdgeMidpoints->setEnabled(enabled);
        m_snapToFaces->setEnabled(enabled);
        m_snapToFaceCenters->setEnabled(enabled);
        m_snapToOrigins->setEnabled(enabled);
    });
    
    connect(m_showIndicator, &QCheckBox::toggled, m_indicatorSize, &QWidget::setEnabled);
}

void SnapSettingsDialog::loadSettings()
{
    if (!m_snapManager) return;
    
    const auto& settings = m_snapManager->settings();
    
    // Grid settings
    m_gridSnapEnabled->setChecked(settings.gridSnapEnabled);
    m_gridSize->setValue(settings.gridSize);
    
    int subdivIndex = m_gridSubdivisions->findData(static_cast<int>(settings.gridSubdivisions));
    if (subdivIndex >= 0) {
        m_gridSubdivisions->setCurrentIndex(subdivIndex);
    }
    
    // Object snap settings
    m_objectSnapEnabled->setChecked(settings.objectSnapEnabled);
    m_snapToVertices->setChecked(settings.snapToVertices);
    m_snapToEdges->setChecked(settings.snapToEdges);
    m_snapToEdgeMidpoints->setChecked(settings.snapToEdgeMidpoints);
    m_snapToFaces->setChecked(settings.snapToFaces);
    m_snapToFaceCenters->setChecked(settings.snapToFaceCenters);
    m_snapToOrigins->setChecked(settings.snapToOrigins);
    
    // Tolerance
    m_snapTolerance->setValue(settings.snapTolerance);
    m_worldTolerance->setValue(settings.worldTolerance);
    
    // Visual
    m_showIndicator->setChecked(settings.showSnapIndicator);
    m_indicatorSize->setValue(settings.indicatorSize);
    
    // Trigger enable/disable state
    emit m_gridSnapEnabled->toggled(settings.gridSnapEnabled);
    emit m_objectSnapEnabled->toggled(settings.objectSnapEnabled);
    emit m_showIndicator->toggled(settings.showSnapIndicator);
}

void SnapSettingsDialog::saveSettings()
{
    if (!m_snapManager) return;
    
    auto& settings = m_snapManager->settings();
    
    // Grid settings
    settings.gridSnapEnabled = m_gridSnapEnabled->isChecked();
    settings.gridSize = static_cast<float>(m_gridSize->value());
    settings.gridSubdivisions = static_cast<float>(
        m_gridSubdivisions->currentData().toInt());
    
    // Object snap settings
    settings.objectSnapEnabled = m_objectSnapEnabled->isChecked();
    settings.snapToVertices = m_snapToVertices->isChecked();
    settings.snapToEdges = m_snapToEdges->isChecked();
    settings.snapToEdgeMidpoints = m_snapToEdgeMidpoints->isChecked();
    settings.snapToFaces = m_snapToFaces->isChecked();
    settings.snapToFaceCenters = m_snapToFaceCenters->isChecked();
    settings.snapToOrigins = m_snapToOrigins->isChecked();
    
    // Tolerance
    settings.snapTolerance = static_cast<float>(m_snapTolerance->value());
    settings.worldTolerance = static_cast<float>(m_worldTolerance->value());
    
    // Visual
    settings.showSnapIndicator = m_showIndicator->isChecked();
    settings.indicatorSize = static_cast<float>(m_indicatorSize->value());
}

void SnapSettingsDialog::accept()
{
    apply();
    QDialog::accept();
}

void SnapSettingsDialog::apply()
{
    saveSettings();
}

void SnapSettingsDialog::resetToDefaults()
{
    // Reset to default values
    m_gridSnapEnabled->setChecked(true);
    m_gridSize->setValue(1.0);
    m_gridSubdivisions->setCurrentIndex(0);  // None
    
    m_objectSnapEnabled->setChecked(true);
    m_snapToVertices->setChecked(true);
    m_snapToEdges->setChecked(true);
    m_snapToEdgeMidpoints->setChecked(true);
    m_snapToFaces->setChecked(true);
    m_snapToFaceCenters->setChecked(true);
    m_snapToOrigins->setChecked(true);
    
    m_snapTolerance->setValue(10.0);
    m_worldTolerance->setValue(0.5);
    
    m_showIndicator->setChecked(true);
    m_indicatorSize->setValue(8.0);
}
