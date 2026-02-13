/**
 * @file ArcTool.cpp
 * @brief Implementation of arc drawing tool
 */

#include "ArcTool.h"
#include "SketchMode.h"
#include "SketchViewport.h"

#include <QKeyEvent>
#include <cmath>

// M_PI is not standard C++, provide fallback for Windows portability
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dc {

ArcTool::ArcTool(SketchMode* sketchMode, QObject* parent)
    : SketchTool(sketchMode, parent)
{
}

void ArcTool::activate()
{
    SketchTool::activate();
    m_state = ArcToolState::Idle;
}

void ArcTool::deactivate()
{
    SketchTool::deactivate();
    m_state = ArcToolState::Idle;
}

void ArcTool::cancel()
{
    m_state = ArcToolState::Idle;
    m_drawing = false;
    m_arcValid = false;
    
    emit previewUpdated();
    emit stateChanged();
}

void ArcTool::finish()
{
    if (m_arcValid) {
        createArc();
    }
    
    m_state = ArcToolState::Idle;
    m_drawing = false;
    m_arcValid = false;
    
    emit previewUpdated();
    emit stateChanged();
}

void ArcTool::reset()
{
    SketchTool::reset();
    m_state = ArcToolState::Idle;
    m_startPoint = QPointF();
    m_endPoint = QPointF();
    m_arcPoint = QPointF();
    m_centerPoint = QPointF();
    m_currentPoint = QPointF();
    m_radius = 0.0f;
    m_startAngle = 0.0f;
    m_endAngle = 0.0f;
    m_sweepAngle = 0.0f;
    m_arcValid = false;
    m_calculatedRadius = 0.0f;
}

void ArcTool::setArcMode(ArcMode mode)
{
    if (m_drawing) {
        cancel();
    }
    m_arcMode = mode;
    emit stateChanged();
}

void ArcTool::handleMousePress(const QPointF& pos, Qt::MouseButtons buttons,
                                Qt::KeyboardModifiers /*modifiers*/)
{
    if (!(buttons & Qt::LeftButton)) {
        return;
    }
    
    if (m_arcMode == ArcMode::ThreePoint) {
        switch (m_state) {
            case ArcToolState::Idle:
                m_startPoint = pos;
                m_state = ArcToolState::StartPoint;
                m_drawing = true;
                break;
                
            case ArcToolState::StartPoint:
                m_endPoint = pos;
                m_state = ArcToolState::EndPoint;
                break;
                
            case ArcToolState::EndPoint:
                m_arcPoint = pos;
                m_state = ArcToolState::ArcPoint;
                calculateArcFromThreePoints();
                if (m_arcValid) {
                    createArc();
                }
                m_state = ArcToolState::Idle;
                m_drawing = false;
                break;
                
            default:
                break;
        }
    } else if (m_arcMode == ArcMode::CenterRadius) {
        switch (m_state) {
            case ArcToolState::Idle:
                m_centerPoint = pos;
                m_state = ArcToolState::Center;
                m_drawing = true;
                break;
                
            case ArcToolState::Center:
                m_startPoint = pos;
                m_radius = std::sqrt(
                    std::pow(pos.x() - m_centerPoint.x(), 2) +
                    std::pow(pos.y() - m_centerPoint.y(), 2)
                );
                m_startAngle = std::atan2(
                    pos.y() - m_centerPoint.y(),
                    pos.x() - m_centerPoint.x()
                );
                m_state = ArcToolState::Radius;
                break;
                
            case ArcToolState::Radius:
                m_endAngle = std::atan2(
                    pos.y() - m_centerPoint.y(),
                    pos.x() - m_centerPoint.x()
                );
                calculateArcFromCenterRadius();
                if (m_arcValid) {
                    createArc();
                }
                m_state = ArcToolState::Idle;
                m_drawing = false;
                break;
                
            default:
                break;
        }
    }
    
    emit previewUpdated();
    emit stateChanged();
}

void ArcTool::handleMouseMove(const QPointF& pos, Qt::MouseButtons /*buttons*/,
                               Qt::KeyboardModifiers /*modifiers*/)
{
    m_currentPoint = pos;
    
    if (m_arcMode == ArcMode::ThreePoint) {
        if (m_state == ArcToolState::EndPoint) {
            // Preview arc with current position as arc point
            m_arcPoint = pos;
            calculateArcFromThreePoints();
        }
    } else if (m_arcMode == ArcMode::CenterRadius) {
        if (m_state == ArcToolState::Center) {
            // Preview radius
            m_radius = std::sqrt(
                std::pow(pos.x() - m_centerPoint.x(), 2) +
                std::pow(pos.y() - m_centerPoint.y(), 2)
            );
        } else if (m_state == ArcToolState::Radius) {
            // Preview end angle
            m_endAngle = std::atan2(
                pos.y() - m_centerPoint.y(),
                pos.x() - m_centerPoint.x()
            );
            calculateArcFromCenterRadius();
        }
    }
    
    emit previewUpdated();
}

void ArcTool::handleMouseRelease(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Arc tool uses click-click
}

void ArcTool::handleKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_M:
            // Toggle arc mode
            if (m_arcMode == ArcMode::ThreePoint) {
                setArcMode(ArcMode::CenterRadius);
            } else {
                setArcMode(ArcMode::ThreePoint);
            }
            event->accept();
            break;
            
        case Qt::Key_C:
            // Toggle construction mode
            m_constructionMode = !m_constructionMode;
            emit stateChanged();
            event->accept();
            break;
            
        case Qt::Key_F:
            // Flip arc direction
            m_clockwise = !m_clockwise;
            if (m_arcMode == ArcMode::CenterRadius && m_state == ArcToolState::Radius) {
                calculateArcFromCenterRadius();
            }
            emit previewUpdated();
            event->accept();
            break;
            
        default:
            SketchTool::handleKeyPress(event);
            break;
    }
}

