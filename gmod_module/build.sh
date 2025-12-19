#!/bin/bash
# Automated build script for GPU Particle System
# Usage: ./build.sh [clean|debug|release|install|help]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default build type
BUILD_TYPE="Release"
BUILD_DIR="build"
DO_INSTALL=false
DO_CLEAN=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        clean)
            DO_CLEAN=true
            ;;
        debug)
            BUILD_TYPE="Debug"
            ;;
        release)
            BUILD_TYPE="Release"
            ;;
        install)
            DO_INSTALL=true
            ;;
        help)
            echo "Usage: ./build.sh [options]"
            echo ""
            echo "Options:"
            echo "  clean     - Clean build directory before building"
            echo "  debug     - Build in Debug mode (default: Release)"
            echo "  release   - Build in Release mode"
            echo "  install   - Install to addon folder after building"
            echo "  help      - Show this help message"
            echo ""
            echo "Examples:"
            echo "  ./build.sh                    # Build in Release mode"
            echo "  ./build.sh clean release      # Clean and build Release"
            echo "  ./build.sh debug              # Build in Debug mode"
            echo "  ./build.sh release install    # Build and install to addon"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $arg${NC}"
            echo "Use './build.sh help' for usage"
            exit 1
            ;;
    esac
done

echo -e "${GREEN}=====================================================${NC}"
echo -e "${GREEN}  GPU Particle System - Build Script${NC}"
echo -e "${GREEN}=====================================================${NC}"
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}ERROR: Please run this script from the gmod_module directory${NC}"
    exit 1
fi

# Check for GMod SDK
if [ ! -d "lib/gmod-module-base" ]; then
    echo -e "${YELLOW}GMod SDK not found. Running setup script...${NC}"
    if [ -f "setup_sdk.sh" ]; then
        chmod +x setup_sdk.sh
        ./setup_sdk.sh
    else
        echo -e "${RED}ERROR: setup_sdk.sh not found!${NC}"
        exit 1
    fi
fi

# Clean if requested
if [ "$DO_CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "${GREEN}[✓] Clean complete${NC}"
fi

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Creating build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Configuring with CMake (${BUILD_TYPE})...${NC}"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
echo -e "${YELLOW}Building...${NC}"
if [ "$(uname)" = "Darwin" ]; then
    # macOS
    CORES=$(sysctl -n hw.ncpu)
else
    # Linux
    CORES=$(nproc)
fi

make -j$CORES

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✓] Build successful!${NC}"
    echo ""
    echo -e "${GREEN}Built modules:${NC}"
    ls -lh bin/
else
    echo -e "${RED}[✗] Build failed!${NC}"
    exit 1
fi

# Install if requested
if [ "$DO_INSTALL" = true ]; then
    echo ""
    echo -e "${YELLOW}Installing to addon...${NC}"
    cmake --install .
    echo -e "${GREEN}[✓] Installation complete${NC}"
fi

echo ""
echo -e "${GREEN}=====================================================${NC}"
echo -e "${GREEN}  Build Complete!${NC}"
echo -e "${GREEN}=====================================================${NC}"
echo ""
echo "Build type: $BUILD_TYPE"
echo "Output directory: $BUILD_DIR/bin/"
echo ""
echo "Next steps:"
if [ "$DO_INSTALL" = false ]; then
    echo "  - Run './build.sh install' to copy to addon"
fi
echo "  - Copy addon to garrysmod/addons/"
echo "  - Start GMod and test with 'lua_run_cl print(particles ~= nil)'"
echo ""
