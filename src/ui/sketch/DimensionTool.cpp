/**
 * @file DimensionTool.cpp
 * @brief Implementation of dimension tool
 */

#include "DimensionTool.h"
#include "SketchMode.h"
#include "SketchViewport.h"

#include <QKeyEvent>
#include <cmath>

namespace dc {

DimensionTool::DimensionTool(SketchMode* sketchMode, QObject* parent)
    : SketchTool(sketchMode, parent)
{
}

void DimensionTool::activate()
{
    SketchTool::activate();
    m_state = DimensionToolState::Idle;
}

void DimensionTool::deactivate()
{
    SketchTool::deactivate();
    m_state = DimensionToolState::Idle;
}

void DimensionTool::cancel()
{
    m_state = DimensionToolState::Idle;
    m_drawing = false;
    m_firstSelection = DimensionEntitySelection();
    m_secondSelection = DimensionEntitySelection();
    
    emit previewUpdated();
    emit stateChanged();
}

void DimensionTool::finish()
{
    if (m_state == DimensionToolState::PositioningText) {
        createDimension();
    }
    
    m_state = DimensionToolState::Idle;
    m_drawing = false;
    m_firstSelection = DimensionEntitySelection();
    m_secondSelection = DimensionEntitySelection();
    
    emit previewUpdated();
    emit stateChanged();
}

void DimensionTool::reset()
{
    SketchTool::reset();
    m_state = DimensionToolState::Idle;
    m_firstSelection = DimensionEntitySelection();
    m_secondSelection = DimensionEntitySelection();
    m_textPosition = QPointF();
    m_dimensionValue = 0.0;
}

void DimensionTool::setDimensionType(DimensionType type)
{
    m_dimensionType = type;
    emit stateChanged();
}

void DimensionTool::handleMousePress(const QPointF& pos, Qt::MouseButtons buttons,
                                      Qt::KeyboardModifiers /*modifiers*/)
{
    if (!(buttons & Qt::LeftButton)) {
        return;
    }
    
    switch (m_state) {
        case DimensionToolState::Idle: {
            // Try to select an entity
            DimensionEntitySelection selection = hitTestEntity(pos);
            
            if (selection.entityId != 0) {
                m_firstSelection = selection;
                m_state = DimensionToolState::FirstEntity;
                m_drawing = true;
                
                // For single-entity dimensions (radius, diameter), 
                // go directly to text positioning
                if (selection.entityType == "circle" || selection.entityType == "arc") {
                    if (m_dimensionType == DimensionType::Auto ||
                        m_dimensionType == DimensionType::Radial ||
                        m_dimensionType == DimensionType::Diameter) {
                        m_state = DimensionToolState::PositioningText;
                        calculateDimensionGeometry();
                    }
                }
            } else {
                // Click on empty space - use as first point for point-to-point dimension
                m_firstSelection.entityId = 0;
                m_firstSelection.point = pos;
                m_firstSelection.entityType = "point";
                m_state = DimensionToolState::FirstEntity;
                m_drawing = true;
            }
            break;
        }
        
        case DimensionToolState::FirstEntity: {
            // Select second entity or point
            DimensionEntitySelection selection = hitTestEntity(pos);
            
            if (selection.entityId != 0) {
                m_secondSelection = selection;
            } else {
                m_secondSelection.entityId = 0;
                m_secondSelection.point = pos;
                m_secondSelection.entityType = "point";
            }
            
            m_state = DimensionToolState::SecondEntity;
            
            // Calculate dimension and go to text positioning
            m_state = DimensionToolState::PositioningText;
            calculateDimensionGeometry();
            break;
        }
        
        case DimensionToolState::SecondEntity:
            // Fall through to text positioning
            m_state = DimensionToolState::PositioningText;
            calculateDimensionGeometry();
            break;
            
        case DimensionToolState::PositioningText:
            // Place dimension text
            m_textPosition = pos;
            createDimension();
            
            // Reset for next dimension
            m_state = DimensionToolState::Idle;
            m_drawing = false;
            m_firstSelection = DimensionEntitySelection();
            m_secondSelection = DimensionEntitySelection();
            break;
    }
    
    emit previewUpdated();
    emit stateChanged();
}

void DimensionTool::handleMouseMove(const QPointF& pos, Qt::MouseButtons /*buttons*/,
                                     Qt::KeyboardModifiers /*modifiers*/)
{
    m_currentPoint = pos;
    
    if (m_state == DimensionToolState::PositioningText) {
        m_textPosition = pos;
    }
    
    emit previewUpdated();
}

void DimensionTool::handleMouseRelease(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Dimension tool uses click-click
}

void DimensionTool::handleDoubleClick(const QPointF& /*pos*/, Qt::MouseButtons /*buttons*/)
{
    // Double-click on existing dimension could edit it
    // For now, just finish current operation
    if (m_state == DimensionToolState::PositioningText) {
        finish();
    }
}

void DimensionTool::handleKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_H:
            setDimensionType(DimensionType::Horizontal);
            event->accept();
            break;
            
