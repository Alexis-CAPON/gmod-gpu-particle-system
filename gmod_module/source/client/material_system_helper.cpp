#include "material_system_helper.h"
#include <Windows.h>
#include <iostream>

// Minimal Source Engine interface definitions
typedef void* (*CreateInterfaceFn)(const char* name, int* returnCode);

// Interface versions for GMod (these may need adjustment)
#define MATERIAL_SYSTEM_INTERFACE_VERSION "VMaterialSystem080"
#define SHADER_API_INTERFACE_VERSION "ShaderApi030"

namespace GPUParticles {

MaterialSystemHelper::MaterialSystemHelper()
    : m_device(nullptr)
    , m_materialSystem(nullptr)
    , m_shaderAPI(nullptr)
    , m_initialized(false)
{
}

MaterialSystemHelper::~MaterialSystemHelper() {
    Shutdown();
}

bool MaterialSystemHelper::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "[MaterialSystemHelper] Initializing..." << std::endl;

    // Get IMaterialSystem interface
    m_materialSystem = static_cast<IMaterialSystem*>(
        GetInterface("materialsystem.dll", MATERIAL_SYSTEM_INTERFACE_VERSION)
    );

    if (!m_materialSystem) {
        m_lastError = "Failed to get IMaterialSystem interface";
        std::cerr << "[MaterialSystemHelper] ERROR: " << m_lastError << std::endl;
        std::cerr << "[MaterialSystemHelper] Tried interface: " << MATERIAL_SYSTEM_INTERFACE_VERSION << std::endl;
        return false;
    }

    std::cout << "[MaterialSystemHelper] Got IMaterialSystem interface" << std::endl;

    // Try to extract DirectX device
    if (!ExtractDevice()) {
        m_lastError = "Failed to extract DirectX device from material system";
        std::cerr << "[MaterialSystemHelper] ERROR: " << m_lastError << std::endl;
        return false;
    }

    if (!m_device) {
        m_lastError = "DirectX device is null";
        std::cerr << "[MaterialSystemHelper] ERROR: " << m_lastError << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "[MaterialSystemHelper] Successfully obtained DirectX device!" << std::endl;

    return true;
}

void MaterialSystemHelper::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "[MaterialSystemHelper] Shutting down..." << std::endl;

    // Don't release m_device - Source Engine owns it
    m_device = nullptr;
    m_materialSystem = nullptr;
    m_shaderAPI = nullptr;
    m_initialized = false;
}

void* MaterialSystemHelper::GetInterface(const char* moduleName, const char* interfaceName) {
    // Load the module
    HMODULE module = GetModuleHandleA(moduleName);
    if (!module) {
        std::cerr << "[MaterialSystemHelper] Failed to get module: " << moduleName << std::endl;
        return nullptr;
    }

    // Get the CreateInterface function
    CreateInterfaceFn createInterface = reinterpret_cast<CreateInterfaceFn>(
        GetProcAddress(module, "CreateInterface")
    );

    if (!createInterface) {
        std::cerr << "[MaterialSystemHelper] Failed to get CreateInterface from " << moduleName << std::endl;
        return nullptr;
    }

    // Create the interface
    int returnCode = 0;
    void* iface = createInterface(interfaceName, &returnCode);

    if (!iface || returnCode != 0) {
        std::cerr << "[MaterialSystemHelper] CreateInterface failed for " << interfaceName
                  << " (return code: " << returnCode << ")" << std::endl;
        return nullptr;
    }

    std::cout << "[MaterialSystemHelper] Successfully got interface: " << interfaceName << std::endl;
    return iface;
}

bool MaterialSystemHelper::ExtractDevice() {
    // Method 1: Try to get through shader API
    // Note: This requires knowing the vtable layout of IMaterialSystem
    // which can vary by Source Engine version

    // For now, we'll use a pattern scan approach
    // This is more robust but requires finding the device in memory

    // The device is typically stored as a member variable in the shader API
    // We need to access it through the vtable

    // IMPORTANT: This is a simplified version
    // In production, you'd need to:
    // 1. Get IShaderAPI from IMaterialSystem
    // 2. Access the D3D device through the shader API's vtable
    // 3. Handle different Source Engine versions

    // Placeholder implementation:
    // We need to reverse engineer the exact offset/method
    // For GMod, this typically involves:

    std::cout << "[MaterialSystemHelper] Attempting to extract D3D device..." << std::endl;

    // Try to get the device through multiple methods
    // Method 1: Check if IMaterialSystem has GetD3DDevice (some versions do)
    // Method 2: Get through IShaderAPI
    // Method 3: Pattern scan in shaderapidx9.dll

    // For this implementation, we'll document what needs to be done:
    std::cerr << "[MaterialSystemHelper] WARNING: Device extraction not yet implemented!" << std::endl;
    std::cerr << "[MaterialSystemHelper] This requires reverse engineering IMaterialSystem vtable" << std::endl;
    std::cerr << "[MaterialSystemHelper] Possible approaches:" << std::endl;
    std::cerr << "[MaterialSystemHelper]   1. Call IMaterialSystem::GetRenderContext()->GetD3DDevice()" << std::endl;
    std::cerr << "[MaterialSystemHelper]   2. Pattern scan for device pointer in shaderapidx9.dll" << std::endl;
    std::cerr << "[MaterialSystemHelper]   3. Hook D3D9 Present() function" << std::endl;

    m_lastError = "Device extraction not implemented - requires vtable analysis";
    return false;
}

} // namespace GPUParticles
