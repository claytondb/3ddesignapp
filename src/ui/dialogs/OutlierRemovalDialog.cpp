#include "OutlierRemovalDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>

OutlierRemovalDialog::OutlierRemovalDialog(QWidget *parent)
    : QDialog(parent)
    , m_viewport(nullptr)
    , m_vertexCount(0)
    , m_outlierCount(0)
{
    setWindowTitle(tr("Remove Outliers"));
    setMinimumWidth(400);
    setModal(false);  // Non-modal to allow viewport interaction
    
    setupUI();
    setupConnections();
    applyStylesheet();
}

void OutlierRemovalDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Statistics section
    QGroupBox* statsGroup = new QGroupBox(tr("Mesh Statistics"));
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
    statsLayout->setSpacing(4);
    
    m_vertexCountLabel = new QLabel(tr("Vertices: 0"));
    m_vertexCountLabel->setObjectName("infoLabel");
    statsLayout->addWidget(m_vertexCountLabel);
    
    m_outlierCountLabel = new QLabel(tr("Detected outliers: -"));
    m_outlierCountLabel->setObjectName("infoLabel");
    statsLayout->addWidget(m_outlierCountLabel);
    
    mainLayout->addWidget(statsGroup);

    // Method group
    QGroupBox* methodGroup = new QGroupBox(tr("Detection Method"));
    QVBoxLayout* methodLayout = new QVBoxLayout(methodGroup);
    methodLayout->setSpacing(12);

    // Distance threshold
    QLabel* thresholdTitle = new QLabel(tr("Distance Threshold"));
    thresholdTitle->setObjectName("sectionLabel");
    methodLayout->addWidget(thresholdTitle);

    QHBoxLayout* thresholdLayout = new QHBoxLayout();
    
    m_thresholdSlider = new QSlider(Qt::Horizontal);
    m_thresholdSlider->setRange(1, 100);
    m_thresholdSlider->setValue(10);
    m_thresholdSlider->setMinimumWidth(150);
    thresholdLayout->addWidget(m_thresholdSlider);
    
    m_thresholdSpinbox = new QDoubleSpinBox();
    m_thresholdSpinbox->setRange(0.01, 100.0);
    m_thresholdSpinbox->setValue(1.0);
    m_thresholdSpinbox->setSingleStep(0.1);
    m_thresholdSpinbox->setSuffix(" mm");
    m_thresholdSpinbox->setDecimals(2);
    m_thresholdSpinbox->setFixedWidth(90);
    thresholdLayout->addWidget(m_thresholdSpinbox);
    
    methodLayout->addLayout(thresholdLayout);

    QLabel* thresholdDesc = new QLabel(tr("Points farther than this distance from their neighbors are outliers."));
    thresholdDesc->setObjectName("descriptionLabel");
    thresholdDesc->setWordWrap(true);
    methodLayout->addWidget(thresholdDesc);

    methodLayout->addSpacing(8);

    // Standard deviation method
    QHBoxLayout* stdDevLayout = new QHBoxLayout();
    QLabel* stdDevLabel = new QLabel(tr("Or use standard deviations:"));
    stdDevLayout->addWidget(stdDevLabel);
    
    m_stdDevSpinbox = new QSpinBox();
    m_stdDevSpinbox->setRange(1, 10);
    m_stdDevSpinbox->setValue(3);
    m_stdDevSpinbox->setSuffix(" σ");
    m_stdDevSpinbox->setFixedWidth(70);
    stdDevLayout->addWidget(m_stdDevSpinbox);
    stdDevLayout->addStretch();
    
    methodLayout->addLayout(stdDevLayout);

    QLabel* stdDevDesc = new QLabel(tr("Points beyond N standard deviations from mean distance are outliers."));
    stdDevDesc->setObjectName("descriptionLabel");
    stdDevDesc->setWordWrap(true);
    methodLayout->addWidget(stdDevDesc);

    mainLayout->addWidget(methodGroup);

    // Cluster size group
    QGroupBox* clusterGroup = new QGroupBox(tr("Cluster Filtering"));
    QVBoxLayout* clusterLayout = new QVBoxLayout(clusterGroup);
    clusterLayout->setSpacing(8);
    
    QHBoxLayout* clusterSizeLayout = new QHBoxLayout();
    QLabel* clusterLabel = new QLabel(tr("Minimum cluster size:"));
    clusterSizeLayout->addWidget(clusterLabel);
    
    m_clusterSizeSpinbox = new QSpinBox();
    m_clusterSizeSpinbox->setRange(1, 10000);
    m_clusterSizeSpinbox->setValue(10);
    m_clusterSizeSpinbox->setFixedWidth(100);
    clusterSizeLayout->addWidget(m_clusterSizeSpinbox);
    clusterSizeLayout->addStretch();
    
    clusterLayout->addLayout(clusterSizeLayout);

    QLabel* clusterDesc = new QLabel(tr("Connected components with fewer vertices than this will be removed."));
    clusterDesc->setObjectName("descriptionLabel");
    clusterDesc->setWordWrap(true);
    clusterLayout->addWidget(clusterDesc);

    mainLayout->addWidget(clusterGroup);

    // Preview option
    m_previewCheck = new QCheckBox(tr("Preview outliers in viewport"));
    m_previewCheck->setChecked(true);
    mainLayout->addWidget(m_previewCheck);

    // Estimated removal
    m_estimatedLabel = new QLabel(tr("Estimated removal: -"));
    m_estimatedLabel->setObjectName("resultLabel");
    mainLayout->addWidget(m_estimatedLabel);

    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    mainLayout->addStretch();

    // Separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setObjectName("dialogSeparator");
    mainLayout->addWidget(separator);

    // Button box
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_analyzeButton = new QPushButton(tr("Analyze"));
    m_analyzeButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_analyzeButton);
    
    m_removeButton = new QPushButton(tr("Remove Outliers"));
    m_removeButton->setObjectName("primaryButton");
    m_removeButton->setEnabled(false);
    buttonLayout->addWidget(m_removeButton);
    
    buttonLayout->addStretch();
    
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void OutlierRemovalDialog::setupConnections()
{
    connect(m_thresholdSlider, &QSlider::valueChanged,
            this, &OutlierRemovalDialog::onThresholdSliderChanged);
    connect(m_thresholdSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OutlierRemovalDialog::onThresholdSpinboxChanged);
    
    connect(m_stdDevSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &OutlierRemovalDialog::onStdDevChanged);
    
    connect(m_clusterSizeSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &OutlierRemovalDialog::onClusterSizeChanged);
    
    connect(m_previewCheck, &QCheckBox::toggled,
            this, &OutlierRemovalDialog::onPreviewToggled);
    
    connect(m_analyzeButton, &QPushButton::clicked,
            this, &OutlierRemovalDialog::onAnalyzeClicked);
    connect(m_removeButton, &QPushButton::clicked,
            this, &OutlierRemovalDialog::onRemoveClicked);
    connect(m_closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
}

void OutlierRemovalDialog::applyStylesheet()
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
        
        QLabel#infoLabel {
            color: #808080;
            font-size: 12px;
        }
        
        QLabel#sectionLabel {
            color: #ffffff;
            font-size: 12px;
            font-weight: 600;
        }
        
        QLabel#descriptionLabel {
            color: #808080;
            font-size: 11px;
            padding: 2px 0;
        }
        
        QLabel#resultLabel {
            color: #ff9800;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 0;
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
            background: #ff9800;
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
        
        QProgressBar {
            background-color: #333333;
            border: none;
            border-radius: 2px;
            height: 4px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #ff9800;
            border-radius: 2px;
        }
        
        QFrame#dialogSeparator {
            background-color: #4a4a4a;
            max-height: 1px;
        }
        
        QPushButton#primaryButton {
            background-color: #ff9800;
            color: #1a1a1a;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 600;
            min-width: 120px;
        }
        
        QPushButton#primaryButton:hover {
            background-color: #ffa726;
        }
        
        QPushButton#primaryButton:pressed {
            background-color: #f57c00;
        }
        
        QPushButton#primaryButton:disabled {
            background-color: #3d3d3d;
            color: #5c5c5c;
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

void OutlierRemovalDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void OutlierRemovalDialog::setVertexCount(int count)
{
    m_vertexCount = count;
    m_vertexCountLabel->setText(tr("Vertices: %1").arg(QLocale().toString(count)));
}

void OutlierRemovalDialog::setOutlierCount(int count)
{
    m_outlierCount = count;
    
    if (count < 0) {
        m_outlierCountLabel->setText(tr("Detected outliers: -"));
        m_estimatedLabel->setText(tr("Estimated removal: -"));
        m_removeButton->setEnabled(false);
    } else {
        m_outlierCountLabel->setText(tr("Detected outliers: %1").arg(QLocale().toString(count)));
        
        if (m_vertexCount > 0) {
            double percent = (static_cast<double>(count) / m_vertexCount) * 100.0;
            m_estimatedLabel->setText(tr("Estimated removal: %1 vertices (%2%)")
                .arg(QLocale().toString(count))
                .arg(QString::number(percent, 'f', 2)));
        } else {
            m_estimatedLabel->setText(tr("Estimated removal: %1 vertices").arg(QLocale().toString(count)));
        }
        
        m_removeButton->setEnabled(count > 0);
    }
}

double OutlierRemovalDialog::distanceThreshold() const
{
    return m_thresholdSpinbox->value();
}

