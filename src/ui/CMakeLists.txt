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
)

add_library(dc3d_ui STATIC
    ${UI_SOURCES}
)

target_include_directories(dc3d_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
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
