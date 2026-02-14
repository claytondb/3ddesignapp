#include "ExportFormatDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDialogButtonBox>

ExportFormatDialog::ExportFormatDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Export Mesh"));
    setMinimumSize(450, 400);
    setModal(true);
    
    setupUI();
    applyStylesheet();
    
    // Set defaults
    m_formatCombo->setCurrentIndex(0);
    onFormatChanged(0);
}

ExportFormatDialog::~ExportFormatDialog()
{
}

void ExportFormatDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    setupFormatGroup();
    setupOptionsStack();
    setupQualityGroup();
    setupGeneralGroup();
    setupButtons();
}

void ExportFormatDialog::setupFormatGroup()
{
    QGroupBox* formatGroup = new QGroupBox(tr("Export Format"), this);
    QVBoxLayout* layout = new QVBoxLayout(formatGroup);
    
    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem(tr("STL (Binary) - 3D Printing"), STL_Binary);
    m_formatCombo->addItem(tr("STL (ASCII) - Compatible"), STL_ASCII);
    m_formatCombo->addItem(tr("OBJ (Wavefront) - Universal"), OBJ);
    m_formatCombo->addItem(tr("PLY (Stanford) - With Colors"), PLY);
    m_formatCombo->addItem(tr("STEP (CAD) - Engineering"), STEP);
    m_formatCombo->addItem(tr("IGES (CAD) - Legacy CAD"), IGES);
    
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportFormatDialog::onFormatChanged);
    
    m_formatDescription = new QLabel(this);
    m_formatDescription->setWordWrap(true);
    m_formatDescription->setStyleSheet("color: #808080; font-size: 11px;");
    
    layout->addWidget(m_formatCombo);
    layout->addWidget(m_formatDescription);
    
    static_cast<QVBoxLayout*>(this->layout())->addWidget(formatGroup);
}

void ExportFormatDialog::setupOptionsStack()
{
    QGroupBox* optionsGroup = new QGroupBox(tr("Format Options"), this);
    QVBoxLayout* layout = new QVBoxLayout(optionsGroup);
    
    m_optionsStack = new QStackedWidget(this);
    
    // Page 0: STL Binary options
    QWidget* stlBinaryPage = new QWidget();
    QVBoxLayout* stlBinaryLayout = new QVBoxLayout(stlBinaryPage);
    QLabel* stlBinaryInfo = new QLabel(tr("Binary STL is compact and fast to load.\nRecommended for 3D printing."));
    stlBinaryInfo->setStyleSheet("color: #808080;");
    stlBinaryLayout->addWidget(stlBinaryInfo);
    stlBinaryLayout->addStretch();
    m_optionsStack->addWidget(stlBinaryPage);
    
    // Page 1: STL ASCII options
    QWidget* stlAsciiPage = new QWidget();
    QVBoxLayout* stlAsciiLayout = new QVBoxLayout(stlAsciiPage);
    QLabel* stlAsciiInfo = new QLabel(tr("ASCII STL is human-readable but larger.\nUse for debugging or legacy software."));
    stlAsciiInfo->setStyleSheet("color: #808080;");
    stlAsciiLayout->addWidget(stlAsciiInfo);
    stlAsciiLayout->addStretch();
    m_optionsStack->addWidget(stlAsciiPage);
    
    // Page 2: OBJ options
    QWidget* objPage = new QWidget();
    QVBoxLayout* objLayout = new QVBoxLayout(objPage);
    m_objNormalsCheck = new QCheckBox(tr("Include vertex normals"), this);
    m_objNormalsCheck->setChecked(true);
    m_objUVsCheck = new QCheckBox(tr("Include texture coordinates (UV)"), this);
    m_objMaterialsCheck = new QCheckBox(tr("Export materials (.mtl file)"), this);
    objLayout->addWidget(m_objNormalsCheck);
    objLayout->addWidget(m_objUVsCheck);
    objLayout->addWidget(m_objMaterialsCheck);
    objLayout->addStretch();
    m_optionsStack->addWidget(objPage);
    
    // Page 3: PLY options
    QWidget* plyPage = new QWidget();
    QVBoxLayout* plyLayout = new QVBoxLayout(plyPage);
    
    QButtonGroup* plyFormatGroup = new QButtonGroup(this);
    m_plyBinaryRadio = new QRadioButton(tr("Binary (compact)"), this);
    m_plyAsciiRadio = new QRadioButton(tr("ASCII (readable)"), this);
    m_plyBinaryRadio->setChecked(true);
    plyFormatGroup->addButton(m_plyBinaryRadio);
    plyFormatGroup->addButton(m_plyAsciiRadio);
    
    m_plyColorsCheck = new QCheckBox(tr("Include vertex colors"), this);
    m_plyColorsCheck->setChecked(true);
    
    plyLayout->addWidget(m_plyBinaryRadio);
    plyLayout->addWidget(m_plyAsciiRadio);
    plyLayout->addWidget(m_plyColorsCheck);
    plyLayout->addStretch();
    m_optionsStack->addWidget(plyPage);
    
    // Page 4: STEP options
    QWidget* stepPage = new QWidget();
    QVBoxLayout* stepLayout = new QVBoxLayout(stepPage);
    QLabel* stepInfo = new QLabel(tr("STEP AP214 format with full geometry.\nIdeal for CAD/CAM software exchange."));
    stepInfo->setStyleSheet("color: #808080;");
    stepLayout->addWidget(stepInfo);
    stepLayout->addStretch();
    m_optionsStack->addWidget(stepPage);
    
    // Page 5: IGES options
    QWidget* igesPage = new QWidget();
    QVBoxLayout* igesLayout = new QVBoxLayout(igesPage);
    QLabel* igesInfo = new QLabel(tr("IGES 5.3 format for legacy CAD systems.\nUse STEP for modern software."));
    igesInfo->setStyleSheet("color: #808080;");
    igesLayout->addWidget(igesInfo);
    igesLayout->addStretch();
    m_optionsStack->addWidget(igesPage);
    
    layout->addWidget(m_optionsStack);
    
    static_cast<QVBoxLayout*>(this->layout())->addWidget(optionsGroup);
}

