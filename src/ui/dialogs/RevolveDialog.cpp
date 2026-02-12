#include "RevolveDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <cmath>

RevolveDialog::RevolveDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Revolve"));
    setMinimumWidth(400);
    setModal(false);  // Allow interaction with viewport
    
    setupUI();
    setupConnections();
    applyStylesheet();
}

void RevolveDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ===============================
    // Axis Group
    // ===============================
    m_axisGroup = new QGroupBox(tr("Revolution Axis"));
    QVBoxLayout* axisLayout = new QVBoxLayout(m_axisGroup);
    
    QHBoxLayout* axisSelectLayout = new QHBoxLayout();
    
    m_axisCombo = new QComboBox();
    m_axisCombo->addItem(tr("X Axis"), static_cast<int>(AxisType::XAxis));
    m_axisCombo->addItem(tr("Y Axis"), static_cast<int>(AxisType::YAxis));
    m_axisCombo->addItem(tr("Z Axis"), static_cast<int>(AxisType::ZAxis));
    m_axisCombo->addItem(tr("Sketch Line"), static_cast<int>(AxisType::SketchLine));
    m_axisCombo->addItem(tr("Pick Edge"), static_cast<int>(AxisType::PickedEdge));
    m_axisCombo->addItem(tr("Custom Axis"), static_cast<int>(AxisType::CustomAxis));
    m_axisCombo->setCurrentIndex(1);  // Default to Y axis
    
    m_pickAxisButton = new QPushButton(tr("Pick"));
    m_pickAxisButton->setToolTip(tr("Pick a line or edge to use as revolution axis"));
    m_pickAxisButton->setMaximumWidth(60);
    
    axisSelectLayout->addWidget(m_axisCombo, 1);
    axisSelectLayout->addWidget(m_pickAxisButton);
    axisLayout->addLayout(axisSelectLayout);
    
    // Custom axis widget (shown when Custom Axis selected)
    m_customAxisWidget = new QWidget();
    QGridLayout* customAxisLayout = new QGridLayout(m_customAxisWidget);
    customAxisLayout->setContentsMargins(0, 8, 0, 0);
    
    m_axisOriginLabel = new QLabel(tr("Origin:"));
    m_originXSpin = new QDoubleSpinBox();
    m_originXSpin->setRange(-10000, 10000);
    m_originXSpin->setDecimals(3);
    m_originXSpin->setValue(0);
    m_originXSpin->setPrefix("X: ");
    
    m_originYSpin = new QDoubleSpinBox();
    m_originYSpin->setRange(-10000, 10000);
    m_originYSpin->setDecimals(3);
    m_originYSpin->setValue(0);
    m_originYSpin->setPrefix("Y: ");
    
    m_originZSpin = new QDoubleSpinBox();
    m_originZSpin->setRange(-10000, 10000);
    m_originZSpin->setDecimals(3);
    m_originZSpin->setValue(0);
    m_originZSpin->setPrefix("Z: ");
    
    customAxisLayout->addWidget(m_axisOriginLabel, 0, 0);
    customAxisLayout->addWidget(m_originXSpin, 0, 1);
    customAxisLayout->addWidget(m_originYSpin, 0, 2);
    customAxisLayout->addWidget(m_originZSpin, 0, 3);
    
    m_axisDirLabel = new QLabel(tr("Direction:"));
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
    m_dirYSpin->setValue(1.0);
    m_dirYSpin->setPrefix("Y: ");
    
    m_dirZSpin = new QDoubleSpinBox();
    m_dirZSpin->setRange(-1.0, 1.0);
    m_dirZSpin->setDecimals(4);
    m_dirZSpin->setSingleStep(0.1);
    m_dirZSpin->setValue(0.0);
    m_dirZSpin->setPrefix("Z: ");
    
    customAxisLayout->addWidget(m_axisDirLabel, 1, 0);
    customAxisLayout->addWidget(m_dirXSpin, 1, 1);
    customAxisLayout->addWidget(m_dirYSpin, 1, 2);
    customAxisLayout->addWidget(m_dirZSpin, 1, 3);
    
    axisLayout->addWidget(m_customAxisWidget);
    m_customAxisWidget->setVisible(false);
    
    m_axisPreview = new QLabel();
    m_axisPreview->setStyleSheet("color: #888; font-size: 11px;");
    axisLayout->addWidget(m_axisPreview);
    
    mainLayout->addWidget(m_axisGroup);

    // ===============================
    // Angle Group
    // ===============================
    m_angleGroup = new QGroupBox(tr("Revolution Angle"));
    QVBoxLayout* angleLayout = new QVBoxLayout(m_angleGroup);
    
    m_fullRevolutionCheck = new QCheckBox(tr("Full Revolution (360°)"));
    m_fullRevolutionCheck->setChecked(true);
    angleLayout->addWidget(m_fullRevolutionCheck);
    
    // Partial angle widget
    m_partialAngleWidget = new QWidget();
    QGridLayout* partialLayout = new QGridLayout(m_partialAngleWidget);
    partialLayout->setContentsMargins(0, 8, 0, 0);
    
    QLabel* startLabel = new QLabel(tr("Start:"));
    m_startAngleSpinbox = new QDoubleSpinBox();
    m_startAngleSpinbox->setRange(0.0, 359.9);
    m_startAngleSpinbox->setDecimals(1);
    m_startAngleSpinbox->setSingleStep(5.0);
    m_startAngleSpinbox->setValue(0.0);
    m_startAngleSpinbox->setSuffix("°");
    
    QLabel* endLabel = new QLabel(tr("End:"));
    m_endAngleSpinbox = new QDoubleSpinBox();
    m_endAngleSpinbox->setRange(0.1, 360.0);
    m_endAngleSpinbox->setDecimals(1);
    m_endAngleSpinbox->setSingleStep(5.0);
    m_endAngleSpinbox->setValue(180.0);
    m_endAngleSpinbox->setSuffix("°");
    
    partialLayout->addWidget(startLabel, 0, 0);
    partialLayout->addWidget(m_startAngleSpinbox, 0, 1);
    partialLayout->addWidget(endLabel, 0, 2);
    partialLayout->addWidget(m_endAngleSpinbox, 0, 3);
    
    // Angle slider
    m_angleSlider = new QSlider(Qt::Horizontal);
    m_angleSlider->setRange(1, 360);
    m_angleSlider->setValue(180);
    m_angleSlider->setTickPosition(QSlider::TicksBelow);
    m_angleSlider->setTickInterval(45);
    
    partialLayout->addWidget(m_angleSlider, 1, 0, 1, 4);
    
    angleLayout->addWidget(m_partialAngleWidget);
    m_partialAngleWidget->setVisible(false);
    
    m_anglePreview = new QLabel(tr("Angle: 360°"));
    m_anglePreview->setStyleSheet("font-weight: bold; font-size: 12px;");
    angleLayout->addWidget(m_anglePreview);
    
    mainLayout->addWidget(m_angleGroup);

    // ===============================
    // Quality Group
    // ===============================
    m_qualityGroup = new QGroupBox(tr("Quality"));
    QFormLayout* qualityLayout = new QFormLayout(m_qualityGroup);
    
    m_segmentsSpinbox = new QSpinBox();
    m_segmentsSpinbox->setRange(3, 360);
    m_segmentsSpinbox->setSingleStep(4);
    m_segmentsSpinbox->setValue(32);
    m_segmentsSpinbox->setToolTip(tr("Number of segments around the revolution"));
    
    m_segmentsPreview = new QLabel(tr("11.25° per segment"));
    m_segmentsPreview->setStyleSheet("color: #888; font-size: 11px;");
    
    qualityLayout->addRow(tr("Segments:"), m_segmentsSpinbox);
    qualityLayout->addRow("", m_segmentsPreview);
    
    mainLayout->addWidget(m_qualityGroup);

    // ===============================
    // Options
    // ===============================
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    
    m_capEndsCheck = new QCheckBox(tr("Cap Ends"));
    m_capEndsCheck->setChecked(true);
    m_capEndsCheck->setToolTip(tr("Create caps for partial revolution"));
    
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
    m_previewButton->setToolTip(tr("Generate preview of revolution"));
    
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
    
    // Initial state
    updateAngleWidgets();
}

