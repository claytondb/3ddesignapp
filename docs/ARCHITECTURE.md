# System Architecture — dc-3ddesignapp

**Version:** 1.0  
**Date:** 2026-02-12  
**Status:** Design Phase

---

## Overview

dc-3ddesignapp is a cross-platform scan-to-CAD application designed to convert 3D scanned meshes into editable CAD models. The architecture prioritizes:

- **Performance:** Handle 10M+ triangle meshes at interactive frame rates
- **Modularity:** Clear separation between UI, geometry engine, and I/O
- **Extensibility:** Plugin-ready architecture for future features
- **Cross-platform:** Identical behavior on Windows and macOS

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              APPLICATION LAYER                               │
│  ┌─────────────────┐  ┌──────────────────┐  ┌─────────────────────────────┐ │
│  │   Main Window   │  │   Tool Panels    │  │      Property Editor        │ │
│  │    (Qt 6)       │  │   (Qt Widgets)   │  │       (Qt Widgets)          │ │
│  └────────┬────────┘  └────────┬─────────┘  └──────────────┬──────────────┘ │
│           │                    │                           │                 │
│           └────────────────────┼───────────────────────────┘                 │
│                                ▼                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │                        VIEWPORT / RENDERER                            │   │
│  │   ┌──────────────┐  ┌──────────────┐  ┌───────────────────────────┐  │   │
│  │   │ OpenGL 4.5   │  │  Camera Ctrl │  │   Selection / Picking     │  │   │
│  │   │  Context     │  │  (Orbit/Pan) │  │   (GPU Ray Casting)       │  │   │
│  │   └──────────────┘  └──────────────┘  └───────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              CORE SERVICES                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │    Scene     │  │   Document   │  │  Undo/Redo   │  │   Selection    │  │
│  │   Manager    │  │   Manager    │  │    Stack     │  │    Manager     │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  └────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │  Command     │  │   Settings   │  │   Progress   │  │   Threading    │  │
│  │  Dispatcher  │  │   Manager    │  │   Reporter   │  │     Pool       │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  └────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           GEOMETRY ENGINE                                    │
│  ┌───────────────────────────────┐  ┌────────────────────────────────────┐  │
│  │       MESH MODULE             │  │         CAD MODULE                 │  │
│  │  ┌─────────────────────────┐  │  │  ┌──────────────────────────────┐  │  │
│  │  │ MeshData (Half-Edge)    │  │  │  │ Open CASCADE Geometry Kernel │  │  │
│  │  │ • Vertices, Faces       │  │  │  │ • BRep Shapes                │  │  │
│  │  │ • Normals, UVs          │  │  │  │ • NURBS Surfaces             │  │  │
│  │  │ • BVH Acceleration      │  │  │  │ • Topology                   │  │  │
│  │  └─────────────────────────┘  │  │  └──────────────────────────────┘  │  │
│  │  ┌─────────────────────────┐  │  │  ┌──────────────────────────────┐  │  │
│  │  │ Mesh Algorithms         │  │  │  │ CAD Operations               │  │  │
│  │  │ • Decimation (QEM)      │  │  │  │ • Primitive Fitting          │  │  │
│  │  │ • Smoothing (Laplacian) │  │  │  │ • Surface Fitting            │  │  │
│  │  │ • Hole Filling          │  │  │  │ • Extrude/Revolve            │  │  │
│  │  │ • Outlier Removal       │  │  │  │ • Boolean Ops                │  │  │
│  │  │ • Segmentation          │  │  │  │ • Fillet/Chamfer             │  │  │
│  │  └─────────────────────────┘  │  │  └──────────────────────────────┘  │  │
│  └───────────────────────────────┘  └────────────────────────────────────┘  │
│  ┌───────────────────────────────┐  ┌────────────────────────────────────┐  │
│  │     SKETCH MODULE             │  │       ANALYSIS MODULE              │  │
│  │  ┌─────────────────────────┐  │  │  ┌──────────────────────────────┐  │  │
│  │  │ 2D Sketch Engine        │  │  │  │ Deviation Analysis           │  │  │
│  │  │ • Lines, Arcs, Splines  │  │  │  │ • Point-to-Surface Distance  │  │  │
│  │  │ • Constraints Solver    │  │  │  │ • Colormap Generation        │  │  │
│  │  │ • Auto-fitting          │  │  │  │ • Statistics                 │  │  │
│  │  └─────────────────────────┘  │  │  └──────────────────────────────┘  │  │
│  └───────────────────────────────┘  └────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              I/O LAYER                                       │
│  ┌───────────────────────────────┐  ┌────────────────────────────────────┐  │
│  │       MESH I/O                │  │          CAD I/O                   │  │
│  │  • STL (ASCII/Binary)         │  │  • STEP (AP203/AP214/AP242)        │  │
│  │  • OBJ (with MTL)             │  │  • IGES                            │  │
│  │  • PLY (ASCII/Binary)         │  │  • Native Format (.dca)            │  │
│  │  • 3MF (future)               │  │                                    │  │
│  └───────────────────────────────┘  └────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Module Breakdown

