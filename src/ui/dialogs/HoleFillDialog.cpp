#include "HoleFillDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFrame>

HoleFillDialog::HoleFillDialog(QWidget *parent)
    : QDialog(parent)
    , m_viewport(nullptr)
{
    setWindowTitle(tr("Fill Holes"));
    setMinimumSize(480, 520);
    setModal(false);  // Non-modal to allow viewport interaction
    
    setupUI();
    setupConnections();
    applyStylesheet();
}

void HoleFillDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Detection section
    QHBoxLayout* detectLayout = new QHBoxLayout();
    
    m_detectButton = new QPushButton(tr("Detect Holes"));
    m_detectButton->setObjectName("primaryButton");
    detectLayout->addWidget(m_detectButton);
    
    m_holeCountLabel = new QLabel(tr("No holes detected"));
    m_holeCountLabel->setObjectName("infoLabel");
    detectLayout->addWidget(m_holeCountLabel);
    detectLayout->addStretch();
    
    mainLayout->addLayout(detectLayout);

    // Filter group
    QGroupBox* filterGroup = new QGroupBox(tr("Filter"));
    QHBoxLayout* filterLayout = new QHBoxLayout(filterGroup);
    
    m_maxHoleFilterCheck = new QCheckBox(tr("Max edge count:"));
    filterLayout->addWidget(m_maxHoleFilterCheck);
    
    m_maxHoleSizeSpinbox = new QSpinBox();
    m_maxHoleSizeSpinbox->setRange(3, 10000);
    m_maxHoleSizeSpinbox->setValue(100);
    m_maxHoleSizeSpinbox->setEnabled(false);
    m_maxHoleSizeSpinbox->setFixedWidth(100);
    filterLayout->addWidget(m_maxHoleSizeSpinbox);
    filterLayout->addStretch();
    
    mainLayout->addWidget(filterGroup);

    // Hole table
    QGroupBox* holesGroup = new QGroupBox(tr("Detected Holes"));
    QVBoxLayout* holesLayout = new QVBoxLayout(holesGroup);
    
    m_holeTable = new QTableWidget();
    m_holeTable->setColumnCount(4);
    m_holeTable->setHorizontalHeaderLabels({tr("ID"), tr("Edges"), tr("Perimeter"), tr("Area")});
    m_holeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_holeTable->verticalHeader()->setVisible(false);
    m_holeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_holeTable->setSelectionMode(QAbstractItemView::MultiSelection);
    m_holeTable->setAlternatingRowColors(true);
    m_holeTable->setMinimumHeight(200);
    holesLayout->addWidget(m_holeTable);
    
    // Selection buttons
    QHBoxLayout* selectionLayout = new QHBoxLayout();
    
    m_selectAllButton = new QPushButton(tr("Select All"));
    m_selectAllButton->setObjectName("smallButton");
    selectionLayout->addWidget(m_selectAllButton);
    
    m_selectNoneButton = new QPushButton(tr("Select None"));
    m_selectNoneButton->setObjectName("smallButton");
    selectionLayout->addWidget(m_selectNoneButton);
    selectionLayout->addStretch();
    
    holesLayout->addLayout(selectionLayout);
    
    mainLayout->addWidget(holesGroup);

    // Fill method group
    QGroupBox* methodGroup = new QGroupBox(tr("Fill Method"));
    QVBoxLayout* methodLayout = new QVBoxLayout(methodGroup);
    
    m_fillMethodCombo = new QComboBox();
    m_fillMethodCombo->addItem(tr("Flat"), static_cast<int>(FillMethod::Flat));
    m_fillMethodCombo->addItem(tr("Smooth"), static_cast<int>(FillMethod::Smooth));
    m_fillMethodCombo->addItem(tr("Curvature-based"), static_cast<int>(FillMethod::CurvatureBased));
    methodLayout->addWidget(m_fillMethodCombo);
    
    m_methodDescription = new QLabel();
    m_methodDescription->setObjectName("descriptionLabel");
    m_methodDescription->setWordWrap(true);
    m_methodDescription->setText(tr("Creates a flat triangulated patch to close the hole."));
    methodLayout->addWidget(m_methodDescription);
    
    mainLayout->addWidget(methodGroup);

    mainLayout->addStretch();

    // Separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setObjectName("dialogSeparator");
    mainLayout->addWidget(separator);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_fillSelectedButton = new QPushButton(tr("Fill Selected"));
    m_fillSelectedButton->setObjectName("primaryButton");
    m_fillSelectedButton->setEnabled(false);
    buttonLayout->addWidget(m_fillSelectedButton);
    
    m_fillAllButton = new QPushButton(tr("Fill All"));
    m_fillAllButton->setObjectName("primaryButton");
    m_fillAllButton->setEnabled(false);
    buttonLayout->addWidget(m_fillAllButton);
    
    buttonLayout->addStretch();
    
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void HoleFillDialog::setupConnections()
{
    connect(m_detectButton, &QPushButton::clicked, 
            this, &HoleFillDialog::onDetectClicked);
    
    connect(m_maxHoleFilterCheck, &QCheckBox::toggled,
            this, &HoleFillDialog::onMaxHoleFilterToggled);
    connect(m_maxHoleSizeSpinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &HoleFillDialog::onMaxHoleSizeChanged);
    
    connect(m_fillMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HoleFillDialog::onFillMethodChanged);
    
    connect(m_holeTable, &QTableWidget::itemSelectionChanged,
            this, &HoleFillDialog::onTableSelectionChanged);
    
    connect(m_selectAllButton, &QPushButton::clicked,
            this, &HoleFillDialog::onSelectAllClicked);
    connect(m_selectNoneButton, &QPushButton::clicked,
            this, &HoleFillDialog::onSelectNoneClicked);
    
    connect(m_fillSelectedButton, &QPushButton::clicked,
            this, &HoleFillDialog::onFillSelectedClicked);
    connect(m_fillAllButton, &QPushButton::clicked,
            this, &HoleFillDialog::onFillAllClicked);
    
    connect(m_closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
}

void HoleFillDialog::applyStylesheet()
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
        
        QLabel#infoLabel {
            color: #808080;
            font-size: 12px;
            padding-left: 12px;
        }
        
        QLabel#descriptionLabel {
            color: #808080;
            font-size: 11px;
            padding: 4px 0;
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
        
        QComboBox {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px 12px;
            color: #ffffff;
            font-size: 13px;
            min-height: 20px;
        }
        
        QComboBox:hover {
            border-color: #5c5c5c;
        }
        
        QComboBox:focus {
            border-color: #0078d4;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 6px solid #b3b3b3;
            margin-right: 8px;
        }
        
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            border: 1px solid #4a4a4a;
            selection-background-color: #0078d4;
            selection-color: #ffffff;
        }
        
        QSpinBox {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 4px 8px;
            color: #ffffff;
            font-family: 'JetBrains Mono', 'Consolas', monospace;
            font-size: 13px;
        }
        
        QSpinBox:focus {
            border: 1px solid #0078d4;
        }
        
        QSpinBox:disabled {
            background-color: #2a2a2a;
            color: #5c5c5c;
            border-color: #333333;
        }
        
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #3d3d3d;
            border: none;
            width: 16px;
        }
        
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #4a4a4a;
        }
        
        QTableWidget {
            background-color: #242424;
            alternate-background-color: #2a2a2a;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            gridline-color: #3d3d3d;
            color: #ffffff;
            font-size: 12px;
        }
        
        QTableWidget::item {
            padding: 4px 8px;
        }
        
        QTableWidget::item:selected {
            background-color: #0078d4;
            color: #ffffff;
        }
        
        QHeaderView::section {
            background-color: #333333;
            color: #b3b3b3;
            padding: 6px 8px;
            border: none;
            border-bottom: 1px solid #4a4a4a;
            font-weight: 600;
            font-size: 11px;
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
        
        QPushButton#primaryButton:disabled {
            background-color: #3d3d3d;
            color: #5c5c5c;
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
            padding: 4px 12px;
            font-size: 11px;
            min-width: 70px;
        }
        
        QPushButton#smallButton:hover {
            background-color: #333333;
            color: #b3b3b3;
        }
    )");
}

