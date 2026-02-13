/**
 * @file SectionPlaneDialog.cpp
 * @brief Implementation of section plane dialog
 */

#include "SectionPlaneDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <cmath>

// ============================================================================
// Constructor / Destructor
// ============================================================================

SectionPlaneDialog::SectionPlaneDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Section Plane");
    setMinimumWidth(380);
    setModal(false);
    
    setupUI();
    setupConnections();
    applyStylesheet();
    
    // Initialize with defaults
    updateOffsetRange();
    updateMultipleControls();
}

// ============================================================================
// Public Methods
// ============================================================================

void SectionPlaneDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void SectionPlaneDialog::setMeshBounds(const QVector3D& min, const QVector3D& max)
{
    m_meshMin = min;
    m_meshMax = max;
    updateOffsetRange();
}

SectionPlaneParams SectionPlaneDialog::parameters() const
{
    return m_params;
}

void SectionPlaneDialog::setParameters(const SectionPlaneParams& params)
{
    m_updatingControls = true;
    
    m_params = params;
    
    // Update UI
    m_orientationCombo->setCurrentIndex(static_cast<int>(params.orientation));
    m_offsetSpinbox->setValue(params.offset);
    m_multipleCheck->setChecked(params.createMultiple);
    m_countSpinbox->setValue(params.sectionCount);
    m_startOffsetSpinbox->setValue(params.startOffset);
    m_endOffsetSpinbox->setValue(params.endOffset);
    m_spacingSpinbox->setValue(params.spacing);
    m_autoFitCheck->setChecked(params.autoFitCurves);
    m_createSketchCheck->setChecked(params.createSketch);
    m_previewCheck->setChecked(params.showPreview);
    
    updateMultipleControls();
    
    m_updatingControls = false;
}

// ============================================================================
// Private Slots
// ============================================================================

void SectionPlaneDialog::onOrientationChanged(int index)
{
    m_params.orientation = static_cast<SectionPlaneOrientation>(index);
    
    // Enable/disable custom normal button
    m_customNormalButton->setEnabled(index == static_cast<int>(SectionPlaneOrientation::Custom));
    
    // Update normal label
    QString normalStr;
    switch (m_params.orientation) {
        case SectionPlaneOrientation::XY:
            normalStr = "Normal: Z (0, 0, 1)";
            break;
        case SectionPlaneOrientation::XZ:
            normalStr = "Normal: Y (0, 1, 0)";
            break;
        case SectionPlaneOrientation::YZ:
            normalStr = "Normal: X (1, 0, 0)";
            break;
        case SectionPlaneOrientation::Custom:
            normalStr = QString("Normal: (%1, %2, %3)")
                .arg(m_params.customNormal.x(), 0, 'f', 2)
                .arg(m_params.customNormal.y(), 0, 'f', 2)
                .arg(m_params.customNormal.z(), 0, 'f', 2);
            break;
    }
    m_normalLabel->setText(normalStr);
    
    updateOffsetRange();
    emitParametersChanged();
}

void SectionPlaneDialog::onOffsetChanged(double value)
{
    if (m_updatingControls) return;
    
    m_params.offset = value;
    
    // Update slider
    m_updatingControls = true;
    double range = m_offsetSpinbox->maximum() - m_offsetSpinbox->minimum();
    int sliderVal = range > 0 ? static_cast<int>((value - m_offsetSpinbox->minimum()) / range * 10000) : 5000;
    m_offsetSlider->setValue(sliderVal);
    m_updatingControls = false;
    
    emitParametersChanged();
}

void SectionPlaneDialog::onOffsetSliderChanged(int value)
{
    if (m_updatingControls) return;
    
    double range = m_offsetSpinbox->maximum() - m_offsetSpinbox->minimum();
    double offset = m_offsetSpinbox->minimum() + (value / 10000.0) * range;
    
    m_updatingControls = true;
    m_offsetSpinbox->setValue(offset);
    m_updatingControls = false;
    
    m_params.offset = offset;
    emitParametersChanged();
}