### 1. Application Layer (`app/`)

The application layer handles all user-facing components.

```
app/
├── main.cpp                    # Entry point
├── MainWindow.h/cpp            # Main application window
├── ViewportWidget.h/cpp        # OpenGL viewport container
├── panels/
│   ├── ToolPanel.h/cpp         # Tool selection sidebar
│   ├── PropertyPanel.h/cpp     # Property editor
│   ├── TreePanel.h/cpp         # Scene tree view
│   └── HistoryPanel.h/cpp      # Undo history
├── dialogs/
│   ├── ImportDialog.h/cpp      # Import settings
│   ├── ExportDialog.h/cpp      # Export settings
│   └── PreferencesDialog.h/cpp # Application preferences
└── widgets/
    ├── ColorPicker.h/cpp       # Color selection widget
    ├── VectorInput.h/cpp       # XYZ input widget
    └── SliderSpinBox.h/cpp     # Combined slider/spinbox
```

**Key Classes:**

| Class | Responsibility |
|-------|----------------|
| `MainWindow` | Application frame, menu bar, status bar, panel layout |
| `ViewportWidget` | Hosts OpenGL context, forwards input to camera/selection |
| `ToolPanel` | Tool buttons organized by category |
| `PropertyPanel` | Displays and edits properties of selected objects |

### 2. Renderer Module (`renderer/`)

GPU-accelerated rendering using OpenGL 4.5.

```
renderer/
├── Renderer.h/cpp              # Main renderer orchestrator
├── RenderContext.h/cpp         # OpenGL context management
├── Camera.h/cpp                # Camera with projection/view matrices
├── CameraController.h/cpp      # Orbit/pan/zoom input handling
├── shaders/
│   ├── mesh.vert               # Mesh vertex shader
│   ├── mesh.frag               # Mesh fragment shader (Phong)
│   ├── wireframe.geom          # Wireframe geometry shader
│   ├── deviation.frag          # Deviation colormap shader
│   └── picking.frag            # GPU picking shader
├── RenderableMesh.h/cpp        # GPU mesh representation (VAO/VBO)
├── RenderableCAD.h/cpp         # Tessellated CAD shape rendering
├── GridRenderer.h/cpp          # Ground plane grid
├── AxisRenderer.h/cpp          # Coordinate axis indicator
├── SelectionRenderer.h/cpp     # Highlight selected objects
└── ClippingBox.h/cpp           # Clipping volume rendering
```