void HoleFillDialog::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void HoleFillDialog::setHoles(const QVector<HoleInfo>& holes)
{
    m_holes = holes;
    filterHolesBySize();
    updateHoleTable();
    updateButtonStates();
    
    int count = m_filteredHoles.size();
    if (count == 0) {
        m_holeCountLabel->setText(tr("No holes detected"));
    } else if (count == 1) {
        m_holeCountLabel->setText(tr("1 hole detected"));
    } else {
        m_holeCountLabel->setText(tr("%1 holes detected").arg(count));
    }
}

void HoleFillDialog::clearHoles()
{
    m_holes.clear();
    m_filteredHoles.clear();
    m_holeTable->setRowCount(0);
    m_holeCountLabel->setText(tr("No holes detected"));
    updateButtonStates();
}

HoleFillDialog::FillMethod HoleFillDialog::fillMethod() const
{
    return static_cast<FillMethod>(m_fillMethodCombo->currentData().toInt());
}

int HoleFillDialog::maxHoleSize() const
{
    return m_maxHoleSizeSpinbox->value();
}

bool HoleFillDialog::useMaxHoleFilter() const
{
    return m_maxHoleFilterCheck->isChecked();
}

QVector<int> HoleFillDialog::selectedHoleIds() const
{
    QVector<int> ids;
    
    for (int row = 0; row < m_holeTable->rowCount(); ++row) {
        if (m_holeTable->item(row, 0)->isSelected()) {
            ids.append(m_holeTable->item(row, 0)->data(Qt::UserRole).toInt());
        }
    }
    
    return ids;
}

