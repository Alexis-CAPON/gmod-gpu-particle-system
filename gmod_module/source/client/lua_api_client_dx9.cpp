// Lua API for GPU Particle System (DirectX 9 with Hooks)
// This file implements the Lua bindings for the DX9 particle system

#include "GarrysMod/Lua/Interface.h"
#include "dx9_context.h"
#include "d3d9_hook.h"
#include "material_system_helper.h"
#include "particle_loader.h"
#include "cpu_particle_simulator.h"
#include "dx9_particle_renderer.h"
#include "../particle_data.h"

#include <memory>
#include <unordered_map>
#include <iostream>

using namespace GPUParticles;
using namespace GarrysMod::Lua;

// External logger from d3d9_hook.cpp (in GPUParticles namespace)
namespace GPUParticles {
    extern void LogToFile(const std::string& msg);
}

// Forward declarations
bool InitializeParticleSystem();
void ShutdownParticleSystem();
void UpdateParticles(float deltaTime);
void RenderParticles(const float* viewMatrix, const float* projMatrix, const float* cameraPos);

// Macro to define Lua functions
#define LUA_FUNCTION(name) int name(lua_State* state)

// GMod Vector structure
struct Vector {
    float x, y, z;
};

// ============================================================================
// Global State
// ============================================================================

struct ParticleSystemInstance {
    std::unique_ptr<CPUParticleSimulator> simulator;
    Vector3 position;
    float scale;
    Color color;
};

// Global components
static std::unique_ptr<D3D9Hook> g_d3dHook;
static std::unique_ptr<DX9Context> g_dxContext;
static std::unique_ptr<DX9ParticleRenderer> g_renderer;
static std::unique_ptr<ParticleLoader> g_loader;

// Loaded particle system data
static std::unordered_map<std::string, std::unique_ptr<ParticleSystemData>> g_loadedSystems;

// Active particle instances
static std::unordered_map<int, ParticleSystemInstance> g_activeInstances;
static int g_nextInstanceID = 1;

// State
static bool g_systemInitialized = false;

// ============================================================================
// Lazy Initialization
// ============================================================================

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

