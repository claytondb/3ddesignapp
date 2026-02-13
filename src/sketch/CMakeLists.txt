# Sketch module - 2D sketch geometry and constraint solver

set(SKETCH_SOURCES
    Sketch.cpp
    Sketch.h
    SketchSolver.cpp
    SketchSolver.h
    SectionCreator.cpp
    SectionCreator.h
    
    # Sketch entities
    entities/SketchEntity.cpp
    entities/SketchEntity.h
    entities/SketchPoint.cpp
    entities/SketchPoint.h
    entities/SketchLine.cpp
    entities/SketchLine.h
    entities/SketchArc.cpp
    entities/SketchArc.h
    entities/SketchCircle.cpp
    entities/SketchCircle.h
    entities/SketchSpline.cpp
    entities/SketchSpline.h
)

add_library(dc3d_sketch STATIC
    ${SKETCH_SOURCES}
)

target_include_directories(dc3d_sketch PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/entities
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# GLM is header-only
find_package(glm CONFIG QUIET)
if(glm_FOUND)
    target_link_libraries(dc3d_sketch PUBLIC glm::glm)
else()
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATHS /usr/include /usr/local/include
    )
    if(GLM_INCLUDE_DIR)
        target_include_directories(dc3d_sketch PUBLIC ${GLM_INCLUDE_DIR})
    endif()
endif()

target_link_libraries(dc3d_sketch PUBLIC
    Qt6::Core
    dc3d_geometry
)
