/**
 * @file PrimitiveCreationDialog.cpp
 * @brief Implementation of PrimitiveCreationDialog
 */

#include "PrimitiveCreationDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QFrame>

PrimitiveCreationDialog::PrimitiveCreationDialog(PrimitiveType type, QWidget* parent)
    : QDialog(parent)
    , m_type(type)
    , m_viewCenter(0.0f)
{
    setWindowTitle(tr("Create %1").arg(typeToString(type)));
    setModal(true);
    setMinimumWidth(320);
    
    setupUI();
    setupConnections();
    applyStylesheet();
    updateDimensionsForType();
    
    // Apply default preset
    applyPreset(SizePreset::Medium);
}

void PrimitiveCreationDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // =========================================================================
    // Size Preset Section
    // =========================================================================
    QGroupBox* presetGroup = new QGroupBox(tr("Size Preset"), this);
    QVBoxLayout* presetLayout = new QVBoxLayout(presetGroup);
    
    m_presetCombo = new QComboBox(this);
    m_presetCombo->addItem(tr("Small (1 unit)"), static_cast<int>(SizePreset::Small));
    m_presetCombo->addItem(tr("Medium (2 units)"), static_cast<int>(SizePreset::Medium));
    m_presetCombo->addItem(tr("Large (5 units)"), static_cast<int>(SizePreset::Large));
    m_presetCombo->addItem(tr("Custom"), static_cast<int>(SizePreset::Custom));
    m_presetCombo->setCurrentIndex(1);  // Default to Medium
    presetLayout->addWidget(m_presetCombo);
    
    mainLayout->addWidget(presetGroup);
    
    // =========================================================================
    // Custom Dimensions Section
    // =========================================================================
    m_dimensionsGroup = new QGroupBox(tr("Dimensions"), this);
    QGridLayout* dimLayout = new QGridLayout(m_dimensionsGroup);
    dimLayout->setSpacing(8);
    
    // Width/Radius
    m_widthLabel = new QLabel(tr("Width:"), this);
    m_widthSpin = new QDoubleSpinBox(this);
    m_widthSpin->setRange(0.01, 1000.0);
    m_widthSpin->setDecimals(2);
    m_widthSpin->setSingleStep(0.1);
    m_widthSpin->setSuffix(tr(" units"));
    m_widthSpin->setValue(2.0);
    dimLayout->addWidget(m_widthLabel, 0, 0);
    dimLayout->addWidget(m_widthSpin, 0, 1);
    
    // Height
    m_heightLabel = new QLabel(tr("Height:"), this);
    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setRange(0.01, 1000.0);
    m_heightSpin->setDecimals(2);
    m_heightSpin->setSingleStep(0.1);
    m_heightSpin->setSuffix(tr(" units"));
    m_heightSpin->setValue(2.0);
    dimLayout->addWidget(m_heightLabel, 1, 0);
    dimLayout->addWidget(m_heightSpin, 1, 1);
    
    // Depth/Minor Radius
    m_depthLabel = new QLabel(tr("Depth:"), this);
    m_depthSpin = new QDoubleSpinBox(this);
    m_depthSpin->setRange(0.01, 1000.0);
    m_depthSpin->setDecimals(2);
    m_depthSpin->setSingleStep(0.1);
    m_depthSpin->setSuffix(tr(" units"));
    m_depthSpin->setValue(2.0);
    dimLayout->addWidget(m_depthLabel, 2, 0);
    dimLayout->addWidget(m_depthSpin, 2, 1);
    
    // Segments (for curved primitives)
    m_segmentsLabel = new QLabel(tr("Segments:"), this);
    m_segmentsSpin = new QSpinBox(this);
    m_segmentsSpin->setRange(8, 128);
    m_segmentsSpin->setSingleStep(4);
    m_segmentsSpin->setValue(32);
    dimLayout->addWidget(m_segmentsLabel, 3, 0);
    dimLayout->addWidget(m_segmentsSpin, 3, 1);
    
    mainLayout->addWidget(m_dimensionsGroup);
    
    // =========================================================================
    // Position Section
    // =========================================================================
    QGroupBox* positionGroup = new QGroupBox(tr("Position"), this);
    QVBoxLayout* posLayout = new QVBoxLayout(positionGroup);
    
    m_positionCombo = new QComboBox(this);
    m_positionCombo->addItem(tr("At View Center"), 0);
    m_positionCombo->addItem(tr("At World Origin"), 1);
    m_positionCombo->addItem(tr("At Cursor (click to place)"), 2);
    m_positionCombo->setCurrentIndex(0);
    posLayout->addWidget(m_positionCombo);
    
    mainLayout->addWidget(positionGroup);
    
    // =========================================================================
    // Options Section
    // =========================================================================
    m_selectAfterCheck = new QCheckBox(tr("Select after creation"), this);
    m_selectAfterCheck->setChecked(true);
    mainLayout->addWidget(m_selectAfterCheck);
    
    // =========================================================================
    // Preview Label
    // =========================================================================
    m_previewLabel = new QLabel(this);
    m_previewLabel->setStyleSheet("QLabel { color: #808080; font-style: italic; }");
    m_previewLabel->setWordWrap(true);
    mainLayout->addWidget(m_previewLabel);
    
    // =========================================================================
    // Buttons
    // =========================================================================
    mainLayout->addSpacing(8);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setObjectName("secondaryButton");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);
    
    m_createButton = new QPushButton(tr("Create"), this);
    m_createButton->setObjectName("primaryButton");
    m_createButton->setDefault(true);
    connect(m_createButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_createButton);
    
    mainLayout->addLayout(buttonLayout);
    
    updatePreview();
}

