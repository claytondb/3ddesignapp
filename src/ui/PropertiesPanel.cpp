#include "PropertiesPanel.h"
#include <QFormLayout>
#include <QScrollArea>
#include <QPushButton>

PropertiesPanel::PropertiesPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("PropertiesPanel");
    setupUI();
}

void PropertiesPanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Create stacked widget for different property pages
    m_stackedWidget = new QStackedWidget(this);
    
    // Add pages
    m_stackedWidget->addWidget(createNoSelectionPage());  // Index 0
    m_stackedWidget->addWidget(createMeshPage());          // Index 1
    m_stackedWidget->addWidget(createPrimitivePage());     // Index 2
    m_stackedWidget->addWidget(createSketchPage());        // Index 3
    m_stackedWidget->addWidget(createSurfacePage());       // Index 4
    m_stackedWidget->addWidget(createBodyPage());          // Index 5

    m_layout->addWidget(m_stackedWidget);
    
    // Start with no selection page
    m_stackedWidget->setCurrentIndex(PageNoSelection);
}

QGroupBox* PropertiesPanel::createCollapsibleGroup(const QString& title)
{
    QGroupBox* group = new QGroupBox(title);
    group->setCheckable(false);
    group->setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            border: none;
            margin-top: 16px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 4px;
            color: #ffffff;
        }
    )");
    return group;
}

QWidget* PropertiesPanel::createSpinBoxRow(const QString& label, QDoubleSpinBox* spinBox,
                                            const QString& suffix)
{
    if (!spinBox) return nullptr;  // Null check to prevent crash
    
    QWidget* row = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    
    QLabel* lbl = new QLabel(label);
    lbl->setFixedWidth(20);
    lbl->setStyleSheet("color: #808080;");
    layout->addWidget(lbl);
    
    spinBox->setDecimals(3);
    spinBox->setRange(-99999.999, 99999.999);
    spinBox->setSingleStep(0.1);
    if (!suffix.isEmpty()) {
        spinBox->setSuffix(" " + suffix);
    }
    spinBox->setStyleSheet(R"(
        QDoubleSpinBox {
            background-color: #333333;
            color: #ffffff;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 2px 4px;
            font-family: 'JetBrains Mono', monospace;
        }
    )");
    layout->addWidget(spinBox, 1);
    
    return row;
}

QWidget* PropertiesPanel::createNoSelectionPage()
{
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    // Scene Statistics section
    QGroupBox* statsGroup = createCollapsibleGroup(tr("Scene Statistics"));
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
    statsLayout->setSpacing(4);
    
    auto createStatRow = [](const QString& label, QLabel*& valueLabel) -> QWidget* {
        QWidget* row = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        
        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("color: #808080;");
        layout->addWidget(lbl);
        
        valueLabel = new QLabel("0");
        valueLabel->setStyleSheet("color: #ffffff;");
        valueLabel->setAlignment(Qt::AlignRight);
        layout->addWidget(valueLabel);
        
        return row;
    };
    
    statsLayout->addWidget(createStatRow(tr("Meshes:"), m_meshCountLabel));
    statsLayout->addWidget(createStatRow(tr("Triangles:"), m_triangleCountLabel));
    statsLayout->addWidget(createStatRow(tr("Surfaces:"), m_surfaceCountLabel));
    statsLayout->addWidget(createStatRow(tr("Bodies:"), m_bodyCountLabel));
    
    layout->addWidget(statsGroup);

    // Coordinate System section
    QGroupBox* coordGroup = createCollapsibleGroup(tr("Coordinate System"));
    QVBoxLayout* coordLayout = new QVBoxLayout(coordGroup);
    coordLayout->setSpacing(4);
    
    QWidget* unitsRow = new QWidget();
    QHBoxLayout* unitsLayout = new QHBoxLayout(unitsRow);
    unitsLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* unitsLabel = new QLabel(tr("Units:"));
    unitsLabel->setStyleSheet("color: #808080;");
    unitsLayout->addWidget(unitsLabel);
    
    m_unitsCombo = new QComboBox();
    m_unitsCombo->addItems({"mm", "cm", "m", "in", "ft"});
    m_unitsCombo->setCurrentText("mm");
    connect(m_unitsCombo, &QComboBox::currentTextChanged, this, &PropertiesPanel::unitsChanged);
    unitsLayout->addWidget(m_unitsCombo, 1);
    
    coordLayout->addWidget(unitsRow);
    
    QLabel* originLabel = new QLabel(tr("Origin: [0, 0, 0]"));
    originLabel->setStyleSheet("color: #808080;");
    coordLayout->addWidget(originLabel);
    
    layout->addWidget(coordGroup);
    
    // Add stretch to push content to top
    layout->addStretch();
    
    scroll->setWidget(page);
    return scroll;
}

