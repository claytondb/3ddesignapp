/**
 * @file MeasureTool.cpp
 * @brief Implementation of interactive measurement tool
 */

#include "MeasureTool.h"
#include "../../renderer/Viewport.h"
#include "../../renderer/Camera.h"
#include "../../geometry/MeshData.h"

#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QPainter>
#include <QFontMetrics>
#include <QtMath>
#include <QDebug>
#include <cmath>

namespace dc {

MeasureTool::MeasureTool(Viewport* viewport, QObject* parent)
    : QObject(parent)
    , m_viewport(viewport)
{
}

MeasureTool::~MeasureTool() = default;

void MeasureTool::activate() {
    if (!m_active) {
        m_active = true;
        m_currentPoints.clear();
        m_hasPreview = false;
        updateToolHint();
        emit activeChanged(true);
    }
}

void MeasureTool::deactivate() {
    if (m_active) {
        m_active = false;
        m_currentPoints.clear();
        m_hasPreview = false;
        emit toolHintUpdate(QString());
        emit activeChanged(false);
    }
}

void MeasureTool::setMode(MeasureMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        cancelCurrent();
        updateToolHint();
        emit modeChanged(mode);
    }
}

QString MeasureTool::modeName() const {
    switch (m_mode) {
        case MeasureMode::None:     return tr("None");
        case MeasureMode::Distance: return tr("Distance");
        case MeasureMode::Angle:    return tr("Angle");
        case MeasureMode::Radius:   return tr("Radius");
    }
    return QString();
}

int MeasureTool::pointsNeeded() const {
    switch (m_mode) {
        case MeasureMode::None:     return 0;
        case MeasureMode::Distance: return 2;
        case MeasureMode::Angle:    return 3;
        case MeasureMode::Radius:   return 1;
    }
    return 0;
}

void MeasureTool::cancelCurrent() {
    m_currentPoints.clear();
    m_hasPreview = false;
    updateToolHint();
    if (m_viewport) {
        m_viewport->update();
    }
}

void MeasureTool::clearAllMeasurements() {
    m_measurements.clear();
    cancelCurrent();
    emit measurementsCleared();
    if (m_viewport) {
        m_viewport->update();
    }
}

void MeasureTool::clearLastMeasurement() {
    if (!m_measurements.empty()) {
        m_measurements.pop_back();
        if (m_viewport) {
            m_viewport->update();
        }
    }
}

void MeasureTool::handleClick(const QVector3D& worldPos, const QPoint& screenPos, Qt::MouseButton button) {
    Q_UNUSED(screenPos);
    
    if (!m_active || m_mode == MeasureMode::None) {
        return;
    }
    
    if (button == Qt::LeftButton) {
        m_currentPoints.push_back(worldPos);
        
        QString progressText;
        int needed = pointsNeeded();
        int current = pointCount();
        
        if (current < needed) {
            progressText = tr("Point %1/%2 placed").arg(current).arg(needed);
            emit statusUpdate(progressText);
            updateToolHint();
        }
        
        // Check if measurement is complete
        if (isComplete()) {
            switch (m_mode) {
                case MeasureMode::Distance:
                    completeDistanceMeasurement();
                    break;
                case MeasureMode::Angle:
                    completeAngleMeasurement();
                    break;
                case MeasureMode::Radius:
                    completeRadiusMeasurement(worldPos);
                    break;
                default:
                    break;
            }
            
            m_currentPoints.clear();
            updateToolHint();
        }
        
        if (m_viewport) {
            m_viewport->update();
        }
    } else if (button == Qt::RightButton) {
        // Right-click cancels current measurement
        cancelCurrent();
        emit statusUpdate(tr("Measurement cancelled"));
    }
}

void MeasureTool::handleMouseMove(const QVector3D& worldPos, const QPoint& screenPos) {
    Q_UNUSED(screenPos);
    
    if (!m_active || m_mode == MeasureMode::None) {
        return;
    }
    
    m_previewPoint = worldPos;
    m_hasPreview = true;
    
    // Show live preview measurement
    if (!m_currentPoints.empty()) {
        QString previewText;
        
        switch (m_mode) {
            case MeasureMode::Distance:
                if (m_currentPoints.size() == 1) {
                    double dist = calculateDistance(m_currentPoints[0], worldPos);
                    previewText = tr("Distance: %1 mm").arg(dist, 0, 'f', 3);
                }
                break;
                
            case MeasureMode::Angle:
                if (m_currentPoints.size() == 2) {
                    double angle = calculateAngle(m_currentPoints[0], m_currentPoints[1], worldPos);
                    previewText = tr("Angle: %1°").arg(angle, 0, 'f', 2);
                }
                break;
                
            default:
                break;
        }
        
        if (!previewText.isEmpty()) {
            emit statusUpdate(previewText);
        }
    }
    
    if (m_viewport) {
        m_viewport->update();
    }
}

