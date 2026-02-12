# Technical Decisions — dc-3ddesignapp

**Version:** 1.0  
**Date:** 2026-02-12  
**Status:** Approved

---

## Overview

This document records the key technology decisions for dc-3ddesignapp with rationale, alternatives considered, and trade-offs.

---

## 1. UI Framework: Qt 6

### Decision

Use **Qt 6** (latest LTS) as the UI framework.

### Rationale

| Factor | Qt 6 | Alternatives Considered |
|--------|------|------------------------|
| **Cross-platform** | ✅ Windows, macOS, Linux with single codebase | wxWidgets (less polished), GTK (poor Windows support) |
| **CAD ecosystem** | ✅ FreeCAD, QCAD, and many commercial CAD apps use Qt | Electron has no CAD precedent at this performance level |
| **OpenGL integration** | ✅ First-class `QOpenGLWidget` support | Native frameworks require more boilerplate |
| **Performance** | ✅ Native C++, minimal overhead | Electron adds 200MB+ runtime, slower startup |
| **Widgets vs QML** | ✅ Widgets for complex dockable panels | QML better for mobile, not desktop CAD |
| **License** | ✅ LGPL allows commercial use | No concerns |
| **Documentation** | ✅ Excellent docs and community | Very mature (25+ years) |

### Alternatives Rejected

#### Electron + React

**Pros:**
- Faster UI development (web tech)
- Large hiring pool
- Three.js for 3D

**Cons:**
- Performance: 3-5x slower than native for compute
- Memory: 500MB+ baseline vs 50MB for Qt
- Large meshes: JavaScript lacks efficient memory handling
- WebAssembly OCCT: Limited to 4GB address space, slow interop

**Verdict:** Unacceptable for professional CAD with 100M+ triangle meshes.

#### wxWidgets

**Pros:**
- More "native" look on each platform
- Lighter than Qt

**Cons:**
- Less polished widgets (no docking framework)
- Smaller community
- Limited 3D integration

**Verdict:** Qt's ecosystem is worth minor native look trade-off.

#### imgui (Dear ImGui)

**Pros:**
- Excellent for real-time 3D apps
- Minimal code for UI

**Cons:**
- Immediate-mode paradigm harder for complex forms
- No file dialogs, menus (need OS integration)
- Property editing UX inferior

**Verdict:** Great for tools/games; not ideal for professional CAD UX.

### Qt 6 Configuration

```cmake
# Required Qt modules
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
)
```

**Key Qt Features Used:**
- `QMainWindow` with dock widgets
- `QOpenGLWidget` for viewport
- `QUndoStack` for undo/redo
- `QSettings` for preferences
- Signal/slot for decoupled communication
- Qt's model/view for scene tree

---

## 2. Geometry Kernel: Open CASCADE Technology (OCCT)

### Decision

Use **Open CASCADE Technology 7.8+** as the B-Rep geometry kernel.

### Rationale

| Factor | OCCT | Alternatives |
|--------|------|--------------|
| **License** | ✅ LGPL 2.1 (commercial-friendly) | CGAL: GPL (viral), Parasolid: Expensive |
| **B-Rep modeling** | ✅ Full boundary representation | CGAL: Excellent, Parasolid: Industry leader |
| **NURBS surfaces** | ✅ Comprehensive | CGAL: Limited, libigl: None |
| **STEP/IGES** | ✅ Built-in readers/writers | Others need external libraries |
| **Boolean ops** | ✅ Robust (after v7.x improvements) | CGAL: Excellent, Parasolid: Best |
| **Industry use** | ✅ FreeCAD, BRL-CAD, many commercial | Proven in production |
| **Learning curve** | ⚠️ Steep, complex API | CGAL: Also complex |
| **Documentation** | ⚠️ Adequate but dated | CGAL: Better tutorials |

### Alternatives Rejected

#### Siemens Parasolid

**Pros:**
- Industry standard (used by SolidWorks, Siemens NX)
- Most robust Boolean operations
- Excellent tolerance handling

**Cons:**
- **Cost:** Licensing fees prohibitive for startup
- **Closed source:** No debugging, customization

**Verdict:** Best technically, but economics don't work for this project.

#### CGAL (Computational Geometry Algorithms Library)

**Pros:**
- Excellent mesh processing algorithms
- Strong academic foundation
- Good for Voronoi, convex hulls, etc.

**Cons:**
- **GPL license:** Would require open-sourcing our code
- No STEP/IGES support
- Not designed for interactive CAD modeling

