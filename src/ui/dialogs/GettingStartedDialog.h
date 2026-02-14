#ifndef DC_GETTING_STARTED_DIALOG_H
#define DC_GETTING_STARTED_DIALOG_H

#include <QDialog>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

/**
 * @brief Getting Started / Tutorial dialog
 * 
 * Shows a multi-page tutorial introducing the app's main features.
 * Can be shown on first run or from Help menu.
 */
class GettingStartedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GettingStartedDialog(QWidget *parent = nullptr);
    
    /**
     * @brief Check if this is first run and show dialog if so
     * @return true if dialog was shown
     */
    static bool showOnFirstRun(QWidget *parent);

private slots:
    void onNextClicked();
    void onPrevClicked();
    void onSkipClicked();
    void updateNavigation();

private:
    void setupUI();
    QWidget* createWelcomePage();
    QWidget* createImportPage();
    QWidget* createMeshToolsPage();
    QWidget* createSketchPage();
    QWidget* createExportPage();
    void applyStylesheet();

    QStackedWidget* m_stackedWidget;
    QLabel* m_pageIndicator;
    QPushButton* m_prevButton;
    QPushButton* m_nextButton;
    QPushButton* m_skipButton;
    QCheckBox* m_dontShowAgain;
};

#endif // DC_GETTING_STARTED_DIALOG_H
