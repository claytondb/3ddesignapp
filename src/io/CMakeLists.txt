# I/O module - File format readers and writers

add_library(dc3d_io STATIC
    MeshImporter.cpp
)

target_include_directories(dc3d_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dc3d_io PUBLIC
    Qt6::Core
    dc3d_core
    dc3d_geometry
)

# Link Open CASCADE for STEP/IGES if available
if(OpenCASCADE_FOUND)
    target_link_libraries(dc3d_io PUBLIC
        TKSTEP
        TKIGES
        TKXSBase
    )
endif()
