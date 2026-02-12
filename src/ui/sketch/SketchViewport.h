/**
 * @file SketchViewport.h
 * @brief Sketch overlay for the 3D viewport
 * 
 * Provides 2D overlay rendering for sketch mode:
 * - Grid on sketch plane
 * - Snap indicators
 * - Constraint icons
 * - Dimension display
 * - Construction geometry
 */

#pragma once

#include <QWidget>
#include <QPainter>
#include <QPointF>
#include <QVector3D>
#include <memory>
#include <vector>

namespace dc3d {
namespace geometry {
class SketchData;
class SketchPlane;
class SketchEntity;
class SketchConstraint;
class SketchDimension;
}
}

namespace dc {

class Viewport;
class SketchMode;
enum class SketchToolType;
struct SketchGridSettings;

/**
 * @brief Snap indicator type
 */
enum class SnapIndicatorType {
    None,
    Endpoint,
    Midpoint,
    Center,
    Intersection,
    Perpendicular,
    Tangent,
    Nearest,
    Grid,
    Horizontal,
    Vertical
};

/**
 * @brief Preview geometry for current tool
 */
struct SketchPreview {
    enum class Type {
        None,
        Line,
        Arc,
        Circle,
        Spline,
        Rectangle,
        Point
    };
    
    Type type = Type::None;
    std::vector<QPointF> points;
    bool valid = false;
    QString statusText;
};

/**
 * @brief 2D sketch overlay for the viewport
 * 
 * Renders sketch elements, grid, snap indicators,
 * and tool previews on top of the 3D viewport.
 */
class SketchViewport : public QWidget {
    Q_OBJECT
    
public:
    explicit SketchViewport(Viewport* viewport, QWidget* parent = nullptr);
    ~SketchViewport() override = default;
    
    // ---- Data ----
    
    /**
     * @brief Set sketch data to display
     */
    void setSketchData(std::shared_ptr<dc3d::geometry::SketchData> data);
    
    /**
     * @brief Set sketch plane
     */
    void setSketchPlane(std::shared_ptr<dc3d::geometry::SketchPlane> plane);
    
    // ---- Grid ----
    
    /**
     * @brief Set grid settings
     */
    void setGridSettings(const SketchGridSettings& settings);
    
    /**
     * @brief Show/hide grid
     */
    void setGridVisible(bool visible);
    
    /**
     * @brief Check if grid is visible
     */
    bool isGridVisible() const { return m_gridVisible; }
    
    // ---- Snap Indicator ----
    
    /**
     * @brief Set current snap indicator
     * @param point Snap point in sketch coordinates
     * @param snapType Type name for display
     */
    void setSnapIndicator(const QPointF& point, const QString& snapType);
    
    /**
     * @brief Clear snap indicator
     */
    void clearSnapIndicator();
    
    // ---- Tool State ----
    
    /**
     * @brief Set current tool for cursor styling
     */
    void setCurrentTool(SketchToolType tool);
    
    /**
     * @brief Set preview geometry
     */
    void setPreview(const SketchPreview& preview);
    
    /**
     * @brief Clear preview
     */
    void clearPreview();
    
    // ---- Selection ----
    
    /**
     * @brief Set selected entities
     */
    void setSelectedEntities(const std::vector<uint64_t>& entityIds);
    
    /**
     * @brief Set highlighted entity (hover)
     */
    void setHighlightedEntity(uint64_t entityId);
    
    /**
     * @brief Clear highlight
     */
    void clearHighlight();
    
    // ---- Display Options ----
    
    /**
     * @brief Show/hide constraints
     */
    void setConstraintsVisible(bool visible);
    
    /**
     * @brief Show/hide dimensions
     */
    void setDimensionsVisible(bool visible);
    
    /**
     * @brief Show/hide construction geometry
     */
    void setConstructionVisible(bool visible);

signals:
    /**
     * @brief Emitted when entity is clicked
     */
    void entityClicked(uint64_t entityId, Qt::MouseButtons buttons);
    
    /**
     * @brief Emitted when entity is hovered
     */
    void entityHovered(uint64_t entityId);
    
    /**
     * @brief Emitted when dimension is double-clicked for editing
     */
    void dimensionEditRequested(uint64_t dimensionId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // Drawing methods
    void drawGrid(QPainter& painter);
    void drawEntities(QPainter& painter);
    void drawEntity(QPainter& painter, const dc3d::geometry::SketchEntity& entity, 
                    bool selected, bool highlighted);
    void drawConstraints(QPainter& painter);
    void drawConstraintIcon(QPainter& painter, const dc3d::geometry::SketchConstraint& constraint);
    void drawDimensions(QPainter& painter);
    void drawDimension(QPainter& painter, const dc3d::geometry::SketchDimension& dimension);
    void drawSnapIndicator(QPainter& painter);
    void drawPreview(QPainter& painter);
    void drawOriginMarker(QPainter& painter);
    void drawStatusText(QPainter& painter);
    
    // Coordinate conversion
    QPointF sketchToScreen(const QPointF& sketchPoint) const;
    QPointF screenToSketch(const QPoint& screenPoint) const;
    float worldToPixels(float worldDist) const;
    
    // Hit testing
    uint64_t hitTestEntity(const QPoint& screenPos) const;
    
    Viewport* m_viewport;
    
    // Data
    std::shared_ptr<dc3d::geometry::SketchData> m_sketchData;
    std::shared_ptr<dc3d::geometry::SketchPlane> m_sketchPlane;
    
    // Grid
    SketchGridSettings m_gridSettings;
    bool m_gridVisible = true;
    
    // Snap indicator
    QPointF m_snapPoint;
    QString m_snapType;
    bool m_hasSnapIndicator = false;
    
    // Tool state
    SketchToolType m_currentTool;
    SketchPreview m_preview;
    
    // Selection
    std::vector<uint64_t> m_selectedEntities;
    uint64_t m_highlightedEntity = 0;
    
    // Display options
    bool m_constraintsVisible = true;
    bool m_dimensionsVisible = true;
    bool m_constructionVisible = true;
    
    // Colors
    struct Colors {
        QColor gridMajor{100, 100, 100, 180};
        QColor gridMinor{60, 60, 60, 100};
        QColor entity{200, 200, 200};
        QColor entitySelected{0, 122, 204};
        QColor entityHighlight{255, 200, 0};
        QColor construction{100, 100, 100};
        QColor preview{0, 200, 100, 180};
        QColor previewInvalid{200, 50, 50, 180};
        QColor dimension{0, 200, 255};
        QColor constraint{255, 150, 0};
        QColor snap{255, 100, 100};
        QColor origin{150, 150, 150};
    } m_colors;
};

} // namespace dc