QWidget* PropertiesPanel::createMeshPage()
{
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    // Mesh name header
    m_meshNameLabel = new QLabel(tr("(No mesh selected)"));
    m_meshNameLabel->setStyleSheet("color: #ffffff; font-weight: bold; font-size: 14px;");
    m_meshNameLabel->setWordWrap(true);
    layout->addWidget(m_meshNameLabel);
    
    // Divider
    QFrame* divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("background-color: #4a4a4a;");
    layout->addWidget(divider);

    // Geometry section
    QGroupBox* geoGroup = createCollapsibleGroup(tr("Geometry"));
    QVBoxLayout* geoLayout = new QVBoxLayout(geoGroup);
    geoLayout->setSpacing(4);
    
    auto createInfoRow = [](const QString& label, QLabel*& valueLabel) -> QWidget* {
        QWidget* row = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        
        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("color: #808080;");
        layout->addWidget(lbl);
        
        valueLabel = new QLabel("-");
        valueLabel->setStyleSheet("color: #ffffff; font-family: 'JetBrains Mono', monospace;");
        valueLabel->setAlignment(Qt::AlignRight);
        layout->addWidget(valueLabel);
        
        return row;
    };
    
    geoLayout->addWidget(createInfoRow(tr("Triangles:"), m_meshTrianglesLabel));
    geoLayout->addWidget(createInfoRow(tr("Vertices:"), m_meshVerticesLabel));
    geoLayout->addWidget(createInfoRow(tr("Bounds:"), m_meshBoundsLabel));
    geoLayout->addWidget(createInfoRow(tr("Has holes:"), m_meshHolesLabel));
    
    layout->addWidget(geoGroup);

    // Display section
    QGroupBox* displayGroup = createCollapsibleGroup(tr("Display"));
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    displayLayout->setSpacing(8);
    
    // Color picker (placeholder)
    QWidget* colorRow = new QWidget();
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* colorLabel = new QLabel(tr("Color:"));
    colorLabel->setStyleSheet("color: #808080;");
    colorLayout->addWidget(colorLabel);
    
    QPushButton* colorButton = new QPushButton();
    colorButton->setFixedSize(24, 24);
    colorButton->setStyleSheet("background-color: #808080; border: 1px solid #4a4a4a; border-radius: 4px;");
    colorLayout->addWidget(colorButton);
    colorLayout->addStretch();
    
    displayLayout->addWidget(colorRow);
    
    // Opacity slider
    QWidget* opacityRow = new QWidget();
    QHBoxLayout* opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* opacityLabel = new QLabel(tr("Opacity:"));
    opacityLabel->setStyleSheet("color: #808080;");
    opacityLayout->addWidget(opacityLabel);
    
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_opacityLabel->setText(QString::number(value) + "%");
        emit meshOpacityChanged(value);
    });
    opacityLayout->addWidget(m_opacitySlider, 1);
    
    m_opacityLabel = new QLabel("100%");
    m_opacityLabel->setFixedWidth(40);
    m_opacityLabel->setStyleSheet("color: #ffffff;");
    opacityLayout->addWidget(m_opacityLabel);
    
    displayLayout->addWidget(opacityRow);
    
    // Show edges checkbox
    m_showEdgesCheck = new QCheckBox(tr("Show edges"));
    m_showEdgesCheck->setChecked(false);
    connect(m_showEdgesCheck, &QCheckBox::toggled, this, &PropertiesPanel::meshShowEdgesChanged);
    displayLayout->addWidget(m_showEdgesCheck);
    
    layout->addWidget(displayGroup);

    // Transform section
    QGroupBox* transformGroup = createCollapsibleGroup(tr("Transform"));
    QVBoxLayout* transformLayout = new QVBoxLayout(transformGroup);
    transformLayout->setSpacing(4);
    
    // Position
    QLabel* posLabel = new QLabel(tr("Position:"));
    posLabel->setStyleSheet("color: #808080;");
    transformLayout->addWidget(posLabel);
    
    m_posXSpin = new QDoubleSpinBox();
    m_posYSpin = new QDoubleSpinBox();
    m_posZSpin = new QDoubleSpinBox();
    
    transformLayout->addWidget(createSpinBoxRow("X:", m_posXSpin, "mm"));
    transformLayout->addWidget(createSpinBoxRow("Y:", m_posYSpin, "mm"));
    transformLayout->addWidget(createSpinBoxRow("Z:", m_posZSpin, "mm"));
    
    // Connect position changes
    auto emitPosition = [this]() {
        emit meshPositionChanged(m_posXSpin->value(), m_posYSpin->value(), m_posZSpin->value());
    };
    connect(m_posXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, emitPosition);
    connect(m_posYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, emitPosition);
    connect(m_posZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, emitPosition);
    
    // Rotation
    QLabel* rotLabel = new QLabel(tr("Rotation:"));
    rotLabel->setStyleSheet("color: #808080;");
    transformLayout->addWidget(rotLabel);
    
    m_rotXSpin = new QDoubleSpinBox();
    m_rotYSpin = new QDoubleSpinBox();
    m_rotZSpin = new QDoubleSpinBox();
    
    m_rotXSpin->setSuffix(" °");
    m_rotYSpin->setSuffix(" °");
    m_rotZSpin->setSuffix(" °");
    m_rotXSpin->setRange(-360, 360);
    m_rotYSpin->setRange(-360, 360);
    m_rotZSpin->setRange(-360, 360);
    
    transformLayout->addWidget(createSpinBoxRow("X:", m_rotXSpin, ""));
    transformLayout->addWidget(createSpinBoxRow("Y:", m_rotYSpin, ""));
    transformLayout->addWidget(createSpinBoxRow("Z:", m_rotZSpin, ""));
    
    // Connect rotation changes
    auto emitRotation = [this]() {
        emit meshRotationChanged(m_rotXSpin->value(), m_rotYSpin->value(), m_rotZSpin->value());
    };
    connect(m_rotXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, emitRotation);
    connect(m_rotYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, emitRotation);
    connect(m_rotZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, emitRotation);
    
    layout->addWidget(transformGroup);

    // Deviation section (hidden by default)
    m_deviationGroup = createCollapsibleGroup(tr("Deviation"));
    QVBoxLayout* devLayout = new QVBoxLayout(m_deviationGroup);
    devLayout->setSpacing(4);
    
    auto createDevRow = [](const QString& label, QLabel*& valueLabel) -> QWidget* {
        QWidget* row = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        
        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("color: #808080;");
        layout->addWidget(lbl);
        
        valueLabel = new QLabel("-");
        valueLabel->setStyleSheet("color: #ffffff; font-family: 'JetBrains Mono', monospace;");
        valueLabel->setAlignment(Qt::AlignRight);
        layout->addWidget(valueLabel);
        
        return row;
    };
    
    devLayout->addWidget(createDevRow(tr("Min:"), m_deviationMinLabel));
    devLayout->addWidget(createDevRow(tr("Max:"), m_deviationMaxLabel));
    devLayout->addWidget(createDevRow(tr("Avg:"), m_deviationAvgLabel));
    devLayout->addWidget(createDevRow(tr("Std Dev:"), m_deviationStdLabel));
    
    m_deviationGroup->setVisible(false);
    layout->addWidget(m_deviationGroup);

    // Add stretch
    layout->addStretch();
    
    scroll->setWidget(page);
    return scroll;
}

