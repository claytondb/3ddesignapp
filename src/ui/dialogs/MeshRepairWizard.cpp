#include "MeshRepairWizard.h"
#include "../HelpSystem.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFont>

MeshRepairWizard::MeshRepairWizard(QWidget *parent)
    : QDialog(parent)
    , m_viewport(nullptr)
    , m_hasAnalyzed(false)
{
    setWindowTitle(tr("Mesh Repair Wizard"));
    setMinimumSize(520, 680);
    setModal(false);  // Non-modal to allow viewport interaction
    
    setupUI();
    setupConnections();
    applyStylesheet();
    loadSettings();
    updateButtonStates();
}

void MeshRepairWizard::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header with description
    QLabel* headerLabel = new QLabel(tr("Automatically detect and fix common mesh problems."));
    headerLabel->setObjectName("headerLabel");
    headerLabel->setWordWrap(true);
    mainLayout->addWidget(headerLabel);

    // Analysis section
    QHBoxLayout* analyzeLayout = new QHBoxLayout();
    
    m_analyzeButton = new QPushButton(tr("🔍 Analyze Mesh"));
    m_analyzeButton->setObjectName("primaryButton");
    m_analyzeButton->setToolTip(tr("Scan the selected mesh for problems"));
    analyzeLayout->addWidget(m_analyzeButton);
    
    m_meshStatusLabel = new QLabel(tr("Select a mesh and click Analyze"));
    m_meshStatusLabel->setObjectName("infoLabel");
    analyzeLayout->addWidget(m_meshStatusLabel);
    analyzeLayout->addStretch();
    
    mainLayout->addLayout(analyzeLayout);

    // Issues group - shows detected problems
    m_issuesGroup = new QGroupBox(tr("Detected Issues"));
    QVBoxLayout* issuesLayout = new QVBoxLayout(m_issuesGroup);
    issuesLayout->setSpacing(8);
    
    // Create issue labels with icons
    m_holeCountLabel = new QLabel(tr("⭕ Holes: —"));
    m_holeCountLabel->setObjectName("issueLabel");
    issuesLayout->addWidget(m_holeCountLabel);
    
    m_nonManifoldLabel = new QLabel(tr("⚠️ Non-manifold edges: —"));
    m_nonManifoldLabel->setObjectName("issueLabel");
    issuesLayout->addWidget(m_nonManifoldLabel);
    
    m_degenerateFacesLabel = new QLabel(tr("📐 Degenerate faces: —"));
    m_degenerateFacesLabel->setObjectName("issueLabel");
    issuesLayout->addWidget(m_degenerateFacesLabel);
    
    m_isolatedVerticesLabel = new QLabel(tr("📍 Isolated vertices: —"));
    m_isolatedVerticesLabel->setObjectName("issueLabel");
    issuesLayout->addWidget(m_isolatedVerticesLabel);
    
    m_duplicateVerticesLabel = new QLabel(tr("🔄 Duplicate vertices: —"));
    m_duplicateVerticesLabel->setObjectName("issueLabel");
    issuesLayout->addWidget(m_duplicateVerticesLabel);
    
    // Overall status
    QFrame* statusSeparator = new QFrame();
    statusSeparator->setFrameShape(QFrame::HLine);
    statusSeparator->setObjectName("thinSeparator");
    issuesLayout->addWidget(statusSeparator);
    
    m_overallStatusLabel = new QLabel(tr("Overall: Not analyzed"));
    m_overallStatusLabel->setObjectName("overallLabel");
    QFont boldFont = m_overallStatusLabel->font();
    boldFont.setBold(true);
    m_overallStatusLabel->setFont(boldFont);
    issuesLayout->addWidget(m_overallStatusLabel);
    
    mainLayout->addWidget(m_issuesGroup);

    // Fix options group
    m_optionsGroup = new QGroupBox(tr("Repair Options"));
    QVBoxLayout* optionsLayout = new QVBoxLayout(m_optionsGroup);
    optionsLayout->setSpacing(10);
    
    // Fill holes option with max size
    QHBoxLayout* holeLayout = new QHBoxLayout();
    m_fillHolesCheck = new QCheckBox(tr("Fill holes"));
    m_fillHolesCheck->setChecked(true);
    m_fillHolesCheck->setToolTip(tr("Automatically fill detected holes using smart algorithms"));
    holeLayout->addWidget(m_fillHolesCheck);
    
    QLabel* maxHoleLabel = new QLabel(tr("Max edges:"));
    maxHoleLabel->setObjectName("optionLabel");
    holeLayout->addWidget(maxHoleLabel);
    
    m_maxHoleSizeSpinbox = new QSpinBox();
    m_maxHoleSizeSpinbox->setRange(3, 10000);
    m_maxHoleSizeSpinbox->setValue(100);
    m_maxHoleSizeSpinbox->setFixedWidth(80);
    m_maxHoleSizeSpinbox->setToolTip(tr("Skip holes larger than this (may need manual attention)"));
    holeLayout->addWidget(m_maxHoleSizeSpinbox);
    holeLayout->addStretch();
    optionsLayout->addLayout(holeLayout);
    
    // Non-manifold
    m_removeNonManifoldCheck = new QCheckBox(tr("Remove non-manifold geometry"));
    m_removeNonManifoldCheck->setChecked(true);
    m_removeNonManifoldCheck->setToolTip(tr("Fix edges shared by more than 2 faces"));
    optionsLayout->addWidget(m_removeNonManifoldCheck);
    
    // Degenerate faces
    m_removeDegenerateFacesCheck = new QCheckBox(tr("Remove degenerate faces"));
    m_removeDegenerateFacesCheck->setChecked(true);
    m_removeDegenerateFacesCheck->setToolTip(tr("Remove zero-area or malformed triangles"));
    optionsLayout->addWidget(m_removeDegenerateFacesCheck);
    
    // Isolated vertices
    m_removeIsolatedVerticesCheck = new QCheckBox(tr("Remove isolated vertices"));
    m_removeIsolatedVerticesCheck->setChecked(true);
    m_removeIsolatedVerticesCheck->setToolTip(tr("Remove vertices not connected to any face"));
    optionsLayout->addWidget(m_removeIsolatedVerticesCheck);
    
    // Duplicate vertices
    m_removeDuplicateVerticesCheck = new QCheckBox(tr("Merge duplicate vertices"));
    m_removeDuplicateVerticesCheck->setChecked(true);
    m_removeDuplicateVerticesCheck->setToolTip(tr("Merge vertices at the same position"));
    optionsLayout->addWidget(m_removeDuplicateVerticesCheck);
    
    // Smoothing option with iterations
    QHBoxLayout* smoothLayout = new QHBoxLayout();
    m_smoothResultCheck = new QCheckBox(tr("Smooth result"));
    m_smoothResultCheck->setChecked(false);
    m_smoothResultCheck->setToolTip(tr("Apply gentle smoothing after repair"));
    smoothLayout->addWidget(m_smoothResultCheck);
    
    QLabel* iterLabel = new QLabel(tr("Iterations:"));
    iterLabel->setObjectName("optionLabel");
    smoothLayout->addWidget(iterLabel);
    
    m_smoothIterationsSpinbox = new QSpinBox();
    m_smoothIterationsSpinbox->setRange(1, 20);
    m_smoothIterationsSpinbox->setValue(3);
    m_smoothIterationsSpinbox->setFixedWidth(60);
    m_smoothIterationsSpinbox->setEnabled(false);
    m_smoothIterationsSpinbox->setToolTip(tr("Number of smoothing passes"));
    smoothLayout->addWidget(m_smoothIterationsSpinbox);
    smoothLayout->addStretch();
    optionsLayout->addLayout(smoothLayout);
    
    mainLayout->addWidget(m_optionsGroup);

    // Progress section (hidden initially)
    QHBoxLayout* progressLayout = new QHBoxLayout();
    
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    progressLayout->addWidget(m_progressBar);
    
    m_progressLabel = new QLabel();
    m_progressLabel->setObjectName("progressLabel");
    m_progressLabel->setVisible(false);
    progressLayout->addWidget(m_progressLabel);
    
    mainLayout->addLayout(progressLayout);

    // Results group (hidden initially)
    m_resultsGroup = new QGroupBox(tr("Repair Results"));
    QVBoxLayout* resultsLayout = new QVBoxLayout(m_resultsGroup);
    
    m_resultsLabel = new QLabel();
    m_resultsLabel->setObjectName("resultsLabel");
    m_resultsLabel->setWordWrap(true);
    resultsLayout->addWidget(m_resultsLabel);
    
    m_resultsGroup->setVisible(false);
    mainLayout->addWidget(m_resultsGroup);

    mainLayout->addStretch();

    // Separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setObjectName("dialogSeparator");
    mainLayout->addWidget(separator);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // Help button
    QPushButton* helpButton = HelpSystem::addContextHelpButton(this, 
        tr("<b>Mesh Repair Wizard</b><br><br>"
           "This wizard helps fix common problems in scanned meshes:<br><br>"
           "<b>• Holes:</b> Gaps in the surface, usually from scanning occlusions<br>"
           "<b>• Non-manifold:</b> Invalid geometry where edges connect to more than 2 faces<br>"
           "<b>• Degenerate faces:</b> Zero-area or malformed triangles<br>"
           "<b>• Isolated vertices:</b> Floating points not connected to any face<br>"
           "<b>• Duplicate vertices:</b> Multiple vertices at the same location<br><br>"
           "<b>Quick Fix:</b> Click <i>Analyze</i> then <i>Fix All</i> to repair with recommended settings.<br><br>"
           "<b>Tip:</b> For large holes, consider using the dedicated Hole Fill tool for more control."));
    buttonLayout->addWidget(helpButton);
    
    m_resetButton = new QPushButton(tr("Reset"));
    m_resetButton->setObjectName("secondaryButton");
    m_resetButton->setToolTip(tr("Reset all options to defaults"));
    buttonLayout->addWidget(m_resetButton);
    
    buttonLayout->addStretch();
    
    m_previewButton = new QPushButton(tr("Preview"));
    m_previewButton->setObjectName("secondaryButton");
    m_previewButton->setToolTip(tr("Preview changes without applying"));
    m_previewButton->setEnabled(false);
    buttonLayout->addWidget(m_previewButton);
    
    m_fixAllButton = new QPushButton(tr("🔧 Fix All"));
    m_fixAllButton->setObjectName("primaryButton");
    m_fixAllButton->setToolTip(tr("Apply all selected repairs"));
    m_fixAllButton->setEnabled(false);
    buttonLayout->addWidget(m_fixAllButton);
    
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setObjectName("secondaryButton");
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void MeshRepairWizard::setupConnections()
{
    connect(m_analyzeButton, &QPushButton::clicked, 
            this, &MeshRepairWizard::onAnalyzeClicked);
    
    connect(m_fixAllButton, &QPushButton::clicked, 
            this, &MeshRepairWizard::onFixAllClicked);
    
    connect(m_previewButton, &QPushButton::clicked,
            this, &MeshRepairWizard::onPreviewClicked);
    
    connect(m_resetButton, &QPushButton::clicked, 
            this, &MeshRepairWizard::onResetClicked);
    
    connect(m_closeButton, &QPushButton::clicked, this, [this]() {
        saveSettings();
        accept();
    });
    
    // Options changes
    connect(m_fillHolesCheck, &QCheckBox::toggled,
            this, &MeshRepairWizard::onOptionsChanged);
    connect(m_removeNonManifoldCheck, &QCheckBox::toggled,
            this, &MeshRepairWizard::onOptionsChanged);
    connect(m_removeDegenerateFacesCheck, &QCheckBox::toggled,
            this, &MeshRepairWizard::onOptionsChanged);
    connect(m_removeIsolatedVerticesCheck, &QCheckBox::toggled,
            this, &MeshRepairWizard::onOptionsChanged);
    connect(m_removeDuplicateVerticesCheck, &QCheckBox::toggled,
            this, &MeshRepairWizard::onOptionsChanged);
    connect(m_smoothResultCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_smoothIterationsSpinbox->setEnabled(checked);
        onOptionsChanged();
    });
    
    // Progress
    connect(this, &MeshRepairWizard::progressUpdated,
            this, &MeshRepairWizard::updateProgress);
}

