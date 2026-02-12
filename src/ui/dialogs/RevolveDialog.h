#ifndef DC_REVOLVEDIALOG_H
#define DC_REVOLVEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>

#include <glm/glm.hpp>

namespace dc {
class Viewport;
}

/**
 * @brief Dialog for revolution operations
 * 
 * Provides controls for:
 * - Axis selection (pick line, edge, or coordinate axis)
 * - Revolution angle (0-360°)
 * - Partial or full revolution
 * - Cap ends for solid
 * - Real-time preview
 */
class RevolveDialog : public QDialog
{
    Q_OBJECT

public:
    enum class AxisType {
        XAxis,           ///< Global X axis
        YAxis,           ///< Global Y axis
        ZAxis,           ///< Global Z axis
        SketchLine,      ///< Line from the sketch
        PickedEdge,      ///< Edge picked from model
        CustomAxis       ///< User-defined axis
    };
    Q_ENUM(AxisType)

    explicit RevolveDialog(QWidget *parent = nullptr);
    ~RevolveDialog() override = default;

    // Set the viewport for preview updates and picking
    void setViewport(dc::Viewport* viewport);

    // Get revolution parameters
    AxisType axisType() const;
    glm::vec3 axisOrigin() const;
    glm::vec3 axisDirection() const;
    float startAngle() const;
    float endAngle() const;
    float sweepAngle() const;
    bool isFullRevolution() const;
    bool capEnds() const;
    int segments() const;
    bool autoPreview() const;

    // Set initial values
    void setAngle(float angle);
    void setAxis(const glm::vec3& origin, const glm::vec3& direction);
    void setSegments(int segments);

signals:
    void previewRequested();
    void applyRequested();
    void axisChanged();
    void parametersChanged();
    void pickAxisRequested();

private slots:
    void onAxisTypeChanged(int index);
    void onAngleSliderChanged(int value);
    void onAngleSpinboxChanged(double value);
    void onFullRevolutionToggled(bool checked);
    void onCustomAxisChanged();
    void onPickAxisClicked();
    void onPreviewToggled(bool checked);
    void onApplyClicked();
    void updatePreview();
    void updateAngleWidgets();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();
    glm::vec3 getAxisDirectionForType(AxisType type) const;

    // Viewport for preview and picking
    dc::Viewport* m_viewport = nullptr;
    
    // Custom axis (set by picking or user input)
    glm::vec3 m_customAxisOrigin{0.0f};
    glm::vec3 m_customAxisDirection{0.0f, 1.0f, 0.0f};
    bool m_hasCustomAxis = false;

    // Axis controls
    QGroupBox* m_axisGroup;
    QComboBox* m_axisCombo;
    QWidget* m_customAxisWidget;
    QLabel* m_axisOriginLabel;
    QDoubleSpinBox* m_originXSpin;
    QDoubleSpinBox* m_originYSpin;
    QDoubleSpinBox* m_originZSpin;
    QLabel* m_axisDirLabel;
    QDoubleSpinBox* m_dirXSpin;
    QDoubleSpinBox* m_dirYSpin;
    QDoubleSpinBox* m_dirZSpin;
    QPushButton* m_pickAxisButton;
    QLabel* m_axisPreview;

    // Angle controls
    QGroupBox* m_angleGroup;
    QCheckBox* m_fullRevolutionCheck;
    QWidget* m_partialAngleWidget;
    QDoubleSpinBox* m_startAngleSpinbox;
    QDoubleSpinBox* m_endAngleSpinbox;
    QSlider* m_angleSlider;
    QLabel* m_anglePreview;

    // Quality controls
    QGroupBox* m_qualityGroup;
    QSpinBox* m_segmentsSpinbox;
    QLabel* m_segmentsPreview;

    // Options
    QCheckBox* m_capEndsCheck;
    QCheckBox* m_autoPreviewCheck;

    // Buttons
    QPushButton* m_previewButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
};

#endif // DC_REVOLVEDIALOG_H
