#include "HelpSystem.h"
#include <QWhatsThis>
#include <QShortcut>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QStyle>
#include <QApplication>

HelpSystem* HelpSystem::s_instance = nullptr;

HelpSystem* HelpSystem::instance()
{
    if (!s_instance) {
        s_instance = new HelpSystem();
    }
    return s_instance;
}

HelpSystem::HelpSystem(QObject* parent)
    : QObject(parent)
{
}

void HelpSystem::enterWhatsThisMode()
{
    QWhatsThis::enterWhatsThisMode();
}

void HelpSystem::installShortcut(QWidget* widget)
{
    QShortcut* shortcut = new QShortcut(QKeySequence("Shift+F1"), widget);
    connect(shortcut, &QShortcut::activated, this, &HelpSystem::enterWhatsThisMode);
}

void HelpSystem::setHelp(QWidget* widget, const QString& tooltip, const QString& whatsThis)
{
    if (!widget) return;
    
    widget->setToolTip(tooltip);
    widget->setStatusTip(tooltip);
    
    if (!whatsThis.isEmpty()) {
        widget->setWhatsThis(whatsThis);
    } else {
        widget->setWhatsThis(tooltip);
    }
}

void HelpSystem::setHelp(QAction* action, const QString& tooltip, const QString& whatsThis)
{
    if (!action) return;
    
    // Include shortcut in tooltip if present
    QString fullTooltip = tooltip;
    if (!action->shortcut().isEmpty()) {
        fullTooltip += QString(" (%1)").arg(action->shortcut().toString());
    }
    
    action->setToolTip(fullTooltip);
    action->setStatusTip(tooltip);
    
    if (!whatsThis.isEmpty()) {
        action->setWhatsThis(whatsThis);
    } else {
        action->setWhatsThis(tooltip);
    }
}

QPushButton* HelpSystem::addContextHelpButton(QWidget* dialog, const QString& helpText)
{
    QPushButton* helpButton = new QPushButton("?");
    helpButton->setFixedSize(24, 24);
    helpButton->setToolTip(QObject::tr("Click for help about this dialog"));
    helpButton->setStyleSheet(R"(
        QPushButton {
            background-color: #333333;
            color: #b3b3b3;
            border: 1px solid #4a4a4a;
            border-radius: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #0078d4;
            color: #ffffff;
            border-color: #0078d4;
        }
    )");
    
    QObject::connect(helpButton, &QPushButton::clicked, [dialog, helpText]() {
        showContextHelp(dialog, QObject::tr("Help"), helpText);
    });
    
    return helpButton;
}

void HelpSystem::showContextHelp(QWidget* parent, const QString& title, const QString& helpText)
{
    QDialog* helpDialog = new QDialog(parent);
    helpDialog->setWindowTitle(title);
    helpDialog->setMinimumWidth(400);
    helpDialog->setModal(true);
    
    QVBoxLayout* layout = new QVBoxLayout(helpDialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    QLabel* content = new QLabel(helpText);
    content->setWordWrap(true);
    content->setTextFormat(Qt::RichText);
    content->setStyleSheet("color: #b3b3b3; font-size: 13px; line-height: 1.5;");
    layout->addWidget(content);
    
    QPushButton* closeButton = new QPushButton(QObject::tr("Got it"));
    closeButton->setDefault(true);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 8px 24px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #1a88e0;
        }
    )");
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);
    
    QObject::connect(closeButton, &QPushButton::clicked, helpDialog, &QDialog::accept);
    
    helpDialog->setStyleSheet(R"(
        QDialog {
            background-color: #2d2d2d;
        }
    )");
    
    helpDialog->exec();
    helpDialog->deleteLater();
}

// ============================================================================
// Help Text Namespace Implementation
// ============================================================================