**Rendering Pipeline:**

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Culling    │───▶│  Depth Pass  │───▶│  Color Pass  │───▶│ Overlay Pass │
│ (Frustum/BVH)│    │   (Z-only)   │    │  (Shading)   │    │ (Selection)  │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
```

### 3. Core Module (`core/`)

Central services shared across all modules.

```
core/
├── SceneManager.h/cpp          # Scene graph management
├── SceneNode.h/cpp             # Base scene node class
├── DocumentManager.h/cpp       # File save/load, dirty tracking
├── Document.h/cpp              # Document state container
├── UndoStack.h/cpp             # Undo/redo command stack
├── Command.h/cpp               # Base command class
├── SelectionManager.h/cpp      # Track selected objects
├── SettingsManager.h/cpp       # Persistent user preferences
├── ProgressReporter.h/cpp      # Progress callback interface
├── ThreadPool.h/cpp            # Worker thread management
└── Types.h                     # Common type definitions
```

**Scene Graph Structure:**

```
Document
└── Scene
    ├── MeshNode (imported scan)
    │   ├── Transform
    │   └── MeshData*
    ├── CADNode (fitted surface)
    │   ├── Transform
    │   └── TopoDS_Shape*
    ├── SketchNode (2D sketch)
    │   ├── SketchPlane
    │   └── SketchEntities[]
    └── AnnotationNode (dimensions)
```

### 4. Geometry Module (`geometry/`)

The computational heart of the application.

```
geometry/
├── mesh/
│   ├── MeshData.h/cpp          # Core mesh data structure
│   ├── HalfEdge.h/cpp          # Half-edge connectivity
│   ├── MeshBuilder.h/cpp       # Construct meshes programmatically
│   ├── MeshBVH.h/cpp           # Bounding volume hierarchy
│   ├── MeshDecimator.h/cpp     # QEM polygon reduction
│   ├── MeshSmoother.h/cpp      # Laplacian/Taubin smoothing
│   ├── HoleFiller.h/cpp        # Detect and fill holes
│   ├── OutlierRemover.h/cpp    # Remove floating geometry
│   ├── MeshClipper.h/cpp       # Clip to bounding box
│   └── MeshSegmenter.h/cpp     # Region segmentation
├── cad/
│   ├── OCCTWrapper.h/cpp       # Open CASCADE integration facade
│   ├── ShapeBuilder.h/cpp      # Build OCCT shapes
│   ├── PrimitiveFitter.h/cpp   # Fit planes, cylinders, spheres
│   ├── SurfaceFitter.h/cpp     # NURBS surface fitting
│   ├── BooleanOps.h/cpp        # Union, intersection, difference
│   ├── Filleter.h/cpp          # Fillet/chamfer operations
│   └── ShapeHealer.h/cpp       # Fix imported CAD geometry
├── sketch/
│   ├── SketchPlane.h/cpp       # 2D sketch coordinate system
│   ├── SketchEntity.h/cpp      # Base sketch element
│   ├── SketchLine.h/cpp        # Line segment
│   ├── SketchArc.h/cpp         # Circular arc
│   ├── SketchSpline.h/cpp      # B-spline curve
│   ├── ConstraintSolver.h/cpp  # Geometric constraint system
│   └── SketchAutoFit.h/cpp     # Fit entities to point cloud
├── analysis/
│   ├── DeviationAnalyzer.h/cpp # Mesh-to-CAD comparison
│   ├── SectionPlane.h/cpp      # Cross-section extraction
│   └── MassProperties.h/cpp    # Volume, surface area, etc.
└── math/
    ├── Vec3.h                  # 3D vector (double precision)
    ├── Mat4.h                  # 4x4 transformation matrix
    ├── BoundingBox.h/cpp       # AABB
    ├── Plane.h/cpp             # Plane equation
    ├── Ray.h/cpp               # Ray for picking
    └── Fitting.h/cpp           # Least-squares fitting utilities
```

**Mesh Data Structure:**

```cpp
// Core mesh representation using half-edge structure
struct Vertex {
    Vec3d position;
    Vec3d normal;      // Per-vertex normal
    Vec2d uv;          // Optional texture coords
    uint32_t halfEdge; // One outgoing half-edge
};

