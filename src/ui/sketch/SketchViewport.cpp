/**
 * @file SketchViewport.cpp
 * @brief Implementation of sketch viewport overlay
 */

#include "SketchViewport.h"
#include "SketchMode.h"
#include "../../renderer/Viewport.h"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <cmath>

namespace dc {

// Forward declare geometry types for placeholder implementation
namespace dc3d {
namespace geometry {

struct SketchEntity {
    uint64_t id;
    enum class Type { Line, Arc, Circle, Spline, Point, Rectangle } type;
    std::vector<QPointF> points;
    bool isConstruction = false;
    
    std::vector<QPointF> getEndpoints() const { return points; }
    QPointF getMidpoint() const {
        if (points.size() >= 2) {
            return QPointF((points[0].x() + points[1].x()) / 2,
                          (points[0].y() + points[1].y()) / 2);
        }
        return points.empty() ? QPointF() : points[0];
    }
    bool hasCenter() const { return type == Type::Circle || type == Type::Arc; }
    QPointF getCenter() const { return points.empty() ? QPointF() : points[0]; }
};

struct SketchConstraint {
    uint64_t id;
    enum class Type { Horizontal, Vertical, Coincident, Parallel, Perpendicular, Tangent, Equal, Fixed } type;
    std::vector<uint64_t> entityIds;
    QPointF displayPosition;
};

struct SketchDimension {
    uint64_t id;
    enum class Type { Linear, Angular, Radial, Diameter } type;
    std::vector<uint64_t> entityIds;
    double value;
    QPointF textPosition;
    QPointF startPoint;
    QPointF endPoint;
};

struct SketchData {
    std::vector<std::shared_ptr<SketchEntity>> entities;
    std::vector<std::shared_ptr<SketchConstraint>> constraints;
    std::vector<std::shared_ptr<SketchDimension>> dimensions;
    std::shared_ptr<SketchPlane> plane;
    
    bool hasContent() const { return !entities.empty(); }
    QRectF getBoundingBox() const {
        if (entities.empty()) return QRectF();
        
        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double maxY = std::numeric_limits<double>::lowest();
        
        for (const auto& e : entities) {
            for (const auto& p : e->points) {
                minX = std::min(minX, p.x());
                minY = std::min(minY, p.y());
                maxX = std::max(maxX, p.x());
                maxY = std::max(maxY, p.y());
            }
        }
        return QRectF(minX, minY, maxX - minX, maxY - minY);
    }
    
    std::vector<QPointF> findIntersections() const { return {}; }
    bool canUndo() const { return false; }
    bool canRedo() const { return false; }
    void undo() {}
    void redo() {}
};

struct SketchPlane {
    QVector3D origin;
    QVector3D normal;
    QVector3D xAxis;
    QVector3D yAxis;
};

}
}

// ============================================================================
// SketchViewport
// ============================================================================

SketchViewport::SketchViewport(Viewport* viewport, QWidget* parent)
    : QWidget(parent ? parent : viewport)
    , m_viewport(viewport)
    , m_currentTool(SketchToolType::None)
{
    // Transparent overlay
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // Match viewport size
    if (viewport) {
        setGeometry(viewport->rect());
    }
}

void SketchViewport::setSketchData(std::shared_ptr<dc3d::geometry::SketchData> data)
{
    m_sketchData = data;
    update();
}

void SketchViewport::setSketchPlane(std::shared_ptr<dc3d::geometry::SketchPlane> plane)
{
    m_sketchPlane = plane;
    update();
}

void SketchViewport::setGridSettings(const SketchGridSettings& settings)
{
    m_gridSettings = settings;
    m_colors.gridMajor = settings.majorColor;
    m_colors.gridMinor = settings.minorColor;
    update();
}

void SketchViewport::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    update();
}

void SketchViewport::setSnapIndicator(const QPointF& point, const QString& snapType)
{
    m_snapPoint = point;
    m_snapType = snapType;
    m_hasSnapIndicator = !snapType.isEmpty();
    update();
}

void SketchViewport::clearSnapIndicator()
{
    m_hasSnapIndicator = false;
    m_snapType.clear();
    update();
}

void SketchViewport::setCurrentTool(SketchToolType tool)
{
    m_currentTool = tool;
    update();
}

void SketchViewport::setPreview(const SketchPreview& preview)
{
    m_preview = preview;
    update();
}

void SketchViewport::clearPreview()
{
    m_preview = SketchPreview();
    update();
}