        case Qt::Key_V:
            setDimensionType(DimensionType::Vertical);
            event->accept();
            break;
            
        case Qt::Key_A:
            setDimensionType(DimensionType::Angular);
            event->accept();
            break;
            
        case Qt::Key_R:
            setDimensionType(DimensionType::Radial);
            event->accept();
            break;
            
        case Qt::Key_D:
            setDimensionType(DimensionType::Diameter);
            event->accept();
            break;
            
        case Qt::Key_L:
            setDimensionType(DimensionType::Linear);
            event->accept();
            break;
            
        case Qt::Key_F:
            // Toggle reference mode
            m_referenceMode = !m_referenceMode;
            emit stateChanged();
            event->accept();
            break;
            
        default:
            SketchTool::handleKeyPress(event);
            break;
    }
}

SketchPreview DimensionTool::getPreview() const
{
    SketchPreview preview;
    
    if (m_state == DimensionToolState::Idle) {
        return preview;
    }
    
    // Show dimension preview
    preview.type = SketchPreview::Type::Line;
    
    switch (m_state) {
        case DimensionToolState::FirstEntity:
            // Show line from first point to cursor
            preview.points = {m_firstSelection.point, m_currentPoint};
            preview.valid = true;
            break;
            
        case DimensionToolState::SecondEntity:
        case DimensionToolState::PositioningText:
            // Show dimension line
            preview.points = {m_startPoint, m_endPoint};
            if (!m_textPosition.isNull()) {
                // Add extension lines to text
                preview.points.push_back(m_textPosition);
            }
            preview.valid = (m_dimensionValue > 0.001);
            break;
            
        default:
            break;
    }
    
    // Status text with value
    QString typeStr;
    switch (determineDimensionType()) {
        case DimensionType::Linear: typeStr = "Linear"; break;
        case DimensionType::Horizontal: typeStr = "Horizontal"; break;
        case DimensionType::Vertical: typeStr = "Vertical"; break;
        case DimensionType::Angular: typeStr = "Angular"; break;
        case DimensionType::Radial: typeStr = "Radial"; break;
        case DimensionType::Diameter: typeStr = "Diameter"; break;
        default: typeStr = "Dimension"; break;
    }
    
    if (m_state == DimensionToolState::PositioningText) {
        if (determineDimensionType() == DimensionType::Angular) {
            preview.statusText = QString("%1: %2°%3")
                .arg(typeStr)
                .arg(m_dimensionValue, 0, 'f', 2)
                .arg(m_referenceMode ? " (Ref)" : "");
        } else {
            preview.statusText = QString("%1: %2 mm%3")
                .arg(typeStr)
                .arg(m_dimensionValue, 0, 'f', 3)
                .arg(m_referenceMode ? " (Ref)" : "");
        }
    }
    
    return preview;
}

QString DimensionTool::getStatusText() const
{
    QString typeStr;
    switch (m_dimensionType) {
        case DimensionType::Auto: typeStr = "Auto"; break;
        case DimensionType::Linear: typeStr = "Linear"; break;
        case DimensionType::Horizontal: typeStr = "Horizontal"; break;
        case DimensionType::Vertical: typeStr = "Vertical"; break;
        case DimensionType::Angular: typeStr = "Angular"; break;
        case DimensionType::Radial: typeStr = "Radial"; break;
        case DimensionType::Diameter: typeStr = "Diameter"; break;
    }
    
    switch (m_state) {
        case DimensionToolState::Idle:
            return QString("[%1] Click entity or point to dimension").arg(typeStr);
            
        case DimensionToolState::FirstEntity:
            return QString("[%1] Click second entity/point").arg(typeStr);
            
        case DimensionToolState::SecondEntity:
        case DimensionToolState::PositioningText:
            return QString("[%1] Click to position dimension text").arg(typeStr);
            
        default:
            return QString();
    }
}

void DimensionTool::createDimension()
{
    if (m_dimensionValue < 0.001 && determineDimensionType() != DimensionType::Angular) {
        return;
    }
    
    // Create dimension entity
    // In a real implementation, this would create a SketchDimension
    
    /*
    auto dimension = std::make_shared<SketchDimension>();
    dimension->type = determineDimensionType();
    dimension->value = m_dimensionValue;
    dimension->textPosition = m_textPosition;
    dimension->startPoint = m_startPoint;
    dimension->endPoint = m_endPoint;
    dimension->isReference = m_referenceMode;
    
    if (m_firstSelection.entityId != 0) {
        dimension->entityIds.push_back(m_firstSelection.entityId);
    }
    if (m_secondSelection.entityId != 0) {
        dimension->entityIds.push_back(m_secondSelection.entityId);
    }
    
    m_sketchMode->sketchData()->addDimension(dimension);
    */
    
    emit entityCreated();
}

