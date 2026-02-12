# Geometry module - Mesh and CAD operations

add_library(dc3d_geometry STATIC
    MeshData.cpp
)

target_include_directories(dc3d_geometry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dc3d_geometry PUBLIC
    Qt6::Core
    dc3d_core
)

# Link Open CASCADE if available
if(OpenCASCADE_FOUND)
    target_link_libraries(dc3d_geometry PUBLIC
        TKernel
        TKMath
        TKG3d
        TKGeomBase
        TKBRep
    )
endif()
