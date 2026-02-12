/**
 * @file SplineTool.h
 * @brief Spline drawing tool for 2D sketches
 * 
 * Features:
 * - Click to add control points
 * - Double-click or Enter to finish
 * - Show control polygon
 * - Cubic B-spline interpolation
 */

#pragma once

#include "SketchTool.h"
#include <QPointF>
#include <vector>

namespace dc {

/**
 * @brief Spline type
 */
enum class SplineType {
    BSpline,        ///< Cubic B-spline (passes near control points)
    Bezier,         ///< Bezier curve (passes through endpoints)
    Interpolating   ///< Interpolating spline (passes through all points)
};

/**
 * @brief Drawing state for spline tool
 */
enum class SplineToolState {
    Idle,           ///< Waiting for first point
    Drawing,        ///< Adding control points
    Editing         ///< Editing existing spline
};

/**
 * @brief Spline drawing tool
 * 
 * Draws smooth curves through control points with
 * support for different spline types.
 */
class SplineTool : public SketchTool {
    Q_OBJECT
    
public:
    explicit SplineTool(SketchMode* sketchMode, QObject* parent = nullptr);
    ~SplineTool() override = default;
    
    // ---- Tool State ----
    
    void activate() override;
    void deactivate() override;
    void cancel() override;
    void finish() override;
    void reset() override;
    
    // ---- Settings ----
    
    /**
     * @brief Set spline type
     */
    void setSplineType(SplineType type);
    
    /**
     * @brief Get current spline type
     */
    SplineType splineType() const { return m_splineType; }
    
    /**
     * @brief Set whether to close the spline
     */
    void setClosed(bool closed) { m_closed = closed; }
    
    /**
     * @brief Check if spline will be closed
     */
    bool isClosed() const { return m_closed; }
    
    /**
     * @brief Set whether to create construction geometry
     */
    void setConstructionMode(bool construction) { m_constructionMode = construction; }
    
    /**
     * @brief Set whether to show control polygon
     */
    void setShowControlPolygon(bool show) { m_showControlPolygon = show; }
    
    // ---- Control Points ----
    
    /**
     * @brief Get number of control points
     */
    int pointCount() const { return static_cast<int>(m_controlPoints.size()); }
    
    /**
     * @brief Remove last control point
     */
    void removeLastPoint();
    
    /**
     * @brief Clear all control points
     */
    void clearPoints();
    
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
    void createSpline();
    std::vector<QPointF> evaluateSpline() const;
    std::vector<QPointF> evaluateBSpline() const;
    std::vector<QPointF> evaluateBezier() const;
    std::vector<QPointF> evaluateInterpolating() const;
    
    SplineType m_splineType = SplineType::BSpline;
    SplineToolState m_state = SplineToolState::Idle;
    
    // Control points
    std::vector<QPointF> m_controlPoints;
    QPointF m_currentPoint;
    
    // Options
    bool m_closed = false;
    bool m_constructionMode = false;
    bool m_showControlPolygon = true;
    
    // Minimum points for valid spline
    static constexpr int MIN_POINTS = 2;
};

} // namespace dc