// particles.LoadFromString(name, jsonString)
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
LUA_FUNCTION(LUA_Spawn) {
    // Ensure system is initialized
    if (!EnsureInitialized()) {
        LUA->PushNumber(-1);
        return 1;
    }

    // Get arguments
    LUA->CheckType(1, Type::STRING);
    const char* name = LUA->GetString(1);

    // Get position (properly extract Vector from Lua)
    LUA->CheckType(2, Type::VECTOR);
    LUA->Push(2); // Push the vector onto the stack

    // Get x component
    LUA->GetField(-1, "x");
    float x = (float)LUA->GetNumber(-1);
    LUA->Pop();

    // Get y component
    LUA->GetField(-1, "y");
    float y = (float)LUA->GetNumber(-1);
    LUA->Pop();

    // Get z component
    LUA->GetField(-1, "z");
    float z = (float)LUA->GetNumber(-1);
    LUA->Pop();

    LUA->Pop(); // Pop the vector

    Vector3 pos(x, y, z);

    // Get scale (optional)
    float scale = 1.0f;
    if (LUA->Top() >= 3 && LUA->IsType(3, Type::NUMBER)) {
        scale = (float)LUA->GetNumber(3);
    }

    // Get color (optional)
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

    // Check if DX9 device is available
    if (!g_d3dHook || !g_d3dHook->HasDevice()) {
        LUA->PushSpecial(SPECIAL_GLOB);
        LUA->GetField(-1, "print");
        LUA->PushString("[C++ Module] ERROR: DirectX device not captured yet. Wait a moment and try again.");
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

    // Create simulator instance
    auto simulator = std::make_unique<CPUParticleSimulator>();
    if (!simulator->Initialize(*it->second)) {
        std::cerr << "[Lua API] Failed to initialize simulator: " << simulator->GetLastError() << std::endl;
        LUA->PushNumber(-1);
        return 1;
    }

    // Create instance
    ParticleSystemInstance instance;
    instance.simulator = std::move(simulator);
    instance.position = pos;
    instance.scale = scale;
    instance.color = color;

    // Debug: Print position being stored
    char posDebug[256];
    sprintf(posDebug, "[LUA_Spawn] Storing position: (%.1f, %.1f, %.1f) scale: %.1f",
            instance.position.x, instance.position.y, instance.position.z, instance.scale);
    LogToFile(posDebug);

    // Assign ID and store
    int instanceID = g_nextInstanceID++;
    g_activeInstances[instanceID] = std::move(instance);

    std::cout << "[Lua API] Spawned instance ID: " << instanceID << std::endl;
    LUA->PushNumber(instanceID);
    return 1;
}

// particles.Update(deltaTime)
LUA_FUNCTION(LUA_Update) {
    LUA->CheckType(1, Type::NUMBER);
    float deltaTime = (float)LUA->GetNumber(1);

    UpdateParticles(deltaTime);

    return 0;
}

// Helper: Extract Angle from Lua table
struct Angle {
    float pitch, yaw, roll;
};

// Helper: Build view matrix from position and angles
void BuildViewMatrix(const Vector& pos, const Angle& angles, float* outMatrix) {
    // Convert angles to radians
    float pitch = angles.pitch * 3.14159f / 180.0f;
    float yaw = angles.yaw * 3.14159f / 180.0f;
    float roll = angles.roll * 3.14159f / 180.0f;

    // Calculate forward, right, up vectors from angles
    float cp = cos(pitch), sp = sin(pitch);
    float cy = cos(yaw), sy = sin(yaw);
    float cr = cos(roll), sr = sin(roll);

    // Forward vector
    float fx = cp * cy;
    float fy = cp * sy;
    float fz = -sp;

    // Right vector
    float rx = sr * sp * cy + cr * sy;
    float ry = sr * sp * sy - cr * cy;
    float rz = sr * cp;

    // Up vector
    float ux = cr * sp * cy - sr * sy;
    float uy = cr * sp * sy + sr * cy;
    float uz = cr * cp;

    // Build view matrix (camera to world, then invert for world to camera)
    outMatrix[0] = rx; outMatrix[1] = ux; outMatrix[2] = -fx; outMatrix[3] = 0;
    outMatrix[4] = ry; outMatrix[5] = uy; outMatrix[6] = -fy; outMatrix[7] = 0;
    outMatrix[8] = rz; outMatrix[9] = uz; outMatrix[10] = -fz; outMatrix[11] = 0;
    outMatrix[12] = -(rx*pos.x + ry*pos.y + rz*pos.z);
    outMatrix[13] = -(ux*pos.x + uy*pos.y + uz*pos.z);
    outMatrix[14] = -(-fx*pos.x + -fy*pos.y + -fz*pos.z);
    outMatrix[15] = 1;
}

// Helper: Build perspective projection matrix
void BuildProjectionMatrix(float fov, float aspect, float zNear, float zFar, float* outMatrix) {
    float f = 1.0f / tan(fov * 3.14159f / 360.0f);

    outMatrix[0] = f / aspect; outMatrix[1] = 0; outMatrix[2] = 0; outMatrix[3] = 0;
    outMatrix[4] = 0; outMatrix[5] = f; outMatrix[6] = 0; outMatrix[7] = 0;
    outMatrix[8] = 0; outMatrix[9] = 0; outMatrix[10] = zFar / (zNear - zFar); outMatrix[11] = -1;
    outMatrix[12] = 0; outMatrix[13] = 0; outMatrix[14] = (zNear * zFar) / (zNear - zFar); outMatrix[15] = 0;
}

// particles.Render(viewSetup)
LUA_FUNCTION(LUA_Render) {
    // Get view setup table
    LUA->CheckType(1, Type::TABLE);

    // Extract camera position - read fields individually to avoid userdata corruption
    LUA->GetField(1, "origin");
    Vector origin;
    LUA->GetField(-1, "x");
    origin.x = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "y");
    origin.y = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "z");
    origin.z = (float)LUA->GetNumber(-1);
    LUA->Pop(2);  // Pop z and origin

    // Extract camera angles (GMod's Angle is {pitch, yaw, roll})
    LUA->GetField(1, "angles");
    Angle angles;
    if (LUA->IsType(-1, Type::ANGLE)) {
        // Get the Angle userdata and extract components
        LUA->PushNumber(1); // pitch
        LUA->GetTable(-2);
        angles.pitch = (float)LUA->GetNumber(-1);
        LUA->Pop();

        LUA->PushNumber(2); // yaw
        LUA->GetTable(-2);
        angles.yaw = (float)LUA->GetNumber(-1);
        LUA->Pop();

        LUA->PushNumber(3); // roll
        LUA->GetTable(-2);
        angles.roll = (float)LUA->GetNumber(-1);
        LUA->Pop();
    } else {
        // Fallback if not an angle
        angles.pitch = 0;
        angles.yaw = 0;
        angles.roll = 0;
    }
    LUA->Pop();

    // Extract FOV
    LUA->GetField(1, "fov");
    float fov = (float)LUA->GetNumber(-1);
    LUA->Pop();

    // Extract aspect ratio
    LUA->GetField(1, "aspect");
    float aspect = (float)LUA->GetNumber(-1);
    LUA->Pop();

    float cameraPos[3] = {origin.x, origin.y, origin.z};

    // Build proper view and projection matrices from GMod's view setup
    float viewMatrix[16];
    float projMatrix[16];

    BuildViewMatrix(origin, angles, viewMatrix);
    BuildProjectionMatrix(fov, aspect, 7.0f, 30000.0f, projMatrix);

    // Log first time
    static bool logged = false;
    if (!logged) {
        char buf[256];
        sprintf(buf, "[LUA_Render] Camera: pos=(%.1f,%.1f,%.1f) angles=(%.1f,%.1f,%.1f) fov=%.1f",
                origin.x, origin.y, origin.z, angles.pitch, angles.yaw, angles.roll, fov);
        LogToFile(buf);
        sprintf(buf, "[LUA_Render] View matrix: [%.2f,%.2f,%.2f,%.2f]",
                viewMatrix[0], viewMatrix[1], viewMatrix[2], viewMatrix[3]);
        LogToFile(buf);
        logged = true;
    }

    RenderParticles(viewMatrix, projMatrix, cameraPos);

    return 0;
}