void MeshRepairWizard::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QLabel#headerLabel {
            color: #b3b3b3;
            font-size: 13px;
            padding-bottom: 8px;
        }
        
        QGroupBox {
            background-color: #242424;
            border: 1px solid #4a4a4a;
            border-radius: 6px;
            margin-top: 14px;
            padding: 14px;
            font-weight: 600;
            font-size: 13px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 10px;
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
        
        QLabel#issueLabel {
            color: #b3b3b3;
            font-size: 13px;
            padding: 4px 0;
        }
        
        QLabel#overallLabel {
            color: #ffffff;
            font-size: 14px;
            padding: 8px 0;
        }
        
        QLabel#optionLabel {
            color: #808080;
            font-size: 12px;
            padding-left: 16px;
        }
        
        QLabel#progressLabel {
            color: #808080;
            font-size: 12px;
            padding-left: 8px;
        }
        
        QLabel#resultsLabel {
            color: #4fc3f7;
            font-size: 13px;
            line-height: 1.4;
        }
        
        QFrame#thinSeparator {
            background-color: #3d3d3d;
            max-height: 1px;
            margin: 4px 0;
        }
        
        QCheckBox {
            color: #b3b3b3;
            spacing: 8px;
            font-size: 13px;
        }
        
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
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
            width: 18px;
        }
        
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #4a4a4a;
        }
        
        QProgressBar {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            height: 20px;
            text-align: center;
            color: #ffffff;
        }
        
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 3px;
        }
        
        QFrame#dialogSeparator {
            background-color: #4a4a4a;
            max-height: 1px;
        }
        
        QPushButton#primaryButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 5px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: 600;
            min-width: 110px;
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
            border-radius: 5px;
            padding: 10px 20px;
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
    )");
}

