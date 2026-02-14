#include "ExtrudeDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

ExtrudeDialog::ExtrudeDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Extrude"));
    setMinimumWidth(380);
    setModal(false);  // Allow interaction with viewport
    
    setupUI();
    setupConnections();
    applyStylesheet();
}

void ExtrudeDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ===============================
    // Direction Group
    // ===============================
    m_directionGroup = new QGroupBox(tr("Direction"));
    QVBoxLayout* dirLayout = new QVBoxLayout(m_directionGroup);
    
    m_directionCombo = new QComboBox();
    m_directionCombo->addItem(tr("Normal to Sketch"), static_cast<int>(Direction::Normal));
    m_directionCombo->addItem(tr("Custom Vector"), static_cast<int>(Direction::CustomVector));
    m_directionCombo->addItem(tr("To Point"), static_cast<int>(Direction::ToPoint));
    m_directionCombo->addItem(tr("To Surface"), static_cast<int>(Direction::ToSurface));
    dirLayout->addWidget(m_directionCombo);
    
    // Custom direction widget (shown when Custom Vector selected)
    m_customDirWidget = new QWidget();
    QHBoxLayout* customDirLayout = new QHBoxLayout(m_customDirWidget);
    customDirLayout->setContentsMargins(0, 8, 0, 0);
    
    m_dirXSpin = new QDoubleSpinBox();
    m_dirXSpin->setRange(-1.0, 1.0);
    m_dirXSpin->setDecimals(4);
    m_dirXSpin->setSingleStep(0.1);
    m_dirXSpin->setValue(0.0);
    m_dirXSpin->setPrefix("X: ");
    
    m_dirYSpin = new QDoubleSpinBox();
    m_dirYSpin->setRange(-1.0, 1.0);
    m_dirYSpin->setDecimals(4);
    m_dirYSpin->setSingleStep(0.1);
    m_dirYSpin->setValue(0.0);
    m_dirYSpin->setPrefix("Y: ");
    
    m_dirZSpin = new QDoubleSpinBox();
    m_dirZSpin->setRange(-1.0, 1.0);
    m_dirZSpin->setDecimals(4);
    m_dirZSpin->setSingleStep(0.1);
    m_dirZSpin->setValue(1.0);
    m_dirZSpin->setPrefix("Z: ");
    
    m_pickDirButton = new QPushButton(tr("Pick"));
    m_pickDirButton->setToolTip(tr("Pick direction by selecting two points or an edge"));
    m_pickDirButton->setMaximumWidth(60);
    
    customDirLayout->addWidget(m_dirXSpin);
    customDirLayout->addWidget(m_dirYSpin);
    customDirLayout->addWidget(m_dirZSpin);
    customDirLayout->addWidget(m_pickDirButton);
    
    dirLayout->addWidget(m_customDirWidget);
    m_customDirWidget->setVisible(false);
    
    m_directionPreview = new QLabel();
    m_directionPreview->setStyleSheet("color: #888; font-size: 11px;");
    dirLayout->addWidget(m_directionPreview);
    
    mainLayout->addWidget(m_directionGroup);

    // ===============================
    // Distance Group
    // ===============================
    m_distanceGroup = new QGroupBox(tr("Distance"));
    QVBoxLayout* distLayout = new QVBoxLayout(m_distanceGroup);
    
    QHBoxLayout* distValueLayout = new QHBoxLayout();
    
    m_distanceSpinbox = new QDoubleSpinBox();
    m_distanceSpinbox->setRange(0.001, 10000.0);
    m_distanceSpinbox->setDecimals(4);
    m_distanceSpinbox->setSingleStep(1.0);
    m_distanceSpinbox->setValue(10.0);
    m_distanceSpinbox->setSuffix(" mm");
    m_distanceSpinbox->setMinimumWidth(120);
    
    m_flipDirection = new QCheckBox(tr("Flip"));
    m_flipDirection->setToolTip(tr("Reverse extrusion direction"));
    
    distValueLayout->addWidget(m_distanceSpinbox, 1);
    distValueLayout->addWidget(m_flipDirection);
    distLayout->addLayout(distValueLayout);
    
    mainLayout->addWidget(m_distanceGroup);

    // ===============================
    // Draft Angle Group
    // ===============================
    m_draftGroup = new QGroupBox(tr("Draft Angle"));
    m_draftGroup->setCheckable(true);
    m_draftGroup->setChecked(false);
    QFormLayout* draftLayout = new QFormLayout(m_draftGroup);
    
    m_draftAngleSpinbox = new QDoubleSpinBox();
    m_draftAngleSpinbox->setRange(-89.0, 89.0);
    m_draftAngleSpinbox->setDecimals(2);
    m_draftAngleSpinbox->setSingleStep(1.0);
    m_draftAngleSpinbox->setValue(0.0);
    m_draftAngleSpinbox->setSuffix("°");
    m_draftAngleSpinbox->setToolTip(tr("Taper angle (positive = expand, negative = shrink)"));
    
    m_draftDirectionCombo = new QComboBox();
    m_draftDirectionCombo->addItem(tr("Outward"));
    m_draftDirectionCombo->addItem(tr("Inward"));
    
    draftLayout->addRow(tr("Angle:"), m_draftAngleSpinbox);
    draftLayout->addRow(tr("Direction:"), m_draftDirectionCombo);
    
    mainLayout->addWidget(m_draftGroup);

    // ===============================
    // Two-Sided Group
    // ===============================
    m_twoSidedGroup = new QGroupBox(tr("Two-Sided"));
    m_twoSidedGroup->setCheckable(true);
    m_twoSidedGroup->setChecked(false);
    QFormLayout* twoSidedLayout = new QFormLayout(m_twoSidedGroup);
    
    m_ratioSpinbox = new QDoubleSpinBox();
    m_ratioSpinbox->setRange(0.0, 1.0);
    m_ratioSpinbox->setDecimals(2);
    m_ratioSpinbox->setSingleStep(0.1);
    m_ratioSpinbox->setValue(0.5);
    m_ratioSpinbox->setToolTip(tr("Ratio of distance in positive direction (0.5 = symmetric)"));
    
    m_ratioLabel = new QLabel(tr("50% / 50%"));
    
    twoSidedLayout->addRow(tr("Ratio:"), m_ratioSpinbox);
    twoSidedLayout->addRow(tr("Distribution:"), m_ratioLabel);
    
    mainLayout->addWidget(m_twoSidedGroup);

    // ===============================
    // Options
    // ===============================
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    
    m_capEndsCheck = new QCheckBox(tr("Cap Ends"));
    m_capEndsCheck->setChecked(true);
    m_capEndsCheck->setToolTip(tr("Create solid by capping the ends"));
    
    m_autoPreviewCheck = new QCheckBox(tr("Auto Preview"));
    m_autoPreviewCheck->setChecked(true);
    m_autoPreviewCheck->setToolTip(tr("Update preview automatically when parameters change"));
    
    optionsLayout->addWidget(m_capEndsCheck);
    optionsLayout->addStretch();
    optionsLayout->addWidget(m_autoPreviewCheck);
    
    mainLayout->addLayout(optionsLayout);

    // ===============================
    // Buttons
    // ===============================
    mainLayout->addStretch();
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_previewButton = new QPushButton(tr("Preview"));
    m_previewButton->setToolTip(tr("Generate preview of extrusion"));
    
    buttonLayout->addWidget(m_previewButton);
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_applyButton = new QPushButton(tr("Apply"));
    m_okButton = new QPushButton(tr("OK"));
    m_okButton->setDefault(true);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Update direction preview
    updateDirectionWidgets();
}

