# User Flows — dc-3ddesignapp

**Version:** 1.0  
**Created:** 2026-02-12  
**Author:** Requirements Lead  
**Reference:** Revo Design / QuickSurface workflows

---

## Table of Contents

1. [Overview](#1-overview)
2. [Flow 1: Basic Scan-to-Print](#2-flow-1-basic-scan-to-print)
3. [Flow 2: Parametric Reverse Engineering](#3-flow-2-parametric-reverse-engineering)
4. [Flow 3: Freeform Surface Reconstruction](#4-flow-3-freeform-surface-reconstruction)
5. [Flow 4: Quality Inspection](#5-flow-4-quality-inspection)
6. [Flow 5: Hybrid Modeling](#6-flow-5-hybrid-modeling)
7. [Common Sub-Flows](#7-common-sub-flows)
8. [Decision Trees](#8-decision-trees)

---

## 1. Overview

### 1.1 Core Workflow Paradigm

The scan-to-CAD process follows a consistent pattern across all use cases:

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   IMPORT    │ →  │   PREPARE   │ →  │  CONSTRUCT  │ →  │   VERIFY    │ →  │   EXPORT    │
│  Load mesh  │    │  Clean/Align│    │  Create CAD │    │  Check fit  │    │  Output     │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
```

### 1.2 Workflow Selection Guide

| Your Goal | Use This Flow |
|-----------|---------------|
| 3D print a scanned object (cleaned up) | Flow 1: Basic Scan-to-Print |
| Create parametric CAD of a mechanical part | Flow 2: Parametric Reverse Engineering |
| Capture organic/sculpted shape as CAD | Flow 3: Freeform Surface Reconstruction |
| Compare scan to existing CAD specification | Flow 4: Quality Inspection |
| Part has both geometric and organic features | Flow 5: Hybrid Modeling |

---

## 2. Flow 1: Basic Scan-to-Print

**Persona:** Hobbyist (Alex the Maker)  
**Goal:** Clean up a scan and export for 3D printing  
**Time:** 10-30 minutes  
**Output:** Clean STL file

### 2.1 Overview Diagram

```
┌───────────────────────────────────────────────────────────────────────────────┐
│                         BASIC SCAN-TO-PRINT FLOW                              │
├───────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│  ┌────────┐    ┌────────────┐    ┌────────┐    ┌────────┐    ┌────────────┐  │
│  │ Import │ →  │  Reduce    │ →  │ Remove │ →  │  Fill  │ →  │   Export   │  │
│  │  STL   │    │ Polygons   │    │Outliers│    │ Holes  │    │    STL     │  │
│  └────────┘    └────────────┘    └────────┘    └────────┘    └────────────┘  │
│       │              │                │             │               │         │
│       ▼              ▼                ▼             ▼               ▼         │
│   Load scan     Make workable     Clean noise   Watertight      Ready to     │
│   from file     (< 2M tris)                       mesh           print       │
│                                                                               │
└───────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Step-by-Step

#### Step 1: Import Mesh
| Action | UI Element | Details |
|--------|------------|---------|
| 1.1 | File → Import Mesh | Or drag-drop file onto viewport |
| 1.2 | Select STL/OBJ/PLY file | Choose file from file dialog |
| 1.3 | Wait for import | Progress bar for large files |
| 1.4 | Review statistics | Check vertex/triangle count in status bar |

**Expected State:** Mesh visible in viewport, can orbit/zoom

**Troubleshooting:**
- If mesh is too large (>10M triangles), proceed to Step 2 immediately
- If mesh doesn't appear, check View → Show Mesh toggle
- If mesh is tiny/huge, check import units (File → Import Settings)

---

#### Step 2: Reduce Polygon Count (if needed)
| Action | UI Element | Details |
|--------|------------|---------|
| 2.1 | Mesh → Polygon Reduction | Opens reduction dialog |
| 2.2 | Set target count | Enter target triangles or percentage |
| 2.3 | Set quality preset | "Fast" / "Balanced" / "Quality" |
| 2.4 | Enable "Preserve Boundaries" | Keeps edges clean |
| 2.5 | Click "Preview" | See result before applying |
| 2.6 | Click "Apply" | Execute reduction |

**Target:** 
- For editing: < 2M triangles
- For 3D printing: depends on printer resolution

**Quality Check:** Deviation colormap shows maximum deviation from original

---

#### Step 3: Remove Outliers
| Action | UI Element | Details |
|--------|------------|---------|
| 3.1 | Mesh → Remove Outliers | Opens outlier removal tool |
| 3.2 | Set minimum size | "Remove components < N triangles" |
| 3.3 | Click "Preview" | Highlighted regions will be removed |
| 3.4 | Adjust threshold | Increase/decrease N as needed |
| 3.5 | Click "Apply" | Execute removal |

**Typical Values:**
- Small parts: Remove < 50 triangles
- Large parts: Remove < 500 triangles
- Or use: Remove < 0.1% of mesh

---

#### Step 4: Fill Holes
| Action | UI Element | Details |
|--------|------------|---------|
| 4.1 | Mesh → Hole Detection | Highlights all boundary loops |
| 4.2 | Review hole list | Panel shows holes by size |
| 4.3 | Click "Fill All" | Or click individual holes to fill one at a time |
| 4.4 | Select fill method | "Flat" / "Smooth" / "Curvature" |
| 4.5 | Click "Apply" | Execute filling |

**Fill Method Selection:**
- **Flat:** For holes in planar regions
- **Smooth:** For holes in curved regions
- **Curvature:** For complex organic surfaces

**Quality Check:** Mesh should now be watertight (no boundary edges)

---

#### Step 5: Optional Smoothing
| Action | UI Element | Details |
|--------|------------|---------|
| 5.1 | Mesh → Smooth | Opens smoothing tool |
| 5.2 | Select method | "Laplacian" / "Taubin" / "Bilateral" |
| 5.3 | Set iterations | 1-10 for light smoothing |
| 5.4 | Click "Preview" | See result |
| 5.5 | Click "Apply" | Execute smoothing |

**When to Smooth:**
- Noisy scan data (visible surface texture)
- Stair-stepping artifacts
- DON'T over-smooth (loses detail)

---

#### Step 6: Export for 3D Printing
| Action | UI Element | Details |
|--------|------------|---------|
| 6.1 | File → Export Mesh | Opens export dialog |
| 6.2 | Choose format | STL Binary (smallest) recommended |
| 6.3 | Set filename | Choose location and name |
| 6.4 | Click "Export" | Save file |

**Pre-Export Checklist:**
- [ ] Mesh is watertight (no holes)
- [ ] No floating geometry (outliers removed)
- [ ] Size is correct (check bounding box dimensions)
- [ ] Orientation is correct (Z-up for most printers)

---

## 3. Flow 2: Parametric Reverse Engineering

**Persona:** Professional Engineer (Jordan)  
**Goal:** Create parametric CAD model of mechanical part  
**Time:** 30 minutes - 4 hours  
**Output:** STEP file with primitives, surfaces, and sketches

### 3.1 Overview Diagram

```
┌───────────────────────────────────────────────────────────────────────────────────────────────┐
│                           PARAMETRIC REVERSE ENGINEERING FLOW                                  │
├───────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                               │
│  ┌────────┐    ┌─────────┐    ┌──────────┐    ┌──────────┐    ┌─────────┐    ┌────────────┐  │
│  │ Import │ →  │ Prepare │ →  │  Align   │ →  │Extract   │ →  │ Sketch  │ →  │  Surface   │  │
│  │        │    │  Mesh   │    │  to WCS  │    │Primitives│    │ Profiles│    │  Creation  │  │
│  └────────┘    └─────────┘    └──────────┘    └──────────┘    └─────────┘    └────────────┘  │
│                                                                                               │
│                                       ▼                                                       │
│                              ┌──────────────────┐                                             │
│                              │   Trim & Join    │                                             │
│                              │    Surfaces      │                                             │
│                              └────────┬─────────┘                                             │
│                                       │                                                       │
│                                       ▼                                                       │
│                              ┌──────────────────┐                                             │
│                              │ Verify & Export  │                                             │
│                              │     STEP         │                                             │
│                              └──────────────────┘                                             │
│                                                                                               │
└───────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Step-by-Step

#### Phase A: Import and Prepare

##### Step A.1: Import Mesh
| Action | UI Element | Details |
|--------|------------|---------|
| A.1.1 | File → Import Mesh | Load STL/OBJ/PLY |
| A.1.2 | Review quality | Check for holes, noise, completeness |
| A.1.3 | Note dimensions | Record bounding box for sanity check |

##### Step A.2: Mesh Cleanup
| Action | UI Element | Details |
|--------|------------|---------|
| A.2.1 | Remove outliers | Mesh → Remove Outliers |
| A.2.2 | Fill critical holes | Only holes that affect features |
| A.2.3 | Reduce if needed | Keep detail, reduce to workable size |

**NOTE:** For parametric work, DON'T over-process the mesh. You need accurate reference data.

---

#### Phase B: Alignment to World Coordinate System

##### Step B.1: Initial Interactive Alignment
| Action | UI Element | Details |
|--------|------------|---------|
| B.1.1 | Select mesh | Click on mesh in viewport or Object Explorer |
| B.1.2 | Align → Interactive | Activate transform gizmo |
| B.1.3 | Roughly orient part | Rotate so primary axis is approximate |
| B.1.4 | Position near origin | Translate so part is centered |

##### Step B.2: Feature-Based WCS Alignment (3-2-1 Method)
| Action | UI Element | Details |
|--------|------------|---------|
| B.2.1 | **Fit primary plane** | Select flat bottom → Primitives → Fit Plane |
| B.2.2 | Align → Set Primary | "Plane normal = Z-axis" |
| B.2.3 | **Fit secondary plane** | Select side face → Fit Plane |
| B.2.4 | Align → Set Secondary | "Plane normal = X-axis (perpendicular to primary)" |
| B.2.5 | **Fit origin feature** | Select hole → Fit Cylinder, or select corner → select point |
| B.2.6 | Align → Set Origin | "Cylinder center = origin" |
| B.2.7 | Click "Apply Alignment" | Execute 3-2-1 alignment |

**Quality Check:** 
- Standard views (Front, Top, Right) should show expected orientations
- Part should sit on XY plane if bottom was primary

---

#### Phase C: Extract Primitives

##### Step C.1: Identify and Fit Planar Features
| Action | UI Element | Details |
|--------|------------|---------|
| C.1.1 | Use magic wand selection | Click on flat region |
| C.1.2 | Primitives → Fit Plane | Fit plane to selection |
| C.1.3 | Review deviation | Check RMS error in properties |
| C.1.4 | Constrain if appropriate | Set parallel/perpendicular to WCS |
| C.1.5 | Repeat for all flat faces | Build complete primitive set |

**Deviation Guidance:**
- Good fit: RMS < 0.05mm
- Acceptable: RMS < 0.1mm
- Poor (may indicate wrong feature type): RMS > 0.2mm

##### Step C.2: Identify and Fit Cylindrical Features
| Action | UI Element | Details |
|--------|------------|---------|
| C.2.1 | Select cylindrical region | Brush or magic wand on hole/shaft |
| C.2.2 | Primitives → Fit Cylinder | Fit cylinder |
| C.2.3 | Note diameter | Record for design intent |
| C.2.4 | Constrain axis | Parallel to Z or perpendicular to face |
| C.2.5 | Repeat for all cylinders | Include holes, bosses, shafts |

**Design Intent Check:**
- Is this a standard diameter? (e.g., 10.0mm not 9.87mm)
- Should this be concentric with another feature?
- Should this axis be perpendicular to a face?

##### Step C.3: Fit Other Primitives
| Action | UI Element | Details |
|--------|------------|---------|
| C.3.1 | Fit cones | For tapered features |
| C.3.2 | Fit spheres | For ball ends, rounds |
| C.3.3 | Apply constraints | Relate primitives to each other |

---

#### Phase D: 2D Sketching for Profiles

##### Step D.1: Create Cross Section
| Action | UI Element | Details |
|--------|------------|---------|
| D.1.1 | Sketch → Create Section | Opens section creation |
| D.1.2 | Select section plane | Choose existing primitive plane or XY/XZ/YZ |
| D.1.3 | Adjust offset | Position plane through feature of interest |
| D.1.4 | Set section thickness | Typically 0.5-2mm for scan noise averaging |
| D.1.5 | Click "Create" | Section points are displayed |

##### Step D.2: Enter 2D Sketch Mode
| Action | UI Element | Details |
|--------|------------|---------|
| D.2.1 | Double-click section | Or right-click → Edit Sketch |
| D.2.2 | View rotates | Now viewing perpendicular to sketch plane |
| D.2.3 | Section points visible | As point cloud or polyline |

##### Step D.3: Fit 2D Entities to Section
| Action | UI Element | Details |
|--------|------------|---------|
| D.3.1 | Select section points | Box-select region of interest |
| D.3.2 | Sketch → Fit Line | Fit line to selected points |
| D.3.3 | Select arc region | Points along curved section |
| D.3.4 | Sketch → Fit Arc | Fit arc |
| D.3.5 | Continue fitting | Build complete profile |

**Real-Time Feedback:**
- Deviation colormap shows fit quality
- Statistics show RMS error
- Adjust tolerance if fit is poor

##### Step D.4: Complete the Sketch
| Action | UI Element | Details |
|--------|------------|---------|
| D.4.1 | Trim overlaps | Sketch → Trim to clean intersections |
| D.4.2 | Add fillets | Sketch → Fillet for rounded corners |
| D.4.3 | Add constraints | Horizontal, vertical, tangent, etc. |
| D.4.4 | Add dimensions | Capture parametric values |
| D.4.5 | Exit sketch | Click "Finish Sketch" |

---

#### Phase E: Surface Creation

##### Step E.1: Extrude Profiles
| Action | UI Element | Details |
|--------|------------|---------|
| E.1.1 | Select closed profile | Click on sketch profile |
| E.1.2 | Surfaces → Extrude | Opens extrude dialog |
| E.1.3 | Set distance | Type value or drag |
| E.1.4 | Enable "To Mesh" option | Extrude until hitting reference mesh |
| E.1.5 | Review deviation | Colormap shows fit quality |
| E.1.6 | Click "Create" | Generate extruded surface |

##### Step E.2: Revolve Profiles (for rotational features)
| Action | UI Element | Details |
|--------|------------|---------|
| E.2.1 | Select profile | Profile must be on one side of axis |
| E.2.2 | Surfaces → Revolve | Opens revolve dialog |
| E.2.3 | Select axis | Use sketch axis or cylinder axis |
| E.2.4 | Set angle | Usually 360° |
| E.2.5 | Review and create | Check deviation before applying |

##### Step E.3: Create Additional Cross Sections as Needed
Repeat Phase D-E for features at different orientations/positions.

---

#### Phase F: Trim and Join

##### Step F.1: Trim Surfaces
| Action | UI Element | Details |
|--------|------------|---------|
| F.1.1 | Select two intersecting surfaces | Ctrl+click |
| F.1.2 | Surfaces → Mutual Trim | Opens trim dialog |
| F.1.3 | Select portions to keep | Click on region to keep |
| F.1.4 | Click "Apply" | Surfaces are trimmed |

##### Step F.2: Sew into Solid (optional)
| Action | UI Element | Details |
|--------|------------|---------|
| F.2.1 | Select all surfaces | Ctrl+A in surfaces |
| F.2.2 | Surfaces → Sew | Attempts to create solid body |
| F.2.3 | Review result | Check for gaps in solid |

---

#### Phase G: Verify and Export

##### Step G.1: Final Deviation Check
| Action | UI Element | Details |
|--------|------------|---------|
| G.1.1 | Analysis → Deviation Map | Shows CAD vs mesh |
| G.1.2 | Set tolerance bands | e.g., ±0.1mm green, ±0.2mm yellow, beyond red |
| G.1.3 | Review problem areas | Red regions need attention |
| G.1.4 | Generate statistics | RMS, max, min deviation |

##### Step G.2: Export STEP
| Action | UI Element | Details |
|--------|------------|---------|
| G.2.1 | File → Export CAD | Opens CAD export dialog |
| G.2.2 | Select STEP AP214 | Standard format for manufacturing |
| G.2.3 | Select bodies to export | All or selection |
| G.2.4 | Click "Export" | Save file |
| G.2.5 | Verify in target CAD | Open in SolidWorks/Inventor to confirm |

---

## 4. Flow 3: Freeform Surface Reconstruction

**Persona:** Professional Engineer or Manufacturer  
**Goal:** Capture organic/sculpted shapes that can't be defined by primitives  
**Time:** 1-4 hours  
**Output:** NURBS surfaces in STEP/IGES

### 4.1 Overview Diagram

```
┌───────────────────────────────────────────────────────────────────────────────────────────────┐
│                           FREEFORM SURFACE RECONSTRUCTION FLOW                                 │
├───────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                               │
│  ┌────────┐    ┌─────────┐    ┌──────────┐    ┌──────────┐    ┌─────────┐    ┌────────────┐  │
│  │ Import │ →  │ Prepare │ →  │  Align   │ →  │  Define  │ →  │   Fit   │ →  │   Trim &   │  │
│  │        │    │  Mesh   │    │          │    │Boundaries│    │ Surfaces│    │   Export   │  │
│  └────────┘    └─────────┘    └──────────┘    └──────────┘    └─────────┘    └────────────┘  │
│                                                                                               │
│  Alternative: Automatic Surfacing (one-click for simple organic shapes)                       │
│                                                                                               │
│  ┌────────┐    ┌─────────┐    ┌──────────────────┐    ┌────────────┐                         │
│  │ Import │ →  │ Prepare │ →  │ Auto-Surface     │ →  │   Export   │                         │
│  │        │    │  Mesh   │    │ (single click)   │    │            │                         │
│  └────────┘    └─────────┘    └──────────────────┘    └────────────┘                         │
│                                                                                               │
└───────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Step-by-Step (Manual Method)

#### Step 1: Define Surface Boundaries with 3D Sketch
| Action | UI Element | Details |
|--------|------------|---------|
| 1.1 | Sketch → 3D Sketch | Enter 3D sketch mode |
| 1.2 | Draw spline on mesh | Curve snaps to mesh surface |
| 1.3 | Define patch boundaries | Create closed loop regions |
| 1.4 | Check deviation | Curves should follow mesh closely |

**Strategy:**
- Divide complex shape into simpler patches
- Boundaries should follow natural feature lines
- 4-sided patches work best for NURBS

#### Step 2: Fit NURBS Surface to Regions
| Action | UI Element | Details |
|--------|------------|---------|
| 2.1 | Select mesh region | Within 3D sketch boundaries |
| 2.2 | Surfaces → Fit Surface | Opens surface fitting dialog |
| 2.3 | Set tolerance | Maximum allowed deviation |
| 2.4 | Set control point density | Higher = more detail, larger file |
| 2.5 | Set surface degree | Typically 3 (cubic) |
| 2.6 | Click "Fit" | Generate surface |
| 2.7 | Review deviation | Colormap shows fit quality |

**Troubleshooting Poor Fits:**
- Increase control point density
- Tighten tolerance (takes longer)
- Split into smaller patches

#### Step 3: Join and Trim Surfaces
| Action | UI Element | Details |
|--------|------------|---------|
| 3.1 | Select adjacent surfaces | Ctrl+click |
| 3.2 | Surfaces → Trim | Or Mutual Trim |
| 3.3 | Extend if needed | Surfaces → Extend to close gaps |

### 4.3 Automatic Surfacing (Quick Method)

For simple organic shapes, use one-click auto-surfacing:

| Action | UI Element | Details |
|--------|------------|---------|
| 1 | Select entire mesh | Ctrl+A or click mesh |
| 2 | Surfaces → Auto-Surface | Opens auto-surface dialog |
| 3 | Set target tolerance | e.g., 0.1mm |
| 4 | Set surface density | Low/Medium/High |
| 5 | Click "Generate" | Creates quad-mesh based NURBS |
| 6 | Review result | Check coverage and quality |
| 7 | Export | File → Export CAD |

**Best For:** Organic shapes like handles, toys, figurines  
**Not For:** Mechanical parts with precise features

---

## 5. Flow 4: Quality Inspection

**Persona:** Manufacturer (Sam the Production Lead)  
**Goal:** Compare scanned part against CAD specification  
**Time:** 5-15 minutes per part  
**Output:** Deviation report with colormap and statistics

### 5.1 Overview Diagram

```
┌───────────────────────────────────────────────────────────────────────────────────────────────┐
│                              QUALITY INSPECTION FLOW                                           │
├───────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                               │
│  ┌────────┐    ┌──────────┐    ┌──────────┐    ┌────────────┐    ┌──────────────────────────┐│
│  │ Import │ →  │  Import  │ →  │  Align   │ →  │  Compare   │ →  │   Generate Report        ││
│  │  Scan  │    │   CAD    │    │ Scan↔CAD │    │ Deviation  │    │   (colormap + stats)     ││
│  └────────┘    └──────────┘    └──────────┘    └────────────┘    └──────────────────────────┘│
│                                                                                               │
└───────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 Step-by-Step

#### Step 1: Import Scan Data
| Action | UI Element | Details |
|--------|------------|---------|
| 1.1 | File → Import Mesh | Load STL from scanner |
| 1.2 | Quick cleanup | Remove obvious outliers only |

#### Step 2: Import Reference CAD
| Action | UI Element | Details |
|--------|------------|---------|
| 2.1 | File → Import CAD | Load STEP/IGES specification |
| 2.2 | Verify import | Check that geometry loaded correctly |

#### Step 3: Align Scan to CAD
| Action | UI Element | Details |
|--------|------------|---------|
| 3.1 | Select both | Scan mesh and CAD |
| 3.2 | Align → N-Point Alignment | Pick corresponding features |
| 3.3 | Pick 3+ point pairs | On both scan and CAD |
| 3.4 | Click "Align" | Computes best-fit transform |
| 3.5 | Fine align (ICP) | Align → Fine Alignment for precision |

#### Step 4: Generate Deviation Analysis
| Action | UI Element | Details |
|--------|------------|---------|
| 4.1 | Analysis → Compare to CAD | Opens comparison tool |
| 4.2 | Set tolerance bands | e.g., ±0.1mm, ±0.2mm, ±0.5mm |
| 4.3 | Generate colormap | Distance coloring on mesh |
| 4.4 | Review statistics | Min, max, mean, RMS, std dev |

#### Step 5: Generate Report
| Action | UI Element | Details |
|--------|------------|---------|
| 5.1 | Analysis → Generate Report | Opens report wizard |
| 5.2 | Add views | Multiple viewport angles |
| 5.3 | Include statistics | Table of deviation values |
| 5.4 | Add annotations | Call out specific measurements |
| 5.5 | Export report | PDF or image format |

---

## 6. Flow 5: Hybrid Modeling

**Persona:** Professional Engineer  
**Goal:** Part with both geometric (mechanical) and organic (sculpted) features  
**Time:** 2-6 hours  
**Output:** STEP file with mixed parametric and freeform surfaces

### 6.1 Overview

Hybrid modeling combines:
- **Parametric reconstruction** for mechanical features (holes, flats, cylinders)
- **Freeform surfacing** for organic regions (handles, ergonomic shapes)

### 6.2 Strategy

```
┌───────────────────────────────────────────────────────────────────────────────────────────────┐
│                                  HYBRID MODELING STRATEGY                                      │
├───────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                               │
│  1. ANALYZE PART                                                                              │
│     └─→ Identify geometric regions (primitives, extrusions, revolutions)                     │
│     └─→ Identify freeform regions (organic, sculpted, molded)                                │
│     └─→ Identify boundaries between regions                                                   │
│                                                                                               │
│  2. RECONSTRUCT GEOMETRIC FEATURES FIRST                                                      │
│     └─→ Fit primitives (planes, cylinders, etc.)                                             │
│     └─→ Create sketch-based features (extrude, revolve)                                      │
│     └─→ These define BOUNDARIES for freeform regions                                         │
│                                                                                               │
│  3. FILL IN FREEFORM REGIONS                                                                  │
│     └─→ Use geometric feature edges as boundaries                                             │
│     └─→ Fit NURBS surfaces to remaining mesh areas                                           │
│     └─→ Ensure surfaces are tangent-continuous at boundaries                                 │
│                                                                                               │
│  4. JOIN AND VERIFY                                                                           │
│     └─→ Trim all surfaces at intersections                                                   │
│     └─→ Sew into solid (if possible)                                                         │
│     └─→ Check global deviation                                                               │
│                                                                                               │
└───────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 6.3 Example: Tool Handle with Mounting Hole

```
Part: Screwdriver handle
- Mounting hole at back (cylinder)
- Flange at hole (planes)
- Ergonomic grip (freeform)
- Flat spot for logo (plane)

Approach:
1. Fit cylinder to mounting hole
2. Fit plane to flange face
3. Fit plane to logo flat
4. Use 3D sketch to define boundary between flat and freeform
5. Fit NURBS surfaces to grip regions
6. Trim surfaces to meet parametric features
```

---

## 7. Common Sub-Flows

### 7.1 Sub-Flow: Mesh Selection Techniques

```
┌─────────────────────────────────────────────────────────────────┐
│                    MESH SELECTION DECISION                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  What are you selecting?                                        │
│                                                                 │
│  ┌─────────────────────┐                                        │
│  │ Single flat region  │ → Use Magic Wand (click, auto-grow)   │
│  └─────────────────────┘                                        │
│                                                                 │
│  ┌─────────────────────┐                                        │
│  │ Curved surface      │ → Use Brush (paint selection)          │
│  │ (cylinder, sphere)  │   Set angle threshold if needed        │
│  └─────────────────────┘                                        │
│                                                                 │
│  ┌─────────────────────┐                                        │
│  │ Specific region     │ → Use Lasso (draw outline)             │
│  │ with clear boundary │                                        │
│  └─────────────────────┘                                        │
│                                                                 │
│  ┌─────────────────────┐                                        │
│  │ Multiple separate   │ → Use Box Select + Shift (additive)    │
│  │ regions             │                                        │
│  └─────────────────────┘                                        │
│                                                                 │
│  ┌─────────────────────┐                                        │
│  │ "Everything except" │ → Select what you DON'T want           │
│  │                     │   Then Invert Selection                │
│  └─────────────────────┘                                        │
│                                                                 │
│  Selection Modifiers:                                           │
│  • Shift+click = Add to selection                               │
│  • Ctrl+click = Remove from selection                           │
│  • Grow = Expand selection by one ring                          │
│  • Shrink = Contract selection by one ring                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 7.2 Sub-Flow: Choosing Fit Method

```
┌─────────────────────────────────────────────────────────────────┐
│                    FITTING ALGORITHM CHOICE                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Is your selection clean (only the feature you want)?           │
│                                                                 │
│  YES → Use Least-Squares fitting                                │
│        (Fast, accurate for clean data)                          │
│                                                                 │
│  NO, selection includes noise/outliers → Use RANSAC fitting     │
│        (Robust to outliers, finds best subset)                  │
│                                                                 │
│  Tolerance Settings:                                            │
│  • Tight (0.01mm) - High quality scan, precise part             │
│  • Medium (0.1mm) - Standard industrial scan                    │
│  • Loose (0.5mm) - Worn part, rough scan, quick estimate        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 7.3 Sub-Flow: Troubleshooting Poor Fits

```
┌─────────────────────────────────────────────────────────────────┐
│                    FIT QUALITY TROUBLESHOOTING                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Problem: High RMS error (>0.2mm for primitives)                │
│                                                                 │
│  Check 1: Wrong feature type?                                   │
│  └─→ A worn cylinder might fit cone better                      │
│  └─→ A deformed plane might need surface fitting                │
│                                                                 │
│  Check 2: Selection includes multiple features?                 │
│  └─→ Reduce selection to single feature                         │
│  └─→ Use tighter magic wand threshold                           │
│                                                                 │
│  Check 3: Selection includes noise/outliers?                    │
│  └─→ Switch to RANSAC fitting                                   │
│  └─→ Clean mesh first (remove outliers, smooth)                 │
│                                                                 │
│  Check 4: Part is actually deformed/worn?                       │
│  └─→ This IS the correct answer                                 │
│  └─→ Decide: fit to actual or ideal geometry?                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8. Decision Trees

### 8.1 What Flow Should I Use?

```
                        ┌───────────────────┐
                        │ What's your goal? │
                        └─────────┬─────────┘
                                  │
            ┌─────────────────────┼─────────────────────┐
            │                     │                     │
            ▼                     ▼                     ▼
    ┌───────────────┐    ┌───────────────┐    ┌───────────────┐
    │ Clean & Print │    │ Create CAD    │    │ Inspect Part  │
    │   (mesh out)  │    │  (STEP out)   │    │  (report out) │
    └───────┬───────┘    └───────┬───────┘    └───────┬───────┘
            │                    │                    │
            ▼                    │                    ▼
    ┌───────────────┐            │            ┌───────────────┐
    │   FLOW 1:     │            │            │   FLOW 4:     │
    │ Scan-to-Print │            │            │  Inspection   │
    └───────────────┘            │            └───────────────┘
                                 │
                    ┌────────────┴────────────┐
                    │                         │
                    ▼                         ▼
            ┌───────────────┐         ┌───────────────┐
            │Is the part    │         │Is the part    │
            │mechanical?    │         │mostly organic?│
            │(flats, holes, │         │(freeform,     │
            │cylinders)     │         │sculpted)      │
            └───────┬───────┘         └───────┬───────┘
                    │                         │
            YES     │                 YES     │
                    ▼                         ▼
            ┌───────────────┐         ┌───────────────┐
            │   FLOW 2:     │         │   FLOW 3:     │
            │  Parametric   │         │   Freeform    │
            │    Reverse    │         │  Surfacing    │
            │  Engineering  │         │               │
            └───────────────┘         └───────────────┘
                    │                         │
                    │                         │
                    ▼                         ▼
            ┌───────────────────────────────────────┐
            │  Part has BOTH mechanical & organic?  │
            │                                       │
            │            Use FLOW 5:                │
            │         Hybrid Modeling               │
            └───────────────────────────────────────┘
```

### 8.2 What Surface Creation Method?

```
                    ┌───────────────────────────────┐
                    │ What shape are you creating?  │
                    └───────────────┬───────────────┘
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
┌───────────────┐           ┌───────────────┐           ┌───────────────┐
│   Constant    │           │  Rotational   │           │   Organic/    │
│   profile     │           │   symmetry    │           │   Freeform    │
│   (prismatic) │           │   (lathe-like)│           │   (sculpted)  │
└───────┬───────┘           └───────┬───────┘           └───────┬───────┘
        │                           │                           │
        ▼                           ▼                           │
┌───────────────┐           ┌───────────────┐                   │
│   EXTRUDE     │           │   REVOLVE     │                   │
│               │           │               │                   │
│ 2D profile +  │           │ 2D profile +  │                   │
│ linear dist   │           │ axis + angle  │                   │
└───────────────┘           └───────────────┘                   │
                                                                │
                                    ┌───────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
                    ▼                               ▼
            ┌───────────────┐               ┌───────────────┐
            │ Simple shape? │               │ Complex shape │
            │ (knob, handle)│               │ (face, animal)│
            └───────┬───────┘               └───────┬───────┘
                    │                               │
                    ▼                               ▼
            ┌───────────────┐               ┌───────────────┐
            │ AUTO-SURFACE  │               │  Manual NURBS │
            │  (one click)  │               │   Surface     │
            │               │               │   Fitting     │
            └───────────────┘               └───────────────┘
```

---

## Appendix A: Keyboard Shortcuts Reference

| Action | Shortcut | Context |
|--------|----------|---------|
| Orbit | MMB Drag | Viewport |
| Pan | Shift + MMB | Viewport |
| Zoom | Scroll | Viewport |
| Fit View | F | Viewport |
| Front View | Numpad 1 | Viewport |
| Right View | Numpad 3 | Viewport |
| Top View | Numpad 7 | Viewport |
| Perspective/Ortho | Numpad 5 | Viewport |
| Undo | Ctrl + Z | Global |
| Redo | Ctrl + Y | Global |
| Save | Ctrl + S | Global |
| Select All | Ctrl + A | Global |
| Deselect | Escape | Selection |
| Invert Selection | Ctrl + I | Selection |
| Delete | Delete | Selected objects |
| Hide | H | Selected objects |
| Show All | Alt + H | Viewport |
| Exit Mode | Escape | Sketch/Edit mode |

---

## Appendix B: Time Estimates by Workflow

| Workflow | Simple Part | Medium Part | Complex Part |
|----------|-------------|-------------|--------------|
| Flow 1: Scan-to-Print | 5 min | 15 min | 30 min |
| Flow 2: Parametric | 30 min | 2 hours | 4+ hours |
| Flow 3: Freeform | 20 min | 1 hour | 3+ hours |
| Flow 4: Inspection | 5 min | 10 min | 20 min |
| Flow 5: Hybrid | 1 hour | 3 hours | 6+ hours |

**Simple:** Few features, clean scan, standard shapes  
**Medium:** Multiple features, moderate complexity, some noise  
**Complex:** Many features, organic + mechanical, poor scan quality

---

*Document Version: 1.0*  
*Last Updated: 2026-02-12*