void SketchViewport::setSelectedEntities(const std::vector<uint64_t>& entityIds)
{
    m_selectedEntities = entityIds;
    update();
}

void SketchViewport::setHighlightedEntity(uint64_t entityId)
{
    m_highlightedEntity = entityId;
    update();
}

void SketchViewport::clearHighlight()
{
    m_highlightedEntity = 0;
    update();
}

void SketchViewport::setConstraintsVisible(bool visible)
{
    m_constraintsVisible = visible;
    update();
}

void SketchViewport::setDimensionsVisible(bool visible)
{
    m_dimensionsVisible = visible;
    update();
}

void SketchViewport::setConstructionVisible(bool visible)
{
    m_constructionVisible = visible;
    update();
}

void SketchViewport::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw layers in order
    if (m_gridVisible) {
        drawGrid(painter);
    }
    
    drawOriginMarker(painter);
    drawEntities(painter);
    
    if (m_constraintsVisible) {
        drawConstraints(painter);
    }
    
    if (m_dimensionsVisible) {
        drawDimensions(painter);
    }
    
    drawPreview(painter);
    
    if (m_hasSnapIndicator) {
        drawSnapIndicator(painter);
    }
    
    drawStatusText(painter);
}

void SketchViewport::resizeEvent(QResizeEvent* /*event*/)
{
    // Keep overlay matching viewport
    if (m_viewport) {
        setGeometry(m_viewport->rect());
    }
}

void SketchViewport::drawGrid(QPainter& painter)
{
    if (!m_viewport) return;
    
    // Get visible area in sketch coordinates
    QRectF visibleRect(-100, -100, 200, 200); // Default visible area
    
    // Calculate grid line positions
    float majorSpacing = m_gridSettings.majorSpacing;
    float minorSpacing = majorSpacing / m_gridSettings.minorDivisions;
    
    // Calculate start/end for grid lines
    float startX = std::floor(visibleRect.left() / majorSpacing) * majorSpacing;
    float endX = std::ceil(visibleRect.right() / majorSpacing) * majorSpacing;
    float startY = std::floor(visibleRect.top() / majorSpacing) * majorSpacing;
    float endY = std::ceil(visibleRect.bottom() / majorSpacing) * majorSpacing;
    
    // Draw minor grid lines
    painter.setPen(QPen(m_colors.gridMinor, 1, Qt::SolidLine));
    
    for (float x = startX; x <= endX; x += minorSpacing) {
        if (std::fmod(std::abs(x), majorSpacing) > 0.001f) {
            QPointF p1 = sketchToScreen(QPointF(x, startY));
            QPointF p2 = sketchToScreen(QPointF(x, endY));
            painter.drawLine(p1, p2);
        }
    }
    
    for (float y = startY; y <= endY; y += minorSpacing) {
        if (std::fmod(std::abs(y), majorSpacing) > 0.001f) {
            QPointF p1 = sketchToScreen(QPointF(startX, y));
            QPointF p2 = sketchToScreen(QPointF(endX, y));
            painter.drawLine(p1, p2);
        }
    }
    
    // Draw major grid lines
    painter.setPen(QPen(m_colors.gridMajor, 1, Qt::SolidLine));
    
    for (float x = startX; x <= endX; x += majorSpacing) {
        QPointF p1 = sketchToScreen(QPointF(x, startY));
        QPointF p2 = sketchToScreen(QPointF(x, endY));
        painter.drawLine(p1, p2);
    }
    
    for (float y = startY; y <= endY; y += majorSpacing) {
        QPointF p1 = sketchToScreen(QPointF(startX, y));
        QPointF p2 = sketchToScreen(QPointF(endX, y));
        painter.drawLine(p1, p2);
    }
}

void SketchViewport::drawEntities(QPainter& painter)
{
    if (!m_sketchData) return;
    
    for (const auto& entity : m_sketchData->entities) {
        if (!entity) continue;
        
        // Skip construction geometry if hidden
        if (entity->isConstruction && !m_constructionVisible) continue;
        
        bool selected = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), 
                                  entity->id) != m_selectedEntities.end();
        bool highlighted = (entity->id == m_highlightedEntity);
        
        drawEntity(painter, *entity, selected, highlighted);
    }
}

