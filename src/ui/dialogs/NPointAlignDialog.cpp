/**
 * @file NPointAlignDialog.cpp
 * @brief Implementation of N-point alignment dialog
 */

#include "NPointAlignDialog.h"
#include "../../geometry/Alignment.h"
#include "../../geometry/MeshData.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QCheckBox>
#include <QMessageBox>
#include <QDialogButtonBox>

namespace dc {

NPointAlignDialog::NPointAlignDialog(Viewport* viewport, QWidget* parent)
    : QDialog(parent)
    , m_viewport(viewport)
    , m_result(std::make_unique<dc3d::geometry::AlignmentResult>())
{
    setWindowTitle(tr("N-Point Alignment"));
    setMinimumSize(600, 500);
    setupUI();
}

NPointAlignDialog::~NPointAlignDialog() = default;

void NPointAlignDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Instructions
    auto* infoLabel = new QLabel(
        tr("Pick at least 3 corresponding point pairs between source and target meshes.\n"
           "The source mesh will be transformed to align with the target."), this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);
    
    // Point pairs table
    auto* tableGroup = new QGroupBox(tr("Point Pairs"), this);
    auto* tableLayout = new QVBoxLayout(tableGroup);
    
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        tr("#"), tr("Source X"), tr("Source Y"), tr("Source Z"),
        tr("Target X"), tr("Target Y"), tr("Target Z")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableLayout->addWidget(m_table);
    
    // Table buttons
    auto* tableButtonLayout = new QHBoxLayout();
    
    m_addPairButton = new QPushButton(tr("Add Pair"), this);
    tableButtonLayout->addWidget(m_addPairButton);
    
    m_removePairButton = new QPushButton(tr("Remove"), this);
    m_removePairButton->setEnabled(false);
    tableButtonLayout->addWidget(m_removePairButton);
    
    m_clearAllButton = new QPushButton(tr("Clear All"), this);
    tableButtonLayout->addWidget(m_clearAllButton);
    
    tableButtonLayout->addStretch();
    
    m_pickSourceButton = new QPushButton(tr("Pick Source Point"), this);
    m_pickSourceButton->setEnabled(false);
    tableButtonLayout->addWidget(m_pickSourceButton);
    
    m_pickTargetButton = new QPushButton(tr("Pick Target Point"), this);
    m_pickTargetButton->setEnabled(false);
    tableButtonLayout->addWidget(m_pickTargetButton);
    
    tableLayout->addLayout(tableButtonLayout);
    mainLayout->addWidget(tableGroup);
    
    // Statistics
    m_statsGroup = new QGroupBox(tr("Alignment Statistics"), this);
    auto* statsLayout = new QGridLayout(m_statsGroup);
    
    statsLayout->addWidget(new QLabel(tr("Point pairs:"), this), 0, 0);
    m_pairCountLabel = new QLabel(tr("0 (minimum 3 required)"), this);
    statsLayout->addWidget(m_pairCountLabel, 0, 1);
    
    statsLayout->addWidget(new QLabel(tr("RMS Error:"), this), 1, 0);
    m_rmsErrorLabel = new QLabel(tr("-"), this);
    statsLayout->addWidget(m_rmsErrorLabel, 1, 1);
    
    statsLayout->addWidget(new QLabel(tr("Max Error:"), this), 2, 0);
    m_maxErrorLabel = new QLabel(tr("-"), this);
    statsLayout->addWidget(m_maxErrorLabel, 2, 1);
    
    statsLayout->addWidget(new QLabel(tr("Status:"), this), 3, 0);
    m_statusLabel = new QLabel(tr("Need more points"), this);
    m_statusLabel->setStyleSheet("color: orange;");
    statsLayout->addWidget(m_statusLabel, 3, 1);
    
    statsLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(m_statsGroup);
    
    // Preview options
    auto* previewLayout = new QHBoxLayout();
    m_livePreviewCheck = new QCheckBox(tr("Live Preview"), this);
    previewLayout->addWidget(m_livePreviewCheck);
    
    m_previewButton = new QPushButton(tr("Preview"), this);
    m_previewButton->setEnabled(false);
    previewLayout->addWidget(m_previewButton);
    
    previewLayout->addStretch();
    mainLayout->addLayout(previewLayout);
    