void MeasureTool::handleKeyPress(int key) {
    if (!m_active) {
        return;
    }
    
    switch (key) {
        case Qt::Key_Escape:
            cancelCurrent();
            emit statusUpdate(tr("Measurement cancelled"));
            break;
            
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
            if (m_currentPoints.empty()) {
                clearLastMeasurement();
                emit statusUpdate(tr("Last measurement cleared"));
            } else {
                cancelCurrent();
                emit statusUpdate(tr("Current measurement cleared"));
            }
            break;
            
        case Qt::Key_C:
            clearAllMeasurements();
            emit statusUpdate(tr("All measurements cleared"));
            break;
            
        case Qt::Key_1:
            setMode(MeasureMode::Distance);
            emit statusUpdate(tr("Mode: Distance measurement"));
            break;
            
        case Qt::Key_2:
            setMode(MeasureMode::Angle);
            emit statusUpdate(tr("Mode: Angle measurement"));
            break;
            
        case Qt::Key_3:
            setMode(MeasureMode::Radius);
            emit statusUpdate(tr("Mode: Radius measurement"));
            break;
    }
}

void MeasureTool::completeDistanceMeasurement() {
    if (m_currentPoints.size() < 2) return;
    
    Measurement m;
    m.type = Measurement::Type::Distance;
    m.points = m_currentPoints;
    m.value = calculateDistance(m_currentPoints[0], m_currentPoints[1]);
    m.color = m_currentColor;
    m.visible = true;
    m.label = tr("D%1").arg(m_measurements.size() + 1);
    
    m_measurements.push_back(m);
    
    QString resultText = tr("Distance: %1 mm").arg(m.value, 0, 'f', 3);
    emit statusUpdate(resultText);
    emit measurementCompleted(m);
}

void MeasureTool::completeAngleMeasurement() {
    if (m_currentPoints.size() < 3) return;
    
    Measurement m;
    m.type = Measurement::Type::Angle;
    m.points = m_currentPoints;
    m.value = calculateAngle(m_currentPoints[0], m_currentPoints[1], m_currentPoints[2]);
    m.color = m_currentColor;
    m.visible = true;
    m.label = tr("A%1").arg(m_measurements.size() + 1);
    
    m_measurements.push_back(m);
    
    QString resultText = tr("Angle: %1°").arg(m.value, 0, 'f', 2);
    emit statusUpdate(resultText);
    emit measurementCompleted(m);
}

void MeasureTool::completeRadiusMeasurement(const QVector3D& clickPos) {
    Measurement m;
    m.type = Measurement::Type::Radius;
    m.points = {clickPos};
    m.value = estimateRadius(clickPos);
    m.secondaryValue = m.value * 2.0;  // Diameter
    m.color = m_currentColor;
    m.visible = true;
    m.label = tr("R%1").arg(m_measurements.size() + 1);
    
    m_measurements.push_back(m);
    
    QString resultText = tr("Radius: %1 mm (Diameter: %2 mm)")
        .arg(m.value, 0, 'f', 3)
        .arg(m.secondaryValue, 0, 'f', 3);
    emit statusUpdate(resultText);
    emit measurementCompleted(m);
}

double MeasureTool::calculateDistance(const QVector3D& p1, const QVector3D& p2) const {
    return static_cast<double>((p2 - p1).length());
}

double MeasureTool::calculateAngle(const QVector3D& p1, const QVector3D& vertex, const QVector3D& p3) const {
    QVector3D v1 = (p1 - vertex).normalized();
    QVector3D v2 = (p3 - vertex).normalized();
    
    float dot = QVector3D::dotProduct(v1, v2);
    // Clamp to avoid numerical issues with acos
    dot = qBound(-1.0f, dot, 1.0f);
    
    double angleRad = std::acos(static_cast<double>(dot));
    return qRadiansToDegrees(angleRad);
}

double MeasureTool::estimateRadius(const QVector3D& clickPos) const {
    // This is a simplified radius estimation
    // A full implementation would use surface curvature analysis from the mesh
    // For now, return a placeholder value
    // In practice, this should analyze local surface geometry
    
    Q_UNUSED(clickPos);
    
    // TODO: Implement proper radius fitting by:
    // 1. Get nearby vertices from the mesh
    // 2. Fit a sphere/cylinder to the local surface
    // 3. Return the fitted radius
    
    // Placeholder: return 10mm as default
    // Real implementation would use mesh sampling and least-squares fitting
    return 10.0;
}