void SectionPlaneDialog::onMultipleToggled(bool checked)
{
    m_params.createMultiple = checked;
    updateMultipleControls();
    emitParametersChanged();
}

void SectionPlaneDialog::onSectionCountChanged(int value)
{
    if (m_updatingControls) return;
    
    m_params.sectionCount = value;
    updateSpacingFromCount();
    emitParametersChanged();
}

void SectionPlaneDialog::onStartOffsetChanged(double value)
{
    if (m_updatingControls) return;
    
    m_params.startOffset = value;
    updateSpacingFromCount();
    emitParametersChanged();
}

void SectionPlaneDialog::onEndOffsetChanged(double value)
{
    if (m_updatingControls) return;
    
    m_params.endOffset = value;
    updateSpacingFromCount();
    emitParametersChanged();
}

void SectionPlaneDialog::onSpacingChanged(double value)
{
    if (m_updatingControls) return;
    
    m_params.spacing = value;
    updateCountFromSpacing();
    emitParametersChanged();
}

void SectionPlaneDialog::onAutoFitToggled(bool checked)
{
    m_params.autoFitCurves = checked;
    emitParametersChanged();
}

void SectionPlaneDialog::onCreateSketchToggled(bool checked)
{
    m_params.createSketch = checked;
}

void SectionPlaneDialog::onPreviewToggled(bool checked)
{
    m_params.showPreview = checked;
    emit previewRequested();
}

void SectionPlaneDialog::onCreateClicked()
{
    if (m_params.createMultiple) {
        emit createMultipleRequested(m_params);
    } else {
        emit createRequested(m_params);
    }
}

void SectionPlaneDialog::onCustomNormalClicked()
{
    // Simple input dialog for custom normal
    bool ok;
    QString input = QInputDialog::getText(this, "Custom Normal",
        "Enter normal vector (x, y, z):",
        QLineEdit::Normal,
        QString("%1, %2, %3")
            .arg(m_params.customNormal.x())
            .arg(m_params.customNormal.y())
            .arg(m_params.customNormal.z()),
        &ok);
    
    if (ok && !input.isEmpty()) {
        QStringList parts = input.split(",");
        if (parts.size() == 3) {
            float x = parts[0].trimmed().toFloat();
            float y = parts[1].trimmed().toFloat();
            float z = parts[2].trimmed().toFloat();
            
            QVector3D normal(x, y, z);
            if (normal.length() > 0.001f) {
                m_params.customNormal = normal.normalized();
                
                // Update label
                m_normalLabel->setText(QString("Normal: (%1, %2, %3)")
                    .arg(m_params.customNormal.x(), 0, 'f', 2)
                    .arg(m_params.customNormal.y(), 0, 'f', 2)
                    .arg(m_params.customNormal.z(), 0, 'f', 2));
                
                emitParametersChanged();
            } else {
                QMessageBox::warning(this, "Invalid Normal", 
                    "Normal vector cannot be zero length.");
            }
        }
    }
}

void SectionPlaneDialog::updateSpacingFromCount()
{
    if (m_params.sectionCount > 1) {
        double range = m_params.endOffset - m_params.startOffset;
        m_params.spacing = range / (m_params.sectionCount - 1);
        
        m_updatingControls = true;
        m_spacingSpinbox->setValue(m_params.spacing);
        m_updatingControls = false;
    }
}

void SectionPlaneDialog::updateCountFromSpacing()
{
    if (std::abs(m_params.spacing) > 0.001) {
        double range = m_params.endOffset - m_params.startOffset;
        int count = static_cast<int>(std::abs(range / m_params.spacing)) + 1;
        count = std::max(2, std::min(count, 100));
        m_params.sectionCount = count;
        
        m_updatingControls = true;
        m_countSpinbox->setValue(count);
        m_updatingControls = false;
    }
}

