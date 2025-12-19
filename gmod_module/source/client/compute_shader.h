#pragma once

#include <string>
#include <unordered_map>

namespace GPUParticles {

/**
 * @brief Manages a single compute shader
 *
 * Loads, compiles, and manages compute shaders.
 * Provides uniform binding and dispatch capabilities.
 */
class ComputeShader {
public:
    ComputeShader();
    ~ComputeShader();

    /**
     * @brief Load and compile compute shader from file
     * @param filepath Path to .comp shader file
     * @return true if successful
     */
    bool LoadFromFile(const std::string& filepath);

    /**
     * @brief Load and compile compute shader from source string
     * @param source GLSL compute shader source code
     * @return true if successful
     */
    bool LoadFromSource(const std::string& source);

    /**
     * @brief Bind this shader for use
     */
    void Bind() const;

    /**
     * @brief Unbind shader
     */
    void Unbind() const;

    /**
     * @brief Dispatch compute shader
     * @param numGroupsX Number of work groups in X dimension
     * @param numGroupsY Number of work groups in Y dimension
     * @param numGroupsZ Number of work groups in Z dimension
     */
    void Dispatch(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) const;

    /**
     * @brief Wait for compute operations to complete
     */
    void MemoryBarrier() const;

    /**
     * @brief Check if shader is valid
     */
    bool IsValid() const { return m_programID != 0; }

    /**
     * @brief Get OpenGL program ID
     */
    unsigned int GetProgramID() const { return m_programID; }

    /**
     * @brief Get last compilation error
     */
    const std::string& GetLastError() const { return m_lastError; }

    // Uniform setters
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, float x, float y);
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetVec4(const std::string& name, float x, float y, float z, float w);
    void SetMat4(const std::string& name, const float* matrix);

    /**
     * @brief Bind a shader storage buffer to a binding point
     * @param bindingPoint Binding point index (matches layout in shader)
     * @param bufferID OpenGL buffer object ID
     */
    void BindStorageBuffer(unsigned int bindingPoint, unsigned int bufferID) const;

    /**
     * @brief Cleanup and delete shader
     */
    void Cleanup();

private:
    unsigned int m_programID;
    std::string m_lastError;
    std::unordered_map<std::string, int> m_uniformLocations;

    bool CompileShader(const std::string& source);
    int GetUniformLocation(const std::string& name);
    std::string ReadFile(const std::string& filepath);
};

/**
 * @brief Manages a regular shader program (vertex + fragment)
 */
class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    /**
     * @brief Load and compile vertex and fragment shaders
     * @param vertexPath Path to vertex shader
     * @param fragmentPath Path to fragment shader
     * @return true if successful
     */
    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    /**
     * @brief Bind this shader program
     */
    void Bind() const;

    /**
     * @brief Unbind shader program
     */
    void Unbind() const;

    /**
     * @brief Check if program is valid
     */
    bool IsValid() const { return m_programID != 0; }

    /**
     * @brief Get OpenGL program ID
     */
    unsigned int GetProgramID() const { return m_programID; }

    /**
     * @brief Get last compilation/link error
     */
    const std::string& GetLastError() const { return m_lastError; }

    // Uniform setters (same as ComputeShader)
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, float x, float y);
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetVec4(const std::string& name, float x, float y, float z, float w);
    void SetMat4(const std::string& name, const float* matrix);

    /**
     * @brief Cleanup and delete program
     */
    void Cleanup();

private:
    unsigned int m_programID;
    std::string m_lastError;
    std::unordered_map<std::string, int> m_uniformLocations;

    unsigned int CompileShader(unsigned int type, const std::string& source);
    bool LinkProgram(unsigned int vertexShader, unsigned int fragmentShader);
    int GetUniformLocation(const std::string& name);
    std::string ReadFile(const std::string& filepath);
};

} // namespace GPUParticles
