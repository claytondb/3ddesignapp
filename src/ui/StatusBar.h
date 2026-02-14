#ifndef DC_STATUSBAR_H
#define DC_STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QWidget>
#include <QProgressBar>
#include <QTimer>

/**
 * @brief Status bar for dc-3ddesignapp
 * 
 * Shows contextual information:
 * - Mode indicator (Mesh/Sketch/Surface/Analysis)
 * - Selection info (count and type)
 * - Cursor position (3D coordinates)
 * - FPS counter
 * - Progress bar for long operations
 * - Temporary feedback messages
 */
class StatusBar : public QStatusBar
{
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);
    ~StatusBar() override = default;

    // Mode indicator
    void setModeIndicator(const QString& mode);
    QString modeIndicator() const;

    // Status message (permanent until changed)
    void setMessage(const QString& message);
    
    // Temporary message that auto-clears after timeout (default 5 seconds)
    void showTemporaryMessage(const QString& message, int timeoutMs = 5000);
    
    // Operation feedback messages with icons
    void showSuccess(const QString& message, int timeoutMs = 5000);
    void showWarning(const QString& message, int timeoutMs = 5000);
    void showError(const QString& message, int timeoutMs = 8000);
    void showInfo(const QString& message, int timeoutMs = 5000);

    // Selection info
    void setSelectionInfo(const QString& info);
    void clearSelectionInfo();

    // Cursor position
    void setCursorPosition(double x, double y, double z);
    void clearCursorPosition();

    // FPS counter
    void setFPS(int fps);
    void setFPSVisible(bool visible);
    
    // Progress bar for long operations
    void showProgress(const QString& operation, int percent);
    void hideProgress();
    void setProgressRange(int min, int max);
    void setProgressValue(int value);
    bool isProgressVisible() const;
    
    // Tool hint - shows what tool is active and how to use it
    void setToolHint(const QString& hint);
    void clearToolHint();

private slots:
    void clearTemporaryMessage();

private:
    void setupUI();

    // UI Components
    QLabel* m_modeLabel;
    QLabel* m_messageLabel;
    QLabel* m_selectionLabel;
    QLabel* m_cursorLabel;
    QLabel* m_fpsLabel;
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;
    QWidget* m_progressWidget;
    QLabel* m_toolHintLabel;
    
    // Message management
    QTimer* m_messageTimer;
    QString m_permanentMessage;

    // Separators
    QWidget* createSeparator();
};

#endif // DC_STATUSBAR_H
