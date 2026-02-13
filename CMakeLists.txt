cmake_minimum_required(VERSION 3.21)

project(dc-3ddesignapp
    VERSION 0.1.0
    DESCRIPTION "Scan-to-CAD application"
    LANGUAGES CXX
)

# ============================================================================
# Options
# ============================================================================
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(USE_TBB "Use Intel TBB for parallelism" OFF)

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
    add_compile_definitions(GL_SILENCE_DEPRECATION)
    
    # Homebrew on Apple Silicon (M1/M2)
    if(EXISTS "/opt/homebrew")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
        include_directories("/opt/homebrew/include")
        include_directories("/opt/homebrew/include/eigen3")
        link_directories("/opt/homebrew/lib")
        message(STATUS "Added Homebrew paths for Apple Silicon: /opt/homebrew")
    endif()
    
    # Homebrew on Intel Mac
    if(EXISTS "/usr/local/Cellar")
        list(APPEND CMAKE_PREFIX_PATH "/usr/local")
        include_directories("/usr/local/include")
        include_directories("/usr/local/include/eigen3")
        link_directories("/usr/local/lib")
        message(STATUS "Added Homebrew paths for Intel Mac: /usr/local")
    endif()
endif()

if(MSVC)
    add_compile_options(/W4 /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# GLM requires this for gtx extensions
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

# ============================================================================
# Find Dependencies
# ============================================================================

# Qt 6
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
)
qt_standard_project_setup()

# OpenGL
find_package(OpenGL REQUIRED)

# Open CASCADE (optional for initial build)
find_package(OpenCASCADE QUIET COMPONENTS
    FoundationClasses
    ModelingData
    ModelingAlgorithms
)

if(OpenCASCADE_FOUND)
    message(STATUS "Open CASCADE found: ${OpenCASCADE_VERSION}")
    add_compile_definitions(HAVE_OCCT)
else()
    message(STATUS "Open CASCADE not found - CAD features disabled")
endif()

# Optional: Intel TBB
if(USE_TBB)
    find_package(TBB QUIET)
    if(TBB_FOUND)
        add_compile_definitions(HAVE_TBB)
        message(STATUS "Intel TBB found")
    endif()
endif()

# Optional: Eigen3 for advanced algorithms
find_package(Eigen3 QUIET)
if(Eigen3_FOUND OR EIGEN3_FOUND)
    add_compile_definitions(HAVE_EIGEN)
    include_directories(${EIGEN3_INCLUDE_DIR})
    message(STATUS "Eigen3 found - advanced algorithms enabled")
else()
    message(STATUS "Eigen3 not found - using fallback algorithms")
endif()

# ============================================================================
# Qt Auto-processing
# ============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ============================================================================
# GLM (header-only math library)
# ============================================================================
find_package(glm QUIET)
if(NOT glm_FOUND)
    # Try to find GLM in common locations
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATHS
            ${GLM_DIR}
            ${GLMINCLUDE_DIRS}
            $ENV{GLM_DIR}
            /usr/include
            /usr/local/include
            /opt/homebrew/include
            /opt/homebrew/opt/glm/include
            C:/glm
    )
    if(GLM_INCLUDE_DIR)
        message(STATUS "GLM found at: ${GLM_INCLUDE_DIR}")
        include_directories(${GLM_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "GLM not found. Set GLM_DIR or install GLM.")
    endif()
else()
    message(STATUS "GLM found via package")
    # Ensure glm include dir is available
    if(TARGET glm::glm)
        get_target_property(GLM_INCLUDE_DIR glm::glm INTERFACE_INCLUDE_DIRECTORIES)
        if(GLM_INCLUDE_DIR)
            include_directories(${GLM_INCLUDE_DIR})
        endif()
    endif()
endif()

# ============================================================================
# Include directories
# ============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# Add subdirectories
# ============================================================================
add_subdirectory(src/core)
add_subdirectory(src/geometry)
add_subdirectory(src/sketch)
add_subdirectory(src/io)
add_subdirectory(src/renderer)
add_subdirectory(src/ui)
add_subdirectory(src/app)

# ============================================================================
# Tests
# ============================================================================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# Installation
# ============================================================================
include(GNUInstallDirs)

# Installation is handled in src/app/CMakeLists.txt where the target is defined

# Qt deployment is handled in src/app/CMakeLists.txt where the target is defined
