/**
 * @file DimensionTool.h
 * @brief Dimension tool for 2D sketches
 * 
 * Features:
 * - Click entity to dimension
 * - Drag to position dimension text
 * - Edit value to drive geometry
 * - Linear, angular, radial, diameter dimensions
 */

#pragma once

#include "SketchTool.h"
#include <QPointF>
#include <vector>

namespace dc {

/**
 * @brief Dimension type
 */
enum class DimensionType {
    Auto,       ///< Automatically determine type
    Linear,     ///< Distance between two points
    Horizontal, ///< Horizontal distance
    Vertical,   ///< Vertical distance
    Angular,    ///< Angle between lines
    Radial,     ///< Radius of circle/arc
    Diameter    ///< Diameter of circle
};

/**
 * @brief Drawing state for dimension tool
 */
enum class DimensionToolState {
    Idle,               ///< Waiting for first selection
    FirstEntity,        ///< First entity selected
    SecondEntity,       ///< Second entity selected (for 2-entity dims)
    PositioningText     ///< Positioning dimension text
};

/**
 * @brief Selected entity info
 */
struct DimensionEntitySelection {
    uint64_t entityId = 0;
    QPointF point;          ///< Click point on entity
    QString entityType;     ///< "line", "circle", "arc", "point"
};

/**
 * @brief Dimension tool
 * 
 * Creates and edits parametric dimensions that
 * can drive sketch geometry.
 */
class DimensionTool : public SketchTool {
    Q_OBJECT
    
public:
    explicit DimensionTool(SketchMode* sketchMode, QObject* parent = nullptr);
    ~DimensionTool() override = default;
    
    // ---- Tool State ----
    
    void activate() override;
    void deactivate() override;
    void cancel() override;
    void finish() override;
    void reset() override;
    
    // ---- Settings ----
    
    /**
     * @brief Set dimension type
     */
    void setDimensionType(DimensionType type);
    
    /**
     * @brief Get current dimension type
     */
    DimensionType dimensionType() const { return m_dimensionType; }
    
    /**
     * @brief Set whether dimension is reference only
     */
    void setReferenceMode(bool reference) { m_referenceMode = reference; }
    
    /**
     * @brief Check if creating reference dimensions
     */
    bool isReferenceMode() const { return m_referenceMode; }
    
    // ---- Input Handling ----
    
    void handleMousePress(const QPointF& pos, Qt::MouseButtons buttons,
                          Qt::KeyboardModifiers modifiers) override;
    void handleMouseMove(const QPointF& pos, Qt::MouseButtons buttons,
                         Qt::KeyboardModifiers modifiers) override;
    void handleMouseRelease(const QPointF& pos, Qt::MouseButtons buttons) override;
    void handleDoubleClick(const QPointF& pos, Qt::MouseButtons buttons) override;
    void handleKeyPress(QKeyEvent* event) override;
    
    // ---- Preview ----
    
    SketchPreview getPreview() const override;
    QString getStatusText() const override;

signals:
    /**
     * @brief Emitted when dimension value should be edited
     */
    void editDimensionRequested(uint64_t dimensionId);

private:
    void createDimension();
    DimensionType determineDimensionType() const;
    double calculateDimensionValue() const;
    void calculateDimensionGeometry();
    
    // Hit testing
    DimensionEntitySelection hitTestEntity(const QPointF& pos) const;
    
    DimensionType m_dimensionType = DimensionType::Auto;
    DimensionToolState m_state = DimensionToolState::Idle;
    
    // Selected entities
    DimensionEntitySelection m_firstSelection;
    DimensionEntitySelection m_secondSelection;
    
    // Current cursor and text position
    QPointF m_currentPoint;
    QPointF m_textPosition;
    
    // Calculated dimension
    double m_dimensionValue = 0.0;
    QPointF m_startPoint;   ///< Dimension line start
    QPointF m_endPoint;     ///< Dimension line end
    
    // Options
    bool m_referenceMode = false;
    bool m_isDraggingText = false;
};

} // namespace dc
