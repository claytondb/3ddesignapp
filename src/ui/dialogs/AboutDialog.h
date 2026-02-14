#ifndef DC_ABOUT_DIALOG_H
#define DC_ABOUT_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>

/**
 * @brief About dialog showing application info, version, and credits
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

    // Application metadata (can be set from CMake-generated version info)
    static QString appName() { return "dc-3ddesignapp"; }
    static QString appVersion() { return "1.0.0"; }
    static QString appDescription() { return "Professional Scan-to-CAD Application"; }

private:
    void setupUI();
    void applyStylesheet();

    QLabel* m_logoLabel;
    QLabel* m_nameLabel;
    QLabel* m_versionLabel;
    QLabel* m_descriptionLabel;
    QLabel* m_creditsLabel;
    QPushButton* m_closeButton;
};

#endif // DC_ABOUT_DIALOG_H
