# Renderer module - OpenGL rendering

add_library(dc3d_renderer STATIC
    Viewport.cpp
)

target_include_directories(dc3d_renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dc3d_renderer PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    dc3d_core
    dc3d_geometry
)
