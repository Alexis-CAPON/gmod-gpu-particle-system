#pragma once

#include "dx9_context.h"
#include "cpu_particle_simulator.h"
#include <d3d9.h>
#include <d3dcompiler.h>
#include <vector>
#include <string>

namespace GPUParticles {

// Simple vector structures to replace D3DX types
struct Vector3f {
    float x, y, z;
    Vector3f() : x(0), y(0), z(0) {}
    Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct Vector2f {
    float x, y;
    Vector2f() : x(0), y(0) {}
    Vector2f(float _x, float _y) : x(_x), y(_y) {}
};

// Simple 4x4 matrix structure to replace D3DXMATRIX
struct Matrix4x4 {
    float m[4][4];

    Matrix4x4() {
        // Identity matrix
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }

    // Access operator
    float* operator[](int row) { return m[row]; }
    const float* operator[](int row) const { return m[row]; }

    // Multiply two matrices
    static Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b) {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i][j] += a.m[i][k] * b.m[k][j];
                }
            }
        }
        return result;
    }

    // Create from float array (row-major)
    static Matrix4x4 FromArray(const float* arr) {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = arr[i * 4 + j];
            }
        }
        return result;
    }
};

/**
 * @brief Vertex format for particle rendering
 */
struct ParticleVertex {
    Vector3f position;       // Particle center
    D3DCOLOR color;          // RGBA color
    Vector2f sizeRot;        // x=size, y=rotation
    Vector2f corner;         // Corner offset (-1 to 1)

    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2;
};

/**
 * @brief DirectX 9 particle renderer
 *
 * Renders particles using vertex/pixel shaders with billboarding
 */
class DX9ParticleRenderer {
public:
    DX9ParticleRenderer();
    ~DX9ParticleRenderer();

    /**
     * @brief Initialize renderer
     * @param context DX9 context
     * @return True if successful
     */
    bool Initialize(DX9Context* context);

    /**
     * @brief Shutdown and release resources
     */
    void Shutdown();

    /**
     * @brief Check if initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Render particles
     * @param simulator Particle simulator with particle data
     * @param viewMatrix View matrix
     * @param projMatrix Projection matrix
     * @param cameraPos Camera position
     * @param emitterPos World position of the particle emitter
     * @param scale Scale multiplier for particle sizes
     */
    void Render(const CPUParticleSimulator& simulator,
                const float* viewMatrix,
                const float* projMatrix,
                const float* cameraPos,
                const float* emitterPos,
                float scale);

    /**
     * @brief Test render - draw a simple quad without billboarding
     * @param worldPos Position in world space
     * @param size Size of the quad
     * @param viewMatrix View matrix
     * @param projMatrix Projection matrix
     */
    void RenderTestQuad(const float* worldPos,
                       float size,
                       const float* viewMatrix,
                       const float* projMatrix);

    /**
     * @brief Get last error
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief Test render in 2D screen space
     * @param screenX X position in screen pixels (0 = left edge)
     * @param screenY Y position in screen pixels (0 = top edge)
     * @param pixelSize Size of the quad in pixels
     */
    void RenderTest2D(float screenX, float screenY, float pixelSize);

    /**
     * @brief Test render in 3D world space
     * @param worldPos World position (x, y, z)
     * @param size Size in world units
     * @param viewMatrix View matrix from camera
     * @param projMatrix Projection matrix
     */
    void RenderTest3D(const float* worldPos, float size,
                     const float* viewMatrix, const float* projMatrix);

    /**
     * @brief Test render using Source Engine's actual matrices
     * @param worldPos World position (x, y, z)
     * @param size Size in world units
     */
    void RenderTest3DSourceMatrices(const float* worldPos, float size);

    /**
     * @brief Render 3D position projected to screen space
     * @param worldPos World position (x, y, z)
     * @param size Size in world units
     * @param viewMatrix View matrix to project with
     * @param projMatrix Projection matrix to project with
     * @param screenWidth Screen width in pixels
     * @param screenHeight Screen height in pixels
     */
    void RenderTest3DProjected(const float* worldPos, float size,
                               const float* viewMatrix, const float* projMatrix,
                               int screenWidth, int screenHeight);

private:
    // Initialization helpers
    bool LoadShaders();
    bool CreateVertexBuffer();
    bool CreateTexture();

    // Rendering helpers
    void UpdateVertexBuffer(const std::vector<Particle>& particles,
                           const float* emitterPos,
                           float scale,
                           const Vector3f& cameraRight,
                           const Vector3f& cameraUp);
    void SetupRenderStates();
    void RestoreRenderStates();

    // Resources
    DX9Context* m_context;
    IDirect3DDevice9* m_device;
    IDirect3DVertexBuffer9* m_vertexBuffer;
    IDirect3DTexture9* m_texture;
    IDirect3DVertexShader9* m_vertexShader;
    IDirect3DPixelShader9* m_pixelShader;
    IDirect3DVertexDeclaration9* m_vertexDeclaration;

    // State
    int m_maxParticles;
    bool m_initialized;
    std::string m_lastError;

    // Saved render states
    DWORD m_savedAlphaBlendEnable;
    DWORD m_savedSrcBlend;
    DWORD m_savedDestBlend;
    DWORD m_savedZEnable;
    DWORD m_savedZWriteEnable;
    DWORD m_savedCullMode;

    // Saved shader states
    IDirect3DVertexShader9* m_savedVertexShader;
    IDirect3DPixelShader9* m_savedPixelShader;
    IDirect3DVertexDeclaration9* m_savedVertexDeclaration;
    IDirect3DBaseTexture9* m_savedTexture;
    IDirect3DVertexBuffer9* m_savedStreamSource;
    UINT m_savedStreamOffset;
    UINT m_savedStreamStride;

    // Saved shader constants (we overwrite constants 0-5)
    float m_savedVSConstants[6 * 4]; // 6 float4 constants = 24 floats
};

} // namespace GPUParticles
