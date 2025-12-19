#include "d3d9_hook.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include "MinHook.h"

namespace GPUParticles {

// File logger for debugging (GMod doesn't capture std::cout)
void LogToFile(const std::string& msg) {
    static std::ofstream logFile;
    static bool logOpened = false;

    if (!logOpened) {
        // Open log file in GMod directory
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string exePath(path);
        size_t lastSlash = exePath.find_last_of("\\/");
        std::string logPath = exePath.substr(0, lastSlash) + "\\garrysmod\\d3d9hook_debug.log";

        logFile.open(logPath, std::ios::out | std::ios::app);
        if (logFile.is_open()) {
            time_t now = time(nullptr);
            logFile << "\n\n========== New Session: " << ctime(&now) << "==========\n";
            logFile.flush();
        }
        logOpened = true;
    }

    if (logFile.is_open()) {
        logFile << msg << std::endl;
        logFile.flush(); // Immediate flush for crash debugging
    }

    // Also output to console (might work in some scenarios)
    std::cout << msg << std::endl;
}

// Static members
D3D9Hook::Present_t D3D9Hook::s_originalPresent = nullptr;
D3D9Hook::EndScene_t D3D9Hook::s_originalEndScene = nullptr;
D3D9Hook::Reset_t D3D9Hook::s_originalReset = nullptr;
IDirect3DDevice9* D3D9Hook::s_capturedDevice = nullptr;
bool D3D9Hook::s_deviceCaptured = false;
D3D9Hook::DeviceCapturedCallback D3D9Hook::s_deviceCapturedCallback = nullptr;
D3D9Hook* D3D9Hook::s_instance = nullptr;

D3D9Hook::D3D9Hook()
    : m_device(nullptr)
    , m_initialized(false)
{
    s_instance = this;
}

D3D9Hook::~D3D9Hook() {
    Shutdown();
}

bool D3D9Hook::Initialize() {
    if (m_initialized) {
        return true;
    }

    LogToFile("[D3D9Hook] Initializing DirectX 9 hook...");

    // Install the hook
    if (!InstallHook()) {
        m_lastError = "Failed to install EndScene hook";
        LogToFile("[D3D9Hook] ERROR: " + m_lastError);
        return false;
    }

    LogToFile("[D3D9Hook] Hook installed successfully");
    LogToFile("[D3D9Hook] Waiting for device to be captured...");

    m_initialized = true;
    return true;
}

void D3D9Hook::Shutdown() {
    if (!m_initialized) {
        return;
    }

    LogToFile("[D3D9Hook] Shutting down...");

    UninstallHook();

    // Don't release device - GMod owns it
    m_device = nullptr;
    s_capturedDevice = nullptr;
    s_deviceCaptured = false;
    s_deviceCapturedCallback = nullptr;
    if (s_instance == this) {
        s_instance = nullptr;
    }
    m_initialized = false;
}

bool D3D9Hook::InstallHook() {
    LogToFile("[D3D9Hook] ========== MinHook Installation ==========");
    LogToFile("[D3D9Hook] Initializing MinHook library...");

    // Initialize MinHook
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        char errorBuf[256];
        sprintf(errorBuf, "[D3D9Hook] MH_Initialize failed with status: %d", status);
        LogToFile(errorBuf);
        m_lastError = "MinHook initialization failed";
        return false;
    }
    LogToFile("[D3D9Hook] MinHook initialized successfully!");

    LogToFile("[D3D9Hook] Creating temporary D3D9 device to get function addresses...");

    // Create a temporary D3D9 device to get function addresses from VTable
    HWND tempWindow = CreateWindowA("BUTTON", "Temp", WS_SYSMENU | WS_MINIMIZEBOX,
                                    CW_USEDEFAULT, CW_USEDEFAULT, 300, 300,
                                    NULL, NULL, NULL, NULL);

    if (!tempWindow) {
        MH_Uninitialize();
        m_lastError = "Failed to create temporary window";
        return false;
    }

