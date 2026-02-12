#ifndef DC_CLIPPINGBOXDIALOG_H
#define DC_CLIPPINGBOXDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

namespace dc {
class Viewport;
}

/**
 * @brief Dialog for mesh clipping box operations
 * 
 * Provides controls for:
 * - Min/max XYZ spinboxes
 * - Visual box in viewport
 * - Invert selection option
 * - Apply (delete outside/inside) button
 */
class ClippingBoxDialog : public QDialog
{
    Q_OBJECT

public:
    struct BoundingBox {
        double minX, minY, minZ;
        double maxX, maxY, maxZ;
    };

    explicit ClippingBoxDialog(QWidget *parent = nullptr);
    ~ClippingBoxDialog() override = default;

    // Set the viewport for visual box display
    void setViewport(dc::Viewport* viewport);

    // Set mesh bounding box (for default values)
    void setMeshBounds(const BoundingBox& bounds);

    // Get clipping box parameters
    BoundingBox clippingBox() const;
    bool invertSelection() const;
    bool showPreview() const;

signals:
    void boxChanged(const BoundingBox& box);
    void previewRequested();
    void applyRequested(bool deleteOutside);
    void resetRequested();

private slots:
    void onMinXChanged(double value);
    void onMinYChanged(double value);
    void onMinZChanged(double value);
    void onMaxXChanged(double value);
    void onMaxYChanged(double value);
    void onMaxZChanged(double value);
    void onInvertToggled(bool checked);
    void onPreviewToggled(bool checked);
    void onResetClicked();
    void onApplyClicked();
    void onDeleteInsideClicked();

private:
    void setupUI();
    void setupConnections();
    void applyStylesheet();
    void emitBoxChanged();
    void validateBounds();

    // Viewport for preview
    dc::Viewport* m_viewport;

    // Mesh bounds (for reset)
    BoundingBox m_meshBounds;

    // Min controls
    QDoubleSpinBox* m_minXSpinbox;
    QDoubleSpinBox* m_minYSpinbox;
    QDoubleSpinBox* m_minZSpinbox;

    // Max controls
    QDoubleSpinBox* m_maxXSpinbox;
    QDoubleSpinBox* m_maxYSpinbox;
    QDoubleSpinBox* m_maxZSpinbox;

    // Info labels
    QLabel* m_sizeLabel;

    // Options
    QCheckBox* m_invertCheck;
    QCheckBox* m_previewCheck;

    // Buttons
    QPushButton* m_resetButton;
    QPushButton* m_deleteInsideButton;
    QPushButton* m_applyButton;  // Delete outside
    QPushButton* m_closeButton;
};

#endif // DC_CLIPPINGBOXDIALOG_H