SketchPreview ArcTool::getPreview() const
{
    SketchPreview preview;
    
    if (m_state == ArcToolState::Idle) {
        return preview;
    }
    
    preview.type = SketchPreview::Type::Arc;
    
    if (m_arcMode == ArcMode::ThreePoint) {
        switch (m_state) {
            case ArcToolState::StartPoint:
                // Show line from start to cursor
                preview.type = SketchPreview::Type::Line;
                preview.points = {m_startPoint, m_currentPoint};
                preview.valid = true;
                break;
                
            case ArcToolState::EndPoint:
                // Show arc preview
                if (m_arcValid) {
                    // Generate arc points for preview
                    const int numPoints = 32;
                    preview.points.clear();
                    
                    for (int i = 0; i <= numPoints; ++i) {
                        float t = static_cast<float>(i) / numPoints;
                        float angle = m_startAngle + t * m_sweepAngle;
                        QPointF p(
                            m_calculatedCenter.x() + m_calculatedRadius * std::cos(angle),
                            m_calculatedCenter.y() + m_calculatedRadius * std::sin(angle)
                        );
                        preview.points.push_back(p);
                    }
                    preview.valid = true;
                } else {
                    // Show lines if arc not yet valid
                    preview.type = SketchPreview::Type::Line;
                    preview.points = {m_startPoint, m_endPoint, m_currentPoint};
                    preview.valid = false;
                }
                break;
                
            default:
                break;
        }
    } else if (m_arcMode == ArcMode::CenterRadius) {
        switch (m_state) {
            case ArcToolState::Center:
                // Show radius line and circle
                preview.type = SketchPreview::Type::Circle;
                preview.points = {m_centerPoint, m_currentPoint};
                preview.valid = (m_radius > 0.001f);
                break;
                
            case ArcToolState::Radius:
                // Show arc preview
                if (m_radius > 0.001f) {
                    const int numPoints = 32;
                    preview.points.clear();
                    
                    for (int i = 0; i <= numPoints; ++i) {
                        float t = static_cast<float>(i) / numPoints;
                        float angle = m_startAngle + t * m_sweepAngle;
                        QPointF p(
                            m_centerPoint.x() + m_radius * std::cos(angle),
                            m_centerPoint.y() + m_radius * std::sin(angle)
                        );
                        preview.points.push_back(p);
                    }
                    preview.valid = true;
                }
                break;
                
            default:
                break;
        }
    }
    
    // Status text
    if (m_arcValid || m_radius > 0.001f) {
        float r = m_arcMode == ArcMode::ThreePoint ? m_calculatedRadius : m_radius;
        float sweep = std::abs(m_sweepAngle * 180.0f / M_PI);
        preview.statusText = QString("Radius: %1 mm  Sweep: %2°")
            .arg(r, 0, 'f', 2)
            .arg(sweep, 0, 'f', 1);
    }
    
    return preview;
}