void RevolveDialog::setupConnections()
{
    connect(m_axisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RevolveDialog::onAxisTypeChanged);
    
    connect(m_pickAxisButton, &QPushButton::clicked,
            this, &RevolveDialog::onPickAxisClicked);
    
    connect(m_originXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::onCustomAxisChanged);
    connect(m_originYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::onCustomAxisChanged);
    connect(m_originZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::onCustomAxisChanged);
    connect(m_dirXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::onCustomAxisChanged);
    connect(m_dirYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::onCustomAxisChanged);
    connect(m_dirZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::onCustomAxisChanged);
    
    connect(m_fullRevolutionCheck, &QCheckBox::toggled,
            this, &RevolveDialog::onFullRevolutionToggled);
    
    connect(m_angleSlider, &QSlider::valueChanged,
            this, &RevolveDialog::onAngleSliderChanged);
    connect(m_endAngleSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::onAngleSpinboxChanged);
    connect(m_startAngleSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RevolveDialog::updatePreview);
    
    connect(m_segmentsSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
                float degPerSeg = 360.0f / value;
                m_segmentsPreview->setText(QString("%1° per segment").arg(degPerSeg, 0, 'f', 2));
                updatePreview();
            });
    
    connect(m_autoPreviewCheck, &QCheckBox::toggled,
            this, &RevolveDialog::onPreviewToggled);
    
    connect(m_previewButton, &QPushButton::clicked,
            this, &RevolveDialog::previewRequested);
    
    connect(m_capEndsCheck, &QCheckBox::toggled, this, &RevolveDialog::updatePreview);
    
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &RevolveDialog::onApplyClicked);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
}

void RevolveDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            border: 1px solid #444;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px;
        }
        QDoubleSpinBox, QSpinBox {
            padding: 4px;
            border: 1px solid #555;
            border-radius: 3px;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background: #444;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            width: 16px;
            margin: -5px 0;
            background: #0078d4;
            border-radius: 8px;
        }
        QPushButton {
            padding: 6px 16px;
            border-radius: 4px;
        }
        QPushButton:default {
            background-color: #0078d4;
            color: white;
        }
    )");
}

void RevolveDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void RevolveDialog::onAxisTypeChanged(int index)
{
    Q_UNUSED(index);
    AxisType type = axisType();
    
    bool showCustom = (type == AxisType::CustomAxis);
    m_customAxisWidget->setVisible(showCustom);
    
    bool showPick = (type == AxisType::SketchLine || type == AxisType::PickedEdge);
    m_pickAxisButton->setEnabled(showPick || showCustom);
    
    // Update preview label
    glm::vec3 dir = getAxisDirectionForType(type);
    QString axisText;
    
    switch (type) {
        case AxisType::XAxis:
            axisText = tr("Axis: X (1, 0, 0)");
            break;
        case AxisType::YAxis:
            axisText = tr("Axis: Y (0, 1, 0)");
            break;
        case AxisType::ZAxis:
            axisText = tr("Axis: Z (0, 0, 1)");
            break;
        case AxisType::SketchLine:
            axisText = tr("Select a line from the sketch");
            break;
        case AxisType::PickedEdge:
            axisText = tr("Pick an edge from the model");
            break;
        case AxisType::CustomAxis:
            axisText = tr("Define custom axis");
            break;
    }
    m_axisPreview->setText(axisText);
    
    emit axisChanged();
    updatePreview();
}

void RevolveDialog::onAngleSliderChanged(int value)
{
    m_endAngleSpinbox->blockSignals(true);
    m_endAngleSpinbox->setValue(value);
    m_endAngleSpinbox->blockSignals(false);
    
    updateAngleWidgets();
    updatePreview();
}

void RevolveDialog::onAngleSpinboxChanged(double value)
{
    m_angleSlider->blockSignals(true);
    m_angleSlider->setValue(static_cast<int>(value));
    m_angleSlider->blockSignals(false);
    
    updateAngleWidgets();
    updatePreview();
}

void RevolveDialog::onFullRevolutionToggled(bool checked)
{
    m_partialAngleWidget->setVisible(!checked);
    m_capEndsCheck->setEnabled(!checked);
    
    updateAngleWidgets();
    updatePreview();
}

