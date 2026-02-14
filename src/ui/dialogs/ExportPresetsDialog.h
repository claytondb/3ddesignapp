#ifndef DC_EXPORTPRESETSDIALOG_H
#define DC_EXPORTPRESETSDIALOG_H

#include <QDialog>
#include "ExportPresetManager.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;
class QGroupBox;

/**
 * @brief Dialog for managing export presets
 * 
 * Allows users to:
 * - View all presets (built-in and user-defined)
 * - Create new presets from current settings
 * - Rename and delete user presets
 * - Set the default preset for Quick Export
 */
class ExportPresetsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportPresetsDialog(QWidget *parent = nullptr);
    ~ExportPresetsDialog() override;

signals:
    void presetSelected(const QString& name);
    void createPresetRequested();

private slots:
    void onPresetSelectionChanged();
    void onSetDefaultClicked();
    void onRenameClicked();
    void onDeleteClicked();
    void onPresetDoubleClicked(QListWidgetItem* item);
    void refreshPresetList();

private:
    void setupUI();
    void applyStylesheet();
    void updatePresetDetails();
    void updateButtonStates();
    QString formatName(int format) const;
    QString qualityName(int quality) const;
    
    QListWidget* m_presetList;
    QGroupBox* m_detailsGroup;
    QLabel* m_descriptionLabel;
    QLabel* m_formatLabel;
    QLabel* m_qualityLabel;
    QLabel* m_settingsLabel;
    QLabel* m_defaultIndicator;
    
    QPushButton* m_setDefaultBtn;
    QPushButton* m_renameBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_closeBtn;
};

#endif // DC_EXPORTPRESETSDIALOG_H
