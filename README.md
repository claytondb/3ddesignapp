# dc-3ddesignapp

<div align="center">

**Professional Scan-to-CAD Software for Reverse Engineering**

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](CHANGELOG.md)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)](#supported-platforms)
[![License](https://img.shields.io/badge/license-Proprietary-red.svg)](#license)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-6.5+-41CD52.svg)](https://www.qt.io/)

*Convert 3D scan meshes into editable CAD models with precision and ease.*

</div>

---

## 📖 Overview

**dc-3ddesignapp** is a cross-platform scan-to-CAD application inspired by Revo Design / QuickSurface. It enables engineers, designers, and makers to convert 3D scanned meshes (STL, OBJ, PLY) into professional CAD models (STEP, IGES) ready for manufacturing.

### Why dc-3ddesignapp?

- **Complete Workflow** — From raw scan to production-ready CAD
- **Cross-Platform** — Native performance on Windows, macOS, and Linux
- **Professional Tools** — Industrial-grade mesh processing and surface fitting
- **Modern UI** — Intuitive dark theme interface designed for productivity
- **Open Standards** — STEP/IGES export compatible with all major CAD software

---

## ✨ Features

### 📦 Import & Export
- **Mesh Formats:** STL (ASCII/binary), OBJ, PLY
- **CAD Formats:** STEP (AP214/AP242), IGES
- **Native Format:** .dc3d project files with compression

### 🔷 Mesh Processing
- Polygon reduction (Quadric Error Metrics)
- Mesh smoothing (Laplacian, Taubin, bilateral)
- Hole detection and intelligent filling
- Outlier removal and mesh repair
- Clipping box for region isolation

### 📐 Alignment & Registration
- Align to World Coordinate System (WCS)
- N-Point registration
- ICP (Iterative Closest Point) fine alignment
- Interactive 3D transform manipulator

### 🎯 Primitive Fitting
- Automatic primitive extraction from mesh
- Plane, cylinder, cone, sphere fitting
- RANSAC-based robust algorithms

### ✏️ 2D Sketching
- Section plane creation
- Full sketch tools: line, arc, circle, spline
- Geometric & dimensional constraints
- Real-time constraint solver
- Extrude and revolve operations

### 🧊 Solid Modeling
- B-Rep solid representation
- CSG boolean operations (union, subtract, intersect)
- Fillets (constant/variable radius)
- Chamfers (symmetric/asymmetric/angle-based)
- Primitive solids (box, cylinder, sphere, cone, torus)

### 🌊 Surface Fitting
- NURBS surface representation
- Automatic surface generation (AutoSurface)
- Quad-dominant remeshing
- Shrink-wrap surfaces

### 🔄 Core Features
- Undo/redo with 100-command history
- Scene manager with object hierarchy
- Selection tools (single, multi, box)
- Real-time properties panel
- Operation preview in dialogs

---

## 📸 Screenshots

<!-- Screenshots will be added before public release -->

| Main Interface | Mesh Processing |
|:--------------:|:---------------:|
| ![Main](docs/screenshots/main.png) | ![Mesh](docs/screenshots/mesh.png) |

| Surface Fitting | CAD Export |
|:---------------:|:----------:|
| ![Surface](docs/screenshots/surface.png) | ![Export](docs/screenshots/export.png) |

---

## 🚀 Getting Started

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| **OS** | Windows 10, macOS 12, Ubuntu 22.04 | Latest version |
| **CPU** | Intel i5 / Apple M1 | Intel i7+ / Apple M2+ |
| **RAM** | 8 GB | 16+ GB |
| **GPU** | OpenGL 4.5 capable | Dedicated GPU |
| **Storage** | 500 MB | 1+ GB |

### Supported Platforms

| Platform | Architecture | Status |
|----------|--------------|--------|
| Windows 10/11 | x64 | ✅ Supported |
| macOS 12+ | x64, ARM64 | ✅ Supported |
| Linux (Ubuntu 22.04+) | x64 | ✅ Supported |

---

## 🔨 Building from Source

### Prerequisites

- **CMake** 3.21+
- **Qt** 6.5+ LTS
- **Open CASCADE** 7.8+
- **C++ Compiler** with C++17 support (MSVC 2022, Clang 15+, GCC 11+)
- **Ninja** (recommended) or Make

### Quick Build

```bash
# Clone the repository
git clone https://github.com/claytondb/dc-3ddesignapp.git
cd dc-3ddesignapp

# Configure with CMake
cmake --preset release

# Build
cmake --build build/release --parallel

# Run
./build/release/bin/dc-3ddesignapp
```

### Platform-Specific Instructions

<details>
<summary><b>Windows (MSVC 2022)</b></summary>

```powershell
# Install dependencies via vcpkg
vcpkg install qt6 opencascade eigen3

# Configure and build
cmake --preset windows-release
cmake --build build/release --config Release

# Run
.\build\release\bin\Release\dc-3ddesignapp.exe
```

</details>

<details>
<summary><b>macOS (Clang)</b></summary>

```bash
# Install dependencies via Homebrew
brew install qt@6 opencascade eigen ninja

# Configure and build
cmake --preset macos-release
cmake --build build/release --parallel

# Run
./build/release/bin/dc-3ddesignapp.app/Contents/MacOS/dc-3ddesignapp
```

</details>

<details>
<summary><b>Linux (GCC)</b></summary>

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install qt6-base-dev libocct-dev libeigen3-dev ninja-build

# Configure and build
cmake --preset linux-release
cmake --build build/release --parallel

# Run
./build/release/bin/dc-3ddesignapp
```

</details>

For detailed build instructions, see [docs/BUILD_SETUP.md](docs/BUILD_SETUP.md).

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [PROJECT_BRIEF.md](PROJECT_BRIEF.md) | Project overview and feature analysis |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Technical architecture guide |
| [docs/BUILD_SETUP.md](docs/BUILD_SETUP.md) | Complete build instructions |
| [docs/UI_DESIGN.md](docs/UI_DESIGN.md) | User interface design guide |
| [docs/USER_FLOWS.md](docs/USER_FLOWS.md) | User workflow documentation |
| [docs/STYLE_GUIDE.md](docs/STYLE_GUIDE.md) | Code style guidelines |
| [CHANGELOG.md](CHANGELOG.md) | Version history |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Contribution guidelines |

---

## 🛠️ Tech Stack

| Layer | Technology |
|-------|------------|
| **Language** | C++ (C++17) |
| **UI Framework** | Qt 6.5+ |
| **Rendering** | OpenGL 4.5 |
| **Geometry Kernel** | Open CASCADE (OCCT) 7.8+ |
| **Mesh Processing** | Custom + libigl algorithms |
| **Build System** | CMake 3.21+ |
| **Testing** | Catch2 |

---

## 📊 Project Statistics

| Metric | Value |
|--------|-------|
| **Version** | 1.0.0 |
| **Source Files** | 203 |
| **Lines of Code** | 75,333 |
| **Modules** | 7 (app, core, geometry, io, renderer, sketch, ui) |

---

## 🤝 Contributing

We welcome contributions! Please read our [Contributing Guide](CONTRIBUTING.md) for details on:

- Code style and standards
- Development workflow
- Submitting pull requests
- Reporting issues

---

## 📄 License

**Proprietary** — All rights reserved.

This software is proprietary and confidential. Unauthorized copying, distribution, or use of this software is strictly prohibited.

For licensing inquiries, please contact the project maintainers.

---

## 🙏 Acknowledgments

- [Qt](https://www.qt.io/) — Cross-platform UI framework
- [Open CASCADE](https://dev.opencascade.org/) — Open-source geometry kernel
- [libigl](https://libigl.github.io/) — Geometry processing library
- [Eigen](https://eigen.tuxfamily.org/) — Linear algebra library

---

## 📬 Contact

- **Author:** Clayton
- **GitHub:** [@claytondb](https://github.com/claytondb)

---

<div align="center">

**Built with ❤️ for the reverse engineering community**

</div>
