#include "gpu_context.h"
#include "opengl_includes.h"
#include <iostream>
#include <cstring>

namespace GPUParticles {

GPUContext::GPUContext()
    : m_initialized(false)
    , m_supportsCompute(false)
    , m_maxWorkGroupInvocations(0)
{
    m_maxWorkGroupCount[0] = 0;
    m_maxWorkGroupCount[1] = 0;
    m_maxWorkGroupCount[2] = 0;
    m_maxWorkGroupSize[0] = 0;
    m_maxWorkGroupSize[1] = 0;
    m_maxWorkGroupSize[2] = 0;
}

GPUContext::~GPUContext() {
    Shutdown();
}

bool GPUContext::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "[GPUContext] Initializing..." << std::endl;

    // Initialize GLEW (or glad, depending on your choice)
    if (!InitializeGLEW()) {
        m_lastError = "Failed to initialize GLEW";
        return false;
    }

    // Query GPU information
    QueryGPUInfo();

    std::cout << "[GPUContext] OpenGL Version: " << m_glVersion << std::endl;
    std::cout << "[GPUContext] GPU Vendor: " << m_gpuVendor << std::endl;
    std::cout << "[GPUContext] GPU Renderer: " << m_gpuRenderer << std::endl;
    std::cout << "[GPUContext] GLSL Version: " << m_glslVersion << std::endl;

    // Check compute shader support
    if (!CheckComputeShaderSupport()) {
        m_lastError = "GPU does not support compute shaders (OpenGL 4.3+ required)";
        std::cerr << "[GPUContext] ERROR: " << m_lastError << std::endl;
        return false;
    }

    std::cout << "[GPUContext] Compute shaders supported!" << std::endl;
    std::cout << "[GPUContext] Max work group count: "
              << m_maxWorkGroupCount[0] << " x "
              << m_maxWorkGroupCount[1] << " x "
              << m_maxWorkGroupCount[2] << std::endl;
    std::cout << "[GPUContext] Max work group size: "
              << m_maxWorkGroupSize[0] << " x "
              << m_maxWorkGroupSize[1] << " x "
              << m_maxWorkGroupSize[2] << std::endl;
    std::cout << "[GPUContext] Max work group invocations: "
              << m_maxWorkGroupInvocations << std::endl;

    // Setup debug callback (if available)
    SetupDebugCallback();

    m_initialized = true;
    std::cout << "[GPUContext] Initialization successful!" << std::endl;

    return true;
}

void GPUContext::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "[GPUContext] Shutting down..." << std::endl;

    // Cleanup resources if needed

    m_initialized = false;
}

bool GPUContext::InitializeGLEW() {
#ifdef _WIN32
    // Windows and Linux use GLEW
    // GMod already has OpenGL context, so use experimental mode
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        // GLEW failed - but GMod might already have OpenGL loaded
        // Try to continue anyway since GMod loads OpenGL itself
        m_lastError = "GLEW Warning: " + std::string((const char*)glewGetErrorString(err)) + " (continuing anyway - GMod has OpenGL)";
        std::cerr << "[GPUContext] " << m_lastError << std::endl;
        std::cout << "[GPUContext] Skipping GLEW, using GMod's OpenGL" << std::endl;
        // Don't return false - try to continue
    } else {
        // Clear any OpenGL error from glewInit (it sometimes generates GL_INVALID_ENUM)
        glGetError();
        std::cout << "[GPUContext] GLEW initialized successfully" << std::endl;
        std::cout << "[GPUContext] GLEW Version: " << glewGetString(GLEW_VERSION) << std::endl;
    }

#elif defined(__APPLE__)
    // macOS doesn't need GLEW, uses native OpenGL
    std::cout << "[GPUContext] Using native OpenGL (macOS)" << std::endl;
#endif

    return true;
}

