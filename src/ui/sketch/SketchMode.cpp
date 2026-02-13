/**
 * @file SketchMode.cpp
 * @brief Implementation of 2D Sketch mode controller
 */

#include "SketchMode.h"
#include "SketchToolbox.h"
#include "SketchViewport.h"
#include "LineTool.h"
#include "ArcTool.h"
#include "SplineTool.h"
#include "DimensionTool.h"
#include "../Viewport.h"

#include <QKeyEvent>
#include <QDebug>
#include <cmath>

namespace dc {

// ============================================================================
// Constructor / Destructor
// ============================================================================

SketchMode::SketchMode(Viewport* viewport, QObject* parent)
    : QObject(parent)
    , m_viewport(viewport)
    , m_toolbox(nullptr)
    , m_viewportOverlay(nullptr)
{
    // Create UI components
    m_toolbox = new SketchToolbox(nullptr);
    m_viewportOverlay = new SketchViewport(viewport);
    
    // Connect toolbox signals
    connect(m_toolbox, &SketchToolbox::toolSelected, this, [this](SketchToolType type) {
        setActiveTool(type);
    });
    
    connect(m_toolbox, &SketchToolbox::exitRequested, this, [this]() {
        exitSketchMode(true);
    });
    
    // Initially hide UI components
    m_toolbox->hide();
    m_viewportOverlay->hide();
}

SketchMode::~SketchMode()
{
    if (m_active) {
        exitSketchMode(false);
    }
    
    destroyTools();
    
    delete m_toolbox;  // OK - nullptr parent
    // Don't delete m_viewportOverlay - it's owned by viewport (Qt parent-child ownership)
}

// ============================================================================
// Mode State
// ============================================================================

void SketchMode::enterSketchMode(SketchPlaneType planeType, float offset)
{
    QVector3D origin, normal, xAxis;
    
    switch (planeType) {
        case SketchPlaneType::XY:
            origin = QVector3D(0, 0, offset);
            normal = QVector3D(0, 0, 1);
            xAxis = QVector3D(1, 0, 0);
            break;
        case SketchPlaneType::XZ:
            origin = QVector3D(0, offset, 0);
            normal = QVector3D(0, 1, 0);
            xAxis = QVector3D(1, 0, 0);
            break;
        case SketchPlaneType::YZ:
            origin = QVector3D(offset, 0, 0);
            normal = QVector3D(1, 0, 0);
            xAxis = QVector3D(0, 1, 0);
            break;
        default:
            origin = QVector3D(0, 0, 0);
            normal = QVector3D(0, 0, 1);
            xAxis = QVector3D(1, 0, 0);
            break;
    }
    
    enterSketchMode(origin, normal, xAxis);
}

void SketchMode::enterSketchMode(const QVector3D& origin, const QVector3D& normal, const QVector3D& xAxis)
{
    if (m_active) {
        exitSketchMode(false);
    }
    
    m_active = true;
    
    // Create new sketch data
    m_sketchData = std::make_shared<dc3d::geometry::SketchData>();
    m_sketchPlane = std::make_shared<dc3d::geometry::SketchPlane>();
    m_sketchPlane->origin = origin;
    m_sketchPlane->normal = normal.normalized();
    m_sketchPlane->xAxis = xAxis.normalized();
    m_sketchPlane->yAxis = QVector3D::crossProduct(normal, xAxis).normalized();
    
    // Setup viewport
    setupViewport();
    
    // Create tools
    createTools();
    
    // Show UI
    m_toolbox->show();
    m_viewportOverlay->show();
    m_viewportOverlay->setSketchPlane(m_sketchPlane);
    m_viewportOverlay->setGridSettings(m_gridSettings);
    
    // Set default tool
    setActiveTool(SketchToolType::Line);
    
    // Look at sketch plane
    lookAtSketchPlane();
    
    emit modeChanged(true);
    
    qDebug() << "Entered sketch mode, plane origin:" << origin 
             << "normal:" << normal;
}

void SketchMode::editSketch(std::shared_ptr<dc3d::geometry::SketchData> sketch)
{
    if (m_active) {
        exitSketchMode(false);
    }
    
    if (!sketch || !sketch->plane) {
        qWarning() << "Cannot edit null sketch or sketch without plane";
        return;
    }
    
    m_active = true;
    m_sketchData = sketch;
    m_sketchPlane = sketch->plane;
    
    // Setup viewport
    setupViewport();
    
    // Create tools
    createTools();
    
    // Show UI
    m_toolbox->show();
    m_viewportOverlay->show();
    m_viewportOverlay->setSketchPlane(m_sketchPlane);
    m_viewportOverlay->setSketchData(m_sketchData);
    m_viewportOverlay->setGridSettings(m_gridSettings);
    
    // Set default tool
    setActiveTool(SketchToolType::Select);
    
    // Look at sketch plane
    lookAtSketchPlane();
    
    emit modeChanged(true);
}

void SketchMode::exitSketchMode(bool save)
{
    if (!m_active) return;
    
    // Cancel any active operation
    cancelCurrentOperation();
    
    // Save sketch if requested
    if (save && m_sketchData && m_sketchData->hasContent()) {
        emit saveRequested(m_sketchData);
    }
    
    // Hide UI
    m_toolbox->hide();
    m_viewportOverlay->hide();
    
    // Restore viewport
    restoreViewport();
    
    // Destroy tools
    destroyTools();
    
    // Clear data
    m_sketchData.reset();
    m_sketchPlane.reset();
    m_activeTool = nullptr;
    m_activeToolType = SketchToolType::None;
    
    m_active = false;
    
    emit modeChanged(false);
    
    qDebug() << "Exited sketch mode, save:" << save;
}

// ============================================================================
// Tool Management
// ============================================================================

void SketchMode::setActiveTool(SketchToolType toolType)
{
    if (m_activeToolType == toolType) return;
    
    // Deactivate current tool
    if (m_activeTool) {
        m_activeTool->deactivate();
    }
    
    m_activeToolType = toolType;
    
    // Find and activate new tool
    auto it = m_tools.find(toolType);
    if (it != m_tools.end()) {
        m_activeTool = it->second.get();
        m_activeTool->activate();
    } else {
        m_activeTool = nullptr;
    }
    
    // Update toolbox selection
    m_toolbox->setSelectedTool(toolType);
    
    // Update viewport cursor
    if (m_viewportOverlay) {
        m_viewportOverlay->setCurrentTool(toolType);
    }
    
    emit activeToolChanged(toolType);
    
    qDebug() << "Active sketch tool changed to:" << static_cast<int>(toolType);
}

void SketchMode::cancelCurrentOperation()
{
    if (m_activeTool) {
        m_activeTool->cancel();
    }
}

void SketchMode::finishCurrentOperation()
{
    if (m_activeTool) {
        m_activeTool->finish();
    }
}

// ============================================================================
// Grid Settings
// ============================================================================

void SketchMode::setGridSettings(const SketchGridSettings& settings)
{
    m_gridSettings = settings;
    
    if (m_viewportOverlay) {
        m_viewportOverlay->setGridSettings(settings);
    }
    
    emit gridSettingsChanged();
}

void SketchMode::setGridVisible(bool visible)
{
    m_gridSettings.visible = visible;
    
    if (m_viewportOverlay) {
        m_viewportOverlay->setGridVisible(visible);
    }
    
    emit gridSettingsChanged();
}

void SketchMode::setGridSnap(bool enabled)
{
    m_gridSettings.snapToGrid = enabled;
    emit gridSettingsChanged();
}

// ============================================================================
// Snap Settings
// ============================================================================

void SketchMode::setSnapSettings(const SketchSnapSettings& settings)
{
    m_snapSettings = settings;
    emit snapSettingsChanged();
}

// ============================================================================
// Coordinate Conversion
// ============================================================================

QPointF SketchMode::screenToSketch(const QPoint& screenPos) const
{
    if (!m_viewport || !m_sketchPlane) {
        return QPointF(screenPos);
    }
    
    // Get ray from screen position
    QVector3D rayOrigin, rayDir;
    m_viewport->screenToRay(screenPos, rayOrigin, rayDir);
    
    // Intersect with sketch plane
    float t = QVector3D::dotProduct(m_sketchPlane->origin - rayOrigin, m_sketchPlane->normal) /
              QVector3D::dotProduct(rayDir, m_sketchPlane->normal);
    
    if (std::abs(QVector3D::dotProduct(rayDir, m_sketchPlane->normal)) < 1e-6f) {
        // Ray parallel to plane
        return QPointF(0, 0);
    }
    
    QVector3D worldPoint = rayOrigin + rayDir * t;
    
    // Convert to sketch plane 2D
    QVector3D localPoint = worldPoint - m_sketchPlane->origin;
    float x = QVector3D::dotProduct(localPoint, m_sketchPlane->xAxis);
    float y = QVector3D::dotProduct(localPoint, m_sketchPlane->yAxis);
    
    return QPointF(x, y);
}

QPoint SketchMode::sketchToScreen(const QPointF& sketchPos) const
{
    if (!m_viewport || !m_sketchPlane) {
        return QPoint(static_cast<int>(sketchPos.x()), static_cast<int>(sketchPos.y()));
    }
    
    QVector3D worldPoint = sketchToWorld(sketchPos);
    return m_viewport->worldToScreen(worldPoint);
}

QVector3D SketchMode::sketchToWorld(const QPointF& sketchPos) const
{
    if (!m_sketchPlane) {
        return QVector3D(static_cast<float>(sketchPos.x()), 
                         static_cast<float>(sketchPos.y()), 0);
    }
    
    return m_sketchPlane->origin + 
           m_sketchPlane->xAxis * static_cast<float>(sketchPos.x()) +
           m_sketchPlane->yAxis * static_cast<float>(sketchPos.y());
}

QPointF SketchMode::getSnapPoint(const QPoint& screenPos, QString& snapType) const
{
    QPointF sketchPos = screenToSketch(screenPos);
    snapType.clear();
    
    if (!m_snapSettings.enabled) {
        return sketchPos;
    }
    
    float snapRadiusWorld = m_snapSettings.snapRadius;
    if (m_viewport) {
        // Convert pixel radius to world units approximately
        snapRadiusWorld = m_viewport->pixelsToWorld(m_snapSettings.snapRadius);
    }
    
    QPointF bestSnap = sketchPos;
    float bestDist = snapRadiusWorld;
    
    // Check sketch entities for snap points
    if (m_sketchData) {
        // Snap to endpoints
        if (m_snapSettings.snapToEndpoints) {
            for (const auto& entity : m_sketchData->entities) {
                auto endpoints = entity->getEndpoints();
                for (const QPointF& ep : endpoints) {
                    float dist = std::sqrt(std::pow(ep.x() - sketchPos.x(), 2) + 
                                          std::pow(ep.y() - sketchPos.y(), 2));
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestSnap = ep;
                        snapType = "Endpoint";
                    }
                }
            }
        }
        
        // Snap to midpoints
        if (m_snapSettings.snapToMidpoints) {
            for (const auto& entity : m_sketchData->entities) {
                QPointF midpoint = entity->getMidpoint();
                float dist = std::sqrt(std::pow(midpoint.x() - sketchPos.x(), 2) + 
                                      std::pow(midpoint.y() - sketchPos.y(), 2));
                if (dist < bestDist) {
                    bestDist = dist;
                    bestSnap = midpoint;
                    snapType = "Midpoint";
                }
            }
        }
        
        // Snap to center (for circles/arcs)
        if (m_snapSettings.snapToCenter) {
            for (const auto& entity : m_sketchData->entities) {
                if (entity->hasCenter()) {
                    QPointF center = entity->getCenter();
                    float dist = std::sqrt(std::pow(center.x() - sketchPos.x(), 2) + 
                                          std::pow(center.y() - sketchPos.y(), 2));
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestSnap = center;
                        snapType = "Center";
                    }
                }
            }
        }
        
