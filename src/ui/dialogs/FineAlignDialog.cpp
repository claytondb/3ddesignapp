/**
 * @file FineAlignDialog.cpp
 * @brief Implementation of ICP fine alignment dialog
 */

#include "FineAlignDialog.h"
#include "../../geometry/Alignment.h"
#include "../../geometry/ICP.h"
#include "../../geometry/MeshData.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QDateTime>
#include <QApplication>
#include <QThread>

namespace dc {

FineAlignDialog::FineAlignDialog(Viewport* viewport, QWidget* parent)
    : QDialog(parent)
    , m_viewport(viewport)
    , m_result(std::make_unique<dc3d::geometry::AlignmentResult>())
{
    setWindowTitle(tr("Fine Alignment (ICP)"));
    setMinimumSize(500, 600);
    setupUI();
    applyStylesheet();
}

void FineAlignDialog::applyStylesheet() {
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
        
        QProgressBar {
            background-color: #333333;
            border: none;
            border-radius: 4px;
            text-align: center;
            color: #ffffff;
            font-size: 12px;
        }
        
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 4px;
        }
        
        QPlainTextEdit {
            background-color: #1a1a1a;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            color: #b3b3b3;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
            font-size: 11px;
            padding: 8px;
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
        
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #5c5c5c;
            border-color: #333333;
        }
        
        QPushButton#startButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
        }
        
        QPushButton#startButton:hover {
            background-color: #1a88e0;
        }
        
        QPushButton#startButton:pressed {
            background-color: #0066b8;
        }
        
        QPushButton#stopButton {
            background-color: #f44336;
            color: #ffffff;
            border: none;
        }
        
        QPushButton#stopButton:hover {
            background-color: #ef5350;
        }
        
        QPushButton#stopButton:disabled {
            background-color: #3d3d3d;
            color: #5c5c5c;
        }
    )");
}

FineAlignDialog::~FineAlignDialog() = default;

void FineAlignDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Algorithm settings
    m_settingsGroup = new QGroupBox(tr("Algorithm Settings"), this);
    auto* settingsLayout = new QGridLayout(m_settingsGroup);
    
    settingsLayout->addWidget(new QLabel(tr("Algorithm:"), this), 0, 0);
    m_algorithmCombo = new QComboBox(this);
    m_algorithmCombo->addItem(tr("Point-to-Point"), static_cast<int>(dc3d::geometry::ICPAlgorithm::PointToPoint));
    m_algorithmCombo->addItem(tr("Point-to-Plane"), static_cast<int>(dc3d::geometry::ICPAlgorithm::PointToPlane));
    m_algorithmCombo->setCurrentIndex(1); // Default to point-to-plane
    settingsLayout->addWidget(m_algorithmCombo, 0, 1);
    
    settingsLayout->addWidget(new QLabel(tr("Max Iterations:"), this), 1, 0);
    m_maxIterationsSpin = new QSpinBox(this);
    m_maxIterationsSpin->setRange(1, 1000);
    m_maxIterationsSpin->setValue(50);
    settingsLayout->addWidget(m_maxIterationsSpin, 1, 1);
    
    settingsLayout->addWidget(new QLabel(tr("Convergence Threshold:"), this), 2, 0);
    m_convergenceThresholdSpin = new QDoubleSpinBox(this);
    m_convergenceThresholdSpin->setRange(1e-10, 1.0);
    m_convergenceThresholdSpin->setDecimals(8);
    m_convergenceThresholdSpin->setValue(1e-5);
    m_convergenceThresholdSpin->setSingleStep(1e-6);
    settingsLayout->addWidget(m_convergenceThresholdSpin, 2, 1);
    
    mainLayout->addWidget(m_settingsGroup);
    
    // Outlier rejection
    m_outlierGroup = new QGroupBox(tr("Outlier Rejection"), this);
    auto* outlierLayout = new QGridLayout(m_outlierGroup);
    
    m_outlierRejectionCheck = new QCheckBox(tr("Enable outlier rejection"), this);
    m_outlierRejectionCheck->setChecked(true);
    outlierLayout->addWidget(m_outlierRejectionCheck, 0, 0, 1, 2);
    
    outlierLayout->addWidget(new QLabel(tr("Threshold (σ):"), this), 1, 0);
    m_outlierThresholdSpin = new QDoubleSpinBox(this);
    m_outlierThresholdSpin->setRange(1.0, 10.0);
    m_outlierThresholdSpin->setValue(3.0);
    m_outlierThresholdSpin->setSingleStep(0.5);
    outlierLayout->addWidget(m_outlierThresholdSpin, 1, 1);
    
    outlierLayout->addWidget(new QLabel(tr("Trim Percentage:"), this), 2, 0);
    m_trimPercentageSpin = new QDoubleSpinBox(this);
    m_trimPercentageSpin->setRange(0.0, 50.0);
    m_trimPercentageSpin->setValue(0.0);
    m_trimPercentageSpin->setSuffix(tr("%"));
    outlierLayout->addWidget(m_trimPercentageSpin, 2, 1);
    
    mainLayout->addWidget(m_outlierGroup);
    
    // Sampling settings
    m_samplingGroup = new QGroupBox(tr("Sampling"), this);
    auto* samplingLayout = new QGridLayout(m_samplingGroup);
    
    samplingLayout->addWidget(new QLabel(tr("Sampling Rate:"), this), 0, 0);
    m_samplingRateSpin = new QSpinBox(this);
    m_samplingRateSpin->setRange(1, 100);
    m_samplingRateSpin->setValue(1);
    m_samplingRateSpin->setToolTip(tr("Use every Nth point (1 = all points)"));
    samplingLayout->addWidget(m_samplingRateSpin, 0, 1);
    
    samplingLayout->addWidget(new QLabel(tr("Max Distance:"), this), 1, 0);
    m_maxDistanceSpin = new QDoubleSpinBox(this);
    m_maxDistanceSpin->setRange(0.0, 1e6);
    m_maxDistanceSpin->setValue(0.0);
    m_maxDistanceSpin->setSpecialValueText(tr("Unlimited"));
    m_maxDistanceSpin->setToolTip(tr("Maximum correspondence distance (0 = unlimited)"));
    samplingLayout->addWidget(m_maxDistanceSpin, 1, 1);
    
    mainLayout->addWidget(m_samplingGroup);
    
    // Progress
    m_progressGroup = new QGroupBox(tr("Progress"), this);
    auto* progressLayout = new QGridLayout(m_progressGroup);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    progressLayout->addWidget(m_progressBar, 0, 0, 1, 4);
    
    progressLayout->addWidget(new QLabel(tr("Iteration:"), this), 1, 0);
    m_iterationLabel = new QLabel(tr("0 / 0"), this);
    progressLayout->addWidget(m_iterationLabel, 1, 1);
    
    progressLayout->addWidget(new QLabel(tr("Current Error:"), this), 1, 2);
    m_currentErrorLabel = new QLabel(tr("-"), this);
    progressLayout->addWidget(m_currentErrorLabel, 1, 3);
    
    auto* progressButtonLayout = new QHBoxLayout();
    m_startButton = new QPushButton(tr("Start"), this);
    m_startButton->setObjectName("startButton");
    progressButtonLayout->addWidget(m_startButton);
    
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setObjectName("stopButton");
    m_stopButton->setEnabled(false);
    progressButtonLayout->addWidget(m_stopButton);
    
    progressButtonLayout->addStretch();
    progressLayout->addLayout(progressButtonLayout, 2, 0, 1, 4);
    
    mainLayout->addWidget(m_progressGroup);
    
    // Results
    m_resultsGroup = new QGroupBox(tr("Results"), this);
    auto* resultsLayout = new QGridLayout(m_resultsGroup);
    
    resultsLayout->addWidget(new QLabel(tr("Initial RMS Error:"), this), 0, 0);
    m_initialErrorLabel = new QLabel(tr("-"), this);
    resultsLayout->addWidget(m_initialErrorLabel, 0, 1);
    
    resultsLayout->addWidget(new QLabel(tr("Final RMS Error:"), this), 0, 2);
    m_finalErrorLabel = new QLabel(tr("-"), this);
    resultsLayout->addWidget(m_finalErrorLabel, 0, 3);
    
    resultsLayout->addWidget(new QLabel(tr("Iterations Used:"), this), 1, 0);
    m_iterationsUsedLabel = new QLabel(tr("-"), this);
    resultsLayout->addWidget(m_iterationsUsedLabel, 1, 1);
    
    resultsLayout->addWidget(new QLabel(tr("Convergence:"), this), 1, 2);
    m_convergenceLabel = new QLabel(tr("-"), this);
    resultsLayout->addWidget(m_convergenceLabel, 1, 3);
    
    resultsLayout->addWidget(new QLabel(tr("Correspondences:"), this), 2, 0);
    m_correspondenceLabel = new QLabel(tr("-"), this);
    resultsLayout->addWidget(m_correspondenceLabel, 2, 1);
    
    mainLayout->addWidget(m_resultsGroup);
    
    // Log
    m_logText = new QPlainTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(100);
    m_logText->setPlaceholderText(tr("Alignment log..."));
    mainLayout->addWidget(m_logText);
    
    // Dialog buttons
    auto* buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton(tr("Reset"), this);
    buttonLayout->addWidget(m_resetButton);
    
    buttonLayout->addStretch();
    
    m_applyButton = new QPushButton(tr("Apply"), this);
    m_applyButton->setEnabled(false);
    buttonLayout->addWidget(m_applyButton);
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(m_algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FineAlignDialog::onAlgorithmChanged);
    connect(m_startButton, &QPushButton::clicked, this, &FineAlignDialog::onStartClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &FineAlignDialog::onStopClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &FineAlignDialog::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_resetButton, &QPushButton::clicked, this, &FineAlignDialog::onResetClicked);
    
    connect(m_outlierRejectionCheck, &QCheckBox::toggled, [this](bool checked) {
        m_outlierThresholdSpin->setEnabled(checked);
        m_trimPercentageSpin->setEnabled(checked);
    });
}