void SectionPlaneDialog::updateOffsetRange()
{
    // Calculate offset range based on mesh bounds and orientation
    double minOffset, maxOffset;
    
    switch (m_params.orientation) {
        case SectionPlaneOrientation::XY:
            minOffset = m_meshMin.z();
            maxOffset = m_meshMax.z();
            break;
        case SectionPlaneOrientation::XZ:
            minOffset = m_meshMin.y();
            maxOffset = m_meshMax.y();
            break;
        case SectionPlaneOrientation::YZ:
            minOffset = m_meshMin.x();
            maxOffset = m_meshMax.x();
            break;
        case SectionPlaneOrientation::Custom:
            // For custom, use maximum extent
            minOffset = -100;
            maxOffset = 100;
            break;
    }
    
    // Add 10% margin
    double range = maxOffset - minOffset;
    minOffset -= range * 0.1;
    maxOffset += range * 0.1;
    
    m_offsetSpinbox->setRange(minOffset, maxOffset);
    m_startOffsetSpinbox->setRange(minOffset, maxOffset);
    m_endOffsetSpinbox->setRange(minOffset, maxOffset);
    
    m_offsetRangeLabel->setText(QString("Range: %1 to %2 mm")
        .arg(minOffset, 0, 'f', 1)
        .arg(maxOffset, 0, 'f', 1));
    
    // Set defaults for multiple sections
    if (!m_updatingControls) {
        m_params.startOffset = minOffset + range * 0.1;
        m_params.endOffset = maxOffset - range * 0.1;
        
        m_updatingControls = true;
        m_startOffsetSpinbox->setValue(m_params.startOffset);
        m_endOffsetSpinbox->setValue(m_params.endOffset);
        m_updatingControls = false;
        
        updateSpacingFromCount();
    }
}

