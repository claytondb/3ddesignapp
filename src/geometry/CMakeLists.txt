# Geometry module - Mesh and CAD operations

set(GEOMETRY_SOURCES
    MeshData.cpp
    MeshData.h
    HalfEdgeMesh.cpp
    HalfEdgeMesh.h
    BVH.cpp
    BVH.h
    NURBSSurface.cpp
    NURBSSurface.h
    
    # Surface operations
    surfaces/Extrude.cpp
    surfaces/Extrude.h
    surfaces/Revolve.cpp
    surfaces/Revolve.h
    surfaces/Loft.cpp
    surfaces/Loft.h
    surfaces/Sweep.cpp
    surfaces/Sweep.h
    surfaces/Pipe.cpp
    surfaces/Pipe.h
    surfaces/PlanarSurface.cpp
    surfaces/PlanarSurface.h
)

add_library(dc3d_geometry STATIC
    ${GEOMETRY_SOURCES}
)

target_include_directories(dc3d_geometry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link only Qt for now - no circular dependencies
target_link_libraries(dc3d_geometry PUBLIC
    Qt6::Core
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