void FineAlignDialog::setSourceMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh) {
    m_sourceMesh = mesh;
    if (mesh) {
        logMessage(tr("Source mesh set: %1 vertices, %2 faces")
                   .arg(mesh->vertexCount())
                   .arg(mesh->faceCount()));
    }
}

void FineAlignDialog::setTargetMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh) {
    m_targetMesh = mesh;
    if (mesh) {
        logMessage(tr("Target mesh set: %1 vertices, %2 faces")
                   .arg(mesh->vertexCount())
                   .arg(mesh->faceCount()));
    }
}

const dc3d::geometry::AlignmentResult& FineAlignDialog::result() const {
    return *m_result;
}

void FineAlignDialog::onAlgorithmChanged(int index) {
    Q_UNUSED(index);
    // Point-to-plane requires normals, could add validation here
}

void FineAlignDialog::onStartClicked() {
    if (!m_sourceMesh || !m_targetMesh) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Both source and target meshes must be set."));
        return;
    }
    
    setRunningState(true);
    runAlignment();
    setRunningState(false);
}

void FineAlignDialog::onStopClicked() {
    m_stopRequested = true;
    logMessage(tr("Stop requested..."));
}

void FineAlignDialog::onApplyClicked() {
    if (!m_result->success) {
        QMessageBox::warning(this, tr("Error"),
                             tr("No successful alignment to apply."));
        return;
    }
    
    emit alignmentApplied(*m_result);
    accept();
}

void FineAlignDialog::onResetClicked() {
    m_result = std::make_unique<dc3d::geometry::AlignmentResult>();
    m_progressBar->setValue(0);
    m_iterationLabel->setText(tr("0 / 0"));
    m_currentErrorLabel->setText(tr("-"));
    m_initialErrorLabel->setText(tr("-"));
    m_finalErrorLabel->setText(tr("-"));
    m_iterationsUsedLabel->setText(tr("-"));
    m_convergenceLabel->setText(tr("-"));
    m_correspondenceLabel->setText(tr("-"));
    m_applyButton->setEnabled(false);
    m_logText->clear();
    logMessage(tr("Reset"));
}

