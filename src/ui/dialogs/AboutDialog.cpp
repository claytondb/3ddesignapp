#include "AboutDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QApplication>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About %1").arg(appName()));
    setFixedSize(450, 380);
    setModal(true);
    
    setupUI();
    applyStylesheet();
}

void AboutDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Logo/Icon
    m_logoLabel = new QLabel();
    QIcon appIcon = QApplication::style()->standardIcon(QStyle::SP_DesktopIcon);
    m_logoLabel->setPixmap(appIcon.pixmap(64, 64));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_logoLabel);

    // Application name
    m_nameLabel = new QLabel(appName());
    m_nameLabel->setObjectName("appName");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_nameLabel);

    // Version
    m_versionLabel = new QLabel(tr("Version %1").arg(appVersion()));
    m_versionLabel->setObjectName("version");
    m_versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_versionLabel);

    // Description
    m_descriptionLabel = new QLabel(appDescription());
    m_descriptionLabel->setObjectName("description");
    m_descriptionLabel->setAlignment(Qt::AlignCenter);
    m_descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(m_descriptionLabel);

    mainLayout->addSpacing(10);

    // Credits
    QString credits = tr(
        "<p><b>Key Features:</b></p>"
        "<ul>"
        "<li>Import and visualize 3D scan data (STL, OBJ, PLY)</li>"
        "<li>Mesh processing: reduction, smoothing, hole filling</li>"
        "<li>Surface fitting and CAD export (STEP, IGES)</li>"
        "<li>2D/3D sketching with constraints</li>"
        "<li>Deviation analysis and quality inspection</li>"
        "</ul>"
        "<p style='color: #808080;'>Built with Qt and OpenGL</p>"
    );
    m_creditsLabel = new QLabel(credits);
    m_creditsLabel->setObjectName("credits");
    m_creditsLabel->setAlignment(Qt::AlignLeft);
    m_creditsLabel->setWordWrap(true);
    m_creditsLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(m_creditsLabel);

    mainLayout->addStretch();

    // Copyright
    QLabel* copyright = new QLabel(tr("© 2024 dc-3ddesignapp Project"));
    copyright->setObjectName("copyright");
    copyright->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(copyright);

    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setObjectName("primaryButton");
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);
    
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
}

void AboutDialog::applyStylesheet()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
            color: #ffffff;
        }
        
        QLabel#appName {
            color: #ffffff;
            font-size: 24px;
            font-weight: bold;
        }
        
        QLabel#version {
            color: #0078d4;
            font-size: 14px;
        }
        
        QLabel#description {
            color: #b3b3b3;
            font-size: 13px;
        }
        
        QLabel#credits {
            color: #b3b3b3;
            font-size: 12px;
            background-color: #242424;
            border-radius: 4px;
            padding: 12px;
        }
        
        QLabel#copyright {
            color: #666666;
            font-size: 11px;
        }
        
        QPushButton#primaryButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 32px;
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
    )");
}
