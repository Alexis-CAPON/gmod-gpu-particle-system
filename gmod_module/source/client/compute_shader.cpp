#include "compute_shader.h"
#include "opengl_includes.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace GPUParticles {

// ============================================================================
// ComputeShader Implementation
// ============================================================================

ComputeShader::ComputeShader()
    : m_programID(0)
{
}

ComputeShader::~ComputeShader() {
    Cleanup();
}

bool ComputeShader::LoadFromFile(const std::string& filepath) {
    std::string source = ReadFile(filepath);
    if (source.empty()) {
        m_lastError = "Failed to read shader file: " + filepath;
        return false;
    }

    return LoadFromSource(source);
}

bool ComputeShader::LoadFromSource(const std::string& source) {
    return CompileShader(source);
}

void ComputeShader::Bind() const {
    if (m_programID != 0) {
        GL_CHECK(glUseProgram(m_programID));
    }
}

void ComputeShader::Unbind() const {
    GL_CHECK(glUseProgram(0));
}

void ComputeShader::Dispatch(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) const {
    if (m_programID == 0) {
        std::cerr << "[ComputeShader] Cannot dispatch: shader not loaded" << std::endl;
        return;
    }

    GL_CHECK(glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ));
}

void ComputeShader::MemoryBarrier() const {
    GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
}

bool ComputeShader::CompileShader(const std::string& source) {
    // 1. Create shader object
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    if (shader == 0) {
        m_lastError = "Failed to create compute shader object";
        return false;
    }

    // 2. Set source
    const char* sourceCStr = source.c_str();
    GL_CHECK(glShaderSource(shader, 1, &sourceCStr, nullptr));

    // 3. Compile
    GL_CHECK(glCompileShader(shader));

    // 4. Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        m_lastError = "Compute shader compilation failed:\n" + std::string(infoLog);
        std::cerr << "[ComputeShader] " << m_lastError << std::endl;
        glDeleteShader(shader);
        return false;
    }

    // 5. Create program
    m_programID = glCreateProgram();
    if (m_programID == 0) {
        m_lastError = "Failed to create shader program";
        glDeleteShader(shader);
        return false;
    }

    GL_CHECK(glAttachShader(m_programID, shader));
    GL_CHECK(glLinkProgram(m_programID));

    // 6. Check link status
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(m_programID, 1024, nullptr, infoLog);
        m_lastError = "Compute shader linking failed:\n" + std::string(infoLog);
        std::cerr << "[ComputeShader] " << m_lastError << std::endl;
        glDeleteProgram(m_programID);
        glDeleteShader(shader);
        m_programID = 0;
        return false;
    }

    // 7. Delete shader (no longer needed after linking)
    glDeleteShader(shader);

    std::cout << "[ComputeShader] Compiled and linked successfully" << std::endl;
    return true;
}

void ComputeShader::SetInt(const std::string& name, int value) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        GL_CHECK(glUniform1i(location, value));
    }
}

void ComputeShader::SetFloat(const std::string& name, float value) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        GL_CHECK(glUniform1f(location, value));
    }
}

void ComputeShader::SetVec2(const std::string& name, float x, float y) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        GL_CHECK(glUniform2f(location, x, y));
    }
}

void ComputeShader::SetVec3(const std::string& name, float x, float y, float z) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        GL_CHECK(glUniform3f(location, x, y, z));
    }
}

void ComputeShader::SetVec4(const std::string& name, float x, float y, float z, float w) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        GL_CHECK(glUniform4f(location, x, y, z, w));
    }
}

void ComputeShader::SetMat4(const std::string& name, const float* matrix) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        GL_CHECK(glUniformMatrix4fv(location, 1, GL_FALSE, matrix));
    }
}

void ComputeShader::BindStorageBuffer(unsigned int bindingPoint, unsigned int bufferID) const {
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, bufferID));
}

int ComputeShader::GetUniformLocation(const std::string& name) {
    // Check cache
    auto it = m_uniformLocations.find(name);
    if (it != m_uniformLocations.end()) {
        return it->second;
    }

    // Query location
    GLint location = glGetUniformLocation(m_programID, name.c_str());

    // Cache it (even if -1, so we don't query again)
    m_uniformLocations[name] = location;

    if (location == -1) {
        // Uniform not found - this might be okay if it's unused in shader
        // Don't spam warnings for every frame
    }

    return location;
}

