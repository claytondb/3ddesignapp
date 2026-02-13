/**
 * @file SketchToolbox.cpp
 * @brief Implementation of floating sketch toolbox
 */

#include "SketchToolbox.h"
#include "SketchMode.h"

#include <QPainter>
#include <QPushButton>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOption>
#include <QApplication>
#include <QScreen>

namespace dc {

// ============================================================================
// SketchToolbox
// ============================================================================

SketchToolbox::SketchToolbox(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_toolGroup(new QButtonGroup(this))
    , m_selectedTool(SketchToolType::None)
{
    // Window flags for floating toolbox
    // Note: Removed Qt::WindowStaysOnTopHint to prevent toolbox staying on top of all windows
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // Explicitly set exclusive mode for clarity (tools should be mutually exclusive)
    m_toolGroup->setExclusive(true);
    
    setupUI();
    applyStylesheet();
    
    // Connect button group
    connect(m_toolGroup, &QButtonGroup::idClicked, this, &SketchToolbox::onToolButtonClicked);
    
    // Position at default location
    if (parent) {
        move(parent->x() + 20, parent->y() + 100);
    } else if (QApplication::primaryScreen()) {
        QRect screenGeom = QApplication::primaryScreen()->availableGeometry();
        move(screenGeom.left() + 20, screenGeom.top() + 100);
    }
}

void SketchToolbox::setSelectedTool(SketchToolType toolType)
{
    m_selectedTool = toolType;
    
    auto it = m_toolButtons.find(toolType);
    if (it != m_toolButtons.end()) {
        it->second->setChecked(true);
    }
}

void SketchToolbox::setToolEnabled(SketchToolType toolType, bool enabled)
{
    auto it = m_toolButtons.find(toolType);
    if (it != m_toolButtons.end()) {
        it->second->setEnabled(enabled);
    }
}

void SketchToolbox::setCategoryVisible(const QString& category, bool visible)
{
    auto it = m_categories.find(category);
    if (it != m_categories.end()) {
        it->second->setVisible(visible);
    }
    adjustSize();
}

void SketchToolbox::setToolTooltip(SketchToolType toolType, const QString& tooltip)
{
    auto it = m_toolButtons.find(toolType);
    if (it != m_toolButtons.end()) {
        it->second->setToolTip(tooltip);
    }
}

void SketchToolbox::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw rounded rectangle background
    QColor bgColor(45, 45, 48, 240);
    QColor borderColor(62, 62, 66);
    
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
}

void SketchToolbox::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Check if click is in title area (top 24px)
        if (event->pos().y() < 24) {
            m_dragging = true;
            m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void SketchToolbox::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void SketchToolbox::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void SketchToolbox::onToolButtonClicked(int id)
{
    SketchToolType toolType = static_cast<SketchToolType>(id);
    m_selectedTool = toolType;
    emit toolSelected(toolType);
}

void SketchToolbox::onExitButtonClicked()
{
    emit exitRequested();
}

void SketchToolbox::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);
    
    // Title bar
    QLabel* titleLabel = new QLabel("Sketch Tools");
    titleLabel->setStyleSheet("color: #CCCCCC; font-weight: bold; padding: 4px;");
    titleLabel->setCursor(Qt::SizeAllCursor);
    m_mainLayout->addWidget(titleLabel);
    
    m_mainLayout->addWidget(createSeparator());
    
    // Setup tool categories
    setupDrawTools();
    m_mainLayout->addWidget(createSeparator());
    setupModifyTools();
    m_mainLayout->addWidget(createSeparator());
    setupConstraintTools();
    m_mainLayout->addWidget(createSeparator());
    setupDimensionTools();
    
    // Exit button
    m_mainLayout->addWidget(createSeparator());
    
    QPushButton* exitButton = new QPushButton("Exit Sketch");
    exitButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #3E3E42;"
        "  color: #CCCCCC;"
        "  border: 1px solid #555555;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #505054;"
        "  border-color: #007ACC;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #007ACC;"
        "}"
    );
    connect(exitButton, &QPushButton::clicked, this, &SketchToolbox::onExitButtonClicked);
    m_mainLayout->addWidget(exitButton);
    
    m_mainLayout->addStretch();
    
    // Set minimum size
    setMinimumWidth(160);
    adjustSize();
}

void SketchToolbox::setupDrawTools()
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    
    layout->addWidget(createSectionLabel("Draw"));
    
    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(2);
    
    // Row 1: Line, Circle, Arc
    grid->addWidget(createToolButton("line", "Line (L)", SketchToolType::Line), 0, 0);
    grid->addWidget(createToolButton("circle", "Circle (O)", SketchToolType::Circle), 0, 1);
    grid->addWidget(createToolButton("arc", "Arc (A)", SketchToolType::Arc), 0, 2);
    
    // Row 2: Spline, Rectangle, Point
    grid->addWidget(createToolButton("spline", "Spline (S)", SketchToolType::Spline), 1, 0);
    grid->addWidget(createToolButton("rectangle", "Rectangle (R)", SketchToolType::Rectangle), 1, 1);
    grid->addWidget(createToolButton("point", "Point", SketchToolType::Point), 1, 2);
    
    layout->addLayout(grid);
    
    m_mainLayout->addWidget(container);
    m_categories["Draw"] = container;
}