    // Dialog buttons
    auto* buttonBox = new QDialogButtonBox(this);
    m_applyButton = buttonBox->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
    m_applyButton->setEnabled(false);
    m_cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    
    // Connections
    connect(m_addPairButton, &QPushButton::clicked, this, &NPointAlignDialog::onAddPairClicked);
    connect(m_removePairButton, &QPushButton::clicked, this, &NPointAlignDialog::onRemovePairClicked);
    connect(m_clearAllButton, &QPushButton::clicked, this, &NPointAlignDialog::onClearAllClicked);
    connect(m_pickSourceButton, &QPushButton::clicked, this, &NPointAlignDialog::onPickSourceClicked);
    connect(m_pickTargetButton, &QPushButton::clicked, this, &NPointAlignDialog::onPickTargetClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &NPointAlignDialog::onTableSelectionChanged);
    connect(m_previewButton, &QPushButton::clicked, this, &NPointAlignDialog::onPreviewClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &NPointAlignDialog::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    connect(m_livePreviewCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked && m_pairs.size() >= 3) {
            onPreviewClicked();
        }
    });
}

void NPointAlignDialog::setSourceMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh) {
    m_sourceMesh = mesh;
    validateInputs();
}

void NPointAlignDialog::setTargetMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh) {
    m_targetMesh = mesh;
    validateInputs();
}

const dc3d::geometry::AlignmentResult& NPointAlignDialog::result() const {
    return *m_result;
}

std::vector<dc3d::geometry::PointPair> NPointAlignDialog::pointPairs() const {
    std::vector<dc3d::geometry::PointPair> pairs;
    for (const auto& row : m_pairs) {
        if (row.valid) {
            dc3d::geometry::PointPair pair;
            pair.source = row.source;
            pair.target = row.target;
            pair.weight = 1.0f;
            pairs.push_back(pair);
        }
    }
    return pairs;
}

void NPointAlignDialog::onSourcePointPicked(int pairId, const glm::vec3& point) {
    for (auto& pair : m_pairs) {
        if (pair.id == pairId) {
            pair.source = point;
            break;
        }
    }
    m_pickMode = PickMode::None;
    m_pickingPairId = -1;
    updateTable();
    validateInputs();
    
    if (m_livePreviewCheck->isChecked() && m_pairs.size() >= 3) {
        onPreviewClicked();
    }
}

void NPointAlignDialog::onTargetPointPicked(int pairId, const glm::vec3& point) {
    for (auto& pair : m_pairs) {
        if (pair.id == pairId) {
            pair.target = point;
            pair.valid = true;
            break;
        }
    }
    m_pickMode = PickMode::None;
    m_pickingPairId = -1;
    updateTable();
    validateInputs();
    
    if (m_livePreviewCheck->isChecked() && m_pairs.size() >= 3) {
        onPreviewClicked();
    }
}

void NPointAlignDialog::onAddPairClicked() {
    PointPairRow row;
    row.id = m_nextPairId++;
    row.source = glm::vec3(0);
    row.target = glm::vec3(0);
    row.error = 0.0f;
    row.valid = false;
    
    m_pairs.push_back(row);
    updateTable();
    
    // Select the new row
    m_table->selectRow(static_cast<int>(m_pairs.size()) - 1);
    
    validateInputs();
}

void NPointAlignDialog::onRemovePairClicked() {
    int idx = getSelectedPairIndex();
    if (idx >= 0 && idx < static_cast<int>(m_pairs.size())) {
        m_pairs.erase(m_pairs.begin() + idx);
        updateTable();
        validateInputs();
    }
}

void NPointAlignDialog::onClearAllClicked() {
    m_pairs.clear();
    m_nextPairId = 1;
    updateTable();
    validateInputs();
}

void NPointAlignDialog::onPickSourceClicked() {
    int idx = getSelectedPairIndex();
    if (idx >= 0 && idx < static_cast<int>(m_pairs.size())) {
        m_pickMode = PickMode::Source;
        m_pickingPairId = m_pairs[idx].id;
        emit requestSourcePointPick(m_pickingPairId);
    }
}

void NPointAlignDialog::onPickTargetClicked() {
    int idx = getSelectedPairIndex();
    if (idx >= 0 && idx < static_cast<int>(m_pairs.size())) {
        m_pickMode = PickMode::Target;
        m_pickingPairId = m_pairs[idx].id;
        emit requestTargetPointPick(m_pickingPairId);
    }
}

void NPointAlignDialog::onTableSelectionChanged() {
    int idx = getSelectedPairIndex();
    bool hasSelection = idx >= 0;
    
    m_removePairButton->setEnabled(hasSelection);
    m_pickSourceButton->setEnabled(hasSelection && m_sourceMesh);
    m_pickTargetButton->setEnabled(hasSelection && m_targetMesh);
}

