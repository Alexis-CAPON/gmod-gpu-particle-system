#include "dx9_particle_renderer.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace GPUParticles {

// External logger from d3d9_hook.cpp
extern void LogToFile(const std::string& msg);

DX9ParticleRenderer::DX9ParticleRenderer()
    : m_context(nullptr)
    , m_device(nullptr)
    , m_vertexBuffer(nullptr)
    , m_texture(nullptr)
    , m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
    , m_vertexDeclaration(nullptr)
    , m_maxParticles(0)
    , m_initialized(false)
    , m_savedVertexShader(nullptr)
    , m_savedPixelShader(nullptr)
    , m_savedVertexDeclaration(nullptr)
    , m_savedTexture(nullptr)
    , m_savedStreamSource(nullptr)
    , m_savedStreamOffset(0)
    , m_savedStreamStride(0)
{
}

DX9ParticleRenderer::~DX9ParticleRenderer() {
    Shutdown();
}

bool DX9ParticleRenderer::Initialize(DX9Context* context) {
    if (m_initialized) {
        return true;
    }

    std::cout << "[DX9ParticleRenderer] Initializing..." << std::endl;

    if (!context || !context->IsInitialized()) {
        m_lastError = "DX9Context is not initialized";
        std::cerr << "[DX9ParticleRenderer] ERROR: " << m_lastError << std::endl;
        return false;
    }

    m_context = context;
    m_device = context->GetDevice();

    if (!m_device) {
        m_lastError = "DirectX device is null";
        std::cerr << "[DX9ParticleRenderer] ERROR: " << m_lastError << std::endl;
        return false;
    }

    // Load shaders
    if (!LoadShaders()) {
        std::cerr << "[DX9ParticleRenderer] Failed to load shaders" << std::endl;
        return false;
    }

    // Create vertex buffer (for max 50000 particles)
    m_maxParticles = 50000;
    if (!CreateVertexBuffer()) {
        std::cerr << "[DX9ParticleRenderer] Failed to create vertex buffer" << std::endl;
        return false;
    }

    // Create default texture
    if (!CreateTexture()) {
        std::cerr << "[DX9ParticleRenderer] Failed to create texture" << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "[DX9ParticleRenderer] Initialization successful!" << std::endl;

    return true;
}

void DX9ParticleRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "[DX9ParticleRenderer] Shutting down..." << std::endl;

    // Release DirectX resources
    if (m_vertexDeclaration) {
        m_vertexDeclaration->Release();
        m_vertexDeclaration = nullptr;
    }

    if (m_pixelShader) {
        m_pixelShader->Release();
        m_pixelShader = nullptr;
    }

    if (m_vertexShader) {
        m_vertexShader->Release();
        m_vertexShader = nullptr;
    }

    if (m_texture) {
        m_texture->Release();
        m_texture = nullptr;
    }

    if (m_vertexBuffer) {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }

    m_device = nullptr;
    m_context = nullptr;
    m_initialized = false;

    std::cout << "[DX9ParticleRenderer] Shutdown complete" << std::endl;
}

bool DX9ParticleRenderer::LoadShaders() {
    std::cout << "[DX9ParticleRenderer] Loading shaders..." << std::endl;

    // For now, we'll compile shaders at runtime
    // In production, you'd pre-compile them

    // Simple passthrough vertex shader (billboarding done on CPU now)
    // NOTE: Matrix multiplication order matters! Trying both to find which works
    const char* vsSource =
        "struct VS_INPUT { \n"
        "    float3 position : POSITION0; \n"
        "    float4 color : COLOR0; \n"
        "    float2 sizeRot : TEXCOORD0; \n"
        "    float2 texcoord : TEXCOORD1; \n"
        "}; \n"
        "struct VS_OUTPUT { \n"
        "    float4 position : POSITION0; \n"
        "    float4 color : COLOR0; \n"
        "    float2 texcoord : TEXCOORD0; \n"
        "}; \n"
        "float4x4 viewProjection : register(c0); \n"
        "VS_OUTPUT main(VS_INPUT input) { \n"
        "    VS_OUTPUT output; \n"
        "    // Position is already expanded on CPU, just transform to screen space \n"
        "    // CHANGED: Reverse multiplication order (matrix * vector instead of vector * matrix) \n"
        "    output.position = mul(viewProjection, float4(input.position, 1.0)); \n"
        "    output.color = input.color; \n"
        "    output.texcoord = input.texcoord; \n"
        "    return output; \n"
        "} \n";

    const char* psSource =
        "struct PS_INPUT { \n"
        "    float4 color : COLOR0; \n"
        "    float2 texcoord : TEXCOORD0; \n"
        "}; \n"
        "sampler2D particleTexture : register(s0); \n"
        "float4 main(PS_INPUT input) : COLOR0 { \n"
        "    float4 texColor = tex2D(particleTexture, input.texcoord); \n"
        "    return texColor * input.color; \n"
        "} \n";

    // Compile vertex shader
    ID3DBlob* vsBuffer = nullptr;
    ID3DBlob* errorBuffer = nullptr;

    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr,
                           "main", "vs_2_0", 0, 0, &vsBuffer, &errorBuffer);

    if (FAILED(hr)) {
        if (errorBuffer) {
            m_lastError = std::string("VS compile error: ") +
                         static_cast<char*>(errorBuffer->GetBufferPointer());
            errorBuffer->Release();
        } else {
            m_lastError = "Failed to compile vertex shader";
        }
        return false;
    }

    hr = m_device->CreateVertexShader(static_cast<DWORD*>(vsBuffer->GetBufferPointer()),
                                      &m_vertexShader);
    vsBuffer->Release();

    if (FAILED(hr)) {
        m_lastError = "Failed to create vertex shader";
        return false;
    }

    // Compile pixel shader
    ID3DBlob* psBuffer = nullptr;
    hr = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr,
                   "main", "ps_2_0", 0, 0, &psBuffer, &errorBuffer);

    if (FAILED(hr)) {
        if (errorBuffer) {
            m_lastError = std::string("PS compile error: ") +
                         static_cast<char*>(errorBuffer->GetBufferPointer());
            errorBuffer->Release();
        } else {
            m_lastError = "Failed to compile pixel shader";
        }
        return false;
    }

    hr = m_device->CreatePixelShader(static_cast<DWORD*>(psBuffer->GetBufferPointer()),
                                     &m_pixelShader);
    psBuffer->Release();

    if (FAILED(hr)) {
        m_lastError = "Failed to create pixel shader";
        return false;
    }

    // Create vertex declaration
    D3DVERTEXELEMENT9 vertexElements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    };

    hr = m_device->CreateVertexDeclaration(vertexElements, &m_vertexDeclaration);
    if (FAILED(hr)) {
        m_lastError = "Failed to create vertex declaration";
        return false;
    }

    std::cout << "[DX9ParticleRenderer] Shaders loaded successfully" << std::endl;
    return true;
}

