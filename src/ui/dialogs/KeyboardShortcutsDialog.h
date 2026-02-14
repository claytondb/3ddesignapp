#ifndef DC_KEYBOARD_SHORTCUTS_DIALOG_H
#define DC_KEYBOARD_SHORTCUTS_DIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>

/**
 * @brief Dialog showing all keyboard shortcuts in the application
 * 
 * Displays shortcuts organized by category with search functionality.
 */
class KeyboardShortcutsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyboardShortcutsDialog(QWidget *parent = nullptr);

private slots:
    void onSearchTextChanged(const QString& text);

private:
    void setupUI();
    void populateShortcuts();
    void addCategory(const QString& category, const QList<QPair<QString, QString>>& shortcuts);
    void applyStylesheet();

    QLineEdit* m_searchEdit;
    QTreeWidget* m_treeWidget;
    QPushButton* m_closeButton;
};

#endif // DC_KEYBOARD_SHORTCUTS_DIALOG_H