void ExportFormatDialog::setupQualityGroup()
{
    QGroupBox* qualityGroup = new QGroupBox(tr("Tessellation Quality"), this);
    QVBoxLayout* layout = new QVBoxLayout(qualityGroup);
    
    m_qualityCombo = new QComboBox(this);
    m_qualityCombo->addItem(tr("Draft (Fast, low detail)"), Draft);
    m_qualityCombo->addItem(tr("Standard (Balanced)"), Standard);
    m_qualityCombo->addItem(tr("Fine (High detail)"), Fine);
    m_qualityCombo->addItem(tr("Custom..."), Custom);
    m_qualityCombo->setCurrentIndex(1); // Standard
    
    connect(m_qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportFormatDialog::onQualityChanged);
    
    m_customQualityGroup = new QGroupBox(tr("Custom Settings"), this);
    m_customQualityGroup->setVisible(false);
    QGridLayout* customLayout = new QGridLayout(m_customQualityGroup);
    
    customLayout->addWidget(new QLabel(tr("Chord tolerance:")), 0, 0);
    m_chordSpin = new QDoubleSpinBox(this);
    m_chordSpin->setRange(0.001, 10.0);
    m_chordSpin->setValue(0.1);
    m_chordSpin->setDecimals(3);
    m_chordSpin->setSuffix(" mm");
    customLayout->addWidget(m_chordSpin, 0, 1);
    
    customLayout->addWidget(new QLabel(tr("Angle tolerance:")), 1, 0);
    m_angleSpin = new QDoubleSpinBox(this);
    m_angleSpin->setRange(1.0, 45.0);
    m_angleSpin->setValue(15.0);
    m_angleSpin->setDecimals(1);
    m_angleSpin->setSuffix("°");
    customLayout->addWidget(m_angleSpin, 1, 1);
    
    layout->addWidget(m_qualityCombo);
    layout->addWidget(m_customQualityGroup);
    
    static_cast<QVBoxLayout*>(this->layout())->addWidget(qualityGroup);
}

void ExportFormatDialog::setupGeneralGroup()
{
    QGroupBox* generalGroup = new QGroupBox(tr("Output"), this);
    QVBoxLayout* layout = new QVBoxLayout(generalGroup);
    
    // File path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit(this);
    m_filePathEdit->setPlaceholderText(tr("Select output file..."));
    
    QPushButton* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &ExportFormatDialog::onBrowseClicked);
    
    pathLayout->addWidget(m_filePathEdit);
    pathLayout->addWidget(browseBtn);
    
    // Options
    m_exportSelectedCheck = new QCheckBox(tr("Export selected objects only"), this);
    
    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel(tr("Scale factor:")));
    m_scaleSpin = new QDoubleSpinBox(this);
    m_scaleSpin->setRange(0.001, 1000.0);
    m_scaleSpin->setValue(1.0);
    m_scaleSpin->setDecimals(3);
    scaleLayout->addWidget(m_scaleSpin);
    scaleLayout->addStretch();
    
    layout->addLayout(pathLayout);
    layout->addWidget(m_exportSelectedCheck);
    layout->addLayout(scaleLayout);
    
    static_cast<QVBoxLayout*>(this->layout())->addWidget(generalGroup);
}

void ExportFormatDialog::setupButtons()
{
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Export"));
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_filePathEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Export Error"),
                tr("Please select an output file."));
            return;
        }
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    static_cast<QVBoxLayout*>(this->layout())->addWidget(buttonBox);
}

void ExportFormatDialog::applyStylesheet()
{
    // Inherit from parent window styling
}

void ExportFormatDialog::onFormatChanged(int index)
{
    ExportFormat format = static_cast<ExportFormat>(m_formatCombo->currentData().toInt());
    m_formatDescription->setText(formatDescription(format));
    m_optionsStack->setCurrentIndex(index);
    updateFileExtension();
    emit formatChanged(format);
}

