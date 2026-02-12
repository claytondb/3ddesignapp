/**
 * @file SketchToolbox.h
 * @brief Floating toolbox for 2D sketch tools
 * 
 * Provides a compact, floating panel with:
 * - Draw tools (Line, Arc, Circle, Spline, Rectangle)
 * - Modify tools (Trim, Extend, Offset, Mirror)
 * - Constraint tools (Horizontal, Vertical, etc.)
 * - Dimension tool
 */

#pragma once

#include <QWidget>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <map>

namespace dc {

enum class SketchToolType;

/**
 * @brief Floating toolbox for sketch mode
 * 
 * Displays all sketch tools organized by category.
 * Can be dragged and repositioned by the user.
 */
class SketchToolbox : public QWidget {
    Q_OBJECT
    
public:
    explicit SketchToolbox(QWidget* parent = nullptr);
    ~SketchToolbox() override = default;
    
    /**
     * @brief Set the currently selected tool
     * @param toolType Tool to select
     */
    void setSelectedTool(SketchToolType toolType);
    
    /**
     * @brief Get currently selected tool
     */
    SketchToolType selectedTool() const { return m_selectedTool; }
    
    /**
     * @brief Enable/disable a specific tool
     * @param toolType Tool to enable/disable
     * @param enabled Enable state
     */
    void setToolEnabled(SketchToolType toolType, bool enabled);
    
    /**
     * @brief Show/hide a tool category
     * @param category Category name
     * @param visible Visibility state
     */
    void setCategoryVisible(const QString& category, bool visible);
    
    /**
     * @brief Update tool tooltip with status
     * @param toolType Tool to update
     * @param tooltip New tooltip text
     */
    void setToolTooltip(SketchToolType toolType, const QString& tooltip);

signals:
    /**
     * @brief Emitted when a tool is selected
     */
    void toolSelected(SketchToolType toolType);
    
    /**
     * @brief Emitted when exit sketch is requested
     */
    void exitRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onToolButtonClicked(int id);
    void onExitButtonClicked();

private:
    void setupUI();
    void setupDrawTools();
    void setupModifyTools();
    void setupConstraintTools();
    void setupDimensionTools();
    void applyStylesheet();
    
    QToolButton* createToolButton(const QString& iconName, const QString& tooltip, 
                                   SketchToolType toolType);
    QWidget* createSeparator();
    QLabel* createSectionLabel(const QString& text);
    
    QVBoxLayout* m_mainLayout;
    QButtonGroup* m_toolGroup;
    
    std::map<SketchToolType, QToolButton*> m_toolButtons;
    SketchToolType m_selectedTool;
    
    // For dragging
    bool m_dragging = false;
    QPoint m_dragOffset;
    
    // Category widgets for show/hide
    std::map<QString, QWidget*> m_categories;
};

/**
 * @brief Custom tool button with checked state styling
 */
class SketchToolButton : public QToolButton {
    Q_OBJECT
    
public:
    explicit SketchToolButton(QWidget* parent = nullptr);
    
    void setToolType(SketchToolType type) { m_toolType = type; }
    SketchToolType toolType() const { return m_toolType; }
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    SketchToolType m_toolType;
    bool m_hovered = false;
};

} // namespace dc