**Verdict:** Use CGAL algorithms where needed, but not as primary kernel.

#### libigl

**Pros:**
- Header-only, easy to integrate
- Great mesh processing (decimation, smoothing, etc.)
- Clean API

**Cons:**
- No B-Rep modeling
- No NURBS surfaces
- No CAD file formats

**Verdict:** **Use alongside OCCT** for mesh processing algorithms.

### OCCT Integration Strategy

```cpp
// Facade pattern to isolate OCCT complexity
class OCCTWrapper {
public:
    // High-level operations hide OCCT details
    static TopoDS_Shape createCylinder(const Vec3d& origin, 
                                        const Vec3d& axis, 
                                        double radius, 
                                        double height);
    
    static TopoDS_Shape fitPlaneToPoints(const std::vector<Vec3d>& points);
    
    static bool exportSTEP(const std::vector<TopoDS_Shape>& shapes,
                           const std::filesystem::path& path);
};
```

### OCCT Modules Used

| Module | Purpose |
|--------|---------|
| **TKernel** | Foundation classes, memory management |
| **TKMath** | Math classes (gp_Pnt, gp_Vec, gp_Trsf) |
| **TKG2d, TKG3d** | 2D/3D geometry primitives |
| **TKGeomBase** | Curves and surfaces |
| **TKGeomAlgo** | Geometric algorithms |
| **TKTopAlgo** | Topology algorithms |
| **TKPrim** | Primitive shapes (box, cylinder, etc.) |
| **TKBRep** | B-Rep data structures |
| **TKBO** | Boolean operations |
| **TKFillet** | Fillets and chamfers |
| **TKOffset** | Offset surfaces, shelling |
| **TKMesh** | Tessellation for visualization |
| **TKSTEP** | STEP import/export |
| **TKIGES** | IGES import/export |
| **TKShHealing** | Shape healing and repair |

---

## 3. Rendering: OpenGL 4.5

### Decision

Use **OpenGL 4.5** (Core Profile) for 3D rendering.

### Rationale

| Factor | OpenGL 4.5 | Vulkan |
|--------|------------|--------|
| **Maturity** | ✅ Stable, well-understood | Newer, more complex |
| **Cross-platform** | ✅ Windows, macOS*, Linux | ✅ Windows, Linux (*no macOS) |
| **Learning curve** | ✅ Moderate | ⚠️ Steep |
| **Qt integration** | ✅ `QOpenGLWidget` built-in | Requires custom integration |
| **Performance** | ✅ Sufficient for CAD | Better for game-level perf |
| **Driver support** | ✅ Universal | ⚠️ Requires newer GPUs |
| **Development time** | ✅ Faster iteration | 2-3x more boilerplate |

### macOS Consideration

macOS deprecated OpenGL (supports up to 4.1) but still works:

```cpp
// macOS: Request 4.1 Core Profile
// Windows/Linux: Request 4.5 Core Profile
#ifdef __APPLE__
    format.setVersion(4, 1);
#else
    format.setVersion(4, 5);
#endif
format.setProfile(QSurfaceFormat::CoreProfile);
```

For advanced features (compute shaders, SSBOs), we'll implement fallbacks for macOS.

### Vulkan Rejected (For Now)

**Pros of Vulkan:**
- Better multi-threading
- Lower driver overhead
- More control over GPU memory

**Cons of Vulkan:**
- No native macOS support (MoltenVK is a translation layer)
- 3-5x more code for same functionality
- Overkill for CAD (not rendering complex game scenes)

**Future Path:** If performance bottlenecks emerge, consider Vulkan via Qt's `QVulkanWindow` or abstract rendering backend.

### Metal on macOS?

Considered Apple Metal for native macOS performance:
- **Rejected** because it would require maintaining two separate rendering backends
- OpenGL 4.1 on macOS is sufficient for our needs
- Revisit if Apple fully removes OpenGL support

### OpenGL Features Used

```glsl
// Vertex Shader (mesh.vert)
#version 410 core  // macOS compatible

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec3 vPosition;

void main() {
    vPosition = vec3(uModelView * vec4(aPosition, 1.0));
    vNormal = normalize(uNormalMatrix * aNormal);
    gl_Position = uProjection * vec4(vPosition, 1.0);
}
```

### Rendering Requirements

