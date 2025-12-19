// Lua API for GPU Particle System (Client-side)
// This file implements the Lua bindings for the particle system

#include "GarrysMod/Lua/Interface.h"
#include "gpu_context.h"
#include "particle_loader.h"
#include "gpu_particle_engine.h"
#include "particle_renderer.h"
#include "../particle_data.h"

#include <memory>
#include <unordered_map>
#include <iostream>

using namespace GPUParticles;
using namespace GarrysMod::Lua;

// Forward declarations (implemented in this file)
bool InitializeParticleSystem();
void ShutdownParticleSystem();
void UpdateParticles(float deltaTime);
void RenderParticles(const float* viewMatrix, const float* projectionMatrix, const float* cameraPos);

// Macro to define Lua functions
#define LUA_FUNCTION(name) int name(lua_State* state)

// GMod Vector structure (for userdata)
struct Vector {
    float x, y, z;
};

// ============================================================================
// Global State
// ============================================================================

struct ParticleSystemInstance {
    std::unique_ptr<GPUParticleEngine> engine;
    Vector3 position;
    float scale;
    Color color;
};

// Global particle system manager
static std::unique_ptr<GPUContext> g_gpuContext;
static std::unique_ptr<ParticleRenderer> g_renderer;
static std::unique_ptr<ParticleLoader> g_loader;
static bool g_systemInitialized = false;

// Loaded particle systems (name -> data)
static std::unordered_map<std::string, std::unique_ptr<ParticleSystemData>> g_loadedSystems;

// Active instances (instanceID -> instance)
static std::unordered_map<int, ParticleSystemInstance> g_activeInstances;
static int g_nextInstanceID = 1;

// Lazy initialization - called on first use
static bool EnsureInitialized() {
    if (g_systemInitialized) {
        return true;
    }

    std::cout << "[Particle System] Performing lazy initialization..." << std::endl;

    if (!InitializeParticleSystem()) {
        std::cerr << "[Particle System] Lazy initialization failed!" << std::endl;
        return false;
    }

    g_systemInitialized = true;
    return true;
}

// ============================================================================
// Lua API Functions
// ============================================================================

// particles.Load(filename)
// Loads a .gpart file and prepares it for spawning
// Returns: boolean success
// DEPRECATED: Use LoadFromString instead (GMod's filesystem doesn't work with std::ifstream)
LUA_FUNCTION(LUA_Load) {
    // Ensure system is initialized
    if (!EnsureInitialized()) {
        LUA->PushBool(false);
        return 1;
    }

    // Get filename argument
    LUA->CheckType(1, Type::STRING);
    const char* filename = LUA->GetString(1);

    // Print to Lua console
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "print");
    LUA->PushString("[C++ Module] Warning: particles.Load() uses filesystem - prefer LoadFromString()");
    LUA->Call(1, 0);
    LUA->Pop();

    // Check if already loaded
    if (g_loadedSystems.find(filename) != g_loadedSystems.end()) {
        LUA->PushBool(true);
        return 1;
    }

    // Build full path
    std::string fullPath = std::string("particles/") + filename;

    // Load from file
    auto data = g_loader->LoadFromFile(fullPath);
    if (!data) {
        // Print error to Lua console
        LUA->PushSpecial(SPECIAL_GLOB);
        LUA->GetField(-1, "print");
        LUA->PushString(("[C++ Module] Failed to load: " + g_loader->GetLastError()).c_str());
        LUA->Call(1, 0);
        LUA->Pop();

        LUA->PushBool(false);
        return 1;
    }

    // Store loaded system
    g_loadedSystems[filename] = std::move(data);

    LUA->PushBool(true);
    return 1;
}

