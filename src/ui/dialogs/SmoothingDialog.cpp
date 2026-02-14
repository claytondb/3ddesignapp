#include "SmoothingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>

SmoothingDialog::SmoothingDialog(QWidget *parent)
    : QDialog(parent)
    , m_viewport(nullptr)
{
    setWindowTitle(tr("Mesh Smoothing"));
    setMinimumWidth(380);
    setModal(true);
    
    setupUI();
    setupConnections();
    applyStylesheet();
    loadSettings();
}

void SmoothingDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Algorithm selection group
    QGroupBox* algorithmGroup = new QGroupBox(tr("Algorithm"));
    QVBoxLayout* algorithmLayout = new QVBoxLayout(algorithmGroup);
    algorithmLayout->setSpacing(12);

    m_algorithmCombo = new QComboBox();
    m_algorithmCombo->addItem(tr("Laplacian (Fast)"), static_cast<int>(Algorithm::Laplacian));
    m_algorithmCombo->addItem(tr("Taubin (Recommended)"), static_cast<int>(Algorithm::Taubin));
    m_algorithmCombo->addItem(tr("HC (Best Quality)"), static_cast<int>(Algorithm::HC));
    // Default to Taubin - best balance for most users
    m_algorithmCombo->setCurrentIndex(1);
    algorithmLayout->addWidget(m_algorithmCombo);

    m_algorithmDescription = new QLabel();
    m_algorithmDescription->setObjectName("descriptionLabel");
    m_algorithmDescription->setWordWrap(true);
    algorithmLayout->addWidget(m_algorithmDescription);

    // Taubin-specific parameter
    m_taubinWidget = new QWidget();
    QHBoxLayout* taubinLayout = new QHBoxLayout(m_taubinWidget);
    taubinLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* passBandLabel = new QLabel(tr("Pass-band:"));
    taubinLayout->addWidget(passBandLabel);
    
    m_passBandSpinbox = new QDoubleSpinBox();
    m_passBandSpinbox->setRange(0.0, 1.0);
    m_passBandSpinbox->setValue(0.1);
    m_passBandSpinbox->setSingleStep(0.01);
    m_passBandSpinbox->setDecimals(3);
    m_passBandSpinbox->setFixedWidth(80);
    taubinLayout->addWidget(m_passBandSpinbox);
    taubinLayout->addStretch();
    
    m_taubinWidget->setVisible(true);  // Visible by default since Taubin is selected
    algorithmLayout->addWidget(m_taubinWidget);

    mainLayout->addWidget(algorithmGroup);

    // Parameters group
    QGroupBox* paramsGroup = new QGroupBox(tr("Parameters"));
    QVBoxLayout* paramsLayout = new QVBoxLayout(paramsGroup);
    paramsLayout->setSpacing(12);

    // Iterations
    QHBoxLayout* iterationsLayout = new QHBoxLayout();
    QLabel* iterationsLabel = new QLabel(tr("Iterations:"));
    iterationsLayout->addWidget(iterationsLabel);
    
    m_iterationsSpinbox = new QSpinBox();
    m_iterationsSpinbox->setRange(1, 100);
    m_iterationsSpinbox->setValue(5);
    m_iterationsSpinbox->setFixedWidth(80);
    iterationsLayout->addWidget(m_iterationsSpinbox);
    iterationsLayout->addStretch();
    paramsLayout->addLayout(iterationsLayout);

    // Strength
    QHBoxLayout* strengthLayout = new QHBoxLayout();
    QLabel* strengthLabel = new QLabel(tr("Strength:"));
    strengthLayout->addWidget(strengthLabel);

    m_strengthSlider = new QSlider(Qt::Horizontal);
    m_strengthSlider->setRange(0, 100);
    m_strengthSlider->setValue(50);
    m_strengthSlider->setMinimumWidth(150);
    strengthLayout->addWidget(m_strengthSlider);

    m_strengthSpinbox = new QDoubleSpinBox();
    m_strengthSpinbox->setRange(0.0, 1.0);
    m_strengthSpinbox->setValue(0.5);
    m_strengthSpinbox->setSingleStep(0.05);
    m_strengthSpinbox->setDecimals(2);
    m_strengthSpinbox->setFixedWidth(70);
    strengthLayout->addWidget(m_strengthSpinbox);

    paramsLayout->addLayout(strengthLayout);

    mainLayout->addWidget(paramsGroup);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"));
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    optionsLayout->setSpacing(8);

    m_preserveBoundaries = new QCheckBox(tr("Preserve boundary edges"));
    m_preserveBoundaries->setChecked(true);
    optionsLayout->addWidget(m_preserveBoundaries);

    m_autoPreviewCheck = new QCheckBox(tr("Auto-preview"));
    m_autoPreviewCheck->setChecked(true);
    optionsLayout->addWidget(m_autoPreviewCheck);

    mainLayout->addWidget(optionsGroup);

    mainLayout->addStretch();

    // Separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setObjectName("dialogSeparator");
    mainLayout->addWidget(separator);

    // Button box
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton(tr("Reset"));
    m_resetButton->setObjectName("secondaryButton");
    m_resetButton->setToolTip(tr("Reset all parameters to default values"));
    buttonLayout->addWidget(m_resetButton);
    
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

    // Set initial description
    updateAlgorithmDescription();
}

