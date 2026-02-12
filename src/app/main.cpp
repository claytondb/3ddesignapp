/**
 * @file main.cpp
 * @brief Application entry point for dc-3ddesignapp
 * 
 * Initializes Qt application, creates the main window, and wires up
 * all components for the integrated 3D design application.
 */

#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>

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
    
    // Set default OpenGL format for the entire application
    QSurfaceFormat format;
    format.setVersion(4, 1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);  // MSAA
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);
    
    // Create Qt application
    QApplication app(argc, argv);
    
    qDebug() << "Starting dc-3ddesignapp v0.1.0";
    
    // Initialize application services (singleton)
    dc3d::Application application;
    if (!application.initialize()) {
        qCritical() << "Failed to initialize application services";
        return 1;
    }
    
    // Create and show main window
    MainWindow mainWindow;
    
    // Connect application to main window
    application.setMainWindow(&mainWindow);
    
    mainWindow.show();
    mainWindow.setStatusMessage("Ready - Use File > Import to load a mesh (STL, OBJ, PLY)");
    
    qDebug() << "Application started successfully";
    
    // Run event loop
    int result = app.exec();
    
    // Shutdown application services
    application.shutdown();
    
    qDebug() << "Application exited with code" << result;
    return result;
}
