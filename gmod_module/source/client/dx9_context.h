#pragma once

#include <d3d9.h>
#include <string>

namespace GPUParticles {

/**
 * @brief DirectX 9 context wrapper for GMod integration
 *
 * This class wraps GMod's existing DirectX 9 device and provides
 * utility functions for particle rendering.
 */
class DX9Context {
public:
    DX9Context();
    ~DX9Context();

    /**
     * @brief Initialize with GMod's DirectX device
     * @param device Pointer to GMod's IDirect3DDevice9
     * @return True if successful
     */
    bool Initialize(IDirect3DDevice9* device);

    /**
     * @brief Shutdown and release resources
     */
    void Shutdown();

    /**
     * @brief Check if context is initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Get the DirectX device
     */
    IDirect3DDevice9* GetDevice() const { return m_device; }

    /**
     * @brief Get device capabilities
     */
    const D3DCAPS9& GetCaps() const { return m_caps; }

    /**
     * @brief Check if vertex shader version is supported
     */
    bool SupportsVertexShader(DWORD major, DWORD minor) const;

    /**
     * @brief Check if pixel shader version is supported
     */
    bool SupportsPixelShader(DWORD major, DWORD minor) const;

    /**
     * @brief Get last error message
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief Query GPU information
     */
    void QueryGPUInfo();

    // GPU Information
    const std::string& GetGPUVendor() const { return m_gpuVendor; }
    const std::string& GetGPURenderer() const { return m_gpuRenderer; }
    const std::string& GetDriverVersion() const { return m_driverVersion; }

private:
    IDirect3DDevice9* m_device;
    D3DCAPS9 m_caps;
    bool m_initialized;
    std::string m_lastError;

    // GPU Info
    std::string m_gpuVendor;
    std::string m_gpuRenderer;
    std::string m_driverVersion;
};

} // namespace GPUParticles
