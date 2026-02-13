@echo off
REM dc-3ddesignapp build script for Windows
REM Usage: build.bat [Debug|Release]
REM 
REM Prerequisites:
REM   - Visual Studio 2019/2022 with C++ workload
REM   - Qt6 installed and Qt6_DIR set (or in PATH)
REM   - CMake 3.21+
REM
REM Example Qt6_DIR:
REM   set Qt6_DIR=C:\Qt\6.6.0\msvc2019_64\lib\cmake\Qt6

setlocal enabledelayedexpansion

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set BUILD_DIR=build

echo ==============================================
echo dc-3ddesignapp Build Script
echo Build type: %BUILD_TYPE%
echo ==============================================

REM Check for CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found. Please install CMake 3.21+ and add to PATH.
    exit /b 1
)

REM Check for Qt6
if not defined Qt6_DIR (
    echo WARNING: Qt6_DIR not set. CMake may not find Qt6.
    echo   Set Qt6_DIR to your Qt6 cmake directory, e.g.:
    echo   set Qt6_DIR=C:\Qt\6.6.0\msvc2019_64\lib\cmake\Qt6
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure
echo.
echo ^>^>^> Configuring...
cmake .. ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_TESTS=ON ^
    %Qt6_DIR_ARG%

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    exit /b 1
)

REM Build
echo.
echo ^>^>^> Building...
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo ==============================================
echo Build complete!
echo Executable: %BUILD_DIR%\%BUILD_TYPE%\dc-3ddesignapp.exe
echo ==============================================

REM Run tests if available
if exist "dc3d_tests.exe" (
    echo.
    echo ^>^>^> Running tests...
    ctest -C %BUILD_TYPE% --output-on-failure
)

cd ..