int OutlierRemovalDialog::minimumClusterSize() const
{
    return m_clusterSizeSpinbox->value();
}

bool OutlierRemovalDialog::previewEnabled() const
{
    return m_previewCheck->isChecked();
}

int OutlierRemovalDialog::standardDeviations() const
{
    return m_stdDevSpinbox->value();
}

void OutlierRemovalDialog::onThresholdSliderChanged(int value)
{
    // Map slider (1-100) to threshold (0.01-100.0) with logarithmic scale
    double threshold = 0.01 * std::pow(10.0, value / 25.0);
    
    m_thresholdSpinbox->blockSignals(true);
    m_thresholdSpinbox->setValue(threshold);
    m_thresholdSpinbox->blockSignals(false);
    
    updateEstimatedRemoval();
    
    if (m_previewCheck->isChecked()) {
        emit previewRequested();
    }
}

void OutlierRemovalDialog::onThresholdSpinboxChanged(double value)
{
    // Map threshold to slider with logarithmic scale
    int sliderValue = static_cast<int>(25.0 * std::log10(value / 0.01));
    sliderValue = qBound(1, sliderValue, 100);
    
    m_thresholdSlider->blockSignals(true);
    m_thresholdSlider->setValue(sliderValue);
    m_thresholdSlider->blockSignals(false);
    
    updateEstimatedRemoval();
    
    if (m_previewCheck->isChecked()) {
        emit previewRequested();
    }
}

void OutlierRemovalDialog::onStdDevChanged(int value)
{
    Q_UNUSED(value)
    
    updateEstimatedRemoval();
    
    if (m_previewCheck->isChecked()) {
        emit previewRequested();
    }
}

void OutlierRemovalDialog::onClusterSizeChanged(int value)
{
    Q_UNUSED(value)
    
    updateEstimatedRemoval();
    
    if (m_previewCheck->isChecked()) {
        emit previewRequested();
    }
}

void OutlierRemovalDialog::onPreviewToggled(bool checked)
{
    if (checked) {
        emit previewRequested();
    }
}

void OutlierRemovalDialog::onAnalyzeClicked()
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    emit analyzeRequested();
}

void OutlierRemovalDialog::onRemoveClicked()
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    emit removeRequested();
}

void OutlierRemovalDialog::updateEstimatedRemoval()
{
    // This would typically trigger a lightweight analysis
    // For now, just mark as needing re-analysis
    m_outlierCountLabel->setText(tr("Detected outliers: (re-analyze)"));
    m_estimatedLabel->setText(tr("Estimated removal: (re-analyze)"));
}
