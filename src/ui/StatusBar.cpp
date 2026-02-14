#include "StatusBar.h"
#include <QHBoxLayout>

StatusBar::StatusBar(QWidget *parent)
    : QStatusBar(parent)
    , m_messageTimer(new QTimer(this))
{
    m_messageTimer->setSingleShot(true);
    connect(m_messageTimer, &QTimer::timeout, this, &StatusBar::clearTemporaryMessage);
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
            min-height: 26px;
        }
        QStatusBar::item {
            border: none;
        }
        QLabel {
            padding: 2px 8px;
        }
        QProgressBar {
            background-color: #333333;
            border: none;
            border-radius: 3px;
            height: 14px;
            text-align: center;
            font-size: 10px;
            color: #ffffff;
        }
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 3px;
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
    
    // Tool hint label (shows current tool usage hints)
    m_toolHintLabel = new QLabel();
    m_toolHintLabel->setStyleSheet("color: #6a9ed9; font-style: italic;");
    m_toolHintLabel->setVisible(false);
    addWidget(m_toolHintLabel);
    
    // Progress widget (hidden by default)
    m_progressWidget = new QWidget(this);
    QHBoxLayout* progressLayout = new QHBoxLayout(m_progressWidget);
    progressLayout->setContentsMargins(4, 0, 4, 0);
    progressLayout->setSpacing(6);
    
    m_progressLabel = new QLabel();
    m_progressLabel->setStyleSheet("color: #b3b3b3; font-size: 11px;");
    progressLayout->addWidget(m_progressLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setFixedWidth(120);
    m_progressBar->setTextVisible(true);
    progressLayout->addWidget(m_progressBar);
    
    m_progressWidget->setVisible(false);
    addWidget(m_progressWidget);

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
    m_permanentMessage = message;
    m_messageLabel->setText(message);
    m_messageLabel->setStyleSheet("color: #b3b3b3;");
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

void StatusBar::showTemporaryMessage(const QString& message, int timeoutMs)
{
    m_messageLabel->setText(message);
    m_messageLabel->setStyleSheet("color: #b3b3b3;");
    m_messageTimer->start(timeoutMs);
}

void StatusBar::showSuccess(const QString& message, int timeoutMs)
{
    m_messageLabel->setText(QString("✓ %1").arg(message));
    m_messageLabel->setStyleSheet("color: #4caf50;");  // Green
    m_messageTimer->start(timeoutMs);
}

void StatusBar::showWarning(const QString& message, int timeoutMs)
{
    m_messageLabel->setText(QString("⚠ %1").arg(message));
    m_messageLabel->setStyleSheet("color: #ff9800;");  // Orange
    m_messageTimer->start(timeoutMs);
}

void StatusBar::showError(const QString& message, int timeoutMs)
{
    m_messageLabel->setText(QString("✗ %1").arg(message));
    m_messageLabel->setStyleSheet("color: #f44336;");  // Red
    m_messageTimer->start(timeoutMs);
}

void StatusBar::showInfo(const QString& message, int timeoutMs)
{
    m_messageLabel->setText(QString("ℹ %1").arg(message));
    m_messageLabel->setStyleSheet("color: #2196f3;");  // Blue
    m_messageTimer->start(timeoutMs);
}

void StatusBar::clearTemporaryMessage()
{
    m_messageLabel->setText(m_permanentMessage);
    m_messageLabel->setStyleSheet("color: #b3b3b3;");
}

void StatusBar::showProgress(const QString& operation, int percent)
{
    m_progressLabel->setText(operation);
    m_progressBar->setValue(percent);
    m_progressWidget->setVisible(true);
}

void StatusBar::hideProgress()
{
    m_progressWidget->setVisible(false);
    m_progressBar->setValue(0);
}

void StatusBar::setProgressRange(int min, int max)
{
    m_progressBar->setRange(min, max);
}

void StatusBar::setProgressValue(int value)
{
    m_progressBar->setValue(value);
}

bool StatusBar::isProgressVisible() const
{
    return m_progressWidget->isVisible();
}

void StatusBar::setToolHint(const QString& hint)
{
    m_toolHintLabel->setText(hint);
    m_toolHintLabel->setVisible(!hint.isEmpty());
}

void StatusBar::clearToolHint()
{
    m_toolHintLabel->clear();
    m_toolHintLabel->setVisible(false);
}