void SketchViewport::drawEntity(QPainter& painter, const dc3d::geometry::SketchEntity& entity,
                                 bool selected, bool highlighted)
{
    // Determine color
    QColor color = m_colors.entity;
    if (entity.isConstruction) {
        color = m_colors.construction;
    }
    if (highlighted) {
        color = m_colors.entityHighlight;
    }
    if (selected) {
        color = m_colors.entitySelected;
    }
    
    // Determine line style
    Qt::PenStyle penStyle = entity.isConstruction ? Qt::DashLine : Qt::SolidLine;
    float lineWidth = selected ? 2.5f : (highlighted ? 2.0f : 1.5f);
    
    painter.setPen(QPen(color, lineWidth, penStyle));
    painter.setBrush(Qt::NoBrush);
    
    switch (entity.type) {
        case dc3d::geometry::SketchEntity::Type::Line:
            if (entity.points.size() >= 2) {
                QPointF p1 = sketchToScreen(entity.points[0]);
                QPointF p2 = sketchToScreen(entity.points[1]);
                painter.drawLine(p1, p2);
            }
            break;
            
        case dc3d::geometry::SketchEntity::Type::Circle:
            if (entity.points.size() >= 2) {
                QPointF center = sketchToScreen(entity.points[0]);
                float radius = worldToPixels(static_cast<float>(
                    std::sqrt(std::pow(entity.points[1].x() - entity.points[0].x(), 2) +
                             std::pow(entity.points[1].y() - entity.points[0].y(), 2))));
                painter.drawEllipse(center, radius, radius);
            }
            break;
            
        case dc3d::geometry::SketchEntity::Type::Arc:
            if (entity.points.size() >= 3) {
                // Draw arc through 3 points
                QPainterPath path;
                QPointF p1 = sketchToScreen(entity.points[0]);
                QPointF p2 = sketchToScreen(entity.points[1]);
                QPointF p3 = sketchToScreen(entity.points[2]);
                
                path.moveTo(p1);
                path.quadTo(p2, p3); // Simplified - should be proper arc
                painter.drawPath(path);
            }
            break;
            
        case dc3d::geometry::SketchEntity::Type::Spline:
            if (entity.points.size() >= 2) {
                QPainterPath path;
                QPointF first = sketchToScreen(entity.points[0]);
                path.moveTo(first);
                
                if (entity.points.size() == 2) {
                    path.lineTo(sketchToScreen(entity.points[1]));
                } else {
                    // Cubic spline approximation
                    for (size_t i = 1; i < entity.points.size(); ++i) {
                        path.lineTo(sketchToScreen(entity.points[i]));
                    }
                }
                painter.drawPath(path);
            }
            break;
            
        case dc3d::geometry::SketchEntity::Type::Point:
            if (!entity.points.empty()) {
                QPointF p = sketchToScreen(entity.points[0]);
                painter.setBrush(color);
                painter.drawEllipse(p, 4, 4);
            }
            break;
            
        case dc3d::geometry::SketchEntity::Type::Rectangle:
            if (entity.points.size() >= 2) {
                QPointF p1 = sketchToScreen(entity.points[0]);
                QPointF p2 = sketchToScreen(entity.points[1]);
                painter.drawRect(QRectF(p1, p2).normalized());
            }
            break;
    }
    
    // Draw endpoints for selected entities
    if (selected && !entity.points.empty()) {
        painter.setPen(QPen(m_colors.entitySelected, 1));
        painter.setBrush(QColor(255, 255, 255));
        
        for (const auto& point : entity.points) {
            QPointF screenPoint = sketchToScreen(point);
            painter.drawEllipse(screenPoint, 4, 4);
        }
    }
}

void SketchViewport::drawConstraints(QPainter& painter)
{
    if (!m_sketchData) return;
    
    for (const auto& constraint : m_sketchData->constraints) {
        if (constraint) {
            drawConstraintIcon(painter, *constraint);
        }
    }
}