        // Snap to intersections
        if (m_snapSettings.snapToIntersection) {
            auto intersections = m_sketchData->findIntersections();
            for (const QPointF& ip : intersections) {
                float dist = std::sqrt(std::pow(ip.x() - sketchPos.x(), 2) + 
                                      std::pow(ip.y() - sketchPos.y(), 2));
                if (dist < bestDist) {
                    bestDist = dist;
                    bestSnap = ip;
                    snapType = "Intersection";
                }
            }
        }
    }
    
    // Snap to grid
    if (m_snapSettings.snapToGrid && m_gridSettings.snapToGrid && snapType.isEmpty()) {
        float gridX = std::round(sketchPos.x() / m_gridSettings.majorSpacing) * m_gridSettings.majorSpacing;
        float gridY = std::round(sketchPos.y() / m_gridSettings.majorSpacing) * m_gridSettings.majorSpacing;
        
        float dist = std::sqrt(std::pow(gridX - sketchPos.x(), 2) + 
                              std::pow(gridY - sketchPos.y(), 2));
        if (dist < bestDist) {
            bestSnap = QPointF(gridX, gridY);
            snapType = "Grid";
        }
    }
    
    return bestSnap;
}

// ============================================================================
// View Control
// ============================================================================

void SketchMode::lookAtSketchPlane()
{
    if (!m_viewport || !m_sketchPlane) return;
    
    // Set orthographic projection
    m_viewport->setOrthographic(true);
    
    // Calculate view direction (opposite of plane normal)
    QVector3D viewDir = -m_sketchPlane->normal;
    
    // Calculate camera position
    float distance = 100.0f; // Default viewing distance
    QVector3D cameraPos = m_sketchPlane->origin - viewDir * distance;
    
    // Calculate up vector
    QVector3D upVector = m_sketchPlane->yAxis;
    
    // Set view
    m_viewport->lookAt(cameraPos, m_sketchPlane->origin, upVector);
}

