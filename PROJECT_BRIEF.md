# 3D Design App — Project Brief

**Project Codename:** dc-3ddesignapp  
**Target:** Clone of Revo Design (powered by QuickSurface)  
**Platforms:** Windows & macOS (cross-platform)  
**Created:** 2026-02-12

---

## Executive Summary

Create a cross-platform scan-to-CAD application that replicates the functionality of Revo Design / QuickSurface — professional reverse engineering software that converts 3D scan meshes into editable CAD models.

---

## Reference Product Analysis

### What is Revo Design?

Revo Design is scan-to-CAD software sold by Revopoint 3D, powered by the QuickSurface engine (by KVS). It's designed for:

- **Reverse engineering** — recreating CAD from physical parts
- **Surface reconstruction** — converting mesh scans to smooth surfaces
- **Parametric modeling** — creating editable CAD with dimensions
- **Deviation analysis** — comparing CAD to original scan

**Pricing:** $540/year (standard), $1000+ (Pro)

### System Requirements (Original)

- Windows 10 (64-bit) only
- Intel i5+ CPU
- 16GB RAM
- Dedicated GPU (NVIDIA/AMD)
- 256GB storage

### Core Technology

- **Geometry Kernel:** Siemens Parasolid (industry standard)
- **Export Formats:** STEP, IGES, PARASOLID
- **Import Formats:** STL, OBJ, PLY

---

## Feature Inventory

### Tier 1: Essential Features (MVP)

| Feature | Description |
|---------|-------------|
| **Mesh Import** | Load STL, OBJ, PLY files |
| **3D Viewport** | Navigate with orbit, pan, zoom |
| **Mesh Display** | Render mesh with shading, wireframe |
| **Polygon Reduction** | Decimate large meshes |
| **Mesh Smoothing** | Laplacian smoothing |
| **Hole Filling** | Fill mesh holes |
| **Outlier Removal** | Remove floating points |
| **Clipping Box** | Filter visible mesh region |
| **Primitive Extraction** | Fit plane, cylinder, cone, sphere |
| **Alignment** | Align mesh to WCS / interactive |
| **Export Mesh** | Save as STL |

### Tier 2: Core CAD Features

| Feature | Description |
|---------|-------------|
| **2D Cross Sections** | Create section planes through mesh |
| **2D Sketching** | Draw lines, arcs, splines on sections |
| **Fit 2D Entities** | Auto-fit lines/arcs to section points |
| **Extrude** | Create surfaces from 2D sketches |
| **Revolve** | Create surfaces by revolution |
| **Trim Surfaces** | Cut surfaces at intersections |
| **3D Sketching** | Draw curves on mesh surface |
| **Fit Surface** | Create NURBS surface from mesh region |
| **Real-time Deviation** | Show distance from curve/surface to mesh |
| **Export CAD** | Save as STEP, IGES |

### Tier 3: Advanced Features (Pro)

| Feature | Description |
|---------|-------------|
| **Automatic Mesh Segmentation** | AI-powered region detection |
| **Loft / Sweep / Pipe** | Advanced surface creation |
| **Boolean Operations** | Cut, combine, intersect solids |
| **Fillet / Chamfer** | Round or bevel edges |
| **Shell / Thicken** | Create wall thickness |
| **Mirror Bodies** | Symmetry operations |
| **Patterns** | Linear and circular arrays |
| **Draft Analysis** | Check moldability |
| **Import CAD** | Load STEP, IGES for comparison |
| **Scan-to-CAD Compare** | Colormap deviation analysis |
| **Free-form Surfacing** | Manual quad-mesh modeling |
| **Auto-Surfacing** | One-click surface generation |

---

## Technical Architecture

### Recommended Stack

| Layer | Technology | Rationale |
|-------|------------|-----------|
| **Language** | C++ with Python bindings | Performance for geometry, Python for scripting |
| **UI Framework** | Qt 6 | Cross-platform, proven for CAD apps |
| **3D Rendering** | OpenGL 4.5 / Vulkan | GPU-accelerated visualization |
| **Geometry Kernel** | Open CASCADE (OCCT) | Open-source STEP/IGES support |
| **Mesh Processing** | libigl, CGAL, VCG | Industry-standard mesh algorithms |
| **NURBS/B-Rep** | Open CASCADE | Surface fitting and modeling |
| **File I/O** | Assimp (mesh), OCCT (CAD) | Format support |
| **Build System** | CMake | Cross-platform compilation |

### Alternative: Electron + Three.js

For faster development with trade-offs:

