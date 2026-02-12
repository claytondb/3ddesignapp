/**
 * @file main.cpp
 * @brief Application entry point for dc-3ddesignapp
 * 
 * Initializes Qt application and creates the main window.
 */

#include <QApplication>
#include "Application.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    // Set application attributes before creating QApplication
    QApplication::setApplicationName("dc-3ddesignapp");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("DC3DDesign");
    
    // Enable high-DPI scaling
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    
    // Create Qt application
    QApplication app(argc, argv);
    
    // Initialize application services
    dc3d::Application application;
    if (!application.initialize()) {
        return 1;
    }
    
    // Create and show main window
    dc3d::ui::MainWindow mainWindow;
    mainWindow.show();
    
    // Run event loop
    return app.exec();
}