| Feature | Implementation |
|---------|---------------|
| Mesh rendering | VBO + VAO, indexed triangles |
| Wireframe overlay | Geometry shader (GL 3.2+) or line mode |
| Selection highlight | Stencil buffer + edge detection |
| Deviation colormap | Per-vertex colors, custom fragment shader |
| Clipping planes | `gl_ClipDistance` (6 planes max) |
| Transparency | Sorted rendering or OIT |
| Anti-aliasing | MSAA (4x default) |

---

## 4. Build System: CMake

### Decision

Use **CMake 3.21+** as the build system.

### Rationale

| Factor | CMake | Alternatives |
|--------|-------|--------------|
| **Industry standard** | ✅ De facto for C++ | qmake (Qt-only), Meson (newer) |
| **Cross-platform** | ✅ Windows, macOS, Linux | All viable options support this |
| **Qt support** | ✅ First-class since Qt 5.15 | qmake is legacy |
| **OCCT support** | ✅ Provides CMake configs | Works with any system |
| **IDE support** | ✅ VS, CLion, Qt Creator, Xcode | Universal support |
| **Package managers** | ✅ vcpkg, Conan integration | Critical for dependencies |

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.21)
project(dc-3ddesignapp VERSION 0.1.0 LANGUAGES CXX)

# C++17 required for std::filesystem, std::optional, etc.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find dependencies
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
find_package(OpenCASCADE 7.8 REQUIRED)

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.dc3ddesignapp")
endif()

if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()
```

### CMake Presets

```json
// CMakePresets.json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "windows-release",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            }
        },
        {
            "name": "macos-release",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_OSX_ARCHITECTURES": "x86_64;arm64"
            }
        }
    ]
}
```

---

## 5. Dependency Management: vcpkg + Git Submodules

### Decision

Use **vcpkg** for most dependencies, with Git submodules for header-only libraries.

### Strategy

| Dependency | Source | Rationale |
|------------|--------|-----------|
| Qt 6 | System install or vcpkg | Large, complex build |
| Open CASCADE | vcpkg | Standard vcpkg port available |
| Eigen | vcpkg or header-only | Math library, header-only option |
| libigl | Git submodule | Header-only, frequently updated |
| stb_image | Vendored | Single header |
| Catch2 | vcpkg | Testing framework |
| spdlog | vcpkg | Logging |

### vcpkg Configuration

```json
// vcpkg.json
{
    "name": "dc-3ddesignapp",
    "version": "0.1.0",
    "dependencies": [
        {
            "name": "opencascade",
            "version>=": "7.8.0",
            "features": ["tbb"]
        },
        "eigen3",
        "spdlog",
        "catch2"
    ],
    "overrides": [
        {
            "name": "opencascade",
            "version": "7.8.0"
        }
    ]
}
```

### vcpkg Manifest Mode

```bash
# Install dependencies
vcpkg install --triplet x64-windows

# Or via CMake toolchain
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

---

## 6. Language: C++17

### Decision

Use **C++17** as the language standard.

### Rationale

| Feature | C++17 | Why Needed |
|---------|-------|------------|
| `std::filesystem` | ✅ | Cross-platform file operations |
| `std::optional` | ✅ | Safe nullable values |
| `std::variant` | ✅ | Type-safe unions |
| `std::string_view` | ✅ | Zero-copy string handling |
| `if constexpr` | ✅ | Compile-time branching |
| Structured bindings | ✅ | Cleaner tuple unpacking |
| `[[nodiscard]]` | ✅ | Prevent ignoring important return values |

### Why Not C++20?

- OCCT 7.8 is C++17 compatible
- Qt 6.5 LTS targets C++17
- C++20 module support still inconsistent across compilers
- Ranges library is nice but not critical

### Future: C++20 Migration

When OCCT and Qt fully support C++20 modules, consider migration for:
- Faster compile times (modules)
- `std::span` for array views
- Concepts for cleaner templates
- `std::format` for string formatting

---

## 7. Mesh Processing: Hybrid Approach

### Decision

Use a **hybrid approach** combining:
- Custom half-edge mesh structure (core data)
- libigl algorithms (decimation, smoothing)
- CGAL algorithms where needed (advanced operations)

### Custom Mesh Structure

We need a custom structure because:
1. **Memory efficiency:** Control exact memory layout
2. **GPU upload:** Optimize for rendering
3. **OCCT integration:** Easy conversion to/from OCCT meshes

### libigl Integration

libigl provides excellent algorithms we shouldn't reimplement:
- Quadric error decimation
- Laplacian smoothing
- ARAP deformation
- Boolean operations on meshes