void MeshRepairWizard::setViewport(dc::Viewport* viewport)
{
    m_viewport = viewport;
}

void MeshRepairWizard::setMeshIssues(const MeshIssues& issues)
{
    m_issues = issues;
    m_hasAnalyzed = true;
    updateIssueDisplay();
    updateButtonStates();
    
    // Update mesh status
    m_meshStatusLabel->setText(
        tr("%1 vertices, %2 faces")
        .arg(issues.vertexCount)
        .arg(issues.faceCount));
}

void MeshRepairWizard::clearIssues()
{
    m_issues = MeshIssues{};
    m_hasAnalyzed = false;
    
    m_holeCountLabel->setText(tr("⭕ Holes: —"));
    m_nonManifoldLabel->setText(tr("⚠️ Non-manifold edges: —"));
    m_degenerateFacesLabel->setText(tr("📐 Degenerate faces: —"));
    m_isolatedVerticesLabel->setText(tr("📍 Isolated vertices: —"));
    m_duplicateVerticesLabel->setText(tr("🔄 Duplicate vertices: —"));
    m_overallStatusLabel->setText(tr("Overall: Not analyzed"));
    m_overallStatusLabel->setStyleSheet("");
    
    m_meshStatusLabel->setText(tr("Select a mesh and click Analyze"));
    m_resultsGroup->setVisible(false);
    
    updateButtonStates();
}

