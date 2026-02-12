#include "StatusBar.h"
#include <QHBoxLayout>

StatusBar::StatusBar(QWidget *parent)
    : QStatusBar(parent)
{
    setupUI();
}

void StatusBar::setupUI()
{
    // Disable the size grip
    setSizeGripEnabled(false);
    
    // Style the status bar
    setStyleSheet(R"(
        QStatusBar {
            background-color: #2a2a2a;
            color: #b3b3b3;
            border-top: 1px solid #4a4a4a;
            min-height: 24px;
        }
        QStatusBar::item {
            border: none;
        }
        QLabel {
            padding: 2px 8px;
        }
    )");

    // Mode indicator (permanent widget on the left)
    m_modeLabel = new QLabel(tr("Ready"));
    m_modeLabel->setStyleSheet(R"(
        QLabel {
            background-color: #0078d4;
            color: #ffffff;
            border-radius: 3px;
            padding: 2px 8px;
            font-weight: bold;
            font-size: 11px;
        }
    )");
    m_modeLabel->setFixedHeight(18);
    addWidget(m_modeLabel);

    // Message label (normal message area)
    m_messageLabel = new QLabel(tr(""));
    m_messageLabel->setStyleSheet("color: #b3b3b3;");
    addWidget(m_messageLabel, 1); // Stretch factor 1

    // Separator
    addWidget(createSeparator());

    // Selection info (permanent widget)
    m_selectionLabel = new QLabel(tr("No selection"));
    m_selectionLabel->setStyleSheet("color: #808080;");
    m_selectionLabel->setMinimumWidth(120);
    addPermanentWidget(m_selectionLabel);

    // Separator
    addPermanentWidget(createSeparator());

    // Cursor position (permanent widget)
    m_cursorLabel = new QLabel(tr(""));
    m_cursorLabel->setStyleSheet(R"(
        QLabel {
            color: #808080;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
        }
    )");
    m_cursorLabel->setMinimumWidth(180);
    addPermanentWidget(m_cursorLabel);

    // Separator
    addPermanentWidget(createSeparator());

    // FPS counter (permanent widget)
    m_fpsLabel = new QLabel(tr("-- FPS"));
    m_fpsLabel->setStyleSheet(R"(
        QLabel {
            color: #4caf50;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
        }
    )");
    m_fpsLabel->setFixedWidth(60);
    m_fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    addPermanentWidget(m_fpsLabel);
}

QWidget* StatusBar::createSeparator()
{
    QWidget* separator = new QWidget(this);
    separator->setFixedWidth(1);
    separator->setStyleSheet("background-color: #4a4a4a;");
    return separator;
}

void StatusBar::setModeIndicator(const QString& mode)
{
    m_modeLabel->setText(mode);
    
    // Change color based on mode
    QString color = "#0078d4"; // Default blue
    
    if (mode == "Mesh") {
        color = "#0078d4"; // Blue
    } else if (mode == "Sketch") {
        color = "#ff9800"; // Orange
    } else if (mode == "Surface") {
        color = "#4caf50"; // Green
    } else if (mode == "Analysis") {
        color = "#9c27b0"; // Purple
    } else if (mode == "Ready") {
        color = "#808080"; // Gray
    }
    
    m_modeLabel->setStyleSheet(QString(R"(
        QLabel {
            background-color: %1;
            color: #ffffff;
            border-radius: 3px;
            padding: 2px 8px;
            font-weight: bold;
            font-size: 11px;
        }
    )").arg(color));
}

QString StatusBar::modeIndicator() const
{
    return m_modeLabel->text();
}

void StatusBar::setMessage(const QString& message)
{
    m_messageLabel->setText(message);
}

void StatusBar::setSelectionInfo(const QString& info)
{
    if (info.isEmpty()) {
        m_selectionLabel->setText(tr("No selection"));
        m_selectionLabel->setStyleSheet("color: #808080;");
    } else {
        m_selectionLabel->setText(info);
        m_selectionLabel->setStyleSheet("color: #b3b3b3;");
    }
}

void StatusBar::clearSelectionInfo()
{
    setSelectionInfo(QString());
}

void StatusBar::setCursorPosition(double x, double y, double z)
{
    m_cursorLabel->setText(QString("(%1, %2, %3) mm")
        .arg(x, 7, 'f', 1)
        .arg(y, 7, 'f', 1)
        .arg(z, 7, 'f', 1));
}

void StatusBar::clearCursorPosition()
{
    m_cursorLabel->setText("");
}

void StatusBar::setFPS(int fps)
{
    m_fpsLabel->setText(QString("%1 FPS").arg(fps));
    
    // Color code by performance
    QString color;
    if (fps >= 50) {
        color = "#4caf50"; // Green - good
    } else if (fps >= 30) {
        color = "#ff9800"; // Orange - acceptable
    } else {
        color = "#f44336"; // Red - poor
    }
    
    m_fpsLabel->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
        }
    )").arg(color));
}

void StatusBar::setFPSVisible(bool visible)
{
    m_fpsLabel->setVisible(visible);
}
