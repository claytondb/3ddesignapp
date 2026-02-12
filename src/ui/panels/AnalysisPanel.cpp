/**
 * @file AnalysisPanel.cpp
 * @brief Implementation of the analysis panel UI
 */

#include "AnalysisPanel.h"
#include "../../geometry/MeshAnalysis.h"
#include "../../geometry/DeviationAnalysis.h"
#include "../../renderer/DeviationRenderer.h"

#include <QPainter>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <algorithm>
#include <cmath>

namespace dc {

// ============================================================================
// ColorLegendWidget
// ============================================================================

ColorLegendWidget::ColorLegendWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 30);
    setMaximumHeight(40);
}

void ColorLegendWidget::setRange(float minVal, float maxVal) {
    minVal_ = minVal;
    maxVal_ = maxVal;
    update();
}

void ColorLegendWidget::setColormap(int colormapType) {
    colormapType_ = colormapType;
    update();
}

void ColorLegendWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    int barHeight = 20;
    int margin = 5;
    
    // Draw color gradient
    for (int x = margin; x < w - margin; ++x) {
        float t = static_cast<float>(x - margin) / (w - 2 * margin);
        
        QColor color;
        switch (colormapType_) {
            case 0:  // BlueGreenRed
                if (t < 0.5f) {
                    float s = t * 2.0f;
                    color = QColor::fromRgbF(0.0, s, 1.0 - s);
                } else {
                    float s = (t - 0.5f) * 2.0f;
                    color = QColor::fromRgbF(s, 1.0 - s, 0.0);
                }
                break;
            case 2:  // CoolWarm
                if (t < 0.5f) {
                    float s = t * 2.0f;
                    color = QColor::fromRgbF(
                        0.231 + s * (0.867 - 0.231),
                        0.298 + s * (0.867 - 0.298),
                        0.753 + s * (0.867 - 0.753)
                    );
                } else {
                    float s = (t - 0.5f) * 2.0f;
                    color = QColor::fromRgbF(
                        0.867 + s * (0.706 - 0.867),
                        0.867 + s * (0.016 - 0.867),
                        0.867 + s * (0.150 - 0.867)
                    );
                }
                break;
            default:  // Default to blue-green-red
                if (t < 0.5f) {
                    float s = t * 2.0f;
                    color = QColor::fromRgbF(0.0, s, 1.0 - s);
                } else {
                    float s = (t - 0.5f) * 2.0f;
                    color = QColor::fromRgbF(s, 1.0 - s, 0.0);
                }
                break;
        }
        
        painter.setPen(color);
        painter.drawLine(x, margin, x, margin + barHeight);
    }
    
    // Draw border
    painter.setPen(Qt::darkGray);
    painter.drawRect(margin, margin, w - 2 * margin, barHeight);
    
    // Draw labels
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    
    QString minLabel = QString::number(minVal_, 'g', 3);
    QString maxLabel = QString::number(maxVal_, 'g', 3);
    QString zeroLabel = "0";
    
    painter.drawText(margin, h - 2, minLabel);
    painter.drawText(w - margin - painter.fontMetrics().horizontalAdvance(maxLabel), h - 2, maxLabel);
    
    if (minVal_ < 0 && maxVal_ > 0) {
        float zeroPos = -minVal_ / (maxVal_ - minVal_);
        int zeroX = margin + static_cast<int>(zeroPos * (w - 2 * margin));
        painter.drawText(zeroX - 3, h - 2, zeroLabel);
    }
}

// ============================================================================
// HistogramWidget
// ============================================================================

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 80);
    setMaximumHeight(120);
}

void HistogramWidget::setData(const std::vector<size_t>& bins, float minVal, float maxVal) {
    bins_ = bins;
    minVal_ = minVal;
    maxVal_ = maxVal;
    
    if (!bins_.empty()) {
        maxCount_ = *std::max_element(bins_.begin(), bins_.end());
    } else {
        maxCount_ = 0;
    }
    
    update();
}

void HistogramWidget::clear() {
    bins_.clear();
    maxCount_ = 0;
    update();
}

void HistogramWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    int margin = 5;
    int barAreaWidth = w - 2 * margin;
    int barAreaHeight = h - 2 * margin - 15;  // Leave space for labels
    
    // Background
    painter.fillRect(rect(), Qt::white);
    painter.setPen(Qt::darkGray);
    painter.drawRect(margin, margin, barAreaWidth, barAreaHeight);
    
    if (bins_.empty() || maxCount_ == 0) {
        painter.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }
    
    // Draw bars
    float barWidth = static_cast<float>(barAreaWidth) / bins_.size();
    
    for (size_t i = 0; i < bins_.size(); ++i) {
        float barHeight = static_cast<float>(bins_[i]) / maxCount_ * barAreaHeight;
        
        float t = static_cast<float>(i) / bins_.size();
        
        // Color based on position (blue-green-red)
        QColor color;
        if (t < 0.5f) {
            float s = t * 2.0f;
            color = QColor::fromRgbF(0.0, s, 1.0 - s);
        } else {
            float s = (t - 0.5f) * 2.0f;
            color = QColor::fromRgbF(s, 1.0 - s, 0.0);
        }
        
        int x = margin + static_cast<int>(i * barWidth);
        int y = margin + barAreaHeight - static_cast<int>(barHeight);
        
        painter.fillRect(x, y, static_cast<int>(barWidth) - 1, static_cast<int>(barHeight), color);
    }
    
    // Draw labels
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    
    QString minLabel = QString::number(minVal_, 'g', 3);
    QString maxLabel = QString::number(maxVal_, 'g', 3);
    
    painter.drawText(margin, h - 2, minLabel);
    painter.drawText(w - margin - painter.fontMetrics().horizontalAdvance(maxLabel), h - 2, maxLabel);
}

// ============================================================================
// AnalysisPanel
// ============================================================================

AnalysisPanel::AnalysisPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void AnalysisPanel::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(10, 10, 10, 10);
    mainLayout_->setSpacing(10);
    
    setupMeshStatsSection();
    setupDeviationSection();
    setupControlsSection();
    
    // Progress bar
    progressBar_ = new QProgressBar();
    progressBar_->setVisible(false);
    progressLabel_ = new QLabel();
    progressLabel_->setVisible(false);
    
    mainLayout_->addWidget(progressLabel_);
    mainLayout_->addWidget(progressBar_);
    mainLayout_->addStretch();
}

void AnalysisPanel::setupMeshStatsSection() {
    meshStatsGroup_ = new QGroupBox("Mesh Statistics");
    auto* layout = new QVBoxLayout(meshStatsGroup_);
    
    meshNameLabel_ = new QLabel("No mesh selected");
    meshNameLabel_->setStyleSheet("font-weight: bold;");
    layout->addWidget(meshNameLabel_);
    
    meshStatsTable_ = new QTableWidget();
    meshStatsTable_->setColumnCount(2);
    meshStatsTable_->setHorizontalHeaderLabels({"Property", "Value"});
    meshStatsTable_->horizontalHeader()->setStretchLastSection(true);
    meshStatsTable_->verticalHeader()->setVisible(false);
    meshStatsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    meshStatsTable_->setAlternatingRowColors(true);
    meshStatsTable_->setMaximumHeight(300);
    layout->addWidget(meshStatsTable_);
    
    topologyStatusLabel_ = new QLabel();
    layout->addWidget(topologyStatusLabel_);
    
    analyzeButton_ = new QPushButton("Analyze Mesh");
    connect(analyzeButton_, &QPushButton::clicked, this, &AnalysisPanel::analyzeRequested);
    layout->addWidget(analyzeButton_);
    
    mainLayout_->addWidget(meshStatsGroup_);
}

void AnalysisPanel::setupDeviationSection() {
    deviationGroup_ = new QGroupBox("Deviation Analysis");
    auto* layout = new QVBoxLayout(deviationGroup_);
    
    deviationHeaderLabel_ = new QLabel("No deviation data");
    deviationHeaderLabel_->setStyleSheet("font-weight: bold;");
    layout->addWidget(deviationHeaderLabel_);
    
    deviationStatsTable_ = new QTableWidget();
    deviationStatsTable_->setColumnCount(2);
    deviationStatsTable_->setHorizontalHeaderLabels({"Statistic", "Value"});
    deviationStatsTable_->horizontalHeader()->setStretchLastSection(true);
    deviationStatsTable_->verticalHeader()->setVisible(false);
    deviationStatsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviationStatsTable_->setAlternatingRowColors(true);
    deviationStatsTable_->setMaximumHeight(200);
    layout->addWidget(deviationStatsTable_);
    
    // Histogram
    auto* histLabel = new QLabel("Distribution:");
    layout->addWidget(histLabel);
    
    histogramWidget_ = new HistogramWidget();
    layout->addWidget(histogramWidget_);
    
    // Color legend
    auto* legendLabel = new QLabel("Color Scale:");
    layout->addWidget(legendLabel);
    
    colorLegendWidget_ = new ColorLegendWidget();
    layout->addWidget(colorLegendWidget_);
    
    computeDeviationButton_ = new QPushButton("Compute Deviation");
    connect(computeDeviationButton_, &QPushButton::clicked, 
            this, &AnalysisPanel::computeDeviationRequested);
    layout->addWidget(computeDeviationButton_);
    
    mainLayout_->addWidget(deviationGroup_);
}