bool DX9ParticleRenderer::CreateVertexBuffer() {
    std::cout << "[DX9ParticleRenderer] Creating vertex buffer for "
              << m_maxParticles << " particles..." << std::endl;

    // Each particle needs 4 vertices (quad corners)
    int vertexCount = m_maxParticles * 4;
    int bufferSize = vertexCount * sizeof(ParticleVertex);

    HRESULT hr = m_device->CreateVertexBuffer(
        bufferSize,
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        ParticleVertex::FVF,
        D3DPOOL_DEFAULT,
        &m_vertexBuffer,
        nullptr
    );

    if (FAILED(hr)) {
        m_lastError = "Failed to create vertex buffer";
        return false;
    }

    std::cout << "[DX9ParticleRenderer] Vertex buffer created" << std::endl;
    return true;
}

bool DX9ParticleRenderer::CreateTexture() {
    LogToFile("[DX9ParticleRenderer] Creating default particle texture...");

    // Create a simple 64x64 white circle texture
    const int size = 64;

    // Use D3DPOOL_DEFAULT with D3DUSAGE_DYNAMIC so we can lock it during hooks
    HRESULT hr = m_device->CreateTexture(size, size, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
                                         D3DPOOL_DEFAULT, &m_texture, nullptr);

    if (FAILED(hr)) {
        char errorBuf[256];
        sprintf(errorBuf, "Failed to create texture: HRESULT=0x%08X", hr);
        m_lastError = errorBuf;
        LogToFile(std::string("[DX9ParticleRenderer] ERROR: ") + m_lastError);
        return false;
    }

    // Fill texture with a circular gradient
    D3DLOCKED_RECT lockedRect;
    hr = m_texture->LockRect(0, &lockedRect, nullptr, 0);
    if (FAILED(hr)) {
        m_lastError = "Failed to lock texture for filling";
        LogToFile(std::string("[DX9ParticleRenderer] ERROR: ") + m_lastError);
        return false;
    }

    LogToFile("[DX9ParticleRenderer] Filling texture with white circle...");
    DWORD* pixels = static_cast<DWORD*>(lockedRect.pBits);
    float center = size * 0.5f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float dist = sqrt(dx * dx + dy * dy) / center;

            // Smooth circular falloff
            float alpha = std::max(0.0f, 1.0f - dist);
            alpha = alpha * alpha; // Square for smoother falloff

            BYTE a = static_cast<BYTE>(alpha * 255);
            // White color with alpha
            pixels[y * (lockedRect.Pitch / 4) + x] = D3DCOLOR_ARGB(a, 255, 255, 255);
        }
    }

    m_texture->UnlockRect(0);
    LogToFile("[DX9ParticleRenderer] Texture created and filled successfully!");
    return true;
}

