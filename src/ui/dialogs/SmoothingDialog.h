#ifndef DC_SMOOTHINGDIALOG_H
#define DC_SMOOTHINGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

namespace dc {
class Viewport;
}

/**
 * @brief Dialog for mesh smoothing operations
 * 
 * Provides controls for:
 * - Algorithm selection (Laplacian, Taubin, HC)
 * - Iterations count (1-100)
 * - Strength slider (0.0-1.0)
 * - Preserve boundaries option
 * - Preview with viewport updates
 */
class SmoothingDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Algorithm {
        Laplacian,
        Taubin,
        HC  // Humphrey's Classes
    };
    Q_ENUM(Algorithm)

    explicit SmoothingDialog(QWidget *parent = nullptr);
    ~SmoothingDialog() override = default;

    // Set the viewport for preview updates
    void setViewport(dc::Viewport* viewport);

    // Get smoothing parameters
    Algorithm algorithm() const;
    int iterations() const;
    double strength() const;
    bool preserveBoundaries() const;
    bool autoPreview() const;

    // Algorithm-specific parameters
    double taubinPassBand() const;  // For Taubin algorithm

signals:
    void previewRequested();
    void applyRequested();

private slots:
    void onAlgorithmChanged(int index);
    void onStrengthSliderChanged(int value);
    void onStrengthSpinboxChanged(double value);
    void onIterationsChanged(int value);
    void onPreviewToggled(bool checked);
    void onApplyClicked();
    void updateAlgorithmDescription();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();

    // Viewport for preview
    dc::Viewport* m_viewport;

    // Algorithm selection
    QComboBox* m_algorithmCombo;
    QLabel* m_algorithmDescription;

    // Parameters
    QSpinBox* m_iterationsSpinbox;
    QSlider* m_strengthSlider;
    QDoubleSpinBox* m_strengthSpinbox;

    // Taubin-specific
    QWidget* m_taubinWidget;
    QDoubleSpinBox* m_passBandSpinbox;

    // Options
    QCheckBox* m_preserveBoundaries;
    QCheckBox* m_autoPreviewCheck;

    // Buttons
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
};

#endif // DC_SMOOTHINGDIALOG_H
