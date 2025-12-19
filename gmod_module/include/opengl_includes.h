#pragma once

// OpenGL includes for the particle system
// This header should be included by all files that use OpenGL

#ifdef _WIN32
    // Windows OpenGL headers
    #include <windows.h>
    #include <GL/glew.h>
    #include <GL/gl.h>

    // OpenGL API calling convention
    #ifndef APIENTRY
        #define APIENTRY __stdcall
    #endif

    // Undefine Windows macros that conflict with our code
    #ifdef MemoryBarrier
        #undef MemoryBarrier
    #endif

#elif defined(__linux__)
    // Linux OpenGL headers
    #include <GL/glew.h>
    #include <GL/gl.h>

    #ifndef APIENTRY
        #define APIENTRY
    #endif

#elif defined(__APPLE__)
    // macOS OpenGL headers
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>

    #ifndef APIENTRY
        #define APIENTRY
    #endif

#endif

// OpenGL version check
#ifndef GL_VERSION_4_3
    #error "OpenGL 4.3 or higher is required"
#endif

// Helper macros for OpenGL error checking
#ifdef _DEBUG
    #define GL_CHECK(call) \
        do { \
            call; \
            GLenum err = glGetError(); \
            if (err != GL_NO_ERROR) { \
                std::cerr << "[OpenGL Error] " << #call << " failed with error " << err \
                          << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            } \
        } while(0)
#else
    #define GL_CHECK(call) call
#endif

// OpenGL debug callback (forward declaration)
namespace GPUParticles {
    void APIENTRY GLDebugCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
    );
}
