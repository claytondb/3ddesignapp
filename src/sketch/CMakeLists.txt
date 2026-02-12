# DC-3DDesignApp Sketch Module
# 2D Sketching Engine with constraint solving

cmake_minimum_required(VERSION 3.16)

# Sketch module library
add_library(dc_sketch STATIC
    # Core sketch
    Sketch.cpp
    SketchSolver.cpp
    SectionCreator.cpp
    
    # Entities
    entities/SketchEntity.cpp
    entities/SketchLine.cpp
    entities/SketchArc.cpp
    entities/SketchCircle.cpp
    entities/SketchSpline.cpp
)

# Include directories
target_include_directories(dc_sketch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link dependencies
target_link_libraries(dc_sketch
    PUBLIC
        glm::glm
)

# C++17 required
target_compile_features(dc_sketch PUBLIC cxx_std_17)

# Compiler warnings
if(MSVC)
    target_compile_options(dc_sketch PRIVATE /W4)
else()
    target_compile_options(dc_sketch PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directories
set_target_properties(dc_sketch PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Alias for cleaner imports
add_library(dc::sketch ALIAS dc_sketch)

# Optional: Install rules
install(TARGETS dc_sketch
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES
    Sketch.h
    SketchSolver.h
    SectionCreator.h
    DESTINATION include/dc/sketch
)

install(FILES
    entities/SketchEntity.h
    entities/SketchLine.h
    entities/SketchArc.h
    entities/SketchCircle.h
    entities/SketchSpline.h
    DESTINATION include/dc/sketch/entities
)
