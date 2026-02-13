# Renderer module - OpenGL rendering

set(RENDERER_SOURCES
    Camera.cpp
    Camera.h
    GridRenderer.cpp
    GridRenderer.h
    Picking.cpp
    Picking.h
    SelectionRenderer.cpp
    SelectionRenderer.h
    ShaderProgram.cpp
    ShaderProgram.h
    Viewport.cpp
    Viewport.h
    
    # Additional renderers
    DeviationRenderer.cpp
    DeviationRenderer.h
    PrimitiveRenderer.cpp
    PrimitiveRenderer.h
    TransformGizmo.cpp
    TransformGizmo.h
)

add_library(dc3d_renderer STATIC
    ${RENDERER_SOURCES}
)

target_include_directories(dc3d_renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
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