QString ArcTool::getStatusText() const
{
    QString modeStr = m_arcMode == ArcMode::ThreePoint ? "3-Point" : "Center";
    
    if (m_arcMode == ArcMode::ThreePoint) {
        switch (m_state) {
            case ArcToolState::Idle:
                return QString("[%1 Arc] Click to place start point").arg(modeStr);
            case ArcToolState::StartPoint:
                return QString("[%1 Arc] Click to place end point").arg(modeStr);
            case ArcToolState::EndPoint:
                return QString("[%1 Arc] Click to set arc point").arg(modeStr);
            default:
                return QString();
        }
    } else {
        switch (m_state) {
            case ArcToolState::Idle:
                return QString("[%1 Arc] Click to place center").arg(modeStr);
            case ArcToolState::Center:
                return QString("[%1 Arc] Click to set radius and start angle").arg(modeStr);
            case ArcToolState::Radius:
                return QString("[%1 Arc] Click to set end angle (F to flip)").arg(modeStr);
            default:
                return QString();
        }
    }
}

void ArcTool::createArc()
{
    if (!m_arcValid && m_radius < 0.001f) {
        return;
    }
    
    // Create arc entity
    // In a real implementation, this would create a SketchArcEntity
    
    /*
    auto arc = std::make_shared<SketchArcEntity>();
    if (m_arcMode == ArcMode::ThreePoint) {
        arc->center = m_calculatedCenter;
        arc->radius = m_calculatedRadius;
    } else {
        arc->center = m_centerPoint;
        arc->radius = m_radius;
    }
    arc->startAngle = m_startAngle;
    arc->endAngle = m_endAngle;
    arc->isConstruction = m_constructionMode;
    
    m_sketchMode->sketchData()->addEntity(arc);
    */
    
    emit entityCreated();
}

void ArcTool::calculateArcFromThreePoints()
{
    // Calculate circle passing through three points
    // Using circumcircle formula
    
    float ax = m_startPoint.x();
    float ay = m_startPoint.y();
    float bx = m_endPoint.x();
    float by = m_endPoint.y();
    float cx = m_arcPoint.x();
    float cy = m_arcPoint.y();
    
    float d = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    
    if (std::abs(d) < 1e-10) {
        // Points are collinear
        m_arcValid = false;
        return;
    }
    
    float aSq = ax * ax + ay * ay;
    float bSq = bx * bx + by * by;
    float cSq = cx * cx + cy * cy;
    
    float centerX = (aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / d;
    float centerY = (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / d;
    
    m_calculatedCenter = QPointF(centerX, centerY);
    m_calculatedRadius = std::sqrt(
        std::pow(ax - centerX, 2) + std::pow(ay - centerY, 2)
    );
    
    // Calculate angles
    m_startAngle = std::atan2(ay - centerY, ax - centerX);
    m_endAngle = std::atan2(by - centerY, bx - centerX);
    
    // Calculate sweep angle (ensure correct direction through arc point)
    float arcAngle = std::atan2(cy - centerY, cx - centerX);
    
    // Determine sweep direction
    float sweep1 = m_endAngle - m_startAngle;
    float sweep2 = (sweep1 > 0) ? sweep1 - 2 * M_PI : sweep1 + 2 * M_PI;
    
    // Check which sweep contains the arc point
    auto angleInSweep = [](float angle, float start, float sweep) {
        float normalized = angle - start;
        if (sweep > 0) {
            while (normalized < 0) normalized += 2 * M_PI;
            return normalized <= sweep;
        } else {
            while (normalized > 0) normalized -= 2 * M_PI;
            return normalized >= sweep;
        }
    };
    
    if (angleInSweep(arcAngle, m_startAngle, sweep1)) {
        m_sweepAngle = sweep1;
    } else {
        m_sweepAngle = sweep2;
    }
    
    m_arcValid = true;
}

void ArcTool::calculateArcFromCenterRadius()
{
    // Calculate sweep angle
    m_sweepAngle = m_endAngle - m_startAngle;
    
    // Normalize to desired direction
    if (m_clockwise) {
        if (m_sweepAngle > 0) {
            m_sweepAngle -= 2 * M_PI;
        }
    } else {
        if (m_sweepAngle < 0) {
            m_sweepAngle += 2 * M_PI;
        }
    }
    
    m_arcValid = (m_radius > 0.001f && std::abs(m_sweepAngle) > 0.001f);
}

} // namespace dc