// particles.RenderTestQuad(position, size)
// Renders a simple textured quad without billboarding for testing
// TEMPORARILY DISABLED - compilation errors
/*
LUA_FUNCTION(LUA_RenderTestQuad) {
    // Get position
    LUA->CheckType(1, Type::VECTOR);
    LUA->Push(1);

    LUA->GetField(-1, "x");
    float x = (float)LUA->GetNumber(-1);
    LUA->Pop();

    LUA->GetField(-1, "y");
    float y = (float)LUA->GetNumber(-1);
    LUA->Pop();

    LUA->GetField(-1, "z");
    float z = (float)LUA->GetNumber(-1);
    LUA->Pop();

    LUA->Pop(); // Pop vector

    // Get size (optional)
    float size = 100.0f;
    if (LUA->Top() >= 2 && LUA->IsType(2, Type::NUMBER)) {
        size = (float)LUA->GetNumber(2);
    }

    // Get view setup from Lua
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "render");
    LUA->GetField(-1, "GetViewSetup");
    LUA->Call(0, 1);  // Call render.GetViewSetup()

    // Extract camera position
    LUA->GetField(-1, "origin");
    Vector* origin = (Vector*)LUA->GetUserdata(-1);
    LUA->Pop();

    // Extract camera angles
    LUA->GetField(-1, "angles");
    QAngle* gmodAngles = (QAngle*)LUA->GetUserdata(-1);
    Angles angles_data;
    angles_data.pitch = gmodAngles->x;
    angles_data.yaw = gmodAngles->y;
    angles_data.roll = gmodAngles->z;
    LUA->Pop();

    // Extract FOV
    LUA->GetField(-1, "fov");
    float fov = (float)LUA->GetNumber(-1);
    LUA->Pop();

    // Get aspect ratio
    UINT width, height;
    if (g_d3dHook && g_d3dHook->GetDevice()) {
        D3DVIEWPORT9 viewport;
        g_d3dHook->GetDevice()->GetViewport(&viewport);
        width = viewport.Width;
        height = viewport.Height;
    } else {
        width = 1920;
        height = 1080;
    }
    float aspect = (float)width / (float)height;

    LUA->Pop(3); // Pop viewSetup, render, global

    float viewMatrix[16];
    float projMatrix[16];

    BuildViewMatrix(*origin, angles_data, viewMatrix);
    BuildProjectionMatrix(fov, aspect, 7.0f, 30000.0f, projMatrix);

    float worldPos[3] = {x, y, z};

    LogToFile("[LUA_RenderTestQuad] Rendering test quad...");
    char buf[256];
    sprintf(buf, "[LUA_RenderTestQuad] Position: (%.1f, %.1f, %.1f) Size: %.1f", x, y, z, size);
    LogToFile(buf);

    if (g_renderer) {
        g_renderer->RenderTestQuad(worldPos, size, viewMatrix, projMatrix);
    }

    return 0;
}
*/

