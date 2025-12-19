# Building the GPU Particle System

Complete guide for building the GPU Particle System binary modules for Garry's Mod.

## Prerequisites

### All Platforms

- **CMake 3.15+** - Build system generator
- **Git** - For cloning dependencies
- **OpenGL 4.3+** - GPU compute shader support

### Windows

- **Visual Studio 2019 or newer** (Community Edition works)
- Windows 10/11
- **Visual C++ Desktop Development** workload installed

### Linux

- **GCC 7+** or **Clang 6+**
- Development packages:
  ```bash
  sudo apt-get install build-essential cmake git
  sudo apt-get install libgl1-mesa-dev libglew-dev
  ```

### macOS

- **Xcode Command Line Tools**:
  ```bash
  xcode-select --install
  ```
- OpenGL 4.1+ (built into macOS)

## Quick Start

### 1. Clone/Download GMod SDK

Run the setup script for your platform:

**Linux/macOS:**
```bash
cd gmod_module
chmod +x setup_sdk.sh
./setup_sdk.sh
```

**Windows:**
```batch
cd gmod_module
setup_sdk.bat
```

This downloads the `gmod-module-base` library which provides the Lua API headers.

### 2. Configure with CMake

**Windows (Visual Studio):**
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
```

**Linux:**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

**macOS:**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### 3. Build

**Windows:**
```bash
cmake --build . --config Release
```

**Linux/macOS:**
```bash
make -j$(nproc)
```

### 4. Install to Addon

```bash
cmake --install .
```

This copies the built modules to `../gmod_addon/starwars_particles/lua/bin/`

## Detailed Build Steps

### Step 1: Verify Directory Structure

Before building, ensure your directory structure looks like this:

```
gmod_module/
├── CMakeLists.txt
├── BUILD.md (this file)
├── SETUP_GMOD_SDK.md
├── setup_sdk.sh
├── setup_sdk.bat
├── include/
│   └── opengl_includes.h
├── source/
│   ├── client/
│   ├── server/
│   └── particle_data.cpp
├── shaders/
│   ├── particle_update.comp
│   ├── particle_emit.comp
│   ├── particle.vert
│   └── particle.frag
└── lib/
    └── gmod-module-base/  (created by setup script)
        └── include/
            └── GarrysMod/Lua/Interface.h
```

### Step 2: Configure Build Options

CMake configuration variables you can customize:

**Install Path:**
```bash
cmake .. -DADDON_INSTALL_PATH="/path/to/addon/lua/bin"
```

**Build Type (Linux/macOS):**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug    # For debugging
cmake .. -DCMAKE_BUILD_TYPE=Release  # For production (default)
```

**Generator (Windows):**
```bash
# Visual Studio 2019
cmake .. -G "Visual Studio 16 2019" -A x64

# Visual Studio 2022
cmake .. -G "Visual Studio 17 2022" -A x64
```

### Step 3: Compile

**Windows - Command Line:**
```bash
cmake --build . --config Release --parallel
```

**Windows - Visual Studio IDE:**
1. Open `gmod_gpu_particles.sln` in Visual Studio
2. Set configuration to `Release` (top toolbar)
3. Build → Build Solution (Ctrl+Shift+B)

**Linux:**
```bash
make -j$(nproc)  # Uses all CPU cores
```

**macOS:**
```bash
make -j$(sysctl -n hw.ncpu)  # Uses all CPU cores
```

### Step 4: Verify Build

After building, you should have these files in `build/bin/`:

**Windows:**
- `gmcl_particles_win64.dll` (client module)
- `gmsv_particles_win64.dll` (server module)

**Linux:**
- `gmcl_particles_linux64.so`
- `gmsv_particles_linux64.so`

**macOS:**
- `gmcl_particles_osx64.dylib`
- `gmsv_particles_osx64.dylib`

### Step 5: Install

**Option A: CMake Install (Recommended)**
```bash
cmake --install .
```

Installs to: `../gmod_addon/starwars_particles/lua/bin/`

**Option B: Manual Copy**

Windows:
```batch
copy build\bin\*.dll ..\gmod_addon\starwars_particles\lua\bin\
```

Linux/macOS:
```bash
cp build/bin/*.so ../gmod_addon/starwars_particles/lua/bin/
```

## Testing

### In Garry's Mod

1. Copy the addon to your GMod addons folder:
   ```
   <garrysmod>/addons/starwars_particles/
   ```

2. Start GMod and open console

3. Verify module loaded:
   ```lua
   lua_run_cl print(particles ~= nil)
   ```
   Should print: `true`