void NPointAlignDialog::onPreviewClicked() {
    auto pairs = pointPairs();
    if (pairs.size() < 3 || !m_sourceMesh || !m_targetMesh) {
        return;
    }
    
    // Create a copy for preview
    dc3d::geometry::MeshData previewMesh = *m_sourceMesh;
    
    dc3d::geometry::AlignmentOptions options;
    options.preview = true;
    options.computeError = true;
    
    *m_result = dc3d::geometry::Alignment::alignByNPoints(previewMesh, *m_targetMesh, pairs, options);
    
    updateStatistics();
    
    if (m_result->success) {
        emit previewRequested(*m_result);
    }
}

void NPointAlignDialog::onApplyClicked() {
    auto pairs = pointPairs();
    if (pairs.size() < 3) {
        QMessageBox::warning(this, tr("Error"), 
                             tr("At least 3 point pairs are required for alignment."));
        return;
    }
    
    if (!m_sourceMesh || !m_targetMesh) {
        QMessageBox::warning(this, tr("Error"), 
                             tr("Both source and target meshes must be set."));
        return;
    }
    
    dc3d::geometry::AlignmentOptions options;
    options.preview = false;
    options.computeError = true;
    
    *m_result = dc3d::geometry::Alignment::alignByNPoints(*m_sourceMesh, *m_targetMesh, pairs, options);
    
    if (m_result->success) {
        emit alignmentApplied(*m_result);
        accept();
    } else {
        QMessageBox::warning(this, tr("Alignment Failed"),
                             QString::fromStdString(m_result->errorMessage));
    }
}

void NPointAlignDialog::updateTable() {
    m_table->setRowCount(static_cast<int>(m_pairs.size()));
    
    for (size_t i = 0; i < m_pairs.size(); ++i) {
        const auto& pair = m_pairs[i];
        int row = static_cast<int>(i);
        
        auto* idItem = new QTableWidgetItem(QString::number(pair.id));
        m_table->setItem(row, 0, idItem);
        
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(pair.source.x, 'f', 3)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(pair.source.y, 'f', 3)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(pair.source.z, 'f', 3)));
        
        if (pair.valid) {
            m_table->setItem(row, 4, new QTableWidgetItem(QString::number(pair.target.x, 'f', 3)));
            m_table->setItem(row, 5, new QTableWidgetItem(QString::number(pair.target.y, 'f', 3)));
            m_table->setItem(row, 6, new QTableWidgetItem(QString::number(pair.target.z, 'f', 3)));
        } else {
            m_table->setItem(row, 4, new QTableWidgetItem(tr("<pick>")));
            m_table->setItem(row, 5, new QTableWidgetItem(tr("")));
            m_table->setItem(row, 6, new QTableWidgetItem(tr("")));
            
            // Gray out incomplete pairs
            for (int col = 4; col < 7; ++col) {
                m_table->item(row, col)->setForeground(Qt::gray);
            }
        }
    }
    
    m_table->resizeColumnsToContents();
}

void NPointAlignDialog::updateStatistics() {
    auto validPairs = pointPairs();
    int count = static_cast<int>(validPairs.size());
    
    m_pairCountLabel->setText(tr("%1 %2")
                              .arg(count)
                              .arg(count < 3 ? tr("(minimum 3 required)") : ""));
    
    if (m_result->success && count >= 3) {
        m_rmsErrorLabel->setText(tr("%1").arg(m_result->rmsError, 0, 'f', 6));
        m_maxErrorLabel->setText(tr("%1").arg(m_result->maxError, 0, 'f', 6));
        m_statusLabel->setText(tr("Ready"));
        m_statusLabel->setStyleSheet("color: green;");
    } else if (count >= 3) {
        m_rmsErrorLabel->setText(tr("-"));
        m_maxErrorLabel->setText(tr("-"));
        m_statusLabel->setText(tr("Click Preview to compute"));
        m_statusLabel->setStyleSheet("color: blue;");
    } else {
        m_rmsErrorLabel->setText(tr("-"));
        m_maxErrorLabel->setText(tr("-"));
        m_statusLabel->setText(tr("Need more points"));
        m_statusLabel->setStyleSheet("color: orange;");
    }
}

void NPointAlignDialog::validateInputs() {
    auto validPairs = pointPairs();
    bool canAlign = validPairs.size() >= 3 && m_sourceMesh && m_targetMesh;
    
    m_applyButton->setEnabled(canAlign);
    m_previewButton->setEnabled(canAlign);
    
    updateStatistics();
}

int NPointAlignDialog::getSelectedPairIndex() const {
    auto selection = m_table->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        return -1;
    }
    return selection.first().row();
}

void NPointAlignDialog::computePreviewErrors() {
    // This would update individual pair errors after preview computation
    // For now, just update overall statistics
    updateStatistics();
}

} // namespace dc