void DX9ParticleRenderer::Render(const CPUParticleSimulator& simulator,
                                 const float* viewMatrix,
                                 const float* projMatrix,
                                 const float* cameraPos,
                                 const float* emitterPos,
                                 float scale) {

    // TEMPORARY FIX: Calculate camera right/up vectors for CPU billboarding
    Matrix4x4 view = Matrix4x4::FromArray(viewMatrix);
    Vector3f cameraRight(view[0][0], view[1][0], view[2][0]);
    Vector3f cameraUp(view[0][1], view[1][1], view[2][1]);
    static int renderCallCount = 0;
    static bool firstRender = true;

    if (!m_initialized || !simulator.IsInitialized()) {
        return;
    }

    const std::vector<Particle>& particles = simulator.GetParticles();
    int aliveCount = simulator.GetAliveCount();

    if (aliveCount == 0) {
        return;
    }

    // Log first render attempt
    if (firstRender) {
        char buf[256];
        sprintf(buf, "[Renderer] *** FIRST RENDER CALL! Alive particles: %d ***", aliveCount);
        LogToFile(buf);
        firstRender = false;
    }

    // Update vertex buffer with particle data, applying world transform
    // Pass camera vectors for CPU billboarding
    UpdateVertexBuffer(particles, emitterPos, scale, cameraRight, cameraUp);

    // Setup render states
    SetupRenderStates();

    // Set shaders
    m_device->SetVertexShader(m_vertexShader);
    m_device->SetPixelShader(m_pixelShader);
    m_device->SetVertexDeclaration(m_vertexDeclaration);

    // Set shader constants (reuse view matrix from top of function)
    Matrix4x4 proj = Matrix4x4::FromArray(projMatrix);
    Matrix4x4 viewProj = Matrix4x4::Multiply(view, proj);
    m_device->SetVertexShaderConstantF(0, &viewProj.m[0][0], 4);

    // Camera vectors already calculated at top of function for CPU billboarding

    static bool loggedVectors = false;
    if (!loggedVectors) {
        char buf[512];
        sprintf(buf, "[Renderer] View matrix row0: [%.3f, %.3f, %.3f, %.3f]", view[0][0], view[0][1], view[0][2], view[0][3]);
        LogToFile(buf);
        sprintf(buf, "[Renderer] View matrix row1: [%.3f, %.3f, %.3f, %.3f]", view[1][0], view[1][1], view[1][2], view[1][3]);
        LogToFile(buf);
        sprintf(buf, "[Renderer] View matrix row2: [%.3f, %.3f, %.3f, %.3f]", view[2][0], view[2][1], view[2][2], view[2][3]);
        LogToFile(buf);
        sprintf(buf, "[Renderer] Camera right: (%.3f, %.3f, %.3f)", cameraRight.x, cameraRight.y, cameraRight.z);
        LogToFile(buf);
        sprintf(buf, "[Renderer] Camera up: (%.3f, %.3f, %.3f)", cameraUp.x, cameraUp.y, cameraUp.z);
        LogToFile(buf);

        // Also log first particle position for debugging
        const std::vector<Particle>& particles = simulator.GetParticles();
        for (const auto& p : particles) {
            if (p.alive) {
                sprintf(buf, "[Renderer] First particle pos: (%.1f, %.1f, %.1f) size: %.1f",
                        p.position.x, p.position.y, p.position.z, p.size);
                LogToFile(buf);
                break;
            }
        }

        loggedVectors = true;
    }

    m_device->SetVertexShaderConstantF(4, &cameraRight.x, 1);
    m_device->SetVertexShaderConstantF(5, &cameraUp.x, 1);

    // Set texture
    m_device->SetTexture(0, m_texture);

    // Set vertex buffer and draw
    m_device->SetStreamSource(0, m_vertexBuffer, 0, sizeof(ParticleVertex));
    m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, aliveCount * 2); // 2 triangles per particle

    // Restore render states
    RestoreRenderStates();
}

void DX9ParticleRenderer::UpdateVertexBuffer(const std::vector<Particle>& particles,
                                              const float* emitterPos,
                                              float scale,
                                              const Vector3f& cameraRight,
                                              const Vector3f& cameraUp) {
    void* data = nullptr;
    HRESULT hr = m_vertexBuffer->Lock(0, 0, &data, D3DLOCK_DISCARD);

    if (FAILED(hr) || !data) {
        return;
    }

    ParticleVertex* vertices = static_cast<ParticleVertex*>(data);
    int vertexIndex = 0;

    // Extract emitter position
    Vector3f emitterPosition(emitterPos[0], emitterPos[1], emitterPos[2]);

    static int particlesLogged = 0;
    const int maxParticlesToLog = 5; // Log first 5 particles for debugging

    // For each alive particle, generate 4 vertices (2 triangles)
    for (const auto& p : particles) {
        if (!p.alive) {
            continue;
        }

        // Apply emitter position to particle position (world transform)
        Vector3f pos(
            p.position.x + emitterPosition.x,
            p.position.y + emitterPosition.y,
            p.position.z + emitterPosition.z
        );

        D3DCOLOR color = D3DCOLOR_COLORVALUE(p.color.r, p.color.g, p.color.b, p.color.a);

        // Apply scale to particle size
        Vector2f sizeRot(p.size * scale, p.rotation);

        // Log first few particles for debugging
        if (particlesLogged < maxParticlesToLog) {
            char buf[512];
            sprintf(buf, "[UpdateVB #%d] Particle: local(%.1f,%.1f,%.1f) + emitter(%.1f,%.1f,%.1f) = world(%.1f,%.1f,%.1f), size=%.1f*%.2f=%.1f",
                    particlesLogged + 1,
                    p.position.x, p.position.y, p.position.z,
                    emitterPosition.x, emitterPosition.y, emitterPosition.z,
                    pos.x, pos.y, pos.z,
                    p.size, scale, sizeRot.x);
            LogToFile(buf);

            // Log the 4 corner vertices
            sprintf(buf, "[UpdateVB #%d] Quad corners:", particlesLogged + 1);
            LogToFile(buf);
            sprintf(buf, "[UpdateVB #%d]   Corner 0: pos(%.1f,%.1f,%.1f) corner(-1,-1) size=%.1f", particlesLogged + 1, pos.x, pos.y, pos.z, sizeRot.x);
            LogToFile(buf);
            sprintf(buf, "[UpdateVB #%d]   Corner 1: pos(%.1f,%.1f,%.1f) corner(+1,-1) size=%.1f", particlesLogged + 1, pos.x, pos.y, pos.z, sizeRot.x);
            LogToFile(buf);
            sprintf(buf, "[UpdateVB #%d]   Corner 2: pos(%.1f,%.1f,%.1f) corner(+1,+1) size=%.1f", particlesLogged + 1, pos.x, pos.y, pos.z, sizeRot.x);
            LogToFile(buf);
            sprintf(buf, "[UpdateVB #%d]   Corner 3: pos(%.1f,%.1f,%.1f) corner(-1,+1) size=%.1f", particlesLogged + 1, pos.x, pos.y, pos.z, sizeRot.x);
            LogToFile(buf);

            particlesLogged++;
        }

        // CPU BILLBOARDING: Expand corners using camera vectors
        float finalSize = sizeRot.x; // Already includes scale

        // Calculate the 4 actual corner positions in world space
        Vector3f bottomLeft = Vector3f(
            pos.x + cameraRight.x * (-finalSize) + cameraUp.x * (-finalSize),
            pos.y + cameraRight.y * (-finalSize) + cameraUp.y * (-finalSize),
            pos.z + cameraRight.z * (-finalSize) + cameraUp.z * (-finalSize)
        );
        Vector3f bottomRight = Vector3f(
            pos.x + cameraRight.x * (finalSize) + cameraUp.x * (-finalSize),
            pos.y + cameraRight.y * (finalSize) + cameraUp.y * (-finalSize),
            pos.z + cameraRight.z * (finalSize) + cameraUp.z * (-finalSize)
        );
        Vector3f topRight = Vector3f(
            pos.x + cameraRight.x * (finalSize) + cameraUp.x * (finalSize),
            pos.y + cameraRight.y * (finalSize) + cameraUp.y * (finalSize),
            pos.z + cameraRight.z * (finalSize) + cameraUp.z * (finalSize)
        );
        Vector3f topLeft = Vector3f(
            pos.x + cameraRight.x * (-finalSize) + cameraUp.x * (finalSize),
            pos.y + cameraRight.y * (-finalSize) + cameraUp.y * (finalSize),
            pos.z + cameraRight.z * (-finalSize) + cameraUp.z * (finalSize)
        );

        // Triangle 1: bottom-left, bottom-right, top-right
        vertices[vertexIndex++] = {bottomLeft, color, sizeRot, Vector2f(0, 1)};   // UV: bottom-left
        vertices[vertexIndex++] = {bottomRight, color, sizeRot, Vector2f(1, 1)};  // UV: bottom-right
        vertices[vertexIndex++] = {topRight, color, sizeRot, Vector2f(1, 0)};     // UV: top-right

        // Triangle 2: bottom-left, top-right, top-left
        vertices[vertexIndex++] = {bottomLeft, color, sizeRot, Vector2f(0, 1)};   // UV: bottom-left
        vertices[vertexIndex++] = {topRight, color, sizeRot, Vector2f(1, 0)};     // UV: top-right
        vertices[vertexIndex++] = {topLeft, color, sizeRot, Vector2f(0, 0)};      // UV: top-left
    }

    m_vertexBuffer->Unlock();
}