QWidget* PropertiesPanel::createPrimitivePage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* label = new QLabel(tr("Primitive Properties"));
    label->setStyleSheet("color: #ffffff; font-weight: bold;");
    layout->addWidget(label);
    
    QLabel* placeholder = new QLabel(tr("Select a primitive to view its properties"));
    placeholder->setStyleSheet("color: #808080;");
    placeholder->setWordWrap(true);
    layout->addWidget(placeholder);
    
    layout->addStretch();
    return page;
}

QWidget* PropertiesPanel::createSketchPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* label = new QLabel(tr("Sketch Properties"));
    label->setStyleSheet("color: #ffffff; font-weight: bold;");
    layout->addWidget(label);
    
    QLabel* placeholder = new QLabel(tr("Select a sketch to view its properties"));
    placeholder->setStyleSheet("color: #808080;");
    placeholder->setWordWrap(true);
    layout->addWidget(placeholder);
    
    layout->addStretch();
    return page;
}

QWidget* PropertiesPanel::createSurfacePage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* label = new QLabel(tr("Surface Properties"));
    label->setStyleSheet("color: #ffffff; font-weight: bold;");
    layout->addWidget(label);
    
    QLabel* placeholder = new QLabel(tr("Select a surface to view its properties"));
    placeholder->setStyleSheet("color: #808080;");
    placeholder->setWordWrap(true);
    layout->addWidget(placeholder);
    
    layout->addStretch();
    return page;
}

