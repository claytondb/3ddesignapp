# Changelog

All notable changes to dc-3ddesignapp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0] - 2026-02-12

### 🎉 Initial Release

We're excited to announce the first stable release of **dc-3ddesignapp** — a professional cross-platform scan-to-CAD application for reverse engineering.

### Highlights

- **Full scan-to-CAD workflow** from mesh import to STEP/IGES export
- **Professional mesh tools** including decimation, smoothing, and repair
- **Parametric solid modeling** with B-Rep geometry kernel
- **2D sketching system** with constraint solver
- **Advanced surface fitting** including NURBS and auto-surfacing
- **Cross-platform** support for Windows, macOS, and Linux

---

### Added

#### 🏗️ Foundation
- CMake project structure with Qt 6 and Open CASCADE integration
- Main window shell with modern dark theme
- Dockable panels (Object Browser, Properties Panel)
- OpenGL 4.5 viewport with arcball camera navigation
- Grid rendering with coordinate axes
- Real-time mesh rendering with shading modes

#### 📦 Import/Export
- **Mesh Import:** STL (ASCII/binary), OBJ (with normals), PLY (all variants)
- **CAD Import:** STEP (AP214/AP242), IGES
- **CAD Export:** STEP, IGES with configurable precision
- **Native Format:** .dc3d project files with compression
- Export options dialog (coordinate system, units, tolerance)

#### 🔷 Mesh Processing
- Polygon reduction using Quadric Error Metrics (QEM)
- Mesh smoothing: Laplacian, Taubin, bilateral
- Automatic hole detection and visualization
- Hole filling: flat, smooth, curvature-based methods
- Statistical outlier removal
- Mesh repair and cleanup operations
- Clipping box for region isolation

#### 📐 Alignment Tools
- Align to World Coordinate System (WCS)
- N-Point registration (3+ point alignment)
- Fine alignment using ICP (Iterative Closest Point)
- Interactive 3D transform manipulator

#### 🎯 Primitive Extraction
- Automatic primitive fitting from mesh regions
- Supported primitives: plane, cylinder, cone, sphere
- RANSAC-based robust fitting
- Least-squares refinement

#### ✏️ 2D Sketching
- Full sketch entity system: point, line, arc, circle, spline
- Section plane creation from mesh cross-sections
- Geometric constraints: coincident, parallel, perpendicular, tangent, concentric
- Dimensional constraints: distance, angle, radius, diameter
- Real-time constraint solver
- Sketch-to-mesh projection

#### 🧊 Solid Operations
- B-Rep solid body representation with full topology
- CSG boolean operations: union, subtract, intersect
- BSP tree implementation for robust booleans
- Fillet operations with rolling ball algorithm
- Variable radius fillets with tangent propagation
- Chamfer operations: symmetric, asymmetric, angle-based
- Primitive solids: box, cylinder, sphere, cone, torus

#### 🌊 Surface Modeling
- NURBS surface representation
- Surface fitting from mesh regions
- Planar and cylindrical surface fitting
- Pipe surface creation along paths
- AutoSurface for automatic surface generation
- QuadMesh for quad-dominant remeshing
- WrapSurface for shrink-wrap operations

#### 🔄 Core Systems
- Command pattern for all operations
- Undo/redo stack with 100-command history
- Command merging for smooth drag operations
- Scene manager for object hierarchy
- Selection system (single, multi, box select)
- Properties panel with live updates
- Object browser with visibility toggles

#### 🖥️ User Interface
- Modern dark theme throughout
- Toolbar with common operations
- Context menus for quick access
- Keyboard shortcuts for efficiency
- Progress dialogs for long operations
- All operation dialogs with preview support:
  - Polygon Reduction Dialog
  - Smoothing Dialog
  - Hole Fill Dialog
  - Outlier Removal Dialog
  - Clipping Box Dialog
  - Alignment Dialogs (WCS, N-Point, Fine)
  - Extrude/Revolve Dialogs
  - Section Plane Dialog

---

### Technical Specifications

| Metric | Value |
|--------|-------|
| **Source Files** | 203 |
| **Lines of Code** | 75,333 |
| **Development Time** | ~78 minutes |
| **Language** | C++ (C++17) |
| **UI Framework** | Qt 6.5+ |
| **Geometry Kernel** | Open CASCADE 7.8+ |
| **Rendering** | OpenGL 4.5 |

### Supported Platforms

| Platform | Architecture | Status |
|----------|--------------|--------|
| Windows 10/11 | x64 | ✅ Supported |
| macOS 12+ | x64, ARM64 | ✅ Supported |
| Linux (Ubuntu 22.04+) | x64 | ✅ Supported |

---

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

---

## [0.0.1] - 2026-02-12

### Added
- Initial project setup
- PROJECT_BRIEF.md with feature analysis
- Research on Revo Design / QuickSurface functionality
- Technical architecture proposal
- Development phase roadmap
