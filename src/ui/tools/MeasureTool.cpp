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
#include <QPen>
#include <QBrush>
#include <QFontMetrics>
#include <QtMath>
#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    
    // Warn that this is a placeholder implementation
    qWarning() << "MeasureTool::estimateRadius() - Using placeholder value. "
                  "Proper radius fitting from mesh curvature not yet implemented.";
    
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
    // Legacy OpenGL rendering - deprecated, use paintOverlay() instead
    Q_UNUSED(point);
    Q_UNUSED(color);
    Q_UNUSED(size);
}

void MeasureTool::renderLine(const QVector3D& p1, const QVector3D& p2, const QColor& color, float width) {
    // Legacy OpenGL rendering - deprecated, use paintOverlay() instead
    Q_UNUSED(p1);
    Q_UNUSED(p2);
    Q_UNUSED(color);
    Q_UNUSED(width);
}

void MeasureTool::renderText(const QVector3D& worldPos, const QString& text, const QColor& color) {
    // Legacy OpenGL rendering - deprecated, use paintOverlay() instead
    Q_UNUSED(worldPos);
    Q_UNUSED(text);
    Q_UNUSED(color);
}

// ============================================================================
// QPainter-based Overlay Rendering
// ============================================================================

QPoint MeasureTool::worldToScreen(const QVector3D& worldPos, const QSize& viewportSize) const {
    if (!m_viewport) {
        return QPoint(-1, -1);
    }
    
    // Get camera matrices
    const Camera& camera = m_viewport->camera();
    QMatrix4x4 viewProj = camera.viewProjectionMatrix();
    
    // Transform world position to clip space
    QVector4D clipPos = viewProj * QVector4D(worldPos, 1.0f);
    
    // Check if point is behind camera
    if (clipPos.w() <= 0.0f) {
        return QPoint(-10000, -10000);  // Off-screen
    }
    
    // Perspective divide to get NDC
    QVector3D ndc = clipPos.toVector3D() / clipPos.w();
    
    // Convert NDC [-1,1] to screen coordinates [0, width/height]
    // Note: Y is flipped (NDC y+ is up, screen y+ is down)
    float screenX = (ndc.x() + 1.0f) * 0.5f * viewportSize.width();
    float screenY = (1.0f - ndc.y()) * 0.5f * viewportSize.height();
    
    return QPoint(static_cast<int>(screenX), static_cast<int>(screenY));
}

bool MeasureTool::isPointVisible(const QVector3D& worldPos, const QSize& viewportSize) const {
    if (!m_viewport) return false;
    
    const Camera& camera = m_viewport->camera();
    QMatrix4x4 viewProj = camera.viewProjectionMatrix();
    
    QVector4D clipPos = viewProj * QVector4D(worldPos, 1.0f);
    
    // Check if behind camera
    if (clipPos.w() <= 0.0f) return false;
    
    // Check if in NDC bounds
    QVector3D ndc = clipPos.toVector3D() / clipPos.w();
    return (ndc.x() >= -1.0f && ndc.x() <= 1.0f &&
            ndc.y() >= -1.0f && ndc.y() <= 1.0f &&
            ndc.z() >= -1.0f && ndc.z() <= 1.0f);
}

void MeasureTool::paintOverlay(QPainter& painter, const QSize& viewportSize) {
    if (!m_active && m_measurements.empty()) {
        return;
    }
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Render all stored measurements
    for (const auto& m : m_measurements) {
        if (!m.visible) continue;
        
        switch (m.type) {
            case Measurement::Type::Distance:
                paintDistanceMeasurement(painter, viewportSize, m);
                break;
            case Measurement::Type::Angle:
                paintAngleMeasurement(painter, viewportSize, m);
                break;
            case Measurement::Type::Radius:
                paintRadiusMeasurement(painter, viewportSize, m);
                break;
        }
    }
    
    // Render current measurement in progress
    if (m_active && !m_currentPoints.empty()) {
        paintCurrentProgress(painter, viewportSize);
    }
}

void MeasureTool::paintDistanceMeasurement(QPainter& painter, const QSize& viewportSize, const Measurement& m) {
    if (m.points.size() < 2) return;
    
    QPoint sp1 = worldToScreen(m.points[0], viewportSize);
    QPoint sp2 = worldToScreen(m.points[1], viewportSize);
    
    // Draw endpoint markers
    paintPoint(painter, sp1, m.color, m_pointSize);
    paintPoint(painter, sp2, m.color, m_pointSize);
    
    // Draw measurement line
    paintLine(painter, sp1, sp2, m.color, m_lineWidth);
    
    // Draw label at midpoint
    if (m_showLabels) {
        QPoint midpoint((sp1.x() + sp2.x()) / 2, (sp1.y() + sp2.y()) / 2);
        QString label = QString("%1 mm").arg(m.value, 0, 'f', 2);
        paintText(painter, midpoint, label, m.color);
    }
}

