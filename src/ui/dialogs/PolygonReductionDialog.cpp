#include "PolygonReductionDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QFrame>

PolygonReductionDialog::PolygonReductionDialog(QWidget *parent)
    : QDialog(parent)
    , m_originalTriangleCount(0)
    , m_originalVertexCount(0)
    , m_viewport(nullptr)
{
    setWindowTitle(tr("Polygon Reduction"));
    setMinimumWidth(420);
    setModal(true);
    
    setupUI();
    setupConnections();
    applyStylesheet();
}

void PolygonReductionDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Original count info
    m_originalCountLabel = new QLabel(tr("Original: 0 triangles"));
    m_originalCountLabel->setObjectName("infoLabel");
    mainLayout->addWidget(m_originalCountLabel);

    // Target type group
    QGroupBox* targetGroup = new QGroupBox(tr("Target"));
    QVBoxLayout* targetLayout = new QVBoxLayout(targetGroup);
    targetLayout->setSpacing(12);

    m_targetTypeGroup = new QButtonGroup(this);

    // Percentage option
    QHBoxLayout* percentageLayout = new QHBoxLayout();
    m_radioPercentage = new QRadioButton(tr("Percentage:"));
    m_radioPercentage->setChecked(true);
    m_targetTypeGroup->addButton(m_radioPercentage, static_cast<int>(TargetType::Percentage));
    percentageLayout->addWidget(m_radioPercentage);

    m_percentageWidget = new QWidget();
    QHBoxLayout* percentageControlsLayout = new QHBoxLayout(m_percentageWidget);
    percentageControlsLayout->setContentsMargins(0, 0, 0, 0);
    percentageControlsLayout->setSpacing(8);

    m_percentageSlider = new QSlider(Qt::Horizontal);
    m_percentageSlider->setRange(1, 100);
    m_percentageSlider->setValue(50);
    m_percentageSlider->setMinimumWidth(150);
    percentageControlsLayout->addWidget(m_percentageSlider);

    m_percentageSpinbox = new QDoubleSpinBox();
    m_percentageSpinbox->setRange(1.0, 100.0);
    m_percentageSpinbox->setValue(50.0);
    m_percentageSpinbox->setSuffix("%");
    m_percentageSpinbox->setDecimals(1);
    m_percentageSpinbox->setFixedWidth(80);
    percentageControlsLayout->addWidget(m_percentageSpinbox);

    percentageLayout->addWidget(m_percentageWidget);
    percentageLayout->addStretch();
    targetLayout->addLayout(percentageLayout);

    // Vertex count option
    QHBoxLayout* vertexLayout = new QHBoxLayout();
    m_radioVertexCount = new QRadioButton(tr("Vertex count:"));
    m_targetTypeGroup->addButton(m_radioVertexCount, static_cast<int>(TargetType::VertexCount));
    vertexLayout->addWidget(m_radioVertexCount);

    m_vertexCountWidget = new QWidget();
    QHBoxLayout* vertexControlsLayout = new QHBoxLayout(m_vertexCountWidget);
    vertexControlsLayout->setContentsMargins(0, 0, 0, 0);

    m_vertexCountSpinbox = new QSpinBox();
    m_vertexCountSpinbox->setRange(100, 10000000);
    m_vertexCountSpinbox->setValue(10000);
    m_vertexCountSpinbox->setFixedWidth(120);
    m_vertexCountSpinbox->setEnabled(false);
    vertexControlsLayout->addWidget(m_vertexCountSpinbox);

    vertexLayout->addWidget(m_vertexCountWidget);
    vertexLayout->addStretch();
    targetLayout->addLayout(vertexLayout);

    // Face count option
    QHBoxLayout* faceLayout = new QHBoxLayout();
    m_radioFaceCount = new QRadioButton(tr("Face count:"));
    m_targetTypeGroup->addButton(m_radioFaceCount, static_cast<int>(TargetType::FaceCount));
    faceLayout->addWidget(m_radioFaceCount);

    m_faceCountWidget = new QWidget();
    QHBoxLayout* faceControlsLayout = new QHBoxLayout(m_faceCountWidget);
    faceControlsLayout->setContentsMargins(0, 0, 0, 0);

    m_faceCountSpinbox = new QSpinBox();
    m_faceCountSpinbox->setRange(100, 10000000);
    m_faceCountSpinbox->setValue(10000);
    m_faceCountSpinbox->setFixedWidth(120);
    m_faceCountSpinbox->setEnabled(false);
    faceControlsLayout->addWidget(m_faceCountSpinbox);

    faceLayout->addWidget(m_faceCountWidget);
    faceLayout->addStretch();
    targetLayout->addLayout(faceLayout);

    mainLayout->addWidget(targetGroup);

    // Estimated result
    m_estimatedResultLabel = new QLabel(tr("Result: ~0 triangles"));
    m_estimatedResultLabel->setObjectName("resultLabel");
    mainLayout->addWidget(m_estimatedResultLabel);

    // Options group
    m_optionsGroup = new QGroupBox(tr("Options"));
    QVBoxLayout* optionsLayout = new QVBoxLayout(m_optionsGroup);
    optionsLayout->setSpacing(8);

    m_preserveBoundaries = new QCheckBox(tr("Preserve boundary edges"));
    m_preserveBoundaries->setChecked(true);
    optionsLayout->addWidget(m_preserveBoundaries);

    // Sharp features with angle
    QHBoxLayout* sharpLayout = new QHBoxLayout();
    m_preserveSharpFeatures = new QCheckBox(tr("Preserve sharp features (angle >"));
    m_preserveSharpFeatures->setChecked(true);
    sharpLayout->addWidget(m_preserveSharpFeatures);

    m_sharpAngleSpinbox = new QDoubleSpinBox();
    m_sharpAngleSpinbox->setRange(1.0, 90.0);
    m_sharpAngleSpinbox->setValue(30.0);
    m_sharpAngleSpinbox->setSuffix("°");
    m_sharpAngleSpinbox->setFixedWidth(70);
    sharpLayout->addWidget(m_sharpAngleSpinbox);

    QLabel* closeParen = new QLabel(")");
    sharpLayout->addWidget(closeParen);
    sharpLayout->addStretch();
    optionsLayout->addLayout(sharpLayout);

    m_lockVertexColors = new QCheckBox(tr("Lock vertex colors"));
    optionsLayout->addWidget(m_lockVertexColors);

    mainLayout->addWidget(m_optionsGroup);

    // Preview checkbox
    m_autoPreviewCheck = new QCheckBox(tr("Auto-preview"));
    m_autoPreviewCheck->setChecked(true);
    mainLayout->addWidget(m_autoPreviewCheck);

    // Progress section
    QWidget* progressWidget = new QWidget();
    QVBoxLayout* progressLayout = new QVBoxLayout(progressWidget);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(4);

    m_progressLabel = new QLabel(tr("Ready"));
    m_progressLabel->setObjectName("progressLabel");
    progressLayout->addWidget(m_progressLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setVisible(false);
    progressLayout->addWidget(m_progressBar);

    mainLayout->addWidget(progressWidget);

    // Separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setObjectName("dialogSeparator");
    mainLayout->addWidget(separator);

    // Button box
    QHBoxLayout* buttonLayout = new QHBoxLayout();
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

void PolygonReductionDialog::setupConnections()
{
    // Target type changes
    connect(m_targetTypeGroup, &QButtonGroup::idClicked, this, &PolygonReductionDialog::onTargetTypeChanged);

    // Percentage controls
    connect(m_percentageSlider, &QSlider::valueChanged, this, &PolygonReductionDialog::onPercentageSliderChanged);
    connect(m_percentageSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PolygonReductionDialog::onPercentageSpinboxChanged);

    // Count controls
    connect(m_vertexCountSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PolygonReductionDialog::onVertexCountChanged);
    connect(m_faceCountSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PolygonReductionDialog::onFaceCountChanged);

    // Preview toggle
    connect(m_autoPreviewCheck, &QCheckBox::toggled, this, &PolygonReductionDialog::onPreviewToggled);

    // Buttons
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &PolygonReductionDialog::onApplyClicked);
    connect(m_okButton, &QPushButton::clicked, this, [this]() {
        onApplyClicked();
        accept();
    });

    // Sharp features checkbox enables/disables angle spinbox
    connect(m_preserveSharpFeatures, &QCheckBox::toggled, m_sharpAngleSpinbox, &QDoubleSpinBox::setEnabled);
}

