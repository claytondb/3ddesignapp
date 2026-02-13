# Changelog

All notable changes to dc-3ddesignapp will be documented in this file.

## [0.1.0] - 2026-02-12

### Added

#### Foundation (Sprint 1)
- CMake project structure with Qt 6 and Open CASCADE integration
- Main window shell with dark theme
- Dockable panels (Object Browser, Properties)
- OpenGL 4.5 viewport with arcball camera
- Grid rendering with coordinate axes
- Half-edge mesh data structure
- STL importer (ASCII and binary)
- OBJ importer (with normals)
- PLY importer (all formats)

#### Core Systems (Sprint 2)
- Command pattern base class for undo/redo
- CommandStack with 100-command limit
- ImportMeshCommand for mesh import
- DeleteCommand for object deletion
- TransformCommand for move/rotate/scale
- Command merging for drag operations
- Qt signal integration for reactive UI
- SceneManager for object management

#### Mesh Processing (Sprint 3)
- Polygon reduction using Quadric Error Metrics (QEM)
- Laplacian, Taubin, and bilateral smoothing
- Automatic hole detection
- Hole filling (flat, smooth, curvature-based)
- Statistical outlier removal
- Mesh repair operations

#### UI Dialogs (Sprint 4)
- PolygonReductionDialog with preview
- SmoothingDialog with multiple methods
- HoleFillDialog with hole list
- OutlierRemovalDialog with statistics
- ClippingBoxDialog for region cropping
- AlignToWCSDialog for coordinate system alignment
- NPointAlignDialog for multi-point registration
- FineAlignDialog for ICP refinement
- ExtrudeDialog for extrusion operations
- RevolveDialog for revolution operations
- SectionPlaneDialog for cross-sections

#### Solid Operations (Sprint 5)
- B-Rep solid body representation
- Full topological connectivity (vertex-edge-face)
- CSG boolean operations (union, subtract, intersect)
- BSP tree implementation for robust booleans
- Fillet operations with rolling ball algorithm
- Variable radius fillets
- Tangent edge propagation
- Corner blend handling
- Chamfer operations (symmetric, asymmetric, angle-based)
- Variable chamfer support
- Primitive creation (box, cylinder, sphere, cone, torus)

#### Export/Import (Sprint 6)
- STEP exporter (AP214/AP242)
- STEP importer
- IGES exporter
- IGES importer
- Native file format (.dc3d) with compression
- Export options (precision, coordinate system, units)

#### Freeform Modeling (Sprint 7)
- AutoSurface for automatic surface fitting
- QuadMesh for quad-dominant remeshing
- SurfaceFit for NURBS surface fitting
- WrapSurface for shrink-wrap operations

#### 2D Sketching
- Sketch entity system (point, line, arc, circle, spline)
- Geometric constraints (coincident, parallel, perpendicular, tangent)
- Dimensional constraints (distance, angle, radius)
- Constraint solver
- Section creator from mesh cross-sections

#### Surfaces
- NURBS surface representation
- Planar surface fitting
- Cylindrical surface fitting
- Pipe surface creation

### Technical Details
- **Files:** 201 source files
- **Lines of Code:** 74,460
- **Build Time:** ~78 minutes total development
- **Target Platforms:** Windows, macOS, Linux

## [0.0.1] - 2026-02-12

### Added
- Initial project setup
- PROJECT_BRIEF.md with feature analysis
- Research on Revo Design / QuickSurface functionality
- Technical architecture proposal
- Development phase roadmap
