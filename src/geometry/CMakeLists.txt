# Geometry module - Mesh and CAD operations

set(GEOMETRY_SOURCES
    # Core mesh types
    MeshData.cpp
    MeshData.h
    HalfEdgeMesh.cpp
    HalfEdgeMesh.h
    BVH.cpp
    BVH.h
    NURBSSurface.cpp
    NURBSSurface.h
    
    # Primitive mesh generation
    PrimitiveGenerator.cpp
    PrimitiveGenerator.h
    
    # Mesh processing
    MeshAnalysis.cpp
    MeshAnalysis.h
    MeshRepair.cpp
    MeshRepair.h
    MeshSmoothing.cpp
    MeshSmoothing.h
    MeshDecimation.cpp
    MeshDecimation.h
    MeshSubdivision.cpp
    MeshSubdivision.h
    
    # Alignment / Registration
    Alignment.cpp
    Alignment.h
    ICP.cpp
    ICP.h
    
    # Analysis
    DeviationAnalysis.cpp
    DeviationAnalysis.h
    
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
    
    # Primitives (fitting)
    primitives/Plane.cpp
    primitives/Plane.h
    primitives/Sphere.cpp
    primitives/Sphere.h
    primitives/Cylinder.cpp
    primitives/Cylinder.h
    primitives/Cone.cpp
    primitives/Cone.h
    primitives/PrimitiveFitter.cpp
    primitives/PrimitiveFitter.h
    primitives/SymmetryPlane.cpp
    primitives/SymmetryPlane.h
    
    # Freeform surfaces
    freeform/AutoSurface.cpp
    freeform/AutoSurface.h
    freeform/QuadMesh.cpp
    freeform/QuadMesh.h
    freeform/SurfaceFit.cpp
    freeform/SurfaceFit.h
    freeform/WrapSurface.cpp
    freeform/WrapSurface.h
    
    # Solid modeling
    solid/Solid.cpp
    solid/Solid.h
    solid/BooleanOps.cpp
    solid/BooleanOps.h
    solid/Fillet.cpp
    solid/Fillet.h
    solid/Chamfer.cpp
    solid/Chamfer.h
)

add_library(dc3d_geometry STATIC
    ${GEOMETRY_SOURCES}
)

target_include_directories(dc3d_geometry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/surfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/primitives
    ${CMAKE_CURRENT_SOURCE_DIR}/freeform
    ${CMAKE_CURRENT_SOURCE_DIR}/solid
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# GLM include directory if found manually (not via package)
if(GLM_INCLUDE_DIR AND NOT GLM_INCLUDE_DIR STREQUAL "")
    target_include_directories(dc3d_geometry PUBLIC ${GLM_INCLUDE_DIR})
endif()

# Link only Qt for now - no circular dependencies
target_link_libraries(dc3d_geometry PUBLIC
    Qt6::Core
)

# Link glm::glm target if found via package
if(TARGET glm::glm)
    target_link_libraries(dc3d_geometry PUBLIC glm::glm)
endif()

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