void MeasureTool::paintAngleMeasurement(QPainter& painter, const QSize& viewportSize, const Measurement& m) {
    if (m.points.size() < 3) return;
    
    QPoint sp1 = worldToScreen(m.points[0], viewportSize);
    QPoint spVertex = worldToScreen(m.points[1], viewportSize);
    QPoint sp3 = worldToScreen(m.points[2], viewportSize);
    
    // Draw point markers
    paintPoint(painter, sp1, m.color, m_pointSize);
    paintPoint(painter, spVertex, m.color, m_pointSize * 1.2f);  // Vertex slightly larger
    paintPoint(painter, sp3, m.color, m_pointSize);
    
    // Draw angle arms
    paintLine(painter, spVertex, sp1, m.color, m_lineWidth);
    paintLine(painter, spVertex, sp3, m.color, m_lineWidth);
    
    // Draw arc indicator
    float arm1Len = std::sqrt(std::pow(sp1.x() - spVertex.x(), 2) + std::pow(sp1.y() - spVertex.y(), 2));
    float arm2Len = std::sqrt(std::pow(sp3.x() - spVertex.x(), 2) + std::pow(sp3.y() - spVertex.y(), 2));
    float arcRadius = 0.25f * qMin(arm1Len, arm2Len);
    arcRadius = qMax(arcRadius, 20.0f);  // Minimum arc size
    
    paintArc(painter, spVertex, sp1, sp3, arcRadius, m.color, m_lineWidth * 0.75f);
    
    // Draw label
    if (m_showLabels) {
        // Position label at arc midpoint
        float angle1 = std::atan2(sp1.y() - spVertex.y(), sp1.x() - spVertex.x());
        float angle2 = std::atan2(sp3.y() - spVertex.y(), sp3.x() - spVertex.x());
        float midAngle = (angle1 + angle2) / 2.0f;
        
        // Handle angle wrap-around
        if (std::abs(angle1 - angle2) > M_PI) {
            midAngle += M_PI;
        }
        
        QPoint labelPos(
            spVertex.x() + static_cast<int>((arcRadius + 15) * std::cos(midAngle)),
            spVertex.y() + static_cast<int>((arcRadius + 15) * std::sin(midAngle))
        );
        QString label = QString("%1°").arg(m.value, 0, 'f', 1);
        paintText(painter, labelPos, label, m.color);
    }
}

void MeasureTool::paintRadiusMeasurement(QPainter& painter, const QSize& viewportSize, const Measurement& m) {
    if (m.points.empty()) return;
    
    QPoint spCenter = worldToScreen(m.points[0], viewportSize);
    
    // Draw center point
    paintPoint(painter, spCenter, m.color, m_pointSize);
    
    // Draw label
    if (m_showLabels) {
        QPoint labelPos(spCenter.x() + 15, spCenter.y() - 15);
        QString label = QString("R=%1\nØ=%2")
            .arg(m.value, 0, 'f', 2)
            .arg(m.secondaryValue, 0, 'f', 2);
        paintText(painter, labelPos, label, m.color);
    }
}

void MeasureTool::paintCurrentProgress(QPainter& painter, const QSize& viewportSize) {
    QColor previewColor(255, 255, 0, 200);  // Semi-transparent yellow
    
    // Draw placed points
    for (const auto& pt : m_currentPoints) {
        QPoint screenPt = worldToScreen(pt, viewportSize);
        paintPoint(painter, screenPt, previewColor, m_pointSize);
    }
    
    // Draw preview line to cursor
    if (m_hasPreview && !m_currentPoints.empty()) {
        QPoint previewScreenPt = worldToScreen(m_previewPoint, viewportSize);
        QColor fadedPreview(255, 255, 0, 150);
        
        if (m_mode == MeasureMode::Distance && m_currentPoints.size() == 1) {
            QPoint sp1 = worldToScreen(m_currentPoints[0], viewportSize);
            paintLine(painter, sp1, previewScreenPt, fadedPreview, m_lineWidth * 0.75f);
            paintPoint(painter, previewScreenPt, fadedPreview, m_pointSize * 0.8f);
            
            // Show preview distance
            if (m_showLabels) {
                QPoint mid((sp1.x() + previewScreenPt.x()) / 2, (sp1.y() + previewScreenPt.y()) / 2);
                double dist = calculateDistance(m_currentPoints[0], m_previewPoint);
                paintText(painter, mid, QString("%1 mm").arg(dist, 0, 'f', 2), fadedPreview);
            }
        } else if (m_mode == MeasureMode::Angle) {
            if (m_currentPoints.size() == 1) {
                QPoint sp1 = worldToScreen(m_currentPoints[0], viewportSize);
                paintLine(painter, sp1, previewScreenPt, fadedPreview, m_lineWidth * 0.75f);
            } else if (m_currentPoints.size() == 2) {
                QPoint sp1 = worldToScreen(m_currentPoints[0], viewportSize);
                QPoint spVertex = worldToScreen(m_currentPoints[1], viewportSize);
                paintLine(painter, spVertex, sp1, fadedPreview, m_lineWidth * 0.75f);
                paintLine(painter, spVertex, previewScreenPt, fadedPreview, m_lineWidth * 0.75f);
                
                // Show preview angle
                if (m_showLabels) {
                    double angle = calculateAngle(m_currentPoints[0], m_currentPoints[1], m_previewPoint);
                    
                    float arm1Len = std::sqrt(std::pow(sp1.x() - spVertex.x(), 2) + 
                                              std::pow(sp1.y() - spVertex.y(), 2));
                    float arm2Len = std::sqrt(std::pow(previewScreenPt.x() - spVertex.x(), 2) + 
                                              std::pow(previewScreenPt.y() - spVertex.y(), 2));
                    float arcRadius = 0.2f * qMin(arm1Len, arm2Len);
                    arcRadius = qMax(arcRadius, 15.0f);
                    
                    float angle1 = std::atan2(sp1.y() - spVertex.y(), sp1.x() - spVertex.x());
                    float angle2 = std::atan2(previewScreenPt.y() - spVertex.y(), 
                                              previewScreenPt.x() - spVertex.x());
                    float midAngle = (angle1 + angle2) / 2.0f;
                    if (std::abs(angle1 - angle2) > M_PI) midAngle += M_PI;
                    
                    QPoint labelPos(
                        spVertex.x() + static_cast<int>((arcRadius + 10) * std::cos(midAngle)),
                        spVertex.y() + static_cast<int>((arcRadius + 10) * std::sin(midAngle))
                    );
                    paintText(painter, labelPos, QString("%1°").arg(angle, 0, 'f', 1), fadedPreview);
                }
            }
            paintPoint(painter, previewScreenPt, fadedPreview, m_pointSize * 0.8f);
        }
    }
}

