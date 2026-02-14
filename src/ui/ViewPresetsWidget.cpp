/**
 * @file ViewPresetsWidget.cpp
 * @brief Implementation of viewport view preset toolbar
 */

#include "ViewPresetsWidget.h"
#include "renderer/Viewport.h"
#include "renderer/Camera.h"

#include <QPainter>
#include <QGraphicsDropShadowEffect>

namespace dc {

ViewPresetsWidget::ViewPresetsWidget(Viewport* viewport, QWidget* parent)
    : QWidget(parent)
    , m_viewport(viewport)
{
    setObjectName("ViewPresetsWidget");
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    
    setupUI();
    applyStyle();
    
    // Position in top-right corner of viewport
    // Will be repositioned by viewport on resize
    setFixedHeight(32);
}

void ViewPresetsWidget::setupUI()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(6, 4, 6, 4);
    m_layout->setSpacing(2);
    
    // Create view preset buttons
    m_btnTop = createViewButton("T", "Top View (Numpad 7)", "top");
    m_btnFront = createViewButton("F", "Front View (Numpad 1)", "front");
    m_btnRight = createViewButton("R", "Right View (Numpad 3)", "right");
    m_btnIso = createViewButton("I", "Isometric View (Numpad 0)", "isometric");
    m_btnPersp = createViewButton("P", "Toggle Perspective/Orthographic", "");
    
    m_layout->addWidget(m_btnTop);
    m_layout->addWidget(m_btnFront);
    m_layout->addWidget(m_btnRight);
    m_layout->addWidget(m_btnIso);
    m_layout->addSpacing(4);
    m_layout->addWidget(m_btnPersp);
    
    // Perspective toggle behaves differently
    m_btnPersp->setCheckable(true);
    m_btnPersp->setChecked(true);  // Start in perspective mode
    connect(m_btnPersp, &QToolButton::toggled, this, [this](bool checked) {
        if (m_viewport) {
            m_viewport->camera().toggleProjectionMode();
            m_btnPersp->setText(checked ? "P" : "O");
            m_btnPersp->setToolTip(checked ? "Perspective View (click for Orthographic)" 
                                           : "Orthographic View (click for Perspective)");
            m_viewport->update();
        }
    });
    
    adjustSize();
}

QToolButton* ViewPresetsWidget::createViewButton(const QString& text, const QString& tooltip,
                                                   const QString& viewName)
{
    QToolButton* btn = new QToolButton(this);
    btn->setText(text);
    btn->setToolTip(tooltip);
    btn->setFixedSize(24, 24);
    btn->setCursor(Qt::PointingHandCursor);
    
    if (!viewName.isEmpty()) {
        connect(btn, &QToolButton::clicked, this, [this, viewName]() {
            if (m_viewport) {
                m_viewport->setStandardView(viewName);
                emit viewChanged(viewName);
            }
        });
    }
    
    return btn;
}

void ViewPresetsWidget::applyStyle()
{
    // Semi-transparent dark style matching the app theme
    setStyleSheet(R"(
        ViewPresetsWidget {
            background-color: transparent;
        }
        
        QToolButton {
            background-color: rgba(42, 42, 42, 200);
            color: #b3b3b3;
            border: 1px solid rgba(74, 74, 74, 180);
            border-radius: 4px;
            font-weight: bold;
            font-size: 11px;
        }
        
        QToolButton:hover {
            background-color: rgba(56, 56, 56, 220);
            color: #ffffff;
            border-color: rgba(90, 90, 90, 200);
        }
        
        QToolButton:pressed {
            background-color: rgba(0, 120, 212, 200);
            color: #ffffff;
        }
        
        QToolButton:checked {
            background-color: rgba(0, 120, 212, 180);
            color: #ffffff;
        }
    )");
}

void ViewPresetsWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw semi-transparent rounded background
    QColor bgColor(30, 30, 30, 180);
    painter.setBrush(bgColor);
    painter.setPen(QColor(60, 60, 60, 200));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
}

} // namespace dc
