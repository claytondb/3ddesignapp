#ifndef DC_STATUSBAR_H
#define DC_STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QWidget>

/**
 * @brief Status bar for dc-3ddesignapp
 * 
 * Shows contextual information:
 * - Mode indicator (Mesh/Sketch/Surface/Analysis)
 * - Selection info (count and type)
 * - Cursor position (3D coordinates)
 * - FPS counter
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

    // Status message
    void setMessage(const QString& message);

    // Selection info
    void setSelectionInfo(const QString& info);
    void clearSelectionInfo();

    // Cursor position
    void setCursorPosition(double x, double y, double z);
    void clearCursorPosition();

    // FPS counter
    void setFPS(int fps);
    void setFPSVisible(bool visible);

private:
    void setupUI();

    // UI Components
    QLabel* m_modeLabel;
    QLabel* m_messageLabel;
    QLabel* m_selectionLabel;
    QLabel* m_cursorLabel;
    QLabel* m_fpsLabel;

    // Separators
    QWidget* createSeparator();
};

#endif // DC_STATUSBAR_H
