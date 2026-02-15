/**
 * @file MeasureTool.h
 * @brief Interactive measurement tool for CAD workflows
 * 
 * Provides measurement capabilities:
 * - Point-to-point distance
 * - Angle measurement (3 points)
 * - Radius/diameter of curved surfaces
 * - Measurement overlay with persistent annotations
 */

#pragma once

#include <QObject>
#include <QVector3D>
#include <QPoint>
#include <QSize>
#include <QColor>
#include <QPainter>
#include <memory>
#include <vector>

namespace dc3d {
namespace geometry {
class MeshData;
}
}

namespace dc {

class Viewport;

/**
 * @brief Measurement mode for the tool
 */
enum class MeasureMode {
    None,           ///< Measurement tool inactive
    Distance,       ///< Point-to-point distance (2 clicks)
    Angle,          ///< Angle measurement (3 clicks)
    Radius          ///< Radius/diameter of curved surface (1 click)
};

/**
 * @brief A single measurement result
 */
struct Measurement {
    enum class Type {
        Distance,
        Angle,
        Radius
    };
    
    Type type;
    std::vector<QVector3D> points;  ///< Click points (2 for distance, 3 for angle, 1 for radius)
    double value;                    ///< Measured value (mm for distance/radius, degrees for angle)
    double secondaryValue;           ///< Secondary value (diameter for radius)
    QColor color;                    ///< Display color
    bool visible;                    ///< Whether to show this measurement
    QString label;                   ///< Optional label
    
    Measurement()
        : type(Type::Distance)
        , value(0)
        , secondaryValue(0)
        , color(Qt::cyan)
        , visible(true)
    {}
};

/**
 * @brief Interactive measurement tool for 3D geometry
 * 
 * Usage:
 * 1. Activate tool and select measurement mode
 * 2. Click points in viewport to create measurements
 * 3. Results displayed in status bar and as overlay annotations
 */
class MeasureTool : public QObject {
    Q_OBJECT
    
public:
    explicit MeasureTool(Viewport* viewport, QObject* parent = nullptr);
    ~MeasureTool() override;
    
    // ---- Tool State ----
    
    /**
     * @brief Activate the measurement tool
     */
    void activate();
    
    /**
     * @brief Deactivate the measurement tool
     */
    void deactivate();
    
    /**
     * @brief Check if tool is active
     */
    bool isActive() const { return m_active; }
    
    // ---- Measurement Mode ----
    
    /**
     * @brief Set measurement mode
     */
    void setMode(MeasureMode mode);
    
    /**
     * @brief Get current measurement mode
     */
    MeasureMode mode() const { return m_mode; }
    
    /**
     * @brief Get mode name for display
     */
    QString modeName() const;
    
    // ---- Current Measurement ----
    
    /**
     * @brief Get number of points collected so far
     */
    int pointCount() const { return static_cast<int>(m_currentPoints.size()); }
    
    /**
     * @brief Get points needed for current mode
     */
    int pointsNeeded() const;
    
    /**
     * @brief Check if current measurement is complete
     */
    bool isComplete() const { return pointCount() >= pointsNeeded(); }
    
    /**
     * @brief Cancel current measurement and clear points
     */
    void cancelCurrent();
    
    // ---- Measurements Storage ----
    
    /**
     * @brief Get all measurements
     */
    const std::vector<Measurement>& measurements() const { return m_measurements; }
    
    /**
     * @brief Clear all measurements
     */
    void clearAllMeasurements();
    
    /**
     * @brief Clear last measurement
     */
    void clearLastMeasurement();
    
    /**
     * @brief Get measurement count
     */
    int measurementCount() const { return static_cast<int>(m_measurements.size()); }
    
    // ---- Display Settings ----
    
    /**
     * @brief Set measurement display color
     */
    void setColor(const QColor& color) { m_currentColor = color; }
    
    /**
     * @brief Get measurement display color
     */
    QColor color() const { return m_currentColor; }
    
    /**
     * @brief Set whether to show measurement labels
     */
    void setShowLabels(bool show) { m_showLabels = show; }
    
    /**
     * @brief Check if labels are shown
     */
    bool showLabels() const { return m_showLabels; }
    
    // ---- Rendering ----
    
    /**
     * @brief Render measurement overlays (OpenGL - deprecated, use paintOverlay)
     * Call this from viewport's paintGL after scene rendering
     */
    void render();
    