// particles.LoadFromString(name, jsonString)
// Loads a particle system from JSON string (preferred method)
// Returns: boolean success
LUA_FUNCTION(LUA_LoadFromString) {
    // Ensure system is initialized
    if (!EnsureInitialized()) {
        LUA->PushSpecial(SPECIAL_GLOB);
        LUA->GetField(-1, "print");
        LUA->PushString("[C++ Module] ERROR: Failed to initialize particle system!");
        LUA->Call(1, 0);
        LUA->Pop();

        LUA->PushBool(false);
        return 1;
    }

    // Get arguments
    LUA->CheckType(1, Type::STRING);
    LUA->CheckType(2, Type::STRING);
    const char* name = LUA->GetString(1);
    const char* jsonString = LUA->GetString(2);

    // Print to Lua console
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "print");
    LUA->PushString(("[C++ Module] Parsing particle system: " + std::string(name)).c_str());
    LUA->Call(1, 0);
    LUA->Pop();

    // Check if already loaded
    if (g_loadedSystems.find(name) != g_loadedSystems.end()) {
        LUA->PushBool(true);
        return 1;
    }

    // Parse JSON string
    auto data = g_loader->LoadFromString(jsonString);
    if (!data) {
        // Print error to Lua console
        LUA->PushSpecial(SPECIAL_GLOB);
        LUA->GetField(-1, "print");
        LUA->PushString(("[C++ Module] Parse error: " + g_loader->GetLastError()).c_str());
        LUA->Call(1, 0);
        LUA->Pop();

        LUA->PushBool(false);
        return 1;
    }

    // Store loaded system
    g_loadedSystems[name] = std::move(data);

    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "print");
    LUA->PushString("[C++ Module] Parse successful!");
    LUA->Call(1, 0);
    LUA->Pop();

    LUA->PushBool(true);
    return 1;
}

// particles.Spawn(name, pos, scale, color)
// Spawns a particle effect instance
// Returns: instance ID (number)
LUA_FUNCTION(LUA_Spawn) {
    // Ensure system is initialized
    if (!EnsureInitialized()) {
        LUA->PushNumber(-1);
        return 1;
    }

    // Get arguments
    LUA->CheckType(1, Type::STRING);
    const char* name = LUA->GetString(1);

    // Get position (GMod Vector)
    LUA->CheckType(2, Type::VECTOR);
    Vector* gmodVec = (Vector*)LUA->GetUserdata(2);
    Vector3 pos(gmodVec->x, gmodVec->y, gmodVec->z);

    // Get scale (optional, default 1.0)
    float scale = 1.0f;
    if (LUA->Top() >= 3 && LUA->IsType(3, Type::NUMBER)) {
        scale = (float)LUA->GetNumber(3);
    }

    // Get color (optional, default white)
    Color color(1, 1, 1, 1);
    if (LUA->Top() >= 4 && LUA->IsType(4, Type::TABLE)) {
        LUA->GetField(4, "r");
        color.r = (float)LUA->GetNumber(-1) / 255.0f;
        LUA->Pop();

        LUA->GetField(4, "g");
        color.g = (float)LUA->GetNumber(-1) / 255.0f;
        LUA->Pop();

        LUA->GetField(4, "b");
        color.b = (float)LUA->GetNumber(-1) / 255.0f;
        LUA->Pop();

        LUA->GetField(4, "a");
        color.a = (float)LUA->GetNumber(-1) / 255.0f;
        LUA->Pop();
    }

    std::cout << "[Lua API] Spawning: " << name << " at (" << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;

    // Check if GPU context is ready
    if (!g_gpuContext || !g_gpuContext->IsInitialized()) {
        LUA->PushSpecial(SPECIAL_GLOB);
        LUA->GetField(-1, "print");
        LUA->PushString("[C++ Module] ERROR: GPU not initialized. Try again in a moment.");
        LUA->Call(1, 0);
        LUA->Pop();

        LUA->PushNumber(-1);
        return 1;
    }

    // Check if system is loaded
    auto it = g_loadedSystems.find(name);
    if (it == g_loadedSystems.end()) {
        std::cerr << "[Lua API] System not loaded: " << name << std::endl;
        LUA->PushNumber(-1);
        return 1;
    }

    // Create engine instance
    auto engine = std::make_unique<GPUParticleEngine>();
    if (!engine->Initialize(*it->second)) {
        std::cerr << "[Lua API] Failed to initialize engine: " << engine->GetLastError() << std::endl;
        LUA->PushNumber(-1);
        return 1;
    }

    // Create instance
    ParticleSystemInstance instance;
    instance.engine = std::move(engine);
    instance.position = pos;
    instance.scale = scale;
    instance.color = color;

    // Assign ID and store
    int instanceID = g_nextInstanceID++;
    g_activeInstances[instanceID] = std::move(instance);

    std::cout << "[Lua API] Spawned instance ID: " << instanceID << std::endl;
    LUA->PushNumber(instanceID);
    return 1;
}

