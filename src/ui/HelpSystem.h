#ifndef DC_HELP_SYSTEM_H
#define DC_HELP_SYSTEM_H

#include <QObject>
#include <QAction>
#include <QWidget>

class QToolButton;
class QPushButton;
class QCheckBox;
class QSlider;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;

/**
 * @brief Centralized help system for the application
 * 
 * Provides:
 * - What's This mode (Shift+F1)
 * - Tooltip and WhatsThis text setup
 * - Context help for dialogs
 */
class HelpSystem : public QObject
{
    Q_OBJECT

public:
    static HelpSystem* instance();
    
    /**
     * @brief Enter What's This mode - next click shows help
     */
    void enterWhatsThisMode();
    
    /**
     * @brief Install What's This mode shortcut on a widget
     */
    void installShortcut(QWidget* widget);
    
    /**
     * @brief Set comprehensive help on a widget (tooltip + whatsthis + status tip)
     * @param widget The widget to configure
     * @param tooltip Short description shown on hover
     * @param whatsThis Detailed help shown in What's This mode
     */
    static void setHelp(QWidget* widget, const QString& tooltip, const QString& whatsThis = QString());
    
    /**
     * @brief Set comprehensive help on an action
     */
    static void setHelp(QAction* action, const QString& tooltip, const QString& whatsThis = QString());
    
    /**
     * @brief Add context help button to a dialog
     * @return The help button that was added
     */
    static QPushButton* addContextHelpButton(QWidget* dialog, const QString& helpText);
    
    /**
     * @brief Show context help for a dialog
     */
    static void showContextHelp(QWidget* parent, const QString& title, const QString& helpText);

private:
    explicit HelpSystem(QObject* parent = nullptr);
    static HelpSystem* s_instance;
};

/**
 * @brief Helper namespace for common help texts
 */
namespace HelpText {

// File operations
QString newProject();
QString openProject();
QString saveProject();
QString importMesh();
QString exportMesh();

// Mesh tools
QString polygonReduction();
QString smoothing();
QString fillHoles();
QString clippingBox();
QString removeOutliers();

// Selection modes
QString selectMode();
QString boxSelectMode();
QString lassoSelectMode();
QString brushSelectMode();

// View modes
QString shadedMode();
QString wireframeMode();
QString shadedWireMode();
QString xrayMode();

// Create tools
QString createPlane();
QString createCylinder();
QString sectionPlane();
QString sketch2D();
QString extrude();
QString revolve();

// Properties panel
QString transformPosition();
QString transformRotation();
QString meshOpacity();

} // namespace HelpText

#endif // DC_HELP_SYSTEM_H