void FineAlignDialog::runAlignment() {
    m_stopRequested = false;
    logMessage(tr("Starting ICP alignment..."));
    
    // Build ICP options
    dc3d::geometry::ICPOptions options;
    options.algorithm = static_cast<dc3d::geometry::ICPAlgorithm>(
        m_algorithmCombo->currentData().toInt());
    options.maxIterations = m_maxIterationsSpin->value();
    options.convergenceThreshold = static_cast<float>(m_convergenceThresholdSpin->value());
    options.outlierRejection = m_outlierRejectionCheck->isChecked();
    options.outlierThreshold = static_cast<float>(m_outlierThresholdSpin->value());
    options.trimPercentage = static_cast<float>(m_trimPercentageSpin->value() / 100.0);
    options.correspondenceSampling = m_samplingRateSpin->value();
    
    float maxDist = static_cast<float>(m_maxDistanceSpin->value());
    if (maxDist > 0.0f) {
        options.maxCorrespondenceDistance = maxDist;
    }
    
    logMessage(tr("Algorithm: %1").arg(m_algorithmCombo->currentText()));
    logMessage(tr("Max iterations: %1, threshold: %2")
               .arg(options.maxIterations)
               .arg(options.convergenceThreshold, 0, 'e', 2));
    
    // Create working copy of source mesh
    dc3d::geometry::MeshData workingMesh = *m_sourceMesh;
    
    // Progress callback
    int maxIter = options.maxIterations;
    auto progressCallback = [this, maxIter](float progress) -> bool {
        int iter = static_cast<int>(progress * maxIter);
        
        // Update UI (must be done carefully for thread safety)
        m_progressBar->setValue(static_cast<int>(progress * 100));
        m_iterationLabel->setText(tr("%1 / %2").arg(iter).arg(maxIter));
        
        // Process events to keep UI responsive
        QApplication::processEvents();
        
        return !m_stopRequested;
    };
    
    // Run ICP
    dc3d::geometry::ICP icp;
    dc3d::geometry::ICPResult icpResult = icp.align(workingMesh, *m_targetMesh, options, progressCallback);
    
    // Convert to AlignmentResult
    m_result->success = icpResult.converged;
    m_result->transform = icpResult.transform;
    m_result->rmsError = icpResult.finalRMSError;
    m_result->iterationsUsed = icpResult.iterationsUsed;
    
    if (!icpResult.converged && m_stopRequested) {
        m_result->errorMessage = "Cancelled by user";
        logMessage(tr("Alignment cancelled"));
    } else if (!icpResult.converged) {
        m_result->errorMessage = "ICP did not converge";
        logMessage(tr("Alignment did not converge"));
    } else {
        logMessage(tr("Alignment completed successfully"));
        
        // Apply to actual source mesh
        m_sourceMesh->transform(icpResult.transform);
    }
    
    // Update statistics
    updateStatistics();
    m_progressBar->setValue(100);
    
    // Enable apply if successful
    m_applyButton->setEnabled(m_result->success);
    
    if (m_result->success) {
        emit previewRequested(*m_result);
    }
}

void FineAlignDialog::updateProgress(float progress) {
    m_progressBar->setValue(static_cast<int>(progress * 100));
    emit progressUpdated(progress);
}

void FineAlignDialog::updateStatistics() {
    // Note: ICPResult doesn't store initial error separately in current implementation
    // This would need to be captured during the alignment process
    m_initialErrorLabel->setText(tr("-")); // Would need to capture this
    m_finalErrorLabel->setText(tr("%1").arg(m_result->rmsError, 0, 'f', 8));
    m_iterationsUsedLabel->setText(tr("%1").arg(m_result->iterationsUsed));
    
    if (m_result->success) {
        m_convergenceLabel->setText(tr("Converged"));
        m_convergenceLabel->setStyleSheet("color: green;");
    } else if (m_stopRequested) {
        m_convergenceLabel->setText(tr("Cancelled"));
        m_convergenceLabel->setStyleSheet("color: orange;");
    } else {
        m_convergenceLabel->setText(tr("Did not converge"));
        m_convergenceLabel->setStyleSheet("color: red;");
    }
    
    m_correspondenceLabel->setText(tr("-")); // Would need to track this
}

void FineAlignDialog::setRunningState(bool running) {
    m_running = running;
    
    // Disable settings while running
    m_settingsGroup->setEnabled(!running);
    m_outlierGroup->setEnabled(!running);
    m_samplingGroup->setEnabled(!running);
    
    m_startButton->setEnabled(!running);
    m_stopButton->setEnabled(running);
    m_applyButton->setEnabled(!running && m_result->success);
    m_resetButton->setEnabled(!running);
}

void FineAlignDialog::logMessage(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logText->appendPlainText(QString("[%1] %2").arg(timestamp).arg(message));
}

} // namespace dc
