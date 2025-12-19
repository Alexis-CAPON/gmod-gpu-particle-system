@echo off
setlocal enabledelayedexpansion

echo =====================================================
echo  GPU Particle System - GMod Installation Script
echo =====================================================
echo.

set "GMOD_PATH=C:\Program Files (x86)\Steam\steamapps\common\GarrysMod"
set "MODULE_PATH=%~dp0build\bin\Release"
set "LUA_BIN=%GMOD_PATH%\garrysmod\lua\bin"
set "AUTORUN_PATH=%GMOD_PATH%\garrysmod\lua\autorun\client"

echo Installation paths:
echo   GMod: %GMOD_PATH%
echo   Module: %MODULE_PATH%
echo.

REM Check if GMod path exists
if not exist "%GMOD_PATH%\garrysmod" (
    echo [ERROR] GMod directory not found at: %GMOD_PATH%
    echo Please edit this script and set the correct GMOD_PATH
    pause
    exit /b 1
)

REM Check if module files exist
if not exist "%MODULE_PATH%\gmcl_particles_win64.dll" (
    echo [ERROR] Client module not found: %MODULE_PATH%\gmcl_particles_win64.dll
    echo Please build the module first using build.bat
    pause
    exit /b 1
)

if not exist "%MODULE_PATH%\gmsv_particles_win64.dll" (
    echo [ERROR] Server module not found: %MODULE_PATH%\gmsv_particles_win64.dll
    echo Please build the module first using build.bat
    pause
    exit /b 1
)

REM Create lua/bin directory if it doesn't exist
if not exist "%LUA_BIN%\" (
    echo Creating lua/bin directory...
    mkdir "%LUA_BIN%" 2>nul
)

REM Create autorun/client directory if it doesn't exist
if not exist "%AUTORUN_PATH%\" (
    echo Creating autorun/client directory...
    if not exist "%GMOD_PATH%\garrysmod\lua\autorun\" mkdir "%GMOD_PATH%\garrysmod\lua\autorun" 2>nul
    mkdir "%AUTORUN_PATH%" 2>nul
)

echo.
echo Installing modules...
echo.

REM Copy client module
echo [1/3] Copying client module...
copy /Y "%MODULE_PATH%\gmcl_particles_win64.dll" "%LUA_BIN%\gmcl_particles_win64.dll"
if !errorlevel! neq 0 (
    echo [ERROR] Failed to copy client module
    pause
    exit /b 1
)
echo   Success: gmcl_particles_win64.dll

REM Copy server module
echo [2/3] Copying server module...
copy /Y "%MODULE_PATH%\gmsv_particles_win64.dll" "%LUA_BIN%\gmsv_particles_win64.dll"
if !errorlevel! neq 0 (
    echo [ERROR] Failed to copy server module
    pause
    exit /b 1
)
echo   Success: gmsv_particles_win64.dll

REM Create Lua autorun file
echo [3/3] Creating Lua autorun file...
(
echo -- GPU Particle System - Client Autorun
echo -- This file loads the GPU particle system module on client startup
echo.
echo if CLIENT then
echo     print^("=====================================================")
echo     print^("  Loading GPU Particle System..."^)
echo     print^("=====================================================")
echo
echo     local success = pcall^(function^(^)
echo         require^("particles"^)
echo     end^)
echo
echo     if success then
echo         print^("[GPU Particles] Module loaded successfully!"^)
echo     else
echo         print^("[GPU Particles] ERROR: Failed to load module!"^)
echo         print^("[GPU Particles] Make sure gmcl_particles_win64.dll is in garrysmod/lua/bin/"^)
echo     end
echo end
) > "%AUTORUN_PATH%\gpu_particles_loader.lua"

if !errorlevel! neq 0 (
    echo [ERROR] Failed to create Lua autorun file
    pause
    exit /b 1
)
echo   Success: gpu_particles_loader.lua

echo.
echo =====================================================
echo  Installation Complete!
echo =====================================================
echo.
echo Files installed:
echo   - %LUA_BIN%\gmcl_particles_win64.dll
echo   - %LUA_BIN%\gmsv_particles_win64.dll
echo   - %AUTORUN_PATH%\gpu_particles_loader.lua
echo.
echo Next steps:
echo   1. Start Garry's Mod
echo   2. Look for the GPU Particle System messages in console
echo   3. You should see:
echo      =====================================================
echo        GPU Particle System for Garry's Mod
echo        Version 1.0.0
echo      =====================================================
echo      [GPU Particles] Module loaded successfully!
echo.
pause