void MeasureTool::paintPoint(QPainter& painter, const QPoint& screenPos, const QColor& color, float size) {
    // Draw filled circle with border
    painter.save();
    
    // Outer circle (darker border)
    QColor borderColor = color.darker(130);
    painter.setPen(QPen(borderColor, 2.0f));
    painter.setBrush(color);
    
    int radius = static_cast<int>(size / 2.0f);
    painter.drawEllipse(screenPos, radius, radius);
    
    // Inner highlight
    QColor highlightColor = color.lighter(150);
    highlightColor.setAlpha(100);
    painter.setPen(Qt::NoPen);
    painter.setBrush(highlightColor);
    painter.drawEllipse(screenPos.x() - radius/3, screenPos.y() - radius/3, radius/2, radius/2);
    
    painter.restore();
}

void MeasureTool::paintLine(QPainter& painter, const QPoint& p1, const QPoint& p2, const QColor& color, float width) {
    painter.save();
    
    // Draw shadow for depth
    QPen shadowPen(QColor(0, 0, 0, 80), width + 2.0f, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(shadowPen);
    painter.drawLine(p1.x() + 1, p1.y() + 1, p2.x() + 1, p2.y() + 1);
    
    // Draw main line
    QPen linePen(color, width, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(linePen);
    painter.drawLine(p1, p2);
    
    painter.restore();
}

void MeasureTool::paintText(QPainter& painter, const QPoint& screenPos, const QString& text, const QColor& color) {
    painter.save();
    
    // Setup font
    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(QRect(), Qt::AlignCenter, text);
    textRect.moveCenter(screenPos);
    textRect.adjust(-6, -3, 6, 3);  // Add padding
    
    // Draw background with rounded corners
    QColor bgColor(30, 30, 30, 200);
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(textRect, 4, 4);
    
    // Draw border
    QColor borderColor = color.darker(120);
    borderColor.setAlpha(200);
    painter.setPen(QPen(borderColor, 1.5f));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(textRect, 4, 4);
    
    // Draw text
    painter.setPen(color);
    painter.drawText(textRect, Qt::AlignCenter, text);
    
    painter.restore();
}

void MeasureTool::paintArc(QPainter& painter, const QPoint& vertex, const QPoint& p1, const QPoint& p2,
                           float arcRadius, const QColor& color, float width) {
    painter.save();
    
    // Calculate angles for the arc
    float angle1 = std::atan2(p1.y() - vertex.y(), p1.x() - vertex.x());
    float angle2 = std::atan2(p2.y() - vertex.y(), p2.x() - vertex.x());
    
    // Convert to degrees and adjust for Qt's coordinate system
    // Qt uses 1/16th of a degree, with 0 at 3 o'clock, going counter-clockwise
    int startAngle = static_cast<int>(-angle1 * 180.0 / M_PI * 16);
    int spanAngle = static_cast<int>(-(angle2 - angle1) * 180.0 / M_PI * 16);
    
    // Normalize span angle to be the shorter arc
    while (spanAngle > 180 * 16) spanAngle -= 360 * 16;
    while (spanAngle < -180 * 16) spanAngle += 360 * 16;
    
    // Define arc bounding rectangle
    int r = static_cast<int>(arcRadius);
    QRect arcRect(vertex.x() - r, vertex.y() - r, r * 2, r * 2);
    
    // Draw arc
    QPen arcPen(color, width, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(arcPen);
    painter.drawArc(arcRect, startAngle, spanAngle);
    
    painter.restore();
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
