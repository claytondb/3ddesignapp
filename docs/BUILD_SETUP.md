# Build Setup Guide — dc-3ddesignapp

**Version:** 1.0  
**Date:** 2026-02-12

---

## Overview

This guide covers setting up the development environment for dc-3ddesignapp on Windows and macOS.

**Estimated Setup Time:**
- Windows: 30-60 minutes
- macOS: 30-45 minutes

---

## Required Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| **CMake** | 3.21+ | Build system |
| **Qt** | 6.5+ LTS | UI framework |
| **Open CASCADE** | 7.8+ | Geometry kernel |
| **Ninja** | 1.10+ | Build tool (recommended) |
| **vcpkg** | Latest | Package manager |
| **Git** | 2.30+ | Version control |
| **Compiler** | MSVC 2022 / Clang 15+ | C++17 support |

### Optional Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| **Intel TBB** | 2021+ | Parallel algorithms |
| **Eigen** | 3.4+ | Linear algebra |
| **libigl** | Latest | Mesh processing |
| **Catch2** | 3.0+ | Testing |
| **spdlog** | 1.12+ | Logging |

---

## Windows Setup

### Prerequisites

1. **Visual Studio 2022** (Community edition is free)
   - Download: https://visualstudio.microsoft.com/
   - Required workloads:
     - "Desktop development with C++"
     - Individual components:
       - "C++ CMake tools for Windows"
       - "Windows 10/11 SDK"

2. **Git for Windows**
   - Download: https://git-scm.com/download/win

### Step 1: Install vcpkg

```powershell
# Open PowerShell as Administrator

# Clone vcpkg
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap
.\bootstrap-vcpkg.bat

# Integrate with Visual Studio
.\vcpkg integrate install

# Set environment variable (add to system PATH)
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "Machine")
```

### Step 2: Install Qt 6

**Option A: Qt Online Installer (Recommended)**

1. Download Qt Online Installer: https://www.qt.io/download-qt-installer
2. Sign in (free account required for open-source)
3. Select components:
   - Qt 6.5.x (LTS)
     - ✅ MSVC 2022 64-bit
     - ✅ Qt Shader Tools
     - ✅ Additional Libraries → Qt OpenGL Widgets
   - Developer and Designer Tools
     - ✅ CMake
     - ✅ Ninja
4. Install to: `C:\Qt`

5. Add to PATH:
```powershell
# Add Qt to PATH
[Environment]::SetEnvironmentVariable(
    "Path",
    "$env:Path;C:\Qt\6.5.3\msvc2022_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja",
    "Machine"
)
[Environment]::SetEnvironmentVariable("Qt6_DIR", "C:\Qt\6.5.3\msvc2022_64", "Machine")
```

**Option B: vcpkg (Longer build time)**

```powershell
cd C:\vcpkg
.\vcpkg install qt:x64-windows
# Warning: This can take 2+ hours
```

### Step 3: Install Open CASCADE via vcpkg

```powershell
cd C:\vcpkg

# Install OCCT with TBB support (recommended)
.\vcpkg install opencascade[tbb]:x64-windows

# This installs:
# - Open CASCADE 7.8.x
# - Intel TBB (for parallel algorithms)
# - FreeType (for text rendering)
# Takes approximately 15-30 minutes
```

### Step 4: Install Additional Dependencies

```powershell
cd C:\vcpkg

# Install remaining dependencies
.\vcpkg install eigen3:x64-windows
.\vcpkg install spdlog:x64-windows
.\vcpkg install catch2:x64-windows
```

### Step 5: Clone and Build Project

```powershell
# Clone repository
cd C:\Projects  # or your preferred location
git clone https://github.com/yourorg/dc-3ddesignapp.git
cd dc-3ddesignapp

# Initialize submodules (for libigl)
git submodule update --init --recursive

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Ninja" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2022_64

# Build
cmake --build . --config Release --parallel

# Run
.\dc-3ddesignapp.exe
```

### Step 6: IDE Setup (Visual Studio 2022)