struct HalfEdge {
    uint32_t vertex;   // Target vertex
    uint32_t face;     // Adjacent face
    uint32_t next;     // Next half-edge in face loop
    uint32_t prev;     // Previous half-edge
    uint32_t twin;     // Opposite half-edge (or INVALID)
};

struct Face {
    uint32_t halfEdge; // One half-edge on boundary
    Vec3d normal;      // Face normal
};

class MeshData {
    std::vector<Vertex> vertices;
    std::vector<HalfEdge> halfEdges;
    std::vector<Face> faces;
    BoundingBox bounds;
    MeshBVH* bvh;      // Lazy-built acceleration structure
};
```

### 5. I/O Module (`io/`)

File format readers and writers.

```
io/
├── MeshIO.h/cpp                # Mesh format dispatcher
├── readers/
│   ├── STLReader.h/cpp         # STL (ASCII + Binary)
│   ├── OBJReader.h/cpp         # Wavefront OBJ
│   ├── PLYReader.h/cpp         # Stanford PLY
│   └── ThreeMFReader.h/cpp     # 3MF format (future)
├── writers/
│   ├── STLWriter.h/cpp         # STL export
│   ├── OBJWriter.h/cpp         # OBJ export
│   └── PLYWriter.h/cpp         # PLY export
├── CADIO.h/cpp                 # CAD format dispatcher
├── STEPReader.h/cpp            # STEP import via OCCT
├── STEPWriter.h/cpp            # STEP export via OCCT
├── IGESReader.h/cpp            # IGES import via OCCT
├── IGESWriter.h/cpp            # IGES export via OCCT
└── NativeFormat.h/cpp          # Native .dca project format
```

---

## Data Flow

### Import Mesh Flow

```
┌─────────┐    ┌───────────┐    ┌───────────┐    ┌───────────┐    ┌──────────┐
│  File   │───▶│  Reader   │───▶│  MeshData │───▶│ MeshNode  │───▶│  Scene   │
│ (*.stl) │    │(STLReader)│    │ (CPU)     │    │(SceneNode)│    │ (render) │
└─────────┘    └───────────┘    └───────────┘    └───────────┘    └──────────┘
                                      │
                                      ▼
                               ┌───────────┐
                               │   Build   │
                               │    BVH    │
                               └───────────┘
                                      │
                                      ▼
                               ┌───────────┐
                               │  Upload   │
                               │  to GPU   │
                               └───────────┘
```

### Primitive Fitting Flow

```
┌──────────┐    ┌───────────┐    ┌───────────┐    ┌───────────┐
│ Selected │───▶│  RANSAC   │───▶│  OCCT     │───▶│  CADNode  │
│ Mesh     │    │  Sample   │    │ BRepPrim  │    │ (Shape)   │
│ Region   │    │  Points   │    │ Builder   │    │           │
└──────────┘    └───────────┘    └───────────┘    └───────────┘
                                      │
                                      ▼
                               ┌───────────┐
                               │ Deviation │
                               │  Check    │
                               └───────────┘
```

### CAD Export Flow

```
┌──────────┐    ┌───────────┐    ┌───────────┐    ┌───────────┐
│  Scene   │───▶│  Collect  │───▶│  Shape    │───▶│  STEP     │
│ CADNodes │    │  Shapes   │    │  Healing  │    │  Writer   │
└──────────┘    └───────────┘    └───────────┘    └───────────┘
                                                        │
                                                        ▼
                                                 ┌───────────┐
                                                 │  *.step   │
                                                 │   File    │
                                                 └───────────┘