void SectionPlaneDialog::emitParametersChanged()
{
    if (!m_updatingControls) {
        emit parametersChanged(m_params);
        
        if (m_params.showPreview) {
            emit previewRequested();
        }
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void SectionPlaneDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // ---- Orientation Group ----
    QGroupBox* orientGroup = new QGroupBox("Plane Orientation");
    QVBoxLayout* orientLayout = new QVBoxLayout(orientGroup);
    
    QHBoxLayout* orientComboLayout = new QHBoxLayout();
    m_orientationCombo = new QComboBox();
    m_orientationCombo->addItem("XY Plane (Horizontal)", static_cast<int>(SectionPlaneOrientation::XY));
    m_orientationCombo->addItem("XZ Plane (Front)", static_cast<int>(SectionPlaneOrientation::XZ));
    m_orientationCombo->addItem("YZ Plane (Side)", static_cast<int>(SectionPlaneOrientation::YZ));
    m_orientationCombo->addItem("Custom", static_cast<int>(SectionPlaneOrientation::Custom));
    orientComboLayout->addWidget(m_orientationCombo, 1);
    
    m_customNormalButton = new QPushButton("Set Normal...");
    m_customNormalButton->setEnabled(false);
    orientComboLayout->addWidget(m_customNormalButton);
    
    orientLayout->addLayout(orientComboLayout);
    
    m_normalLabel = new QLabel("Normal: Z (0, 0, 1)");
    m_normalLabel->setStyleSheet("color: #888888; font-size: 11px;");
    orientLayout->addWidget(m_normalLabel);
    
    mainLayout->addWidget(orientGroup);
    
    // ---- Offset Group ----
    QGroupBox* offsetGroup = new QGroupBox("Plane Offset");
    QVBoxLayout* offsetLayout = new QVBoxLayout(offsetGroup);
    
    QHBoxLayout* offsetControlLayout = new QHBoxLayout();
    m_offsetSpinbox = new QDoubleSpinBox();
    m_offsetSpinbox->setDecimals(2);
    m_offsetSpinbox->setSuffix(" mm");
    m_offsetSpinbox->setRange(-1000, 1000);
    offsetControlLayout->addWidget(new QLabel("Offset:"));
    offsetControlLayout->addWidget(m_offsetSpinbox, 1);
    offsetLayout->addLayout(offsetControlLayout);
    
    m_offsetSlider = new QSlider(Qt::Horizontal);
    m_offsetSlider->setRange(0, 10000);  // Higher resolution for finer control
    m_offsetSlider->setValue(5000);
    offsetLayout->addWidget(m_offsetSlider);
    
    m_offsetRangeLabel = new QLabel("Range: -50 to 50 mm");
    m_offsetRangeLabel->setStyleSheet("color: #888888; font-size: 11px;");
    offsetLayout->addWidget(m_offsetRangeLabel);
    
    mainLayout->addWidget(offsetGroup);
    
    // ---- Multiple Sections Group ----
    QGroupBox* multiGroup = new QGroupBox("Multiple Sections");
    QVBoxLayout* multiLayout = new QVBoxLayout(multiGroup);
    
    m_multipleCheck = new QCheckBox("Create multiple sections");
    multiLayout->addWidget(m_multipleCheck);
    
    m_multipleContainer = new QWidget();
    QGridLayout* multiGrid = new QGridLayout(m_multipleContainer);
    multiGrid->setContentsMargins(20, 8, 0, 0);
    
    multiGrid->addWidget(new QLabel("Count:"), 0, 0);
    m_countSpinbox = new QSpinBox();
    m_countSpinbox->setRange(2, 100);
    m_countSpinbox->setValue(5);
    multiGrid->addWidget(m_countSpinbox, 0, 1);
    
    multiGrid->addWidget(new QLabel("Start:"), 1, 0);
    m_startOffsetSpinbox = new QDoubleSpinBox();
    m_startOffsetSpinbox->setDecimals(2);
    m_startOffsetSpinbox->setSuffix(" mm");
    multiGrid->addWidget(m_startOffsetSpinbox, 1, 1);
    
    multiGrid->addWidget(new QLabel("End:"), 2, 0);
    m_endOffsetSpinbox = new QDoubleSpinBox();
    m_endOffsetSpinbox->setDecimals(2);
    m_endOffsetSpinbox->setSuffix(" mm");
    multiGrid->addWidget(m_endOffsetSpinbox, 2, 1);
    
    multiGrid->addWidget(new QLabel("Spacing:"), 3, 0);
    m_spacingSpinbox = new QDoubleSpinBox();
    m_spacingSpinbox->setDecimals(2);
    m_spacingSpinbox->setSuffix(" mm");
    m_spacingSpinbox->setRange(0.1, 1000);
    multiGrid->addWidget(m_spacingSpinbox, 3, 1);
    
    multiGrid->addWidget(new QLabel("Distribution:"), 4, 0);
    m_distributionCombo = new QComboBox();
    m_distributionCombo->addItem("Uniform");
    m_distributionCombo->addItem("Curvature-based");
    multiGrid->addWidget(m_distributionCombo, 4, 1);
    
    multiLayout->addWidget(m_multipleContainer);
    m_multipleContainer->setEnabled(false);
    
    mainLayout->addWidget(multiGroup);
    
    // ---- Options Group ----
    QGroupBox* optionsGroup = new QGroupBox("Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    m_autoFitCheck = new QCheckBox("Auto-fit curves to section");
    m_autoFitCheck->setChecked(true);
    optionsLayout->addWidget(m_autoFitCheck);
    
    m_createSketchCheck = new QCheckBox("Create sketch from section");
    m_createSketchCheck->setChecked(true);
    optionsLayout->addWidget(m_createSketchCheck);
    
    m_previewCheck = new QCheckBox("Show preview");
    m_previewCheck->setChecked(true);
    optionsLayout->addWidget(m_previewCheck);
    
    mainLayout->addWidget(optionsGroup);
    
    // ---- Preview Info ----
    m_previewInfoLabel = new QLabel();
    m_previewInfoLabel->setStyleSheet(
        "background-color: #2D2D30;"
        "border: 1px solid #3E3E42;"
        "border-radius: 4px;"
        "padding: 8px;"
        "color: #AAAAAA;"
    );
    m_previewInfoLabel->setText("Section info will appear here");
    m_previewInfoLabel->setMinimumHeight(50);
    mainLayout->addWidget(m_previewInfoLabel);
    
    // ---- Buttons ----
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_previewButton = new QPushButton("Preview");
    buttonLayout->addWidget(m_previewButton);
    
    m_cancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(m_cancelButton);
    
    m_createButton = new QPushButton("Create");
    m_createButton->setDefault(true);
    buttonLayout->addWidget(m_createButton);
    
    mainLayout->addLayout(buttonLayout);
}

void SectionPlaneDialog::setupConnections()
{
    connect(m_orientationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SectionPlaneDialog::onOrientationChanged);
    connect(m_customNormalButton, &QPushButton::clicked,
            this, &SectionPlaneDialog::onCustomNormalClicked);
    
    connect(m_offsetSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SectionPlaneDialog::onOffsetChanged);
    connect(m_offsetSlider, &QSlider::valueChanged,
            this, &SectionPlaneDialog::onOffsetSliderChanged);
    
    connect(m_multipleCheck, &QCheckBox::toggled,
            this, &SectionPlaneDialog::onMultipleToggled);
    connect(m_countSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SectionPlaneDialog::onSectionCountChanged);
    connect(m_startOffsetSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SectionPlaneDialog::onStartOffsetChanged);
    connect(m_endOffsetSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SectionPlaneDialog::onEndOffsetChanged);
    connect(m_spacingSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SectionPlaneDialog::onSpacingChanged);
    
    connect(m_autoFitCheck, &QCheckBox::toggled,
            this, &SectionPlaneDialog::onAutoFitToggled);
    connect(m_createSketchCheck, &QCheckBox::toggled,
            this, &SectionPlaneDialog::onCreateSketchToggled);
    connect(m_previewCheck, &QCheckBox::toggled,
            this, &SectionPlaneDialog::onPreviewToggled);
    
    connect(m_previewButton, &QPushButton::clicked, this, &SectionPlaneDialog::previewRequested);
    connect(m_createButton, &QPushButton::clicked, this, &SectionPlaneDialog::onCreateClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void SectionPlaneDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #252526;
            color: #CCCCCC;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid #3E3E42;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #CCCCCC;
        }
        QLabel {
            color: #CCCCCC;
        }
        QComboBox, QSpinBox, QDoubleSpinBox {
            background-color: #3E3E42;
            border: 1px solid #555555;
            border-radius: 3px;
            padding: 4px 8px;
            color: #CCCCCC;
            min-height: 20px;
        }
        QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover {
            border-color: #007ACC;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QSlider::groove:horizontal {
            height: 4px;
            background-color: #3E3E42;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background-color: #007ACC;
            width: 16px;
            height: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            background-color: #1C97EA;
        }
        QCheckBox {
            color: #CCCCCC;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #555555;
            border-radius: 3px;
            background-color: #3E3E42;
        }
        QCheckBox::indicator:checked {
            background-color: #007ACC;
            border-color: #007ACC;
        }
        QPushButton {
            background-color: #3E3E42;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 6px 16px;
            color: #CCCCCC;
            min-width: 70px;
        }
        QPushButton:hover {
            background-color: #505054;
            border-color: #007ACC;
        }
        QPushButton:pressed {
            background-color: #007ACC;
        }
        QPushButton:default {
            border-color: #007ACC;
        }
    )");
}

void SectionPlaneDialog::updateMultipleControls()
{
    m_multipleContainer->setEnabled(m_params.createMultiple);
    
    // Update button text
    if (m_params.createMultiple) {
        m_createButton->setText(QString("Create %1 Sections").arg(m_params.sectionCount));
    } else {
        m_createButton->setText("Create");
    }
}
