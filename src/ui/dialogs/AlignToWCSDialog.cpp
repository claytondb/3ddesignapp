/**
 * @file AlignToWCSDialog.cpp
 * @brief Implementation of WCS alignment dialog
 */

#include "AlignToWCSDialog.h"
#include "../../geometry/Alignment.h"
#include "../../geometry/MeshData.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDialogButtonBox>

namespace dc {

AlignToWCSDialog::AlignToWCSDialog(Viewport* viewport, QWidget* parent)
    : QDialog(parent)
    , m_viewport(viewport)
    , m_result(std::make_unique<dc3d::geometry::AlignmentResult>())
{
    setWindowTitle(tr("Align to WCS"));
    setMinimumWidth(400);
    setupUI();
}

AlignToWCSDialog::~AlignToWCSDialog() = default;

void AlignToWCSDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Primary axis group
    m_primaryGroup = new QGroupBox(tr("Primary Axis"), this);
    auto* primaryLayout = new QGridLayout(m_primaryGroup);
    
    primaryLayout->addWidget(new QLabel(tr("Feature:"), this), 0, 0);
    m_primaryFeatureLabel = new QLabel(tr("<not selected>"), this);
    m_primaryFeatureLabel->setStyleSheet("color: gray; font-style: italic;");
    primaryLayout->addWidget(m_primaryFeatureLabel, 0, 1);
    
    m_pickPrimaryButton = new QPushButton(tr("Pick..."), this);
    primaryLayout->addWidget(m_pickPrimaryButton, 0, 2);
    
    primaryLayout->addWidget(new QLabel(tr("Align to:"), this), 1, 0);
    m_primaryAxisCombo = new QComboBox(this);
    m_primaryAxisCombo->addItem(tr("+X"), static_cast<int>(dc3d::geometry::WCSAxis::PositiveX));
    m_primaryAxisCombo->addItem(tr("-X"), static_cast<int>(dc3d::geometry::WCSAxis::NegativeX));
    m_primaryAxisCombo->addItem(tr("+Y"), static_cast<int>(dc3d::geometry::WCSAxis::PositiveY));
    m_primaryAxisCombo->addItem(tr("-Y"), static_cast<int>(dc3d::geometry::WCSAxis::NegativeY));
    m_primaryAxisCombo->addItem(tr("+Z"), static_cast<int>(dc3d::geometry::WCSAxis::PositiveZ));
    m_primaryAxisCombo->addItem(tr("-Z"), static_cast<int>(dc3d::geometry::WCSAxis::NegativeZ));
    m_primaryAxisCombo->setCurrentIndex(4); // Default to +Z
    primaryLayout->addWidget(m_primaryAxisCombo, 1, 1, 1, 2);
    
    mainLayout->addWidget(m_primaryGroup);
    
    // Secondary axis group
    m_secondaryGroup = new QGroupBox(tr("Secondary Axis"), this);
    auto* secondaryLayout = new QGridLayout(m_secondaryGroup);
    
    secondaryLayout->addWidget(new QLabel(tr("Feature:"), this), 0, 0);
    m_secondaryFeatureLabel = new QLabel(tr("<not selected>"), this);
    m_secondaryFeatureLabel->setStyleSheet("color: gray; font-style: italic;");
    secondaryLayout->addWidget(m_secondaryFeatureLabel, 0, 1);
    
    m_pickSecondaryButton = new QPushButton(tr("Pick..."), this);
    secondaryLayout->addWidget(m_pickSecondaryButton, 0, 2);
    