void SmoothingDialog::setupConnections()
{
    connect(m_algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SmoothingDialog::onAlgorithmChanged);

    connect(m_strengthSlider, &QSlider::valueChanged,
            this, &SmoothingDialog::onStrengthSliderChanged);
    connect(m_strengthSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SmoothingDialog::onStrengthSpinboxChanged);

    connect(m_iterationsSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SmoothingDialog::onIterationsChanged);

    connect(m_autoPreviewCheck, &QCheckBox::toggled,
            this, &SmoothingDialog::onPreviewToggled);

    connect(m_resetButton, &QPushButton::clicked, this, &SmoothingDialog::onResetClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SmoothingDialog::onCancelClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &SmoothingDialog::onApplyClicked);
    connect(m_okButton, &QPushButton::clicked, this, [this]() {
        saveSettings();
        onApplyClicked();
        accept();
    });
}

void SmoothingDialog::applyStylesheet()
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
        
        QLabel#descriptionLabel {
            color: #808080;
            font-size: 11px;
            padding: 4px 0;
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

void SmoothingDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

SmoothingDialog::Algorithm SmoothingDialog::algorithm() const
{
    return static_cast<Algorithm>(m_algorithmCombo->currentData().toInt());
}

int SmoothingDialog::iterations() const
{
    return m_iterationsSpinbox->value();
}

double SmoothingDialog::strength() const
{
    return m_strengthSpinbox->value();
}

bool SmoothingDialog::preserveBoundaries() const
{
    return m_preserveBoundaries->isChecked();
}

bool SmoothingDialog::autoPreview() const
{
    return m_autoPreviewCheck->isChecked();
}

double SmoothingDialog::taubinPassBand() const
{
    return m_passBandSpinbox->value();
}

void SmoothingDialog::onAlgorithmChanged(int index)
{
    Q_UNUSED(index)
    
    Algorithm algo = algorithm();
    m_taubinWidget->setVisible(algo == Algorithm::Taubin);
    
    updateAlgorithmDescription();
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void SmoothingDialog::onStrengthSliderChanged(int value)
{
    double doubleValue = value / 100.0;
    m_strengthSpinbox->blockSignals(true);
    m_strengthSpinbox->setValue(doubleValue);
    m_strengthSpinbox->blockSignals(false);
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void SmoothingDialog::onStrengthSpinboxChanged(double value)
{
    int intValue = static_cast<int>(value * 100);
    m_strengthSlider->blockSignals(true);
    m_strengthSlider->setValue(intValue);
    m_strengthSlider->blockSignals(false);
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void SmoothingDialog::onIterationsChanged(int value)
{
    Q_UNUSED(value)
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void SmoothingDialog::onPreviewToggled(bool checked)
{
    if (checked) {
        emit previewRequested();
    }
}

void SmoothingDialog::onApplyClicked()
{
    emit applyRequested();
}

void SmoothingDialog::updateAlgorithmDescription()
{
    QString description;
    
    switch (algorithm()) {
        case Algorithm::Laplacian:
            description = tr("Fastest option. Good for quick previews. May shrink the mesh slightly with many iterations - use 1-3 iterations.");
            break;
        case Algorithm::Taubin:
            description = tr("Best for most use cases. Smooths without shrinking your model. Start with default settings, increase iterations if needed.");
            break;
        case Algorithm::HC:
            description = tr("Highest quality results, especially for organic shapes. Slower but preserves volume and features very well.");
            break;
    }
    
    m_algorithmDescription->setText(description);
}

void SmoothingDialog::loadSettings()
{
    QSettings settings;
    settings.beginGroup("SmoothingDialog");
    
    // Algorithm
    int algo = settings.value("algorithm", 0).toInt();
    if (algo >= 0 && algo <= 2) {
        m_algorithmCombo->setCurrentIndex(algo);
    }
    
    // Parameters
    m_iterationsSpinbox->setValue(settings.value("iterations", 5).toInt());
    m_strengthSpinbox->setValue(settings.value("strength", 0.5).toDouble());
    m_strengthSlider->setValue(static_cast<int>(m_strengthSpinbox->value() * 100));
    m_passBandSpinbox->setValue(settings.value("passBand", 0.1).toDouble());
    
    // Options
    m_preserveBoundaries->setChecked(settings.value("preserveBoundaries", true).toBool());
    m_autoPreviewCheck->setChecked(settings.value("autoPreview", true).toBool());
    
    settings.endGroup();
    
    updateAlgorithmDescription();
}

void SmoothingDialog::saveSettings()
{
    QSettings settings;
    settings.beginGroup("SmoothingDialog");
    
    settings.setValue("algorithm", m_algorithmCombo->currentIndex());
    settings.setValue("iterations", m_iterationsSpinbox->value());
    settings.setValue("strength", m_strengthSpinbox->value());
    settings.setValue("passBand", m_passBandSpinbox->value());
    settings.setValue("preserveBoundaries", m_preserveBoundaries->isChecked());
    settings.setValue("autoPreview", m_autoPreviewCheck->isChecked());
    
    settings.endGroup();
}

void SmoothingDialog::resetToDefaults()
{
    m_algorithmCombo->setCurrentIndex(0);  // Laplacian
    m_iterationsSpinbox->setValue(5);
    m_strengthSpinbox->setValue(0.5);
    m_strengthSlider->setValue(50);
    m_passBandSpinbox->setValue(0.1);
    m_preserveBoundaries->setChecked(true);
    m_autoPreviewCheck->setChecked(true);
    
    updateAlgorithmDescription();
}

void SmoothingDialog::onResetClicked()
{
    resetToDefaults();
    
    if (m_autoPreviewCheck->isChecked()) {
        emit previewRequested();
    }
}

void SmoothingDialog::onCancelClicked()
{
    emit previewCanceled();  // Signal to revert any preview changes
    reject();
}
