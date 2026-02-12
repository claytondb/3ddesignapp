# UI module - Qt widgets

# Include subdirectories
add_subdirectory(sketch)

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
)

# Sketch module sources
set(SKETCH_SOURCES
    sketch/SketchMode.cpp
    sketch/SketchMode.h
    sketch/SketchToolbox.cpp
    sketch/SketchToolbox.h
    sketch/SketchViewport.cpp
    sketch/SketchViewport.h
    sketch/SketchTool.cpp
    sketch/SketchTool.h
    sketch/LineTool.cpp
    sketch/LineTool.h
    sketch/ArcTool.cpp
    sketch/ArcTool.h
    sketch/SplineTool.cpp
    sketch/SplineTool.h
    sketch/DimensionTool.cpp
    sketch/DimensionTool.h
)

# Dialog sources
set(DIALOG_SOURCES
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
)

# Tools sources
set(TOOLS_SOURCES
    tools/AlignmentTool.cpp
    tools/AlignmentTool.h
)

add_library(dc3d_ui STATIC
    ${UI_SOURCES}
    ${SKETCH_SOURCES}
    ${DIALOG_SOURCES}
    ${TOOLS_SOURCES}
)

target_include_directories(dc3d_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sketch
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
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
)