// Helper function to print to Lua console
static void LuaPrint(ILuaBase* lua, const std::string& msg) {
    lua->PushSpecial(SPECIAL_GLOB);
    lua->GetField(-1, "print");
    lua->PushString(msg.c_str());
    lua->Call(1, 0);
    lua->Pop();
}

// particles.GetTotalParticleCount()
LUA_FUNCTION(LUA_GetTotalParticleCount) {
    int total = 0;
    for (const auto& pair : g_activeInstances) {
        total += pair.second.simulator->GetAliveCount();
    }

    LUA->PushNumber(total);
    return 1;
}

// particles.InitGPU() - Debug function
LUA_FUNCTION(LUA_InitGPU) {
    LuaPrint(LUA, "[C++ Module] ===== Diagnostics =====");
    LuaPrint(LUA, "[C++ Module] Attempting initialization...");

    // Check if hook object exists
    LuaPrint(LUA, std::string("[C++ Module] D3D9Hook exists: ") + (g_d3dHook ? "YES" : "NO"));

    if (!g_d3dHook) {
        LuaPrint(LUA, "[C++ Module] Creating D3D9Hook...");
        g_d3dHook = std::make_unique<D3D9Hook>();
    }

    // Check if hook is initialized
    if (g_d3dHook) {
        LuaPrint(LUA, "[C++ Module] Calling hook->Initialize()...");
        bool initResult = g_d3dHook->Initialize();
        LuaPrint(LUA, std::string("[C++ Module] Initialize() returned: ") + (initResult ? "TRUE" : "FALSE"));

        if (!initResult) {
            LuaPrint(LUA, std::string("[C++ Module] Hook error: ") + g_d3dHook->GetLastError());
        }

        // Check device capture
        bool hasDevice = g_d3dHook->HasDevice();
        LuaPrint(LUA, std::string("[C++ Module] HasDevice(): ") + (hasDevice ? "TRUE" : "FALSE"));

        if (hasDevice) {
            IDirect3DDevice9* device = g_d3dHook->GetDevice();
            char hexBuf[32];
            sprintf(hexBuf, "0x%p", (void*)device);
            LuaPrint(LUA, std::string("[C++ Module] Device pointer: ") + hexBuf);
        } else {
            LuaPrint(LUA, "[C++ Module] Device not captured yet - waiting for EndScene call...");
        }
    }

    bool success = EnsureInitialized();

    // Check actual state
    bool hookReady = g_d3dHook && g_d3dHook->HasDevice();
    bool dxReady = g_dxContext && g_dxContext->IsInitialized();
    bool rendererReady = g_renderer && g_renderer->IsInitialized();

    LuaPrint(LUA, "[C++ Module] ===== Final Status =====");
    std::string msg = "[C++ Module] ";
    msg += "Hook=" + std::string(hookReady ? "OK" : "WAITING") + ", ";
    msg += "DX=" + std::string(dxReady ? "OK" : "FAIL") + ", ";
    msg += "Renderer=" + std::string(rendererReady ? "OK" : "FAIL");
    LuaPrint(LUA, msg);

    LUA->PushBool(hookReady && dxReady && rendererReady);
    return 1;
}

