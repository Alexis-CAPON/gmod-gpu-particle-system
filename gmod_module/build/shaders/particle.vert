#version 330 core

// ============================================================================
// Particle Vertex Shader
// Generates billboard quads for each particle
// Uses instancing - one instance per particle
// ============================================================================

layout(location = 0) in vec3 aVertexPosition;  // Quad vertex (-0.5 to 0.5)
layout(location = 1) in vec2 aTexCoord;        // Texture coordinates

// Per-instance attributes (from particle buffer)
layout(location = 2) in vec3 aParticlePosition;
layout(location = 3) in vec4 aParticleColor;
layout(location = 4) in float aParticleSize;
layout(location = 5) in float aParticleRotation;
layout(location = 6) in int aParticleAlive;

// Uniforms
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;

// Output to fragment shader
out vec2 vTexCoord;
out vec4 vColor;
out float vDepth;

void main() {
    // Skip dead particles
    if (aParticleAlive == 0) {
        gl_Position = vec4(0, 0, -10, 1);  // Off-screen
        return;
    }

    // Apply rotation to quad
    float cosRot = cos(aParticleRotation);
    float sinRot = sin(aParticleRotation);
    vec2 rotatedVertex = vec2(
        aVertexPosition.x * cosRot - aVertexPosition.y * sinRot,
        aVertexPosition.x * sinRot + aVertexPosition.y * cosRot
    );

    // Create billboard facing camera
    vec3 billboardPos = aParticlePosition
                       + uCameraRight * rotatedVertex.x * aParticleSize
                       + uCameraUp * rotatedVertex.y * aParticleSize;

    // Transform to clip space
    vec4 viewPos = uViewMatrix * vec4(billboardPos, 1.0);
    gl_Position = uProjectionMatrix * viewPos;

    // Pass data to fragment shader
    vTexCoord = aTexCoord;
    vColor = aParticleColor;
    vDepth = -viewPos.z;  // Distance from camera (for soft particles)
}
