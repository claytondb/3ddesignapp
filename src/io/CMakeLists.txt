# I/O module - File format readers and writers

set(IO_SOURCES
    MeshImporter.cpp
    MeshImporter.h
    STLImporter.cpp
    STLImporter.h
    OBJImporter.cpp
    OBJImporter.h
    PLYImporter.cpp
    PLYImporter.h
)

add_library(dc3d_io STATIC
    ${IO_SOURCES}
)

target_include_directories(dc3d_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(dc3d_io PUBLIC
    Qt6::Core
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
