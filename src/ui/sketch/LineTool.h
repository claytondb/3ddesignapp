/**
 * @file LineTool.h
 * @brief Line drawing tool for 2D sketches
 * 
 * Features:
 * - Click first point, click second point
 * - Rubber-band preview
 * - Snap to existing points/lines
 * - Shift for horizontal/vertical constraint
 * - Chain drawing mode (continue from last endpoint)
 */

#pragma once

#include "SketchTool.h"
#include <QPointF>
#include <vector>

namespace dc {

/**
 * @brief Drawing state for line tool
 */
enum class LineToolState {
    Idle,           ///< Waiting for first click
    FirstPoint,     ///< First point placed, waiting for second
    ChainMode       ///< Chaining lines from previous endpoint
};

/**
 * @brief Line drawing tool
 * 
 * Draws line segments with support for:
 * - Single line creation
 * - Chained polyline creation
 * - Horizontal/vertical constraints
 * - Point snapping
 */
class LineTool : public SketchTool {
    Q_OBJECT
    
public:
    explicit LineTool(SketchMode* sketchMode, QObject* parent = nullptr);
    ~LineTool() override = default;
    
    // ---- Tool State ----
    
    void activate() override;
    void deactivate() override;
    void cancel() override;
    void finish() override;
    void reset() override;
    
    // ---- Constraints ----
    
    bool supportsOrthoConstraint() const override { return true; }
    QPointF applyOrthoConstraint(const QPointF& point) const override;
    
    // ---- Settings ----
    
    /**
     * @brief Enable/disable chain mode
     * @param enabled If true, continue drawing from last endpoint
     */
    void setChainMode(bool enabled) { m_chainEnabled = enabled; }
    
    /**
     * @brief Check if chain mode is enabled
     */
    bool isChainMode() const { return m_chainEnabled; }
    
    /**
     * @brief Set whether to create construction geometry
     */
    void setConstructionMode(bool construction) { m_constructionMode = construction; }
    
    /**
     * @brief Check if creating construction geometry
     */
    bool isConstructionMode() const { return m_constructionMode; }
    
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

private:
    void createLine();
    void startNewLine(const QPointF& point);
    
    LineToolState m_state = LineToolState::Idle;
    
    // Points
    QPointF m_startPoint;
    QPointF m_currentPoint;
    
    // Options
    bool m_chainEnabled = true;
    bool m_constructionMode = false;
    bool m_orthoConstrained = false;
    
    // Line properties
    float m_length = 0.0f;
    float m_angle = 0.0f;
};

} // namespace dc
