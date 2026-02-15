#include "ExportPresetsDialog.h"
#include "ExportPresetManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QFont>

ExportPresetsDialog::ExportPresetsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Export Presets"));
    setMinimumSize(550, 450);
    setModal(true);
    
    setupUI();
    applyStylesheet();
    refreshPresetList();
    
    // Connect to preset manager changes
    connect(ExportPresetManager::instance(), &ExportPresetManager::presetsChanged,
            this, &ExportPresetsDialog::refreshPresetList);
    connect(ExportPresetManager::instance(), &ExportPresetManager::defaultPresetChanged,
            this, &ExportPresetsDialog::refreshPresetList);
}

ExportPresetsDialog::~ExportPresetsDialog()
{
}

void ExportPresetsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // Header
    QLabel* headerLabel = new QLabel(tr("Manage export presets for quick access to common export configurations."), this);
    headerLabel->setWordWrap(true);
    headerLabel->setStyleSheet("color: #808080; margin-bottom: 8px;");
    mainLayout->addWidget(headerLabel);
    
    // Main content area
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);
    
    // Preset list
    QVBoxLayout* listLayout = new QVBoxLayout();
    
    QLabel* presetsLabel = new QLabel(tr("Presets:"), this);
    QFont boldFont = presetsLabel->font();
    boldFont.setBold(true);
    presetsLabel->setFont(boldFont);
    listLayout->addWidget(presetsLabel);
    
    m_presetList = new QListWidget(this);
    m_presetList->setMinimumWidth(200);
    connect(m_presetList, &QListWidget::currentItemChanged,
            this, &ExportPresetsDialog::onPresetSelectionChanged);
    connect(m_presetList, &QListWidget::itemDoubleClicked,
            this, &ExportPresetsDialog::onPresetDoubleClicked);
    listLayout->addWidget(m_presetList);
    
    // List buttons
    QHBoxLayout* listButtonsLayout = new QHBoxLayout();
    listButtonsLayout->setSpacing(8);
    
    m_setDefaultBtn = new QPushButton(tr("Set as Default"), this);
    m_setDefaultBtn->setToolTip(tr("Set this preset as the default for Quick Export"));
    connect(m_setDefaultBtn, &QPushButton::clicked, this, &ExportPresetsDialog::onSetDefaultClicked);
    listButtonsLayout->addWidget(m_setDefaultBtn);
    
    m_renameBtn = new QPushButton(tr("Rename..."), this);
    m_renameBtn->setToolTip(tr("Rename this preset"));
    connect(m_renameBtn, &QPushButton::clicked, this, &ExportPresetsDialog::onRenameClicked);
    listButtonsLayout->addWidget(m_renameBtn);
    
    m_deleteBtn = new QPushButton(tr("Delete"), this);
    m_deleteBtn->setToolTip(tr("Delete this preset"));
    connect(m_deleteBtn, &QPushButton::clicked, this, &ExportPresetsDialog::onDeleteClicked);
    listButtonsLayout->addWidget(m_deleteBtn);
    
    listLayout->addLayout(listButtonsLayout);
    contentLayout->addLayout(listLayout);
    
    // Details panel
    m_detailsGroup = new QGroupBox(tr("Preset Details"), this);
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsGroup);
    detailsLayout->setSpacing(8);
    
    m_defaultIndicator = new QLabel(this);
    m_defaultIndicator->setStyleSheet("color: #4CAF50; font-weight: bold;");
    detailsLayout->addWidget(m_defaultIndicator);
    
    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setStyleSheet("color: #b3b3b3; margin-bottom: 12px;");
    detailsLayout->addWidget(m_descriptionLabel);
    
    m_formatLabel = new QLabel(this);
    detailsLayout->addWidget(m_formatLabel);
    
    m_qualityLabel = new QLabel(this);
    detailsLayout->addWidget(m_qualityLabel);
    
    m_settingsLabel = new QLabel(this);
    m_settingsLabel->setWordWrap(true);
    m_settingsLabel->setStyleSheet("color: #808080; font-size: 11px;");
    detailsLayout->addWidget(m_settingsLabel);
    
    detailsLayout->addStretch();
    
    contentLayout->addWidget(m_detailsGroup, 1);
    mainLayout->addLayout(contentLayout);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    m_closeBtn = buttonBox->button(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ExportPresetsDialog::applyStylesheet()
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
        
        QListWidget {
            background-color: #242424;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            outline: none;
        }
        
        QListWidget::item {
            padding: 8px;
        }
        
        QListWidget::item:hover {
            background-color: #383838;
        }
        
        QListWidget::item:selected {
            background-color: #0078d4;
            color: #ffffff;
        }
        
        QPushButton {
            background-color: #383838;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 6px 12px;
            font-size: 12px;
            min-width: 70px;
        }
        
        QPushButton:hover {
            background-color: #404040;
            color: #ffffff;
        }
        
        QPushButton:pressed {
            background-color: #333333;
        }
        
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #5c5c5c;
            border-color: #333333;
        }
        
        QDialogButtonBox QPushButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
            min-width: 80px;
        }
        
        QDialogButtonBox QPushButton:hover {
            background-color: #1a88e0;
        }
        
        QDialogButtonBox QPushButton:pressed {
            background-color: #0066b8;
        }
    )");
}

