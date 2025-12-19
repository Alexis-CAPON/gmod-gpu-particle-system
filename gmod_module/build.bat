@echo off
REM Automated build script for GPU Particle System (Windows)
REM Usage: build.bat [clean|debug|release|install|help]

setlocal enabledelayedexpansion

REM Default settings
set BUILD_TYPE=Release
set BUILD_DIR=build
set DO_INSTALL=0
set DO_CLEAN=0
set VS_GENERATOR=Visual Studio 17 2022

REM Parse arguments
:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="clean" (
    set DO_CLEAN=1
    shift
    goto parse_args
)
if /i "%~1"=="debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%~1"=="release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if /i "%~1"=="install" (
    set DO_INSTALL=1
    shift
    goto parse_args
)
if /i "%~1"=="help" (
    echo Usage: build.bat [options]
    echo.
    echo Options:
    echo   clean     - Clean build directory before building
    echo   debug     - Build in Debug mode (default: Release)
    echo   release   - Build in Release mode
    echo   install   - Install to addon folder after building
    echo   help      - Show this help message
    echo.
    echo Examples:
    echo   build.bat                    # Build in Release mode
    echo   build.bat clean release      # Clean and build Release
    echo   build.bat debug              # Build in Debug mode
    echo   build.bat release install    # Build and install to addon
    exit /b 0
)
echo Unknown option: %~1
echo Use 'build.bat help' for usage
exit /b 1

:end_parse

echo =====================================================
echo   GPU Particle System - Build Script
echo =====================================================
echo.

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo ERROR: Please run this script from the gmod_module directory
    exit /b 1
)

REM Check for GMod SDK
if not exist "lib\gmod-module-base" (
    echo GMod SDK not found. Running setup script...
    if exist "setup_sdk.bat" (
        call setup_sdk.bat
    ) else (
        echo ERROR: setup_sdk.bat not found!
        exit /b 1
    )
)

REM Clean if requested
if %DO_CLEAN%==1 (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" (
        rmdir /s /q "%BUILD_DIR%"
    )
    echo [OK] Clean complete
)

REM Create build directory
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%"

REM Configure
echo Configuring with CMake (%BUILD_TYPE%)...
echo Auto-detecting Visual Studio version...
echo Building for 32-bit (Win32) to match Garry's Mod...
cmake .. -A Win32

if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    cd ..
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)

echo [OK] Build successful!
echo.
echo Built modules:
dir /b bin\

REM Install if requested
if %DO_INSTALL%==1 (
    echo.
    echo Installing to addon...
    cmake --install .
    echo [OK] Installation complete
)

cd ..

echo.
echo =====================================================
echo   Build Complete!
echo =====================================================
echo.
echo Build type: %BUILD_TYPE%
echo Output directory: %BUILD_DIR%\bin\
echo.
echo Next steps:
if %DO_INSTALL%==0 (
    echo   - Run 'build.bat install' to copy to addon
)
echo   - Copy addon to garrysmod\addons\
echo   - Start GMod and test with 'lua_run_cl print(particles ~= nil)'
echo.

pause
