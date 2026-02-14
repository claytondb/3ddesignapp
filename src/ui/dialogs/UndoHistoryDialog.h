/**
 * @file UndoHistoryDialog.h
 * @brief Dialog for viewing and navigating undo/redo history
 * 
 * Provides a visual list of all commands in the undo/redo stack,
 * allowing users to jump to any point in history with a single click.
 */

#ifndef DC_UNDO_HISTORY_DIALOG_H
#define DC_UNDO_HISTORY_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class QUndoStack;

/**
 * @brief Dialog showing undo/redo history with click-to-jump navigation
 * 
 * Features:
 * - Shows all commands in chronological order
 * - Current state indicator
 * - Click any item to jump to that point in history
 * - Live updates as commands are executed/undone
 * - Stack usage indicator (X/Y commands)
 */
class UndoHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UndoHistoryDialog(QWidget* parent = nullptr);
    ~UndoHistoryDialog() override = default;
    
    /**
     * @brief Connect to a QUndoStack for live updates
     * @param stack The command stack to monitor
     */
    void setUndoStack(QUndoStack* stack);

private slots:
    void onStackChanged();
    void onItemClicked(QListWidgetItem* item);
    void onClearHistoryClicked();

private:
    void setupUI();
    void applyStylesheet();
    void rebuildList();
    void updateStatusLabel();
    
    QUndoStack* m_undoStack;
    
    QListWidget* m_historyList;
    QLabel* m_statusLabel;
    QLabel* m_cleanStateLabel;
    QPushButton* m_clearButton;
    QPushButton* m_closeButton;
};

#endif // DC_UNDO_HISTORY_DIALOG_H
