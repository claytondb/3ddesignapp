/**
 * @file SnapSettingsDialog.h
 * @brief Dialog for configuring snap behavior
 */

#ifndef DC_SNAPSETTINGSDIALOG_H
#define DC_SNAPSETTINGSDIALOG_H

#include <QDialog>

class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QGroupBox;

namespace dc3d {
namespace core {
class SnapManager;
}
}

/**
 * @brief Dialog for configuring snap settings
 */
class SnapSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SnapSettingsDialog(dc3d::core::SnapManager* snapManager, 
                                QWidget* parent = nullptr);
    ~SnapSettingsDialog() override = default;

public slots:
    void accept() override;
    void apply();
    void resetToDefaults();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    
    dc3d::core::SnapManager* m_snapManager;
    
    // Grid settings
    QGroupBox* m_gridGroup;
    QCheckBox* m_gridSnapEnabled;
    QDoubleSpinBox* m_gridSize;
    QComboBox* m_gridSubdivisions;
    
    // Object snap settings
    QGroupBox* m_objectGroup;
    QCheckBox* m_objectSnapEnabled;
    QCheckBox* m_snapToVertices;
    QCheckBox* m_snapToEdges;
    QCheckBox* m_snapToEdgeMidpoints;
    QCheckBox* m_snapToFaces;
    QCheckBox* m_snapToFaceCenters;
    QCheckBox* m_snapToOrigins;
    
    // Tolerance settings
    QDoubleSpinBox* m_snapTolerance;
    QDoubleSpinBox* m_worldTolerance;
    
    // Visual settings
    QCheckBox* m_showIndicator;
    QDoubleSpinBox* m_indicatorSize;
};

#endif // DC_SNAPSETTINGSDIALOG_H