void AnalysisPanel::setupControlsSection() {
    controlsGroup_ = new QGroupBox("Visualization Settings");
    auto* layout = new QVBoxLayout(controlsGroup_);
    
    // Colormap selection
    auto* colormapLayout = new QHBoxLayout();
    colormapLayout->addWidget(new QLabel("Colormap:"));
    colormapCombo_ = new QComboBox();
    colormapCombo_->addItem("Blue-Green-Red", 0);
    colormapCombo_->addItem("Rainbow", 1);
    colormapCombo_->addItem("Cool-Warm", 2);
    colormapCombo_->addItem("Viridis", 3);
    colormapCombo_->addItem("Magma", 4);
    colormapCombo_->addItem("Grayscale", 5);
    connect(colormapCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AnalysisPanel::onColormapChanged);
    colormapLayout->addWidget(colormapCombo_);
    layout->addLayout(colormapLayout);
    
    // Auto range
    autoRangeCheck_ = new QCheckBox("Auto Range");
    autoRangeCheck_->setChecked(true);
    connect(autoRangeCheck_, &QCheckBox::toggled, this, &AnalysisPanel::onAutoRangeToggled);
    layout->addWidget(autoRangeCheck_);
    
    // Manual range
    auto* rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(new QLabel("Min:"));
    minRangeSpin_ = new QDoubleSpinBox();
    minRangeSpin_->setRange(-1000.0, 1000.0);
    minRangeSpin_->setDecimals(4);
    minRangeSpin_->setValue(-1.0);
    minRangeSpin_->setEnabled(false);
    connect(minRangeSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnalysisPanel::onRangeChanged);
    rangeLayout->addWidget(minRangeSpin_);
    
    rangeLayout->addWidget(new QLabel("Max:"));
    maxRangeSpin_ = new QDoubleSpinBox();
    maxRangeSpin_->setRange(-1000.0, 1000.0);
    maxRangeSpin_->setDecimals(4);
    maxRangeSpin_->setValue(1.0);
    maxRangeSpin_->setEnabled(false);
    connect(maxRangeSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnalysisPanel::onRangeChanged);
    rangeLayout->addWidget(maxRangeSpin_);
    layout->addLayout(rangeLayout);
    
    // Export button
    exportButton_ = new QPushButton("Export Report...");
    connect(exportButton_, &QPushButton::clicked, this, &AnalysisPanel::onExportReport);
    layout->addWidget(exportButton_);
    
    mainLayout_->addWidget(controlsGroup_);
}