void HoleFillDialog::onDetectClicked()
{
    emit detectHolesRequested();
}

void HoleFillDialog::onFillMethodChanged(int index)
{
    Q_UNUSED(index)
    
    QString description;
    switch (fillMethod()) {
        case FillMethod::Flat:
            description = tr("Creates a flat triangulated patch to close the hole.");
            break;
        case FillMethod::Smooth:
            description = tr("Creates a smooth surface patch that blends with surrounding geometry.");
            break;
        case FillMethod::CurvatureBased:
            description = tr("Creates a patch that follows the surrounding surface curvature. Best for organic shapes.");
            break;
    }
    m_methodDescription->setText(description);
}

void HoleFillDialog::onMaxHoleSizeChanged(int value)
{
    Q_UNUSED(value)
    filterHolesBySize();
    updateHoleTable();
    updateButtonStates();
}

void HoleFillDialog::onMaxHoleFilterToggled(bool checked)
{
    m_maxHoleSizeSpinbox->setEnabled(checked);
    filterHolesBySize();
    updateHoleTable();
    updateButtonStates();
}

void HoleFillDialog::onTableSelectionChanged()
{
    updateButtonStates();
    
    QVector<int> ids = selectedHoleIds();
    emit holeSelectionChanged(ids);
    emit previewRequested(ids);
}

void HoleFillDialog::onSelectAllClicked()
{
    m_holeTable->selectAll();
}

void HoleFillDialog::onSelectNoneClicked()
{
    m_holeTable->clearSelection();
}

void HoleFillDialog::onFillSelectedClicked()
{
    QVector<int> ids = selectedHoleIds();
    if (!ids.isEmpty()) {
        emit fillSelectedRequested(ids);
    }
}

void HoleFillDialog::onFillAllClicked()
{
    emit fillAllRequested();
}

void HoleFillDialog::updateHoleTable()
{
    m_holeTable->setRowCount(m_filteredHoles.size());
    
    for (int i = 0; i < m_filteredHoles.size(); ++i) {
        const HoleInfo& hole = m_filteredHoles[i];
        
        QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(hole.id));
        idItem->setData(Qt::UserRole, hole.id);
        idItem->setTextAlignment(Qt::AlignCenter);
        m_holeTable->setItem(i, 0, idItem);
        
        QTableWidgetItem* edgesItem = new QTableWidgetItem(QString::number(hole.edgeCount));
        edgesItem->setTextAlignment(Qt::AlignCenter);
        m_holeTable->setItem(i, 1, edgesItem);
        
        QTableWidgetItem* perimeterItem = new QTableWidgetItem(QString::number(hole.perimeter, 'f', 2) + " mm");
        perimeterItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_holeTable->setItem(i, 2, perimeterItem);
        
        QTableWidgetItem* areaItem = new QTableWidgetItem(QString::number(hole.area, 'f', 2) + " mm²");
        areaItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_holeTable->setItem(i, 3, areaItem);
    }
}

void HoleFillDialog::updateButtonStates()
{
    bool hasHoles = !m_filteredHoles.isEmpty();
    bool hasSelection = !selectedHoleIds().isEmpty();
    
    m_fillAllButton->setEnabled(hasHoles);
    m_fillSelectedButton->setEnabled(hasSelection);
    m_selectAllButton->setEnabled(hasHoles);
    m_selectNoneButton->setEnabled(hasSelection);
}

void HoleFillDialog::filterHolesBySize()
{
    m_filteredHoles.clear();
    
    bool useFilter = m_maxHoleFilterCheck->isChecked();
    int maxSize = m_maxHoleSizeSpinbox->value();
    
    for (const HoleInfo& hole : m_holes) {
        if (!useFilter || hole.edgeCount <= maxSize) {
            m_filteredHoles.append(hole);
        }
    }
}
