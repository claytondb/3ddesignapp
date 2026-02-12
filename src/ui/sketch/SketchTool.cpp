/**
 * @file SketchTool.cpp
 * @brief Base class implementation for sketch tools
 */

#include "SketchTool.h"
#include "SketchMode.h"
#include "SketchViewport.h"

#include <cmath>

namespace dc {

SketchTool::SketchTool(SketchMode* sketchMode, QObject* parent)
    : QObject(parent)
    , m_sketchMode(sketchMode)
{
}

void SketchTool::activate()
{
    m_active = true;
    m_drawing = false;
    reset();
    emit stateChanged();
}

void SketchTool::deactivate()
{
    if (m_drawing) {
        cancel();
    }
    m_active = false;
    emit stateChanged();
}

void SketchTool::cancel()
{
    m_drawing = false;
    reset();
    emit previewUpdated();
}

void SketchTool::finish()
{
    // Default implementation - subclasses override
    m_drawing = false;
}

void SketchTool::reset()
{
    m_drawing = false;
    m_orthoReference = QPointF();
}

QPointF SketchTool::applyOrthoConstraint(const QPointF& point) const
{
    if (m_orthoReference.isNull()) {
        return point;
    }
    
    float dx = point.x() - m_orthoReference.x();
    float dy = point.y() - m_orthoReference.y();
    
    // Snap to horizontal or vertical based on which is closer
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal constraint
        return QPointF(point.x(), m_orthoReference.y());
    } else {
        // Vertical constraint
        return QPointF(m_orthoReference.x(), point.y());
    }
}

void SketchTool::handleDoubleClick(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Default: finish operation on double-click
    if (m_drawing) {
        finish();
    }
}

void SketchTool::handleKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
            cancel();
            event->accept();
            break;
            
        case Qt::Key_Return:
        case Qt::Key_Enter:
            finish();
            event->accept();
            break;
            
        default:
            break;
    }
}

void SketchTool::handleKeyRelease(QKeyEvent* /*event*/)
{
    // Default: no action
}

SketchPreview SketchTool::getPreview() const
{
    return SketchPreview(); // Empty preview by default
}

QString SketchTool::getStatusText() const
{
    return QString();
}

void SketchTool::addEntity(/* entity data */)
{
    // Add entity to sketch data via SketchMode
    // m_sketchMode->sketchData()->addEntity(entity);
    emit entityCreated();
}

} // namespace dc
