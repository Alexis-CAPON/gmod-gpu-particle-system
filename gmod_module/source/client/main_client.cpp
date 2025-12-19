// GMod Binary Module - GPU Particle System (Client)
// Entry point for the client-side particle system (DirectX 9)

#include "GarrysMod/Lua/Interface.h"
#include <iostream>

// Use the GarrysMod::Lua namespace
using namespace GarrysMod::Lua;

// Forward declarations from lua_api_client_dx9.cpp
extern bool InitializeParticleSystem();
extern void ShutdownParticleSystem();
extern void RegisterLuaAPI(ILuaBase* lua);
extern void UpdateParticles(float deltaTime);
extern void RenderParticles(const float* viewMatrix, const float* projMatrix, const float* cameraPos);

// ============================================================================
// Module Entry Point
// ============================================================================

// Called when the module is loaded
GMOD_MODULE_OPEN()
{
    std::cout << "=====================================================" << std::endl;
    std::cout << "  GPU Particle System for Garry's Mod (DirectX 9)" << std::endl;
    std::cout << "  Version 1.0.0" << std::endl;
    std::cout << "=====================================================" << std::endl;

    // Initialize the particle system (installs DirectX hook)
    std::cout << "[Module] Initializing DirectX 9 hook..." << std::endl;
    InitializeParticleSystem();

    // Register Lua API
    RegisterLuaAPI(LUA);

    std::cout << "[Module] Client module loaded successfully!" << std::endl;
    return 0;
}

// Called when the module is unloaded
GMOD_MODULE_CLOSE()
{
    std::cout << "[Module] Shutting down client module..." << std::endl;

    ShutdownParticleSystem();

    std::cout << "[Module] Client module unloaded" << std::endl;
    return 0;
}

// ============================================================================
// Frame Hooks (Called by GMod)
// ============================================================================

// Called every frame to update particles
// In actual implementation, this would be hooked into GMod's think/tick system
void OnThink(float deltaTime) {
    UpdateParticles(deltaTime);
}

// Called every frame to render particles
// In actual implementation, this would be hooked into GMod's render system
void OnRender(const float* viewMatrix, const float* projMatrix, const float* cameraPos) {
    RenderParticles(viewMatrix, projMatrix, cameraPos);
}

// ============================================================================
// Module Information
// ============================================================================

const char* GetModuleName() {
    return "GPU Particle System";
}

const char* GetModuleVersion() {
    return "1.0.0";
}

const char* GetModuleAuthor() {
    return "Claude Code";
}

const char* GetModuleDescription() {
    return "DirectX 9 GPU-accelerated particle system with Unity integration";
}