QWidget* PropertiesPanel::createBodyPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* label = new QLabel(tr("Body Properties"));
    label->setStyleSheet("color: #ffffff; font-weight: bold;");
    layout->addWidget(label);
    
    QLabel* placeholder = new QLabel(tr("Select a body to view its properties"));
    placeholder->setStyleSheet("color: #808080;");
    placeholder->setWordWrap(true);
    layout->addWidget(placeholder);
    
    layout->addStretch();
    return page;
}

void PropertiesPanel::setPage(Page page)
{
    m_stackedWidget->setCurrentIndex(static_cast<int>(page));
}

PropertiesPanel::Page PropertiesPanel::currentPage() const
{
    return static_cast<Page>(m_stackedWidget->currentIndex());
}

void PropertiesPanel::setSceneStats(int meshCount, int triangleCount, int surfaceCount, int bodyCount)
{
    m_meshCountLabel->setText(QString::number(meshCount));
    m_triangleCountLabel->setText(QLocale().toString(triangleCount));
    m_surfaceCountLabel->setText(QString::number(surfaceCount));
    m_bodyCountLabel->setText(QString::number(bodyCount));
}

void PropertiesPanel::setUnits(const QString& units)
{
    m_unitsCombo->setCurrentText(units);
}

void PropertiesPanel::setMeshName(const QString& name)
{
    m_meshNameLabel->setText(name);
}

void PropertiesPanel::setMeshTriangles(int count)
{
    m_meshTrianglesLabel->setText(QLocale().toString(count));
}

void PropertiesPanel::setMeshVertices(int count)
{
    m_meshVerticesLabel->setText(QLocale().toString(count));
}

