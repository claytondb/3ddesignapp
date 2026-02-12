# Requirements Specification — dc-3ddesignapp

**Version:** 1.0  
**Created:** 2026-02-12  
**Author:** Requirements Lead  
**Reference:** Revo Design / QuickSurface

---

## Table of Contents

1. [User Personas](#1-user-personas)
2. [User Stories](#2-user-stories)
3. [Functional Requirements](#3-functional-requirements)
4. [Non-Functional Requirements](#4-non-functional-requirements)
5. [Acceptance Criteria](#5-acceptance-criteria)
6. [Priority Matrix](#6-priority-matrix)

---

## 1. User Personas

### 1.1 Hobbyist — "Alex the Maker"

| Attribute | Description |
|-----------|-------------|
| **Background** | 3D printing enthusiast, owns a consumer 3D scanner (Revopoint, Creality), makes custom parts |
| **Technical Skill** | Intermediate; familiar with Blender, Fusion 360 basics; comfortable with mesh editing |
| **Goals** | Scan physical objects → repair/modify → 3D print; create replacement parts for broken items |
| **Pain Points** | Current tools are too expensive ($500+/year); steep learning curves; mesh-to-CAD is frustrating |
| **Use Frequency** | 2-5 hours/week; project-based usage |
| **Budget** | Under $200 one-time or free/open-source |
| **Key Workflows** | Import STL → Clean mesh → Fit basic shapes → Export for printing |
| **Success Metric** | Complete a scan-to-print workflow in under 30 minutes |

### 1.2 Professional Engineer — "Jordan the Design Engineer"

| Attribute | Description |
|-----------|-------------|
| **Background** | Mechanical engineer at mid-size manufacturing company; performs reverse engineering for legacy parts |
| **Technical Skill** | Expert CAD user (SolidWorks, Inventor); understands GD&T, tolerances, surface quality |
| **Goals** | Create accurate parametric CAD from worn/legacy parts; perform deviation analysis; produce manufacturing-ready STEP files |
| **Pain Points** | Manual reconstruction is slow; accuracy verification is tedious; collaboration with CAM team requires clean geometry |
| **Use Frequency** | 20-40 hours/week during active projects |
| **Budget** | $500-2000/year (company budget) |
| **Key Workflows** | High-res scan → Align to WCS → Section sketching → Parametric reconstruction → Deviation analysis → STEP export |
| **Success Metric** | Reconstruct complex part within 0.1mm tolerance in under 4 hours |

### 1.3 Manufacturer — "Sam the Production Lead"

| Attribute | Description |
|-----------|-------------|
| **Background** | Production engineer at contract manufacturer; handles incoming parts without CAD, quality inspection |
| **Technical Skill** | CAD proficient; expert in manufacturing processes (CNC, injection molding); familiar with CMM inspection |
| **Goals** | Rapidly create CAD for quotations; verify incoming part quality against specifications; create tooling from scans |
| **Pain Points** | Customers send physical samples, not CAD; quick turnaround needed; accuracy impacts profitability |
| **Use Frequency** | Daily, high-volume throughput |
| **Budget** | Enterprise ($2000+/year acceptable if ROI proven) |
| **Key Workflows** | Batch import → Automated primitive detection → Quick reconstruction → Scan-to-CAD comparison report → Export |
| **Success Metric** | Process 10+ parts/day; generate comparison report in under 15 minutes |

---

## 2. User Stories

### 2.1 Mesh Import & Management (Epic: MESH)

#### MESH-001: Import Mesh Files
**As a** user  
**I want to** import STL, OBJ, and PLY mesh files  
**So that** I can work with scan data from any 3D scanner

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Import STL files (ASCII and binary formats)
- [ ] Import OBJ files (with vertex normals if present)
- [ ] Import PLY files (ASCII and binary)
- [ ] Handle files up to 50 million triangles without crash
- [ ] Display import progress for files > 1 million triangles
- [ ] Show file statistics after import (vertex count, triangle count, bounding box dimensions)
- [ ] Support drag-and-drop import
- [ ] Gracefully handle corrupt/invalid files with user-friendly error message

**Technical Notes:**
- Use Assimp or custom loaders for format support
- Implement streaming/chunked loading for large files
- Build spatial index (BVH/octree) during import for fast selection

---

#### MESH-002: Polygon Reduction (Decimation)
**As a** user with high-resolution scans  
**I want to** reduce triangle count while preserving shape  
**So that** I can work efficiently with large meshes

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Reduce mesh to user-specified target triangle count
- [ ] Reduce mesh by percentage (e.g., "reduce to 50%")
- [ ] Preview decimation result before applying
- [ ] Preserve mesh boundaries (edges)
- [ ] Option to preserve sharp features/edges
- [ ] Maintain deviation within user-specified tolerance
- [ ] Show deviation colormap after decimation
- [ ] Processing time < 30 seconds for 10M triangles on reference hardware

**Technical Notes:**
- Implement Quadric Error Metrics (QEM) decimation algorithm
- Consider edge collapse vs. vertex clustering approaches
- Provide quality vs. speed presets

---

#### MESH-003: Mesh Smoothing
**As a** user with noisy scan data  
**I want to** smooth the mesh surface  
**So that** I can reduce noise while preserving features

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Apply Laplacian smoothing with adjustable iterations (1-100)
- [ ] Apply Taubin smoothing (reduces shrinkage)
- [ ] Bilateral smoothing option (preserves edges)
- [ ] Preview smoothing result before applying
- [ ] Allow selective smoothing on brush-selected region
- [ ] Undo smoothing operation
- [ ] Real-time preview for meshes < 500K triangles

---

#### MESH-004: Hole Detection and Filling
**As a** user with incomplete scan data  
**I want to** detect and fill holes in the mesh  
**So that** I have a watertight model for CAD operations

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Automatically detect all boundary loops (holes)
- [ ] Highlight holes with distinct color
- [ ] List holes with size (area, perimeter, vertex count)
- [ ] Fill individual holes on click
- [ ] Fill all holes with single command
- [ ] Filling options: flat, smooth, curvature-based
- [ ] Handle complex/non-planar holes
- [ ] Allow manual hole boundary editing before fill

---

#### MESH-005: Outlier Removal
**As a** user with noisy scan data  
**I want to** remove floating/disconnected geometry  
**So that** I have clean reference data

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Detect disconnected mesh components
- [ ] Remove components smaller than N triangles (user-defined)
- [ ] Remove components smaller than N% of total mesh
- [ ] Preview components to be removed
- [ ] Keep only largest N components option
- [ ] Statistical outlier removal (points far from local surface)
- [ ] Undo operation

---

#### MESH-006: Clipping Box
**As a** user  
**I want to** isolate a region of the mesh  
**So that** I can focus on specific areas and reduce visual clutter

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Define axis-aligned bounding box interactively
- [ ] Adjust each face of the box independently
- [ ] Type exact dimensions for box
- [ ] Clip inside or outside the box (toggle)
- [ ] Clipped regions affect both display and operations
- [ ] Create new mesh from clipped region
- [ ] Multiple clipping boxes support
- [ ] Reset/remove clipping

---

### 2.2 3D Viewport & Navigation (Epic: VIEW)

#### VIEW-001: 3D Viewport Display
**As a** user  
**I want to** view my mesh in a 3D viewport  
**So that** I can visualize and interact with scan data

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Render mesh with smooth shading (Phong/Blinn-Phong)
- [ ] Render mesh wireframe overlay
- [ ] Render flat shading mode
- [ ] Render point cloud mode (vertices only)
- [ ] Toggle visibility of mesh, primitives, surfaces independently
- [ ] X-ray/transparency mode for occluded geometry
- [ ] Configurable background color/gradient
- [ ] Grid display at origin plane
- [ ] Coordinate axes indicator (XYZ gizmo)
- [ ] Maintain 60 FPS for meshes < 5M triangles on reference hardware

---

#### VIEW-002: Camera Navigation
**As a** user  
**I want to** navigate the 3D view with standard controls  
**So that** I can examine my model from any angle

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Orbit: rotate camera around focal point (middle mouse drag or Alt+left drag)
- [ ] Pan: move camera parallel to view plane (Shift+middle mouse or middle mouse drag)
- [ ] Zoom: move camera closer/farther (scroll wheel, or Alt+right drag)
- [ ] Fit view: frame entire model in viewport (F key or button)
- [ ] Fit selection: frame selected geometry
- [ ] Standard views: Front, Back, Left, Right, Top, Bottom, Isometric (numpad or buttons)
- [ ] Orthographic/Perspective toggle
- [ ] Walk/fly mode for interior inspection (optional P2)
- [ ] Customizable mouse button mapping

---

#### VIEW-003: Selection Tools
**As a** user  
**I want to** select mesh regions  
**So that** I can apply operations to specific areas

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Click select: single triangle/vertex
- [ ] Rectangle select: drag box selection
- [ ] Lasso select: freeform polygon selection
- [ ] Brush select: paint selection with adjustable radius
- [ ] Magic wand: select connected regions by angle threshold
- [ ] Grow/shrink selection
- [ ] Select by color (if mesh has vertex colors)
- [ ] Invert selection
- [ ] Clear selection
- [ ] Selection modes: add, subtract, intersect (Shift, Ctrl modifiers)
- [ ] Visual feedback: selected regions highlighted in distinct color

---

### 2.3 Alignment (Epic: ALIGN)

#### ALIGN-001: Interactive Alignment
**As a** user  
**I want to** manually position and orient the mesh  
**So that** I can align it to the world coordinate system

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] 3D manipulator gizmo for translation (XYZ arrows)
- [ ] 3D manipulator gizmo for rotation (XYZ rings)
- [ ] Type exact transform values in dialog
- [ ] Snap to grid option
- [ ] Snap to 90° rotation increments
- [ ] Reset to original position
- [ ] Undo/redo transform steps
- [ ] Show mirrored points preview (for symmetry check)

---

#### ALIGN-002: Align to World Coordinate System (WCS)
**As a** professional user  
**I want to** align mesh using detected features  
**So that** the model is precisely oriented for manufacturing

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Define primary axis from plane (normal defines axis)
- [ ] Define secondary axis from plane, cylinder axis, or line
- [ ] Define origin point from vertex, plane intersection, or cylinder center
- [ ] Support 3-2-1 alignment workflow (primary, secondary, tertiary features)
- [ ] Preview alignment before applying
- [ ] Show alignment deviation/error
- [ ] Save/load alignment configurations

---

#### ALIGN-003: N-Point Alignment
**As a** user  
**I want to** align two meshes (or mesh to CAD) using point pairs  
**So that** I can register scans from multiple positions

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Pick corresponding points on source and target (minimum 3 pairs)
- [ ] Compute best-fit transformation (rigid: rotation + translation)
- [ ] Show RMS error of alignment
- [ ] Support weighted points
- [ ] Allow iterative refinement (ICP)
- [ ] Fine alignment option (ICP-based) after initial alignment

---

### 2.4 Primitive Fitting (Epic: PRIM)

#### PRIM-001: Fit Plane
**As a** user  
**I want to** fit a plane to selected mesh region  
**So that** I can identify flat surfaces for CAD reconstruction

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Fit plane to selected triangles using least-squares
- [ ] Fit plane using RANSAC for noisy data
- [ ] Display plane as bounded rectangle (sized to selection)
- [ ] Display plane as infinite (extends to viewport edges)
- [ ] Show deviation colormap (distance from points to plane)
- [ ] Show statistics: RMS error, max deviation, normal vector
- [ ] Constrain plane: parallel to XY/XZ/YZ, through point
- [ ] Edit plane after creation (move, resize)

---

#### PRIM-002: Fit Cylinder
**As a** user  
**I want to** fit a cylinder to selected mesh region  
**So that** I can reconstruct cylindrical features (holes, shafts)

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Fit cylinder to selection using least-squares
- [ ] Fit cylinder using RANSAC
- [ ] Display cylinder with correct axis orientation
- [ ] Display cylinder extent (start/end along axis)
- [ ] Show deviation colormap
- [ ] Show statistics: diameter, axis vector, length, RMS error
- [ ] Constrain: axis parallel to X/Y/Z, specific diameter
- [ ] Edit after creation (radius, length, position)

---

#### PRIM-003: Fit Cone
**As a** user  
**I want to** fit a cone to selected mesh region  
**So that** I can reconstruct tapered features

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Fit cone to selection
- [ ] Display with apex position and base
- [ ] Show deviation colormap
- [ ] Show statistics: half-angle, axis, apex position, RMS error
- [ ] Constrain: axis parallel to X/Y/Z, specific angle
- [ ] Handle truncated cones (frustums)

---

#### PRIM-004: Fit Sphere
**As a** user  
**I want to** fit a sphere to selected mesh region  
**So that** I can reconstruct spherical features

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Fit sphere to selection
- [ ] Display with correct center and radius
- [ ] Show deviation colormap
- [ ] Show statistics: center point, radius, RMS error
- [ ] Handle partial spheres (hemisphere, etc.)
- [ ] Constrain: specific radius, through specific point

---

#### PRIM-005: Detect Symmetry Plane
**As a** user  
**I want to** detect symmetry in the mesh  
**So that** I can work on half the model and mirror

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Automatic symmetry plane detection
- [ ] Manual symmetry plane definition
- [ ] Display mirrored mesh preview
- [ ] Use symmetry for alignment (secondary axis)
- [ ] Show symmetry deviation (how symmetric the mesh is)

---

#### PRIM-006: Auto-Constrain Primitives
**As a** professional user  
**I want to** primitives to snap to relationships  
**So that** I get design-intent geometry

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Auto-detect perpendicular planes
- [ ] Auto-detect parallel planes within tolerance
- [ ] Auto-detect concentric cylinders
- [ ] Auto-detect coaxial cylinders
- [ ] Auto-detect tangent relationships
- [ ] Manually add/remove constraints
- [ ] Constraint tolerance configurable

---

### 2.5 2D Cross Sections & Sketching (Epic: SKETCH)

#### SKETCH-001: Create Cross Sections
**As a** user  
**I want to** create section planes through the mesh  
**So that** I can extract 2D profiles for sketching

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Create section plane aligned to XY, XZ, YZ
- [ ] Create section plane at arbitrary orientation
- [ ] Create section through existing primitive (plane)
- [ ] Section plane positioning via slider
- [ ] Section plane positioning via typed offset value
- [ ] Display section as point cloud (intersection points)
- [ ] Display section as polyline (connected)
- [ ] Display section thickness option (slab)
- [ ] Create multiple parallel sections at once (stacked)
- [ ] Section offset configurable for stacked sections

---

#### SKETCH-002: 2D Sketch Mode
**As a** user  
**I want to** draw 2D geometry on section planes  
**So that** I can create profiles for extrusion/revolution

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Enter 2D sketch mode on selected section plane
- [ ] View automatically orients perpendicular to sketch plane
- [ ] Grid display on sketch plane
- [ ] Snap to grid, endpoints, midpoints, intersections
- [ ] Construction geometry mode (reference lines)
- [ ] Exit sketch mode returns to 3D view

---

#### SKETCH-003: Draw 2D Entities
**As a** user  
**I want to** draw lines, arcs, and splines  
**So that** I can create sketch profiles

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Draw line: click two points
- [ ] Draw polyline: continuous connected lines
- [ ] Draw arc: 3-point, center+start+end, tangent arc
- [ ] Draw circle: center+radius, 3-point
- [ ] Draw rectangle: two corners
- [ ] Draw spline: through N points, B-spline
- [ ] Edit vertices after creation (drag)
- [ ] Delete entities
- [ ] Trim entities at intersections
- [ ] Extend entities to intersection
- [ ] Fillet corners (arc insert)
- [ ] Chamfer corners (line insert)

---

#### SKETCH-004: Fit 2D Entities to Section Points
**As a** user  
**I want to** automatically fit sketch geometry to section data  
**So that** I can quickly create accurate profiles

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Auto-fit line to selected section points
- [ ] Auto-fit arc to selected section points
- [ ] Auto-fit spline to selected section points
- [ ] Show deviation from fitted entity to source points
- [ ] Real-time deviation display during fitting
- [ ] Tolerance setting for fit quality
- [ ] Combine fitted entities into profile automatically
- [ ] Auto-sketcher: one-click profile fitting for simple shapes

---

#### SKETCH-005: Sketch Constraints & Dimensions
**As a** user  
**I want to** apply geometric constraints and dimensions  
**So that** my sketch is fully defined and parametric

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Horizontal/Vertical constraint on lines
- [ ] Coincident constraint (point-to-point)
- [ ] Concentric constraint (circle centers)
- [ ] Tangent constraint (line-arc, arc-arc)
- [ ] Perpendicular/Parallel constraints
- [ ] Equal length/radius constraints
- [ ] Fixed constraint (lock position)
- [ ] Linear dimension (distance)
- [ ] Angular dimension
- [ ] Radial/Diameter dimension
- [ ] Driven dimensions (display only, from geometry)
- [ ] Under/over/fully constrained status indicator

---

### 2.6 Surface Creation (Epic: SURF)

#### SURF-001: Extrude from 2D Sketch
**As a** user  
**I want to** extrude 2D profiles into 3D surfaces  
**So that** I can create prismatic CAD geometry

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Extrude closed profile to specified distance
- [ ] Extrude open profile as surface (not solid)
- [ ] Symmetric extrude (both directions)
- [ ] Extrude to mesh (until surface hits mesh)
- [ ] Extrude with draft angle (tapered)
- [ ] Preview extrusion with deviation colormap
- [ ] Real-time deviation display while adjusting distance

---

#### SURF-002: Revolve from 2D Sketch
**As a** user  
**I want to** revolve 2D profiles around an axis  
**So that** I can create rotational CAD geometry

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Revolve profile around sketch axis (X or Y in sketch plane)
- [ ] Revolve around existing primitive axis
- [ ] Specify revolution angle (0-360°)
- [ ] Preview revolution with deviation colormap
- [ ] Handle profiles that don't touch axis

---

#### SURF-003: Fit NURBS Surface
**As a** user  
**I want to** fit a smooth surface to mesh region  
**So that** I can capture freeform shapes as CAD

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Fit B-spline surface to selected mesh region
- [ ] Control surface degree (1-5)
- [ ] Control surface control point density (U × V)
- [ ] Show deviation colormap (surface to mesh)
- [ ] Specify maximum deviation tolerance
- [ ] Interactive boundary adjustment
- [ ] Support multi-span surfaces
- [ ] Trim surface to boundary curves

---

#### SURF-004: Surface Trimming
**As a** user  
**I want to** trim surfaces at intersections  
**So that** I can create connected solid geometry

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Trim surface A by surface B
- [ ] Mutual trim (both surfaces cut each other)
- [ ] Select which side to keep
- [ ] Preview trim result before applying
- [ ] Automatic trim at all surface intersections
- [ ] Sew trimmed surfaces into solid

---

#### SURF-005: 3D Sketch on Mesh
**As a** user  
**I want to** draw curves directly on the mesh surface  
**So that** I can create boundaries for surface fitting

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Draw 3D spline on mesh surface (constrained to surface)
- [ ] Draw 3D polyline on mesh
- [ ] Edit 3D curve points
- [ ] Show real-time deviation from curve to mesh
- [ ] Use 3D curves as surface boundaries
- [ ] Project 2D sketch onto mesh

---

### 2.7 Export (Epic: EXPORT)

#### EXPORT-001: Export Mesh
**As a** user  
**I want to** export modified mesh  
**So that** I can use it in other applications

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Export as STL (ASCII and binary)
- [ ] Export as OBJ
- [ ] Export as PLY
- [ ] Option to export only visible/selected geometry
- [ ] Option to export with applied transforms
- [ ] File size optimization options

---

#### EXPORT-002: Export CAD (STEP/IGES)
**As a** user  
**I want to** export reconstructed CAD  
**So that** I can use it in CAD/CAM software

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Export as STEP AP203/AP214
- [ ] Export as IGES
- [ ] Export surfaces only or solid bodies
- [ ] Include primitives in export
- [ ] Include fitted surfaces in export
- [ ] Validate geometry before export (check for errors)
- [ ] Export 2D sketches as DXF option

---

### 2.8 Analysis & Comparison (Epic: ANALYSIS)

#### ANALYSIS-001: Real-time Deviation Display
**As a** user  
**I want to** see distance from CAD to mesh  
**So that** I can verify reconstruction accuracy

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Colormap display of deviation on primitives/surfaces
- [ ] Colormap display of deviation on mesh
- [ ] Configurable color scale (min/max values)
- [ ] Configurable color palette (rainbow, red-blue, etc.)
- [ ] Display statistics: min, max, mean, RMS, standard deviation
- [ ] Histogram of deviation values
- [ ] Highlight out-of-tolerance regions
- [ ] Real-time update as geometry is edited

---

#### ANALYSIS-002: Scan-to-CAD Comparison
**As a** professional user  
**I want to** generate comparison reports  
**So that** I can document quality metrics

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Generate comparison colormap with legend
- [ ] Export comparison as image (PNG, PDF)
- [ ] Export comparison statistics as CSV/Excel
- [ ] GD&T-style callouts (flatness, cylindricity, etc.)
- [ ] Comparison report template customization
- [ ] Batch comparison for multiple features

---

### 2.9 Application Infrastructure (Epic: APP)

#### APP-001: Undo/Redo
**As a** user  
**I want to** undo and redo operations  
**So that** I can correct mistakes and experiment safely

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Unlimited undo history (or configurable limit)
- [ ] Undo/Redo via Ctrl+Z / Ctrl+Y
- [ ] Undo history list view
- [ ] Undo to specific point in history
- [ ] Clear history option (to free memory)
- [ ] Works for all mesh and CAD operations

---

#### APP-002: Project Save/Load
**As a** user  
**I want to** save and load my work  
**So that** I can continue projects later

**Priority:** P0 (Must Have)

**Acceptance Criteria:**
- [ ] Save project to native format (includes all data)
- [ ] Auto-save at configurable interval
- [ ] Recent files list
- [ ] Save reminders for unsaved changes
- [ ] Project file includes: meshes, primitives, surfaces, sketches, settings
- [ ] Backward compatibility for file format versions

---

#### APP-003: Application Settings
**As a** user  
**I want to** customize application behavior  
**So that** it fits my workflow

**Priority:** P1 (Should Have)

**Acceptance Criteria:**
- [ ] Configurable color schemes (light/dark mode)
- [ ] Configurable keyboard shortcuts
- [ ] Mouse button customization
- [ ] Default units (mm, inch)
- [ ] Default tolerances
- [ ] Performance settings (GPU acceleration, LOD)
- [ ] Language localization (future)

---

## 3. Functional Requirements

### 3.1 Core Geometry Processing

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-001 | Import STL, OBJ, PLY mesh files | P0 |
| FR-002 | Render mesh with shading, wireframe, points | P0 |
| FR-003 | Navigate 3D viewport (orbit, pan, zoom) | P0 |
| FR-004 | Select mesh regions (brush, lasso, box, magic wand) | P0 |
| FR-005 | Reduce polygon count with quality control | P0 |
| FR-006 | Smooth mesh with multiple algorithms | P0 |
| FR-007 | Fill holes with smart surface generation | P0 |
| FR-008 | Remove outliers and disconnected components | P0 |
| FR-009 | Clip display/operations by bounding box | P0 |

### 3.2 Alignment

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-010 | Interactive 3D transform (translate, rotate) | P0 |
| FR-011 | Align mesh to WCS using primitives | P0 |
| FR-012 | N-point alignment for mesh registration | P1 |
| FR-013 | ICP fine alignment | P1 |
| FR-014 | Symmetry plane detection | P1 |

### 3.3 Primitive Fitting

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-015 | Fit plane with deviation analysis | P0 |
| FR-016 | Fit cylinder with deviation analysis | P0 |
| FR-017 | Fit cone with deviation analysis | P0 |
| FR-018 | Fit sphere with deviation analysis | P0 |
| FR-019 | Auto-constrain relationships (parallel, perpendicular, concentric) | P1 |
| FR-020 | Edit primitives after creation | P0 |

### 3.4 2D Sketching

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-021 | Create section planes (single, stacked) | P0 |
| FR-022 | 2D sketch mode with grid/snap | P0 |
| FR-023 | Draw line, arc, spline, circle | P0 |
| FR-024 | Auto-fit entities to section points | P0 |
| FR-025 | Trim/extend/fillet sketch entities | P0 |
| FR-026 | Geometric constraints | P1 |
| FR-027 | Dimensional constraints | P1 |

### 3.5 Surface Creation

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-028 | Extrude sketch profiles | P0 |
| FR-029 | Revolve sketch profiles | P0 |
| FR-030 | Fit NURBS surface to mesh | P1 |
| FR-031 | Trim surfaces at intersections | P1 |
| FR-032 | 3D sketching on mesh surface | P1 |

### 3.6 Export & Analysis

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-033 | Export mesh (STL, OBJ, PLY) | P0 |
| FR-034 | Export CAD (STEP, IGES) | P0 |
| FR-035 | Real-time deviation colormap | P0 |
| FR-036 | Deviation statistics display | P0 |
| FR-037 | Scan-to-CAD comparison report | P1 |
| FR-038 | Export 2D sketches as DXF | P1 |

### 3.7 Application

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-039 | Undo/Redo for all operations | P0 |
| FR-040 | Save/Load project files | P0 |
| FR-041 | Auto-save | P1 |
| FR-042 | Application settings/preferences | P1 |
| FR-043 | Keyboard shortcuts | P0 |

---

## 4. Non-Functional Requirements

### 4.1 Performance

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-001 | Maintain 60 FPS for meshes < 5M triangles | On reference hardware (RTX 3060, 32GB RAM) |
| NFR-002 | Maintain 30 FPS for meshes 5M-20M triangles | On reference hardware |
| NFR-003 | Import 10M triangle STL in < 10 seconds | On reference hardware |
| NFR-004 | Decimation of 10M triangles in < 30 seconds | On reference hardware |
| NFR-005 | Memory usage < 8GB for 10M triangle mesh | Typical working set |
| NFR-006 | Cold start to usable viewport < 5 seconds | On SSD |

### 4.2 Accuracy

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-007 | Primitive fitting accuracy | < 0.01mm RMS for clean data |
| NFR-008 | Surface fitting accuracy | User-configurable, default < 0.1mm |
| NFR-009 | Double-precision geometry throughout | No single-precision shortcuts in geometry kernel |
| NFR-010 | STEP export validates without errors | Tested with CAD package import |

### 4.3 Usability

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-011 | Time to complete basic scan-to-CAD workflow | < 30 minutes for hobbyist persona |
| NFR-012 | Tooltips on all UI elements | 100% coverage |
| NFR-013 | Error messages are actionable | Include what to do, not just what failed |
| NFR-014 | Keyboard navigable | All features accessible without mouse |

### 4.4 Reliability

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-015 | Crash-free operation | < 1 crash per 100 hours typical use |
| NFR-016 | No data loss on crash | Auto-save recovery |
| NFR-017 | Graceful handling of corrupt files | No crash, informative error |
| NFR-018 | Memory leak-free | < 1% memory growth per hour |

### 4.5 Compatibility

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-019 | Windows 10/11 (64-bit) | Full support |
| NFR-020 | macOS 12+ (Intel and Apple Silicon) | Full support |
| NFR-021 | Linux (Ubuntu 22.04+) | Best effort (P2) |
| NFR-022 | OpenGL 4.5 or Vulkan 1.2 | Minimum GPU requirement |

---

## 5. Acceptance Criteria

### 5.1 MVP Acceptance Test Scenarios

#### Scenario 1: Import and View
```
GIVEN a user launches the application
WHEN they import a 5M triangle STL file
THEN the mesh is displayed within 10 seconds
AND the user can orbit, pan, zoom smoothly at 30+ FPS
AND mesh statistics are displayed (vertex count, triangle count, bounding box)
```

#### Scenario 2: Basic Mesh Cleanup
```
GIVEN a mesh with 10M triangles, holes, and outliers
WHEN the user applies polygon reduction to 1M triangles
AND fills all holes
AND removes outliers < 100 triangles
THEN the resulting mesh is watertight
AND processing completes within 60 seconds
AND deviation from original is < 0.5mm max
```

#### Scenario 3: Primitive Fitting Workflow
```
GIVEN a scanned part with cylindrical hole and flat surface
WHEN the user selects the flat region and fits a plane
AND selects the hole and fits a cylinder
THEN the plane RMS error is displayed
AND the cylinder diameter, axis, and RMS error are displayed
AND both primitives can be used to align the mesh to WCS
```

#### Scenario 4: Sketch and Extrude
```
GIVEN an aligned mesh of a prismatic part
WHEN the user creates a cross-section perpendicular to Z-axis
AND enters 2D sketch mode
AND auto-fits lines and arcs to the section profile
AND extrudes the profile
THEN the extruded surface is created
AND deviation colormap shows fit to original mesh
AND deviation max is within user tolerance
```

#### Scenario 5: Export Workflow
```
GIVEN a mesh with fitted primitives and extruded surfaces
WHEN the user exports as STEP
THEN the file is created without errors
AND imports successfully into SolidWorks/Fusion 360
AND geometry matches within tolerance
```

---

## 6. Priority Matrix

### P0: Must Have (MVP)

These features are required for initial release:

| Feature Area | Features |
|--------------|----------|
| Import/Export | STL/OBJ/PLY import, STL export, STEP/IGES export |
| Viewport | 3D rendering, camera navigation, selection tools |
| Mesh Editing | Decimation, smoothing, hole filling, outlier removal, clipping |
| Alignment | Interactive alignment, WCS alignment |
| Primitives | Plane, cylinder, cone, sphere fitting with deviation display |
| Sketching | Section planes, 2D sketch mode, line/arc/spline drawing, auto-fit to points |
| Surfaces | Extrude, revolve from sketches |
| Analysis | Real-time deviation colormap |
| App | Undo/redo, save/load project |

### P1: Should Have (v1.x)

Essential for professional users:

| Feature Area | Features |
|--------------|----------|
| Alignment | N-point alignment, ICP, symmetry detection |
| Primitives | Auto-constrain relationships |
| Sketching | Constraints, dimensions, patterns |
| Surfaces | NURBS fitting, surface trimming, 3D sketching |
| Analysis | Scan-to-CAD comparison reports, DXF export |
| App | Auto-save, preferences, keyboard customization |

### P2: Nice to Have (v2.0+)

Pro features and polish:

| Feature Area | Features |
|--------------|----------|
| Mesh | Automatic segmentation (AI-based) |
| Surfaces | Loft, sweep, pipe, fill surface |
| Solids | Boolean operations, shell, thicken |
| Edges | Fillet, chamfer, variable fillet |
| Patterns | Linear, circular patterns, mirror |
| Import | STEP/IGES CAD import for comparison |
| Analysis | Draft analysis, straightness, GD&T |
| Automation | Auto-surfacing (one-click reconstruction) |
| Free-form | Quad-mesh subdivision surface modeling |
| Platform | Linux support |

---

## Appendix A: Glossary

| Term | Definition |
|------|------------|
| **B-Rep** | Boundary Representation — CAD solid defined by faces, edges, vertices |
| **Decimation** | Reducing triangle count while preserving shape |
| **Deviation** | Distance between reconstructed CAD and reference mesh |
| **ICP** | Iterative Closest Point — algorithm for fine alignment |
| **IGES** | Initial Graphics Exchange Specification — CAD interchange format |
| **Mesh** | 3D model represented as vertices connected by triangles |
| **NURBS** | Non-Uniform Rational B-Spline — smooth parametric surface representation |
| **Primitive** | Basic geometric shape (plane, cylinder, cone, sphere) |
| **RANSAC** | Random Sample Consensus — robust fitting algorithm for noisy data |
| **Reverse Engineering** | Creating CAD from physical part (via scanning) |
| **RMS** | Root Mean Square — statistical measure of fit quality |
| **STEP** | Standard for the Exchange of Product Data — CAD interchange format |
| **WCS** | World Coordinate System — global XYZ reference frame |

---

## Appendix B: Reference Materials

- Revo Design product page: https://www.revopoint3d.com/products/revodesign
- QuickSurface tutorials: https://www.quicksurface3d.com/?p=quicksurface7_tutorials
- QuickSurface workflow guide: https://www.quicksurface.com/3d-reverse-engineering-workflow/
- Open CASCADE documentation: https://dev.opencascade.org/doc/overview/html/index.html

---

*Document Version: 1.0*  
*Last Updated: 2026-02-12*
