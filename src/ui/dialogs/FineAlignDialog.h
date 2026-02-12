/**
 * @file FineAlignDialog.h
 * @brief Dialog for fine alignment using ICP
 * 
 * Allows user to:
 * - Select ICP algorithm (point-to-point, point-to-plane)
 * - Configure max iterations and convergence threshold
 * - Monitor progress during alignment
 * - View result statistics (RMS error, iterations used)
 */

#pragma once

#include <QDialog>
#include <memory>

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QProgressBar;
class QPushButton;
class QLabel;
class QGroupBox;
class QCheckBox;
class QPlainTextEdit;

namespace dc3d {
namespace geometry {
class MeshData;
struct AlignmentResult;
enum class ICPAlgorithm;
}
}

namespace dc {

class Viewport;

/**
 * @brief Dialog for ICP fine alignment operations
 */
class FineAlignDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit FineAlignDialog(Viewport* viewport, QWidget* parent = nullptr);
    ~FineAlignDialog() override;
    
    /**
     * @brief Set the source mesh (mesh to be transformed)
     */
    void setSourceMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh);
    
    /**
     * @brief Set the target mesh (reference mesh)
     */
    void setTargetMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh);
    
    /**
     * @brief Get alignment result
     */
    const dc3d::geometry::AlignmentResult& result() const;

signals:
    /**
     * @brief Emitted during alignment to show progress
     */
    void progressUpdated(float progress);
    
    /**
     * @brief Emitted when preview should update
     */
    void previewRequested(const dc3d::geometry::AlignmentResult& result);
    
    /**
     * @brief Emitted when alignment is applied
     */
    void alignmentApplied(const dc3d::geometry::AlignmentResult& result);

private slots:
    void onAlgorithmChanged(int index);
    void onStartClicked();
    void onStopClicked();
    void onApplyClicked();
    void onResetClicked();
    
private:
    void setupUI();
    void runAlignment();
    void updateProgress(float progress);
    void updateStatistics();
    void setRunningState(bool running);
    void logMessage(const QString& message);
    
    Viewport* m_viewport;
    std::shared_ptr<dc3d::geometry::MeshData> m_sourceMesh;
    std::shared_ptr<dc3d::geometry::MeshData> m_targetMesh;
    std::unique_ptr<dc3d::geometry::AlignmentResult> m_result;
    
    // Running state
    bool m_running = false;
    bool m_stopRequested = false;
    
    // Algorithm settings
    QGroupBox* m_settingsGroup;
    QComboBox* m_algorithmCombo;
    QSpinBox* m_maxIterationsSpin;
    QDoubleSpinBox* m_convergenceThresholdSpin;
    
    // Outlier rejection
    QGroupBox* m_outlierGroup;
    QCheckBox* m_outlierRejectionCheck;
    QDoubleSpinBox* m_outlierThresholdSpin;
    QDoubleSpinBox* m_trimPercentageSpin;
    
    // Sampling
    QGroupBox* m_samplingGroup;
    QSpinBox* m_samplingRateSpin;
    QDoubleSpinBox* m_maxDistanceSpin;
    
    // Progress
    QGroupBox* m_progressGroup;
    QProgressBar* m_progressBar;
    QLabel* m_iterationLabel;
    QLabel* m_currentErrorLabel;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    
    // Results
    QGroupBox* m_resultsGroup;
    QLabel* m_initialErrorLabel;
    QLabel* m_finalErrorLabel;
    QLabel* m_iterationsUsedLabel;
    QLabel* m_convergenceLabel;
    QLabel* m_correspondenceLabel;
    
    // Log
    QPlainTextEdit* m_logText;
    
    // Dialog buttons
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QPushButton* m_resetButton;
};

} // namespace dc
