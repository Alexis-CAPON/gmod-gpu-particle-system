#!/bin/bash
# Setup script for GMod SDK
# This script clones the gmod-module-base library needed for building

set -e

echo "======================================================"
echo "  GMod GPU Particle System - SDK Setup"
echo "======================================================"
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Please run this script from the gmod_module directory"
    exit 1
fi

# Create lib directory if it doesn't exist
mkdir -p lib

# Check if gmod-module-base already exists
if [ -d "lib/gmod-module-base" ]; then
    echo "[✓] gmod-module-base already exists"
    echo "    Updating to latest version..."
    cd lib/gmod-module-base
    git pull
    cd ../..
else
    echo "[...] Cloning gmod-module-base from GitHub..."
    git clone https://github.com/Facepunch/gmod-module-base.git lib/gmod-module-base
    echo "[✓] gmod-module-base cloned successfully"
fi

# Verify the headers exist
if [ -f "lib/gmod-module-base/include/GarrysMod/Lua/Interface.h" ]; then
    echo "[✓] GMod SDK headers found"
else
    echo "[✗] ERROR: GMod SDK headers not found!"
    echo "    Expected: lib/gmod-module-base/include/GarrysMod/Lua/Interface.h"
    exit 1
fi

echo ""
echo "======================================================"
echo "  Setup Complete!"
echo "======================================================"
echo ""
echo "Next steps:"
echo "  1. mkdir build && cd build"
echo "  2. cmake .."
echo "  3. cmake --build . --config Release"
echo ""
echo "See SETUP_GMOD_SDK.md for more information"
echo ""