    secondaryLayout->addWidget(new QLabel(tr("Align to:"), this), 1, 0);
    m_secondaryAxisCombo = new QComboBox(this);
    m_secondaryAxisCombo->addItem(tr("+X"), static_cast<int>(dc3d::geometry::WCSAxis::PositiveX));
    m_secondaryAxisCombo->addItem(tr("-X"), static_cast<int>(dc3d::geometry::WCSAxis::NegativeX));
    m_secondaryAxisCombo->addItem(tr("+Y"), static_cast<int>(dc3d::geometry::WCSAxis::PositiveY));
    m_secondaryAxisCombo->addItem(tr("-Y"), static_cast<int>(dc3d::geometry::WCSAxis::NegativeY));
    m_secondaryAxisCombo->addItem(tr("+Z"), static_cast<int>(dc3d::geometry::WCSAxis::PositiveZ));
    m_secondaryAxisCombo->addItem(tr("-Z"), static_cast<int>(dc3d::geometry::WCSAxis::NegativeZ));
    m_secondaryAxisCombo->setCurrentIndex(0); // Default to +X
    secondaryLayout->addWidget(m_secondaryAxisCombo, 1, 1, 1, 2);
    
    mainLayout->addWidget(m_secondaryGroup);
    
    // Origin group
    m_originGroup = new QGroupBox(tr("Origin"), this);
    auto* originLayout = new QGridLayout(m_originGroup);
    
    m_useFeatureOriginCheck = new QCheckBox(tr("Use primary feature point"), this);
    m_useFeatureOriginCheck->setChecked(true);
    originLayout->addWidget(m_useFeatureOriginCheck, 0, 0, 1, 4);
    
    m_originLabel = new QLabel(tr("<from feature>"), this);
    m_originLabel->setStyleSheet("color: gray; font-style: italic;");
    originLayout->addWidget(m_originLabel, 1, 0, 1, 3);
    
    m_pickOriginButton = new QPushButton(tr("Pick..."), this);
    m_pickOriginButton->setEnabled(false);
    originLayout->addWidget(m_pickOriginButton, 1, 3);
    
    originLayout->addWidget(new QLabel(tr("X:"), this), 2, 0);
    m_originX = new QDoubleSpinBox(this);
    m_originX->setRange(-1e6, 1e6);
    m_originX->setDecimals(4);
    m_originX->setEnabled(false);
    originLayout->addWidget(m_originX, 2, 1);
    
    originLayout->addWidget(new QLabel(tr("Y:"), this), 2, 2);
    m_originY = new QDoubleSpinBox(this);
    m_originY->setRange(-1e6, 1e6);
    m_originY->setDecimals(4);
    m_originY->setEnabled(false);
    originLayout->addWidget(m_originY, 2, 3);
    
    originLayout->addWidget(new QLabel(tr("Z:"), this), 3, 0);
    m_originZ = new QDoubleSpinBox(this);
    m_originZ->setRange(-1e6, 1e6);
    m_originZ->setDecimals(4);
    m_originZ->setEnabled(false);
    originLayout->addWidget(m_originZ, 3, 1);
    
    mainLayout->addWidget(m_originGroup);
    
    // Preview options
    auto* previewLayout = new QHBoxLayout();
    m_livePreviewCheck = new QCheckBox(tr("Live Preview"), this);
    previewLayout->addWidget(m_livePreviewCheck);
    
    m_previewButton = new QPushButton(tr("Preview"), this);
    previewLayout->addWidget(m_previewButton);
    
    m_resetButton = new QPushButton(tr("Reset"), this);
    previewLayout->addWidget(m_resetButton);
    
    previewLayout->addStretch();
    mainLayout->addLayout(previewLayout);
    
    // Dialog buttons
    auto* buttonBox = new QDialogButtonBox(this);
    m_applyButton = buttonBox->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
    m_cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    
    // Initial state
    m_applyButton->setEnabled(false);
    
    // Connections
    connect(m_pickPrimaryButton, &QPushButton::clicked, this, &AlignToWCSDialog::onPickPrimaryClicked);
    connect(m_pickSecondaryButton, &QPushButton::clicked, this, &AlignToWCSDialog::onPickSecondaryClicked);
    connect(m_pickOriginButton, &QPushButton::clicked, this, &AlignToWCSDialog::onPickOriginClicked);
    
    connect(m_primaryAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AlignToWCSDialog::onPrimaryAxisChanged);
    connect(m_secondaryAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AlignToWCSDialog::onSecondaryAxisChanged);
    