4. Test loading a particle system:
   ```lua
   lua_run_cl particles.Load("explosion.gpart")
   lua_run_cl particles.Spawn("explosion.gpart", LocalPlayer():GetPos(), 1.0)
   ```

### Common Build Issues

#### Issue: "Cannot find GarrysMod/Lua/Interface.h"

**Solution:** Run the setup script to download GMod SDK:
```bash
./setup_sdk.sh  # Linux/macOS
setup_sdk.bat   # Windows
```

#### Issue: "Cannot find OpenGL headers"

**Windows:** Install Visual C++ Desktop Development workload
**Linux:** `sudo apt-get install libgl1-mesa-dev libglew-dev`
**macOS:** OpenGL is built-in, but run `xcode-select --install`

#### Issue: CMake can't find compiler

**Windows:** Install Visual Studio with C++ support
**Linux:** `sudo apt-get install build-essential`
**macOS:** Run `xcode-select --install`

#### Issue: Link errors about glew32.lib

**Windows Only:** The project expects GLEW to be available. You may need to:
1. Download GLEW from http://glew.sourceforge.net/
2. Extract to `C:\Program Files\GLEW\`
3. Or use vcpkg: `vcpkg install glew:x64-windows`

#### Issue: Shaders not found at runtime

The shaders are copied to `build/shaders/`. Make sure GMod's working directory can access them, or update the paths in the code.

## Cross-Platform Building

### Building for Multiple Platforms

To distribute on Workshop, you need modules for Windows and Linux:

**On Windows:**
```bash
# Build Windows modules
cmake --build . --config Release
```

**On Linux:**
```bash
# Build Linux modules
cmake --build . --config Release
```

**Collect both:**
Copy all `.dll` and `.so` files to the addon's `lua/bin/` folder.

### Using Docker for Linux Builds (from Windows)

Create `Dockerfile`:
```dockerfile
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    libgl1-mesa-dev libglew-dev
WORKDIR /build
```

Build:
```bash
docker build -t gmod-builder .
docker run -v "%cd%:/build" gmod-builder cmake --build . --config Release
```

## Advanced Options

### Debug Builds

For development and debugging:

**Windows:**
```bash
cmake --build . --config Debug
```

**Linux/macOS:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

Debug builds include:
- OpenGL error checking (`GL_CHECK` macro)
- Verbose console output
- Debug symbols for debuggers

### Static Analysis

**Clang-Tidy:**
```bash
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy source/**/*.cpp
```

**CppCheck:**
```bash
cppcheck --enable=all source/
```

### Performance Profiling

**Windows - Visual Studio:**
- Use built-in profiler (Debug → Performance Profiler)

**Linux - perf:**
```bash
perf record -g ./gmod
perf report
```

**GPU Profiling:**
- NVIDIA Nsight Graphics
- AMD Radeon GPU Profiler
- RenderDoc

## Clean Build

To clean and rebuild from scratch:

```bash
cd build
rm -rf *        # Linux/macOS
# OR
del /s /q *     # Windows

# Reconfigure and build
cmake ..
cmake --build . --config Release
```

## Distribution

### Packaging for Workshop

After building for all platforms:

1. Verify structure:
   ```
   starwars_particles/
   ├── addon.json
   ├── lua/
   │   ├── bin/
   │   │   ├── gmcl_particles_win64.dll
   │   │   ├── gmcl_particles_linux64.so
   │   │   ├── gmsv_particles_win64.dll
   │   │   └── gmsv_particles_linux64.so
   │   └── autorun/
   │       └── ...
   └── particles/
       └── *.gpart
   ```

2. Test locally in `garrysmod/addons/`

3. Upload to Workshop:
   ```bash
   gmpublish create -addon starwars_particles.gma -icon icon.jpg
   ```

## Troubleshooting

### Module Won't Load in GMod

Check console for errors:
- "Module not found" → Wrong filename or location
- "Failed to load module" → Missing dependencies (OpenGL, GLEW)
- "OpenGL 4.3 required" → GPU doesn't support compute shaders

### Crashes on Load

- **Windows:** Check Visual C++ Redistributables are installed
- **Linux:** Check `ldd` for missing shared libraries:
  ```bash
  ldd gmcl_particles_linux64.so
  ```

### Performance Issues

- Build in **Release** mode, not Debug
- Verify GPU supports OpenGL 4.3+:
  ```lua
  lua_run_cl print(render.GetOpenGLVersion())
  ```

## Next Steps

After successful build:
1. See `../docs/USAGE_GUIDE.md` for how to use the system
2. See `../docs/QUICK_REFERENCE.md` for Lua API reference
3. See `../unity_exporter/` for creating particle effects

---

*For issues, see [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) or the GitHub issues page*
