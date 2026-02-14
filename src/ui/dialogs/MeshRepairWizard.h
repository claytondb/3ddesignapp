#ifndef DC_MESHREPAIRWIZARD_H
#define DC_MESHREPAIRWIZARD_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QSpinBox>
#include <QSettings>
#include <memory>

namespace dc {
class Viewport;
}

namespace dc3d {
namespace geometry {
class MeshData;
struct MeshHealthReport;
}
}

/**
 * @brief One-click mesh repair wizard for scanned meshes
 * 
 * Provides a simple interface for fixing common mesh issues:
 * - Analyze mesh and report problems
 * - Fill holes (with configurable max size)
 * - Remove non-manifold geometry
 * - Remove degenerate faces
 * - Remove isolated vertices
 * - Optional smoothing
 * 
 * Designed to be accessible to non-experts with sensible defaults.
 */
class MeshRepairWizard : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Statistics about mesh issues
     */
    struct MeshIssues {
        size_t holeCount = 0;
        size_t nonManifoldEdges = 0;
        size_t nonManifoldVertices = 0;
        size_t degenerateFaces = 0;
        size_t isolatedVertices = 0;
        size_t duplicateVertices = 0;
        
        // Additional info
        size_t vertexCount = 0;
        size_t faceCount = 0;
        size_t boundaryEdges = 0;
        bool isWatertight = false;
        bool isManifold = false;
        
        bool hasIssues() const {
            return holeCount > 0 || nonManifoldEdges > 0 || 
                   degenerateFaces > 0 || isolatedVertices > 0 ||
                   duplicateVertices > 0;
        }
        
        size_t totalIssues() const {
            return holeCount + nonManifoldEdges + degenerateFaces + 
                   isolatedVertices + duplicateVertices;
        }
    };
    
    /**
     * @brief Results from repair operation
     */
    struct RepairResults {
        size_t holesFilled = 0;
        size_t nonManifoldFixed = 0;
        size_t degenerateFacesRemoved = 0;
        size_t isolatedVerticesRemoved = 0;
        size_t duplicateVerticesMerged = 0;
        bool smoothingApplied = false;
        bool success = true;
        QString message;
    };

    explicit MeshRepairWizard(QWidget *parent = nullptr);
    ~MeshRepairWizard() override = default;

    /**
     * @brief Set the viewport for preview updates
     */
    void setViewport(dc::Viewport* viewport);
    
    /**
     * @brief Set the detected mesh issues to display
     */
    void setMeshIssues(const MeshIssues& issues);
    
    /**
     * @brief Clear all issues (no mesh selected)
     */
    void clearIssues();
    
    /**
     * @brief Set the repair results after operation completes
     */
    void setRepairResults(const RepairResults& results);
    
    // Options accessors
    bool fillHolesEnabled() const;
    bool removeNonManifoldEnabled() const;
    bool removeDegenerateFacesEnabled() const;
    bool removeIsolatedVerticesEnabled() const;
    bool removeDuplicateVerticesEnabled() const;
    bool smoothResultEnabled() const;
    int maxHoleSize() const;
    int smoothIterations() const;

signals:
    /**
     * @brief Request mesh analysis
     */
    void analyzeRequested();
    
    /**
     * @brief Request to fix all issues with current settings
     */
    void fixAllRequested();
    
    /**
     * @brief Request preview of changes
     */
    void previewRequested();
    
    /**
     * @brief Progress update during repair
     */
    void progressUpdated(int percent, const QString& message);

private slots:
    void onAnalyzeClicked();
    void onFixAllClicked();
    void onPreviewClicked();
    void onResetClicked();
    void onOptionsChanged();
    void updateProgress(int percent, const QString& message);

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();
    void updateIssueDisplay();
    void updateButtonStates();
    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    QString formatIssueCount(size_t count, const QString& singular, const QString& plural) const;

    // Viewport for preview
    dc::Viewport* m_viewport;
    
    // Current issues
    MeshIssues m_issues;
    bool m_hasAnalyzed;
    
    // Analysis section
    QPushButton* m_analyzeButton;
    QLabel* m_meshStatusLabel;
    
    // Issues display
    QGroupBox* m_issuesGroup;
    QLabel* m_holeCountLabel;
    QLabel* m_nonManifoldLabel;
    QLabel* m_degenerateFacesLabel;
    QLabel* m_isolatedVerticesLabel;
    QLabel* m_duplicateVerticesLabel;
    QLabel* m_overallStatusLabel;
    
    // Fix options
    QGroupBox* m_optionsGroup;
    QCheckBox* m_fillHolesCheck;
    QSpinBox* m_maxHoleSizeSpinbox;
    QCheckBox* m_removeNonManifoldCheck;
    QCheckBox* m_removeDegenerateFacesCheck;
    QCheckBox* m_removeIsolatedVerticesCheck;
    QCheckBox* m_removeDuplicateVerticesCheck;
    QCheckBox* m_smoothResultCheck;
    QSpinBox* m_smoothIterationsSpinbox;
    
    // Progress
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;
    
    // Results display
    QGroupBox* m_resultsGroup;
    QLabel* m_resultsLabel;
    
    // Action buttons
    QPushButton* m_resetButton;
    QPushButton* m_previewButton;
    QPushButton* m_fixAllButton;
    QPushButton* m_closeButton;
};

#endif // DC_MESHREPAIRWIZARD_H