void AnalysisPanel::setMeshStats(const dc3d::geometry::MeshAnalysisStats& stats,
                                  const QString& meshName) {
    meshNameLabel_->setText(meshName);
    
    meshStatsTable_->setRowCount(0);
    
    auto addRow = [this](const QString& name, const QString& value) {
        int row = meshStatsTable_->rowCount();
        meshStatsTable_->insertRow(row);
        meshStatsTable_->setItem(row, 0, new QTableWidgetItem(name));
        meshStatsTable_->setItem(row, 1, new QTableWidgetItem(value));
    };
    
    // Basic counts
    addRow("Vertices", formatCount(stats.vertexCount));
    addRow("Faces", formatCount(stats.faceCount));
    addRow("Edges", formatCount(stats.edgeCount));
    
    // Geometry
    addRow("Surface Area", formatNumber(stats.surfaceArea) + " mm²");
    if (stats.volumeValid) {
        addRow("Volume", formatNumber(stats.volume) + " mm³");
    }
    
    // Bounding box
    auto dims = stats.bounds.dimensions();
    addRow("Dimensions", QString("%1 × %2 × %3 mm")
           .arg(formatNumber(dims.x))
           .arg(formatNumber(dims.y))
           .arg(formatNumber(dims.z)));
    
    // Edge lengths
    addRow("Min Edge Length", formatNumber(stats.minEdgeLength) + " mm");
    addRow("Max Edge Length", formatNumber(stats.maxEdgeLength) + " mm");
    addRow("Avg Edge Length", formatNumber(stats.avgEdgeLength) + " mm");
    
    // Face quality
    size_t totalFaces = stats.aspectRatios.excellent + stats.aspectRatios.good +
                        stats.aspectRatios.fair + stats.aspectRatios.poor +
                        stats.aspectRatios.terrible;
    if (totalFaces > 0) {
        float goodPercent = 100.0f * (stats.aspectRatios.excellent + stats.aspectRatios.good) / totalFaces;
        addRow("Good Quality Faces", QString("%1%").arg(formatNumber(goodPercent, 1)));
    }
    
    // Topology
    addRow("Boundary Edges", formatCount(stats.boundaryEdgeCount));
    addRow("Non-Manifold Edges", formatCount(stats.nonManifoldEdgeCount));
    addRow("Holes", formatCount(stats.holeCount));
    
    // Data flags
    addRow("Has Normals", stats.hasNormals ? "Yes" : "No");
    addRow("Has UVs", stats.hasUVs ? "Yes" : "No");
    
    // Topology status
    QString statusText;
    QString statusStyle;
    
    if (stats.isWatertight) {
        statusText = "✓ Watertight (manifold, closed, consistent winding)";
        statusStyle = "color: green; font-weight: bold;";
    } else if (stats.isManifold) {
        statusText = "○ Manifold but not watertight";
        statusStyle = "color: orange; font-weight: bold;";
    } else {
        statusText = "✗ Non-manifold geometry detected";
        statusStyle = "color: red; font-weight: bold;";
    }
    
    topologyStatusLabel_->setText(statusText);
    topologyStatusLabel_->setStyleSheet(statusStyle);
}

void AnalysisPanel::clearMeshStats() {
    meshNameLabel_->setText("No mesh selected");
    meshStatsTable_->setRowCount(0);
    topologyStatusLabel_->clear();
}

void AnalysisPanel::setDeviationStats(const dc3d::geometry::DeviationStats& stats,
                                       const std::vector<float>& deviations,
                                       const QString& sourceName,
                                       const QString& targetName) {
    currentDeviations_ = deviations;
    
    deviationHeaderLabel_->setText(QString("%1 → %2").arg(sourceName, targetName));
    
    deviationStatsTable_->setRowCount(0);
    
    auto addRow = [this](const QString& name, const QString& value) {
        int row = deviationStatsTable_->rowCount();
        deviationStatsTable_->insertRow(row);
        deviationStatsTable_->setItem(row, 0, new QTableWidgetItem(name));
        deviationStatsTable_->setItem(row, 1, new QTableWidgetItem(value));
    };
    
    addRow("Total Points", formatCount(stats.totalPoints));
    addRow("Min Deviation", formatNumber(stats.minDeviation) + " mm");
    addRow("Max Deviation", formatNumber(stats.maxDeviation) + " mm");
    addRow("Average", formatNumber(stats.avgDeviation) + " mm");
    addRow("Std Dev", formatNumber(stats.stddevDeviation) + " mm");
    addRow("RMS", formatNumber(stats.rmsDeviation) + " mm");
    
    // Signed statistics
    if (stats.minSigned != stats.maxSigned) {
        addRow("Min Signed", formatNumber(stats.minSigned) + " mm");
        addRow("Max Signed", formatNumber(stats.maxSigned) + " mm");
        addRow("Avg Signed", formatNumber(stats.avgSigned) + " mm");
    }
    
    // Percentiles
    addRow("Median (50%)", formatNumber(stats.percentile50) + " mm");
    addRow("90th Percentile", formatNumber(stats.percentile90) + " mm");
    addRow("95th Percentile", formatNumber(stats.percentile95) + " mm");
    addRow("99th Percentile", formatNumber(stats.percentile99) + " mm");
    
    // Tolerance
    float tolerancePercent = 100.0f * stats.pointsWithinTolerance / stats.totalPoints;
    addRow(QString("Within %1mm").arg(formatNumber(stats.toleranceThreshold, 2)),
           QString("%1% (%2 points)")
           .arg(formatNumber(tolerancePercent, 1))
           .arg(formatCount(stats.pointsWithinTolerance)));
    
    // Update histogram
    auto histogram = dc3d::geometry::DeviationAnalysis::createHistogram(deviations, 50);
    float minVal = stats.minSigned != 0 ? stats.minSigned : -stats.maxDeviation;
    float maxVal = stats.maxSigned != 0 ? stats.maxSigned : stats.maxDeviation;
    histogramWidget_->setData(histogram, minVal, maxVal);
    
    // Update color legend
    colorLegendWidget_->setRange(minVal, maxVal);
    
    // Update range spinners
    if (autoRangeCheck_->isChecked()) {
        minRangeSpin_->blockSignals(true);
        maxRangeSpin_->blockSignals(true);
        minRangeSpin_->setValue(minVal);
        maxRangeSpin_->setValue(maxVal);
        minRangeSpin_->blockSignals(false);
        maxRangeSpin_->blockSignals(false);
    }
}