// particles.Kill(instanceID)
// Kills a specific particle effect instance
// Returns: boolean success
LUA_FUNCTION(LUA_Kill) {
    LUA->CheckType(1, Type::NUMBER);
    int instanceID = (int)LUA->GetNumber(1);

    std::cout << "[Lua API] Killing instance: " << instanceID << std::endl;

    auto it = g_activeInstances.find(instanceID);
    if (it == g_activeInstances.end()) {
        LUA->PushBool(false);
        return 1;
    }

    g_activeInstances.erase(it);

    std::cout << "[Lua API] Killed successfully" << std::endl;
    LUA->PushBool(true);
    return 1;
}

// particles.KillInRadius(pos, radius)
// Kills all particle instances within a radius
// Returns: number of instances killed
LUA_FUNCTION(LUA_KillInRadius) {
    LUA->CheckType(1, Type::VECTOR);
    LUA->CheckType(2, Type::NUMBER);

    Vector* gmodVec = (Vector*)LUA->GetUserdata(1);
    Vector3 pos(gmodVec->x, gmodVec->y, gmodVec->z);
    float radius = (float)LUA->GetNumber(2);

    int killedCount = 0;
    float radiusSq = radius * radius;

    for (auto it = g_activeInstances.begin(); it != g_activeInstances.end();) {
        const Vector3& instancePos = it->second.position;
        float dx = instancePos.x - pos.x;
        float dy = instancePos.y - pos.y;
        float dz = instancePos.z - pos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= radiusSq) {
            it = g_activeInstances.erase(it);
            killedCount++;
        } else {
            ++it;
        }
    }

    std::cout << "[Lua API] Killed " << killedCount << " instances in radius" << std::endl;
    LUA->PushNumber(killedCount);
    return 1;
}

// particles.GetTotalParticleCount()
// Returns total particle count across all instances
// Returns: number
LUA_FUNCTION(LUA_GetTotalParticleCount) {
    int total = 0;
    for (const auto& pair : g_activeInstances) {
        total += pair.second.engine->GetAliveCount();
    }

    LUA->PushNumber(total);
    return 1;
}

// particles.GetGPUTime()
// Returns GPU time spent on particles (stub for now)
// Returns: number (milliseconds)
LUA_FUNCTION(LUA_GetGPUTime) {
    // TODO: Implement actual GPU profiling
    float gpuTime = 0.0f;

    LUA->PushNumber(gpuTime);
    return 1;
}

// particles.InitGPU()
// Debug function to manually try GPU initialization and report status
// Returns: boolean success
LUA_FUNCTION(LUA_InitGPU) {
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "print");
    LUA->PushString("[C++ Module] Attempting GPU initialization...");
    LUA->Call(1, 0);
    LUA->Pop();

    bool success = EnsureInitialized();

    // Check actual GPU state
    bool gpuReady = g_gpuContext && g_gpuContext->IsInitialized();
    bool rendererReady = g_renderer && g_renderer->IsInitialized();

    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "print");
    std::string msg = "[C++ Module] Init result: ";
    msg += "Loader=" + std::string(g_loader ? "OK" : "FAIL") + ", ";
    msg += "GPU=" + std::string(gpuReady ? "OK" : "FAIL");
    if (!gpuReady && g_gpuContext) {
        msg += " (Error: " + g_gpuContext->GetLastError() + ")";
    }
    msg += ", Renderer=" + std::string(rendererReady ? "OK" : "FAIL");
    LUA->PushString(msg.c_str());
    LUA->Call(1, 0);
    LUA->Pop();

    LUA->PushBool(gpuReady);
    return 1;
}

// particles.Update(deltaTime)
// Updates all particle systems
// Called every frame by Lua Think hook
LUA_FUNCTION(LUA_Update) {
    LUA->CheckType(1, Type::NUMBER);
    float deltaTime = (float)LUA->GetNumber(1);

    UpdateParticles(deltaTime);

    return 0;
}

// particles.Render(viewSetup)
// Renders all particle systems
// Called every frame by Lua render hook
LUA_FUNCTION(LUA_Render) {
    // Get view setup table with camera info
    LUA->CheckType(1, Type::TABLE);

    // Extract camera position
    LUA->GetField(1, "origin");
    Vector* origin = (Vector*)LUA->GetUserdata(-1);
    LUA->Pop();

    // Extract angles and build matrices
    // For now, we'll build the matrices inside the render function
    // TODO: Extract view and projection matrices from GMod

    float cameraPos[3] = {origin->x, origin->y, origin->z};
    float viewMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; // Identity for now
    float projMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; // Identity for now

    RenderParticles(viewMatrix, projMatrix, cameraPos);

    return 0;
}

