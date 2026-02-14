#include "GettingStartedDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QStyle>
#include <QApplication>

GettingStartedDialog::GettingStartedDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Getting Started"));
    setMinimumSize(600, 480);
    setModal(true);
    
    setupUI();
    applyStylesheet();
    updateNavigation();
}

bool GettingStartedDialog::showOnFirstRun(QWidget *parent)
{
    QSettings settings("dc-3ddesignapp", "dc-3ddesignapp");
    
    if (settings.value("app/firstRunComplete", false).toBool()) {
        return false;
    }
    
    GettingStartedDialog dialog(parent);
    dialog.exec();
    
    // Mark first run as complete if user checked "don't show again"
    if (dialog.m_dontShowAgain->isChecked()) {
        settings.setValue("app/firstRunComplete", true);
    }
    
    return true;
}

void GettingStartedDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Stacked widget for pages
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->addWidget(createWelcomePage());
    m_stackedWidget->addWidget(createImportPage());
    m_stackedWidget->addWidget(createMeshToolsPage());
    m_stackedWidget->addWidget(createSketchPage());
    m_stackedWidget->addWidget(createExportPage());
    mainLayout->addWidget(m_stackedWidget, 1);

    // Navigation bar
    QWidget* navBar = new QWidget();
    navBar->setObjectName("navBar");
    QHBoxLayout* navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(20, 16, 20, 16);

    // Don't show again checkbox
    m_dontShowAgain = new QCheckBox(tr("Don't show on startup"));
    navLayout->addWidget(m_dontShowAgain);

    navLayout->addStretch();

    // Page indicator
    m_pageIndicator = new QLabel();
    m_pageIndicator->setObjectName("pageIndicator");
    navLayout->addWidget(m_pageIndicator);

    navLayout->addStretch();

    // Navigation buttons
    m_skipButton = new QPushButton(tr("Skip"));
    m_skipButton->setObjectName("skipButton");
    connect(m_skipButton, &QPushButton::clicked, this, &GettingStartedDialog::onSkipClicked);
    navLayout->addWidget(m_skipButton);

    m_prevButton = new QPushButton(tr("← Previous"));
    m_prevButton->setObjectName("navButton");
    connect(m_prevButton, &QPushButton::clicked, this, &GettingStartedDialog::onPrevClicked);
    navLayout->addWidget(m_prevButton);

    m_nextButton = new QPushButton(tr("Next →"));
    m_nextButton->setObjectName("primaryButton");
    connect(m_nextButton, &QPushButton::clicked, this, &GettingStartedDialog::onNextClicked);
    navLayout->addWidget(m_nextButton);

    mainLayout->addWidget(navBar);
}