void GPUContext::QueryGPUInfo() {
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    m_glVersion = version ? (const char*)version : "Unknown";
    m_gpuVendor = vendor ? (const char*)vendor : "Unknown";
    m_gpuRenderer = renderer ? (const char*)renderer : "Unknown";
    m_glslVersion = glslVersion ? (const char*)glslVersion : "Unknown";

    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "[GPUContext] OpenGL error while querying info: " << error << std::endl;
    }
}

bool GPUContext::CheckComputeShaderSupport() {
#ifdef _WIN32
    // Check for compute shader extension
    if (!GLEW_ARB_compute_shader && !GLEW_VERSION_4_3) {
        std::cerr << "[GPUContext] Compute shaders not supported (requires OpenGL 4.3+)" << std::endl;
        m_supportsCompute = false;
        return false;
    }
#endif

    // Query compute shader limits
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &m_maxWorkGroupCount[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &m_maxWorkGroupCount[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &m_maxWorkGroupCount[2]);

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &m_maxWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &m_maxWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &m_maxWorkGroupSize[2]);

    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &m_maxWorkGroupInvocations);

    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "[GPUContext] Error querying compute shader limits: " << error << std::endl;
        m_supportsCompute = false;
        return false;
    }

    m_supportsCompute = true;
    return true;
}

void GPUContext::SetupDebugCallback() {
#ifdef _DEBUG
    // Only enable in debug builds
    #ifdef _WIN32
        if (GLEW_ARB_debug_output || GLEW_VERSION_4_3) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(GLDebugCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            std::cout << "[GPUContext] OpenGL debug callback enabled" << std::endl;
        } else {
            std::cout << "[GPUContext] OpenGL debug output not available" << std::endl;
        }
    #elif defined(__APPLE__)
        // macOS OpenGL 4.1+ supports debug output
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        std::cout << "[GPUContext] OpenGL debug output enabled" << std::endl;
    #endif
#else
    std::cout << "[GPUContext] Debug callback disabled (release build)" << std::endl;
#endif
}

void GPUContext::SetDebugOutput(bool enabled) {
#ifdef _DEBUG
    if (enabled) {
        glEnable(GL_DEBUG_OUTPUT);
    } else {
        glDisable(GL_DEBUG_OUTPUT);
    }
#endif
}

void GPUContext::GetMaxWorkGroupCount(int& x, int& y, int& z) const {
    x = m_maxWorkGroupCount[0];
    y = m_maxWorkGroupCount[1];
    z = m_maxWorkGroupCount[2];
}

void GPUContext::GetMaxWorkGroupSize(int& x, int& y, int& z) const {
    x = m_maxWorkGroupSize[0];
    y = m_maxWorkGroupSize[1];
    z = m_maxWorkGroupSize[2];
}

void APIENTRY GPUContext::GLDebugCallback(
    unsigned int source,
    unsigned int type,
    unsigned int id,
    unsigned int severity,
    int length,
    const char* message,
    const void* userParam)
{
    // Skip non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    std::string sourceStr;
    switch (source) {
        case GL_DEBUG_SOURCE_API: sourceStr = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: sourceStr = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION: sourceStr = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER: sourceStr = "Other"; break;
        default: sourceStr = "Unknown"; break;
    }

    std::string typeStr;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: typeStr = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "Undefined Behavior"; break;
        case GL_DEBUG_TYPE_PORTABILITY: typeStr = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: typeStr = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER: typeStr = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: typeStr = "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP: typeStr = "Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER: typeStr = "Other"; break;
        default: typeStr = "Unknown"; break;
    }

    std::string severityStr;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: severityStr = "HIGH"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: severityStr = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_LOW: severityStr = "LOW"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "INFO"; break;
        default: severityStr = "UNKNOWN"; break;
    }

    // Only print warnings and errors (not notifications)
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::cout << "[OpenGL " << severityStr << "] " << typeStr
                  << " (" << sourceStr << "): " << message << std::endl;
    }
}

} // namespace GPUParticles