void SketchViewport::drawConstraintIcon(QPainter& painter, const dc3d::geometry::SketchConstraint& constraint)
{
    QPointF pos = sketchToScreen(constraint.displayPosition);
    
    painter.setPen(QPen(m_colors.constraint, 1));
    painter.setBrush(QColor(45, 45, 48, 200));
    
    // Draw icon background
    QRectF iconRect(pos.x() - 8, pos.y() - 8, 16, 16);
    painter.drawRoundedRect(iconRect, 3, 3);
    
    // Draw constraint symbol
    painter.setPen(QPen(m_colors.constraint, 1.5));
    
    switch (constraint.type) {
        case dc3d::geometry::SketchConstraint::Type::Horizontal:
            painter.drawLine(pos.x() - 5, pos.y(), pos.x() + 5, pos.y());
            break;
            
        case dc3d::geometry::SketchConstraint::Type::Vertical:
            painter.drawLine(pos.x(), pos.y() - 5, pos.x(), pos.y() + 5);
            break;
            
        case dc3d::geometry::SketchConstraint::Type::Perpendicular:
            painter.drawLine(pos.x() - 4, pos.y() + 4, pos.x() - 4, pos.y() - 4);
            painter.drawLine(pos.x() - 4, pos.y() + 4, pos.x() + 4, pos.y() + 4);
            break;
            
        case dc3d::geometry::SketchConstraint::Type::Parallel:
            painter.drawLine(pos.x() - 4, pos.y() - 2, pos.x() + 4, pos.y() - 2);
            painter.drawLine(pos.x() - 4, pos.y() + 2, pos.x() + 4, pos.y() + 2);
            break;
            
        case dc3d::geometry::SketchConstraint::Type::Coincident:
            painter.setBrush(m_colors.constraint);
            painter.drawEllipse(pos, 3, 3);
            break;
            
        case dc3d::geometry::SketchConstraint::Type::Tangent:
            painter.drawArc(iconRect.adjusted(2, 2, -2, -2).toRect(), 0, 180 * 16);
            break;
            
        case dc3d::geometry::SketchConstraint::Type::Equal:
            painter.drawLine(pos.x() - 4, pos.y() - 2, pos.x() + 4, pos.y() - 2);
            painter.drawLine(pos.x() - 4, pos.y() + 2, pos.x() + 4, pos.y() + 2);
            break;
            
        case dc3d::geometry::SketchConstraint::Type::Fixed:
            painter.drawRect(iconRect.adjusted(3, 3, -3, -3));
            break;
    }
}

void SketchViewport::drawDimensions(QPainter& painter)
{
    if (!m_sketchData) return;
    
    for (const auto& dimension : m_sketchData->dimensions) {
        if (dimension) {
            drawDimension(painter, *dimension);
        }
    }
}

void SketchViewport::drawDimension(QPainter& painter, const dc3d::geometry::SketchDimension& dimension)
{
    painter.setPen(QPen(m_colors.dimension, 1));
    
    QPointF startScreen = sketchToScreen(dimension.startPoint);
    QPointF endScreen = sketchToScreen(dimension.endPoint);
    QPointF textPos = sketchToScreen(dimension.textPosition);
    
    // Draw dimension lines
    painter.drawLine(startScreen, textPos);
    painter.drawLine(endScreen, textPos);
    
    // Draw extension lines
    painter.setPen(QPen(m_colors.dimension, 0.5, Qt::DashLine));
    // Extension lines would connect entities to dimension line
    
    // Draw arrowheads
    painter.setPen(QPen(m_colors.dimension, 1));
    drawArrowhead(painter, textPos, startScreen);
    drawArrowhead(painter, textPos, endScreen);
    
    // Draw dimension text
    QString text;
    switch (dimension.type) {
        case dc3d::geometry::SketchDimension::Type::Linear:
            text = QString::number(dimension.value, 'f', 2) + " mm";
            break;
        case dc3d::geometry::SketchDimension::Type::Angular:
            text = QString::number(dimension.value, 'f', 1) + "°";
            break;
        case dc3d::geometry::SketchDimension::Type::Radial:
            text = "R" + QString::number(dimension.value, 'f', 2);
            break;
        case dc3d::geometry::SketchDimension::Type::Diameter:
            text = "Ø" + QString::number(dimension.value, 'f', 2);
            break;
    }
    
    // Draw text background
    QFontMetrics fm(painter.font());
    QRectF textRect = fm.boundingRect(text);
    textRect.moveCenter(textPos);
    textRect.adjust(-3, -1, 3, 1);
    
    painter.setBrush(QColor(30, 30, 30, 220));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(textRect, 2, 2);
    
    // Draw text
    painter.setPen(m_colors.dimension);
    painter.drawText(textRect, Qt::AlignCenter, text);
}