void RevolveDialog::onCustomAxisChanged()
{
    m_customAxisOrigin = glm::vec3(
        m_originXSpin->value(),
        m_originYSpin->value(),
        m_originZSpin->value()
    );
    
    m_customAxisDirection = glm::vec3(
        m_dirXSpin->value(),
        m_dirYSpin->value(),
        m_dirZSpin->value()
    );
    
    // Normalize
    float len = glm::length(m_customAxisDirection);
    if (len > 1e-6f) {
        m_customAxisDirection /= len;
    }
    
    m_hasCustomAxis = true;
    
    QString axisText = QString("Axis: (%1, %2, %3)")
        .arg(m_customAxisDirection.x, 0, 'f', 3)
        .arg(m_customAxisDirection.y, 0, 'f', 3)
        .arg(m_customAxisDirection.z, 0, 'f', 3);
    m_axisPreview->setText(axisText);
    
    emit axisChanged();
    updatePreview();
}

void RevolveDialog::onPickAxisClicked()
{
    emit pickAxisRequested();
    
    // For now, show a message
    QMessageBox::information(this, tr("Pick Axis"),
        tr("Click on a line or edge in the viewport to define the revolution axis."));
}

void RevolveDialog::onPreviewToggled(bool checked)
{
    m_previewButton->setEnabled(!checked);
    if (checked) {
        updatePreview();
    }
}

void RevolveDialog::onApplyClicked()
{
    emit applyRequested();
}

void RevolveDialog::updatePreview()
{
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void RevolveDialog::updateAngleWidgets()
{
    float angle = sweepAngle();
    m_anglePreview->setText(QString(tr("Angle: %1°")).arg(angle, 0, 'f', 1));
}

glm::vec3 RevolveDialog::getAxisDirectionForType(AxisType type) const
{
    switch (type) {
        case AxisType::XAxis:
            return glm::vec3(1, 0, 0);
        case AxisType::YAxis:
            return glm::vec3(0, 1, 0);
        case AxisType::ZAxis:
            return glm::vec3(0, 0, 1);
        default:
            return m_customAxisDirection;
    }
}

RevolveDialog::AxisType RevolveDialog::axisType() const
{
    return static_cast<AxisType>(m_axisCombo->currentData().toInt());
}

glm::vec3 RevolveDialog::axisOrigin() const
{
    AxisType type = axisType();
    if (type == AxisType::CustomAxis || type == AxisType::SketchLine || type == AxisType::PickedEdge) {
        return m_customAxisOrigin;
    }
    return glm::vec3(0.0f);
}

glm::vec3 RevolveDialog::axisDirection() const
{
    return getAxisDirectionForType(axisType());
}

float RevolveDialog::startAngle() const
{
    if (m_fullRevolutionCheck->isChecked()) {
        return 0.0f;
    }
    return static_cast<float>(m_startAngleSpinbox->value());
}

float RevolveDialog::endAngle() const
{
    if (m_fullRevolutionCheck->isChecked()) {
        return 360.0f;
    }
    return static_cast<float>(m_endAngleSpinbox->value());
}

float RevolveDialog::sweepAngle() const
{
    return endAngle() - startAngle();
}

bool RevolveDialog::isFullRevolution() const
{
    return m_fullRevolutionCheck->isChecked();
}

bool RevolveDialog::capEnds() const
{
    return m_capEndsCheck->isChecked();
}

int RevolveDialog::segments() const
{
    return m_segmentsSpinbox->value();
}

bool RevolveDialog::autoPreview() const
{
    return m_autoPreviewCheck->isChecked();
}

void RevolveDialog::setAngle(float angle)
{
    if (std::abs(angle - 360.0f) < 0.1f) {
        m_fullRevolutionCheck->setChecked(true);
    } else {
        m_fullRevolutionCheck->setChecked(false);
        m_endAngleSpinbox->setValue(angle);
        m_angleSlider->setValue(static_cast<int>(angle));
    }
    updateAngleWidgets();
}

void RevolveDialog::setAxis(const glm::vec3& origin, const glm::vec3& direction)
{
    m_customAxisOrigin = origin;
    m_customAxisDirection = glm::normalize(direction);
    m_hasCustomAxis = true;
    
    m_originXSpin->setValue(origin.x);
    m_originYSpin->setValue(origin.y);
    m_originZSpin->setValue(origin.z);
    
    m_dirXSpin->setValue(direction.x);
    m_dirYSpin->setValue(direction.y);
    m_dirZSpin->setValue(direction.z);
    
    m_axisCombo->setCurrentIndex(5);  // Custom Axis
}

void RevolveDialog::setSegments(int segs)
{
    m_segmentsSpinbox->setValue(segs);
}