```

---

## File Format Specifications

### Native Project Format (`.dca`)

The native format preserves full project state including:

- Mesh data (compressed)
- CAD shapes (OCCT BRep format)
- Sketches and constraints
- Undo history
- View settings

**Structure (ZIP-based):**

```
project.dca (ZIP archive)
├── manifest.json           # Version, metadata
├── document.json           # Scene structure
├── meshes/
│   └── mesh_001.bin        # Compressed mesh data
├── shapes/
│   └── shape_001.brep      # OCCT BRep
├── sketches/
│   └── sketch_001.json     # Sketch entities + constraints
├── thumbnails/
│   └── preview.png         # Project thumbnail
└── settings.json           # Project-specific settings
```

**manifest.json Schema:**

```json
{
  "version": "1.0",
  "app": "dc-3ddesignapp",
  "appVersion": "0.1.0",
  "created": "2026-02-12T10:30:00Z",
  "modified": "2026-02-12T14:22:00Z",
  "units": "mm"
}
```

**document.json Schema:**

```json
{
  "scene": {
    "nodes": [
      {
        "id": "uuid-1",
        "type": "mesh",
        "name": "Imported Scan",
        "visible": true,
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
        "dataRef": "meshes/mesh_001.bin"
      },
      {
        "id": "uuid-2",
        "type": "cad",
        "name": "Fitted Cylinder",
        "visible": true,
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 10,0,0,1],
        "dataRef": "shapes/shape_001.brep"
      }
    ]
  }
}
```

### Mesh Binary Format (Internal)

For fast mesh serialization:

```
Header (32 bytes):
  - Magic: "DCAM" (4 bytes)
  - Version: uint32
  - VertexCount: uint64
  - FaceCount: uint64
  - Flags: uint32 (hasNormals, hasUVs, etc.)
  - Reserved: 4 bytes

Vertex Data:
  - Positions: float64[VertexCount * 3]
  - Normals: float32[VertexCount * 3] (if hasNormals)
  - UVs: float32[VertexCount * 2] (if hasUVs)

Face Data:
  - Indices: uint32[FaceCount * 3]

Optional:
  - BVH serialization (for fast reload)
```

---

## Memory Management Strategy

### Challenge: Large Mesh Handling

Scans can exceed 100M triangles (several GB of vertex data). Memory strategy:

### 1. Chunked Loading

```cpp
class ChunkedMesh {
    struct Chunk {
        BoundingBox bounds;
        std::vector<uint32_t> faceIndices;  // Indices into main mesh
        bool loadedToGPU = false;
        GLuint vaoId = 0;
    };
    
    std::vector<Chunk> chunks;           // Spatial partition
    static constexpr size_t CHUNK_SIZE = 100000;  // faces per chunk
};
```

### 2. Level of Detail (LOD)

```cpp
class MeshLOD {
    MeshData* original;        // Full resolution
    MeshData* lod1;            // 50% decimated
    MeshData* lod2;            // 25% decimated
    MeshData* lod3;            // 10% decimated
    
    // Select LOD based on screen-space size
    MeshData* selectLOD(float screenSize) const;
};
```

### 3. GPU Memory Budget

```cpp
class GPUMemoryManager {
    static constexpr size_t GPU_BUDGET = 2ULL * 1024 * 1024 * 1024;  // 2GB
    
    struct GPUAllocation {
        GLuint bufferId;
        size_t size;
        uint64_t lastUsedFrame;
    };
    
    std::vector<GPUAllocation> allocations;
    
    // Evict least-recently-used when over budget
    void enforeBudget();
};
```

### 4. Lazy Computation

```cpp
class MeshData {
    mutable std::unique_ptr<MeshBVH> bvh_;
    mutable std::atomic<bool> bvhBuilding_{false};
    
    const MeshBVH& getBVH() const {
        if (!bvh_) {
            // Build on first access (or in background)
            buildBVHAsync();
        }
        return *bvh_;
    }
};
```

### 5. Object Pooling for Algorithms

```cpp
class AlgorithmContext {
    // Reusable scratch buffers
    std::vector<Vec3d> tempPositions;
    std::vector<uint32_t> tempIndices;
    std::vector<bool> tempFlags;
    
