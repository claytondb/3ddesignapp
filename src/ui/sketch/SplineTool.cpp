/**
 * @file SplineTool.cpp
 * @brief Implementation of spline drawing tool
 */

#include "SplineTool.h"
#include "SketchMode.h"
#include "SketchViewport.h"

#include <QKeyEvent>
#include <cmath>

namespace dc {

SplineTool::SplineTool(SketchMode* sketchMode, QObject* parent)
    : SketchTool(sketchMode, parent)
{
}

void SplineTool::activate()
{
    SketchTool::activate();
    m_state = SplineToolState::Idle;
    m_controlPoints.clear();
}

void SplineTool::deactivate()
{
    SketchTool::deactivate();
    m_state = SplineToolState::Idle;
}

void SplineTool::cancel()
{
    m_state = SplineToolState::Idle;
    m_drawing = false;
    m_controlPoints.clear();
    
    emit previewUpdated();
    emit stateChanged();
}

void SplineTool::finish()
{
    if (m_controlPoints.size() >= MIN_POINTS) {
        createSpline();
    }
    
    m_state = SplineToolState::Idle;
    m_drawing = false;
    m_controlPoints.clear();
    
    emit previewUpdated();
    emit stateChanged();
}

void SplineTool::reset()
{
    SketchTool::reset();
    m_state = SplineToolState::Idle;
    m_controlPoints.clear();
    m_currentPoint = QPointF();
}

void SplineTool::setSplineType(SplineType type)
{
    m_splineType = type;
    emit previewUpdated();
    emit stateChanged();
}

void SplineTool::removeLastPoint()
{
    if (!m_controlPoints.empty()) {
        m_controlPoints.pop_back();
        
        if (m_controlPoints.empty()) {
            m_state = SplineToolState::Idle;
            m_drawing = false;
        }
        
        emit previewUpdated();
    }
}

void SplineTool::clearPoints()
{
    m_controlPoints.clear();
    m_state = SplineToolState::Idle;
    m_drawing = false;
    
    emit previewUpdated();
}

void SplineTool::handleMousePress(const QPointF& pos, Qt::MouseButtons buttons,
                                   Qt::KeyboardModifiers /*modifiers*/)
{
    if (!(buttons & Qt::LeftButton)) {
        // Right-click to finish
        if (buttons & Qt::RightButton && m_state == SplineToolState::Drawing) {
            finish();
        }
        return;
    }
    
    // Add control point
    m_controlPoints.push_back(pos);
    
    if (m_state == SplineToolState::Idle) {
        m_state = SplineToolState::Drawing;
        m_drawing = true;
    }
    
    emit previewUpdated();
    emit stateChanged();
}

void SplineTool::handleMouseMove(const QPointF& pos, Qt::MouseButtons /*buttons*/,
                                  Qt::KeyboardModifiers /*modifiers*/)
{
    m_currentPoint = pos;
    emit previewUpdated();
}

void SplineTool::handleMouseRelease(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Spline uses click-click
}

void SplineTool::handleDoubleClick(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Remove the point added by the double-click
    if (!m_controlPoints.empty()) {
        m_controlPoints.pop_back();
    }
    
    // Finish the spline
    if (m_state == SplineToolState::Drawing) {
        finish();
    }
}

void SplineTool::handleKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
            // Remove last point
            removeLastPoint();
            event->accept();
            break;
            
        case Qt::Key_C:
            // Toggle closed spline
            m_closed = !m_closed;
            emit previewUpdated();
            emit stateChanged();
            event->accept();
            break;
            
        case Qt::Key_P:
            // Toggle control polygon visibility
            m_showControlPolygon = !m_showControlPolygon;
            emit previewUpdated();
            event->accept();
            break;
            
        case Qt::Key_T:
            // Cycle through spline types
            if (m_splineType == SplineType::BSpline) {
                setSplineType(SplineType::Bezier);
            } else if (m_splineType == SplineType::Bezier) {
                setSplineType(SplineType::Interpolating);
            } else {
                setSplineType(SplineType::BSpline);
            }
            event->accept();
            break;
            
        default:
            SketchTool::handleKeyPress(event);
            break;
    }
}