    connect(m_useFeatureOriginCheck, &QCheckBox::toggled, [this](bool checked) {
        m_pickOriginButton->setEnabled(!checked);
        m_originX->setEnabled(!checked);
        m_originY->setEnabled(!checked);
        m_originZ->setEnabled(!checked);
        if (checked && m_primaryFeature) {
            m_originLabel->setText(tr("<from feature>"));
        }
    });
    
    connect(m_previewButton, &QPushButton::clicked, this, &AlignToWCSDialog::onPreviewClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &AlignToWCSDialog::onResetClicked);
    
    connect(m_applyButton, &QPushButton::clicked, this, &AlignToWCSDialog::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    connect(m_livePreviewCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked) {
            updatePreview();
        }
    });
}

void AlignToWCSDialog::setMesh(std::shared_ptr<dc3d::geometry::MeshData> mesh) {
    m_mesh = mesh;
    onResetClicked();
}

const dc3d::geometry::AlignmentResult& AlignToWCSDialog::result() const {
    return *m_result;
}

void AlignToWCSDialog::onFeaturePicked(const dc3d::geometry::AlignmentFeatureData& feature) {
    QString desc;
    switch (feature.type) {
        case dc3d::geometry::AlignmentFeature::Plane:
            desc = tr("Plane (%.3f, %.3f, %.3f)")
                   .arg(feature.direction.x)
                   .arg(feature.direction.y)
                   .arg(feature.direction.z);
            break;
        case dc3d::geometry::AlignmentFeature::Line:
            desc = tr("Line (%.3f, %.3f, %.3f)")
                   .arg(feature.direction.x)
                   .arg(feature.direction.y)
                   .arg(feature.direction.z);
            break;
        case dc3d::geometry::AlignmentFeature::CylinderAxis:
            desc = tr("Cylinder axis");
            break;
        case dc3d::geometry::AlignmentFeature::Point:
            desc = tr("Point (%.3f, %.3f, %.3f)")
                   .arg(feature.point.x)
                   .arg(feature.point.y)
                   .arg(feature.point.z);
            break;
        default:
            desc = tr("Feature");
            break;
    }
    
    switch (m_pickMode) {
        case PickMode::Primary:
            m_primaryFeature = std::make_unique<dc3d::geometry::AlignmentFeatureData>(feature);
            m_primaryFeatureLabel->setText(desc);
            m_primaryFeatureLabel->setStyleSheet("");
            break;
            
        case PickMode::Secondary:
            m_secondaryFeature = std::make_unique<dc3d::geometry::AlignmentFeatureData>(feature);
            m_secondaryFeatureLabel->setText(desc);
            m_secondaryFeatureLabel->setStyleSheet("");
            break;
            
        case PickMode::Origin:
            m_originX->setValue(feature.point.x);
            m_originY->setValue(feature.point.y);
            m_originZ->setValue(feature.point.z);
            m_originSet = true;
            m_originLabel->setText(tr("(%.3f, %.3f, %.3f)")
                                   .arg(feature.point.x)
                                   .arg(feature.point.y)
                                   .arg(feature.point.z));
            m_originLabel->setStyleSheet("");
            break;
            
        default:
            break;
    }
    
    m_pickMode = PickMode::None;
    validateInputs();
    
    if (m_livePreviewCheck->isChecked()) {
        updatePreview();
    }
}

void AlignToWCSDialog::onPickPrimaryClicked() {
    m_pickMode = PickMode::Primary;
    emit requestFeaturePick(tr("primary axis"));
}

void AlignToWCSDialog::onPickSecondaryClicked() {
    m_pickMode = PickMode::Secondary;
    emit requestFeaturePick(tr("secondary axis"));
}

void AlignToWCSDialog::onPickOriginClicked() {
    m_pickMode = PickMode::Origin;
    emit requestFeaturePick(tr("origin point"));
}

void AlignToWCSDialog::onPrimaryAxisChanged(int index) {
    Q_UNUSED(index);
    if (m_livePreviewCheck->isChecked()) {
        updatePreview();
    }
}