void MeasureTool::render() {
    if (!m_active && m_measurements.empty()) {
        return;
    }
    
    // Render all stored measurements
    for (const auto& m : m_measurements) {
        if (!m.visible) continue;
        
        switch (m.type) {
            case Measurement::Type::Distance:
                renderDistanceMeasurement(m);
                break;
            case Measurement::Type::Angle:
                renderAngleMeasurement(m);
                break;
            case Measurement::Type::Radius:
                renderRadiusMeasurement(m);
                break;
        }
    }
    
    // Render current measurement in progress
    if (m_active && !m_currentPoints.empty()) {
        renderCurrentProgress();
    }
}

void MeasureTool::renderDistanceMeasurement(const Measurement& m) {
    if (m.points.size() < 2) return;
    
    const QVector3D& p1 = m.points[0];
    const QVector3D& p2 = m.points[1];
    
    // Draw endpoint markers
    renderPoint(p1, m.color, m_pointSize);
    renderPoint(p2, m.color, m_pointSize);
    
    // Draw measurement line
    renderLine(p1, p2, m.color, m_lineWidth);
    
    // Draw label at midpoint
    if (m_showLabels) {
        QVector3D midpoint = (p1 + p2) * 0.5f;
        QString label = QString("%1 mm").arg(m.value, 0, 'f', 2);
        renderText(midpoint, label, m.color);
    }
}

void MeasureTool::renderAngleMeasurement(const Measurement& m) {
    if (m.points.size() < 3) return;
    
    const QVector3D& p1 = m.points[0];
    const QVector3D& vertex = m.points[1];
    const QVector3D& p3 = m.points[2];
    
    // Draw point markers
    renderPoint(p1, m.color, m_pointSize);
    renderPoint(vertex, m.color, m_pointSize * 1.2f);  // Vertex slightly larger
    renderPoint(p3, m.color, m_pointSize);
    
    // Draw angle arms
    renderLine(vertex, p1, m.color, m_lineWidth);
    renderLine(vertex, p3, m.color, m_lineWidth);
    
    // Draw arc indicator (simplified as lines)
    // Full implementation would draw a proper arc
    QVector3D v1 = (p1 - vertex).normalized();
    QVector3D v2 = (p3 - vertex).normalized();
    float arcRadius = 0.2f * qMin((p1 - vertex).length(), (p3 - vertex).length());
    
    QVector3D arcP1 = vertex + v1 * arcRadius;
    QVector3D arcP2 = vertex + v2 * arcRadius;
    QVector3D arcMid = vertex + ((v1 + v2).normalized() * arcRadius);
    
    renderLine(arcP1, arcMid, m.color, m_lineWidth * 0.5f);
    renderLine(arcMid, arcP2, m.color, m_lineWidth * 0.5f);
    
    // Draw label
    if (m_showLabels) {
        QString label = QString("%1°").arg(m.value, 0, 'f', 1);
        renderText(arcMid, label, m.color);
    }
}

void MeasureTool::renderRadiusMeasurement(const Measurement& m) {
    if (m.points.empty()) return;
    
    const QVector3D& center = m.points[0];
    
    // Draw center point
    renderPoint(center, m.color, m_pointSize);
    
    // Draw label
    if (m_showLabels) {
        QString label = QString("R=%1\nØ=%2")
            .arg(m.value, 0, 'f', 2)
            .arg(m.secondaryValue, 0, 'f', 2);
        renderText(center, label, m.color);
    }
}

void MeasureTool::renderCurrentProgress() {
    // Draw placed points
    for (const auto& pt : m_currentPoints) {
        renderPoint(pt, Qt::yellow, m_pointSize);
    }
    
    // Draw preview line to cursor
    if (m_hasPreview && !m_currentPoints.empty()) {
        QColor previewColor(255, 255, 0, 150);  // Semi-transparent yellow
        
        if (m_mode == MeasureMode::Distance && m_currentPoints.size() == 1) {
            renderLine(m_currentPoints[0], m_previewPoint, previewColor, m_lineWidth * 0.75f);
            renderPoint(m_previewPoint, previewColor, m_pointSize * 0.8f);
            
            // Show preview distance
            if (m_showLabels) {
                QVector3D mid = (m_currentPoints[0] + m_previewPoint) * 0.5f;
                double dist = calculateDistance(m_currentPoints[0], m_previewPoint);
                renderText(mid, QString("%1 mm").arg(dist, 0, 'f', 2), previewColor);
            }
        } else if (m_mode == MeasureMode::Angle) {
            if (m_currentPoints.size() == 1) {
                renderLine(m_currentPoints[0], m_previewPoint, previewColor, m_lineWidth * 0.75f);
            } else if (m_currentPoints.size() == 2) {
                renderLine(m_currentPoints[1], m_currentPoints[0], previewColor, m_lineWidth * 0.75f);
                renderLine(m_currentPoints[1], m_previewPoint, previewColor, m_lineWidth * 0.75f);
                
                // Show preview angle
                if (m_showLabels) {
                    double angle = calculateAngle(m_currentPoints[0], m_currentPoints[1], m_previewPoint);
                    QVector3D v1 = (m_currentPoints[0] - m_currentPoints[1]).normalized();
                    QVector3D v2 = (m_previewPoint - m_currentPoints[1]).normalized();
                    float arcRadius = 0.2f * qMin((m_currentPoints[0] - m_currentPoints[1]).length(), 
                                                   (m_previewPoint - m_currentPoints[1]).length());
                    QVector3D arcMid = m_currentPoints[1] + ((v1 + v2).normalized() * arcRadius);
                    renderText(arcMid, QString("%1°").arg(angle, 0, 'f', 1), previewColor);
                }
            }
            renderPoint(m_previewPoint, previewColor, m_pointSize * 0.8f);
        }
    }
}