void ExportFormatDialog::onQualityChanged(int index)
{
    QualityPreset preset = static_cast<QualityPreset>(m_qualityCombo->currentData().toInt());
    m_customQualityGroup->setVisible(preset == Custom);
    
    // Set preset values
    switch (preset) {
        case Draft:
            m_chordSpin->setValue(0.5);
            m_angleSpin->setValue(30.0);
            break;
        case Standard:
            m_chordSpin->setValue(0.1);
            m_angleSpin->setValue(15.0);
            break;
        case Fine:
            m_chordSpin->setValue(0.01);
            m_angleSpin->setValue(5.0);
            break;
        case Custom:
            // Keep current values
            break;
    }
}

void ExportFormatDialog::onBrowseClicked()
{
    ExportFormat format = static_cast<ExportFormat>(m_formatCombo->currentData().toInt());
    
    QString filter;
    QString defaultExt;
    
    switch (format) {
        case STL_Binary:
        case STL_ASCII:
            filter = tr("STL Files (*.stl);;All Files (*)");
            defaultExt = "stl";
            break;
        case OBJ:
            filter = tr("OBJ Files (*.obj);;All Files (*)");
            defaultExt = "obj";
            break;
        case PLY:
            filter = tr("PLY Files (*.ply);;All Files (*)");
            defaultExt = "ply";
            break;
        case STEP:
            filter = tr("STEP Files (*.step *.stp);;All Files (*)");
            defaultExt = "step";
            break;
        case IGES:
            filter = tr("IGES Files (*.iges *.igs);;All Files (*)");
            defaultExt = "igs";
            break;
    }
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export As"), 
        m_filePathEdit->text(), filter);
    
    if (!filePath.isEmpty()) {
        // Ensure correct extension
        QFileInfo fileInfo(filePath);
        if (fileInfo.suffix().isEmpty()) {
            filePath += "." + defaultExt;
        }
        m_filePathEdit->setText(filePath);
    }
}

void ExportFormatDialog::updateFileExtension()
{
    QString currentPath = m_filePathEdit->text();
    if (currentPath.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(currentPath);
    QString baseName = fileInfo.completeBaseName();
    QString dir = fileInfo.absolutePath();
    
    QString newPath = dir + "/" + baseName + "." + currentExtension();
    m_filePathEdit->setText(newPath);
}

void ExportFormatDialog::validateInput()
{
    // Validation handled in accept()
}

QString ExportFormatDialog::currentExtension() const
{
    ExportFormat format = static_cast<ExportFormat>(m_formatCombo->currentData().toInt());
    
    switch (format) {
        case STL_Binary:
        case STL_ASCII:
            return "stl";
        case OBJ:
            return "obj";
        case PLY:
            return "ply";
        case STEP:
            return "step";
        case IGES:
            return "igs";
        default:
            return "stl";
    }
}

void ExportFormatDialog::setFilePath(const QString& path)
{
    m_filePathEdit->setText(path);
}

ExportFormatDialog::ExportSettings ExportFormatDialog::settings() const
{
    ExportSettings s;
    s.format = static_cast<ExportFormat>(m_formatCombo->currentData().toInt());
    s.filePath = m_filePathEdit->text();
    s.stlBinary = (s.format == STL_Binary);
    s.objIncludeNormals = m_objNormalsCheck->isChecked();
    s.objIncludeUVs = m_objUVsCheck->isChecked();
    s.objIncludeMaterials = m_objMaterialsCheck->isChecked();
    s.plyBinary = m_plyBinaryRadio->isChecked();
    s.plyIncludeColors = m_plyColorsCheck->isChecked();
    s.quality = static_cast<QualityPreset>(m_qualityCombo->currentData().toInt());
    s.chordTolerance = m_chordSpin->value();
    s.angleTolerance = m_angleSpin->value();
    s.exportSelected = m_exportSelectedCheck->isChecked();
    s.scaleFactor = m_scaleSpin->value();
    return s;
}

QString ExportFormatDialog::formatDescription(ExportFormat format)
{
    switch (format) {
        case STL_Binary:
            return tr("Binary STL is the most common format for 3D printing. "
                     "Compact file size, fast loading. No color support.");
        case STL_ASCII:
            return tr("ASCII STL is human-readable but creates larger files. "
                     "Use for debugging or legacy software compatibility.");
        case OBJ:
            return tr("Wavefront OBJ is widely supported by 3D software. "
                     "Supports normals, UVs, and materials via .mtl files.");
        case PLY:
            return tr("Stanford PLY format supports vertex colors. "
                     "Common for 3D scanning and point cloud data.");
        case STEP:
            return tr("STEP (AP214) is the standard for CAD data exchange. "
                     "Preserves geometry precisely for engineering use.");
        case IGES:
            return tr("IGES is a legacy CAD format. Use STEP for modern software. "
                     "Only for compatibility with older CAD systems.");
        default:
            return QString();
    }
}