void PropertiesPanel::setMeshBounds(double x, double y, double z)
{
    m_meshBoundsLabel->setText(QString("%1 × %2 × %3 mm")
        .arg(x, 0, 'f', 1)
        .arg(y, 0, 'f', 1)
        .arg(z, 0, 'f', 1));
}

void PropertiesPanel::setMeshHasHoles(bool hasHoles, int holeCount)
{
    if (hasHoles) {
        m_meshHolesLabel->setText(QString("Yes (%1)").arg(holeCount));
        m_meshHolesLabel->setStyleSheet("color: #ff9800;"); // Warning color
    } else {
        m_meshHolesLabel->setText("No");
        m_meshHolesLabel->setStyleSheet("color: #4caf50;"); // Success color
    }
}

void PropertiesPanel::setMeshColor(const QColor& color)
{
    Q_UNUSED(color);
    // Would update color button background
}

void PropertiesPanel::setMeshOpacity(int percent)
{
    m_opacitySlider->blockSignals(true);
    m_opacitySlider->setValue(percent);
    m_opacitySlider->blockSignals(false);
    m_opacityLabel->setText(QString::number(percent) + "%");
}

void PropertiesPanel::setMeshShowEdges(bool show)
{
    m_showEdgesCheck->blockSignals(true);
    m_showEdgesCheck->setChecked(show);
    m_showEdgesCheck->blockSignals(false);
}

void PropertiesPanel::setMeshPosition(double x, double y, double z)
{
    m_posXSpin->blockSignals(true);
    m_posYSpin->blockSignals(true);
    m_posZSpin->blockSignals(true);
    m_posXSpin->setValue(x);
    m_posYSpin->setValue(y);
    m_posZSpin->setValue(z);
    m_posXSpin->blockSignals(false);
    m_posYSpin->blockSignals(false);
    m_posZSpin->blockSignals(false);
}

void PropertiesPanel::setMeshRotation(double x, double y, double z)
{
    m_rotXSpin->blockSignals(true);
    m_rotYSpin->blockSignals(true);
    m_rotZSpin->blockSignals(true);
    m_rotXSpin->setValue(x);
    m_rotYSpin->setValue(y);
    m_rotZSpin->setValue(z);
    m_rotXSpin->blockSignals(false);
    m_rotYSpin->blockSignals(false);
    m_rotZSpin->blockSignals(false);
}

void PropertiesPanel::setDeviationStats(double min, double max, double avg, double stdDev)
{
    m_deviationMinLabel->setText(QString("%1 mm").arg(min, 0, 'f', 3));
    m_deviationMaxLabel->setText(QString("+%1 mm").arg(max, 0, 'f', 3));
    m_deviationAvgLabel->setText(QString("%1 mm").arg(avg, 0, 'f', 3));
    m_deviationStdLabel->setText(QString("%1 mm").arg(stdDev, 0, 'f', 3));
}

void PropertiesPanel::showDeviation(bool show)
{
    m_deviationGroup->setVisible(show);
}

void PropertiesPanel::setProperties(const QVariantMap& props)
{
    // Switch to mesh page for now (most common case)
    setPage(PageMesh);
    
    // Set properties from map
    if (props.contains("Name")) {
        setMeshName(props["Name"].toString());
    }
    if (props.contains("Vertices")) {
        setMeshVertices(props["Vertices"].toString().toInt());
    }
    if (props.contains("Triangles")) {
        setMeshTriangles(props["Triangles"].toString().toInt());
    }
    
    // For multiple selection, we show different info
    if (props.contains("Selected Objects")) {
        m_meshNameLabel->setText(props["Selected Objects"].toString() + " objects selected");
    }
    if (props.contains("Total Vertices")) {
        m_meshVerticesLabel->setText(props["Total Vertices"].toString());
    }
    if (props.contains("Total Triangles")) {
        m_meshTrianglesLabel->setText(props["Total Triangles"].toString());
    }
}

void PropertiesPanel::clearProperties()
{
    setPage(PageNoSelection);
}