void MeasureTool::renderPoint(const QVector3D& point, const QColor& color, float size) {
    if (!m_viewport) return;
    
    // Get OpenGL functions
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) return;
    
    QOpenGLFunctions* f = ctx->functions();
    if (!f) return;
    
    // Enable point rendering
    // Note: glPointSize is not available in core profile, use gl_PointSize in shader
    f->glEnable(GL_PROGRAM_POINT_SIZE);
    // Point size is controlled via gl_PointSize in vertex shader
    Q_UNUSED(size);
    
    // Simple point rendering using immediate mode (legacy but simple)
    // Full implementation would use shaders and VBOs
    
    // For now, use QPainter overlay
    // The actual GL rendering would need proper shader setup
    Q_UNUSED(point);
    Q_UNUSED(color);
}

void MeasureTool::renderLine(const QVector3D& p1, const QVector3D& p2, const QColor& color, float width) {
    if (!m_viewport) return;
    
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) return;
    
    QOpenGLFunctions* f = ctx->functions();
    if (!f) return;
    
    f->glLineWidth(width);
    
    // Simple line rendering
    // Full implementation would use proper shaders
    Q_UNUSED(p1);
    Q_UNUSED(p2);
    Q_UNUSED(color);
}

void MeasureTool::renderText(const QVector3D& worldPos, const QString& text, const QColor& color) {
    if (!m_viewport) return;
    
    // Convert world position to screen coordinates
    // This would use the viewport's projection matrices
    // For now, this is a placeholder
    Q_UNUSED(worldPos);
    Q_UNUSED(text);
    Q_UNUSED(color);
}

QString MeasureTool::statusText() const {
    if (!m_active) {
        return QString();
    }
    
    if (m_measurements.empty() && m_currentPoints.empty()) {
        return tr("Measure: Click to place first point");
    }
    
    QString status;
    
    // Show last measurement
    if (!m_measurements.empty()) {
        const Measurement& last = m_measurements.back();
        switch (last.type) {
            case Measurement::Type::Distance:
                status = tr("Last: %1 mm").arg(last.value, 0, 'f', 3);
                break;
            case Measurement::Type::Angle:
                status = tr("Last: %1°").arg(last.value, 0, 'f', 2);
                break;
            case Measurement::Type::Radius:
                status = tr("Last: R=%1 mm").arg(last.value, 0, 'f', 3);
                break;
        }
    }
    
    // Show progress
    if (!m_currentPoints.empty()) {
        int needed = pointsNeeded();
        int current = pointCount();
        QString progress = tr(" (%1/%2 points)").arg(current).arg(needed);
        status += progress;
    }
    
    return status;
}

void MeasureTool::updateToolHint() {
    if (!m_active) {
        emit toolHintUpdate(QString());
        return;
    }
    
    QString hint;
    int current = pointCount();
    
    switch (m_mode) {
        case MeasureMode::Distance:
            if (current == 0) {
                hint = tr("Click first point for distance measurement");
            } else {
                hint = tr("Click second point to complete distance measurement");
            }
            break;
            
        case MeasureMode::Angle:
            if (current == 0) {
                hint = tr("Click first point (angle arm start)");
            } else if (current == 1) {
                hint = tr("Click vertex point (angle center)");
            } else {
                hint = tr("Click third point to complete angle measurement");
            }
            break;
            
        case MeasureMode::Radius:
            hint = tr("Click on curved surface to measure radius");
            break;
            
        default:
            break;
    }
    
    if (!hint.isEmpty()) {
        hint += tr(" | ESC=Cancel, C=Clear all, 1/2/3=Change mode");
    }
    
    emit toolHintUpdate(hint);
}

} // namespace dc