void ExportPresetsDialog::refreshPresetList()
{
    m_presetList->clear();
    
    ExportPresetManager* mgr = ExportPresetManager::instance();
    QString defaultPreset = mgr->defaultPreset();
    
    // Add built-in presets first
    QStringList builtIn = mgr->builtInPresetNames();
    for (const QString& name : builtIn) {
        QListWidgetItem* item = new QListWidgetItem(m_presetList);
        QString displayName = name;
        if (name == defaultPreset) {
            displayName += tr(" ★");
        }
        item->setText(displayName);
        item->setData(Qt::UserRole, name);
        item->setData(Qt::UserRole + 1, true);  // isBuiltIn
        item->setIcon(QIcon(":/icons/preset-builtin.png"));
        item->setToolTip(tr("Built-in preset"));
    }
    
    // Separator
    if (!builtIn.isEmpty() && !mgr->userPresetNames().isEmpty()) {
        QListWidgetItem* separator = new QListWidgetItem(m_presetList);
        separator->setText("─────────────");
        separator->setFlags(Qt::NoItemFlags);
        separator->setData(Qt::UserRole, QString());
    }
    
    // Add user presets
    QStringList userPresets = mgr->userPresetNames();
    for (const QString& name : userPresets) {
        QListWidgetItem* item = new QListWidgetItem(m_presetList);
        QString displayName = name;
        if (name == defaultPreset) {
            displayName += tr(" ★");
        }
        item->setText(displayName);
        item->setData(Qt::UserRole, name);
        item->setData(Qt::UserRole + 1, false);  // isBuiltIn
        item->setIcon(QIcon(":/icons/preset-user.png"));
        item->setToolTip(tr("User preset"));
    }
    
    updateButtonStates();
}

void ExportPresetsDialog::onPresetSelectionChanged()
{
    updatePresetDetails();
    updateButtonStates();
}

void ExportPresetsDialog::updatePresetDetails()
{
    QListWidgetItem* item = m_presetList->currentItem();
    if (!item) {
        m_detailsGroup->setTitle(tr("Preset Details"));
        m_defaultIndicator->clear();
        m_descriptionLabel->clear();
        m_formatLabel->clear();
        m_qualityLabel->clear();
        m_settingsLabel->clear();
        return;
    }
    
    QString presetName = item->data(Qt::UserRole).toString();
    if (presetName.isEmpty()) {
        return;  // Separator
    }
    
    ExportPresetManager* mgr = ExportPresetManager::instance();
    ExportPresetManager::ExportPreset preset = mgr->preset(presetName);
    
    m_detailsGroup->setTitle(presetName);
    
    // Default indicator
    if (presetName == mgr->defaultPreset()) {
        m_defaultIndicator->setText(tr("★ Default Preset (Quick Export)"));
    } else {
        m_defaultIndicator->clear();
    }
    
    // Description
    if (!preset.description.isEmpty()) {
        m_descriptionLabel->setText(preset.description);
    } else {
        m_descriptionLabel->setText(preset.isBuiltIn ? tr("Built-in preset") : tr("User-defined preset"));
    }
    
    // Format
    m_formatLabel->setText(tr("<b>Format:</b> %1").arg(formatName(preset.format)));
    
    // Quality
    m_qualityLabel->setText(tr("<b>Quality:</b> %1").arg(qualityName(preset.quality)));
    
    // Additional settings
    QStringList settings;
    if (preset.quality == 3) {  // Custom
        settings << tr("Chord: %1mm, Angle: %2°")
            .arg(preset.chordTolerance, 0, 'f', 3)
            .arg(preset.angleTolerance, 0, 'f', 1);
    }
    if (preset.scaleFactor != 1.0) {
        settings << tr("Scale: %1x").arg(preset.scaleFactor);
    }
    if (preset.format == 2) {  // OBJ
        QStringList objSettings;
        if (preset.objIncludeNormals) objSettings << tr("Normals");
        if (preset.objIncludeUVs) objSettings << tr("UVs");
        if (preset.objIncludeMaterials) objSettings << tr("Materials");
        if (!objSettings.isEmpty()) {
            settings << tr("OBJ: %1").arg(objSettings.join(", "));
        }
    }
    if (preset.format == 3) {  // PLY
        QStringList plySettings;
        plySettings << (preset.plyBinary ? tr("Binary") : tr("ASCII"));
        if (preset.plyIncludeColors) plySettings << tr("Colors");
        settings << tr("PLY: %1").arg(plySettings.join(", "));
    }
    
    m_settingsLabel->setText(settings.join("\n"));
}

