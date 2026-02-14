/**
 * @file UndoHistoryDialog.cpp
 * @brief Implementation of UndoHistoryDialog
 */

#include "UndoHistoryDialog.h"

#include <QUndoStack>
#include <QUndoCommand>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFont>

UndoHistoryDialog::UndoHistoryDialog(QWidget* parent)
    : QDialog(parent)
    , m_undoStack(nullptr)
    , m_historyList(nullptr)
    , m_statusLabel(nullptr)
    , m_cleanStateLabel(nullptr)
    , m_clearButton(nullptr)
    , m_closeButton(nullptr)
{
    setupUI();
    applyStylesheet();
}

void UndoHistoryDialog::setupUI()
{
    setWindowTitle(tr("Undo History"));
    setMinimumSize(350, 400);
    resize(400, 500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // Header with description
    QLabel* headerLabel = new QLabel(tr("Click any item to jump to that state:"), this);
    headerLabel->setProperty("heading", true);
    mainLayout->addWidget(headerLabel);
    
    // History list
    m_historyList = new QListWidget(this);
    m_historyList->setAlternatingRowColors(true);
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyList->setToolTip(tr("Click to jump to this point in history"));
    connect(m_historyList, &QListWidget::itemClicked, this, &UndoHistoryDialog::onItemClicked);
    mainLayout->addWidget(m_historyList, 1);
    
    // Status section
    QHBoxLayout* statusLayout = new QHBoxLayout();
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    statusLayout->addWidget(m_statusLabel);
    
    statusLayout->addStretch();
    
    m_cleanStateLabel = new QLabel(this);
    m_cleanStateLabel->setObjectName("cleanStateLabel");
    statusLayout->addWidget(m_cleanStateLabel);
    
    mainLayout->addLayout(statusLayout);
    
    // Button row
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_clearButton = new QPushButton(tr("Clear History"), this);
    m_clearButton->setToolTip(tr("Clear all undo/redo history (cannot be undone)"));
    connect(m_clearButton, &QPushButton::clicked, this, &UndoHistoryDialog::onClearHistoryClicked);
    buttonLayout->addWidget(m_clearButton);
    
    buttonLayout->addStretch();
    
    m_closeButton = new QPushButton(tr("Close"), this);
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void UndoHistoryDialog::applyStylesheet()
{
    // Additional styles specific to this dialog
    setStyleSheet(R"(
        QListWidget {
            border: 1px solid #4a4a4a;
            border-radius: 4px;
        }
        
        QListWidget::item {
            padding: 8px 12px;
            border-bottom: 1px solid #333333;
        }
        
        QListWidget::item:selected {
            background-color: #0078d4;
            color: white;
        }
        
        QListWidget::item:hover:!selected {
            background-color: #383838;
        }
        
        QLabel#statusLabel {
            color: #808080;
            font-size: 12px;
        }
        
        QLabel#cleanStateLabel {
            color: #4caf50;
            font-size: 12px;
        }
    )");
}

void UndoHistoryDialog::setUndoStack(QUndoStack* stack)
{
    // Disconnect from old stack
    if (m_undoStack) {
        disconnect(m_undoStack, nullptr, this, nullptr);
    }
    
    m_undoStack = stack;
    
    if (m_undoStack) {
        // Connect to stack signals for live updates
        connect(m_undoStack, &QUndoStack::indexChanged,
                this, &UndoHistoryDialog::onStackChanged);
        connect(m_undoStack, &QUndoStack::cleanChanged,
                this, &UndoHistoryDialog::onStackChanged);
    }
    
    rebuildList();
    updateStatusLabel();
}

void UndoHistoryDialog::onStackChanged()
{
    rebuildList();
    updateStatusLabel();
}

