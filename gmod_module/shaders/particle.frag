#version 330 core

// ============================================================================
// Particle Fragment Shader
// Renders individual pixels of particle billboards
// ============================================================================

in vec2 vTexCoord;
in vec4 vColor;
in float vDepth;

out vec4 fragColor;

// Textures
uniform sampler2D uParticleTexture;
uniform sampler2D uDepthTexture;  // Scene depth for soft particles

// Uniforms
uniform bool uUseSoftParticles;
uniform float uSoftParticleDistance;
uniform vec2 uScreenSize;
uniform int uBlendMode;  // 0=Alpha, 1=Additive, 2=Multiply

// Texture sheet animation
uniform bool uUseTextureAnimation;
uniform int uTilesX;
uniform int uTilesY;
uniform float uAnimationFrame;

void main() {
    vec2 texCoord = vTexCoord;

    // ========================================================================
    // Texture Sheet Animation
    // ========================================================================

    if (uUseTextureAnimation) {
        // Calculate which tile to use
        int frameIndex = int(uAnimationFrame) % (uTilesX * uTilesY);
        int tileX = frameIndex % uTilesX;
        int tileY = frameIndex / uTilesX;

        // Calculate tile UV offset
        float tileWidth = 1.0 / float(uTilesX);
        float tileHeight = 1.0 / float(uTilesY);

        texCoord = vec2(
            (float(tileX) + vTexCoord.x) * tileWidth,
            (float(tileY) + vTexCoord.y) * tileHeight
        );
    }

    // ========================================================================
    // Sample Particle Texture
    // ========================================================================

    vec4 texColor = texture(uParticleTexture, texCoord);

    // Apply vertex color
    vec4 particleColor = texColor * vColor;

    // Discard fully transparent pixels
    if (particleColor.a < 0.01) {
        discard;
    }

    // ========================================================================
    // Soft Particles
    // ========================================================================

    if (uUseSoftParticles) {
        // Calculate screen-space coordinates
        vec2 screenCoord = gl_FragCoord.xy / uScreenSize;

        // Sample scene depth
        float sceneDepth = texture(uDepthTexture, screenCoord).r;

        // Convert to linear depth (assuming depth is in [0,1] range)
        // This formula depends on your projection matrix setup
        float linearSceneDepth = sceneDepth * 100.0;  // Adjust multiplier as needed

        // Calculate depth difference
        float depthDiff = linearSceneDepth - vDepth;

        // Fade out when particle is close to scene geometry
        if (depthDiff < uSoftParticleDistance) {
            float fade = depthDiff / uSoftParticleDistance;
            fade = clamp(fade, 0.0, 1.0);
            particleColor.a *= fade;
        }
    }

    // ========================================================================
    // Blend Mode Application
    // ========================================================================

    if (uBlendMode == 1) {
        // Additive blending - multiply RGB by alpha for proper additive
        particleColor.rgb *= particleColor.a;
        particleColor.a = 1.0;
    } else if (uBlendMode == 2) {
        // Multiply blending
        particleColor.rgb = mix(vec3(1.0), particleColor.rgb, particleColor.a);
    }

    // ========================================================================
    // Output
    // ========================================================================

    fragColor = particleColor;
}
