#ifndef DC_POLYGONREDUCTIONDIALOG_H
#define DC_POLYGONREDUCTIONDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QGroupBox>

namespace dc {
class Viewport;
}

/**
 * @brief Dialog for mesh polygon reduction operations
 * 
 * Provides controls for:
 * - Target type: Percentage, Vertex Count, Face Count
 * - Slider for percentage (1-100%)
 * - Spinbox for exact counts
 * - Preserve boundaries option
 * - Preview with viewport updates
 * - Progress bar during operation
 */
class PolygonReductionDialog : public QDialog
{
    Q_OBJECT

public:
    enum class TargetType {
        Percentage,
        VertexCount,
        FaceCount
    };

    explicit PolygonReductionDialog(QWidget *parent = nullptr);
    ~PolygonReductionDialog() override = default;

    // Set the viewport for preview updates
    void setViewport(dc::Viewport* viewport);

    // Set original mesh statistics
    void setOriginalTriangleCount(int count);
    void setOriginalVertexCount(int count);

    // Get reduction parameters
    TargetType targetType() const;
    double percentage() const;
    int targetVertexCount() const;
    int targetFaceCount() const;
    bool preserveBoundaries() const;
    bool preserveSharpFeatures() const;
    double sharpFeatureAngle() const;
    bool lockVertexColors() const;
    bool autoPreview() const;

    // Progress updates
    void setProgress(int percent);
    void setProgressText(const QString& text);

signals:
    void previewRequested();
    void applyRequested();
    void reductionStarted();
    void reductionFinished();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onTargetTypeChanged();
    void onPercentageSliderChanged(int value);
    void onPercentageSpinboxChanged(double value);
    void onVertexCountChanged(int value);
    void onFaceCountChanged(int value);
    void onPreviewToggled(bool checked);
    void onApplyClicked();
    void updateEstimatedResult();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();
    void updateControlVisibility();

    // Original mesh statistics
    int m_originalTriangleCount;
    int m_originalVertexCount;

    // Viewport for preview
    dc::Viewport* m_viewport;

    // Target type selection
    QButtonGroup* m_targetTypeGroup;
    QRadioButton* m_radioPercentage;
    QRadioButton* m_radioVertexCount;
    QRadioButton* m_radioFaceCount;

    // Percentage controls
    QWidget* m_percentageWidget;
    QSlider* m_percentageSlider;
    QDoubleSpinBox* m_percentageSpinbox;

    // Count controls
    QWidget* m_vertexCountWidget;
    QSpinBox* m_vertexCountSpinbox;
    
    QWidget* m_faceCountWidget;
    QSpinBox* m_faceCountSpinbox;

    // Options
    QGroupBox* m_optionsGroup;
    QCheckBox* m_preserveBoundaries;
    QCheckBox* m_preserveSharpFeatures;
    QDoubleSpinBox* m_sharpAngleSpinbox;
    QCheckBox* m_lockVertexColors;

    // Preview
    QCheckBox* m_autoPreviewCheck;

    // Info labels
    QLabel* m_originalCountLabel;
    QLabel* m_estimatedResultLabel;

    // Progress
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;

    // Buttons
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
};

#endif // DC_POLYGONREDUCTIONDIALOG_H