void PrimitiveCreationDialog::setupConnections()
{
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PrimitiveCreationDialog::onPresetChanged);
    
    connect(m_widthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PrimitiveCreationDialog::onDimensionsChanged);
    connect(m_heightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PrimitiveCreationDialog::onDimensionsChanged);
    connect(m_depthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PrimitiveCreationDialog::onDimensionsChanged);
    connect(m_segmentsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PrimitiveCreationDialog::updatePreview);
}

void PrimitiveCreationDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QGroupBox {
            background-color: #242424;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            margin-top: 12px;
            padding: 12px;
            font-weight: 600;
            font-size: 12px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 8px;
            color: #ffffff;
        }
        
        QLabel {
            color: #b3b3b3;
            font-size: 13px;
        }
        
        QComboBox {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px 12px;
            color: #ffffff;
            font-size: 13px;
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
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 6px solid #b3b3b3;
            margin-right: 8px;
        }
        
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            border: 1px solid #4a4a4a;
            selection-background-color: #0078d4;
            selection-color: #ffffff;
        }
        
        QSpinBox, QDoubleSpinBox {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 4px 8px;
            color: #ffffff;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
            font-size: 13px;
        }
        
        QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid #0078d4;
        }
        
        QSpinBox:disabled, QDoubleSpinBox:disabled {
            background-color: #2a2a2a;
            color: #5c5c5c;
            border-color: #333333;
        }
        
        QSpinBox::up-button, QDoubleSpinBox::up-button,
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            background-color: #3d3d3d;
            border: none;
            width: 16px;
        }
        
        QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
            background-color: #4a4a4a;
        }
        
        QCheckBox {
            color: #b3b3b3;
            spacing: 8px;
            font-size: 13px;
        }
        
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }
        
        QCheckBox::indicator:checked {
            background-color: #0078d4;
            border: none;
            border-radius: 3px;
        }
        
        QCheckBox::indicator:unchecked {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 3px;
        }
        
        QPushButton#primaryButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
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
            background-color: transparent;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton#secondaryButton:hover {
            background-color: #383838;
            color: #ffffff;
        }
        
        QPushButton#secondaryButton:pressed {
            background-color: #404040;
        }
    )");
}