void AnalysisPanel::clearDeviationStats() {
    deviationHeaderLabel_->setText("No deviation data");
    deviationStatsTable_->setRowCount(0);
    histogramWidget_->clear();
    currentDeviations_.clear();
}

void AnalysisPanel::setProgress(int progress, const QString& message) {
    progressBar_->setValue(progress);
    progressBar_->setVisible(true);
    
    if (!message.isEmpty()) {
        progressLabel_->setText(message);
    }
    progressLabel_->setVisible(true);
}

void AnalysisPanel::hideProgress() {
    progressBar_->setVisible(false);
    progressLabel_->setVisible(false);
}

void AnalysisPanel::setDeviationRenderer(DeviationRenderer* renderer) {
    renderer_ = renderer;
}

void AnalysisPanel::onRangeChanged() {
    float minVal = static_cast<float>(minRangeSpin_->value());
    float maxVal = static_cast<float>(maxRangeSpin_->value());
    
    colorLegendWidget_->setRange(minVal, maxVal);
    
    if (renderer_) {
        renderer_->setRange(minVal, maxVal);
    }
    
    emit rangeChanged(minVal, maxVal);
}

void AnalysisPanel::onColormapChanged(int index) {
    int colormapType = colormapCombo_->itemData(index).toInt();
    
    colorLegendWidget_->setColormap(colormapType);
    
    if (renderer_) {
        renderer_->setColormap(static_cast<DeviationColormap>(colormapType));
    }
    
    emit colormapChanged(colormapType);
}

void AnalysisPanel::onAutoRangeToggled(bool checked) {
    minRangeSpin_->setEnabled(!checked);
    maxRangeSpin_->setEnabled(!checked);
    
    if (renderer_) {
        renderer_->setAutoRange(checked);
    }
}

void AnalysisPanel::onExportReport() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Export Analysis Report",
        QString(),
        "Text Files (*.txt);;CSV Files (*.csv);;All Files (*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    
    QTextStream out(&file);
    
    out << "=== Mesh Analysis Report ===" << Qt::endl;
    out << "Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << Qt::endl;
    out << Qt::endl;
    
    // Mesh statistics
    out << "--- Mesh Statistics ---" << Qt::endl;
    for (int row = 0; row < meshStatsTable_->rowCount(); ++row) {
        out << meshStatsTable_->item(row, 0)->text() << ": "
            << meshStatsTable_->item(row, 1)->text() << Qt::endl;
    }
    out << Qt::endl;
    
    // Deviation statistics
    if (deviationStatsTable_->rowCount() > 0) {
        out << "--- Deviation Analysis ---" << Qt::endl;
        out << "Comparison: " << deviationHeaderLabel_->text() << Qt::endl;
        for (int row = 0; row < deviationStatsTable_->rowCount(); ++row) {
            out << deviationStatsTable_->item(row, 0)->text() << ": "
                << deviationStatsTable_->item(row, 1)->text() << Qt::endl;
        }
    }
    
    file.close();
    
    emit exportReportRequested();
}

void AnalysisPanel::updateColorLegend() {
    colorLegendWidget_->setRange(
        static_cast<float>(minRangeSpin_->value()),
        static_cast<float>(maxRangeSpin_->value())
    );
    colorLegendWidget_->setColormap(colormapCombo_->currentData().toInt());
}

QString AnalysisPanel::formatNumber(double value, int precision) const {
    return QString::number(value, 'g', precision);
}

QString AnalysisPanel::formatCount(size_t count) const {
    if (count >= 1000000) {
        return QString("%1M").arg(count / 1000000.0, 0, 'f', 2);
    } else if (count >= 1000) {
        return QString("%1K").arg(count / 1000.0, 0, 'f', 1);
    }
    return QString::number(count);
}

} // namespace dc
