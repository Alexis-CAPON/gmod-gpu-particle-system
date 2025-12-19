// GMod Binary Module - GPU Particle System (Server)
// Entry point for the server-side particle system coordination

// TODO: Include GMod module headers
// #include "GarrysMod/Lua/Interface.h"

#include <iostream>

// ============================================================================
// Server Module (Minimal)
// ============================================================================
// The server module is minimal because:
// 1. All particle rendering happens client-side
// 2. Networking is handled in Lua (see sv_particles.lua)
// 3. Server only needs to exist for module loading symmetry
//
// In the future, this could be extended to:
// - Track server-side particle effects for replication
// - Handle collision/damage from particles
// - Provide server-side particle management API

// ============================================================================
// Module Entry Point
// ============================================================================

// Called when the module is loaded
// TODO: Replace with actual GMod module macro
// GMOD_MODULE_OPEN()
extern "C" int gmsv_particles_win64_open(void* luaState) {
    std::cout << "=====================================================" << std::endl;
    std::cout << "  GPU Particle System - Server Module" << std::endl;
    std::cout << "  Version 1.0.0" << std::endl;
    std::cout << "=====================================================" << std::endl;

    std::cout << "[Server Module] Loaded successfully!" << std::endl;
    std::cout << "[Server Module] Particle coordination handled by Lua" << std::endl;

    // Register any server-side Lua functions if needed
    // For now, the server module is just a stub

    return 0;
}

// Called when the module is unloaded
// TODO: Replace with actual GMod module macro
// GMOD_MODULE_CLOSE()
extern "C" int gmsv_particles_win64_close(void* luaState) {
    std::cout << "[Server Module] Shutting down..." << std::endl;
    std::cout << "[Server Module] Unloaded" << std::endl;
    return 0;
}

// ============================================================================
// Module Information
// ============================================================================

const char* GetModuleName() {
    return "GPU Particle System (Server)";
}

const char* GetModuleVersion() {
    return "1.0.0";
}

const char* GetModuleDescription() {
    return "Server-side coordination for GPU particle system";
}
