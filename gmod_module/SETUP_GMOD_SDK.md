# GMod SDK Setup Guide

This guide explains how to set up the Garry's Mod module SDK for building the GPU Particle System.

## Prerequisites

- Git installed
- CMake 3.15+
- C++17 compiler (Visual Studio 2019+, GCC 7+, or Clang)

## Step 1: Clone the GMod Module Base

The `gmod-module-base` library provides the necessary headers and macros for creating GMod binary modules.

### Option A: Using Git Submodule (Recommended)

```bash
cd gmod_module
git submodule add https://github.com/Facepunch/gmod-module-base.git lib/gmod-module-base
git submodule update --init --recursive
```

### Option B: Manual Clone

```bash
cd gmod_module/lib
git clone https://github.com/Facepunch/gmod-module-base.git
```

## Step 2: Verify Directory Structure

After cloning, your directory structure should look like this:

```
gmod_module/
├── CMakeLists.txt
├── include/
├── source/
└── lib/
    └── gmod-module-base/
        └── include/
            └── GarrysMod/
                └── Lua/
                    ├── Interface.h
                    ├── LuaBase.h
                    ├── Types.h
                    └── ...
```

## Step 3: Build the Project

Now you can build with CMake:

```bash
cd gmod_module
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## What is gmod-module-base?

The `gmod-module-base` library provides:

1. **Lua Interface Headers** (`GarrysMod/Lua/Interface.h`)
   - `ILuaBase` class for interacting with Lua
   - Type checking and conversion functions
   - Stack manipulation

2. **Module Macros** (`GMOD_MODULE_OPEN`, `GMOD_MODULE_CLOSE`)
   - Proper module entry/exit points
   - Automatic function signature handling

3. **Type Definitions** (`GarrysMod/Lua/Types.h`)
   - Lua type constants
   - Special table indices
   - Error handling

## Common Issues

### Issue: "fatal error: GarrysMod/Lua/Interface.h: No such file or directory"

**Solution:** Make sure you've cloned `gmod-module-base` into `lib/gmod-module-base/` and that `include_directories()` in CMakeLists.txt points to it.

### Issue: Git submodule is empty

**Solution:** Run:
```bash
git submodule update --init --recursive
```

### Issue: CMake can't find the headers

**Solution:** Verify the CMakeLists.txt includes:
```cmake
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/gmod-module-base/include
)
```

## Alternative: Header-Only Setup

If you can't use git submodules, you can manually download just the headers:

1. Download from: https://github.com/Facepunch/gmod-module-base/tree/master/include
2. Place in: `gmod_module/lib/gmod-module-base/include/`
3. Ensure the `GarrysMod/` directory structure is preserved

## Verifying Installation

After setup, you should be able to compile files that include:

```cpp
#include "GarrysMod/Lua/Interface.h"

using namespace GarrysMod::Lua;

GMOD_MODULE_OPEN()
{
    // Your code here
    return 0;
}

GMOD_MODULE_CLOSE()
{
    return 0;
}
```

## Next Steps

Once the SDK is set up:
1. Build the project with CMake
2. The compiled modules will be in `build/bin/`
3. Install to addon with `cmake --install .`
4. Test in Garry's Mod

## Documentation Links

- **GMod Module Base**: https://github.com/Facepunch/gmod-module-base
- **GMod Lua API**: https://wiki.facepunch.com/gmod/
- **Creating Binary Modules**: https://wiki.facepunch.com/gmod/Creating_Binary_Modules

---

*This guide is part of the GPU Particle System for Garry's Mod*
