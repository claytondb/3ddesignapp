/**
 * @file AlignToWCSDialog.h
 * @brief Dialog for aligning mesh to World Coordinate System
 * 
 * Allows user to:
 * - Select primary axis feature (plane normal, axis direction)
 * - Select secondary axis feature
 * - Choose origin point
 * - Preview and apply alignment
 */

#pragma once

#include <QDialog>
#include <memory>

class QComboBox;
class QPushButton;
class QLabel;
class QGroupBox;
class QDoubleSpinBox;
class QCheckBox;

namespace dc3d {
namespace geometry {
class MeshData;
struct AlignmentFeatureData;
struct AlignmentResult;
enum class WCSAxis;
}
}

namespace dc {

class Viewport;

/**
 * @brief Feature selection state
 */
struct FeatureSelection {
    bool valid = false;
    dc3d::geometry::AlignmentFeatureData* feature = nullptr;
    QString description;
};

/**
 * @brief Dialog for WCS alignment operations
 */
class AlignToWCSDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit AlignToWCSDialog(Viewport* viewport, QWidget* parent = nullptr);
    ~AlignToWCSDialog() override;
    
    /**
     * @brief Set the mesh to align
     */
    void setMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh);
    
    /**
     * @brief Get alignment result
     */
    const dc3d::geometry::AlignmentResult& result() const;

signals:
    /**
     * @brief Request to pick a feature on the mesh
     */
    void requestFeaturePick(const QString& featureType);
    
    /**
     * @brief Emitted when preview should update
     */
    void previewRequested(const dc3d::geometry::AlignmentResult& result);
    
    /**
     * @brief Emitted when alignment is applied
     */
    void alignmentApplied(const dc3d::geometry::AlignmentResult& result);

public slots:
    /**
     * @brief Called when a feature has been picked in viewport
     */
    void onFeaturePicked(const dc3d::geometry::AlignmentFeatureData& feature);
    
private slots:
    void onPickPrimaryClicked();
    void onPickSecondaryClicked();
    void onPickOriginClicked();
    void onPrimaryAxisChanged(int index);
    void onSecondaryAxisChanged(int index);
    void onPreviewClicked();
    void onApplyClicked();
    void onResetClicked();
    
private:
    void setupUI();
    void updatePreview();
    void validateInputs();
    dc3d::geometry::WCSAxis getSelectedAxis(QComboBox* combo) const;
    
    Viewport* m_viewport;
    std::shared_ptr<dc3d::geometry::MeshData> m_mesh;
    std::unique_ptr<dc3d::geometry::AlignmentResult> m_result;
    
    // Primary axis
    QGroupBox* m_primaryGroup;
    QLabel* m_primaryFeatureLabel;
    QPushButton* m_pickPrimaryButton;
    QComboBox* m_primaryAxisCombo;
    std::unique_ptr<dc3d::geometry::AlignmentFeatureData> m_primaryFeature;
    
    // Secondary axis
    QGroupBox* m_secondaryGroup;
    QLabel* m_secondaryFeatureLabel;
    QPushButton* m_pickSecondaryButton;
    QComboBox* m_secondaryAxisCombo;
    std::unique_ptr<dc3d::geometry::AlignmentFeatureData> m_secondaryFeature;
    
    // Origin
    QGroupBox* m_originGroup;
    QLabel* m_originLabel;
    QPushButton* m_pickOriginButton;
    QCheckBox* m_useFeatureOriginCheck;
    QDoubleSpinBox* m_originX;
    QDoubleSpinBox* m_originY;
    QDoubleSpinBox* m_originZ;
    bool m_originSet = false;
    
    // Preview
    QCheckBox* m_livePreviewCheck;
    QPushButton* m_previewButton;
    
    // Buttons
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QPushButton* m_resetButton;
    
    // State
    enum class PickMode {
        None,
        Primary,
        Secondary,
        Origin
    };
    PickMode m_pickMode = PickMode::None;
};

} // namespace dc
