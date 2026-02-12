/**
 * @file NPointAlignDialog.h
 * @brief Dialog for N-point correspondence alignment
 * 
 * Allows user to:
 * - Pick corresponding point pairs on source and target meshes
 * - Minimum 3 pairs required
 * - View and manage point pairs in a table
 * - See alignment error and apply transformation
 */

#pragma once

#include <QDialog>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

class QTableWidget;
class QPushButton;
class QLabel;
class QGroupBox;
class QCheckBox;

namespace dc3d {
namespace geometry {
class MeshData;
struct PointPair;
struct AlignmentResult;
}
}

namespace dc {

class Viewport;

/**
 * @brief Row data for point pair table
 */
struct PointPairRow {
    int id;
    glm::vec3 source;
    glm::vec3 target;
    float error;
    bool valid;
};

/**
 * @brief Dialog for N-point alignment operations
 */
class NPointAlignDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit NPointAlignDialog(Viewport* viewport, QWidget* parent = nullptr);
    ~NPointAlignDialog() override;
    
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
    
    /**
     * @brief Get current point pairs
     */
    std::vector<dc3d::geometry::PointPair> pointPairs() const;

signals:
    /**
     * @brief Request to pick a point on source mesh
     */
    void requestSourcePointPick(int pairId);
    
    /**
     * @brief Request to pick a point on target mesh
     */
    void requestTargetPointPick(int pairId);
    
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
     * @brief Called when a source point has been picked
     */
    void onSourcePointPicked(int pairId, const glm::vec3& point);
    
    /**
     * @brief Called when a target point has been picked
     */
    void onTargetPointPicked(int pairId, const glm::vec3& point);
    
private slots:
    void onAddPairClicked();
    void onRemovePairClicked();
    void onClearAllClicked();
    void onPickSourceClicked();
    void onPickTargetClicked();
    void onTableSelectionChanged();
    void onPreviewClicked();
    void onApplyClicked();
    
private:
    void setupUI();
    void updateTable();
    void updateStatistics();
    void validateInputs();
    int getSelectedPairIndex() const;
    void computePreviewErrors();
    
    Viewport* m_viewport;
    std::shared_ptr<dc3d::geometry::MeshData> m_sourceMesh;
    std::shared_ptr<dc3d::geometry::MeshData> m_targetMesh;
    std::unique_ptr<dc3d::geometry::AlignmentResult> m_result;
    
    // Point pairs
    std::vector<PointPairRow> m_pairs;
    int m_nextPairId = 1;
    
    // Picking state
    enum class PickMode { None, Source, Target };
    PickMode m_pickMode = PickMode::None;
    int m_pickingPairId = -1;
    
    // UI Elements
    QTableWidget* m_table;
    
    QPushButton* m_addPairButton;
    QPushButton* m_removePairButton;
    QPushButton* m_clearAllButton;
    
    QPushButton* m_pickSourceButton;
    QPushButton* m_pickTargetButton;
    
    // Statistics
    QGroupBox* m_statsGroup;
    QLabel* m_pairCountLabel;
    QLabel* m_rmsErrorLabel;
    QLabel* m_maxErrorLabel;
    QLabel* m_statusLabel;
    
    // Preview and apply
    QCheckBox* m_livePreviewCheck;
    QPushButton* m_previewButton;
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
};

} // namespace dc
