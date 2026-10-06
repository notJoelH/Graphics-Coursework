#define MAX_LIGHTS 6

Texture2D heightMap : register(t0);
SamplerState sampler0 : register(s0);

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightView[MAX_LIGHTS];   // Make Height Shader multi-light capable
    matrix lightProjection[MAX_LIGHTS];
    int numLights;
    float3 padding;
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPos;
    float camPad;
};

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
    float4 lightViewPos[MAX_LIGHTS] : TEXCOORD3;
};

// Declare constants (must match PS)
static const float HEIGHT_SCALE = 100.0f;

// Get height from heightmap
float GetHeight(float2 uv)
{
    float rawHeight = heightMap.SampleLevel(sampler0, uv, 0).r;
    rawHeight = pow(rawHeight, 2.0f); // Exaggerate height differences bc map is bright)
    return rawHeight * HEIGHT_SCALE;
}

OutputType main(InputType input)
{
    OutputType output;
	
    // Displace vertex by heightmap
    input.position.y += GetHeight(input.tex);

    // Transform to world space
    float4 worldPos = mul(input.position, worldMatrix);
    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Pass-through and per-vertex outputs
    output.tex = input.tex;
    output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    output.worldPos = worldPos.xyz;
    output.viewVector = normalize(cameraPos - worldPos.xyz);

    // Per-light shadow coordinates
    for (int i = 0; i < numLights; i++)
    {
        float4 lsPos = mul(worldPos, lightView[i]);
        lsPos = mul(lsPos, lightProjection[i]);
        output.lightViewPos[i] = lsPos;
    }

    return output;
}