void AlignToWCSDialog::onSecondaryAxisChanged(int index) {
    Q_UNUSED(index);
    if (m_livePreviewCheck->isChecked()) {
        updatePreview();
    }
}

void AlignToWCSDialog::onPreviewClicked() {
    updatePreview();
}

void AlignToWCSDialog::onApplyClicked() {
    if (!m_mesh || !m_primaryFeature || !m_secondaryFeature) {
        QMessageBox::warning(this, tr("Error"), tr("Please select all required features."));
        return;
    }
    
    // Get axis selections
    dc3d::geometry::WCSAxis primaryAxis = getSelectedAxis(m_primaryAxisCombo);
    dc3d::geometry::WCSAxis secondaryAxis = getSelectedAxis(m_secondaryAxisCombo);
    
    // Get origin
    std::optional<glm::vec3> origin;
    if (!m_useFeatureOriginCheck->isChecked()) {
        origin = glm::vec3(
            static_cast<float>(m_originX->value()),
            static_cast<float>(m_originY->value()),
            static_cast<float>(m_originZ->value())
        );
    }
    
    // Perform alignment
    dc3d::geometry::AlignmentOptions options;
    options.preview = false;
    
    *m_result = dc3d::geometry::Alignment::alignToWCS(
        *m_mesh,
        *m_primaryFeature,
        primaryAxis,
        *m_secondaryFeature,
        secondaryAxis,
        origin,
        options
    );
    
    if (m_result->success) {
        emit alignmentApplied(*m_result);
        accept();
    } else {
        QMessageBox::warning(this, tr("Alignment Failed"), 
                             QString::fromStdString(m_result->errorMessage));
    }
}

void AlignToWCSDialog::onResetClicked() {
    m_primaryFeature.reset();
    m_secondaryFeature.reset();
    m_primaryFeatureLabel->setText(tr("<not selected>"));
    m_primaryFeatureLabel->setStyleSheet("color: gray; font-style: italic;");
    m_secondaryFeatureLabel->setText(tr("<not selected>"));
    m_secondaryFeatureLabel->setStyleSheet("color: gray; font-style: italic;");
    m_originLabel->setText(tr("<from feature>"));
    m_originLabel->setStyleSheet("color: gray; font-style: italic;");
    m_useFeatureOriginCheck->setChecked(true);
    m_originX->setValue(0);
    m_originY->setValue(0);
    m_originZ->setValue(0);
    m_originSet = false;
    m_pickMode = PickMode::None;
    validateInputs();
}

void AlignToWCSDialog::updatePreview() {
    if (!m_mesh || !m_primaryFeature || !m_secondaryFeature) {
        return;
    }
    
    // Create a copy for preview
    dc3d::geometry::MeshData previewMesh = *m_mesh;
    
    dc3d::geometry::WCSAxis primaryAxis = getSelectedAxis(m_primaryAxisCombo);
    dc3d::geometry::WCSAxis secondaryAxis = getSelectedAxis(m_secondaryAxisCombo);
    
    std::optional<glm::vec3> origin;
    if (!m_useFeatureOriginCheck->isChecked()) {
        origin = glm::vec3(
            static_cast<float>(m_originX->value()),
            static_cast<float>(m_originY->value()),
            static_cast<float>(m_originZ->value())
        );
    }
    
    dc3d::geometry::AlignmentOptions options;
    options.preview = true;
    
    *m_result = dc3d::geometry::Alignment::alignToWCS(
        previewMesh,
        *m_primaryFeature,
        primaryAxis,
        *m_secondaryFeature,
        secondaryAxis,
        origin,
        options
    );
    
    if (m_result->success) {
        emit previewRequested(*m_result);
    }
}

void AlignToWCSDialog::validateInputs() {
    bool valid = m_mesh && m_primaryFeature && m_secondaryFeature;
    m_applyButton->setEnabled(valid);
    m_previewButton->setEnabled(valid);
}

dc3d::geometry::WCSAxis AlignToWCSDialog::getSelectedAxis(QComboBox* combo) const {
    return static_cast<dc3d::geometry::WCSAxis>(combo->currentData().toInt());
}

} // namespace dc
