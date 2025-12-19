// Particle Pixel Shader for DirectX 9
// Shader Model 2.0

// Pixel input
struct PS_INPUT
{
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

// Pixel output
struct PS_OUTPUT
{
    float4 color : COLOR0;
};

// Texture sampler
sampler2D particleTexture : register(s0);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;

    // Sample particle texture
    float4 texColor = tex2D(particleTexture, input.texcoord);

    // Multiply with vertex color (includes alpha)
    output.color = texColor * input.color;

    return output;
}
