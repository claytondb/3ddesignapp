/**
 * @file AnalysisPanel.h
 * @brief UI panel for mesh analysis results and deviation visualization
 * 
 * Displays mesh statistics, deviation analysis results, and provides
 * controls for deviation visualization settings.
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QTableWidget>
#include <QProgressBar>
#include <memory>
#include <vector>

namespace dc3d {
namespace geometry {
struct MeshAnalysisStats;
struct DeviationStats;
class MeshData;
}
}

namespace dc {

class DeviationRenderer;
enum class DeviationColormap;

/**
 * @brief Custom widget for displaying a color gradient legend
 */
class ColorLegendWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ColorLegendWidget(QWidget* parent = nullptr);
    
    void setRange(float minVal, float maxVal);
    void setColormap(int colormapType);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    float minVal_ = -1.0f;
    float maxVal_ = 1.0f;
    int colormapType_ = 0;
};

/**
 * @brief Custom widget for displaying a histogram
 */
class HistogramWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    
    void setData(const std::vector<size_t>& bins, float minVal, float maxVal);
    void clear();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    std::vector<size_t> bins_;
    float minVal_ = 0.0f;
    float maxVal_ = 1.0f;
    size_t maxCount_ = 0;
};

/**
 * @brief Panel for displaying mesh analysis and deviation results
 */
class AnalysisPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit AnalysisPanel(QWidget* parent = nullptr);
    ~AnalysisPanel() override = default;
    
    // ---- Mesh Statistics ----
    
    /**
     * @brief Display mesh analysis statistics
     * @param stats Analysis results to display
     * @param meshName Name of the mesh
     */
    void setMeshStats(const dc3d::geometry::MeshAnalysisStats& stats, 
                      const QString& meshName = "Mesh");
    
    /**
     * @brief Clear mesh statistics display
     */
    void clearMeshStats();
    
    // ---- Deviation Analysis ----
    
    /**
     * @brief Display deviation analysis results
     * @param stats Deviation statistics
     * @param deviations Per-vertex deviation values
     * @param sourceName Name of source mesh
     * @param targetName Name of target mesh
     */
    void setDeviationStats(const dc3d::geometry::DeviationStats& stats,
                           const std::vector<float>& deviations,
                           const QString& sourceName = "Source",
                           const QString& targetName = "Target");
    
    /**
     * @brief Clear deviation analysis display
     */
    void clearDeviationStats();
    
    // ---- Progress ----
    
    /**
     * @brief Show analysis progress
     * @param progress Progress value 0-100
     * @param message Status message
     */
    void setProgress(int progress, const QString& message = QString());
    
    /**
     * @brief Hide progress bar
     */
    void hideProgress();
    
    // ---- Renderer Connection ----
    
    /**
     * @brief Connect to deviation renderer for live updates
     */
    void setDeviationRenderer(DeviationRenderer* renderer);
    
signals:
    /**
     * @brief Emitted when color range changes
     */
    void rangeChanged(float minVal, float maxVal);
    
    /**
     * @brief Emitted when colormap selection changes
     */
    void colormapChanged(int colormapType);
    
    /**
     * @brief Emitted when export report button is clicked
     */
    void exportReportRequested();
    
    /**
     * @brief Emitted when analyze button is clicked
     */
    void analyzeRequested();
    
    /**
     * @brief Emitted when compute deviation button is clicked
     */
    void computeDeviationRequested();
    
private slots:
    void onRangeChanged();
    void onColormapChanged(int index);
    void onAutoRangeToggled(bool checked);
    void onExportReport();
    
private:
    void setupUI();
    void setupMeshStatsSection();
    void setupDeviationSection();
    void setupControlsSection();
    
    void updateColorLegend();
    QString formatNumber(double value, int precision = 4) const;
    QString formatCount(size_t count) const;
    
    // Main layout
    QVBoxLayout* mainLayout_ = nullptr;
    
    // Mesh Statistics Section
    QGroupBox* meshStatsGroup_ = nullptr;
    QLabel* meshNameLabel_ = nullptr;
    QTableWidget* meshStatsTable_ = nullptr;
    QLabel* topologyStatusLabel_ = nullptr;
    
    // Deviation Section
    QGroupBox* deviationGroup_ = nullptr;
    QLabel* deviationHeaderLabel_ = nullptr;
    QTableWidget* deviationStatsTable_ = nullptr;
    HistogramWidget* histogramWidget_ = nullptr;
    ColorLegendWidget* colorLegendWidget_ = nullptr;
    
    // Controls Section
    QGroupBox* controlsGroup_ = nullptr;
    QComboBox* colormapCombo_ = nullptr;
    QCheckBox* autoRangeCheck_ = nullptr;
    QDoubleSpinBox* minRangeSpin_ = nullptr;
    QDoubleSpinBox* maxRangeSpin_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QPushButton* analyzeButton_ = nullptr;
    QPushButton* computeDeviationButton_ = nullptr;
    
    // Progress
    QProgressBar* progressBar_ = nullptr;
    QLabel* progressLabel_ = nullptr;
    
    // Connected renderer
    DeviationRenderer* renderer_ = nullptr;
    
    // Current deviation data
    std::vector<float> currentDeviations_;
};

} // namespace dc