void SketchViewport::drawSnapIndicator(QPainter& painter)
{
    QPointF screenPoint = sketchToScreen(m_snapPoint);
    
    // Draw snap marker
    painter.setPen(QPen(m_colors.snap, 2));
    painter.setBrush(Qt::NoBrush);
    
    // Different markers for different snap types
    if (m_snapType == "Endpoint") {
        // Square
        painter.drawRect(QRectF(screenPoint.x() - 6, screenPoint.y() - 6, 12, 12));
    } else if (m_snapType == "Midpoint") {
        // Triangle
        QPainterPath path;
        path.moveTo(screenPoint.x(), screenPoint.y() - 7);
        path.lineTo(screenPoint.x() + 7, screenPoint.y() + 5);
        path.lineTo(screenPoint.x() - 7, screenPoint.y() + 5);
        path.closeSubpath();
        painter.drawPath(path);
    } else if (m_snapType == "Center") {
        // Circle with cross
        painter.drawEllipse(screenPoint, 6, 6);
        painter.drawLine(screenPoint.x() - 8, screenPoint.y(), screenPoint.x() + 8, screenPoint.y());
        painter.drawLine(screenPoint.x(), screenPoint.y() - 8, screenPoint.x(), screenPoint.y() + 8);
    } else if (m_snapType == "Intersection") {
        // X mark
        painter.drawLine(screenPoint.x() - 6, screenPoint.y() - 6, 
                        screenPoint.x() + 6, screenPoint.y() + 6);
        painter.drawLine(screenPoint.x() + 6, screenPoint.y() - 6, 
                        screenPoint.x() - 6, screenPoint.y() + 6);
    } else if (m_snapType == "Grid") {
        // Small plus
        painter.drawLine(screenPoint.x() - 4, screenPoint.y(), screenPoint.x() + 4, screenPoint.y());
        painter.drawLine(screenPoint.x(), screenPoint.y() - 4, screenPoint.x(), screenPoint.y() + 4);
    } else {
        // Default: circle
        painter.drawEllipse(screenPoint, 5, 5);
    }
    
    // Draw snap type label
    if (!m_snapType.isEmpty()) {
        painter.setPen(m_colors.snap);
        painter.drawText(screenPoint.x() + 12, screenPoint.y() + 4, m_snapType);
    }
}

