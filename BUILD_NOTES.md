# Build Notes - dc-3ddesignapp

## Quick Start

### Linux/macOS
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install cmake build-essential qt6-base-dev qt6-opengl-dev libglm-dev

# Or with Homebrew (macOS)
brew install cmake qt@6 glm

# Build
./build.sh Release
```

### Windows
```batch
REM Install Qt6 from https://www.qt.io/download
REM Set Qt6_DIR environment variable
set Qt6_DIR=C:\Qt\6.6.0\msvc2019_64\lib\cmake\Qt6

REM Build
build.bat Release
```

## Dependencies

| Dependency | Version | Required | Notes |
|------------|---------|----------|-------|
| CMake | ≥3.21 | Yes | Build system |
| Qt6 | ≥6.5 | Yes | Core, Gui, Widgets, OpenGL, OpenGLWidgets |
| OpenGL | 4.1+ | Yes | Graphics rendering |
| GLM | ≥0.9.9 | Yes | Math library (header-only) |
| Open CASCADE | ≥7.6 | Optional | STEP/IGES import/export |
| Intel TBB | ≥2021 | Optional | Parallel processing |
| Catch2 | ≥3.0 | Optional | Unit tests |

## Build Issues & Fixes Applied

### 1. Missing Source Files in CMakeLists.txt

The following files were missing from CMakeLists.txt and have been added:

**src/core/CMakeLists.txt:**
- Commands/DeleteCommand.cpp/h
- Commands/ImportMeshCommand.cpp/h  
- Commands/MeshEditCommand.cpp/h
- Commands/TransformCommand.cpp/h

**src/geometry/CMakeLists.txt:**
- Alignment.cpp/h, ICP.cpp/h
- DeviationAnalysis.cpp/h
- MeshAnalysis.cpp/h, MeshRepair.cpp/h, MeshSmoothing.cpp/h
- MeshDecimation.cpp/h, MeshSubdivision.cpp/h
- primitives/* (Plane, Sphere, Cylinder, Cone, PrimitiveFitter, SymmetryPlane)
- freeform/* (AutoSurface, QuadMesh, SurfaceFit, WrapSurface)
- solid/* (Solid, BooleanOps, Fillet, Chamfer)

**src/io/CMakeLists.txt:**
- STEPImporter.cpp/h, STEPExporter.cpp/h
- IGESImporter.cpp/h, IGESExporter.cpp/h
- ExportOptions.h, NativeFormat.cpp/h

**src/renderer/CMakeLists.txt:**
- DeviationRenderer.cpp/h
- PrimitiveRenderer.cpp/h
- TransformGizmo.cpp/h

**src/ui/CMakeLists.txt:**
- panels/AnalysisPanel.cpp/h
- tools/FreeformTool.cpp/h
- dialogs/ExportDialog.cpp/h
- dialogs/AutoSurfaceDialog.cpp/h
- dialogs/FitSurfaceDialog.cpp/h

### 2. New Sketch Module

Created `src/sketch/CMakeLists.txt` for the sketch module which was not included in the build:
- Sketch.cpp/h
- SketchSolver.cpp/h
- SectionCreator.cpp/h
- entities/* (SketchEntity, SketchPoint, SketchLine, SketchArc, SketchCircle, SketchSpline)

### 3. Missing Shader Resources

Added deviation shaders to `resources/resources.qrc`:
- shaders/deviation.vert
- shaders/deviation.frag

### 4. GLM Configuration

Added GLM detection to geometry and sketch CMakeLists.txt files. If GLM is not found via CMake, it falls back to checking system include paths.

## Known Issues

### Namespace Inconsistency

The project uses two namespaces:

| Namespace | Used by |
|-----------|---------|
| `dc3d` | core/*, geometry/*, io/*, app/* |
| `dc` | renderer/*, sketch/* |

This is intentional based on code analysis but should be documented. Key examples:
- `dc3d::core::SceneManager` 
- `dc3d::geometry::MeshData`
- `dc::Viewport`
- `dc::Camera`
- `dc::sketch::Sketch`

The `IntegrationController.h` bridges both namespaces properly.

### Platform-Specific Notes

**Windows:**
- Use Visual Studio 2019 or 2022 with C++ workload
- Qt6 must be compiled with MSVC (not MinGW)
- windeployqt runs automatically post-build

**macOS:**
- Minimum macOS 11.0 (Big Sur)
- GL_SILENCE_DEPRECATION is defined (OpenGL deprecated on macOS)
- macdeployqt runs automatically post-build

**Linux:**
- Requires OpenGL development headers (libgl1-mesa-dev)
- May need to set LD_LIBRARY_PATH for Qt6

### Open CASCADE (Optional)

If Open CASCADE is not found, STEP/IGES import/export is disabled:
- `HAVE_OCCT` preprocessor macro is NOT defined
- STEPImporter, STEPExporter, IGESImporter, IGESExporter still compile but may have stub implementations

### Intel TBB (Optional)

If TBB is not found:
- `HAVE_TBB` preprocessor macro is NOT defined
- Algorithms fall back to single-threaded implementations

## Testing

```bash
cd build
ctest --output-on-failure
```

Tests require Catch2 v3. If not found, tests are skipped.

## IDE Setup

### Visual Studio Code
```json
// .vscode/settings.json
{
    "cmake.configureSettings": {
        "Qt6_DIR": "/path/to/Qt/6.6.0/gcc_64/lib/cmake/Qt6"
    }
}
```

### CLion
CMake should auto-detect Qt6 if it's in PATH. Otherwise, add to CMake options:
```
-DQt6_DIR=/path/to/Qt/6.6.0/lib/cmake/Qt6
```

### Visual Studio
Use "Open Folder" and let CMake integration handle configuration.

## Troubleshooting

### "Qt6 not found"
Set Qt6_DIR to point to your Qt installation's cmake directory:
```bash
export Qt6_DIR=/path/to/Qt/6.6.0/gcc_64/lib/cmake/Qt6
```

### "GLM not found"
Install GLM or add to cmake:
```bash
cmake .. -DGLM_INCLUDE_DIR=/path/to/glm
```

### "OpenGL not found"
Install development packages:
```bash
# Ubuntu/Debian
sudo apt install libgl1-mesa-dev

# Fedora
sudo dnf install mesa-libGL-devel
```

### Link errors about undefined references
Check that all source files exist. Some files may be stubs that need implementation.
