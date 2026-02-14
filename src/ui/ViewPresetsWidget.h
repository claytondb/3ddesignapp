/**
 * @file ViewPresetsWidget.h
 * @brief Quick access view preset buttons for the viewport
 * 
 * A small, semi-transparent toolbar that floats in the corner of the viewport
 * providing quick access to standard views (Top, Front, Right, Isometric).
 */

#pragma once

#include <QWidget>
#include <QToolButton>
#include <QHBoxLayout>

namespace dc {

class Viewport;

/**
 * @brief Floating toolbar with view preset buttons
 * 
 * Provides quick access to standard orthographic views and isometric view.
 * Styled to be semi-transparent and non-intrusive.
 */
class ViewPresetsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ViewPresetsWidget(Viewport* viewport, QWidget* parent = nullptr);
    ~ViewPresetsWidget() override = default;

signals:
    void viewChanged(const QString& viewName);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    QToolButton* createViewButton(const QString& text, const QString& tooltip, 
                                   const QString& viewName);
    void applyStyle();

    Viewport* m_viewport;
    QHBoxLayout* m_layout;
    
    QToolButton* m_btnTop;
    QToolButton* m_btnFront;
    QToolButton* m_btnRight;
    QToolButton* m_btnIso;
    QToolButton* m_btnPersp;  // Toggle perspective/ortho
};

} // namespace dc