    /**
     * @brief Paint measurement overlays using QPainter
     * Call this from viewport's paintEvent after GL content is rendered
     * @param painter QPainter initialized on the viewport
     * @param viewportSize Size of the viewport widget
     */
    void paintOverlay(QPainter& painter, const QSize& viewportSize);
    
    /**
     * @brief Get formatted measurement string for status bar
     */
    QString statusText() const;

signals:
    /**
     * @brief Emitted when tool is activated/deactivated
     */
    void activeChanged(bool active);
    
    /**
     * @brief Emitted when measurement mode changes
     */
    void modeChanged(MeasureMode mode);
    
    /**
     * @brief Emitted when a measurement is completed
     */
    void measurementCompleted(const Measurement& measurement);
    
    /**
     * @brief Emitted when measurements are cleared
     */
    void measurementsCleared();
    
    /**
     * @brief Emitted with status text update for status bar
     */
    void statusUpdate(const QString& text);
    
    /**
     * @brief Emitted with tool hint for status bar
     */
    void toolHintUpdate(const QString& hint);

public slots:
    /**
     * @brief Handle mouse press for point collection
     * @param worldPos 3D world position of click
     * @param screenPos Screen position
     * @param button Mouse button
     */
    void handleClick(const QVector3D& worldPos, const QPoint& screenPos, Qt::MouseButton button);
    
    /**
     * @brief Handle mouse move for preview
     * @param worldPos 3D world position under cursor
     * @param screenPos Screen position
     */
    void handleMouseMove(const QVector3D& worldPos, const QPoint& screenPos);
    
    /**
     * @brief Handle key press
     * @param key Key code
     */
    void handleKeyPress(int key);

private:
    void completeDistanceMeasurement();
    void completeAngleMeasurement();
    void completeRadiusMeasurement(const QVector3D& clickPos);
    
    // Legacy OpenGL rendering (deprecated)
    void renderDistanceMeasurement(const Measurement& m);
    void renderAngleMeasurement(const Measurement& m);
    void renderRadiusMeasurement(const Measurement& m);
    void renderCurrentProgress();
    void renderPoint(const QVector3D& point, const QColor& color, float size = 8.0f);
    void renderLine(const QVector3D& p1, const QVector3D& p2, const QColor& color, float width = 2.0f);
    void renderText(const QVector3D& worldPos, const QString& text, const QColor& color);
    
    // QPainter overlay rendering
    void paintDistanceMeasurement(QPainter& painter, const QSize& viewportSize, const Measurement& m);
    void paintAngleMeasurement(QPainter& painter, const QSize& viewportSize, const Measurement& m);
    void paintRadiusMeasurement(QPainter& painter, const QSize& viewportSize, const Measurement& m);
    void paintCurrentProgress(QPainter& painter, const QSize& viewportSize);
    void paintPoint(QPainter& painter, const QPoint& screenPos, const QColor& color, float size = 8.0f);
    void paintLine(QPainter& painter, const QPoint& p1, const QPoint& p2, const QColor& color, float width = 2.0f);
    void paintText(QPainter& painter, const QPoint& screenPos, const QString& text, const QColor& color);
    void paintArc(QPainter& painter, const QPoint& vertex, const QPoint& p1, const QPoint& p2, 
                  float arcRadius, const QColor& color, float width = 2.0f);
    
    // Coordinate conversion
    QPoint worldToScreen(const QVector3D& worldPos, const QSize& viewportSize) const;
    bool isPointVisible(const QVector3D& worldPos, const QSize& viewportSize) const;
    
    double calculateDistance(const QVector3D& p1, const QVector3D& p2) const;
    double calculateAngle(const QVector3D& p1, const QVector3D& vertex, const QVector3D& p3) const;
    double estimateRadius(const QVector3D& clickPos) const;
    
    void updateToolHint();
    
    Viewport* m_viewport;
    
    bool m_active = false;
    MeasureMode m_mode = MeasureMode::Distance;
    
    // Current measurement in progress
    std::vector<QVector3D> m_currentPoints;
    QVector3D m_previewPoint;  // Cursor position for preview
    bool m_hasPreview = false;
    
    // Completed measurements
    std::vector<Measurement> m_measurements;
    
    // Display settings
    QColor m_currentColor{0, 200, 255};  // Cyan
    bool m_showLabels = true;
    float m_pointSize = 8.0f;
    float m_lineWidth = 2.0f;
};

} // namespace dc