void UndoHistoryDialog::rebuildList()
{
    m_historyList->clear();
    
    if (!m_undoStack) {
        return;
    }
    
    int count = m_undoStack->count();
    int currentIndex = m_undoStack->index();
    
    // Add initial state marker
    QListWidgetItem* initialItem = new QListWidgetItem(m_historyList);
    initialItem->setData(Qt::UserRole, 0);  // Index 0 = initial state
    if (currentIndex == 0) {
        initialItem->setText(tr("▶ Initial State (current)"));
        QFont font = initialItem->font();
        font.setBold(true);
        initialItem->setFont(font);
    } else {
        initialItem->setText(tr("  Initial State"));
        initialItem->setForeground(QColor("#808080"));
    }
    
    // Add all commands
    for (int i = 0; i < count; ++i) {
        const QUndoCommand* cmd = m_undoStack->command(i);
        QListWidgetItem* item = new QListWidgetItem(m_historyList);
        item->setData(Qt::UserRole, i + 1);  // Index 1-based for commands
        
        QString text = cmd->text();
        if (text.isEmpty()) {
            text = tr("Command %1").arg(i + 1);
        }
        
        if (i + 1 == currentIndex) {
            // This is the current state
            item->setText(tr("▶ %1 (current)").arg(text));
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        } else if (i + 1 < currentIndex) {
            // Already executed (can be undone)
            item->setText(tr("  %1").arg(text));
        } else {
            // In redo stack (has been undone)
            item->setText(tr("  %1 (undone)").arg(text));
            item->setForeground(QColor("#808080"));
        }
    }
    
    // Scroll to current item
    for (int i = 0; i < m_historyList->count(); ++i) {
        QListWidgetItem* item = m_historyList->item(i);
        int itemIndex = item->data(Qt::UserRole).toInt();
        if (itemIndex == currentIndex) {
            m_historyList->scrollToItem(item);
            m_historyList->setCurrentItem(item);
            break;
        }
    }
}

void UndoHistoryDialog::updateStatusLabel()
{
    if (!m_undoStack) {
        m_statusLabel->setText(tr("No history"));
        m_cleanStateLabel->clear();
        m_clearButton->setEnabled(false);
        return;
    }
    
    int count = m_undoStack->count();
    int currentIndex = m_undoStack->index();
    int undoLimit = m_undoStack->undoLimit();
    
    // Status text
    QString statusText;
    if (undoLimit > 0) {
        statusText = tr("%1 / %2 commands").arg(count).arg(undoLimit);
        
        // Warning if near limit
        if (count >= undoLimit) {
            statusText += tr(" (limit reached)");
            m_statusLabel->setStyleSheet("color: #ff9800;");  // Orange warning
        } else if (count >= undoLimit * 0.9) {
            statusText += tr(" (90%% full)");
            m_statusLabel->setStyleSheet("color: #ffeb3b;");  // Yellow
        } else {
            m_statusLabel->setStyleSheet("color: #808080;");
        }
    } else {
        statusText = tr("%1 command(s)").arg(count);
        m_statusLabel->setStyleSheet("color: #808080;");
    }
    
    int redoCount = count - currentIndex;
    if (redoCount > 0) {
        statusText += tr(", %1 redo").arg(redoCount);
    }
    
    m_statusLabel->setText(statusText);
    
    // Clean state indicator
    if (m_undoStack->isClean()) {
        m_cleanStateLabel->setText(tr("✓ Saved"));
        m_cleanStateLabel->setStyleSheet("color: #4caf50;");
    } else {
        m_cleanStateLabel->setText(tr("● Modified"));
        m_cleanStateLabel->setStyleSheet("color: #ff9800;");
    }
    
    // Enable/disable clear button
    m_clearButton->setEnabled(count > 0);
}

void UndoHistoryDialog::onItemClicked(QListWidgetItem* item)
{
    if (!m_undoStack || !item) {
        return;
    }
    
    QVariant data = item->data(Qt::UserRole);
    if (!data.isValid()) {
        return;
    }
    
    int targetIndex = data.toInt();
    
    // Set the index directly - QUndoStack will handle undo/redo
    m_undoStack->setIndex(targetIndex);
    
    // List will be rebuilt via signal
}

void UndoHistoryDialog::onClearHistoryClicked()
{
    if (!m_undoStack) {
        return;
    }
    
    // Confirm with user
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Clear History"),
        tr("Are you sure you want to clear all undo/redo history?\n\n"
           "This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_undoStack->clear();
    }
}