void SketchMode::zoomToFit()
{
    if (!m_viewport || !m_sketchData) return;
    
    // Get sketch bounding box
    QRectF bounds = m_sketchData->getBoundingBox();
    if (bounds.isEmpty()) {
        bounds = QRectF(-50, -50, 100, 100); // Default bounds
    }
    
    // Add margin
    bounds.adjust(-10, -10, 10, 10);
    
    // Convert to 3D and zoom
    QVector3D min3D = sketchToWorld(bounds.topLeft());
    QVector3D max3D = sketchToWorld(bounds.bottomRight());
    
    m_viewport->zoomToBox(min3D, max3D);
}

// ============================================================================
// Undo/Redo
// ============================================================================

void SketchMode::undo()
{
    if (m_sketchData && m_sketchData->canUndo()) {
        m_sketchData->undo();
        emit sketchModified();
        emit undoStateChanged(canUndo(), canRedo());
    }
}

void SketchMode::redo()
{
    if (m_sketchData && m_sketchData->canRedo()) {
        m_sketchData->redo();
        emit sketchModified();
        emit undoStateChanged(canUndo(), canRedo());
    }
}

bool SketchMode::canUndo() const
{
    return m_sketchData && m_sketchData->canUndo();
}

bool SketchMode::canRedo() const
{
    return m_sketchData && m_sketchData->canRedo();
}

