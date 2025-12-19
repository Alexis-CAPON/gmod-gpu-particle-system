#pragma once

#include <d3d9.h>
#include <string>

// Forward declarations for Source Engine interfaces
class IMaterialSystem;
class IShaderAPI;

namespace GPUParticles {

/**
 * @brief Helper to access Source Engine's material system and DirectX device
 *
 * This class interfaces with Source Engine's materialsystem.dll to obtain
 * the DirectX 9 device pointer through the official API.
 */
class MaterialSystemHelper {
public:
    MaterialSystemHelper();
    ~MaterialSystemHelper();

    /**
     * @brief Initialize and get DirectX device from Source Engine
     * @return True if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Check if initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Get the DirectX 9 device
     */
    IDirect3DDevice9* GetDevice() const { return m_device; }

    /**
     * @brief Get last error message
     */
    const std::string& GetLastError() const { return m_lastError; }

private:
    // Load Source Engine interface
    void* GetInterface(const char* moduleName, const char* interfaceName);

    // Extract device from material system
    bool ExtractDevice();

    IDirect3DDevice9* m_device;
    IMaterialSystem* m_materialSystem;
    IShaderAPI* m_shaderAPI;
    bool m_initialized;
    std::string m_lastError;
};

} // namespace GPUParticles