| Layer | Technology |
|-------|------------|
| **Runtime** | Electron |
| **UI** | React + Tailwind |
| **3D Rendering** | Three.js |
| **Geometry** | Open CASCADE (WASM) |
| **Mesh** | three-mesh-bvh, earcut |

**Trade-offs:** Easier to build but slower performance, larger memory footprint

---

## Development Phases

### Phase 1: Foundation (Weeks 1-4)

- [ ] Project setup (CMake, Qt, OCCT)
- [ ] Main window with 3D viewport
- [ ] Mesh import (STL/OBJ/PLY)
- [ ] Camera controls (orbit/pan/zoom)
- [ ] Mesh rendering (shaded/wireframe)
- [ ] Basic selection tools

### Phase 2: Mesh Tools (Weeks 5-8)

- [ ] Polygon reduction (QEM decimation)
- [ ] Mesh smoothing
- [ ] Hole detection and filling
- [ ] Outlier removal
- [ ] Clipping box
- [ ] Undo/redo system

### Phase 3: Alignment & Primitives (Weeks 9-12)

- [ ] Interactive alignment (3D manipulator)
- [ ] WCS alignment by features
- [ ] Plane fitting (RANSAC/least-squares)
- [ ] Cylinder fitting
- [ ] Cone fitting
- [ ] Sphere fitting
- [ ] Symmetry plane detection

### Phase 4: 2D Sketching (Weeks 13-18)

- [ ] Section plane creation
- [ ] 2D sketch mode
- [ ] Line/arc/spline tools
- [ ] Auto-fit to section points
- [ ] Constraints system
- [ ] Dimension display
- [ ] Extrude from sketch
- [ ] Revolve from sketch

### Phase 5: Surfaces & CAD (Weeks 19-24)

- [ ] Surface fitting (NURBS)
- [ ] Real-time deviation display
- [ ] Surface trimming
- [ ] 3D sketching on mesh
- [ ] STEP export
- [ ] IGES export

### Phase 6: Polish & Pro Features (Weeks 25+)

- [ ] Loft/sweep/pipe
- [ ] Boolean operations
- [ ] Fillet/chamfer
- [ ] Auto-surfacing
- [ ] Scan-to-CAD comparison
- [ ] Performance optimization

---

## Key Challenges

### 1. Geometry Kernel Complexity

- Parasolid is proprietary ($$$)
- Open CASCADE is complex but capable
- NURBS surface fitting is mathematically hard

### 2. Cross-Platform CAD

- Windows: Native feel expected
- macOS: Different UX conventions
- Must handle both without compromises

### 3. Performance

- Meshes can be 100M+ triangles
- Need GPU acceleration
- Memory management critical

### 4. Precision

- CAD requires double-precision
- Floating point errors compound
- Tolerance handling is non-trivial

---

## Competitive Landscape

| Product | Price | Notes |
|---------|-------|-------|
| **Revo Design** | $540/yr | Our reference |
| **QuickSurface Pro** | $990 | Same engine |
| **Geomagic Design X** | $15,000+ | Industry leader |
| **Mesh2Surface** | $1,500 | Rhino plugin |
| **Artec Studio** | $2,500 | Scanner-specific |

**Opportunity:** No good open-source or low-cost cross-platform option exists.

---

## Success Criteria

1. **Import/export** works with real scan data
2. **Performance** handles 10M+ triangle meshes
3. **Accuracy** within 0.1mm tolerance
4. **Usability** learnable in hours, not days
5. **Stability** no data loss crashes
6. **Cross-platform** identical features Win/Mac

---

## Team Structure

| Role | Responsibilities |
|------|------------------|
| **Lead** | Architecture, coordination, reviews |
| **Requirements** | Feature specs, user stories, validation |
| **UX Design** | Interface design, user flows, prototypes |
| **Core Dev** | Geometry engine, algorithms |
| **UI Dev** | Qt/frontend implementation |
| **QA** | Testing, bug tracking, quality gates |

---

## References

- Revo Design: https://www.revopoint3d.com/products/revodesign
- QuickSurface: https://www.quicksurface.com/
- QuickSurface Tutorials: https://www.quicksurface3d.com/?p=quicksurface7_tutorials
- Open CASCADE: https://dev.opencascade.org/
- libigl: https://libigl.github.io/
- Qt: https://www.qt.io/

---

## Next Steps

1. **Validate scope** with stakeholder
2. **Choose tech stack** (native vs Electron)
3. **Create detailed requirements document**
4. **Design UI mockups**
5. **Set up development environment**
6. **Begin Phase 1 implementation**

---

*Document Version: 1.0*  
*Last Updated: 2026-02-12*
