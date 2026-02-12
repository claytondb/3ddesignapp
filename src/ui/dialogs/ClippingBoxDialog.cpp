#include "ClippingBoxDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <cmath>

ClippingBoxDialog::ClippingBoxDialog(QWidget *parent)
    : QDialog(parent)
    , m_viewport(nullptr)
{
    setWindowTitle(tr("Clipping Box"));
    setMinimumWidth(360);
    setModal(false);  // Non-modal to allow viewport interaction
    
    // Initialize bounds to zero
    m_meshBounds = {0, 0, 0, 0, 0, 0};
    
    setupUI();
    setupConnections();
    applyStylesheet();
}

void ClippingBoxDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Min bounds group
    QGroupBox* minGroup = new QGroupBox(tr("Minimum"));
    QGridLayout* minLayout = new QGridLayout(minGroup);
    minLayout->setSpacing(8);
    
    // Labels
    QLabel* minXLabel = new QLabel("X:");
    minXLabel->setObjectName("axisLabelX");
    minLayout->addWidget(minXLabel, 0, 0);
    
    QLabel* minYLabel = new QLabel("Y:");
    minYLabel->setObjectName("axisLabelY");
    minLayout->addWidget(minYLabel, 1, 0);
    
    QLabel* minZLabel = new QLabel("Z:");
    minZLabel->setObjectName("axisLabelZ");
    minLayout->addWidget(minZLabel, 2, 0);

    // Spinboxes
    m_minXSpinbox = new QDoubleSpinBox();
    m_minXSpinbox->setRange(-10000, 10000);
    m_minXSpinbox->setDecimals(3);
    m_minXSpinbox->setSuffix(" mm");
    minLayout->addWidget(m_minXSpinbox, 0, 1);

    m_minYSpinbox = new QDoubleSpinBox();
    m_minYSpinbox->setRange(-10000, 10000);
    m_minYSpinbox->setDecimals(3);
    m_minYSpinbox->setSuffix(" mm");
    minLayout->addWidget(m_minYSpinbox, 1, 1);

    m_minZSpinbox = new QDoubleSpinBox();
    m_minZSpinbox->setRange(-10000, 10000);
    m_minZSpinbox->setDecimals(3);
    m_minZSpinbox->setSuffix(" mm");
    minLayout->addWidget(m_minZSpinbox, 2, 1);

    mainLayout->addWidget(minGroup);

    // Max bounds group
    QGroupBox* maxGroup = new QGroupBox(tr("Maximum"));
    QGridLayout* maxLayout = new QGridLayout(maxGroup);
    maxLayout->setSpacing(8);
    
    // Labels
    QLabel* maxXLabel = new QLabel("X:");
    maxXLabel->setObjectName("axisLabelX");
    maxLayout->addWidget(maxXLabel, 0, 0);
    
    QLabel* maxYLabel = new QLabel("Y:");
    maxYLabel->setObjectName("axisLabelY");
    maxLayout->addWidget(maxYLabel, 1, 0);
    
    QLabel* maxZLabel = new QLabel("Z:");
    maxZLabel->setObjectName("axisLabelZ");
    maxLayout->addWidget(maxZLabel, 2, 0);

    // Spinboxes
    m_maxXSpinbox = new QDoubleSpinBox();
    m_maxXSpinbox->setRange(-10000, 10000);
    m_maxXSpinbox->setDecimals(3);
    m_maxXSpinbox->setSuffix(" mm");
    maxLayout->addWidget(m_maxXSpinbox, 0, 1);

    m_maxYSpinbox = new QDoubleSpinBox();
    m_maxYSpinbox->setRange(-10000, 10000);
    m_maxYSpinbox->setDecimals(3);
    m_maxYSpinbox->setSuffix(" mm");
    maxLayout->addWidget(m_maxYSpinbox, 1, 1);

    m_maxZSpinbox = new QDoubleSpinBox();
    m_maxZSpinbox->setRange(-10000, 10000);
    m_maxZSpinbox->setDecimals(3);
    m_maxZSpinbox->setSuffix(" mm");
    maxLayout->addWidget(m_maxZSpinbox, 2, 1);

    mainLayout->addWidget(maxGroup);

    // Size info
    m_sizeLabel = new QLabel(tr("Size: 0.000 × 0.000 × 0.000 mm"));
    m_sizeLabel->setObjectName("infoLabel");
    m_sizeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_sizeLabel);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"));
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    optionsLayout->setSpacing(8);

    m_invertCheck = new QCheckBox(tr("Invert selection (keep inside)"));
    optionsLayout->addWidget(m_invertCheck);

    m_previewCheck = new QCheckBox(tr("Show preview in viewport"));
    m_previewCheck->setChecked(true);
    optionsLayout->addWidget(m_previewCheck);

    mainLayout->addWidget(optionsGroup);

    mainLayout->addStretch();

    // Separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setObjectName("dialogSeparator");
    mainLayout->addWidget(separator);

    // Button box
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton(tr("Reset"));
    m_resetButton->setObjectName("smallButton");
    buttonLayout->addWidget(m_resetButton);
    
    buttonLayout->addStretch();
    
    m_deleteInsideButton = new QPushButton(tr("Delete Inside"));
    m_deleteInsideButton->setObjectName("warningButton");
    buttonLayout->addWidget(m_deleteInsideButton);
    
    m_applyButton = new QPushButton(tr("Delete Outside"));
    m_applyButton->setObjectName("primaryButton");
    buttonLayout->addWidget(m_applyButton);
    
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void ClippingBoxDialog::setupConnections()
{
    // Min bounds
    connect(m_minXSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ClippingBoxDialog::onMinXChanged);
    connect(m_minYSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ClippingBoxDialog::onMinYChanged);
    connect(m_minZSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ClippingBoxDialog::onMinZChanged);

    // Max bounds
    connect(m_maxXSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ClippingBoxDialog::onMaxXChanged);
    connect(m_maxYSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ClippingBoxDialog::onMaxYChanged);
    connect(m_maxZSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ClippingBoxDialog::onMaxZChanged);

    // Options
    connect(m_invertCheck, &QCheckBox::toggled,
            this, &ClippingBoxDialog::onInvertToggled);
    connect(m_previewCheck, &QCheckBox::toggled,
            this, &ClippingBoxDialog::onPreviewToggled);

    // Buttons
    connect(m_resetButton, &QPushButton::clicked,
            this, &ClippingBoxDialog::onResetClicked);
    connect(m_deleteInsideButton, &QPushButton::clicked,
            this, &ClippingBoxDialog::onDeleteInsideClicked);
    connect(m_applyButton, &QPushButton::clicked,
            this, &ClippingBoxDialog::onApplyClicked);
    connect(m_closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
}

void ClippingBoxDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QGroupBox {
            background-color: #242424;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            margin-top: 12px;
            padding: 12px;
            font-weight: 600;
            font-size: 12px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 8px;
            color: #ffffff;
        }
        
        QLabel {
            color: #b3b3b3;
            font-size: 13px;
        }
        
        QLabel#axisLabelX {
            color: #f44336;
            font-weight: 600;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
        }
        
        QLabel#axisLabelY {
            color: #4caf50;
            font-weight: 600;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
        }
        
        QLabel#axisLabelZ {
            color: #2196f3;
            font-weight: 600;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
        }
        
        QLabel#infoLabel {
            color: #808080;
            font-size: 12px;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
            padding: 8px;
        }
        
        QCheckBox {
            color: #b3b3b3;
            spacing: 8px;
            font-size: 13px;
        }
        
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }
        
        QCheckBox::indicator:checked {
            background-color: #0078d4;
            border: none;
            border-radius: 3px;
        }
        
        QCheckBox::indicator:unchecked {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 3px;
        }
        
        QDoubleSpinBox {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px 8px;
            color: #ffffff;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
            font-size: 13px;
            min-width: 140px;
        }
        
        QDoubleSpinBox:focus {
            border: 1px solid #0078d4;
        }
        
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
            background-color: #3d3d3d;
            border: none;
            width: 16px;
        }
        
        QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
            background-color: #4a4a4a;
        }
        
        QFrame#dialogSeparator {
            background-color: #4a4a4a;
            max-height: 1px;
        }
        
        QPushButton#primaryButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
            min-width: 100px;
        }
        
        QPushButton#primaryButton:hover {
            background-color: #1a88e0;
        }
        
        QPushButton#primaryButton:pressed {
            background-color: #0066b8;
        }
        
        QPushButton#warningButton {
            background-color: #f44336;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
            min-width: 100px;
        }
        
        QPushButton#warningButton:hover {
            background-color: #ef5350;
        }
        
        QPushButton#warningButton:pressed {
            background-color: #d32f2f;
        }
        
        QPushButton#secondaryButton {
            background-color: transparent;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QPushButton#secondaryButton:hover {
            background-color: #383838;
            color: #ffffff;
        }
        
        QPushButton#secondaryButton:pressed {
            background-color: #404040;
        }
        
        QPushButton#smallButton {
            background-color: transparent;
            color: #808080;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            padding: 6px 12px;
            font-size: 12px;
            min-width: 60px;
        }
        
        QPushButton#smallButton:hover {
            background-color: #333333;
            color: #b3b3b3;
        }
    )");
}

void ClippingBoxDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void ClippingBoxDialog::setMeshBounds(const BoundingBox& bounds)
{
    m_meshBounds = bounds;
    
    // Set spinbox values without triggering signals
    m_minXSpinbox->blockSignals(true);
    m_minYSpinbox->blockSignals(true);
    m_minZSpinbox->blockSignals(true);
    m_maxXSpinbox->blockSignals(true);
    m_maxYSpinbox->blockSignals(true);
    m_maxZSpinbox->blockSignals(true);
    
    m_minXSpinbox->setValue(bounds.minX);
    m_minYSpinbox->setValue(bounds.minY);
    m_minZSpinbox->setValue(bounds.minZ);
    m_maxXSpinbox->setValue(bounds.maxX);
    m_maxYSpinbox->setValue(bounds.maxY);
    m_maxZSpinbox->setValue(bounds.maxZ);
    
    m_minXSpinbox->blockSignals(false);
    m_minYSpinbox->blockSignals(false);
    m_minZSpinbox->blockSignals(false);
    m_maxXSpinbox->blockSignals(false);
    m_maxYSpinbox->blockSignals(false);
    m_maxZSpinbox->blockSignals(false);
    
    // Update size label
    double sizeX = bounds.maxX - bounds.minX;
    double sizeY = bounds.maxY - bounds.minY;
    double sizeZ = bounds.maxZ - bounds.minZ;
    m_sizeLabel->setText(tr("Size: %1 × %2 × %3 mm")
        .arg(QString::number(sizeX, 'f', 3))
        .arg(QString::number(sizeY, 'f', 3))
        .arg(QString::number(sizeZ, 'f', 3)));
    
    emitBoxChanged();
}

