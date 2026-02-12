#ifndef DC_OUTLIERREMOVALDIALOG_H
#define DC_OUTLIERREMOVALDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QProgressBar>

namespace dc {
class Viewport;
}

/**
 * @brief Dialog for mesh outlier removal operations
 * 
 * Provides controls for:
 * - Distance threshold slider
 * - Minimum cluster size spinbox
 * - Preview highlighting of outliers
 * - Remove button
 */
class OutlierRemovalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OutlierRemovalDialog(QWidget *parent = nullptr);
    ~OutlierRemovalDialog() override = default;

    // Set the viewport for preview updates
    void setViewport(dc::Viewport* viewport);

    // Set mesh statistics
    void setVertexCount(int count);
    void setOutlierCount(int count);

    // Get removal parameters
    double distanceThreshold() const;
    int minimumClusterSize() const;
    bool previewEnabled() const;
    int standardDeviations() const;  // Alternative method

signals:
    void analyzeRequested();
    void previewRequested();
    void removeRequested();

private slots:
    void onThresholdSliderChanged(int value);
    void onThresholdSpinboxChanged(double value);
    void onStdDevChanged(int value);
    void onClusterSizeChanged(int value);
    void onPreviewToggled(bool checked);
    void onAnalyzeClicked();
    void onRemoveClicked();
    void updateEstimatedRemoval();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();

    // Viewport for preview
    dc::Viewport* m_viewport;

    // Statistics
    int m_vertexCount;
    int m_outlierCount;

    // Threshold controls
    QSlider* m_thresholdSlider;
    QDoubleSpinBox* m_thresholdSpinbox;
    
    // Standard deviation method
    QSpinBox* m_stdDevSpinbox;

    // Cluster size
    QSpinBox* m_clusterSizeSpinbox;

    // Preview
    QCheckBox* m_previewCheck;

    // Info labels
    QLabel* m_vertexCountLabel;
    QLabel* m_outlierCountLabel;
    QLabel* m_estimatedLabel;

    // Progress
    QProgressBar* m_progressBar;

    // Buttons
    QPushButton* m_analyzeButton;
    QPushButton* m_removeButton;
    QPushButton* m_closeButton;
};

#endif // DC_OUTLIERREMOVALDIALOG_H
