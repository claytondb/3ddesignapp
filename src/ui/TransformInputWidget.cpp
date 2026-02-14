#include "TransformInputWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace dc::ui {

TransformInputWidget::TransformInputWidget(Mode mode, QWidget* parent)
    : QWidget(parent)
    , m_mode(mode)
{
    setupUI();
    updateLabels();
}

void TransformInputWidget::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // X
    m_labelX = new QLabel("X:", this);
    m_labelX->setStyleSheet("color: #ff6b6b; font-weight: bold;");
    m_spinX = new QDoubleSpinBox(this);
    m_spinX->setRange(-99999.0, 99999.0);
    m_spinX->setDecimals(3);
    m_spinX->setSingleStep(0.1);

    // Y
    m_labelY = new QLabel("Y:", this);
    m_labelY->setStyleSheet("color: #69db7c; font-weight: bold;");
    m_spinY = new QDoubleSpinBox(this);
    m_spinY->setRange(-99999.0, 99999.0);
    m_spinY->setDecimals(3);
    m_spinY->setSingleStep(0.1);

    // Z
    m_labelZ = new QLabel("Z:", this);
    m_labelZ->setStyleSheet("color: #74c0fc; font-weight: bold;");
    m_spinZ = new QDoubleSpinBox(this);
    m_spinZ->setRange(-99999.0, 99999.0);
    m_spinZ->setDecimals(3);
    m_spinZ->setSingleStep(0.1);

    layout->addWidget(m_labelX);
    layout->addWidget(m_spinX);
    layout->addWidget(m_labelY);
    layout->addWidget(m_spinY);
    layout->addWidget(m_labelZ);
    layout->addWidget(m_spinZ);
    layout->addStretch();

    // Connect signals
    connect(m_spinX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TransformInputWidget::onValueChanged);
    connect(m_spinY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TransformInputWidget::onValueChanged);
    connect(m_spinZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TransformInputWidget::onValueChanged);

    connect(m_spinX, &QDoubleSpinBox::editingFinished,
            this, &TransformInputWidget::onEditingFinished);
    connect(m_spinY, &QDoubleSpinBox::editingFinished,
            this, &TransformInputWidget::onEditingFinished);
    connect(m_spinZ, &QDoubleSpinBox::editingFinished,
            this, &TransformInputWidget::onEditingFinished);
}

void TransformInputWidget::setMode(Mode mode)
{
    m_mode = mode;
    updateLabels();
}

void TransformInputWidget::updateLabels()
{
    switch (m_mode) {
        case Mode::Position:
            m_spinX->setSuffix(" mm");
            m_spinY->setSuffix(" mm");
            m_spinZ->setSuffix(" mm");
            break;
        case Mode::Rotation:
            m_spinX->setSuffix("°");
            m_spinY->setSuffix("°");
            m_spinZ->setSuffix("°");
            m_spinX->setRange(-360.0, 360.0);
            m_spinY->setRange(-360.0, 360.0);
            m_spinZ->setRange(-360.0, 360.0);
            break;
        case Mode::Scale:
            m_spinX->setSuffix("");
            m_spinY->setSuffix("");
            m_spinZ->setSuffix("");
            m_spinX->setRange(0.001, 1000.0);
            m_spinY->setRange(0.001, 1000.0);
            m_spinZ->setRange(0.001, 1000.0);
            m_spinX->setValue(1.0);
            m_spinY->setValue(1.0);
            m_spinZ->setValue(1.0);
            break;
    }
}

void TransformInputWidget::setValues(const glm::vec3& values)
{
    m_spinX->blockSignals(true);
    m_spinY->blockSignals(true);
    m_spinZ->blockSignals(true);

    m_spinX->setValue(values.x);
    m_spinY->setValue(values.y);
    m_spinZ->setValue(values.z);

    m_spinX->blockSignals(false);
    m_spinY->blockSignals(false);
    m_spinZ->blockSignals(false);
}

glm::vec3 TransformInputWidget::values() const
{
    return glm::vec3(m_spinX->value(), m_spinY->value(), m_spinZ->value());
}

void TransformInputWidget::setEnabled(bool enabled)
{
    m_spinX->setEnabled(enabled);
    m_spinY->setEnabled(enabled);
    m_spinZ->setEnabled(enabled);
}

void TransformInputWidget::onValueChanged()
{
    emit valuesChanged(values());
}

void TransformInputWidget::onEditingFinished()
{
    emit editingFinished();
}

} // namespace dc::ui