// ============================================================================
// Module Update/Render
// ============================================================================

void UpdateParticles(float deltaTime) {
    static int updateCount = 0;
    updateCount++;

    // DEBUG: Log updates
    if (updateCount % 60 == 1 && !g_activeInstances.empty()) {
        char buf[512];
        sprintf(buf, "[UpdateParticles] UPDATE #%d - deltaTime=%.4f, instances=%d",
                updateCount, deltaTime, (int)g_activeInstances.size());
        LogToFile(buf);
    }

    // Update all active instances
    for (auto& pair : g_activeInstances) {
        int beforeCount = pair.second.simulator->GetAliveCount();
        pair.second.simulator->Update(deltaTime);
        int afterCount = pair.second.simulator->GetAliveCount();

        if (updateCount % 60 == 1) {
            char buf[512];
            sprintf(buf, "[UpdateParticles] Instance %d: before=%d after=%d",
                    pair.first, beforeCount, afterCount);
            LogToFile(buf);
        }
    }
}

void RenderParticles(const float* viewMatrix, const float* projMatrix, const float* cameraPos) {
    static int callCount = 0;
    callCount++;

    // DEBUG: Log every call for now
    // Only log first call to avoid spam
    static bool firstCall = true;
    if (firstCall && callCount == 1) {
        LogToFile("[RenderParticles] Render hook active");
        firstCall = false;
    }

    // Try to initialize if not already done
    if (!g_systemInitialized) {
        EnsureInitialized();
    }

    // Check if we can render
    if (!g_renderer || !g_renderer->IsInitialized()) {
        if (callCount == 1) {
            LogToFile("[RenderParticles] ERROR: Renderer not initialized!");
        }
        return;
    }

    if (!g_d3dHook || !g_d3dHook->HasDevice()) {
        if (callCount == 1) {
            LogToFile("[RenderParticles] ERROR: No device!");
        }
        return;
    }

    if (g_activeInstances.empty()) {
        return;  // No particles to render
    }

    // Render all active instances
    for (const auto& pair : g_activeInstances) {
        const ParticleSystemInstance& instance = pair.second;

        int aliveCount = instance.simulator ? instance.simulator->GetAliveCount() : 0;

        // Log each instance being rendered
        if (callCount % 60 == 1) {
            char buf[512];
            sprintf(buf, "[RenderParticles] Instance %d: pos=(%.1f,%.1f,%.1f) scale=%.1f alive=%d",
                    pair.first, instance.position.x, instance.position.y, instance.position.z,
                    instance.scale, aliveCount);
            LogToFile(buf);
        }

        if (aliveCount == 0) {
            continue;  // Skip instances with no alive particles
        }

        float emitterPos[3] = { instance.position.x, instance.position.y, instance.position.z };
        g_renderer->Render(*instance.simulator, viewMatrix, projMatrix, cameraPos, emitterPos, instance.scale);
    }
}

// ============================================================================
// Module Initialization
// ============================================================================