void MeshRepairWizard::setRepairResults(const RepairResults& results)
{
    QString resultText;
    
    if (results.success) {
        QStringList fixes;
        
        if (results.holesFilled > 0) {
            fixes << tr("• Filled %1 hole(s)").arg(results.holesFilled);
        }
        if (results.nonManifoldFixed > 0) {
            fixes << tr("• Fixed %1 non-manifold edge(s)").arg(results.nonManifoldFixed);
        }
        if (results.degenerateFacesRemoved > 0) {
            fixes << tr("• Removed %1 degenerate face(s)").arg(results.degenerateFacesRemoved);
        }
        if (results.isolatedVerticesRemoved > 0) {
            fixes << tr("• Removed %1 isolated vertex/vertices").arg(results.isolatedVerticesRemoved);
        }
        if (results.duplicateVerticesMerged > 0) {
            fixes << tr("• Merged %1 duplicate vertex/vertices").arg(results.duplicateVerticesMerged);
        }
        if (results.smoothingApplied) {
            fixes << tr("• Applied smoothing");
        }
        
        if (fixes.isEmpty()) {
            resultText = tr("✅ No repairs were needed!");
        } else {
            resultText = tr("✅ Repair completed successfully:\n\n") + fixes.join("\n");
        }
    } else {
        resultText = tr("❌ Repair failed: %1").arg(results.message);
    }
    
    m_resultsLabel->setText(resultText);
    m_resultsGroup->setVisible(true);
    
    // Hide progress
    m_progressBar->setVisible(false);
    m_progressLabel->setVisible(false);
    
    // Re-analyze to show updated state
    // (The main window should trigger this after applying repairs)
}