1. Open Visual Studio 2022
2. File → Open → CMake...
3. Navigate to `dc-3ddesignapp/CMakeLists.txt`
4. Wait for CMake configuration
5. Set CMake Settings:
   - Configuration: x64-Release
   - CMake toolchain file: `C:\vcpkg\scripts\buildsystems\vcpkg.cmake`
   - CMake command arguments: `-DQt6_DIR=C:/Qt/6.5.3/msvc2022_64/lib/cmake/Qt6`

### Step 7: IDE Setup (VS Code)

1. Install extensions:
   - C/C++ (Microsoft)
   - CMake Tools (Microsoft)
   - CMake (twxs)

2. Create `.vscode/settings.json`:
```json
{
    "cmake.configureSettings": {
        "CMAKE_TOOLCHAIN_FILE": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake",
        "Qt6_DIR": "C:/Qt/6.5.3/msvc2022_64/lib/cmake/Qt6"
    },
    "cmake.generator": "Ninja",
    "cmake.buildDirectory": "${workspaceFolder}/build"
}
```

### Windows Troubleshooting

**Problem:** `Qt6_DIR not found`
```powershell
# Verify Qt installation
dir C:\Qt\6.5.3\msvc2022_64\lib\cmake\Qt6

# If missing, reinstall Qt with correct components
```

**Problem:** `OpenCASCADE_DIR not found`
```powershell
# Verify vcpkg installation
C:\vcpkg\vcpkg list | findstr opencascade

# If missing, install again
C:\vcpkg\vcpkg install opencascade:x64-windows
```

**Problem:** DLL not found at runtime
```powershell
# Add to PATH or copy DLLs to executable directory
copy C:\vcpkg\installed\x64-windows\bin\*.dll .\build\
copy C:\Qt\6.5.3\msvc2022_64\bin\*.dll .\build\
```

---

## macOS Setup

### Prerequisites

1. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **Homebrew** (package manager)
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

### Step 1: Install Development Tools

```bash
# Install build tools
brew install cmake ninja git

# Verify installations
cmake --version   # Should be 3.21+
ninja --version   # Should be 1.10+
```

### Step 2: Install vcpkg

```bash
# Clone vcpkg
cd ~
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap
./bootstrap-vcpkg.sh

# Add to shell profile (~/.zshrc or ~/.bash_profile)
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.zshrc
echo 'export PATH="$VCPKG_ROOT:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### Step 3: Install Qt 6

**Option A: Homebrew (Simplest)**

```bash
brew install qt@6

# Add Qt to PATH
echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.zshrc
echo 'export Qt6_DIR="/opt/homebrew/opt/qt@6/lib/cmake/Qt6"' >> ~/.zshrc
source ~/.zshrc

# Verify
qmake --version
```

**Option B: Qt Online Installer**

1. Download Qt Online Installer: https://www.qt.io/download-qt-installer
2. Select components:
   - Qt 6.5.x (LTS)
     - ✅ macOS
     - ✅ Qt Shader Tools
     - ✅ Qt OpenGL Widgets
3. Install to: `~/Qt`

```bash
# Add to PATH
echo 'export Qt6_DIR="$HOME/Qt/6.5.3/macos/lib/cmake/Qt6"' >> ~/.zshrc
echo 'export PATH="$HOME/Qt/6.5.3/macos/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### Step 4: Install Open CASCADE

**Option A: Homebrew (Quick)**

```bash
brew install opencascade

# Note: Homebrew OCCT may be older version
# Check version
brew info opencascade
```

**Option B: vcpkg (Recommended for consistency)**

```bash
cd ~/vcpkg

# Install OCCT (takes 15-30 minutes)
./vcpkg install opencascade:arm64-osx  # Apple Silicon
# OR
./vcpkg install opencascade:x64-osx    # Intel Mac
```

**Option C: Build from Source (Latest version)**

```bash
# Clone OCCT
cd ~
git clone https://github.com/Open-Cascade-SAS/OCCT.git
cd OCCT
git checkout V7_8_0  # or latest tag

# Install dependencies
brew install freetype tcl-tk

# Build
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local/opencascade \
    -DUSE_FREETYPE=ON \
    -DUSE_TBB=OFF

make -j$(sysctl -n hw.ncpu)
sudo make install

# Add to environment
echo 'export OpenCASCADE_DIR="/usr/local/opencascade/lib/cmake/opencascade"' >> ~/.zshrc
source ~/.zshrc
```