```cpp
// Example: libigl decimation
#include <igl/decimate.h>

void decimateMesh(MeshData& mesh, float ratio) {
    Eigen::MatrixXd V = toEigenVertices(mesh);
    Eigen::MatrixXi F = toEigenFaces(mesh);
    
    Eigen::MatrixXd U;
    Eigen::MatrixXi G;
    Eigen::VectorXi J;
    
    igl::decimate(V, F, 
                  static_cast<size_t>(F.rows() * ratio),
                  U, G, J);
    
    mesh = fromEigen(U, G);
}
```

---

## 8. Testing: Catch2

### Decision

Use **Catch2** for unit and integration testing.

### Rationale

| Factor | Catch2 | Google Test |
|--------|--------|-------------|
| **Header-only option** | ✅ | Requires build |
| **BDD style** | ✅ GIVEN/WHEN/THEN | Less natural |
| **Matchers** | ✅ Rich set | Good |
| **Benchmarking** | ✅ Built-in | Separate library |
| **Learning curve** | ✅ Simple | More boilerplate |

### Test Example

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "geometry/mesh/MeshDecimator.h"

TEST_CASE("Mesh decimation reduces face count", "[mesh][decimation]") {
    MeshData mesh = createTestSphere(1000);  // 1000 faces
    
    SECTION("50% decimation") {
        auto result = MeshDecimator::decimate(mesh, 0.5);
        
        REQUIRE(result.ok());
        REQUIRE(result.value().faceCount() <= 500);
        // Should preserve topology
        REQUIRE(result.value().isManifold());
    }
    
    SECTION("Extreme decimation preserves basic shape") {
        auto result = MeshDecimator::decimate(mesh, 0.1);
        
        REQUIRE(result.ok());
        // Bounding box should be similar
        REQUIRE_THAT(result.value().bounds().diagonal(),
                     Catch::Matchers::WithinRel(mesh.bounds().diagonal(), 0.1));
    }
}
```

---

## 9. Logging: spdlog

### Decision

Use **spdlog** for application logging.

### Configuration

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void initLogging() {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/dc3ddesignapp.log", 1024 * 1024 * 5, 3);
    
    auto logger = std::make_shared<spdlog::logger>("app",
        spdlog::sinks_init_list{consoleSink, fileSink});
    
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug);
}

// Usage
spdlog::info("Loaded mesh: {} vertices, {} faces", 
             mesh.vertexCount(), mesh.faceCount());
spdlog::error("Failed to export STEP: {}", error.message());
```

---

## 10. Code Style

### Decision

Follow **Google C++ Style Guide** with modifications.

### Key Rules

```cpp
// Naming
class MeshData;              // PascalCase for types
void calculateNormals();     // camelCase for functions
int vertexCount;             // camelCase for variables
static const int kMaxLOD;    // k-prefix for constants
MESH_MAGIC_NUMBER;           // SCREAMING_CASE for macros

// Files
mesh_data.h                  // snake_case for files
mesh_data.cpp

// Braces
if (condition) {             // Opening brace on same line
    doSomething();
}

// Namespaces
namespace dc3d {
namespace geometry {
// No indentation inside namespaces
}  // namespace geometry
}  // namespace dc3d
```

### Clang-Format

```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
```

---

## Decision Log

| ID | Decision | Date | Status |
|----|----------|------|--------|
| D001 | Use Qt 6 for UI | 2026-02-12 | Approved |
| D002 | Use Open CASCADE for geometry kernel | 2026-02-12 | Approved |
| D003 | Use OpenGL 4.5 for rendering | 2026-02-12 | Approved |
| D004 | Use CMake for build system | 2026-02-12 | Approved |
| D005 | Use vcpkg for dependencies | 2026-02-12 | Approved |
| D006 | Use C++17 standard | 2026-02-12 | Approved |
| D007 | Use libigl for mesh algorithms | 2026-02-12 | Approved |
| D008 | Use Catch2 for testing | 2026-02-12 | Approved |
| D009 | Use spdlog for logging | 2026-02-12 | Approved |
| D010 | Follow Google C++ style | 2026-02-12 | Approved |

---

## References

- Qt 6 Documentation: https://doc.qt.io/qt-6/
- Open CASCADE Documentation: https://dev.opencascade.org/doc/overview/html/
- libigl Tutorial: https://libigl.github.io/tutorial/
- CMake Documentation: https://cmake.org/cmake/help/latest/
- vcpkg Documentation: https://vcpkg.io/en/docs/
- Google C++ Style Guide: https://google.github.io/styleguide/cppguide.html