SketchPreview SplineTool::getPreview() const
{
    SketchPreview preview;
    preview.type = SketchPreview::Type::Spline;
    
    if (m_state == SplineToolState::Idle && m_controlPoints.empty()) {
        return preview;
    }
    
    // Build preview points including current cursor
    std::vector<QPointF> points = m_controlPoints;
    if (m_state == SplineToolState::Drawing) {
        points.push_back(m_currentPoint);
    }
    
    if (points.size() >= MIN_POINTS) {
        // Temporarily store points for evaluation
        auto* nonConstThis = const_cast<SplineTool*>(this);
        auto originalPoints = m_controlPoints;
        nonConstThis->m_controlPoints = points;
        
        preview.points = evaluateSpline();
        preview.valid = true;
        
        nonConstThis->m_controlPoints = originalPoints;
    } else {
        // Just show control points
        preview.points = points;
        preview.valid = false;
    }
    
    // Status text
    QString typeStr;
    switch (m_splineType) {
        case SplineType::BSpline: typeStr = "B-Spline"; break;
        case SplineType::Bezier: typeStr = "Bézier"; break;
        case SplineType::Interpolating: typeStr = "Interpolating"; break;
    }
    
    preview.statusText = QString("%1: %2 points%3")
        .arg(typeStr)
        .arg(points.size())
        .arg(m_closed ? " [Closed]" : "");
    
    return preview;
}

QString SplineTool::getStatusText() const
{
    QString typeStr;
    switch (m_splineType) {
        case SplineType::BSpline: typeStr = "B-Spline"; break;
        case SplineType::Bezier: typeStr = "Bézier"; break;
        case SplineType::Interpolating: typeStr = "Interpolating"; break;
    }
    
    switch (m_state) {
        case SplineToolState::Idle:
            return QString("[%1] Click to add first control point").arg(typeStr);
            
        case SplineToolState::Drawing:
            return QString("[%1] Click to add points, double-click/Enter to finish (%2 points)")
                .arg(typeStr)
                .arg(m_controlPoints.size());
            
        default:
            return QString();
    }
}

void SplineTool::createSpline()
{
    if (m_controlPoints.size() < MIN_POINTS) {
        return;
    }
    
    // Create spline entity
    // In a real implementation, this would create a SketchSplineEntity
    
    /*
    auto spline = std::make_shared<SketchSplineEntity>();
    spline->controlPoints = m_controlPoints;
    spline->splineType = m_splineType;
    spline->closed = m_closed;
    spline->isConstruction = m_constructionMode;
    
    m_sketchMode->sketchData()->addEntity(spline);
    */
    
    emit entityCreated();
}

std::vector<QPointF> SplineTool::evaluateSpline() const
{
    switch (m_splineType) {
        case SplineType::BSpline:
            return evaluateBSpline();
        case SplineType::Bezier:
            return evaluateBezier();
        case SplineType::Interpolating:
            return evaluateInterpolating();
        default:
            return m_controlPoints;
    }
}

std::vector<QPointF> SplineTool::evaluateBSpline() const
{
    std::vector<QPointF> result;
    
    if (m_controlPoints.size() < 2) {
        return m_controlPoints;
    }
    
    // Uniform cubic B-spline evaluation
    // For simplicity, use Catmull-Rom style interpolation
    
    const int segments = static_cast<int>(m_controlPoints.size()) - 1;
    const int pointsPerSegment = 16;
    
    for (int seg = 0; seg < segments; ++seg) {
        // Get 4 control points (with clamping at ends)
        int i0 = std::max(0, seg - 1);
        int i1 = seg;
        int i2 = std::min(seg + 1, segments);
        int i3 = std::min(seg + 2, segments);
        
        QPointF p0 = m_controlPoints[i0];
        QPointF p1 = m_controlPoints[i1];
        QPointF p2 = m_controlPoints[i2];
        QPointF p3 = m_controlPoints[i3];
        
        for (int i = 0; i <= pointsPerSegment; ++i) {
            float t = static_cast<float>(i) / pointsPerSegment;
            float t2 = t * t;
            float t3 = t2 * t;
            
            // Catmull-Rom basis functions
            float b0 = -0.5f * t3 + t2 - 0.5f * t;
            float b1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
            float b2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
            float b3 = 0.5f * t3 - 0.5f * t2;
            
            QPointF pt(
                b0 * p0.x() + b1 * p1.x() + b2 * p2.x() + b3 * p3.x(),
                b0 * p0.y() + b1 * p1.y() + b2 * p2.y() + b3 * p3.y()
            );
            
            result.push_back(pt);
        }
    }
    
    // Close the spline if requested
    if (m_closed && !result.empty()) {
        result.push_back(result.front());
    }
    
    return result;
}