void PrimitiveCreationDialog::updateDimensionsForType()
{
    // Customize labels and visibility based on primitive type
    switch (m_type) {
        case PrimitiveType::Cube:
            m_widthLabel->setText(tr("Width (X):"));
            m_heightLabel->setText(tr("Height (Y):"));
            m_depthLabel->setText(tr("Depth (Z):"));
            m_depthLabel->setVisible(true);
            m_depthSpin->setVisible(true);
            m_segmentsLabel->setVisible(false);
            m_segmentsSpin->setVisible(false);
            break;
            
        case PrimitiveType::Sphere:
            m_widthLabel->setText(tr("Radius:"));
            m_heightLabel->setVisible(false);
            m_heightSpin->setVisible(false);
            m_depthLabel->setVisible(false);
            m_depthSpin->setVisible(false);
            m_segmentsLabel->setVisible(true);
            m_segmentsSpin->setVisible(true);
            break;
            
        case PrimitiveType::Cylinder:
            m_widthLabel->setText(tr("Radius:"));
            m_heightLabel->setText(tr("Height:"));
            m_heightLabel->setVisible(true);
            m_heightSpin->setVisible(true);
            m_depthLabel->setVisible(false);
            m_depthSpin->setVisible(false);
            m_segmentsLabel->setVisible(true);
            m_segmentsSpin->setVisible(true);
            break;
            
        case PrimitiveType::Cone:
            m_widthLabel->setText(tr("Base Radius:"));
            m_heightLabel->setText(tr("Height:"));
            m_heightLabel->setVisible(true);
            m_heightSpin->setVisible(true);
            m_depthLabel->setVisible(false);
            m_depthSpin->setVisible(false);
            m_segmentsLabel->setVisible(true);
            m_segmentsSpin->setVisible(true);
            break;
            
        case PrimitiveType::Plane:
            m_widthLabel->setText(tr("Width:"));
            m_heightLabel->setText(tr("Length:"));
            m_heightLabel->setVisible(true);
            m_heightSpin->setVisible(true);
            m_depthLabel->setVisible(false);
            m_depthSpin->setVisible(false);
            m_segmentsLabel->setVisible(false);
            m_segmentsSpin->setVisible(false);
            break;
            
        case PrimitiveType::Torus:
            m_widthLabel->setText(tr("Major Radius:"));
            m_heightLabel->setText(tr("Minor Radius:"));
            m_heightLabel->setVisible(true);
            m_heightSpin->setVisible(true);
            m_depthLabel->setVisible(false);
            m_depthSpin->setVisible(false);
            m_segmentsLabel->setVisible(true);
            m_segmentsSpin->setVisible(true);
            break;
    }
}

void PrimitiveCreationDialog::onPresetChanged(int index)
{
    SizePreset preset = static_cast<SizePreset>(m_presetCombo->itemData(index).toInt());
    
    // Enable/disable custom dimensions based on preset
    bool isCustom = (preset == SizePreset::Custom);
    m_dimensionsGroup->setEnabled(isCustom);
    
    if (!isCustom) {
        applyPreset(preset);
    }
    
    updatePreview();
}

void PrimitiveCreationDialog::onDimensionsChanged()
{
    // Switch to custom preset when user manually changes dimensions
    if (m_presetCombo->currentIndex() != 3) {  // Not already on Custom
        m_presetCombo->blockSignals(true);
        m_presetCombo->setCurrentIndex(3);  // Set to Custom
        m_presetCombo->blockSignals(false);
        m_dimensionsGroup->setEnabled(true);
    }
    
    updatePreview();
}

void PrimitiveCreationDialog::applyPreset(SizePreset preset)
{
    float baseSize = 2.0f;  // Default medium
    
    switch (preset) {
        case SizePreset::Small:
            baseSize = 1.0f;
            break;
        case SizePreset::Medium:
            baseSize = 2.0f;
            break;
        case SizePreset::Large:
            baseSize = 5.0f;
            break;
        case SizePreset::Custom:
            return;  // Don't change values for custom
    }
    
    // Apply size based on primitive type
    m_widthSpin->blockSignals(true);
    m_heightSpin->blockSignals(true);
    m_depthSpin->blockSignals(true);
    
    switch (m_type) {
        case PrimitiveType::Cube:
            m_widthSpin->setValue(baseSize);
            m_heightSpin->setValue(baseSize);
            m_depthSpin->setValue(baseSize);
            break;
            
        case PrimitiveType::Sphere:
            m_widthSpin->setValue(baseSize / 2.0f);  // Radius
            break;
            
        case PrimitiveType::Cylinder:
        case PrimitiveType::Cone:
            m_widthSpin->setValue(baseSize / 4.0f);  // Radius
            m_heightSpin->setValue(baseSize);        // Height
            break;
            
        case PrimitiveType::Plane:
            m_widthSpin->setValue(baseSize);
            m_heightSpin->setValue(baseSize);
            break;
            
        case PrimitiveType::Torus:
            m_widthSpin->setValue(baseSize / 2.0f);   // Major radius
            m_heightSpin->setValue(baseSize / 6.0f);  // Minor radius
            break;
    }
    
    m_widthSpin->blockSignals(false);
    m_heightSpin->blockSignals(false);
    m_depthSpin->blockSignals(false);
}