void DX9ParticleRenderer::SetupRenderStates() {
    static bool firstCall = true;
    if (firstCall) {
        LogToFile("[Renderer] SetupRenderStates() called for first time");
    }

    // Save current render states
    m_device->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_savedAlphaBlendEnable);
    m_device->GetRenderState(D3DRS_SRCBLEND, &m_savedSrcBlend);
    m_device->GetRenderState(D3DRS_DESTBLEND, &m_savedDestBlend);
    m_device->GetRenderState(D3DRS_ZENABLE, &m_savedZEnable);
    m_device->GetRenderState(D3DRS_ZWRITEENABLE, &m_savedZWriteEnable);
    m_device->GetRenderState(D3DRS_CULLMODE, &m_savedCullMode);

    // Save current shader states (CRITICAL for GMod compatibility!)
    m_device->GetVertexShader(&m_savedVertexShader);
    m_device->GetPixelShader(&m_savedPixelShader);
    m_device->GetVertexDeclaration(&m_savedVertexDeclaration);
    m_device->GetTexture(0, &m_savedTexture);
    m_device->GetStreamSource(0, &m_savedStreamSource, &m_savedStreamOffset, &m_savedStreamStride);

    // CRITICAL: Save vertex shader constants that we will overwrite (0-5)
    // This is what was causing the black screen!
    m_device->GetVertexShaderConstantF(0, m_savedVSConstants, 6);

    if (firstCall) {
        char buf[512];
        sprintf(buf, "[Renderer] Saved states: VS=%p PS=%p VDecl=%p Tex=%p VB=%p",
                m_savedVertexShader, m_savedPixelShader, m_savedVertexDeclaration,
                m_savedTexture, m_savedStreamSource);
        LogToFile(buf);
        LogToFile("[Renderer] CRITICAL FIX: Saving shader constants 0-5");
        firstCall = false;
    }

    // Set particle render states
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);  // Disable depth testing completely
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);  // No depth writes for transparency
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}

void DX9ParticleRenderer::RenderTestQuad(const float* worldPos,
                                          float size,
                                          const float* viewMatrix,
                                          const float* projMatrix) {
    if (!m_initialized) {
        return;
    }

    LogToFile("[TestQuad] Rendering simple quad without billboarding");

    // Create 6 vertices for 2 triangles (a quad)
    struct SimpleVertex {
        float x, y, z;
        D3DCOLOR color;
        float u, v;
    };

    // Fixed quad in XY plane
    SimpleVertex vertices[6] = {
        // Triangle 1
        {worldPos[0] - size, worldPos[1] - size, worldPos[2], D3DCOLOR_ARGB(255, 255, 0, 0), 0, 0},  // Bottom-left (red)
        {worldPos[0] + size, worldPos[1] - size, worldPos[2], D3DCOLOR_ARGB(255, 0, 255, 0), 1, 0},  // Bottom-right (green)
        {worldPos[0] + size, worldPos[1] + size, worldPos[2], D3DCOLOR_ARGB(255, 0, 0, 255), 1, 1},  // Top-right (blue)
        // Triangle 2
        {worldPos[0] - size, worldPos[1] - size, worldPos[2], D3DCOLOR_ARGB(255, 255, 0, 0), 0, 0},  // Bottom-left (red)
        {worldPos[0] + size, worldPos[1] + size, worldPos[2], D3DCOLOR_ARGB(255, 0, 0, 255), 1, 1},  // Top-right (blue)
        {worldPos[0] - size, worldPos[1] + size, worldPos[2], D3DCOLOR_ARGB(255, 255, 255, 0), 0, 1}, // Top-left (yellow)
    };

    char buf[256];
    sprintf(buf, "[TestQuad] Quad at (%.1f, %.1f, %.1f) with size %.1f", worldPos[0], worldPos[1], worldPos[2], size);
    LogToFile(buf);

    // Build matrices
    Matrix4x4 view = Matrix4x4::FromArray(viewMatrix);
    Matrix4x4 proj = Matrix4x4::FromArray(projMatrix);
    Matrix4x4 viewProj = Matrix4x4::Multiply(view, proj);

    // Transform vertices manually (no shader)
    for (int i = 0; i < 6; i++) {
        // Transform to clip space
        float x = vertices[i].x;
        float y = vertices[i].y;
        float z = vertices[i].z;

        // Apply view-projection
        float clipX = x * viewProj[0][0] + y * viewProj[0][1] + z * viewProj[0][2] + viewProj[0][3];
        float clipY = x * viewProj[1][0] + y * viewProj[1][1] + z * viewProj[1][2] + viewProj[1][3];
        float clipZ = x * viewProj[2][0] + y * viewProj[2][1] + z * viewProj[2][2] + viewProj[2][3];
        float clipW = x * viewProj[3][0] + y * viewProj[3][1] + z * viewProj[3][2] + viewProj[3][3];

        vertices[i].x = clipX / clipW;
        vertices[i].y = clipY / clipW;
        vertices[i].z = clipZ / clipW;
    }

    // Setup render states
    SetupRenderStates();

    // Disable shaders - use fixed function pipeline
    m_device->SetVertexShader(NULL);
    m_device->SetPixelShader(NULL);

    // Set FVF for simple vertices
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

    // Set texture
    m_device->SetTexture(0, m_texture);

    // Draw directly (no vertex buffer)
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(SimpleVertex));

    LogToFile("[TestQuad] Drew 2 triangles (6 vertices)");

    // Restore states
    RestoreRenderStates();
}