DimensionType DimensionTool::determineDimensionType() const
{
    if (m_dimensionType != DimensionType::Auto) {
        return m_dimensionType;
    }
    
    // Auto-detect based on selection
    if (m_firstSelection.entityType == "circle") {
        return DimensionType::Diameter;
    }
    
    if (m_firstSelection.entityType == "arc") {
        return DimensionType::Radial;
    }
    
    // Two lines = angular
    if (m_firstSelection.entityType == "line" && m_secondSelection.entityType == "line") {
        return DimensionType::Angular;
    }
    
    // Default to linear
    return DimensionType::Linear;
}

double DimensionTool::calculateDimensionValue() const
{
    DimensionType type = determineDimensionType();
    
    switch (type) {
        case DimensionType::Linear:
        case DimensionType::Auto: {
            // Distance between two points
            float dx = m_secondSelection.point.x() - m_firstSelection.point.x();
            float dy = m_secondSelection.point.y() - m_firstSelection.point.y();
            return std::sqrt(dx * dx + dy * dy);
        }
        
        case DimensionType::Horizontal: {
            return std::abs(m_secondSelection.point.x() - m_firstSelection.point.x());
        }
        
        case DimensionType::Vertical: {
            return std::abs(m_secondSelection.point.y() - m_firstSelection.point.y());
        }
        
        case DimensionType::Angular: {
            // Angle between two lines
            // Simplified - would use actual line vectors
            float dx1 = 1.0f, dy1 = 0.0f;  // First line direction
            float dx2 = 0.0f, dy2 = 1.0f;  // Second line direction
            
            float dot = dx1 * dx2 + dy1 * dy2;
            float mag1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
            float mag2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
            
            float cosAngle = dot / (mag1 * mag2);
            return std::acos(std::clamp(cosAngle, -1.0f, 1.0f)) * 180.0 / M_PI;
        }
        
        case DimensionType::Radial:
        case DimensionType::Diameter: {
            // Would get radius from circle/arc entity
            // Simplified - calculate from selection point
            return 10.0;  // Placeholder
        }
    }
    
    return 0.0;
}

void DimensionTool::calculateDimensionGeometry()
{
    m_dimensionValue = calculateDimensionValue();
    
    DimensionType type = determineDimensionType();
    
    switch (type) {
        case DimensionType::Linear:
        case DimensionType::Auto:
            m_startPoint = m_firstSelection.point;
            m_endPoint = m_secondSelection.point;
            break;
            
        case DimensionType::Horizontal:
            m_startPoint = m_firstSelection.point;
            m_endPoint = QPointF(m_secondSelection.point.x(), m_firstSelection.point.y());
            break;
            
        case DimensionType::Vertical:
            m_startPoint = m_firstSelection.point;
            m_endPoint = QPointF(m_firstSelection.point.x(), m_secondSelection.point.y());
            break;
            
        case DimensionType::Radial:
        case DimensionType::Diameter:
            // For circles, start = center, end = point on circle
            m_startPoint = m_firstSelection.point;
            m_endPoint = m_firstSelection.point + QPointF(m_dimensionValue, 0);
            break;
            
        case DimensionType::Angular:
            // Vertex point
            m_startPoint = m_firstSelection.point;
            m_endPoint = m_secondSelection.point;
            break;
    }
    
    // Default text position at midpoint
    if (m_textPosition.isNull()) {
        m_textPosition = QPointF(
            (m_startPoint.x() + m_endPoint.x()) / 2,
            (m_startPoint.y() + m_endPoint.y()) / 2
        );
        
        // Offset from dimension line
        float dx = m_endPoint.x() - m_startPoint.x();
        float dy = m_endPoint.y() - m_startPoint.y();
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.001f) {
            // Perpendicular offset
            m_textPosition.rx() -= dy / len * 15.0f;
            m_textPosition.ry() += dx / len * 15.0f;
        }
    }
}

DimensionEntitySelection DimensionTool::hitTestEntity(const QPointF& pos) const
{
    DimensionEntitySelection result;
    result.point = pos;
    
    // Hit test against sketch entities
    // In a real implementation, this would check against actual entities
    
    /*
    if (m_sketchMode && m_sketchMode->sketchData()) {
        for (const auto& entity : m_sketchMode->sketchData()->entities) {
            if (entity->hitTest(pos, 5.0f)) {
                result.entityId = entity->id;
                result.entityType = entity->typeName();
                result.point = entity->nearestPoint(pos);
                return result;
            }
        }
    }
    */
    
    return result;
}

} // namespace dc