void ExtrudeDialog::setupConnections()
{
    connect(m_directionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExtrudeDialog::onDirectionChanged);
    
    connect(m_distanceSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ExtrudeDialog::onDistanceChanged);
    
    connect(m_draftGroup, &QGroupBox::toggled, this, &ExtrudeDialog::updatePreview);
    connect(m_draftAngleSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ExtrudeDialog::onDraftAngleChanged);
    
    connect(m_twoSidedGroup, &QGroupBox::toggled, this, &ExtrudeDialog::onTwoSidedToggled);
    connect(m_ratioSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ExtrudeDialog::onRatioChanged);
    
    connect(m_dirXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ExtrudeDialog::onCustomDirectionChanged);
    connect(m_dirYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ExtrudeDialog::onCustomDirectionChanged);
    connect(m_dirZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ExtrudeDialog::onCustomDirectionChanged);
    
    connect(m_pickDirButton, &QPushButton::clicked,
            this, &ExtrudeDialog::onPickDirectionClicked);
    
    connect(m_autoPreviewCheck, &QCheckBox::toggled,
            this, &ExtrudeDialog::onPreviewToggled);
    
    connect(m_previewButton, &QPushButton::clicked,
            this, &ExtrudeDialog::previewRequested);
    
    connect(m_flipDirection, &QCheckBox::toggled, this, &ExtrudeDialog::updatePreview);
    connect(m_capEndsCheck, &QCheckBox::toggled, this, &ExtrudeDialog::updatePreview);
    
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &ExtrudeDialog::onApplyClicked);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ExtrudeDialog::applyStylesheet()
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
        
        QLabel {
            color: #b3b3b3;
            font-size: 13px;
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
        
        QDoubleSpinBox, QSpinBox {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px 8px;
            color: #ffffff;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
            font-size: 13px;
        }
        
        QDoubleSpinBox:focus, QSpinBox:focus {
            border: 1px solid #0078d4;
        }
        
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button,
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #3d3d3d;
            border: none;
            width: 16px;
        }
        
        QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover,
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #4a4a4a;
        }
        
        QPushButton {
            background-color: transparent;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton:hover {
            background-color: #383838;
            color: #ffffff;
        }
        
        QPushButton:pressed {
            background-color: #404040;
        }
        
        QPushButton:default {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
        }
        
        QPushButton:default:hover {
            background-color: #1a88e0;
        }
        
        QPushButton:default:pressed {
            background-color: #0066b8;
        }
        
        QPushButton#previewButton {
            background-color: #383838;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            min-width: 70px;
        }
        
        QPushButton#previewButton:hover {
            background-color: #444444;
            color: #ffffff;
        }
    )");
}

void ExtrudeDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void ExtrudeDialog::setSketchNormal(const glm::vec3& normal)
{
    m_sketchNormal = normal;
    updateDirectionWidgets();
}

void ExtrudeDialog::onDirectionChanged(int index)
{
    Q_UNUSED(index);
    updateDirectionWidgets();
    emit directionChanged();
    updatePreview();
}

void ExtrudeDialog::updateDirectionWidgets()
{
    Direction dir = direction();
    
    m_customDirWidget->setVisible(dir == Direction::CustomVector);
    
    QString dirText;
    switch (dir) {
        case Direction::Normal:
            dirText = QString("Direction: (%1, %2, %3)")
                .arg(m_sketchNormal.x, 0, 'f', 3)
                .arg(m_sketchNormal.y, 0, 'f', 3)
                .arg(m_sketchNormal.z, 0, 'f', 3);
            break;
        case Direction::CustomVector:
            dirText = tr("Enter custom direction vector");
            break;
        case Direction::ToPoint:
            dirText = tr("Click to select target point");
            break;
        case Direction::ToSurface:
            dirText = tr("Click to select target surface");
            break;
    }
    m_directionPreview->setText(dirText);
}

void ExtrudeDialog::onDistanceChanged(double value)
{
    Q_UNUSED(value);
    emit parametersChanged();
    updatePreview();
}