void MeshRepairWizard::updateIssueDisplay()
{
    // Format each issue with color coding
    auto formatIssue = [](size_t count, const QString& icon, const QString& label) -> QString {
        if (count == 0) {
            return QString("%1 %2: <span style='color: #4caf50;'>0 ✓</span>")
                   .arg(icon).arg(label);
        } else {
            return QString("%1 %2: <span style='color: #ff9800;'>%3</span>")
                   .arg(icon).arg(label).arg(count);
        }
    };
    
    m_holeCountLabel->setText(formatIssue(m_issues.holeCount, "⭕", tr("Holes")));
    m_holeCountLabel->setTextFormat(Qt::RichText);
    
    m_nonManifoldLabel->setText(formatIssue(m_issues.nonManifoldEdges + m_issues.nonManifoldVertices, 
                                            "⚠️", tr("Non-manifold edges")));
    m_nonManifoldLabel->setTextFormat(Qt::RichText);
    
    m_degenerateFacesLabel->setText(formatIssue(m_issues.degenerateFaces, 
                                                "📐", tr("Degenerate faces")));
    m_degenerateFacesLabel->setTextFormat(Qt::RichText);
    
    m_isolatedVerticesLabel->setText(formatIssue(m_issues.isolatedVertices, 
                                                 "📍", tr("Isolated vertices")));
    m_isolatedVerticesLabel->setTextFormat(Qt::RichText);
    
    m_duplicateVerticesLabel->setText(formatIssue(m_issues.duplicateVertices, 
                                                  "🔄", tr("Duplicate vertices")));
    m_duplicateVerticesLabel->setTextFormat(Qt::RichText);
    
    // Overall status
    if (!m_issues.hasIssues()) {
        m_overallStatusLabel->setText(tr("✅ Overall: Mesh is healthy!"));
        m_overallStatusLabel->setStyleSheet("color: #4caf50;");
    } else {
        size_t total = m_issues.totalIssues();
        m_overallStatusLabel->setText(tr("⚠️ Overall: %1 issue(s) found").arg(total));
        m_overallStatusLabel->setStyleSheet("color: #ff9800;");
    }
}

void MeshRepairWizard::updateButtonStates()
{
    bool hasAnalyzed = m_hasAnalyzed;
    bool hasIssues = m_issues.hasIssues();
    bool anyOptionEnabled = m_fillHolesCheck->isChecked() ||
                            m_removeNonManifoldCheck->isChecked() ||
                            m_removeDegenerateFacesCheck->isChecked() ||
                            m_removeIsolatedVerticesCheck->isChecked() ||
                            m_removeDuplicateVerticesCheck->isChecked() ||
                            m_smoothResultCheck->isChecked();
    
    m_fixAllButton->setEnabled(hasAnalyzed && (hasIssues || m_smoothResultCheck->isChecked()) && anyOptionEnabled);
    m_previewButton->setEnabled(hasAnalyzed && hasIssues && anyOptionEnabled);
}

void MeshRepairWizard::onAnalyzeClicked()
{
    m_resultsGroup->setVisible(false);
    emit analyzeRequested();
}

void MeshRepairWizard::onFixAllClicked()
{
    // Show progress
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_progressLabel->setText(tr("Starting repair..."));
    m_progressLabel->setVisible(true);
    m_resultsGroup->setVisible(false);
    
    emit fixAllRequested();
}

void MeshRepairWizard::onPreviewClicked()
{
    emit previewRequested();
}

void MeshRepairWizard::onResetClicked()
{
    resetToDefaults();
}

