#include "dx9_context.h"
#include <iostream>
#include <sstream>

namespace GPUParticles {

DX9Context::DX9Context()
    : m_device(nullptr)
    , m_initialized(false)
{
    memset(&m_caps, 0, sizeof(D3DCAPS9));
}

DX9Context::~DX9Context() {
    Shutdown();
}

bool DX9Context::Initialize(IDirect3DDevice9* device) {
    if (m_initialized) {
        return true;
    }

    std::cout << "[DX9Context] Initializing with GMod's DirectX device..." << std::endl;

    if (!device) {
        m_lastError = "DirectX device pointer is null";
        std::cerr << "[DX9Context] ERROR: " << m_lastError << std::endl;
        return false;
    }

    m_device = device;

    // Get device capabilities
    HRESULT hr = m_device->GetDeviceCaps(&m_caps);
    if (FAILED(hr)) {
        m_lastError = "Failed to get device capabilities";
        std::cerr << "[DX9Context] ERROR: " << m_lastError << std::endl;
        m_device = nullptr;
        return false;
    }

    // Query GPU information
    QueryGPUInfo();

    // Log capabilities
    std::cout << "[DX9Context] GPU Vendor: " << m_gpuVendor << std::endl;
    std::cout << "[DX9Context] GPU Device: " << m_gpuRenderer << std::endl;
    std::cout << "[DX9Context] Driver Version: " << m_driverVersion << std::endl;

    // Check shader support
    DWORD vsVersion = m_caps.VertexShaderVersion;
    DWORD psMajor = D3DSHADER_VERSION_MAJOR(m_caps.PixelShaderVersion);
    DWORD psMinor = D3DSHADER_VERSION_MINOR(m_caps.PixelShaderVersion);

    std::cout << "[DX9Context] Vertex Shader Version: "
              << D3DSHADER_VERSION_MAJOR(vsVersion) << "."
              << D3DSHADER_VERSION_MINOR(vsVersion) << std::endl;
    std::cout << "[DX9Context] Pixel Shader Version: "
              << psMajor << "." << psMinor << std::endl;

    // Check minimum requirements (Shader Model 2.0)
    if (!SupportsVertexShader(2, 0)) {
        m_lastError = "GPU does not support Vertex Shader 2.0 (required)";
        std::cerr << "[DX9Context] ERROR: " << m_lastError << std::endl;
        m_device = nullptr;
        return false;
    }

    if (!SupportsPixelShader(2, 0)) {
        m_lastError = "GPU does not support Pixel Shader 2.0 (required)";
        std::cerr << "[DX9Context] ERROR: " << m_lastError << std::endl;
        m_device = nullptr;
        return false;
    }

    m_initialized = true;
    std::cout << "[DX9Context] Initialization successful!" << std::endl;

    return true;
}

void DX9Context::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "[DX9Context] Shutting down..." << std::endl;

    // Note: We don't release m_device because GMod owns it
    m_device = nullptr;
    m_initialized = false;

    std::cout << "[DX9Context] Shutdown complete" << std::endl;
}

bool DX9Context::SupportsVertexShader(DWORD major, DWORD minor) const {
    DWORD version = D3DVS_VERSION(major, minor);
    return m_caps.VertexShaderVersion >= version;
}

bool DX9Context::SupportsPixelShader(DWORD major, DWORD minor) const {
    DWORD version = D3DPS_VERSION(major, minor);
    return m_caps.PixelShaderVersion >= version;
}

void DX9Context::QueryGPUInfo() {
    if (!m_device) {
        return;
    }

    // Get adapter identifier
    IDirect3D9* d3d9 = nullptr;
    HRESULT hr = m_device->GetDirect3D(&d3d9);
    if (SUCCEEDED(hr) && d3d9) {
        D3DADAPTER_IDENTIFIER9 identifier;
        hr = d3d9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier);
        if (SUCCEEDED(hr)) {
            m_gpuRenderer = identifier.Description;
            m_gpuVendor = identifier.DeviceName;

            // Format driver version
            std::stringstream ss;
            ss << HIWORD(identifier.DriverVersion.HighPart) << "."
               << LOWORD(identifier.DriverVersion.HighPart) << "."
               << HIWORD(identifier.DriverVersion.LowPart) << "."
               << LOWORD(identifier.DriverVersion.LowPart);
            m_driverVersion = ss.str();
        }
        d3d9->Release();
    }
}

} // namespace GPUParticles
