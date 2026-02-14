#ifndef DC_EXPORTFORMATDIALOG_H
#define DC_EXPORTFORMATDIALOG_H

#include <QDialog>
#include <QString>

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QStackedWidget;
class QRadioButton;

/**
 * @brief Qt dialog for selecting export format and options
 * 
 * Provides a user-friendly interface to:
 * - Choose export format (STL, OBJ, PLY, STEP, IGES)
 * - Configure format-specific options
 * - Set quality and unit preferences
 */
class ExportFormatDialog : public QDialog
{
    Q_OBJECT

public:
    enum ExportFormat {
        STL_Binary,
        STL_ASCII,
        OBJ,
        PLY,
        STEP,
        IGES
    };
    
    enum QualityPreset {
        Draft,
        Standard,
        Fine,
        Custom
    };
    
    struct ExportSettings {
        ExportFormat format = STL_Binary;
        QString filePath;
        
        // STL options
        bool stlBinary = true;
        
        // OBJ options
        bool objIncludeNormals = true;
        bool objIncludeUVs = false;
        bool objIncludeMaterials = false;
        
        // PLY options
        bool plyBinary = true;
        bool plyIncludeColors = true;
        
        // Quality settings
        QualityPreset quality = Standard;
        double chordTolerance = 0.1;
        double angleTolerance = 15.0;
        
        // General
        bool exportSelected = false;
        double scaleFactor = 1.0;
    };

    explicit ExportFormatDialog(QWidget *parent = nullptr);
    ~ExportFormatDialog() override;

    /**
     * @brief Get the configured export settings
     */
    ExportSettings settings() const;
    
    /**
     * @brief Set initial file path
     */
    void setFilePath(const QString& path);
    
    /**
     * @brief Get file extension for current format
     */
    QString currentExtension() const;

signals:
    void formatChanged(ExportFormat format);

private slots:
    void onFormatChanged(int index);
    void onQualityChanged(int index);
    void onBrowseClicked();
    void updateFileExtension();
    void validateInput();

private:
    void setupUI();
    void setupFormatGroup();
    void setupOptionsStack();
    void setupQualityGroup();
    void setupGeneralGroup();
    void setupButtons();
    void applyStylesheet();
    
    // Format selection
    QComboBox* m_formatCombo;
    QLabel* m_formatDescription;
    
    // Format-specific options (stacked)
    QStackedWidget* m_optionsStack;
    
    // STL options
    QRadioButton* m_stlBinaryRadio;
    QRadioButton* m_stlAsciiRadio;
    
    // OBJ options
    QCheckBox* m_objNormalsCheck;
    QCheckBox* m_objUVsCheck;
    QCheckBox* m_objMaterialsCheck;
    
    // PLY options
    QRadioButton* m_plyBinaryRadio;
    QRadioButton* m_plyAsciiRadio;
    QCheckBox* m_plyColorsCheck;
    
    // Quality settings
    QComboBox* m_qualityCombo;
    QGroupBox* m_customQualityGroup;
    QDoubleSpinBox* m_chordSpin;
    QDoubleSpinBox* m_angleSpin;
    
    // General settings
    QLineEdit* m_filePathEdit;
    QCheckBox* m_exportSelectedCheck;
    QDoubleSpinBox* m_scaleSpin;
    
    // Format descriptions
    static QString formatDescription(ExportFormat format);
};

#endif // DC_EXPORTFORMATDIALOG_H
