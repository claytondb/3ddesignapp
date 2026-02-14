#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <glm/glm.hpp>

namespace dc::ui {

/**
 * @brief Widget for inputting transform values (position, rotation, scale)
 * 
 * Provides numeric input fields for precise transform editing.
 */
class TransformInputWidget : public QWidget
{
    Q_OBJECT

public:
    enum class Mode {
        Position,
        Rotation,
        Scale
    };

    explicit TransformInputWidget(Mode mode = Mode::Position, QWidget* parent = nullptr);
    ~TransformInputWidget() override = default;

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    void setValues(const glm::vec3& values);
    glm::vec3 values() const;

    void setEnabled(bool enabled);

signals:
    void valuesChanged(const glm::vec3& values);
    void editingFinished();

private slots:
    void onValueChanged();
    void onEditingFinished();

private:
    void setupUI();
    void updateLabels();

    Mode m_mode = Mode::Position;
    QDoubleSpinBox* m_spinX = nullptr;
    QDoubleSpinBox* m_spinY = nullptr;
    QDoubleSpinBox* m_spinZ = nullptr;
    QLabel* m_labelX = nullptr;
    QLabel* m_labelY = nullptr;
    QLabel* m_labelZ = nullptr;
};

} // namespace dc::ui