namespace HelpText {

QString newProject()
{
    return QObject::tr(
        "<b>New Project</b><br><br>"
        "Creates a new empty project, clearing the current workspace.<br><br>"
        "If you have unsaved changes, you will be prompted to save them first.<br><br>"
        "<b>Shortcut:</b> Ctrl+N"
    );
}

QString openProject()
{
    return QObject::tr(
        "<b>Open Project</b><br><br>"
        "Opens an existing project file (.dc3d) or imports a mesh file directly.<br><br>"
        "<b>Supported formats:</b><br>"
        "• Project files (.dc3d)<br>"
        "• STL, OBJ, PLY mesh files<br><br>"
        "<b>Shortcut:</b> Ctrl+O"
    );
}

QString saveProject()
{
    return QObject::tr(
        "<b>Save Project</b><br><br>"
        "Saves the current project to disk. If this is a new project, "
        "you will be prompted to choose a location.<br><br>"
        "Projects are saved as .dc3d files and include all meshes, surfaces, "
        "sketches, and settings.<br><br>"
        "<b>Shortcut:</b> Ctrl+S"
    );
}

QString importMesh()
{
    return QObject::tr(
        "<b>Import Mesh</b><br><br>"
        "Import 3D mesh data from various file formats.<br><br>"
        "<b>Supported formats:</b><br>"
        "• STL (Stereolithography) - binary and ASCII<br>"
        "• OBJ (Wavefront) - with materials and textures<br>"
        "• PLY (Polygon File Format) - with vertex colors<br><br>"
        "<b>Tips:</b><br>"
        "• Large meshes may take time to load<br>"
        "• Use Polygon Reduction if the mesh is too dense<br>"
        "• Drag and drop files directly into the viewport<br><br>"
        "<b>Shortcut:</b> Ctrl+I"
    );
}

QString exportMesh()
{
    return QObject::tr(
        "<b>Export Mesh</b><br><br>"
        "Export the selected mesh to a file.<br><br>"
        "<b>Export options:</b><br>"
        "• STL binary (smaller file size)<br>"
        "• STL ASCII (human-readable)<br>"
        "• OBJ with materials<br>"
        "• PLY with vertex colors<br><br>"
        "<b>Note:</b> Only the currently selected mesh(es) will be exported.<br><br>"
        "<b>Shortcut:</b> Ctrl+E"
    );
}

QString polygonReduction()
{
    return QObject::tr(
        "<b>Polygon Reduction</b><br><br>"
        "Reduces the number of triangles in a mesh while preserving its shape. "
        "Essential for working with large scanned data.<br><br>"
        "<b>Options:</b><br>"
        "• <b>Percentage:</b> Reduce to X% of original triangles<br>"
        "• <b>Vertex/Face count:</b> Reduce to specific number<br>"
        "• <b>Preserve boundaries:</b> Keep mesh edges intact<br>"
        "• <b>Preserve sharp edges:</b> Protect corners and creases<br><br>"
        "<b>When to use:</b><br>"
        "• Scanned meshes are often too dense for editing<br>"
        "• Faster rendering and manipulation<br>"
        "• Before exporting for 3D printing or CAD<br><br>"
        "<b>Shortcut:</b> Ctrl+Shift+R"
    );
}

QString smoothing()
{
    return QObject::tr(
        "<b>Mesh Smoothing</b><br><br>"
        "Smooths the mesh surface to reduce noise, bumps, and scan artifacts.<br><br>"
        "<b>Methods:</b><br>"
        "• <b>Laplacian:</b> Simple averaging, fast<br>"
        "• <b>Taubin:</b> Reduces shrinkage, better for features<br>"
        "• <b>Bilateral:</b> Preserves edges while smoothing<br><br>"
        "<b>Parameters:</b><br>"
        "• <b>Iterations:</b> More passes = smoother result<br>"
        "• <b>Strength:</b> How much to smooth per iteration<br><br>"
        "<b>Tip:</b> Start with low iterations and increase gradually.<br><br>"
        "<b>Shortcut:</b> Ctrl+Shift+M"
    );
}

QString fillHoles()
{
    return QObject::tr(
        "<b>Fill Holes</b><br><br>"
        "Automatically detects and fills holes in the mesh.<br><br>"
        "<b>Fill methods:</b><br>"
        "• <b>Flat:</b> Simple planar fill<br>"
        "• <b>Smooth:</b> Curved fill that blends with surroundings<br>"
        "• <b>Curvature-based:</b> Continues the surface curvature<br><br>"
        "<b>Options:</b><br>"
        "• Set maximum hole size to fill<br>"
        "• Preview holes before filling<br>"
        "• Fill all holes or select specific ones<br><br>"
        "<b>When to use:</b><br>"
        "• Scanned data often has gaps from occlusion<br>"
        "• Required for watertight meshes (3D printing)<br><br>"
        "<b>Shortcut:</b> Ctrl+Shift+H"
    );
}

QString clippingBox()
{
    return QObject::tr(
        "<b>Clipping Box</b><br><br>"
        "Creates a 3D box that clips (hides or removes) parts of the mesh outside it.<br><br>"
        "<b>Usage:</b><br>"
        "1. Enable clipping box<br>"
        "2. Drag box handles to resize<br>"
        "3. Drag faces to move the box<br>"
        "4. Choose to hide or delete clipped regions<br><br>"
        "<b>Great for:</b><br>"
        "• Isolating regions of interest<br>"
        "• Removing scan artifacts at edges<br>"
        "• Focusing on specific parts of large scans<br><br>"
        "<b>Shortcut:</b> Ctrl+Shift+B"
    );
}

QString removeOutliers()
{
    return QObject::tr(
        "<b>Remove Outliers</b><br><br>"
        "Removes isolated points and small disconnected mesh regions.<br><br>"
        "<b>Detection methods:</b><br>"
        "• <b>Statistical:</b> Points far from neighbors<br>"
        "• <b>Radius:</b> Points with few neighbors in radius<br>"
        "• <b>Component size:</b> Small disconnected regions<br><br>"
        "<b>When to use:</b><br>"
        "• Clean up scan noise and floating points<br>"
        "• Remove small debris fragments<br>"
        "• Prepare mesh for surface fitting"
    );
}

QString selectMode()
{
    return QObject::tr(
        "<b>Select Mode</b><br><br>"
        "Click to select individual objects in the viewport.<br><br>"
        "<b>Modifiers:</b><br>"
        "• <b>Click:</b> Select single object (clears previous selection)<br>"
        "• <b>Shift+Click:</b> Add to selection<br>"
        "• <b>Ctrl+Click:</b> Toggle selection<br>"
        "• <b>Click empty:</b> Deselect all<br><br>"
        "<b>Shortcut:</b> Q"
    );
}

QString boxSelectMode()
{
    return QObject::tr(
        "<b>Box Select Mode</b><br><br>"
        "Draw a rectangle to select multiple objects.<br><br>"
        "<b>Usage:</b><br>"
        "1. Click and drag to draw selection rectangle<br>"
        "2. All objects inside/touching the box are selected<br><br>"
        "<b>Modifiers:</b><br>"
        "• <b>Shift:</b> Add to existing selection<br>"
        "• <b>Ctrl:</b> Remove from selection<br><br>"
        "<b>Shortcut:</b> B"
    );
}

QString lassoSelectMode()
{
    return QObject::tr(
        "<b>Lasso Select Mode</b><br><br>"
        "Draw a freehand shape to select objects inside it.<br><br>"
        "<b>Usage:</b><br>"
        "1. Click and drag to draw a freehand selection boundary<br>"
        "2. Objects completely inside the lasso are selected<br><br>"
        "<b>Tip:</b> Great for selecting irregular groups of objects.<br><br>"
        "<b>Shortcut:</b> L"
    );
}

QString brushSelectMode()
{
    return QObject::tr(
        "<b>Brush Select Mode</b><br><br>"
        "Paint to select faces or vertices directly on the mesh.<br><br>"
        "<b>Usage:</b><br>"
        "• Click and drag to paint selection<br>"
        "• Scroll wheel to change brush size<br>"
        "• Shift+drag to deselect<br><br>"
        "<b>Great for:</b><br>"
        "• Selecting specific mesh regions<br>"
        "• Organic selection patterns<br>"
        "• Face-level editing operations"
    );
}

QString shadedMode()
{
    return QObject::tr(
        "<b>Shaded Display Mode</b><br><br>"
        "Shows surfaces with realistic lighting and materials.<br><br>"
        "This is the default view mode, showing solid surfaces "
        "with shadows and highlights for depth perception.<br><br>"
        "<b>Shortcut:</b> Alt+1"
    );
}

QString wireframeMode()
{
    return QObject::tr(
        "<b>Wireframe Display Mode</b><br><br>"
        "Shows only mesh edges without filled surfaces.<br><br>"
        "<b>Use when:</b><br>"
        "• Checking mesh topology and edge flow<br>"
        "• Seeing through objects to internal geometry<br>"
        "• Evaluating mesh density<br><br>"
        "<b>Shortcut:</b> Alt+2"
    );
}

QString shadedWireMode()
{
    return QObject::tr(
        "<b>Shaded + Wireframe Mode</b><br><br>"
        "Shows solid surfaces with wireframe overlay.<br><br>"
        "Combines the benefits of both modes - see the surface "
        "appearance while also viewing mesh structure.<br><br>"
        "<b>Shortcut:</b> Alt+3"
    );
}

QString xrayMode()
{
    return QObject::tr(
        "<b>X-Ray Display Mode</b><br><br>"
        "Makes surfaces semi-transparent to see through them.<br><br>"
        "<b>Use when:</b><br>"
        "• Selecting objects behind other objects<br>"
        "• Viewing internal structures<br>"
        "• Working with overlapping meshes<br><br>"
        "<b>Shortcut:</b> Alt+4"
    );
}

QString createPlane()
{
    return QObject::tr(
        "<b>Create Plane</b><br><br>"
        "Creates a reference plane in the scene.<br><br>"
        "<b>Uses:</b><br>"
        "• Sketch on it (2D sketch mode)<br>"
        "• Use as section plane<br>"
        "• Reference for alignment<br><br>"
        "<b>Placement:</b><br>"
        "• Click in viewport to place<br>"
        "• Select a face to create plane on it<br>"
        "• Use standard views for aligned planes<br><br>"
        "<b>Shortcut:</b> P"
    );
}

QString createCylinder()
{
    return QObject::tr(
        "<b>Create Cylinder</b><br><br>"
        "Creates a cylinder primitive in the scene.<br><br>"
        "<b>Interactive creation:</b><br>"
        "1. Click to place center point<br>"
        "2. Drag to set radius<br>"
        "3. Click again to set height<br><br>"
        "<b>Or:</b> Use Properties Panel to set exact dimensions.<br><br>"
        "<b>Shortcut:</b> C"
    );
}

QString sectionPlane()
{
    return QObject::tr(
        "<b>Section Plane</b><br><br>"
        "Creates a cutting plane to see inside meshes.<br><br>"
        "<b>Features:</b><br>"
        "• Drag to move cut location<br>"
        "• Rotate to change cut angle<br>"
        "• Extract 2D section curves<br>"
        "• Create multiple parallel sections<br><br>"
        "<b>Great for:</b><br>"
        "• Inspecting internal geometry<br>"
        "• Extracting profiles for sketching<br>"
        "• Creating cross-section views<br><br>"
        "<b>Shortcut:</b> S"
    );
}

QString sketch2D()
{
    return QObject::tr(
        "<b>2D Sketch</b><br><br>"
        "Start a 2D sketch on a plane or face.<br><br>"
        "<b>Sketch tools:</b><br>"
        "• Line, Arc, Spline drawing<br>"
        "• Rectangle, Circle, Polygon<br>"
        "• Dimensions and constraints<br>"
        "• Trim, Extend, Offset<br><br>"
        "<b>Workflow:</b><br>"
        "1. Select a plane or face<br>"
        "2. Draw your profile<br>"
        "3. Add dimensions and constraints<br>"
        "4. Exit sketch and use Extrude/Revolve<br><br>"
        "<b>Shortcut:</b> K"
    );
}

QString extrude()
{
    return QObject::tr(
        "<b>Extrude</b><br><br>"
        "Pushes a 2D sketch profile into 3D geometry.<br><br>"
        "<b>Options:</b><br>"
        "• <b>Distance:</b> Fixed extrusion length<br>"
        "• <b>To face:</b> Extrude until hitting a surface<br>"
        "• <b>Symmetric:</b> Extrude both directions<br>"
        "• <b>Draft angle:</b> Tapered extrusion<br><br>"
        "<b>Result types:</b><br>"
        "• Solid body<br>"
        "• Surface<br>"
        "• Add/Cut from existing body<br><br>"
        "<b>Shortcut:</b> E"
    );
}

QString revolve()
{
    return QObject::tr(
        "<b>Revolve</b><br><br>"
        "Spins a 2D sketch profile around an axis to create 3D geometry.<br><br>"
        "<b>Options:</b><br>"
        "• <b>Angle:</b> 0-360° revolution<br>"
        "• <b>Axis:</b> X, Y, Z, or sketch line<br><br>"
        "<b>Great for:</b><br>"
        "• Round shapes (cups, bottles, wheels)<br>"
        "• Turned parts (shafts, knobs)<br>"
        "• Symmetric geometry<br><br>"
        "<b>Shortcut:</b> R"
    );
}

QString transformPosition()
{
    return QObject::tr(
        "<b>Position (Transform)</b><br><br>"
        "The X, Y, Z coordinates of the object's position in space.<br><br>"
        "<b>Usage:</b><br>"
        "• Type exact values for precise positioning<br>"
        "• Use arrow keys to increment<br>"
        "• Values are in current units (see Coordinate System)<br><br>"
        "<b>Tip:</b> Hold Shift while dragging in viewport for constrained movement."
    );
}

QString transformRotation()
{
    return QObject::tr(
        "<b>Rotation (Transform)</b><br><br>"
        "The rotation angles around X, Y, Z axes in degrees.<br><br>"
        "<b>Usage:</b><br>"
        "• Type exact angles for precise rotation<br>"
        "• Values are -360° to +360°<br>"
        "• Rotations are applied in X→Y→Z order<br><br>"
        "<b>Tip:</b> Hold Ctrl while rotating in viewport for 15° snapping."
    );
}

QString meshOpacity()
{
    return QObject::tr(
        "<b>Mesh Opacity</b><br><br>"
        "Controls how transparent the mesh appears.<br><br>"
        "• 100%: Fully opaque (solid)<br>"
        "• 50%: Semi-transparent<br>"
        "• 0%: Fully transparent (invisible)<br><br>"
        "<b>Use when:</b><br>"
        "• Seeing through to geometry behind<br>"
        "• Comparing overlapping meshes<br>"
        "• Checking internal fit"
    );
}

} // namespace HelpText