void ExportPresetsDialog::updateButtonStates()
{
    QListWidgetItem* item = m_presetList->currentItem();
    bool hasSelection = item && !item->data(Qt::UserRole).toString().isEmpty();
    bool isBuiltIn = item && item->data(Qt::UserRole + 1).toBool();
    
    ExportPresetManager* mgr = ExportPresetManager::instance();
    QString currentName = hasSelection ? item->data(Qt::UserRole).toString() : QString();
    bool isDefault = hasSelection && (currentName == mgr->defaultPreset());
    
    m_setDefaultBtn->setEnabled(hasSelection && !isDefault);
    m_renameBtn->setEnabled(hasSelection && !isBuiltIn);
    m_deleteBtn->setEnabled(hasSelection && !isBuiltIn);
}

void ExportPresetsDialog::onSetDefaultClicked()
{
    QListWidgetItem* item = m_presetList->currentItem();
    if (!item) return;
    
    QString presetName = item->data(Qt::UserRole).toString();
    if (!presetName.isEmpty()) {
        ExportPresetManager::instance()->setDefaultPreset(presetName);
        refreshPresetList();
        
        // Reselect the same preset
        for (int i = 0; i < m_presetList->count(); ++i) {
            if (m_presetList->item(i)->data(Qt::UserRole).toString() == presetName) {
                m_presetList->setCurrentRow(i);
                break;
            }
        }
    }
}

void ExportPresetsDialog::onRenameClicked()
{
    QListWidgetItem* item = m_presetList->currentItem();
    if (!item) return;
    
    QString oldName = item->data(Qt::UserRole).toString();
    if (oldName.isEmpty()) return;
    
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Preset"),
        tr("New name:"), QLineEdit::Normal, oldName, &ok);
    
    if (ok && !newName.isEmpty() && newName != oldName) {
        ExportPresetManager* mgr = ExportPresetManager::instance();
        if (mgr->hasPreset(newName)) {
            QMessageBox::warning(this, tr("Rename Failed"),
                tr("A preset with name \"%1\" already exists.").arg(newName));
            return;
        }
        
        if (mgr->renamePreset(oldName, newName)) {
            refreshPresetList();
            // Select the renamed preset
            for (int i = 0; i < m_presetList->count(); ++i) {
                if (m_presetList->item(i)->data(Qt::UserRole).toString() == newName) {
                    m_presetList->setCurrentRow(i);
                    break;
                }
            }
        }
    }
}

void ExportPresetsDialog::onDeleteClicked()
{
    QListWidgetItem* item = m_presetList->currentItem();
    if (!item) return;
    
    QString presetName = item->data(Qt::UserRole).toString();
    if (presetName.isEmpty()) return;
    
    int result = QMessageBox::question(this, tr("Delete Preset"),
        tr("Are you sure you want to delete the preset \"%1\"?").arg(presetName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        ExportPresetManager::instance()->deletePreset(presetName);
    }
}

void ExportPresetsDialog::onPresetDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    
    QString presetName = item->data(Qt::UserRole).toString();
    if (!presetName.isEmpty()) {
        emit presetSelected(presetName);
        accept();
    }
}

QString ExportPresetsDialog::formatName(int format) const
{
    switch (format) {
        case 0: return tr("STL (Binary)");
        case 1: return tr("STL (ASCII)");
        case 2: return tr("OBJ (Wavefront)");
        case 3: return tr("PLY (Stanford)");
        case 4: return tr("STEP (CAD)");
        case 5: return tr("IGES (CAD)");
        default: return tr("Unknown");
    }
}

QString ExportPresetsDialog::qualityName(int quality) const
{
    switch (quality) {
        case 0: return tr("Draft (Fast)");
        case 1: return tr("Standard");
        case 2: return tr("Fine (High Quality)");
        case 3: return tr("Custom");
        default: return tr("Standard");
    }
}
