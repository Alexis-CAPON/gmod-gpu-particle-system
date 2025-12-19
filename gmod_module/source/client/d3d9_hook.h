#pragma once

#include <d3d9.h>
#include <string>

namespace GPUParticles {

/**
 * @brief D3D9 EndScene hook to capture the device pointer
 *
 * This hooks the D3D9 EndScene function to obtain the IDirect3DDevice9 pointer
 * from GMod's rendering. This is a common technique used in Source Engine mods.
 */
class D3D9Hook {
public:
    D3D9Hook();
    ~D3D9Hook();

    /**
     * @brief Initialize the hook
     * @return True if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and unhook
     */
    void Shutdown();

    /**
     * @brief Check if device was captured
     */
    bool HasDevice() const { return m_device != nullptr; }

    /**
     * @brief Get the captured DirectX device
     */
    IDirect3DDevice9* GetDevice() const { return m_device; }

    /**
     * @brief Get last error
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief Set callback for when device is captured
     */
    using DeviceCapturedCallback = void(*)(IDirect3DDevice9*);
    void SetDeviceCapturedCallback(DeviceCapturedCallback callback) {
        s_deviceCapturedCallback = callback;
    }

private:
    // Hook functions to capture device
    static HRESULT WINAPI PresentHook(IDirect3DDevice9* device, CONST RECT* pSourceRect,
                                      CONST RECT* pDestRect, HWND hDestWindowOverride,
                                      CONST RGNDATA* pDirtyRegion);
    static HRESULT WINAPI EndSceneHook(IDirect3DDevice9* device);
    static HRESULT WINAPI ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);

    // Original function pointers
    typedef HRESULT(WINAPI* Present_t)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
    typedef HRESULT(WINAPI* EndScene_t)(IDirect3DDevice9*);
    typedef HRESULT(WINAPI* Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

    static Present_t s_originalPresent;
    static EndScene_t s_originalEndScene;
    static Reset_t s_originalReset;

    // Device captured from hook
    static IDirect3DDevice9* s_capturedDevice;
    static bool s_deviceCaptured;

    // Instance data
    IDirect3DDevice9* m_device;
    bool m_initialized;
    std::string m_lastError;

    // Callback
    static DeviceCapturedCallback s_deviceCapturedCallback;
    static D3D9Hook* s_instance;

    // Hook installation
    bool InstallHook();
    bool UninstallHook();
    void OnDeviceCaptured(IDirect3DDevice9* device);

    // VTable manipulation helpers
    void** GetVTable(void* instance);
    bool HookVTableFunction(void** vtable, int index, void* hookFunc, void** originalFunc);
};

} // namespace GPUParticles
