# UI module - Qt widgets

add_library(dc3d_ui STATIC
    MainWindow.cpp
)

target_include_directories(dc3d_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dc3d_ui PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    dc3d_core
    dc3d_renderer
)