    // Create D3D9
    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9) {
        DestroyWindow(tempWindow);
        MH_Uninitialize();
        m_lastError = "Failed to create D3D9 interface";
        return false;
    }

    // Setup present parameters
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.hDeviceWindow = tempWindow;

    // Create device
    IDirect3DDevice9* tempDevice = nullptr;
    HRESULT hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                    tempWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                    &d3dpp, &tempDevice);

    if (FAILED(hr) || !tempDevice) {
        d3d9->Release();
        DestroyWindow(tempWindow);
        MH_Uninitialize();
        m_lastError = "Failed to create temporary D3D9 device";
        return false;
    }

    LogToFile("[D3D9Hook] Temporary device created, extracting function addresses from VTable...");

    // Get VTable to extract function addresses
    void** vtable = GetVTable(tempDevice);
    if (!vtable) {
        tempDevice->Release();
        d3d9->Release();
        DestroyWindow(tempWindow);
        MH_Uninitialize();
        m_lastError = "Failed to get device VTable";
        return false;
    }

    // VTable indices for IDirect3DDevice9
    const int PRESENT_INDEX = 17;
    const int ENDSCENE_INDEX = 42;
    const int RESET_INDEX = 16;

    // Extract function addresses (these point to actual code in d3d9.dll)
    void* presentTarget = vtable[PRESENT_INDEX];
    void* endSceneTarget = vtable[ENDSCENE_INDEX];
    void* resetTarget = vtable[RESET_INDEX];

    char hexBuf[256];
    sprintf(hexBuf, "[D3D9Hook] VTable address: 0x%p", vtable);
    LogToFile(hexBuf);
    sprintf(hexBuf, "[D3D9Hook] Present target address: 0x%p", presentTarget);
    LogToFile(hexBuf);
    sprintf(hexBuf, "[D3D9Hook] EndScene target address: 0x%p", endSceneTarget);
    LogToFile(hexBuf);
    sprintf(hexBuf, "[D3D9Hook] Reset target address: 0x%p", resetTarget);
    LogToFile(hexBuf);

    // Now use MinHook to hook the actual function code (not VTable entries)
    LogToFile("[D3D9Hook] Creating MinHook for Present...");
    status = MH_CreateHook(presentTarget, &PresentHook, reinterpret_cast<LPVOID*>(&s_originalPresent));
    if (status != MH_OK) {
        sprintf(hexBuf, "[D3D9Hook] MH_CreateHook(Present) failed with status: %d", status);
        LogToFile(hexBuf);
        tempDevice->Release();
        d3d9->Release();
        DestroyWindow(tempWindow);
        MH_Uninitialize();
        m_lastError = "Failed to create Present hook";
        return false;
    }
    LogToFile("[D3D9Hook] Present hook created successfully!");

    LogToFile("[D3D9Hook] Creating MinHook for EndScene...");
    status = MH_CreateHook(endSceneTarget, &EndSceneHook, reinterpret_cast<LPVOID*>(&s_originalEndScene));
    if (status != MH_OK) {
        sprintf(hexBuf, "[D3D9Hook] MH_CreateHook(EndScene) failed with status: %d", status);
        LogToFile(hexBuf);
        MH_RemoveHook(presentTarget);
        tempDevice->Release();
        d3d9->Release();
        DestroyWindow(tempWindow);
        MH_Uninitialize();
        m_lastError = "Failed to create EndScene hook";
        return false;
    }
    LogToFile("[D3D9Hook] EndScene hook created successfully!");

    LogToFile("[D3D9Hook] Creating MinHook for Reset...");
    status = MH_CreateHook(resetTarget, &ResetHook, reinterpret_cast<LPVOID*>(&s_originalReset));
    if (status != MH_OK) {
        sprintf(hexBuf, "[D3D9Hook] MH_CreateHook(Reset) failed with status: %d", status);
        LogToFile(hexBuf);
        MH_RemoveHook(presentTarget);
        MH_RemoveHook(endSceneTarget);
        tempDevice->Release();
        d3d9->Release();
        DestroyWindow(tempWindow);
        MH_Uninitialize();
        m_lastError = "Failed to create Reset hook";
        return false;
    }
    LogToFile("[D3D9Hook] Reset hook created successfully!");

    // Enable all hooks
    LogToFile("[D3D9Hook] Enabling all hooks...");
    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        sprintf(hexBuf, "[D3D9Hook] MH_EnableHook failed with status: %d", status);
        LogToFile(hexBuf);
        MH_RemoveHook(presentTarget);
        MH_RemoveHook(endSceneTarget);
        MH_RemoveHook(resetTarget);
        tempDevice->Release();
        d3d9->Release();
        DestroyWindow(tempWindow);
        MH_Uninitialize();
        m_lastError = "Failed to enable hooks";
        return false;
    }
    LogToFile("[D3D9Hook] All hooks enabled successfully!");

    // Cleanup temporary resources
    tempDevice->Release();
    d3d9->Release();
    DestroyWindow(tempWindow);

    LogToFile("[D3D9Hook] ========== Hooks installed successfully! ==========");
    LogToFile("[D3D9Hook] Waiting for GMod to call Present/EndScene...");
    return true;
}

bool D3D9Hook::UninstallHook() {
    LogToFile("[D3D9Hook] Uninstalling MinHook hooks...");

    // Disable all hooks
    MH_STATUS status = MH_DisableHook(MH_ALL_HOOKS);
    if (status != MH_OK && status != MH_ERROR_NOT_INITIALIZED) {
        char hexBuf[256];
        sprintf(hexBuf, "[D3D9Hook] MH_DisableHook failed with status: %d", status);
        LogToFile(hexBuf);
    }

    // Uninitialize MinHook
    status = MH_Uninitialize();
    if (status != MH_OK && status != MH_ERROR_NOT_INITIALIZED) {
        char hexBuf[256];
        sprintf(hexBuf, "[D3D9Hook] MH_Uninitialize failed with status: %d", status);
        LogToFile(hexBuf);
        return false;
    }

    s_originalPresent = nullptr;
    s_originalEndScene = nullptr;
    s_originalReset = nullptr;

    LogToFile("[D3D9Hook] MinHook uninstalled successfully");
    return true;
}

