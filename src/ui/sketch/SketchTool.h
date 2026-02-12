/**
 * @file SketchTool.h
 * @brief Base class for sketch drawing/editing tools
 */

#pragma once

#include <QObject>
#include <QPointF>
#include <QKeyEvent>

namespace dc {

class SketchMode;
struct SketchPreview;

/**
 * @brief Base class for all sketch tools
 * 
 * Provides common interface for sketch tools including
 * mouse handling, preview generation, and entity creation.
 */
class SketchTool : public QObject {
    Q_OBJECT
    
public:
    explicit SketchTool(SketchMode* sketchMode, QObject* parent = nullptr);
    ~SketchTool() override = default;
    
    // ---- Tool State ----
    
    /**
     * @brief Activate this tool
     */
    virtual void activate();
    
    /**
     * @brief Deactivate this tool
     */
    virtual void deactivate();
    
    /**
     * @brief Check if tool is active
     */
    bool isActive() const { return m_active; }
    
    /**
     * @brief Check if currently drawing
     */
    bool isDrawing() const { return m_drawing; }
    
    /**
     * @brief Cancel current operation
     */
    virtual void cancel();
    
    /**
     * @brief Finish current operation
     */
    virtual void finish();
    
    /**
     * @brief Reset tool state
     */
    virtual void reset();
    
    // ---- Constraints ----
    
    /**
     * @brief Check if tool supports ortho constraint (Shift for H/V)
     */
    virtual bool supportsOrthoConstraint() const { return false; }
    
    /**
     * @brief Apply orthogonal constraint to point
     * @param point Current cursor position
     * @return Constrained position
     */
    virtual QPointF applyOrthoConstraint(const QPointF& point) const;
    
    // ---- Input Handling ----
    
    /**
     * @brief Handle mouse press
     */
    virtual void handleMousePress(const QPointF& pos, Qt::MouseButtons buttons, 
                                   Qt::KeyboardModifiers modifiers) = 0;
    
    /**
     * @brief Handle mouse move
     */
    virtual void handleMouseMove(const QPointF& pos, Qt::MouseButtons buttons,
                                  Qt::KeyboardModifiers modifiers) = 0;
    
    /**
     * @brief Handle mouse release
     */
    virtual void handleMouseRelease(const QPointF& pos, Qt::MouseButtons buttons) = 0;
    
    /**
     * @brief Handle double click
     */
    virtual void handleDoubleClick(const QPointF& pos, Qt::MouseButtons buttons);
    
    /**
     * @brief Handle key press
     */
    virtual void handleKeyPress(QKeyEvent* event);
    
    /**
     * @brief Handle key release
     */
    virtual void handleKeyRelease(QKeyEvent* event);
    
    // ---- Preview ----
    
    /**
     * @brief Get current preview geometry
     */
    virtual SketchPreview getPreview() const;
    
    /**
     * @brief Get status text for current state
     */
    virtual QString getStatusText() const;

signals:
    /**
     * @brief Emitted when entity is created
     */
    void entityCreated();
    
    /**
     * @brief Emitted when preview is updated
     */
    void previewUpdated();
    
    /**
     * @brief Emitted when tool state changes
     */
    void stateChanged();

protected:
    /**
     * @brief Add entity to sketch
     */
    void addEntity(/* entity data */);
    
    /**
     * @brief Get reference point for ortho constraint
     */
    QPointF getOrthoReference() const { return m_orthoReference; }
    
    /**
     * @brief Set reference point for ortho constraint
     */
    void setOrthoReference(const QPointF& point) { m_orthoReference = point; }
    
    SketchMode* m_sketchMode;
    bool m_active = false;
    bool m_drawing = false;
    
    QPointF m_orthoReference;
};

} // namespace dc
