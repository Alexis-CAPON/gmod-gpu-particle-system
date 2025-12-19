#pragma once

#include "../../include/opengl_includes.h"
#include <string>

namespace GPUParticles {

/**
 * @brief Manages OpenGL context and compute shader support
 *
 * This class initializes OpenGL from GMod's existing context,
 * verifies compute shader support, and provides debug utilities.
 */
class GPUContext {
public:
    GPUContext();
    ~GPUContext();

    /**
     * @brief Initialize the GPU context
     * @return true if successful, false otherwise
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Check if the context is initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Check if compute shaders are supported
     */
    bool SupportsComputeShaders() const { return m_supportsCompute; }

    /**
     * @brief Get OpenGL version string
     */
    const std::string& GetGLVersion() const { return m_glVersion; }

    /**
     * @brief Get GPU vendor string
     */
    const std::string& GetGPUVendor() const { return m_gpuVendor; }

    /**
     * @brief Get GPU renderer string
     */
    const std::string& GetGPURenderer() const { return m_gpuRenderer; }

    /**
     * @brief Get GLSL version string
     */
    const std::string& GetGLSLVersion() const { return m_glslVersion; }

    /**
     * @brief Get maximum compute work group count
     */
    void GetMaxWorkGroupCount(int& x, int& y, int& z) const;

    /**
     * @brief Get maximum compute work group size
     */
    void GetMaxWorkGroupSize(int& x, int& y, int& z) const;

    /**
     * @brief Get maximum compute work group invocations
     */
    int GetMaxWorkGroupInvocations() const { return m_maxWorkGroupInvocations; }

    /**
     * @brief Get last error message
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief Enable/disable debug output
     */
    void SetDebugOutput(bool enabled);

private:
    bool m_initialized;
    bool m_supportsCompute;

    std::string m_glVersion;
    std::string m_gpuVendor;
    std::string m_gpuRenderer;
    std::string m_glslVersion;
    std::string m_lastError;

    int m_maxWorkGroupCount[3];
    int m_maxWorkGroupSize[3];
    int m_maxWorkGroupInvocations;

    bool InitializeGLEW();
    bool CheckComputeShaderSupport();
    void QueryGPUInfo();
    void SetupDebugCallback();

    // OpenGL debug callback
    static void APIENTRY GLDebugCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
    );
};

} // namespace GPUParticles