void DX9ParticleRenderer::RestoreRenderStates() {
    static bool firstCall = true;
    if (firstCall) {
        LogToFile("[Renderer] RestoreRenderStates() called for first time");
    }

    // Restore saved render states
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, m_savedAlphaBlendEnable);
    m_device->SetRenderState(D3DRS_SRCBLEND, m_savedSrcBlend);
    m_device->SetRenderState(D3DRS_DESTBLEND, m_savedDestBlend);
    m_device->SetRenderState(D3DRS_ZENABLE, m_savedZEnable);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, m_savedZWriteEnable);
    m_device->SetRenderState(D3DRS_CULLMODE, m_savedCullMode);

    // Restore shader states (CRITICAL for GMod compatibility!)
    m_device->SetVertexShader(m_savedVertexShader);
    m_device->SetPixelShader(m_savedPixelShader);
    m_device->SetVertexDeclaration(m_savedVertexDeclaration);
    m_device->SetTexture(0, m_savedTexture);
    m_device->SetStreamSource(0, m_savedStreamSource, m_savedStreamOffset, m_savedStreamStride);

    // CRITICAL: Restore vertex shader constants that we overwrote (0-5)
    // This fixes the black screen!
    m_device->SetVertexShaderConstantF(0, m_savedVSConstants, 6);

    if (firstCall) {
        LogToFile("[Renderer] States restored successfully");
        LogToFile("[Renderer] CRITICAL FIX: Restored shader constants 0-5");
        firstCall = false;
    }

    // Release the references we got from Get* calls
    if (m_savedVertexShader) {
        m_savedVertexShader->Release();
        m_savedVertexShader = nullptr;
    }
    if (m_savedPixelShader) {
        m_savedPixelShader->Release();
        m_savedPixelShader = nullptr;
    }
    if (m_savedVertexDeclaration) {
        m_savedVertexDeclaration->Release();
        m_savedVertexDeclaration = nullptr;
    }
    if (m_savedTexture) {
        m_savedTexture->Release();
        m_savedTexture = nullptr;
    }
    if (m_savedStreamSource) {
        m_savedStreamSource->Release();
        m_savedStreamSource = nullptr;
    }
}