// Device captured callback - automatically initializes DX context and renderer
static void OnDeviceCaptured(IDirect3DDevice9* device) {
    LogToFile("[OnDeviceCaptured] ===== Device Captured Callback =====");
    LogToFile("[OnDeviceCaptured] Device captured callback invoked!");

    // Initialize DX context if not already done
    if (!g_dxContext) {
        LogToFile("[OnDeviceCaptured] Initializing DX9 context...");
        g_dxContext = std::make_unique<DX9Context>();
        if (!g_dxContext->Initialize(device)) {
            std::string error = "[OnDeviceCaptured] ERROR: Failed to initialize DX9 context: " + g_dxContext->GetLastError();
            LogToFile(error);
            return;
        }
        LogToFile("[OnDeviceCaptured] DX9 context initialized successfully!");
    }

    // Initialize renderer if not already done
    if (!g_renderer && g_dxContext->IsInitialized()) {
        LogToFile("[OnDeviceCaptured] Initializing particle renderer...");
        g_renderer = std::make_unique<DX9ParticleRenderer>();
        if (!g_renderer->Initialize(g_dxContext.get())) {
            std::string error = "[OnDeviceCaptured] ERROR: Failed to initialize renderer: " + g_renderer->GetLastError();
            LogToFile(error);
            return;
        }
        LogToFile("[OnDeviceCaptured] Particle renderer initialized successfully!");
    }

    LogToFile("[OnDeviceCaptured] ===== System Ready! =====");
}

bool InitializeParticleSystem() {
    std::cout << "[Particle System] Initializing..." << std::endl;

    // Initialize loader first (doesn't need GPU)
    if (!g_loader) {
        g_loader = std::make_unique<ParticleLoader>();
    }

    // Initialize D3D9 hook
    if (!g_d3dHook) {
        g_d3dHook = std::make_unique<D3D9Hook>();

        // Set callback for when device is captured
        g_d3dHook->SetDeviceCapturedCallback(&OnDeviceCaptured);

        if (!g_d3dHook->Initialize()) {
            std::cerr << "[Particle System] Failed to initialize D3D9 hook" << std::endl;
            return false;
        }
    }

    // If device was already captured, initialize DX context and renderer immediately
    if (g_d3dHook->HasDevice()) {
        OnDeviceCaptured(g_d3dHook->GetDevice());
    } else {
        std::cout << "[Particle System] Waiting for DirectX device to be captured..." << std::endl;
        std::cout << "[Particle System] Device will be captured automatically on next frame" << std::endl;
    }

    std::cout << "[Particle System] Initialization complete!" << std::endl;
    return true;
}

void ShutdownParticleSystem() {
    std::cout << "[Particle System] Shutting down..." << std::endl;

    // Clear all active instances
    g_activeInstances.clear();

    // Clear loaded systems
    g_loadedSystems.clear();

    // Shutdown components
    g_renderer.reset();
    g_dxContext.reset();
    g_d3dHook.reset();
    g_loader.reset();

    std::cout << "[Particle System] Shutdown complete" << std::endl;
}

// particles.RenderTest2D(screenX, screenY, size)
// Simple 2D test: render a quad in screen space (pixels)
LUA_FUNCTION(LUA_RenderTest2D) {
    LUA->CheckType(1, Type::NUMBER);
    LUA->CheckType(2, Type::NUMBER);
    LUA->CheckType(3, Type::NUMBER);

    float screenX = (float)LUA->GetNumber(1);
    float screenY = (float)LUA->GetNumber(2);
    float pixelSize = (float)LUA->GetNumber(3);

    if (!g_renderer || !g_renderer->IsInitialized()) {
        LUA->PushBool(false);
        return 1;
    }

    g_renderer->RenderTest2D(screenX, screenY, pixelSize);
    LUA->PushBool(true);
    return 1;
}