QWidget* GettingStartedDialog::createWelcomePage()
{
    QWidget* page = new QWidget();
    page->setObjectName("tutorialPage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 20);
    layout->setSpacing(20);

    // Icon
    QLabel* icon = new QLabel();
    icon->setPixmap(QApplication::style()->standardIcon(QStyle::SP_DesktopIcon).pixmap(80, 80));
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    // Title
    QLabel* title = new QLabel(tr("Welcome to dc-3ddesignapp!"));
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // Description
    QLabel* desc = new QLabel(tr(
        "Your professional Scan-to-CAD solution.\n\n"
        "This quick tour will introduce you to the main features:\n\n"
        "• Import 3D scans and meshes\n"
        "• Clean up and process mesh data\n"
        "• Create 2D sketches and surfaces\n"
        "• Export to CAD formats\n\n"
        "Let's get started!"
    ));
    desc->setObjectName("pageDescription");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    layout->addStretch();
    return page;
}

QWidget* GettingStartedDialog::createImportPage()
{
    QWidget* page = new QWidget();
    page->setObjectName("tutorialPage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 20);
    layout->setSpacing(20);

    QLabel* title = new QLabel(tr("Step 1: Import Your Data"));
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* desc = new QLabel(tr(
        "<b>Getting your 3D data into the app:</b><br><br>"
        "<b>Mesh Import (Ctrl+I)</b><br>"
        "Import STL, OBJ, or PLY files from 3D scanners or other software.<br><br>"
        "<b>CAD Import (Ctrl+Shift+I)</b><br>"
        "Import STEP or IGES files for reference geometry.<br><br>"
        "<b>Supported Formats:</b><br>"
        "• STL (Binary & ASCII)<br>"
        "• OBJ (with materials)<br>"
        "• PLY (with vertex colors)<br>"
        "• STEP / IGES (CAD)<br><br>"
        "<i>Tip: Drag and drop files directly into the viewport!</i>"
    ));
    desc->setObjectName("pageDescription");
    desc->setAlignment(Qt::AlignLeft);
    desc->setWordWrap(true);
    desc->setTextFormat(Qt::RichText);
    layout->addWidget(desc);

    layout->addStretch();
    return page;
}

QWidget* GettingStartedDialog::createMeshToolsPage()
{
    QWidget* page = new QWidget();
    page->setObjectName("tutorialPage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 20);
    layout->setSpacing(20);

    QLabel* title = new QLabel(tr("Step 2: Clean Up Your Mesh"));
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* desc = new QLabel(tr(
        "<b>Prepare your mesh for CAD conversion:</b><br><br>"
        "<b>Polygon Reduction (Ctrl+Shift+R)</b><br>"
        "Simplify large meshes while preserving shape. Great for scanned data.<br><br>"
        "<b>Smoothing (Ctrl+Shift+M)</b><br>"
        "Remove noise and bumps from scan data.<br><br>"
        "<b>Fill Holes (Ctrl+Shift+H)</b><br>"
        "Automatically detect and fill gaps in your mesh.<br><br>"
        "<b>Clipping Box (Ctrl+Shift+B)</b><br>"
        "Isolate regions of interest by clipping away unwanted parts.<br><br>"
        "<i>Tip: Use the Properties Panel (F3) to see mesh statistics!</i>"
    ));
    desc->setObjectName("pageDescription");
    desc->setAlignment(Qt::AlignLeft);
    desc->setWordWrap(true);
    desc->setTextFormat(Qt::RichText);
    layout->addWidget(desc);

    layout->addStretch();
    return page;
}

QWidget* GettingStartedDialog::createSketchPage()
{
    QWidget* page = new QWidget();
    page->setObjectName("tutorialPage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 20);
    layout->setSpacing(20);

    QLabel* title = new QLabel(tr("Step 3: Create Surfaces"));
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* desc = new QLabel(tr(
        "<b>Build CAD geometry from your mesh:</b><br><br>"
        "<b>2D Sketch (K)</b><br>"
        "Create constrained sketches on planes or mesh faces.<br>"
        "Draw lines, arcs, splines, and add dimensions.<br><br>"
        "<b>Extrude (E)</b><br>"
        "Push sketch profiles into 3D solid or surface geometry.<br><br>"
        "<b>Revolve (R)</b><br>"
        "Spin a sketch profile around an axis.<br><br>"
        "<b>Surface Fitting</b><br>"
        "Fit analytical surfaces (planes, cylinders) to mesh regions.<br><br>"
        "<i>Tip: Section planes (S) help you trace mesh cross-sections!</i>"
    ));
    desc->setObjectName("pageDescription");
    desc->setAlignment(Qt::AlignLeft);
    desc->setWordWrap(true);
    desc->setTextFormat(Qt::RichText);
    layout->addWidget(desc);

    layout->addStretch();
    return page;
}

QWidget* GettingStartedDialog::createExportPage()
{
    QWidget* page = new QWidget();
    page->setObjectName("tutorialPage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 20);
    layout->setSpacing(20);

    QLabel* title = new QLabel(tr("Step 4: Export Your Work"));
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* desc = new QLabel(tr(
        "<b>Save your results:</b><br><br>"
        "<b>Export Mesh (Ctrl+E)</b><br>"
        "Save processed meshes as STL, OBJ, or PLY.<br><br>"
        "<b>Export CAD</b><br>"
        "Export surfaces and bodies to STEP or IGES for use in CAD software.<br><br>"
        "<b>Project Files</b><br>"
        "Save your complete project (Ctrl+S) to continue later.<br><br>"
        "<hr><br>"
        "<b>Need help?</b><br>"
        "• Press <b>Shift+F1</b> then click any button to see what it does<br>"
        "• Use <b>Help → Keyboard Shortcuts</b> to see all hotkeys<br>"
        "• Hover over buttons to see tooltips<br><br>"
        "<i>You're ready to go! Happy modeling!</i>"
    ));
    desc->setObjectName("pageDescription");
    desc->setAlignment(Qt::AlignLeft);
    desc->setWordWrap(true);
    desc->setTextFormat(Qt::RichText);
    layout->addWidget(desc);

    layout->addStretch();
    return page;
}

void GettingStartedDialog::onNextClicked()
{
    int current = m_stackedWidget->currentIndex();
    if (current < m_stackedWidget->count() - 1) {
        m_stackedWidget->setCurrentIndex(current + 1);
        updateNavigation();
    } else {
        accept();  // Last page - close dialog
    }
}

void GettingStartedDialog::onPrevClicked()
{
    int current = m_stackedWidget->currentIndex();
    if (current > 0) {
        m_stackedWidget->setCurrentIndex(current - 1);
        updateNavigation();
    }
}

void GettingStartedDialog::onSkipClicked()
{
    accept();
}

void GettingStartedDialog::updateNavigation()
{
    int current = m_stackedWidget->currentIndex();
    int total = m_stackedWidget->count();
    
    m_pageIndicator->setText(tr("%1 of %2").arg(current + 1).arg(total));
    m_prevButton->setEnabled(current > 0);
    m_nextButton->setText(current == total - 1 ? tr("Get Started!") : tr("Next →"));
}

void GettingStartedDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
        }
        
        QWidget#tutorialPage {
            background-color: #242424;
        }
        
        QLabel#pageTitle {
            color: #ffffff;
            font-size: 22px;
            font-weight: bold;
        }
        
        QLabel#pageDescription {
            color: #b3b3b3;
            font-size: 14px;
            line-height: 1.5;
        }
        
        QLabel#pageIndicator {
            color: #808080;
            font-size: 12px;
        }
        
        QWidget#navBar {
            background-color: #2d2d2d;
            border-top: 1px solid #4a4a4a;
        }
        
        QCheckBox {
            color: #808080;
            font-size: 12px;
        }
        
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
        }
        
        QCheckBox::indicator:checked {
            background-color: #0078d4;
            border-radius: 3px;
        }
        
        QCheckBox::indicator:unchecked {
            background-color: #333333;
            border: 1px solid #4a4a4a;
            border-radius: 3px;
        }
        
        QPushButton#primaryButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 20px;
            font-size: 13px;
            font-weight: 500;
            min-width: 100px;
        }
        
        QPushButton#primaryButton:hover {
            background-color: #1a88e0;
        }
        
        QPushButton#navButton {
            background-color: transparent;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 13px;
            min-width: 90px;
        }
        
        QPushButton#navButton:hover {
            background-color: #383838;
            color: #ffffff;
        }
        
        QPushButton#navButton:disabled {
            color: #5c5c5c;
            border-color: #333333;
        }
        
        QPushButton#skipButton {
            background-color: transparent;
            color: #808080;
            border: none;
            padding: 8px 16px;
            font-size: 12px;
        }
        
        QPushButton#skipButton:hover {
            color: #b3b3b3;
        }
    )");
}