void DX9ParticleRenderer::RenderTest2D(float screenX, float screenY, float pixelSize) {
    char buf[512];
    sprintf(buf, "[RenderTest2D] === CALLED: x=%.1f, y=%.1f, size=%.1f ===", screenX, screenY, pixelSize);
    LogToFile(buf);

    if (!m_initialized) {
        LogToFile("[RenderTest2D] ERROR: Not initialized!");
        return;
    }

    if (!m_device) {
        LogToFile("[RenderTest2D] ERROR: Device is null!");
        return;
    }

    LogToFile("[RenderTest2D] Checks passed, proceeding...");

    // Get screen dimensions
    D3DVIEWPORT9 viewport;
    m_device->GetViewport(&viewport);
    float screenWidth = (float)viewport.Width;
    float screenHeight = (float)viewport.Height;

    sprintf(buf, "[RenderTest2D] Screen: %dx%d", (int)screenWidth, (int)screenHeight);
    LogToFile(buf);

    // Setup orthographic projection for screen-space rendering
    // This maps (0,0) to top-left and (width,height) to bottom-right
    Matrix4x4 orthoProj;
    orthoProj.m[0][0] = 2.0f / screenWidth;
    orthoProj.m[1][1] = -2.0f / screenHeight;  // Negative Y for screen coords
    orthoProj.m[2][2] = 1.0f;
    orthoProj.m[3][0] = -1.0f;
    orthoProj.m[3][1] = 1.0f;
    orthoProj.m[3][3] = 1.0f;

    LogToFile("[RenderTest2D] Ortho projection created");

    // Setup render states
    LogToFile("[RenderTest2D] Setting up render states...");
    SetupRenderStates();

    // TEST WITH SHADERS and texture
    LogToFile("[RenderTest2D] Testing with shaders + texture...");

    // Use our vertex/pixel shaders (with texture sampling)
    m_device->SetVertexShader(m_vertexShader);
    m_device->SetPixelShader(m_pixelShader);
    m_device->SetVertexDeclaration(m_vertexDeclaration);
    m_device->SetVertexShaderConstantF(0, &orthoProj.m[0][0], 4);

    // Use our particle texture
    m_device->SetTexture(0, m_texture);
    LogToFile("[RenderTest2D] Texture set");

    // Create a quad using ParticleVertex format (for shaders)
    ParticleVertex quad[6];
    D3DCOLOR testColor = D3DCOLOR_RGBA(255, 0, 0, 255); // Bright red

    float halfSize = pixelSize * 0.5f;
    float left = screenX - halfSize;
    float right = screenX + halfSize;
    float top = screenY - halfSize;
    float bottom = screenY + halfSize;

    sprintf(buf, "[RenderTest2D] Quad corners: L=%.0f R=%.0f T=%.0f B=%.0f", left, right, top, bottom);
    LogToFile(buf);

    // Triangle 1
    quad[0] = {Vector3f(left, bottom, 0.5f), testColor, Vector2f(0, 0), Vector2f(0, 1)};
    quad[1] = {Vector3f(right, bottom, 0.5f), testColor, Vector2f(0, 0), Vector2f(1, 1)};
    quad[2] = {Vector3f(right, top, 0.5f), testColor, Vector2f(0, 0), Vector2f(1, 0)};
    // Triangle 2
    quad[3] = {Vector3f(left, bottom, 0.5f), testColor, Vector2f(0, 0), Vector2f(0, 1)};
    quad[4] = {Vector3f(right, top, 0.5f), testColor, Vector2f(0, 0), Vector2f(1, 0)};
    quad[5] = {Vector3f(left, top, 0.5f), testColor, Vector2f(0, 0), Vector2f(0, 0)};

    // Lock vertex buffer and write quad
    LogToFile("[RenderTest2D] Attempting to lock vertex buffer...");
    void* data = nullptr;
    HRESULT hr = m_vertexBuffer->Lock(0, 6 * sizeof(ParticleVertex), &data, D3DLOCK_DISCARD);

    sprintf(buf, "[RenderTest2D] Lock result: hr=0x%08X, data=%p", hr, data);
    LogToFile(buf);

    if (SUCCEEDED(hr) && data) {
        LogToFile("[RenderTest2D] Lock succeeded! Writing vertices...");
        memcpy(data, quad, 6 * sizeof(ParticleVertex));
        m_vertexBuffer->Unlock();

        LogToFile("[RenderTest2D] Vertices written, setting stream source...");
        m_device->SetStreamSource(0, m_vertexBuffer, 0, sizeof(ParticleVertex));

        LogToFile("[RenderTest2D] Drawing 2 triangles...");
        m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

        LogToFile("[RenderTest2D] === DRAW COMPLETE ===");
    } else {
        LogToFile("[RenderTest2D] ERROR: Vertex buffer lock FAILED!");
    }

    // Restore render states
    LogToFile("[RenderTest2D] Restoring render states...");
    RestoreRenderStates();
    LogToFile("[RenderTest2D] === FINISHED ===");
}