// particles.RenderTest3D(worldPos, size, viewSetup)
// Simple 3D test: render a billboard in world space
LUA_FUNCTION(LUA_RenderTest3D) {
    LUA->CheckType(1, Type::VECTOR);
    LUA->CheckType(2, Type::NUMBER);
    LUA->CheckType(3, Type::TABLE);

    // Get world position
    LUA->Push(1);
    LUA->GetField(-1, "x");
    float x = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "y");
    float y = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "z");
    float z = (float)LUA->GetNumber(-1);
    LUA->Pop(2);  // Pop z and vector

    float worldPos[3] = {x, y, z};
    float size = (float)LUA->GetNumber(2);

    // Get view setup (same as Render function)
    LUA->Push(3);
    LUA->GetField(-1, "origin");

    // NEW APPROACH: Read x, y, z fields individually instead of as userdata pointer
    // This avoids the memory corruption issue
    Vector origin;
    LUA->GetField(-1, "x");
    origin.x = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "y");
    origin.y = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "z");
    origin.z = (float)LUA->GetNumber(-1);
    LUA->Pop(2);  // Pop z and origin

    LUA->GetField(-1, "angles");
    Angle angles;
    if (LUA->IsType(-1, Type::ANGLE)) {
        LUA->PushNumber(1);
        LUA->GetTable(-2);
        angles.pitch = (float)LUA->GetNumber(-1);
        LUA->Pop();
        LUA->PushNumber(2);
        LUA->GetTable(-2);
        angles.yaw = (float)LUA->GetNumber(-1);
        LUA->Pop();
        LUA->PushNumber(3);
        LUA->GetTable(-2);
        angles.roll = (float)LUA->GetNumber(-1);
        LUA->Pop();
    }
    LUA->Pop();

    LUA->GetField(-1, "fov");
    float fov = (float)LUA->GetNumber(-1);
    LUA->Pop();

    LUA->GetField(-1, "aspect");
    float aspect = (float)LUA->GetNumber(-1);
    LUA->Pop(2);  // Pop aspect and table

    // Log origin values before building matrices
    static bool logged = false;
    if (!logged) {
        char buf[512];
        sprintf(buf, "[LUA_RenderTest3D] Origin values: (%.1f, %.1f, %.1f)",
                origin.x, origin.y, origin.z);
        LogToFile(buf);
        sprintf(buf, "[LUA_RenderTest3D] Angles: (%.1f, %.1f, %.1f), FOV: %.1f",
                angles.pitch, angles.yaw, angles.roll, fov);
        LogToFile(buf);
        logged = true;
    }

    // Build matrices
    float viewMatrix[16];
    float projMatrix[16];
    BuildViewMatrix(origin, angles, viewMatrix);
    BuildProjectionMatrix(fov, aspect, 7.0f, 30000.0f, projMatrix);

    if (!g_renderer || !g_renderer->IsInitialized()) {
        LUA->PushBool(false);
        return 1;
    }

    g_renderer->RenderTest3D(worldPos, size, viewMatrix, projMatrix);
    LUA->PushBool(true);
    return 1;
}

// particles.RenderTest3DSource(worldPos, size)
// Test 3D rendering using Source Engine's actual matrices
LUA_FUNCTION(LUA_RenderTest3DSource) {
    LUA->CheckType(1, Type::VECTOR);
    LUA->CheckType(2, Type::NUMBER);

    // Get world position
    LUA->Push(1);
    LUA->GetField(-1, "x");
    float x = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "y");
    float y = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "z");
    float z = (float)LUA->GetNumber(-1);
    LUA->Pop(2);

    float worldPos[3] = {x, y, z};
    float size = (float)LUA->GetNumber(2);

    if (!g_renderer || !g_renderer->IsInitialized()) {
        LUA->PushBool(false);
        return 1;
    }

    g_renderer->RenderTest3DSourceMatrices(worldPos, size);
    LUA->PushBool(true);
    return 1;
}

