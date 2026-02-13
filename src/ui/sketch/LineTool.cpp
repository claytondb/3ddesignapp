/**
 * @file LineTool.cpp
 * @brief Implementation of line drawing tool
 */

#include "LineTool.h"
#include "SketchMode.h"
#include "SketchViewport.h"

#include <QKeyEvent>
#include <cmath>

// M_PI is not standard C++, provide fallback for Windows portability
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dc {

LineTool::LineTool(SketchMode* sketchMode, QObject* parent)
    : SketchTool(sketchMode, parent)
{
}

void LineTool::activate()
{
    SketchTool::activate();
    m_state = LineToolState::Idle;
}

void LineTool::deactivate()
{
    SketchTool::deactivate();
    m_state = LineToolState::Idle;
}

void LineTool::cancel()
{
    m_state = LineToolState::Idle;
    m_drawing = false;
    m_orthoConstrained = false;
    
    emit previewUpdated();
    emit stateChanged();
}

void LineTool::finish()
{
    if (m_state == LineToolState::FirstPoint || m_state == LineToolState::ChainMode) {
        // Exit chain mode, go back to idle
        m_state = LineToolState::Idle;
        m_drawing = false;
    }
    
    emit previewUpdated();
    emit stateChanged();
}

void LineTool::reset()
{
    SketchTool::reset();
    m_state = LineToolState::Idle;
    m_startPoint = QPointF();
    m_currentPoint = QPointF();
    m_length = 0.0f;
    m_angle = 0.0f;
    m_orthoConstrained = false;
}

QPointF LineTool::applyOrthoConstraint(const QPointF& point) const
{
    if (m_state == LineToolState::Idle) {
        return point;
    }
    
    float dx = point.x() - m_startPoint.x();
    float dy = point.y() - m_startPoint.y();
    
    // Calculate angle
    float angle = std::atan2(dy, dx) * 180.0f / M_PI;
    
    // Snap to nearest 45 degree increment
    float snapAngle = std::round(angle / 45.0f) * 45.0f;
    
    // For pure horizontal/vertical, check which is closer
    if (std::abs(dx) > std::abs(dy) * 2) {
        // Horizontal
        return QPointF(point.x(), m_startPoint.y());
    } else if (std::abs(dy) > std::abs(dx) * 2) {
        // Vertical
        return QPointF(m_startPoint.x(), point.y());
    } else {
        // 45 degree
        float length = std::sqrt(dx * dx + dy * dy);
        float radians = snapAngle * M_PI / 180.0f;
        return QPointF(
            m_startPoint.x() + length * std::cos(radians),
            m_startPoint.y() + length * std::sin(radians)
        );
    }
}

void LineTool::handleMousePress(const QPointF& pos, Qt::MouseButtons buttons,
                                 Qt::KeyboardModifiers modifiers)
{
    if (!(buttons & Qt::LeftButton)) {
        return;
    }
    
    m_orthoConstrained = (modifiers & Qt::ShiftModifier);
    
    QPointF snappedPos = pos;
    if (m_orthoConstrained && m_state != LineToolState::Idle) {
        snappedPos = applyOrthoConstraint(pos);
    }
    
    switch (m_state) {
        case LineToolState::Idle:
            // Start new line
            startNewLine(snappedPos);
            break;
            
        case LineToolState::FirstPoint:
        case LineToolState::ChainMode:
            // Complete current line
            m_currentPoint = snappedPos;
            createLine();
            
            // Continue in chain mode if enabled
            if (m_chainEnabled) {
                m_startPoint = m_currentPoint;
                m_state = LineToolState::ChainMode;
                setOrthoReference(m_startPoint);
            } else {
                m_state = LineToolState::Idle;
                m_drawing = false;
            }
            break;
    }
    
    emit previewUpdated();
    emit stateChanged();
}

void LineTool::handleMouseMove(const QPointF& pos, Qt::MouseButtons /*buttons*/,
                                Qt::KeyboardModifiers modifiers)
{
    if (m_state == LineToolState::Idle) {
        return;
    }
    
    m_orthoConstrained = (modifiers & Qt::ShiftModifier);
    
    QPointF snappedPos = pos;
    if (m_orthoConstrained) {
        snappedPos = applyOrthoConstraint(pos);
    }
    
    m_currentPoint = snappedPos;
    
    // Calculate line properties
    float dx = m_currentPoint.x() - m_startPoint.x();
    float dy = m_currentPoint.y() - m_startPoint.y();
    m_length = std::sqrt(dx * dx + dy * dy);
    m_angle = std::atan2(dy, dx) * 180.0f / M_PI;
    
    emit previewUpdated();
}

void LineTool::handleMouseRelease(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Line tool uses click-click, not click-drag
    // So no action on release
}

void LineTool::handleDoubleClick(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Exit chain mode on double-click
    if (m_state == LineToolState::ChainMode) {
        finish();
    }
}

void LineTool::handleKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_C:
            // Toggle construction mode
            m_constructionMode = !m_constructionMode;
            emit stateChanged();
            event->accept();
            break;
            
        case Qt::Key_X:
            // Toggle chain mode
            m_chainEnabled = !m_chainEnabled;
            emit stateChanged();
            event->accept();
            break;
            
        default:
            SketchTool::handleKeyPress(event);
            break;
    }
}

SketchPreview LineTool::getPreview() const
{
    SketchPreview preview;
    
    if (m_state == LineToolState::Idle) {
        return preview;
    }
    
    preview.type = SketchPreview::Type::Line;
    preview.points = {m_startPoint, m_currentPoint};
    preview.valid = (m_length > 0.001f);
    
    // Status text
    QString constraint;
    if (m_orthoConstrained) {
        if (std::abs(m_angle) < 1 || std::abs(m_angle - 180) < 1 || std::abs(m_angle + 180) < 1) {
            constraint = " [Horizontal]";
        } else if (std::abs(std::abs(m_angle) - 90) < 1) {
            constraint = " [Vertical]";
        } else {
            constraint = QString(" [%1°]").arg(m_angle, 0, 'f', 1);
        }
    }
    
    preview.statusText = QString("Length: %1 mm  Angle: %2°%3")
        .arg(m_length, 0, 'f', 2)
        .arg(m_angle, 0, 'f', 1)
        .arg(constraint);
    
    return preview;
}

QString LineTool::getStatusText() const
{
    switch (m_state) {
        case LineToolState::Idle:
            return "Click to place first point";
            
        case LineToolState::FirstPoint:
            return "Click to place second point (Shift for H/V constraint)";
            
        case LineToolState::ChainMode:
            return "Click to continue line, double-click or Esc to finish";
            
        default:
            return QString();
    }
}

void LineTool::createLine()
{
    // Validate line
    if (m_length < 0.001f) {
        return;
    }
    
    // Create line entity
    // In a real implementation, this would create a SketchLineEntity
    // and add it to the sketch data
    
    /*
    auto line = std::make_shared<SketchLineEntity>();
    line->startPoint = m_startPoint;
    line->endPoint = m_currentPoint;
    line->isConstruction = m_constructionMode;
    
    m_sketchMode->sketchData()->addEntity(line);
    */
    
    // TODO: Uncomment entityCreated() when entity creation is implemented
    // Don't emit signal since nothing was actually created yet
    // emit entityCreated();
}

void LineTool::startNewLine(const QPointF& point)
{
    m_startPoint = point;
    m_currentPoint = point;
    m_state = LineToolState::FirstPoint;
    m_drawing = true;
    
    setOrthoReference(m_startPoint);
}

} // namespace dc