    // Avoid allocation during operations
    void reserveFor(size_t vertexCount, size_t faceCount);
};
```

### Memory Layout Guidelines

| Data Type | Storage | Notes |
|-----------|---------|-------|
| Vertex positions | `double[3]` | Precision required for CAD |
| Normals | `float[3]` | Single precision sufficient |
| Indices | `uint32_t` | 4B vertices max (sufficient) |
| Colors | `uint8_t[4]` | RGBA for deviation maps |
| OCCT Shapes | `Handle<>` | OCCT smart pointers |

---

## Threading Model

### Thread Architecture

```
┌───────────────────────────────────────────────────────────────┐
│                        MAIN THREAD                            │
│  • Qt event loop                                              │
│  • UI updates                                                 │
│  • OpenGL rendering                                           │
│  • Command dispatch                                           │
└───────────────────────────────────────────────────────────────┘
        │                    │                    │
        ▼                    ▼                    ▼
┌───────────────┐  ┌─────────────────┐  ┌────────────────────┐
│ Worker Pool   │  │  I/O Thread     │  │  BVH Builder       │
│ (CPU-bound)   │  │  (File ops)     │  │  (Background)      │
│ • Decimation  │  │  • Load mesh    │  │  • Build BVH       │
│ • Smoothing   │  │  • Save project │  │  • Ray queries     │
│ • Fitting     │  │  • Export CAD   │  │                    │
└───────────────┘  └─────────────────┘  └────────────────────┘
```

### Synchronization Patterns

```cpp
// Pattern 1: Progress reporting from worker
class ProgressReporter {
    std::function<void(float)> callback;  // Called on main thread via signal
    std::atomic<bool> cancelRequested{false};
    
    bool reportProgress(float percent) {
        emit progressSignal(percent);  // Qt will queue to main thread
        return !cancelRequested.load();
    }
};

// Pattern 2: Result delivery
template<typename T>
class AsyncResult {
    std::future<T> future;
    std::function<void(T)> onComplete;  // Called on main thread
};
```

---

## Error Handling Strategy

### Error Categories

| Category | Handling | Example |
|----------|----------|---------|
| **User Error** | Message dialog | Invalid file format |
| **Recoverable** | Retry/fallback | Network timeout |
| **Logic Error** | Assert (debug) | Null pointer |
| **System Error** | Graceful exit | Out of memory |

### Exception Boundaries

```cpp
// Commands catch and report errors
class Command {
public:
    virtual void execute() = 0;
    
protected:
    void reportError(const QString& message) {
        emit errorOccurred(message);  // Display to user
    }
};

// Algorithms don't throw; return Result<T>
template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;
    
    bool ok() const { return value.has_value(); }
    explicit operator bool() const { return ok(); }
};
```

---

## Testing Architecture

```
tests/
├── unit/
│   ├── mesh/
│   │   ├── test_MeshData.cpp
│   │   ├── test_Decimation.cpp
│   │   └── test_BVH.cpp
│   ├── cad/
│   │   ├── test_PrimitiveFitting.cpp
│   │   └── test_BooleanOps.cpp
│   └── io/
│       ├── test_STL.cpp
│       └── test_STEP.cpp
├── integration/
│   ├── test_ImportExport.cpp
│   └── test_WorkflowScenarios.cpp
├── fixtures/
│   ├── cube.stl
│   ├── sphere.ply
│   └── complex_scan.obj
└── CMakeLists.txt
```

---

## Build Configuration

### Directory Structure

```
dc-3ddesignapp/
├── CMakeLists.txt              # Root CMake
├── cmake/
│   ├── FindOCCT.cmake          # Find Open CASCADE
│   └── CompilerWarnings.cmake  # Warning flags
├── src/
│   ├── app/                    # Application layer
│   ├── core/                   # Core services
│   ├── geometry/               # Geometry engine
│   ├── renderer/               # OpenGL renderer
│   └── io/                     # File I/O
├── include/                    # Public headers
├── tests/                      # Test suite
├── resources/
│   ├── shaders/                # GLSL shaders
│   ├── icons/                  # UI icons
│   └── translations/           # i18n files
├── docs/                       # Documentation
└── third_party/               # Vendored dependencies
```

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-02-12 | Technical Architect | Initial architecture design |
