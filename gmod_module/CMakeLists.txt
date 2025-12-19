cmake_minimum_required(VERSION 3.15)
project(gmod_gpu_particles VERSION 1.0.0 LANGUAGES CXX)

# C++17 required for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_SUFFIX "win32")
    set(LIB_PREFIX "")
    set(LIB_SUFFIX ".dll")
    message(STATUS "Building for Windows 32-bit (.dll)")
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SUFFIX "linux64")
    set(LIB_PREFIX "")
    set(LIB_SUFFIX ".so")
    message(STATUS "Building for Linux (.so)")
elseif(APPLE)
    set(PLATFORM_SUFFIX "osx64")
    set(LIB_PREFIX "")
    set(LIB_SUFFIX ".dylib")
    message(WARNING "Building for macOS - NOTE: Garry's Mod no longer supports macOS!")
    message(WARNING "These .dylib files will NOT work in GMod. Build on Windows or Linux instead.")
endif()

# Output directory for built modules
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Build MinHook library (Windows only)
if(WIN32)
    add_subdirectory(lib/minhook)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/gmod-module-base/include
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/minhook/include
)

# Source files shared between client and server
set(COMMON_SOURCES
    source/particle_data.h
    source/particle_data.cpp
)

# Client-only sources (DirectX 9 CPU/GPU hybrid rendering with hooks)
set(CLIENT_SOURCES
    ${COMMON_SOURCES}
    source/client/main_client.cpp
    source/client/particle_loader.cpp
    source/client/particle_loader.h
    source/client/dx9_context.cpp
    source/client/dx9_context.h
    source/client/cpu_particle_simulator.cpp
    source/client/cpu_particle_simulator.h
    source/client/dx9_particle_renderer.cpp
    source/client/dx9_particle_renderer.h
    source/client/d3d9_hook.cpp
    source/client/d3d9_hook.h
    source/client/lua_api_client_dx9.cpp
)

# Server-only sources (networking coordination)
set(SERVER_SOURCES
    ${COMMON_SOURCES}
    source/server/main_server.cpp
)

# Client module (gmcl_particles)
add_library(gmcl_particles SHARED ${CLIENT_SOURCES})
set_target_properties(gmcl_particles PROPERTIES
    PREFIX "${LIB_PREFIX}"
    OUTPUT_NAME "gmcl_particles_${PLATFORM_SUFFIX}"
    SUFFIX "${LIB_SUFFIX}"
)

# Link DirectX 9 libraries (Windows only)
if(WIN32)
    target_link_libraries(gmcl_particles PRIVATE d3d9 d3dcompiler minhook)
endif()

# Server module (gmsv_particles)
add_library(gmsv_particles SHARED ${SERVER_SOURCES})
set_target_properties(gmsv_particles PROPERTIES
    PREFIX "${LIB_PREFIX}"
    OUTPUT_NAME "gmsv_particles_${PLATFORM_SUFFIX}"
    SUFFIX "${LIB_SUFFIX}"
)

# Platform-specific settings
if(WIN32)
    # Windows specific - DirectX 9
    target_compile_definitions(gmcl_particles PRIVATE WIN32 _WINDOWS GMMODULE NOMINMAX)
    target_compile_definitions(gmsv_particles PRIVATE WIN32 _WINDOWS GMMODULE NOMINMAX)

elseif(UNIX AND NOT APPLE)
    # Linux specific
    target_compile_options(gmcl_particles PRIVATE -fPIC)
    target_compile_options(gmsv_particles PRIVATE -fPIC)
    target_compile_definitions(gmcl_particles PRIVATE GMMODULE)
    target_compile_definitions(gmsv_particles PRIVATE GMMODULE)

    # Link against OpenGL on Linux
    target_link_libraries(gmcl_particles PRIVATE GL GLEW)

elseif(APPLE)
    # macOS specific
    target_compile_options(gmcl_particles PRIVATE -fPIC)
    target_compile_options(gmsv_particles PRIVATE -fPIC)
    target_compile_definitions(gmcl_particles PRIVATE GMMODULE)
    target_compile_definitions(gmsv_particles PRIVATE GMMODULE)

    # Link against OpenGL framework on macOS
    target_link_libraries(gmcl_particles PRIVATE "-framework OpenGL")
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Installation options
set(ADDON_INSTALL_PATH "${CMAKE_SOURCE_DIR}/../gmod_addon/starwars_particles/lua/bin"
    CACHE PATH "Path to addon's lua/bin folder (for Workshop)")

# Install to addon folder (recommended for Workshop distribution)
install(TARGETS gmcl_particles gmsv_particles
    LIBRARY DESTINATION ${ADDON_INSTALL_PATH}
    RUNTIME DESTINATION ${ADDON_INSTALL_PATH}
)

# Alternative: Install to global GMod lua/bin (uncomment if needed)
# install(TARGETS gmcl_particles gmsv_particles
#     LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/garrysmod/lua/bin
#     RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/garrysmod/lua/bin
# )

# Print build information
message(STATUS "=== GMod GPU Particle System ===")
message(STATUS "Platform: ${PLATFORM_SUFFIX}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Client module: gmcl_particles_${PLATFORM_SUFFIX}${LIB_SUFFIX}")
message(STATUS "Server module: gmsv_particles_${PLATFORM_SUFFIX}${LIB_SUFFIX}")
message(STATUS "Install path: ${ADDON_INSTALL_PATH}")
message(STATUS "===============================")