void PolygonReductionDialog::applyStylesheet()
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
        
        QRadioButton, QCheckBox {
            color: #b3b3b3;
            spacing: 8px;
            font-size: 13px;
        }
        
        QRadioButton::indicator, QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }
        
        QRadioButton::indicator:checked {
            background-color: #0078d4;
            border: 2px solid #0078d4;
            border-radius: 8px;
        }
        
        QRadioButton::indicator:unchecked {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 8px;
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
        
        QSlider::groove:horizontal {
            background: #4a4a4a;
            height: 4px;
            border-radius: 2px;
        }
        
        QSlider::handle:horizontal {
            background: #ffffff;
            width: 14px;
            height: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
        
        QSlider::sub-page:horizontal {
            background: #0078d4;
            border-radius: 2px;
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
        
        QLabel#infoLabel {
            color: #b3b3b3;
            font-size: 13px;
            padding: 4px 0;
        }
        
        QLabel#resultLabel {
            color: #4caf50;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 0;
        }
        
        QLabel#progressLabel {
            color: #808080;
            font-size: 11px;
        }
        
        QProgressBar {
            background-color: #333333;
            border: none;
            border-radius: 2px;
            height: 4px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 2px;
        }
        
        QFrame#dialogSeparator {
            background-color: #4a4a4a;
            max-height: 1px;
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

void PolygonReductionDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void PolygonReductionDialog::setOriginalTriangleCount(int count)
{
    m_originalTriangleCount = count;
    m_originalCountLabel->setText(tr("Original: %1 triangles").arg(QLocale().toString(count)));
    m_faceCountSpinbox->setMaximum(count);
    m_faceCountSpinbox->setValue(count / 2);
    updateEstimatedResult();
}

void PolygonReductionDialog::setOriginalVertexCount(int count)
{
    m_originalVertexCount = count;
    m_vertexCountSpinbox->setMaximum(count);
    m_vertexCountSpinbox->setValue(count / 2);
}

PolygonReductionDialog::TargetType PolygonReductionDialog::targetType() const
{
    return static_cast<TargetType>(m_targetTypeGroup->checkedId());
}

double PolygonReductionDialog::percentage() const
{
    return m_percentageSpinbox->value();
}

int PolygonReductionDialog::targetVertexCount() const
{
    return m_vertexCountSpinbox->value();
}

int PolygonReductionDialog::targetFaceCount() const
{
    return m_faceCountSpinbox->value();
}

bool PolygonReductionDialog::preserveBoundaries() const
{
    return m_preserveBoundaries->isChecked();
}

bool PolygonReductionDialog::preserveSharpFeatures() const
{
    return m_preserveSharpFeatures->isChecked();
}

double PolygonReductionDialog::sharpFeatureAngle() const
{
    return m_sharpAngleSpinbox->value();
}

bool PolygonReductionDialog::lockVertexColors() const
{
    return m_lockVertexColors->isChecked();
}

bool PolygonReductionDialog::autoPreview() const
{
    return m_autoPreviewCheck->isChecked();
}

void PolygonReductionDialog::setProgress(int percent)
{
    m_progressBar->setValue(percent);
    m_progressBar->setVisible(percent > 0 && percent < 100);
}

void PolygonReductionDialog::setProgressText(const QString& text)
{
    m_progressLabel->setText(text);
}

void PolygonReductionDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    updateEstimatedResult();
}

void PolygonReductionDialog::onTargetTypeChanged()
{
    TargetType type = targetType();
    
    m_percentageSlider->setEnabled(type == TargetType::Percentage);
    m_percentageSpinbox->setEnabled(type == TargetType::Percentage);
    m_vertexCountSpinbox->setEnabled(type == TargetType::VertexCount);
    m_faceCountSpinbox->setEnabled(type == TargetType::FaceCount);
    
    updateEstimatedResult();
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void PolygonReductionDialog::onPercentageSliderChanged(int value)
{
    m_percentageSpinbox->blockSignals(true);
    m_percentageSpinbox->setValue(static_cast<double>(value));
    m_percentageSpinbox->blockSignals(false);
    
    updateEstimatedResult();
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void PolygonReductionDialog::onPercentageSpinboxChanged(double value)
{
    m_percentageSlider->blockSignals(true);
    m_percentageSlider->setValue(static_cast<int>(value));
    m_percentageSlider->blockSignals(false);
    
    updateEstimatedResult();
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void PolygonReductionDialog::onVertexCountChanged(int value)
{
    Q_UNUSED(value)
    updateEstimatedResult();
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void PolygonReductionDialog::onFaceCountChanged(int value)
{
    Q_UNUSED(value)
    updateEstimatedResult();
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void PolygonReductionDialog::onPreviewToggled(bool checked)
{
    if (checked) {
        emit previewRequested();
    }
}

void PolygonReductionDialog::onApplyClicked()
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_progressLabel->setText(tr("Processing..."));
    emit reductionStarted();
    emit applyRequested();
}

void PolygonReductionDialog::updateEstimatedResult()
{
    int estimated = 0;
    
    switch (targetType()) {
        case TargetType::Percentage:
            estimated = static_cast<int>(m_originalTriangleCount * (m_percentageSpinbox->value() / 100.0));
            break;
        case TargetType::VertexCount:
            // Estimate faces from vertices (rough: faces ≈ 2 * vertices for manifold meshes)
            estimated = m_vertexCountSpinbox->value() * 2;
            break;
        case TargetType::FaceCount:
            estimated = m_faceCountSpinbox->value();
            break;
    }
    
    m_estimatedResultLabel->setText(tr("Result: ~%1 triangles").arg(QLocale().toString(estimated)));
}

void PolygonReductionDialog::updateControlVisibility()
{
    // Currently all controls are always visible, just enabled/disabled
    // This method is available for future use if we want to hide controls
}