// particles.RenderTest3DProjected(worldPos, size, viewSetup)
// Project 3D world position to screen, render as 2D
LUA_FUNCTION(LUA_RenderTest3DProjected) {
    LUA->CheckType(1, Type::VECTOR);
    LUA->CheckType(2, Type::NUMBER);
    LUA->CheckType(3, Type::TABLE);

    // Get world position
    LUA->Push(1);
    LUA->GetField(-1, "x");
    float x = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "y");
    float y = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "z");
    float z = (float)LUA->GetNumber(-1);
    LUA->Pop(2);

    float worldPos[3] = {x, y, z};
    float size = (float)LUA->GetNumber(2);

    // Get view setup - read fields individually to avoid corruption
    LUA->Push(3);
    LUA->GetField(-1, "origin");
    Vector origin;
    LUA->GetField(-1, "x");
    origin.x = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "y");
    origin.y = (float)LUA->GetNumber(-1);
    LUA->Pop();
    LUA->GetField(-1, "z");
    origin.z = (float)LUA->GetNumber(-1);
    LUA->Pop(2);  // Pop z and origin

    LUA->GetField(-1, "angles");
    Angle angles = {0, 0, 0};
    if (LUA->IsType(-1, Type::ANGLE)) {
        LUA->PushNumber(1);
        LUA->GetTable(-2);
        angles.pitch = (float)LUA->GetNumber(-1);
        LUA->Pop();
        LUA->PushNumber(2);
        LUA->GetTable(-2);
        angles.yaw = (float)LUA->GetNumber(-1);
        LUA->Pop();
        LUA->PushNumber(3);
        LUA->GetTable(-2);
        angles.roll = (float)LUA->GetNumber(-1);
        LUA->Pop();
    }
    LUA->Pop();

    LUA->GetField(-1, "fov");
    float fov = (float)LUA->GetNumber(-1);
    LUA->Pop();

    LUA->GetField(-1, "aspect");
    float aspect = (float)LUA->GetNumber(-1);
    LUA->Pop(2);

    // Get screen size
    int screenWidth = 1920;  // Default, will be overridden
    int screenHeight = 1080;

    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "ScrW");
    if (LUA->IsType(-1, Type::FUNCTION)) {
        LUA->Call(0, 1);
        screenWidth = (int)LUA->GetNumber(-1);
        LUA->Pop();
    } else {
        LUA->Pop();
    }

    LUA->GetField(-1, "ScrH");
    if (LUA->IsType(-1, Type::FUNCTION)) {
        LUA->Call(0, 1);
        screenHeight = (int)LUA->GetNumber(-1);
        LUA->Pop();
    } else {
        LUA->Pop();
    }
    LUA->Pop();  // Pop global table

    // Build matrices
    float viewMatrix[16];
    float projMatrix[16];
    BuildViewMatrix(origin, angles, viewMatrix);
    BuildProjectionMatrix(fov, aspect, 7.0f, 30000.0f, projMatrix);

    if (!g_renderer || !g_renderer->IsInitialized()) {
        LUA->PushBool(false);
        return 1;
    }

    g_renderer->RenderTest3DProjected(worldPos, size, viewMatrix, projMatrix, screenWidth, screenHeight);
    LUA->PushBool(true);
    return 1;
}

// ============================================================================
// Register Lua Functions
// ============================================================================

void RegisterLuaAPI(ILuaBase* lua) {
    // Push global table
    lua->PushSpecial(SPECIAL_GLOB);

    // Create "particles" table
    lua->CreateTable();

    // Register functions
    lua->PushCFunction(LUA_LoadFromString);
    lua->SetField(-2, "LoadFromString");

    lua->PushCFunction(LUA_Spawn);
    lua->SetField(-2, "Spawn");

    lua->PushCFunction(LUA_GetTotalParticleCount);
    lua->SetField(-2, "GetTotalParticleCount");

    lua->PushCFunction(LUA_InitGPU);
    lua->SetField(-2, "InitGPU");

    lua->PushCFunction(LUA_Update);
    lua->SetField(-2, "Update");

    lua->PushCFunction(LUA_Render);
    lua->SetField(-2, "Render");

    lua->PushCFunction(LUA_RenderTest2D);
    lua->SetField(-2, "RenderTest2D");

    lua->PushCFunction(LUA_RenderTest3D);
    lua->SetField(-2, "RenderTest3D");

    lua->PushCFunction(LUA_RenderTest3DSource);
    lua->SetField(-2, "RenderTest3DSource");

    lua->PushCFunction(LUA_RenderTest3DProjected);
    lua->SetField(-2, "RenderTest3DProjected");

    // DISABLED: Test function has compilation errors
    // lua->PushCFunction(LUA_RenderTestQuad);
    // lua->SetField(-2, "RenderTestQuad");

    // Register table as "particles" global
    lua->SetField(-2, "particles");

    // Pop global table
    lua->Pop();

    std::cout << "[Particle System] Lua API registered" << std::endl;
}
