/**
 * @file SectionPlaneDialog.h
 * @brief Dialog for creating section planes through meshes
 * 
 * Features:
 * - Create section plane through mesh
 * - Offset distance slider
 * - Multiple sections option (for lofting)
 * - Live preview of section curve
 */

#ifndef DC_SECTIONPLANEDIALOG_H
#define DC_SECTIONPLANEDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVector3D>
#include <QSettings>
#include <memory>

namespace dc {
class Viewport;
}

/**
 * @brief Section plane orientation
 */
enum class SectionPlaneOrientation {
    XY,         ///< Horizontal (Z normal)
    XZ,         ///< Front (Y normal)
    YZ,         ///< Side (X normal)
    Custom      ///< User-defined normal
};

/**
 * @brief Section plane parameters
 */
struct SectionPlaneParams {
    SectionPlaneOrientation orientation = SectionPlaneOrientation::XY;
    QVector3D customNormal{0, 0, 1};
    QVector3D customOrigin{0, 0, 0};
    double offset = 0.0;
    
    // Multiple sections
    bool createMultiple = false;
    int sectionCount = 5;
    double startOffset = 0.0;
    double endOffset = 100.0;
    double spacing = 20.0;
    
    // Options
    bool autoFitCurves = true;
    bool createSketch = true;
    bool showPreview = true;
};

/**
 * @brief Dialog for creating section planes
 */
class SectionPlaneDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SectionPlaneDialog(QWidget* parent = nullptr);
    ~SectionPlaneDialog() override = default;
    
    /**
     * @brief Set viewport for preview
     */
    void setViewport(dc::Viewport* viewport);
    
    /**
     * @brief Set mesh bounds for offset range
     * @param min Minimum point of bounding box
     * @param max Maximum point of bounding box
     */
    void setMeshBounds(const QVector3D& min, const QVector3D& max);
    
    /**
     * @brief Get section plane parameters
     */
    SectionPlaneParams parameters() const;
    
    /**
     * @brief Set section plane parameters
     */
    void setParameters(const SectionPlaneParams& params);

signals:
    /**
     * @brief Emitted when parameters change for live preview
     */
    void parametersChanged(const SectionPlaneParams& params);
    
    /**
     * @brief Emitted when preview is requested
     */
    void previewRequested();
    
    /**
     * @brief Emitted when section should be created
     */
    void createRequested(const SectionPlaneParams& params);
    
    /**
     * @brief Emitted when multiple sections should be created
     */
    void createMultipleRequested(const SectionPlaneParams& params);
    
    /**
     * @brief Emitted when dialog is canceled to revert preview
     */
    void previewCanceled();

private slots:
    void onOrientationChanged(int index);
    void onOffsetChanged(double value);
    void onOffsetSliderChanged(int value);
    void onMultipleToggled(bool checked);
    void onSectionCountChanged(int value);
    void onStartOffsetChanged(double value);
    void onEndOffsetChanged(double value);
    void onSpacingChanged(double value);
    void onAutoFitToggled(bool checked);
    void onCreateSketchToggled(bool checked);
    void onPreviewToggled(bool checked);
    void onCreateClicked();
    void onCustomNormalClicked();
    
    void updateSpacingFromCount();
    void updateCountFromSpacing();
    void updateOffsetRange();
    void emitParametersChanged();
    void onResetClicked();
    void onCancelClicked();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();
    void updateMultipleControls();
    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    
    dc::Viewport* m_viewport = nullptr;
    
    // Mesh bounds
    QVector3D m_meshMin{-50, -50, -50};
    QVector3D m_meshMax{50, 50, 50};
    
    // Orientation controls
    QComboBox* m_orientationCombo;
    QPushButton* m_customNormalButton;
    QLabel* m_normalLabel;
    
    // Offset controls
    QDoubleSpinBox* m_offsetSpinbox;
    QSlider* m_offsetSlider;
    QLabel* m_offsetRangeLabel;
    
    // Multiple sections controls
    QCheckBox* m_multipleCheck;
    QWidget* m_multipleContainer;
    QSpinBox* m_countSpinbox;
    QDoubleSpinBox* m_startOffsetSpinbox;
    QDoubleSpinBox* m_endOffsetSpinbox;
    QDoubleSpinBox* m_spacingSpinbox;
    QComboBox* m_distributionCombo;
    
    // Options
    QCheckBox* m_autoFitCheck;
    QCheckBox* m_createSketchCheck;
    QCheckBox* m_previewCheck;
    
    // Preview info
    QLabel* m_previewInfoLabel;
    
    // Buttons
    QPushButton* m_resetButton;
    QPushButton* m_previewButton;
    QPushButton* m_createButton;
    QPushButton* m_cancelButton;
    
    // Current parameters
    SectionPlaneParams m_params;
    
    // State
    bool m_updatingControls = false;
};

#endif // DC_SECTIONPLANEDIALOG_H