### Step 5: Install Additional Dependencies

```bash
cd ~/vcpkg

# For Apple Silicon (M1/M2/M3)
./vcpkg install eigen3:arm64-osx
./vcpkg install spdlog:arm64-osx
./vcpkg install catch2:arm64-osx

# For Intel Mac
./vcpkg install eigen3:x64-osx
./vcpkg install spdlog:x64-osx
./vcpkg install catch2:x64-osx
```

### Step 6: Clone and Build Project

```bash
# Clone repository
cd ~/Projects  # or your preferred location
git clone https://github.com/yourorg/dc-3ddesignapp.git
cd dc-3ddesignapp

# Initialize submodules
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure (Apple Silicon)
cmake .. -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=arm64-osx

# For Intel Mac, use: -DVCPKG_TARGET_TRIPLET=x64-osx

# If using Homebrew Qt:
cmake .. -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6;/opt/homebrew/opt/opencascade"

# Build
cmake --build . --parallel

# Run
open ./dc-3ddesignapp.app
# OR
./dc-3ddesignapp.app/Contents/MacOS/dc-3ddesignapp
```

### Step 7: IDE Setup (CLion)

1. Open CLion
2. File → Open → select `dc-3ddesignapp` folder
3. Preferences → Build, Execution, Deployment → CMake
4. Add CMake options:
   ```
   -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
   -DVCPKG_TARGET_TRIPLET=arm64-osx
   ```

### Step 8: IDE Setup (Xcode)

```bash
cd dc-3ddesignapp

# Generate Xcode project
cmake -B build-xcode -G "Xcode" \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake

# Open in Xcode
open build-xcode/dc-3ddesignapp.xcodeproj
```

### macOS Troubleshooting

**Problem:** OpenGL deprecation warnings
```cpp
// Suppress warnings in code:
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
```

**Problem:** Code signing errors
```bash
# For development, disable code signing
cmake .. -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO
```

**Problem:** Homebrew packages not found
```bash
# Make sure Homebrew paths are in CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH="/opt/homebrew:$CMAKE_PREFIX_PATH"
```

**Problem:** vcpkg wrong architecture
```bash
# Explicitly set triplet
./vcpkg install package:arm64-osx   # Apple Silicon
./vcpkg install package:x64-osx     # Intel
```

---

## CMake Configuration Reference

### Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.21)

project(dc-3ddesignapp
    VERSION 0.1.0
    DESCRIPTION "Scan-to-CAD application"
    LANGUAGES CXX
)

# ============================================================================
# Options
# ============================================================================
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(USE_TBB "Use Intel TBB for parallelism" ON)

# ============================================================================
# Global Settings
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# RPATH settings for finding shared libraries
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ============================================================================
# Platform-Specific Settings
# ============================================================================
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    # Universal binary (Intel + Apple Silicon)
    # set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures")
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif()

if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Find Dependencies
# ============================================================================

# Qt 6
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
)
qt_standard_project_setup()

# Open CASCADE
find_package(OpenCASCADE 7.8 REQUIRED COMPONENTS
    FoundationClasses
    ModelingData
    ModelingAlgorithms
    DataExchange
    Visualization
)

# OpenGL
find_package(OpenGL REQUIRED)

# Eigen (header-only)
find_package(Eigen3 3.4 REQUIRED)

# spdlog
find_package(spdlog REQUIRED)

# Optional: Intel TBB
if(USE_TBB)
    find_package(TBB)
    if(TBB_FOUND)
        add_compile_definitions(HAVE_TBB)
    endif()
endif()

# ============================================================================
# Source Files
# ============================================================================

# Collect sources (or specify explicitly for better control)
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)

file(GLOB_RECURSE HEADERS
    "include/*.h"
)

# Qt MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ============================================================================
# Main Executable
# ============================================================================

if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "DC 3D Design App")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.dc3ddesignapp")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${SOURCES}
        ${HEADERS}
        resources/resources.qrc
    )
elseif(WIN32)
    add_executable(${PROJECT_NAME} WIN32
        ${SOURCES}
        ${HEADERS}
        resources/resources.qrc
        resources/app.rc
    )
