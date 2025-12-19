// Particle Vertex Shader for DirectX 9
// Shader Model 2.0

// Vertex input
struct VS_INPUT
{
    float3 position : POSITION0;      // Particle center position
    float4 color : COLOR0;            // Particle color (RGBA)
    float2 sizeRot : TEXCOORD0;       // x = size, y = rotation (radians)
    float2 corner : TEXCOORD1;        // Corner offset (-1 to 1)
};

// Vertex output
struct VS_OUTPUT
{
    float4 position : POSITION0;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

// Constants
float4x4 viewProjection : register(c0);  // View-projection matrix
float3 cameraRight : register(c4);       // Camera right vector
float3 cameraUp : register(c5);          // Camera up vector

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // Extract size and rotation
    float size = input.sizeRot.x;
    float rotation = input.sizeRot.y;

    // Calculate corner offset in world space (billboarding)
    float2 corner = input.corner;

    // Apply rotation
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    float2 rotatedCorner;
    rotatedCorner.x = corner.x * cosR - corner.y * sinR;
    rotatedCorner.y = corner.x * sinR + corner.y * cosR;

    // Billboard position: face camera
    float3 worldPos = input.position;
    worldPos += cameraRight * rotatedCorner.x * size;
    worldPos += cameraUp * rotatedCorner.y * size;

    // Transform to clip space
    output.position = mul(float4(worldPos, 1.0), viewProjection);

    // Pass through color
    output.color = input.color;

    // Generate texture coordinates from corner
    output.texcoord = corner * 0.5 + 0.5;  // Convert from [-1,1] to [0,1]

    return output;
}