ClippingBoxDialog::BoundingBox ClippingBoxDialog::clippingBox() const
{
    return {
        m_minXSpinbox->value(),
        m_minYSpinbox->value(),
        m_minZSpinbox->value(),
        m_maxXSpinbox->value(),
        m_maxYSpinbox->value(),
        m_maxZSpinbox->value()
    };
}

bool ClippingBoxDialog::invertSelection() const
{
    return m_invertCheck->isChecked();
}

bool ClippingBoxDialog::showPreview() const
{
    return m_previewCheck->isChecked();
}

void ClippingBoxDialog::onMinXChanged(double value)
{
    Q_UNUSED(value)
    validateBounds();
    emitBoxChanged();
}

void ClippingBoxDialog::onMinYChanged(double value)
{
    Q_UNUSED(value)
    validateBounds();
    emitBoxChanged();
}

void ClippingBoxDialog::onMinZChanged(double value)
{
    Q_UNUSED(value)
    validateBounds();
    emitBoxChanged();
}

void ClippingBoxDialog::onMaxXChanged(double value)
{
    Q_UNUSED(value)
    validateBounds();
    emitBoxChanged();
}

void ClippingBoxDialog::onMaxYChanged(double value)
{
    Q_UNUSED(value)
    validateBounds();
    emitBoxChanged();
}

void ClippingBoxDialog::onMaxZChanged(double value)
{
    Q_UNUSED(value)
    validateBounds();
    emitBoxChanged();
}

void ClippingBoxDialog::onInvertToggled(bool checked)
{
    Q_UNUSED(checked)
    if (m_previewCheck->isChecked()) {
        emit previewRequested();
    }
}

void ClippingBoxDialog::onPreviewToggled(bool checked)
{
    if (checked) {
        emit previewRequested();
    }
}

void ClippingBoxDialog::onResetClicked()
{
    setMeshBounds(m_meshBounds);
    emit resetRequested();
}

void ClippingBoxDialog::onApplyClicked()
{
    emit applyRequested(true);  // Delete outside
}

void ClippingBoxDialog::onDeleteInsideClicked()
{
    emit applyRequested(false);  // Delete inside
}

void ClippingBoxDialog::emitBoxChanged()
{
    // Update size label
    BoundingBox box = clippingBox();
    double sizeX = std::abs(box.maxX - box.minX);
    double sizeY = std::abs(box.maxY - box.minY);
    double sizeZ = std::abs(box.maxZ - box.minZ);
    m_sizeLabel->setText(tr("Size: %1 × %2 × %3 mm")
        .arg(QString::number(sizeX, 'f', 3))
        .arg(QString::number(sizeY, 'f', 3))
        .arg(QString::number(sizeZ, 'f', 3)));
    
    emit boxChanged(box);
    
    if (m_previewCheck->isChecked()) {
        emit previewRequested();
    }
}

void ClippingBoxDialog::validateBounds()
{
    // Ensure min <= max for each axis
    // We just warn visually but don't force correction
    // (user might be in the middle of typing)
}