else()
    add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
        resources/resources.qrc
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(${PROJECT_NAME} PRIVATE
    # Qt
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    
    # OpenGL
    OpenGL::GL
    
    # Open CASCADE
    TKernel
    TKMath
    TKG2d
    TKG3d
    TKGeomBase
    TKGeomAlgo
    TKTopAlgo
    TKPrim
    TKBRep
    TKBO
    TKFillet
    TKOffset
    TKMesh
    TKSTEP
    TKSTEP209
    TKSTEPAttr
    TKSTEPBase
    TKIGES
    TKShHealing
    TKV3d
    TKService
    TKOpenGl
    
    # Other
    Eigen3::Eigen
    spdlog::spdlog
)

if(TBB_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb)
endif()

# ============================================================================
# Tests
# ============================================================================

if(BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 REQUIRED)
    
    add_executable(tests
        tests/unit/test_main.cpp
        tests/unit/mesh/test_MeshData.cpp
        # Add more test files...
    )
    
    target_link_libraries(tests PRIVATE
        Catch2::Catch2WithMain
        # Link project libraries...
    )
    
    include(CTest)
    include(Catch)
    catch_discover_tests(tests)
endif()

# ============================================================================
# Installation
# ============================================================================

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Windows: Deploy Qt DLLs
if(WIN32)
    # Qt deployment
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT}
                --no-translations
                --no-system-d3d-compiler
                $<TARGET_FILE:${PROJECT_NAME}>
        )
    endif()
endif()

# macOS: Deploy Qt frameworks
if(APPLE)
    find_program(MACDEPLOYQT macdeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${MACDEPLOYQT}
                $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>
                -always-overwrite
        )
    endif()
endif()
```

### CMakePresets.json

```json
{
    "version": 6,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 21,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "base",
            "hidden": true,
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/${presetName}",
            "cacheVariables": {
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
            }
        },
        {
            "name": "windows-debug",
            "inherits": "base",
            "displayName": "Windows Debug",
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Windows"
            },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            }
        },
        {
            "name": "windows-release",
            "inherits": "base",
            "displayName": "Windows Release",
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Windows"
            },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            }
        },
        {
            "name": "macos-debug",
            "inherits": "base",
            "displayName": "macOS Debug",
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Darwin"
            },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_TOOLCHAIN_FILE": "$env{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake"
            }
        },
        {
            "name": "macos-release",
            "inherits": "base",
            "displayName": "macOS Release",
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Darwin"
            },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_TOOLCHAIN_FILE": "$env{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "windows-debug",
            "configurePreset": "windows-debug"
        },
        {
            "name": "windows-release",
            "configurePreset": "windows-release"
        },
        {
            "name": "macos-debug",
            "configurePreset": "macos-debug"
        },
        {
            "name": "macos-release",
            "configurePreset": "macos-release"
        }
    ],
    "testPresets": [
        {
            "name": "test-all",
            "configurePreset": "windows-debug",
            "output": {
                "outputOnFailure": true
            }
        }
    ]
}
```

---

## Quick Reference

### Windows Commands

```powershell
# Configure
cmake --preset windows-release

# Build
cmake --build --preset windows-release --parallel

# Test
ctest --preset test-all

# Clean
cmake --build --preset windows-release --target clean
```

### macOS Commands

```bash
# Configure
cmake --preset macos-release

# Build
cmake --build --preset macos-release --parallel

# Test
ctest --preset test-all

# Clean
cmake --build --preset macos-release --target clean
```

---

## Version Matrix

| Component | Minimum | Recommended | Tested |
|-----------|---------|-------------|--------|
| CMake | 3.21 | 3.28+ | 3.28.1 |
| Qt | 6.5 | 6.5 LTS | 6.5.3 |
| OCCT | 7.7 | 7.8+ | 7.8.0 |
| MSVC | 2019 | 2022 | 17.8 |
| Clang | 14 | 16+ | 16.0 |
| Xcode | 14 | 15+ | 15.2 |

---

## Getting Help

1. **Build issues:** Check compiler output, verify dependencies with `cmake --version`, `qmake --version`
2. **Runtime issues:** Check logs in `logs/dc3ddesignapp.log`
3. **OCCT issues:** Consult https://dev.opencascade.org/forums
4. **Qt issues:** Consult https://forum.qt.io/

---

*Document maintained by: Technical Architecture Team*