// ============================================================================
// Input Handling
// ============================================================================

void SketchMode::handleMousePress(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    if (!m_active) return;
    
    m_shiftPressed = modifiers & Qt::ShiftModifier;
    m_ctrlPressed = modifiers & Qt::ControlModifier;
    
    if (m_activeTool && (buttons & Qt::LeftButton)) {
        // Get snapped position
        QString snapType;
        QPointF sketchPos = getSnapPoint(pos, snapType);
        
        m_activeTool->handleMousePress(sketchPos, buttons, modifiers);
    }
}

void SketchMode::handleMouseMove(const QPoint& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    if (!m_active) return;
    
    m_shiftPressed = modifiers & Qt::ShiftModifier;
    m_ctrlPressed = modifiers & Qt::ControlModifier;
    
    // Update snap indicator
    updateSnapIndicator(pos);
    
    if (m_activeTool) {
        QString snapType;
        QPointF sketchPos = getSnapPoint(pos, snapType);
        
        // Apply horizontal/vertical constraint if shift is held
        if (m_shiftPressed && m_activeTool->supportsOrthoConstraint()) {
            sketchPos = m_activeTool->applyOrthoConstraint(sketchPos);
        }
        
        m_activeTool->handleMouseMove(sketchPos, buttons, modifiers);
    }
    
    // Update viewport overlay
    if (m_viewportOverlay) {
        m_viewportOverlay->update();
    }
}