// ============================================================================
// Module Update (Called every frame)
// ============================================================================

void UpdateParticles(float deltaTime) {
    // Update all active instances
    for (auto& pair : g_activeInstances) {
        pair.second.engine->Update(deltaTime);
    }
}

void RenderParticles(const float* viewMatrix, const float* projectionMatrix, const float* cameraPos) {
    // Try to initialize GPU if not already done (OpenGL context should be ready now)
    if ((!g_gpuContext || !g_gpuContext->IsInitialized()) && !g_systemInitialized) {
        EnsureInitialized();
    }

    if (!g_renderer || !g_renderer->IsInitialized()) {
        return;
    }

    // Render all active instances
    for (const auto& pair : g_activeInstances) {
        g_renderer->Render(*pair.second.engine, viewMatrix, projectionMatrix, cameraPos);
    }
}

// ============================================================================
// Module Initialization
// ============================================================================

bool InitializeParticleSystem() {
    std::cout << "[Particle System] Initializing..." << std::endl;

    // Initialize loader first (doesn't need GPU)
    if (!g_loader) {
        g_loader = std::make_unique<ParticleLoader>();
    }

    // Initialize GPU context (may fail if OpenGL not ready yet, but that's OK for loading)
    if (!g_gpuContext) {
        g_gpuContext = std::make_unique<GPUContext>();
    }

    if (!g_gpuContext->IsInitialized()) {
        if (!g_gpuContext->Initialize()) {
            std::cerr << "[Particle System] Warning: GPU context not ready yet (this is normal on first call)" << std::endl;
            // Don't return false - we can still load particle data
        }
    }

    // Initialize renderer (only if GPU context worked)
    if (!g_renderer && g_gpuContext && g_gpuContext->IsInitialized()) {
        g_renderer = std::make_unique<ParticleRenderer>();
        if (!g_renderer->Initialize()) {
            std::cerr << "[Particle System] Failed to initialize renderer" << std::endl;
            return false;
        }
    }

    std::cout << "[Particle System] Initialization successful!" << std::endl;
    return true;
}

void ShutdownParticleSystem() {
    std::cout << "[Particle System] Shutting down..." << std::endl;

    // Clear all active instances
    g_activeInstances.clear();

    // Clear loaded systems
    g_loadedSystems.clear();

    // Shutdown components
    g_loader.reset();
    g_renderer.reset();
    g_gpuContext.reset();

    std::cout << "[Particle System] Shutdown complete" << std::endl;
}

// ============================================================================
// Register Lua Functions (Called by module init)
// ============================================================================

void RegisterLuaAPI(ILuaBase* lua) {
    // Push global table
    lua->PushSpecial(SPECIAL_GLOB);

    // Create "particles" table
    lua->CreateTable();

    // Register functions
    lua->PushCFunction(LUA_Load);
    lua->SetField(-2, "Load");

    lua->PushCFunction(LUA_LoadFromString);
    lua->SetField(-2, "LoadFromString");

    lua->PushCFunction(LUA_Spawn);
    lua->SetField(-2, "Spawn");

    lua->PushCFunction(LUA_Kill);
    lua->SetField(-2, "Kill");

    lua->PushCFunction(LUA_KillInRadius);
    lua->SetField(-2, "KillInRadius");

    lua->PushCFunction(LUA_GetTotalParticleCount);
    lua->SetField(-2, "GetTotalParticleCount");

    lua->PushCFunction(LUA_GetGPUTime);
    lua->SetField(-2, "GetGPUTime");

    lua->PushCFunction(LUA_InitGPU);
    lua->SetField(-2, "InitGPU");

    lua->PushCFunction(LUA_Update);
    lua->SetField(-2, "Update");

    lua->PushCFunction(LUA_Render);
    lua->SetField(-2, "Render");

    // Register table as "particles" global
    lua->SetField(-2, "particles");

    // Pop global table
    lua->Pop();

    std::cout << "[Particle System] Lua API registered" << std::endl;
}
