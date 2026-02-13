#!/bin/bash
# dc-3ddesignapp build script for Linux/macOS
# Usage: ./build.sh [Debug|Release]

set -e

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "=============================================="
echo "dc-3ddesignapp Build Script"
echo "Build type: ${BUILD_TYPE}"
echo "Jobs: ${JOBS}"
echo "=============================================="

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "ERROR: $1 not found. Please install it."
        exit 1
    fi
}

check_tool cmake
check_tool make

# Check for Qt6
if ! pkg-config --exists Qt6Core 2>/dev/null; then
    if [ -z "$Qt6_DIR" ]; then
        echo "WARNING: Qt6 not found via pkg-config and Qt6_DIR not set"
        echo "  Set Qt6_DIR to your Qt6 cmake directory, e.g.:"
        echo "  export Qt6_DIR=/path/to/Qt/6.x.x/gcc_64/lib/cmake/Qt6"
    fi
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure
echo ""
echo ">>> Configuring..."
cmake .. \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DBUILD_TESTS=ON \
    ${Qt6_DIR:+-DQt6_DIR="${Qt6_DIR}"}

# Build
echo ""
echo ">>> Building..."
cmake --build . --parallel "${JOBS}"

echo ""
echo "=============================================="
echo "Build complete!"
echo "Executable: ${BUILD_DIR}/dc-3ddesignapp"
echo "=============================================="

# Run tests if available
if [ -f "dc3d_tests" ]; then
    echo ""
    echo ">>> Running tests..."
    ctest --output-on-failure || true
fi