std::string ComputeShader::ReadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void ComputeShader::Cleanup() {
    if (m_programID != 0) {
        GL_CHECK(glDeleteProgram(m_programID));
        m_programID = 0;
    }
    m_uniformLocations.clear();
}

// ============================================================================
// ShaderProgram Implementation
// ============================================================================

ShaderProgram::ShaderProgram()
    : m_programID(0)
{
}

ShaderProgram::~ShaderProgram() {
    Cleanup();
}

bool ShaderProgram::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = ReadFile(vertexPath);
    if (vertexSource.empty()) {
        m_lastError = "Failed to read vertex shader: " + vertexPath;
        return false;
    }

    std::string fragmentSource = ReadFile(fragmentPath);
    if (fragmentSource.empty()) {
        m_lastError = "Failed to read fragment shader: " + fragmentPath;
        return false;
    }

    // Compile vertex shader
    // unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    // if (vertexShader == 0) {
    //     return false;
    // }

    // Compile fragment shader
    // unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    // if (fragmentShader == 0) {
    //     glDeleteShader(vertexShader);
    //     return false;
    // }

    // Link program
    // if (!LinkProgram(vertexShader, fragmentShader)) {
    //     glDeleteShader(vertexShader);
    //     glDeleteShader(fragmentShader);
    //     return false;
    // }

    // glDeleteShader(vertexShader);
    // glDeleteShader(fragmentShader);

    // Stub
    m_programID = 2;
    std::cout << "[ShaderProgram] Loaded successfully (stub)" << std::endl;
    return true;
}

void ShaderProgram::Bind() const {
    if (m_programID != 0) {
        // glUseProgram(m_programID);
    }
}

void ShaderProgram::Unbind() const {
    // glUseProgram(0);
}

unsigned int ShaderProgram::CompileShader(unsigned int type, const std::string& source) {
    // unsigned int shader = glCreateShader(type);
    // const char* sourceCStr = source.c_str();
    // glShaderSource(shader, 1, &sourceCStr, nullptr);
    // glCompileShader(shader);

    // int success;
    // glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    // if (!success) {
    //     char infoLog[1024];
    //     glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
    //     m_lastError = "Shader compilation failed: " + std::string(infoLog);
    //     glDeleteShader(shader);
    //     return 0;
    // }

    // return shader;
    return 1; // Stub
}

bool ShaderProgram::LinkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    // m_programID = glCreateProgram();
    // glAttachShader(m_programID, vertexShader);
    // glAttachShader(m_programID, fragmentShader);
    // glLinkProgram(m_programID);

    // int success;
    // glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    // if (!success) {
    //     char infoLog[1024];
    //     glGetProgramInfoLog(m_programID, 1024, nullptr, infoLog);
    //     m_lastError = "Shader linking failed: " + std::string(infoLog);
    //     glDeleteProgram(m_programID);
    //     m_programID = 0;
    //     return false;
    // }

    return true;
}

void ShaderProgram::SetInt(const std::string& name, int value) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        // glUniform1i(location, value);
    }
}

void ShaderProgram::SetFloat(const std::string& name, float value) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        // glUniform1f(location, value);
    }
}

void ShaderProgram::SetVec2(const std::string& name, float x, float y) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        // glUniform2f(location, x, y);
    }
}

void ShaderProgram::SetVec3(const std::string& name, float x, float y, float z) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        // glUniform3f(location, x, y, z);
    }
}

void ShaderProgram::SetVec4(const std::string& name, float x, float y, float z, float w) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        // glUniform4f(location, x, y, z, w);
    }
}

void ShaderProgram::SetMat4(const std::string& name, const float* matrix) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        // glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
    }
}

int ShaderProgram::GetUniformLocation(const std::string& name) {
    auto it = m_uniformLocations.find(name);
    if (it != m_uniformLocations.end()) {
        return it->second;
    }

    // int location = glGetUniformLocation(m_programID, name.c_str());
    int location = 0; // Stub
    m_uniformLocations[name] = location;
    return location;
}

std::string ShaderProgram::ReadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void ShaderProgram::Cleanup() {
    if (m_programID != 0) {
        // glDeleteProgram(m_programID);
        m_programID = 0;
    }
}

} // namespace GPUParticles