void DX9ParticleRenderer::RenderTest3D(const float* worldPos, float size,
                                        const float* viewMatrix, const float* projMatrix) {
    // Only log once to avoid spam
    static bool firstCall = true;
    if (firstCall) {
        char buf[1024];
        sprintf(buf, "[RenderTest3D] === First Call Diagnostics ===\n"
                     "World Pos: (%.1f, %.1f, %.1f)\n"
                     "Size: %.1f\n"
                     "View Matrix Row 0: [%.3f, %.3f, %.3f, %.3f]\n"
                     "View Matrix Row 1: [%.3f, %.3f, %.3f, %.3f]\n"
                     "View Matrix Row 2: [%.3f, %.3f, %.3f, %.3f]\n"
                     "View Matrix Row 3: [%.3f, %.3f, %.3f, %.3f]\n"
                     "Proj Matrix Row 0: [%.3f, %.3f, %.3f, %.3f]\n"
                     "Proj Matrix Row 1: [%.3f, %.3f, %.3f, %.3f]",
                worldPos[0], worldPos[1], worldPos[2], size,
                viewMatrix[0], viewMatrix[1], viewMatrix[2], viewMatrix[3],
                viewMatrix[4], viewMatrix[5], viewMatrix[6], viewMatrix[7],
                viewMatrix[8], viewMatrix[9], viewMatrix[10], viewMatrix[11],
                viewMatrix[12], viewMatrix[13], viewMatrix[14], viewMatrix[15],
                projMatrix[0], projMatrix[1], projMatrix[2], projMatrix[3],
                projMatrix[4], projMatrix[5], projMatrix[6], projMatrix[7]);
        LogToFile(buf);
        firstCall = false;
    }

    if (!m_initialized || !m_device) {
        return;
    }

    // Log world position to track if it's changing (first 10 frames)
    static int frameCount = 0;
    if (frameCount < 10) {
        char buf[256];
        sprintf(buf, "[RenderTest3D] Frame %d - World Position: (%.1f, %.1f, %.1f)",
                frameCount, worldPos[0], worldPos[1], worldPos[2]);
        LogToFile(buf);
        frameCount++;
    }

    // Extract camera vectors from view matrix for CPU billboarding
    Matrix4x4 view = Matrix4x4::FromArray(viewMatrix);
    Vector3f cameraRight(view[0][0], view[1][0], view[2][0]);
    Vector3f cameraUp(view[0][1], view[1][1], view[2][1]);

    // Also log camera position from view matrix for first 10 frames
    if (frameCount <= 10) {
        // Camera position is in the 4th column (but view matrix has it inverted)
        // For a standard view matrix, camera pos can be extracted from inverse
        char buf[256];
        sprintf(buf, "[RenderTest3D] Frame %d - View translation: (%.1f, %.1f, %.1f)",
                frameCount - 1, view[3][0], view[3][1], view[3][2]);
        LogToFile(buf);
    }

    // Setup render states
    SetupRenderStates();

    // Set shaders and matrices
    m_device->SetVertexShader(m_vertexShader);
    m_device->SetPixelShader(m_pixelShader);
    m_device->SetVertexDeclaration(m_vertexDeclaration);

    Matrix4x4 proj = Matrix4x4::FromArray(projMatrix);
    Matrix4x4 viewProj = Matrix4x4::Multiply(view, proj);
    m_device->SetVertexShaderConstantF(0, &viewProj.m[0][0], 4);
    m_device->SetTexture(0, m_texture);

    // Create billboard quad using CPU billboarding (same as 3D particles)
    Vector3f center(worldPos[0], worldPos[1], worldPos[2]);
    float halfSize = size * 0.5f;

    // Calculate 4 corners expanded from center using camera vectors
    Vector3f bottomLeft = Vector3f(
        center.x + cameraRight.x * (-halfSize) + cameraUp.x * (-halfSize),
        center.y + cameraRight.y * (-halfSize) + cameraUp.y * (-halfSize),
        center.z + cameraRight.z * (-halfSize) + cameraUp.z * (-halfSize)
    );
    Vector3f bottomRight = Vector3f(
        center.x + cameraRight.x * halfSize + cameraUp.x * (-halfSize),
        center.y + cameraRight.y * halfSize + cameraUp.y * (-halfSize),
        center.z + cameraRight.z * halfSize + cameraUp.z * (-halfSize)
    );
    Vector3f topRight = Vector3f(
        center.x + cameraRight.x * halfSize + cameraUp.x * halfSize,
        center.y + cameraRight.y * halfSize + cameraUp.y * halfSize,
        center.z + cameraRight.z * halfSize + cameraUp.z * halfSize
    );
    Vector3f topLeft = Vector3f(
        center.x + cameraRight.x * (-halfSize) + cameraUp.x * halfSize,
        center.y + cameraRight.y * (-halfSize) + cameraUp.y * halfSize,
        center.z + cameraRight.z * (-halfSize) + cameraUp.z * halfSize
    );

    D3DCOLOR testColor = D3DCOLOR_RGBA(255, 0, 0, 255); // Bright red
    ParticleVertex quad[6];

    // Triangle 1
    quad[0] = {bottomLeft, testColor, Vector2f(size, 0), Vector2f(0, 1)};
    quad[1] = {bottomRight, testColor, Vector2f(size, 0), Vector2f(1, 1)};
    quad[2] = {topRight, testColor, Vector2f(size, 0), Vector2f(1, 0)};
    // Triangle 2
    quad[3] = {bottomLeft, testColor, Vector2f(size, 0), Vector2f(0, 1)};
    quad[4] = {topRight, testColor, Vector2f(size, 0), Vector2f(1, 0)};
    quad[5] = {topLeft, testColor, Vector2f(size, 0), Vector2f(0, 0)};

    // Lock and upload
    void* data = nullptr;
    HRESULT hr = m_vertexBuffer->Lock(0, 6 * sizeof(ParticleVertex), &data, D3DLOCK_DISCARD);

    if (SUCCEEDED(hr) && data) {
        memcpy(data, quad, 6 * sizeof(ParticleVertex));
        m_vertexBuffer->Unlock();

        m_device->SetStreamSource(0, m_vertexBuffer, 0, sizeof(ParticleVertex));
        m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
    }

    RestoreRenderStates();
}

