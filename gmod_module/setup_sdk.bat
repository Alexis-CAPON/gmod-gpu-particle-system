@echo off
REM Setup script for GMod SDK (Windows)
REM This script clones the gmod-module-base library needed for building

echo ======================================================
echo   GMod GPU Particle System - SDK Setup
echo ======================================================
echo.

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo ERROR: Please run this script from the gmod_module directory
    exit /b 1
)

REM Create lib directory if it doesn't exist
if not exist "lib" mkdir lib

REM Check if gmod-module-base already exists
if exist "lib\gmod-module-base" (
    echo [OK] gmod-module-base already exists
    echo     Updating to latest version...
    cd lib\gmod-module-base
    git pull
    cd ..\..
) else (
    echo [...] Cloning gmod-module-base from GitHub...
    git clone https://github.com/Facepunch/gmod-module-base.git lib\gmod-module-base
    echo [OK] gmod-module-base cloned successfully
)

REM Verify the headers exist
if exist "lib\gmod-module-base\include\GarrysMod\Lua\Interface.h" (
    echo [OK] GMod SDK headers found
) else (
    echo [ERROR] GMod SDK headers not found!
    echo     Expected: lib\gmod-module-base\include\GarrysMod\Lua\Interface.h
    exit /b 1
)

echo.
echo ======================================================
echo   Setup Complete!
echo ======================================================
echo.
echo Next steps:
echo   1. mkdir build ^&^& cd build
echo   2. cmake .. -G "Visual Studio 16 2019" -A x64
echo   3. cmake --build . --config Release
echo.
echo See SETUP_GMOD_SDK.md for more information
echo.

pause