std::vector<QPointF> SplineTool::evaluateBezier() const
{
    std::vector<QPointF> result;
    
    if (m_controlPoints.size() < 2) {
        return m_controlPoints;
    }
    
    // De Casteljau's algorithm for Bezier curve
    const int numPoints = std::max(32, static_cast<int>(m_controlPoints.size()) * 8);
    
    for (int i = 0; i <= numPoints; ++i) {
        float t = static_cast<float>(i) / numPoints;
        
        // De Casteljau
        std::vector<QPointF> temp = m_controlPoints;
        
        while (temp.size() > 1) {
            std::vector<QPointF> next;
            for (size_t j = 0; j < temp.size() - 1; ++j) {
                QPointF p(
                    (1 - t) * temp[j].x() + t * temp[j + 1].x(),
                    (1 - t) * temp[j].y() + t * temp[j + 1].y()
                );
                next.push_back(p);
            }
            temp = next;
        }
        
        result.push_back(temp[0]);
    }
    
    return result;
}

std::vector<QPointF> SplineTool::evaluateInterpolating() const
{
    std::vector<QPointF> result;
    
    if (m_controlPoints.size() < 2) {
        return m_controlPoints;
    }
    
    // Interpolating spline using centripetal Catmull-Rom
    const int segments = static_cast<int>(m_controlPoints.size()) - 1;
    const int pointsPerSegment = 16;
    
    for (int seg = 0; seg < segments; ++seg) {
        // Get 4 control points
        int i0 = std::max(0, seg - 1);
        int i1 = seg;
        int i2 = std::min(seg + 1, segments);
        int i3 = std::min(seg + 2, segments);
        
        QPointF p0 = m_controlPoints[i0];
        QPointF p1 = m_controlPoints[i1];
        QPointF p2 = m_controlPoints[i2];
        QPointF p3 = m_controlPoints[i3];
        
        // Calculate centripetal parameterization
        auto dist = [](const QPointF& a, const QPointF& b) {
            return std::sqrt(std::pow(a.x() - b.x(), 2) + std::pow(a.y() - b.y(), 2));
        };
        
        float t0 = 0.0f;
        float t1 = t0 + std::pow(dist(p0, p1), 0.5f);
        float t2 = t1 + std::pow(dist(p1, p2), 0.5f);
        float t3 = t2 + std::pow(dist(p2, p3), 0.5f);
        
        for (int i = 0; i <= pointsPerSegment; ++i) {
            float t = t1 + (t2 - t1) * static_cast<float>(i) / pointsPerSegment;
            
            // Centripetal Catmull-Rom interpolation
            auto lerp = [](const QPointF& a, const QPointF& b, float u) {
                return QPointF(a.x() * (1 - u) + b.x() * u, a.y() * (1 - u) + b.y() * u);
            };
            
            QPointF A1 = lerp(p0, p1, (t - t0) / (t1 - t0 + 1e-6f));
            QPointF A2 = lerp(p1, p2, (t - t1) / (t2 - t1 + 1e-6f));
            QPointF A3 = lerp(p2, p3, (t - t2) / (t3 - t2 + 1e-6f));
            
            QPointF B1 = lerp(A1, A2, (t - t0) / (t2 - t0 + 1e-6f));
            QPointF B2 = lerp(A2, A3, (t - t1) / (t3 - t1 + 1e-6f));
            
            QPointF C = lerp(B1, B2, (t - t1) / (t2 - t1 + 1e-6f));
            
            result.push_back(C);
        }
    }
    
    // Close the spline if requested
    if (m_closed && !result.empty()) {
        result.push_back(result.front());
    }
    
    return result;
}

} // namespace dc