void D3D9Hook::OnDeviceCaptured(IDirect3DDevice9* device) {
    if (s_deviceCaptured || !device) {
        return;
    }

    s_deviceCaptured = true;
    s_capturedDevice = device;

    char hexBuf[256];
    LogToFile("[D3D9Hook] ===== Device Captured! =====");
    sprintf(hexBuf, "[D3D9Hook] Device pointer: 0x%p", device);
    LogToFile(hexBuf);

    // Update instance
    if (s_instance) {
        s_instance->m_device = device;
    }

    // Invoke callback if set
    if (s_deviceCapturedCallback) {
        LogToFile("[D3D9Hook] Invoking device captured callback...");
        s_deviceCapturedCallback(device);
    }
}

HRESULT WINAPI D3D9Hook::PresentHook(IDirect3DDevice9* device, CONST RECT* pSourceRect,
                                      CONST RECT* pDestRect, HWND hDestWindowOverride,
                                      CONST RGNDATA* pDirtyRegion) {
    // Debug: Print that hook was called
    static bool firstCall = true;
    if (firstCall) {
        char hexBuf[256];
        sprintf(hexBuf, "[D3D9Hook] *** PresentHook CALLED! Device: 0x%p ***", device);
        LogToFile(hexBuf);
        firstCall = false;
    }

    // Capture device on first call
    if (!s_deviceCaptured && device && s_instance) {
        s_instance->OnDeviceCaptured(device);
    }

    // Call original Present
    if (s_originalPresent) {
        return s_originalPresent(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }

    return D3D_OK;
}

HRESULT WINAPI D3D9Hook::EndSceneHook(IDirect3DDevice9* device) {
    // Debug: Print that hook was called
    static bool firstCall = true;
    if (firstCall) {
        char hexBuf[256];
        sprintf(hexBuf, "[D3D9Hook] *** EndSceneHook CALLED! Device: 0x%p ***", device);
        LogToFile(hexBuf);
        firstCall = false;
    }

    // Capture device on first call
    if (!s_deviceCaptured && device && s_instance) {
        s_instance->OnDeviceCaptured(device);
    }

    // Call original EndScene
    if (s_originalEndScene) {
        return s_originalEndScene(device);
    }

    return D3D_OK;
}

HRESULT WINAPI D3D9Hook::ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    LogToFile("[D3D9Hook] Reset called - device may be lost/recreated");

    // Device is being reset - we need to handle this
    // Resources may need to be recreated after reset

    // Call original Reset
    HRESULT hr = D3D_OK;
    if (s_originalReset) {
        hr = s_originalReset(device, pPresentationParameters);
    }

    // After reset, recapture device if successful
    if (SUCCEEDED(hr) && device) {
        LogToFile("[D3D9Hook] Device reset successful, updating device pointer");
        s_capturedDevice = device;
        if (s_instance) {
            s_instance->m_device = device;
        }
    }

    return hr;
}

void** D3D9Hook::GetVTable(void* instance) {
    // The first 4/8 bytes of any C++ object with virtual functions
    // points to its VTable
    return *reinterpret_cast<void***>(instance);
}

bool D3D9Hook::HookVTableFunction(void** vtable, int index, void* hookFunc, void** originalFunc) {
    if (!vtable || !hookFunc) {
        LogToFile("[D3D9Hook] HookVTableFunction: Invalid parameters");
        return false;
    }

    char hexBuf[256]; sprintf(hexBuf, "[D3D9Hook] Attempting to hook index %d at address 0x%p", index
              , &vtable[index]); LogToFile(hexBuf);

    // Save original function
    if (originalFunc) {
        *originalFunc = vtable[index];
        sprintf(hexBuf, "[D3D9Hook] Original function at index %d: 0x%p", index
                  , vtable[index]); LogToFile(hexBuf);
    }

    // Change memory protection to write
    DWORD oldProtect;
    if (!VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        sprintf(hexBuf, "[D3D9Hook] Failed to change memory protection. Error code: %d", GetLastError());
        LogToFile(hexBuf);
        sprintf(hexBuf, "[D3D9Hook] VTable address: 0x%p", &vtable[index]);
        LogToFile(hexBuf);
        return false;
    }

    sprintf(hexBuf, "[D3D9Hook] Memory protection changed successfully (old: %d)", oldProtect); LogToFile(hexBuf);

    // Replace with hook
    vtable[index] = hookFunc;

    LogToFile("[D3D9Hook] VTable entry replaced with hook function");

    // Restore memory protection
    if (!VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect)) {
        LogToFile("[D3D9Hook] WARNING: Failed to restore memory protection");
    }

    return true;
}

} // namespace GPUParticles
