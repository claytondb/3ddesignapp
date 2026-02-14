# UI module - Qt widgets

set(UI_SOURCES
    MainWindow.cpp
    MainWindow.h
    MenuBar.cpp
    MenuBar.h
    ObjectBrowser.cpp
    ObjectBrowser.h
    PropertiesPanel.cpp
    PropertiesPanel.h
    StatusBar.cpp
    StatusBar.h
    Toolbar.cpp
    Toolbar.h
    HelpSystem.cpp
    HelpSystem.h
    TransformInputWidget.cpp
    TransformInputWidget.h
    ViewPresetsWidget.cpp
    ViewPresetsWidget.h
)

# Sketch UI sources - DISABLED: requires incomplete geometry classes and Viewport methods
# To re-enable, implement dc3d::geometry::SketchData, SketchPlane
# and add Viewport methods: zoomToBox, viewMatrix, projectionMatrix, etc.
set(SKETCH_UI_SOURCES
    # sketch/SketchMode.cpp
    # sketch/SketchMode.h
    # sketch/SketchToolbox.cpp
    # sketch/SketchToolbox.h
    # sketch/SketchViewport.cpp
    # sketch/SketchViewport.h
    # sketch/SketchTool.cpp
    # sketch/SketchTool.h
    # sketch/LineTool.cpp
    # sketch/LineTool.h
    # sketch/ArcTool.cpp
    # sketch/ArcTool.h
    # sketch/SplineTool.cpp
    # sketch/SplineTool.h
    # sketch/DimensionTool.cpp
    # sketch/DimensionTool.h
)

# Dialog sources
set(DIALOG_SOURCES
    dialogs/PreferencesDialog.cpp
    dialogs/PreferencesDialog.h
    dialogs/PolygonReductionDialog.cpp
    dialogs/PolygonReductionDialog.h
    dialogs/SmoothingDialog.cpp
    dialogs/SmoothingDialog.h
    dialogs/HoleFillDialog.cpp
    dialogs/HoleFillDialog.h
    dialogs/OutlierRemovalDialog.cpp
    dialogs/OutlierRemovalDialog.h
    dialogs/ClippingBoxDialog.cpp
    dialogs/ClippingBoxDialog.h
    dialogs/AlignToWCSDialog.cpp
    dialogs/AlignToWCSDialog.h
    dialogs/NPointAlignDialog.cpp
    dialogs/NPointAlignDialog.h
    dialogs/FineAlignDialog.cpp
    dialogs/FineAlignDialog.h
    dialogs/SectionPlaneDialog.cpp
    dialogs/SectionPlaneDialog.h
    dialogs/ExtrudeDialog.cpp
    dialogs/ExtrudeDialog.h
    dialogs/RevolveDialog.cpp
    dialogs/RevolveDialog.h
    dialogs/ExportFormatDialog.cpp
    dialogs/ExportFormatDialog.h
    dialogs/ExportPresetManager.cpp
    dialogs/ExportPresetManager.h
    dialogs/ExportPresetsDialog.cpp
    dialogs/ExportPresetsDialog.h
    dialogs/KeyboardShortcutsDialog.cpp
    dialogs/KeyboardShortcutsDialog.h
    dialogs/AboutDialog.cpp
    dialogs/AboutDialog.h
    dialogs/GettingStartedDialog.cpp
    dialogs/GettingStartedDialog.h
    dialogs/UndoHistoryDialog.cpp
    dialogs/UndoHistoryDialog.h
    dialogs/PrimitiveCreationDialog.cpp
    dialogs/PrimitiveCreationDialog.h
    dialogs/MeshRepairWizard.cpp
    dialogs/MeshRepairWizard.h
    dialogs/SnapSettingsDialog.cpp
    dialogs/SnapSettingsDialog.h
    # Excluded - requires imgui dependency:
    # dialogs/ExportDialog.cpp
    # dialogs/ExportDialog.h
    # Excluded - requires Dialog.h base class (not implemented):
    # dialogs/AutoSurfaceDialog.cpp
    # dialogs/AutoSurfaceDialog.h
    # dialogs/FitSurfaceDialog.cpp
    # dialogs/FitSurfaceDialog.h
)

# Panels sources
set(PANEL_SOURCES
    panels/AnalysisPanel.cpp
    panels/AnalysisPanel.h
)

# Tools sources
set(TOOLS_SOURCES
    tools/AlignmentTool.cpp
    tools/AlignmentTool.h
    tools/MeasureTool.cpp
    tools/MeasureTool.h
    # Excluded - requires Tool.h base class (not implemented):
    # tools/FreeformTool.cpp
    # tools/FreeformTool.h
)

add_library(dc3d_ui STATIC
    ${UI_SOURCES}
    ${SKETCH_UI_SOURCES}
    ${DIALOG_SOURCES}
    ${PANEL_SOURCES}
    ${TOOLS_SOURCES}
)

target_include_directories(dc3d_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sketch
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(dc3d_ui PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    dc3d_renderer
    # dc3d_sketch  # Disabled - sketch UI not built
)