void PrimitiveCreationDialog::updatePreview()
{
    QString preview;
    
    switch (m_type) {
        case PrimitiveType::Cube:
            preview = tr("Cube: %1 × %2 × %3 units")
                .arg(m_widthSpin->value(), 0, 'f', 2)
                .arg(m_heightSpin->value(), 0, 'f', 2)
                .arg(m_depthSpin->value(), 0, 'f', 2);
            break;
            
        case PrimitiveType::Sphere:
            preview = tr("Sphere: radius %1 units, %2 segments")
                .arg(m_widthSpin->value(), 0, 'f', 2)
                .arg(m_segmentsSpin->value());
            break;
            
        case PrimitiveType::Cylinder:
            preview = tr("Cylinder: radius %1, height %2 units")
                .arg(m_widthSpin->value(), 0, 'f', 2)
                .arg(m_heightSpin->value(), 0, 'f', 2);
            break;
            
        case PrimitiveType::Cone:
            preview = tr("Cone: base radius %1, height %2 units")
                .arg(m_widthSpin->value(), 0, 'f', 2)
                .arg(m_heightSpin->value(), 0, 'f', 2);
            break;
            
        case PrimitiveType::Plane:
            preview = tr("Plane: %1 × %2 units")
                .arg(m_widthSpin->value(), 0, 'f', 2)
                .arg(m_heightSpin->value(), 0, 'f', 2);
            break;
            
        case PrimitiveType::Torus:
            preview = tr("Torus: major radius %1, tube radius %2 units")
                .arg(m_widthSpin->value(), 0, 'f', 2)
                .arg(m_heightSpin->value(), 0, 'f', 2);
            break;
    }
    
    m_previewLabel->setText(preview);
}

QString PrimitiveCreationDialog::typeToString(PrimitiveType type) const
{
    switch (type) {
        case PrimitiveType::Cube:     return tr("Cube");
        case PrimitiveType::Sphere:   return tr("Sphere");
        case PrimitiveType::Cylinder: return tr("Cylinder");
        case PrimitiveType::Cone:     return tr("Cone");
        case PrimitiveType::Plane:    return tr("Plane");
        case PrimitiveType::Torus:    return tr("Torus");
        default:                      return tr("Primitive");
    }
}

void PrimitiveCreationDialog::setViewCenter(const glm::vec3& center)
{
    m_viewCenter = center;
}

PrimitiveCreationDialog::PrimitiveConfig PrimitiveCreationDialog::config() const
{
    PrimitiveConfig cfg;
    cfg.type = m_type;
    cfg.width = static_cast<float>(m_widthSpin->value());
    cfg.height = static_cast<float>(m_heightSpin->value());
    cfg.depth = static_cast<float>(m_depthSpin->value());
    cfg.segments = m_segmentsSpin->value();
    
    int posIndex = m_positionCombo->currentIndex();
    cfg.positionAtViewCenter = (posIndex == 0);
    cfg.positionAtCursor = (posIndex == 2);
    cfg.position = cfg.positionAtViewCenter ? m_viewCenter : glm::vec3(0.0f);
    
    cfg.selectAfterCreation = m_selectAfterCheck->isChecked();
    
    return cfg;
}

bool PrimitiveCreationDialog::getConfig(PrimitiveType type, PrimitiveConfig& outConfig, 
                                         QWidget* parent)
{
    PrimitiveCreationDialog dialog(type, parent);
    
    if (dialog.exec() == QDialog::Accepted) {
        outConfig = dialog.config();
        return true;
    }
    
    return false;
}