void MeshRepairWizard::onOptionsChanged()
{
    updateButtonStates();
}

void MeshRepairWizard::updateProgress(int percent, const QString& message)
{
    m_progressBar->setValue(percent);
    m_progressLabel->setText(message);
}

bool MeshRepairWizard::fillHolesEnabled() const
{
    return m_fillHolesCheck->isChecked();
}

bool MeshRepairWizard::removeNonManifoldEnabled() const
{
    return m_removeNonManifoldCheck->isChecked();
}

bool MeshRepairWizard::removeDegenerateFacesEnabled() const
{
    return m_removeDegenerateFacesCheck->isChecked();
}

bool MeshRepairWizard::removeIsolatedVerticesEnabled() const
{
    return m_removeIsolatedVerticesCheck->isChecked();
}

bool MeshRepairWizard::removeDuplicateVerticesEnabled() const
{
    return m_removeDuplicateVerticesCheck->isChecked();
}

bool MeshRepairWizard::smoothResultEnabled() const
{
    return m_smoothResultCheck->isChecked();
}

int MeshRepairWizard::maxHoleSize() const
{
    return m_maxHoleSizeSpinbox->value();
}

int MeshRepairWizard::smoothIterations() const
{
    return m_smoothIterationsSpinbox->value();
}

void MeshRepairWizard::loadSettings()
{
    QSettings settings;
    settings.beginGroup("MeshRepairWizard");
    
    m_fillHolesCheck->setChecked(settings.value("fillHoles", true).toBool());
    m_maxHoleSizeSpinbox->setValue(settings.value("maxHoleSize", 100).toInt());
    m_removeNonManifoldCheck->setChecked(settings.value("removeNonManifold", true).toBool());
    m_removeDegenerateFacesCheck->setChecked(settings.value("removeDegenerateFaces", true).toBool());
    m_removeIsolatedVerticesCheck->setChecked(settings.value("removeIsolatedVertices", true).toBool());
    m_removeDuplicateVerticesCheck->setChecked(settings.value("removeDuplicateVertices", true).toBool());
    m_smoothResultCheck->setChecked(settings.value("smoothResult", false).toBool());
    m_smoothIterationsSpinbox->setValue(settings.value("smoothIterations", 3).toInt());
    m_smoothIterationsSpinbox->setEnabled(m_smoothResultCheck->isChecked());
    
    settings.endGroup();
}

void MeshRepairWizard::saveSettings()
{
    QSettings settings;
    settings.beginGroup("MeshRepairWizard");
    
    settings.setValue("fillHoles", m_fillHolesCheck->isChecked());
    settings.setValue("maxHoleSize", m_maxHoleSizeSpinbox->value());
    settings.setValue("removeNonManifold", m_removeNonManifoldCheck->isChecked());
    settings.setValue("removeDegenerateFaces", m_removeDegenerateFacesCheck->isChecked());
    settings.setValue("removeIsolatedVertices", m_removeIsolatedVerticesCheck->isChecked());
    settings.setValue("removeDuplicateVertices", m_removeDuplicateVerticesCheck->isChecked());
    settings.setValue("smoothResult", m_smoothResultCheck->isChecked());
    settings.setValue("smoothIterations", m_smoothIterationsSpinbox->value());
    
    settings.endGroup();
}

void MeshRepairWizard::resetToDefaults()
{
    m_fillHolesCheck->setChecked(true);
    m_maxHoleSizeSpinbox->setValue(100);
    m_removeNonManifoldCheck->setChecked(true);
    m_removeDegenerateFacesCheck->setChecked(true);
    m_removeIsolatedVerticesCheck->setChecked(true);
    m_removeDuplicateVerticesCheck->setChecked(true);
    m_smoothResultCheck->setChecked(false);
    m_smoothIterationsSpinbox->setValue(3);
    m_smoothIterationsSpinbox->setEnabled(false);
    
    updateButtonStates();
}

QString MeshRepairWizard::formatIssueCount(size_t count, const QString& singular, const QString& plural) const
{
    if (count == 1) {
        return QString("1 %1").arg(singular);
    }
    return QString("%1 %2").arg(count).arg(plural);
}
