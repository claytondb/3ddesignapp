#ifndef DC_PREFERENCESDIALOG_H
#define DC_PREFERENCESDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QColor>

class QSettings;
class QPushButton;
class QLabel;

/**
 * @brief Application preferences dialog
 * 
 * Provides user-configurable settings organized into tabs:
 * - General: Theme, language, recent files, auto-save
 * - Viewport: Background color, grid, camera FOV
 * - Units: Display units, decimal precision
 * - Performance: Undo limit, large file threshold
 * - Mouse: Zoom direction, rotation/pan sensitivity
 * 
 * Settings are persisted via QSettings.
 */
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog() override = default;

    // Static access to current preferences (loaded from QSettings)
    static QString theme();
    static QString language();
    static int recentFilesCount();
    static int autoSaveInterval();  // 0 = disabled
    
    static QColor viewportBackgroundColor();
    static bool gridVisibleByDefault();
    static double gridSpacing();
    static double defaultCameraFOV();
    
    static QString displayUnits();  // "mm", "cm", "in"
    static int decimalPrecision();
    
    static int undoHistoryLimit();
    static qint64 largeFileWarningThreshold();  // bytes
    
    static bool invertZoomDirection();
    static double rotationSensitivity();
    static double panSensitivity();

signals:
    void settingsChanged();
    void themeChanged(const QString& theme);

private slots:
    void onApplyClicked();
    void onOkClicked();
    void onCancelClicked();
    void onRestoreDefaultsClicked();
    void onBackgroundColorClicked();

private:
    void setupUI();
    void setupGeneralTab();
    void setupViewportTab();
    void setupUnitsTab();
    void setupPerformanceTab();
    void setupMouseTab();
    void setupConnections();
    void applyStylesheet();
    
    void loadSettings();
    void saveSettings();
    void applySettings();
    void restoreDefaults();
    void updateBackgroundColorButton();
    
    // Main layout
    QTabWidget* m_tabWidget;
    
    // General tab widgets
    QComboBox* m_themeCombo;
    QComboBox* m_languageCombo;
    QSpinBox* m_recentFilesSpinbox;
    QSpinBox* m_autoSaveSpinbox;
    
    // Viewport tab widgets
    QPushButton* m_backgroundColorButton;
    QColor m_backgroundColor;
    QCheckBox* m_gridVisibleCheck;
    QDoubleSpinBox* m_gridSpacingSpinbox;
    QDoubleSpinBox* m_cameraFOVSpinbox;
    
    // Units tab widgets
    QComboBox* m_unitsCombo;
    QSpinBox* m_precisionSpinbox;
    
    // Performance tab widgets
    QSpinBox* m_undoLimitSpinbox;
    QSpinBox* m_largeFileThresholdSpinbox;  // MB
    
    // Mouse tab widgets
    QCheckBox* m_invertZoomCheck;
    QDoubleSpinBox* m_rotationSensitivitySpinbox;
    QDoubleSpinBox* m_panSensitivitySpinbox;
    
    // Button box
    QPushButton* m_restoreDefaultsButton;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    // Track if settings were modified
    bool m_settingsModified;
};

#endif // DC_PREFERENCESDIALOG_H