void SketchMode::handleMouseRelease(const QPoint& pos, Qt::MouseButtons buttons)
{
    if (!m_active) return;
    
    if (m_activeTool) {
        QString snapType;
        QPointF sketchPos = getSnapPoint(pos, snapType);
        m_activeTool->handleMouseRelease(sketchPos, buttons);
    }
}

void SketchMode::handleDoubleClick(const QPoint& pos, Qt::MouseButtons buttons)
{
    if (!m_active) return;
    
    if (m_activeTool) {
        QString snapType;
        QPointF sketchPos = getSnapPoint(pos, snapType);
        m_activeTool->handleDoubleClick(sketchPos, buttons);
    }
}

void SketchMode::handleKeyPress(QKeyEvent* event)
{
    if (!m_active) return;
    
    // Global sketch shortcuts
    switch (event->key()) {
        case Qt::Key_Escape:
            if (m_activeTool && m_activeTool->isDrawing()) {
                cancelCurrentOperation();
            } else {
                exitSketchMode(true);
            }
            event->accept();
            return;
            
        case Qt::Key_Return:
        case Qt::Key_Enter:
            finishCurrentOperation();
            event->accept();
            return;
            
        case Qt::Key_L:
            setActiveTool(SketchToolType::Line);
            event->accept();
            return;
            
        case Qt::Key_O:
            setActiveTool(SketchToolType::Circle);
            event->accept();
            return;
            
        case Qt::Key_A:
            setActiveTool(SketchToolType::Arc);
            event->accept();
            return;
            
        case Qt::Key_S:
            setActiveTool(SketchToolType::Spline);
            event->accept();
            return;
            
        case Qt::Key_R:
            setActiveTool(SketchToolType::Rectangle);
            event->accept();
            return;
            
        case Qt::Key_T:
            setActiveTool(SketchToolType::Trim);
            event->accept();
            return;
            
        case Qt::Key_H:
            setActiveTool(SketchToolType::ConstraintHorizontal);
            event->accept();
            return;
            
        case Qt::Key_V:
            setActiveTool(SketchToolType::ConstraintVertical);
            event->accept();
            return;
            
        case Qt::Key_D:
            setActiveTool(SketchToolType::Dimension);
            event->accept();
            return;
            
        case Qt::Key_Z:
            if (event->modifiers() & Qt::ControlModifier) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    redo();
                } else {
                    undo();
                }
                event->accept();
                return;
            }
            break;
            
        case Qt::Key_Y:
            if (event->modifiers() & Qt::ControlModifier) {
                redo();
                event->accept();
                return;
            }
            break;
            
        case Qt::Key_G:
            setGridVisible(!m_gridSettings.visible);
            event->accept();
            return;
    }
    
    // Pass to active tool
    if (m_activeTool) {
        m_activeTool->handleKeyPress(event);
    }
}

