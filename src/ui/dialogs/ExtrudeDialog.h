#ifndef DC_EXTRUDEDIALOG_H
#define DC_EXTRUDEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>

#include <glm/glm.hpp>

namespace dc {
class Viewport;
}

/**
 * @brief Dialog for extrusion operations
 * 
 * Provides controls for:
 * - Direction selection (normal, custom vector)
 * - Distance with spinbox
 * - Draft angle (optional)
 * - Two-sided extrusion
 * - Real-time preview
 */
class ExtrudeDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Direction {
        Normal,          ///< Perpendicular to sketch plane
        CustomVector,    ///< User-specified direction
        ToPoint,         ///< Toward a picked point
        ToSurface        ///< Up to a surface
    };
    Q_ENUM(Direction)

    explicit ExtrudeDialog(QWidget *parent = nullptr);
    ~ExtrudeDialog() override = default;

    // Set the viewport for preview updates
    void setViewport(dc::Viewport* viewport);

    // Set the sketch plane normal (for Normal direction mode)
    void setSketchNormal(const glm::vec3& normal);

    // Get extrusion parameters
    Direction direction() const;
    glm::vec3 customDirection() const;
    float distance() const;
    float draftAngle() const;
    bool isTwoSided() const;
    float twoSidedRatio() const;
    bool capEnds() const;
    bool autoPreview() const;

    // Set initial values
    void setDistance(float distance);
    void setDraftAngle(float angle);
    void setTwoSided(bool twoSided);

signals:
    void previewRequested();
    void applyRequested();
    void directionChanged();
    void parametersChanged();

private slots:
    void onDirectionChanged(int index);
    void onDistanceChanged(double value);
    void onDraftAngleChanged(double value);
    void onTwoSidedToggled(bool checked);
    void onRatioChanged(double value);
    void onCustomDirectionChanged();
    void onPreviewToggled(bool checked);
    void onApplyClicked();
    void onPickDirectionClicked();
    void updatePreview();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();
    void updateDirectionWidgets();
    glm::vec3 parseCustomDirection() const;

    // Viewport for preview
    dc::Viewport* m_viewport = nullptr;
    glm::vec3 m_sketchNormal{0.0f, 0.0f, 1.0f};

    // Direction controls
    QGroupBox* m_directionGroup;
    QComboBox* m_directionCombo;
    QWidget* m_customDirWidget;
    QDoubleSpinBox* m_dirXSpin;
    QDoubleSpinBox* m_dirYSpin;
    QDoubleSpinBox* m_dirZSpin;
    QPushButton* m_pickDirButton;
    QLabel* m_directionPreview;

    // Distance controls
    QGroupBox* m_distanceGroup;
    QDoubleSpinBox* m_distanceSpinbox;
    QCheckBox* m_flipDirection;

    // Draft angle controls
    QGroupBox* m_draftGroup;
    QCheckBox* m_enableDraft = nullptr;  // Initialize to nullptr (unused, m_draftGroup->setCheckable used instead)
    QDoubleSpinBox* m_draftAngleSpinbox;
    QComboBox* m_draftDirectionCombo;

    // Two-sided controls
    QGroupBox* m_twoSidedGroup;
    QCheckBox* m_twoSidedCheck = nullptr;  // Initialize to nullptr (unused, m_twoSidedGroup->setChecked used instead)
    QDoubleSpinBox* m_ratioSpinbox;
    QLabel* m_ratioLabel;

    // Options
    QCheckBox* m_capEndsCheck;
    QCheckBox* m_autoPreviewCheck;

    // Buttons
    QPushButton* m_previewButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
};

#endif // DC_EXTRUDEDIALOG_H
