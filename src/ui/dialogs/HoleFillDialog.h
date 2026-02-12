#ifndef DC_HOLEFILLDIALOG_H
#define DC_HOLEFILLDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QVector>

namespace dc {
class Viewport;
}

/**
 * @brief Dialog for mesh hole filling operations
 * 
 * Provides controls for:
 * - List of detected holes with edge counts
 * - Selection of which holes to fill
 * - Fill method (Flat, Smooth, Curvature-based)
 * - Max hole size filter
 * - Fill All / Fill Selected buttons
 */
class HoleFillDialog : public QDialog
{
    Q_OBJECT

public:
    enum class FillMethod {
        Flat,
        Smooth,
        CurvatureBased
    };
    Q_ENUM(FillMethod)

    struct HoleInfo {
        int id;
        int edgeCount;
        double perimeter;
        double area;
        bool selected;
    };

    explicit HoleFillDialog(QWidget *parent = nullptr);
    ~HoleFillDialog() override = default;

    // Set the viewport for preview updates
    void setViewport(dc::Viewport* viewport);

    // Set detected holes
    void setHoles(const QVector<HoleInfo>& holes);
    void clearHoles();

    // Get fill parameters
    FillMethod fillMethod() const;
    int maxHoleSize() const;
    bool useMaxHoleFilter() const;
    QVector<int> selectedHoleIds() const;

signals:
    void detectHolesRequested();
    void previewRequested(const QVector<int>& holeIds);
    void fillSelectedRequested(const QVector<int>& holeIds);
    void fillAllRequested();
    void holeSelectionChanged(const QVector<int>& holeIds);

private slots:
    void onDetectClicked();
    void onFillMethodChanged(int index);
    void onMaxHoleSizeChanged(int value);
    void onMaxHoleFilterToggled(bool checked);
    void onTableSelectionChanged();
    void onSelectAllClicked();
    void onSelectNoneClicked();
    void onFillSelectedClicked();
    void onFillAllClicked();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();
    void updateHoleTable();
    void updateButtonStates();
    void filterHolesBySize();

    // Viewport for preview
    dc::Viewport* m_viewport;

    // Hole data
    QVector<HoleInfo> m_holes;
    QVector<HoleInfo> m_filteredHoles;

    // Hole list
    QTableWidget* m_holeTable;
    QLabel* m_holeCountLabel;

    // Detection
    QPushButton* m_detectButton;

    // Filter options
    QCheckBox* m_maxHoleFilterCheck;
    QSpinBox* m_maxHoleSizeSpinbox;

    // Fill method
    QComboBox* m_fillMethodCombo;
    QLabel* m_methodDescription;

    // Selection buttons
    QPushButton* m_selectAllButton;
    QPushButton* m_selectNoneButton;

    // Action buttons
    QPushButton* m_fillSelectedButton;
    QPushButton* m_fillAllButton;
    QPushButton* m_closeButton;
};

#endif // DC_HOLEFILLDIALOG_H