void SketchMode::handleKeyRelease(QKeyEvent* event)
{
    if (!m_active) return;
    
    if (m_activeTool) {
        m_activeTool->handleKeyRelease(event);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void SketchMode::setupViewport()
{
    if (!m_viewport) return;
    
    // Save current viewport state
    m_savedViewMatrix = m_viewport->viewMatrix();
    m_savedProjectionMatrix = m_viewport->projectionMatrix();
    m_savedOrthographic = m_viewport->isOrthographic();
    
    // Switch to orthographic mode
    m_viewport->setOrthographic(true);
}

void SketchMode::restoreViewport()
{
    if (!m_viewport) return;
    
    // Restore saved viewport state
    m_viewport->setViewMatrix(m_savedViewMatrix);
    m_viewport->setProjectionMatrix(m_savedProjectionMatrix);
    m_viewport->setOrthographic(m_savedOrthographic);
}

void SketchMode::createTools()
{
    // Create all sketch tools
    m_tools[SketchToolType::Line] = std::make_unique<LineTool>(this);
    m_tools[SketchToolType::Arc] = std::make_unique<ArcTool>(this);
    m_tools[SketchToolType::Spline] = std::make_unique<SplineTool>(this);
    m_tools[SketchToolType::Dimension] = std::make_unique<DimensionTool>(this);
    
    // TODO: Create remaining tools
    // m_tools[SketchToolType::Circle] = std::make_unique<CircleTool>(this);
    // m_tools[SketchToolType::Rectangle] = std::make_unique<RectangleTool>(this);
    // m_tools[SketchToolType::Point] = std::make_unique<PointTool>(this);
    // m_tools[SketchToolType::Trim] = std::make_unique<TrimTool>(this);
    // m_tools[SketchToolType::Extend] = std::make_unique<ExtendTool>(this);
    // m_tools[SketchToolType::Offset] = std::make_unique<OffsetTool>(this);
    // m_tools[SketchToolType::Mirror] = std::make_unique<MirrorTool>(this);
    
    // Connect tool signals
    for (auto& [type, tool] : m_tools) {
        connect(tool.get(), &SketchTool::entityCreated, this, [this]() {
            emit sketchModified();
            emit undoStateChanged(canUndo(), canRedo());
        });
        
        connect(tool.get(), &SketchTool::previewUpdated, this, [this]() {
            if (m_viewportOverlay) {
                m_viewportOverlay->update();
            }
        });
    }
}

void SketchMode::destroyTools()
{
    m_activeTool = nullptr;
    m_tools.clear();
}

void SketchMode::updateSnapIndicator(const QPoint& screenPos)
{
    QString snapType;
    QPointF snapPoint = getSnapPoint(screenPos, snapType);
    
    if (snapType != m_currentSnapType || snapPoint != m_currentSnapPoint) {
        m_currentSnapPoint = snapPoint;
        m_currentSnapType = snapType;
        
        // Update viewport overlay
        if (m_viewportOverlay) {
            m_viewportOverlay->setSnapIndicator(snapPoint, snapType);
        }
        
        emit snapPointChanged(snapPoint, snapType);
    }
}

QMatrix4x4 SketchMode::computePlaneTransform() const
{
    if (!m_sketchPlane) return QMatrix4x4();
    
    QMatrix4x4 transform;
    
    // Build rotation matrix from plane axes
    QVector3D zAxis = m_sketchPlane->normal;
    QVector3D xAxis = m_sketchPlane->xAxis;
    QVector3D yAxis = m_sketchPlane->yAxis;
    
    transform.setColumn(0, QVector4D(xAxis, 0));
    transform.setColumn(1, QVector4D(yAxis, 0));
    transform.setColumn(2, QVector4D(zAxis, 0));
    transform.setColumn(3, QVector4D(m_sketchPlane->origin, 1));
    
    return transform;
}

} // namespace dc