void ExtrudeDialog::onDraftAngleChanged(double value)
{
    Q_UNUSED(value);
    emit parametersChanged();
    updatePreview();
}

void ExtrudeDialog::onTwoSidedToggled(bool checked)
{
    Q_UNUSED(checked);
    emit parametersChanged();
    updatePreview();
}

void ExtrudeDialog::onRatioChanged(double value)
{
    int posPercent = static_cast<int>(value * 100);
    int negPercent = 100 - posPercent;
    m_ratioLabel->setText(QString("%1% / %2%").arg(posPercent).arg(negPercent));
    
    emit parametersChanged();
    updatePreview();
}

void ExtrudeDialog::onCustomDirectionChanged()
{
    emit directionChanged();
    updatePreview();
}

void ExtrudeDialog::onPreviewToggled(bool checked)
{
    m_previewButton->setEnabled(!checked);
    if (checked) {
        updatePreview();
    }
}

void ExtrudeDialog::onApplyClicked()
{
    emit applyRequested();
}

void ExtrudeDialog::onPickDirectionClicked()
{
    // This would typically enter a picking mode in the viewport
    // For now, just show a message
    QMessageBox::information(this, tr("Pick Direction"),
        tr("Click two points in the viewport to define a direction,\n"
           "or select an edge to use its direction."));
}

void ExtrudeDialog::updatePreview()
{
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

ExtrudeDialog::Direction ExtrudeDialog::direction() const
{
    return static_cast<Direction>(m_directionCombo->currentData().toInt());
}

glm::vec3 ExtrudeDialog::customDirection() const
{
    Direction dir = direction();
    
    if (dir == Direction::Normal) {
        glm::vec3 result = m_sketchNormal;
        if (m_flipDirection->isChecked()) {
            result = -result;
        }
        return result;
    }
    
    glm::vec3 custom(
        static_cast<float>(m_dirXSpin->value()),
        static_cast<float>(m_dirYSpin->value()),
        static_cast<float>(m_dirZSpin->value())
    );
    
    float len = glm::length(custom);
    if (len > 1e-6f) {
        custom /= len;
    } else {
        custom = glm::vec3(0, 0, 1);
    }
    
    if (m_flipDirection->isChecked()) {
        custom = -custom;
    }
    
    return custom;
}

float ExtrudeDialog::distance() const
{
    return static_cast<float>(m_distanceSpinbox->value());
}

float ExtrudeDialog::draftAngle() const
{
    if (!m_draftGroup->isChecked()) return 0.0f;
    
    float angle = static_cast<float>(m_draftAngleSpinbox->value());
    if (m_draftDirectionCombo->currentIndex() == 1) {  // Inward
        angle = -angle;
    }
    return angle;
}

bool ExtrudeDialog::isTwoSided() const
{
    return m_twoSidedGroup->isChecked();
}

float ExtrudeDialog::twoSidedRatio() const
{
    return static_cast<float>(m_ratioSpinbox->value());
}

bool ExtrudeDialog::capEnds() const
{
    return m_capEndsCheck->isChecked();
}

bool ExtrudeDialog::autoPreview() const
{
    return m_autoPreviewCheck->isChecked();
}

void ExtrudeDialog::setDistance(float dist)
{
    m_distanceSpinbox->setValue(dist);
}

void ExtrudeDialog::setDraftAngle(float angle)
{
    if (angle != 0.0f) {
        m_draftGroup->setChecked(true);
        if (angle < 0) {
            m_draftDirectionCombo->setCurrentIndex(1);
            angle = -angle;
        }
        m_draftAngleSpinbox->setValue(angle);
    }
}

void ExtrudeDialog::setTwoSided(bool twoSided)
{
    m_twoSidedGroup->setChecked(twoSided);
}

glm::vec3 ExtrudeDialog::parseCustomDirection() const
{
    return customDirection();
}
