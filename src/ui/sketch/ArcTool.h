/**
 * @file ArcTool.h
 * @brief Arc drawing tool for 2D sketches
 * 
 * Features:
 * - 3-point arc mode (start, end, point on arc)
 * - Center-radius mode
 * - Preview during drawing
 * - Tangent arc from existing geometry
 */

#pragma once

#include "SketchTool.h"
#include <QPointF>

namespace dc {

/**
 * @brief Arc drawing mode
 */
enum class ArcMode {
    ThreePoint,     ///< Start point, end point, point on arc
    CenterRadius,   ///< Center, start point (radius), end point
    Tangent         ///< Tangent from existing line/arc
};

/**
 * @brief Drawing state for arc tool
 */
enum class ArcToolState {
    Idle,
    // Three-point mode states
    StartPoint,     ///< Waiting for start point
    EndPoint,       ///< Start placed, waiting for end point
    ArcPoint,       ///< Start & end placed, waiting for arc point
    // Center-radius mode states
    Center,         ///< Waiting for center point
    Radius,         ///< Center placed, waiting for radius point
    EndAngle        ///< Center & radius placed, waiting for end angle
};

/**
 * @brief Arc drawing tool
 * 
 * Draws arc segments with multiple input modes.
 */
class ArcTool : public SketchTool {
    Q_OBJECT
    
public:
    explicit ArcTool(SketchMode* sketchMode, QObject* parent = nullptr);
    ~ArcTool() override = default;
    
    // ---- Tool State ----
    
    void activate() override;
    void deactivate() override;
    void cancel() override;
    void finish() override;
    void reset() override;
    
    // ---- Mode ----
    
    /**
     * @brief Set arc drawing mode
     */
    void setArcMode(ArcMode mode);
    
    /**
     * @brief Get current arc mode
     */
    ArcMode arcMode() const { return m_arcMode; }
    
    /**
     * @brief Set whether to create construction geometry
     */
    void setConstructionMode(bool construction) { m_constructionMode = construction; }
    
    // ---- Input Handling ----
    
    void handleMousePress(const QPointF& pos, Qt::MouseButtons buttons,
                          Qt::KeyboardModifiers modifiers) override;
    void handleMouseMove(const QPointF& pos, Qt::MouseButtons buttons,
                         Qt::KeyboardModifiers modifiers) override;
    void handleMouseRelease(const QPointF& pos, Qt::MouseButtons buttons) override;
    void handleKeyPress(QKeyEvent* event) override;
    
    // ---- Preview ----
    
    SketchPreview getPreview() const override;
    QString getStatusText() const override;

private:
    void createArc();
    void calculateArcFromThreePoints();
    void calculateArcFromCenterRadius();
    
    ArcMode m_arcMode = ArcMode::ThreePoint;
    ArcToolState m_state = ArcToolState::Idle;
    
    // Three-point mode
    QPointF m_startPoint;
    QPointF m_endPoint;
    QPointF m_arcPoint;
    
    // Center-radius mode
    QPointF m_centerPoint;
    float m_radius = 0.0f;
    float m_startAngle = 0.0f;
    float m_endAngle = 0.0f;
    float m_sweepAngle = 0.0f;
    
    // Current cursor position
    QPointF m_currentPoint;
    
    // Calculated arc parameters
    bool m_arcValid = false;
    QPointF m_calculatedCenter;
    float m_calculatedRadius = 0.0f;
    
    // Options
    bool m_constructionMode = false;
    bool m_clockwise = false;
};

} // namespace dc