void SketchViewport::drawPreview(QPainter& painter)
{
    if (m_preview.type == SketchPreview::Type::None || m_preview.points.empty()) {
        return;
    }
    
    QColor color = m_preview.valid ? m_colors.preview : m_colors.previewInvalid;
    painter.setPen(QPen(color, 1.5, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    
    switch (m_preview.type) {
        case SketchPreview::Type::Line:
            if (m_preview.points.size() >= 2) {
                QPointF p1 = sketchToScreen(m_preview.points[0]);
                QPointF p2 = sketchToScreen(m_preview.points[1]);
                painter.drawLine(p1, p2);
            }
            break;
            
        case SketchPreview::Type::Circle:
            if (m_preview.points.size() >= 2) {
                QPointF center = sketchToScreen(m_preview.points[0]);
                float radius = worldToPixels(static_cast<float>(
                    std::sqrt(std::pow(m_preview.points[1].x() - m_preview.points[0].x(), 2) +
                             std::pow(m_preview.points[1].y() - m_preview.points[0].y(), 2))));
                painter.drawEllipse(center, radius, radius);
            }
            break;
            
        case SketchPreview::Type::Arc:
            if (m_preview.points.size() >= 2) {
                QPainterPath path;
                path.moveTo(sketchToScreen(m_preview.points[0]));
                for (size_t i = 1; i < m_preview.points.size(); ++i) {
                    path.lineTo(sketchToScreen(m_preview.points[i]));
                }
                painter.drawPath(path);
            }
            break;
            
        case SketchPreview::Type::Spline:
            if (m_preview.points.size() >= 2) {
                // Draw control polygon
                painter.setPen(QPen(color.darker(120), 1, Qt::DotLine));
                for (size_t i = 0; i < m_preview.points.size() - 1; ++i) {
                    painter.drawLine(sketchToScreen(m_preview.points[i]),
                                    sketchToScreen(m_preview.points[i + 1]));
                }
                
                // Draw control points
                painter.setPen(QPen(color, 1));
                painter.setBrush(color);
                for (const auto& pt : m_preview.points) {
                    painter.drawEllipse(sketchToScreen(pt), 4, 4);
                }
            }
            break;
            
        case SketchPreview::Type::Rectangle:
            if (m_preview.points.size() >= 2) {
                QPointF p1 = sketchToScreen(m_preview.points[0]);
                QPointF p2 = sketchToScreen(m_preview.points[1]);
                painter.drawRect(QRectF(p1, p2).normalized());
            }
            break;
            
        case SketchPreview::Type::Point:
            if (!m_preview.points.empty()) {
                QPointF p = sketchToScreen(m_preview.points[0]);
                painter.setBrush(color);
                painter.drawEllipse(p, 4, 4);
            }
            break;
            
        default:
            break;
    }
}

void SketchViewport::drawOriginMarker(QPainter& painter)
{
    QPointF origin = sketchToScreen(QPointF(0, 0));
    
    painter.setPen(QPen(m_colors.origin, 1));
    
    // X axis (red)
    painter.setPen(QPen(QColor(200, 60, 60), 1.5));
    painter.drawLine(origin, origin + QPointF(30, 0));
    painter.drawText(origin + QPointF(32, 4), "X");
    
    // Y axis (green)
    painter.setPen(QPen(QColor(60, 200, 60), 1.5));
    painter.drawLine(origin, origin + QPointF(0, -30));
    painter.drawText(origin + QPointF(4, -32), "Y");
    
    // Origin point
    painter.setPen(QPen(m_colors.origin, 1));
    painter.setBrush(QColor(200, 200, 200));
    painter.drawEllipse(origin, 3, 3);
}

void SketchViewport::drawStatusText(QPainter& painter)
{
    if (m_preview.statusText.isEmpty()) return;
    
    // Draw status text at bottom of viewport
    painter.setPen(QColor(200, 200, 200));
    
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(m_preview.statusText);
    textRect.moveBottomLeft(QPoint(10, height() - 10));
    textRect.adjust(-4, -2, 4, 2);
    
    // Background
    painter.setBrush(QColor(30, 30, 30, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(textRect, 3, 3);
    
    // Text
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(textRect, Qt::AlignCenter, m_preview.statusText);
}

// Helper to draw arrowhead
void drawArrowhead(QPainter& painter, const QPointF& from, const QPointF& to)
{
    const float arrowSize = 8.0f;
    
    QLineF line(from, to);
    double angle = std::atan2(-line.dy(), line.dx());
    
    QPointF arrowP1 = to + QPointF(std::sin(angle + M_PI / 3) * arrowSize,
                                   std::cos(angle + M_PI / 3) * arrowSize);
    QPointF arrowP2 = to + QPointF(std::sin(angle + M_PI - M_PI / 3) * arrowSize,
                                   std::cos(angle + M_PI - M_PI / 3) * arrowSize);
    
    QPainterPath path;
    path.moveTo(to);
    path.lineTo(arrowP1);
    path.lineTo(arrowP2);
    path.closeSubpath();
    
    painter.fillPath(path, painter.pen().color());
}

QPointF SketchViewport::sketchToScreen(const QPointF& sketchPoint) const
{
    if (!m_viewport) {
        return sketchPoint;
    }
    
    // Convert sketch 2D to world 3D
    QVector3D worldPoint(static_cast<float>(sketchPoint.x()),
                         static_cast<float>(sketchPoint.y()), 0);
    
    if (m_sketchPlane) {
        worldPoint = m_sketchPlane->origin +
                     m_sketchPlane->xAxis * static_cast<float>(sketchPoint.x()) +
                     m_sketchPlane->yAxis * static_cast<float>(sketchPoint.y());
    }
    
    // Project to screen
    QPoint screenPoint = m_viewport->worldToScreen(worldPoint);
    return QPointF(screenPoint);
}

QPointF SketchViewport::screenToSketch(const QPoint& screenPoint) const
{
    // This would use ray-plane intersection
    // Simplified for now
    return QPointF(screenPoint);
}

float SketchViewport::worldToPixels(float worldDist) const
{
    if (!m_viewport) return worldDist;
    
    // Get approximate scale from viewport
    return m_viewport->worldToPixels(worldDist);
}

uint64_t SketchViewport::hitTestEntity(const QPoint& screenPos) const
{
    if (!m_sketchData) return 0;
    
    const float hitRadius = 5.0f; // Pixels
    
    for (const auto& entity : m_sketchData->entities) {
        if (!entity) continue;
        
        // Simplified hit testing
        for (const auto& point : entity->points) {
            QPointF screenPoint = sketchToScreen(point);
            float dist = std::sqrt(std::pow(screenPoint.x() - screenPos.x(), 2) +
                                  std::pow(screenPoint.y() - screenPos.y(), 2));
            if (dist < hitRadius) {
                return entity->id;
            }
        }
    }
    
    return 0;
}

} // namespace dc