void SketchToolbox::setupModifyTools()
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    
    layout->addWidget(createSectionLabel("Modify"));
    
    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(2);
    
    // Row 1: Trim, Extend
    grid->addWidget(createToolButton("trim", "Trim (T)", SketchToolType::Trim), 0, 0);
    grid->addWidget(createToolButton("extend", "Extend", SketchToolType::Extend), 0, 1);
    
    // Row 2: Offset, Mirror
    grid->addWidget(createToolButton("offset", "Offset", SketchToolType::Offset), 1, 0);
    grid->addWidget(createToolButton("mirror", "Mirror", SketchToolType::Mirror), 1, 1);
    
    layout->addLayout(grid);
    
    m_mainLayout->addWidget(container);
    m_categories["Modify"] = container;
}

void SketchToolbox::setupConstraintTools()
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    
    layout->addWidget(createSectionLabel("Constraints"));
    
    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(2);
    
    // Row 1: Horizontal, Vertical, Coincident
    grid->addWidget(createToolButton("horizontal", "Horizontal (H)", SketchToolType::ConstraintHorizontal), 0, 0);
    grid->addWidget(createToolButton("vertical", "Vertical (V)", SketchToolType::ConstraintVertical), 0, 1);
    grid->addWidget(createToolButton("coincident", "Coincident", SketchToolType::ConstraintCoincident), 0, 2);
    
    // Row 2: Parallel, Perpendicular, Tangent
    grid->addWidget(createToolButton("parallel", "Parallel", SketchToolType::ConstraintParallel), 1, 0);
    grid->addWidget(createToolButton("perpendicular", "Perpendicular", SketchToolType::ConstraintPerpendicular), 1, 1);
    grid->addWidget(createToolButton("tangent", "Tangent", SketchToolType::ConstraintTangent), 1, 2);
    
    // Row 3: Equal, Fix
    grid->addWidget(createToolButton("equal", "Equal", SketchToolType::ConstraintEqual), 2, 0);
    grid->addWidget(createToolButton("fix", "Fix", SketchToolType::ConstraintFix), 2, 1);
    
    layout->addLayout(grid);
    
    m_mainLayout->addWidget(container);
    m_categories["Constraints"] = container;
}

void SketchToolbox::setupDimensionTools()
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    
    layout->addWidget(createSectionLabel("Dimension"));
    
    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->setSpacing(2);
    
    hbox->addWidget(createToolButton("dimension", "Dimension (D)", SketchToolType::Dimension));
    hbox->addStretch();
    
    layout->addLayout(hbox);
    
    m_mainLayout->addWidget(container);
    m_categories["Dimension"] = container;
}

void SketchToolbox::applyStylesheet()
{
    // Main stylesheet is handled in paintEvent for custom background
    setStyleSheet(
        "QLabel {"
        "  color: #AAAAAA;"
        "  font-size: 11px;"
        "}"
    );
}

QToolButton* SketchToolbox::createToolButton(const QString& iconName, const QString& tooltip, 
                                              SketchToolType toolType)
{
    SketchToolButton* button = new SketchToolButton(this);
    button->setToolType(toolType);
    button->setToolTip(tooltip);
    button->setCheckable(true);
    button->setFixedSize(40, 40);
    button->setIconSize(QSize(24, 24));
    
    // Try to load icon, fall back to text
    QString iconPath = QString(":/icons/sketch/%1.png").arg(iconName);
    QIcon icon(iconPath);
    if (!icon.isNull() && !icon.pixmap(24, 24).isNull()) {
        button->setIcon(icon);
    } else {
        // Use first letter as fallback
        button->setText(iconName.left(1).toUpper());
    }
    
    // Add to button group
    m_toolGroup->addButton(button, static_cast<int>(toolType));
    m_toolButtons[toolType] = button;
    
    return button;
}

QWidget* SketchToolbox::createSeparator()
{
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #555555; max-height: 1px;");
    return separator;
}

QLabel* SketchToolbox::createSectionLabel(const QString& text)
{
    QLabel* label = new QLabel(text);
    label->setStyleSheet(
        "color: #888888;"
        "font-size: 10px;"
        "font-weight: bold;"
        "text-transform: uppercase;"
        "padding: 2px 0px;"
    );
    return label;
}

// ============================================================================
// SketchToolButton
// ============================================================================

SketchToolButton::SketchToolButton(QWidget* parent)
    : QToolButton(parent)
    , m_toolType(SketchToolType::None)
{
    setStyleSheet(
        "QToolButton {"
        "  background-color: #3E3E42;"
        "  border: 1px solid #555555;"
        "  border-radius: 4px;"
        "  color: #CCCCCC;"
        "  font-weight: bold;"
        "}"
        "QToolButton:hover {"
        "  background-color: #505054;"
        "  border-color: #007ACC;"
        "}"
        "QToolButton:checked {"
        "  background-color: #007ACC;"
        "  border-color: #007ACC;"
        "}"
        "QToolButton:pressed {"
        "  background-color: #005A9E;"
        "}"
        "QToolButton:disabled {"
        "  background-color: #2D2D30;"
        "  color: #555555;"
        "  border-color: #3E3E42;"
        "}"
    );
}

void SketchToolButton::paintEvent(QPaintEvent* event)
{
    // Let the stylesheet handle all styling including hover effects
    // The :hover pseudo-class in the stylesheet already provides visual feedback
    QToolButton::paintEvent(event);
}

void SketchToolButton::enterEvent(QEnterEvent* /*event*/)
{
    m_hovered = true;
    update();
}

void SketchToolButton::leaveEvent(QEvent* /*event*/)
{
    m_hovered = false;
    update();
}

} // namespace dc