void DX9ParticleRenderer::RenderTest3DSourceMatrices(const float* worldPos, float size) {
    static bool firstCall = true;
    if (firstCall) {
        LogToFile("[RenderTest3DSource] Using Source Engine's actual matrices!");
        firstCall = false;
    }

    if (!m_initialized || !m_device) {
        return;
    }

    // Get Source Engine's current view and projection matrices
    D3DMATRIX d3dView, d3dProj;
    m_device->GetTransform(D3DTS_VIEW, &d3dView);
    m_device->GetTransform(D3DTS_PROJECTION, &d3dProj);

    // Convert to our matrix type
    Matrix4x4 viewMatrix, projMatrix;
    memcpy(&viewMatrix, &d3dView, sizeof(D3DMATRIX));
    memcpy(&projMatrix, &d3dProj, sizeof(D3DMATRIX));

    // Log them once
    static bool logged = false;
    if (!logged) {
        char buf[512];
        sprintf(buf, "[RenderTest3DSource] Source View Matrix:\n"
                     "  [%.3f, %.3f, %.3f, %.3f]\n"
                     "  [%.3f, %.3f, %.3f, %.3f]\n"
                     "  [%.3f, %.3f, %.3f, %.3f]\n"
                     "  [%.3f, %.3f, %.3f, %.3f]",
                viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2], viewMatrix[0][3],
                viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2], viewMatrix[1][3],
                viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2], viewMatrix[2][3],
                viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2], viewMatrix[3][3]);
        LogToFile(buf);
        logged = true;
    }

    // Extract camera vectors from Source's view matrix
    Vector3f cameraRight(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    Vector3f cameraUp(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

    SetupRenderStates();

    // Set shaders
    m_device->SetVertexShader(m_vertexShader);
    m_device->SetPixelShader(m_pixelShader);
    m_device->SetVertexDeclaration(m_vertexDeclaration);

    // Combine view and projection
    Matrix4x4 viewProj = Matrix4x4::Multiply(viewMatrix, projMatrix);
    m_device->SetVertexShaderConstantF(0, &viewProj.m[0][0], 4);
    m_device->SetTexture(0, m_texture);

    // Create billboard using Source's camera vectors
    Vector3f center(worldPos[0], worldPos[1], worldPos[2]);
    float halfSize = size * 0.5f;

    Vector3f bottomLeft = Vector3f(
        center.x + cameraRight.x * (-halfSize) + cameraUp.x * (-halfSize),
        center.y + cameraRight.y * (-halfSize) + cameraUp.y * (-halfSize),
        center.z + cameraRight.z * (-halfSize) + cameraUp.z * (-halfSize)
    );
    Vector3f bottomRight = Vector3f(
        center.x + cameraRight.x * halfSize + cameraUp.x * (-halfSize),
        center.y + cameraRight.y * halfSize + cameraUp.y * (-halfSize),
        center.z + cameraRight.z * halfSize + cameraUp.z * (-halfSize)
    );
    Vector3f topRight = Vector3f(
        center.x + cameraRight.x * halfSize + cameraUp.x * halfSize,
        center.y + cameraRight.y * halfSize + cameraUp.y * halfSize,
        center.z + cameraRight.z * halfSize + cameraUp.z * halfSize
    );
    Vector3f topLeft = Vector3f(
        center.x + cameraRight.x * (-halfSize) + cameraUp.x * halfSize,
        center.y + cameraRight.y * (-halfSize) + cameraUp.y * halfSize,
        center.z + cameraRight.z * (-halfSize) + cameraUp.z * halfSize
    );

    D3DCOLOR testColor = D3DCOLOR_RGBA(255, 0, 0, 255);
    ParticleVertex quad[6];
    quad[0] = {bottomLeft, testColor, Vector2f(size, 0), Vector2f(0, 1)};
    quad[1] = {bottomRight, testColor, Vector2f(size, 0), Vector2f(1, 1)};
    quad[2] = {topRight, testColor, Vector2f(size, 0), Vector2f(1, 0)};
    quad[3] = {bottomLeft, testColor, Vector2f(size, 0), Vector2f(0, 1)};
    quad[4] = {topRight, testColor, Vector2f(size, 0), Vector2f(1, 0)};
    quad[5] = {topLeft, testColor, Vector2f(size, 0), Vector2f(0, 0)};

    void* data = nullptr;
    HRESULT hr = m_vertexBuffer->Lock(0, 6 * sizeof(ParticleVertex), &data, D3DLOCK_DISCARD);
    if (SUCCEEDED(hr) && data) {
        memcpy(data, quad, 6 * sizeof(ParticleVertex));
        m_vertexBuffer->Unlock();
        m_device->SetStreamSource(0, m_vertexBuffer, 0, sizeof(ParticleVertex));
        m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
    }

    RestoreRenderStates();
}

void DX9ParticleRenderer::RenderTest3DProjected(const float* worldPos, float size,
                                                 const float* viewMatrix, const float* projMatrix,
                                                 int screenWidth, int screenHeight) {
    static bool firstCall = true;
    if (firstCall) {
        LogToFile("[RenderTest3DProjected] Project 3D to screen, render as 2D!");
        firstCall = false;
    }

    if (!m_initialized || !m_device) {
        return;
    }

    // Project 3D world position to clip space
    Matrix4x4 view = Matrix4x4::FromArray(viewMatrix);
    Matrix4x4 proj = Matrix4x4::FromArray(projMatrix);
    Matrix4x4 viewProj = Matrix4x4::Multiply(view, proj);

    // Transform world position to clip space
    float worldX = worldPos[0];
    float worldY = worldPos[1];
    float worldZ = worldPos[2];

    float clipX = worldX * viewProj[0][0] + worldY * viewProj[1][0] + worldZ * viewProj[2][0] + viewProj[3][0];
    float clipY = worldX * viewProj[0][1] + worldY * viewProj[1][1] + worldZ * viewProj[2][1] + viewProj[3][1];
    float clipZ = worldX * viewProj[0][2] + worldY * viewProj[1][2] + worldZ * viewProj[2][2] + viewProj[3][2];
    float clipW = worldX * viewProj[0][3] + worldY * viewProj[1][3] + worldZ * viewProj[2][3] + viewProj[3][3];

    // Perspective divide
    if (clipW <= 0.0f) {
        // Behind camera, don't render
        return;
    }

    float ndcX = clipX / clipW;  // -1 to 1
    float ndcY = clipY / clipW;  // -1 to 1
    float ndcZ = clipZ / clipW;  // 0 to 1

    // Convert to screen space
    float screenX = (ndcX + 1.0f) * 0.5f * screenWidth;
    float screenY = (1.0f - ndcY) * 0.5f * screenHeight;  // Flip Y

    // Scale size based on distance (simple perspective scaling)
    // Make particles MUCH larger - multiply by screen height for proper scaling
    float screenSize = (size / clipW) * (screenHeight * 0.5f);  // Scale by half screen height

    // Log first projection (MORE DETAIL)
    static bool logged = false;
    if (!logged) {
        char buf[1024];
        sprintf(buf, "[RenderTest3DProjected] === PROJECTION DEBUG ===\n"
                     "World: (%.1f, %.1f, %.1f)\n"
                     "Clip: (%.3f, %.3f, %.3f, %.3f)\n"
                     "NDC: (%.3f, %.3f, %.3f)\n"
                     "Screen: (%.1f, %.1f) size=%.1f\n"
                     "ScreenRes: %dx%d",
                worldPos[0], worldPos[1], worldPos[2],
                clipX, clipY, clipZ, clipW,
                ndcX, ndcY, ndcZ,
                screenX, screenY, screenSize,
                screenWidth, screenHeight);
        LogToFile(buf);
        logged = true;
    }

    // Check if position is valid
    if (screenX < 0 || screenX > screenWidth || screenY < 0 || screenY > screenHeight) {
        char buf[256];
        sprintf(buf, "[RenderTest3DProjected] WARNING: Position off-screen! (%.1f, %.1f)", screenX, screenY);
        LogToFile(buf);
    }

    // Render using the working 2D approach!
    RenderTest2D(screenX, screenY, screenSize);
}

} // namespace GPUParticles
