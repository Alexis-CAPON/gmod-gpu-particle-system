@echo off
REM Launch Garry's Mod for Particle System Testing
REM This script launches GMod with the console enabled

echo ========================================
echo   GPU Particle System - Launch GMod
echo ========================================
echo.

set GMOD_PATH=C:\Program Files (x86)\Steam\steamapps\common\GarrysMod

if not exist "%GMOD_PATH%\hl2.exe" (
    echo ERROR: GMod not found at expected location
    echo Expected: %GMOD_PATH%
    echo.
    echo Please update the GMOD_PATH variable in this script
    pause
    exit /b 1
)

echo GMod found at: %GMOD_PATH%
echo.
echo Launching GMod with console enabled...
echo.
echo Quick Test Commands:
echo   1. Wait for game to load completely
echo   2. Open console (~ key)
echo   3. Type: lua_openscript_cl data/particle_test.lua
echo   4. Press Enter
echo.
echo Starting in 3 seconds...
timeout /t 3 /nobreak > nul

REM Launch GMod through Steam with console enabled
start steam://rungameid/4000//-console +map gm_flatgrass

echo.
echo GMod is launching...
echo.
echo REMEMBER:
echo   - Load into a map (Singleplayer -^> New Game -^> gm_flatgrass)
echo   - Open console (~)
echo   - Run: lua_openscript_cl data/particle_test.lua
echo.
pause
